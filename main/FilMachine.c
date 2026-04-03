#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "page_splash.h"
#include "esp_timer.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* FilMachine.h pulls in board.h which defines DISPLAY_DRIVER_xxx
 * and TOUCH_DRIVER_xxx — must come BEFORE the conditional includes. */
#include "FilMachine.h"

/* ═══════════════════════════════════════════════
 * Board-specific display and touch driver includes
 * ═══════════════════════════════════════════════ */
#if defined(DISPLAY_DRIVER_ST7701)
    #include "st7701_lcd.h"
    #include "ppa_engine.h"
    #include "esp_heap_caps.h"
#elif !defined(DISPLAY_DRIVER_SDL)
    #error "No display driver defined — check board.h"
#endif

#if defined(TOUCH_DRIVER_GT911)
    #include "esp_lcd_touch_gt911.h"
#elif !defined(DISPLAY_DRIVER_SDL)
    #error "No touch driver defined — check board.h"
#endif

/* Global Stuff */
struct gui_components gui;
struct sys_components sys;

static const char *TAG = "FilMachine"; /* ESP Debug Message Tag */
bool stopMotorManTask = false;
uint8_t initErrors = 0;

#if defined(DISPLAY_DRIVER_ST7701)
/* ── P4: LVGL partial rendering with double-buffered PSRAM draw buffers ── */
static uint8_t *lvgl_buf1 = NULL;
static uint8_t *lvgl_buf2 = NULL;

/* ── Dual-core async flush: core 0 renders, core 1 flushes via PPA ── */
typedef struct {
    lv_display_t *disp;
    uint16_t x1, y1, w, h;
    const uint16_t *data;
    bool is_last_flush;
} flush_job_t;

static QueueHandle_t      s_flush_queue = NULL;    /* core 0 → core 1 */
static lv_display_t      *s_flush_disp  = NULL;    /* cached display handle for flush_ready */

/* FPS tracking (updated from core 1) */
static volatile int64_t s_fps_window_start = 0;
static volatile int     s_fps_frame_count  = 0;
static volatile int     s_fps_flush_count  = 0;

static void flush_task(void *arg)
{
    flush_job_t job;
    ESP_LOGI("FLUSH", "Flush task started on core %d", xPortGetCoreID());

    while (1) {
        /* Block until core 0 posts a flush job */
        if (xQueueReceive(s_flush_queue, &job, portMAX_DELAY) != pdTRUE) continue;

        /* PPA rotate + write directly to DPI framebuffer */
        st7701_lcd_draw_to_fb_rotated(job.x1, job.y1, job.w, job.h, job.data);

        /* FPS tracking */
        s_fps_flush_count++;
        if (job.is_last_flush) {
            s_fps_frame_count++;
            int64_t now = esp_timer_get_time();
            if (s_fps_window_start == 0) s_fps_window_start = now;
            int64_t elapsed_us = now - s_fps_window_start;
            if (elapsed_us >= 2000000) {
                float fps = (float)s_fps_frame_count * 1000000.0f / (float)elapsed_us;
                ESP_LOGI("FPS", "%.1f FPS  (%d frames, %d flushes in %lld ms)",
                         fps, s_fps_frame_count, s_fps_flush_count, elapsed_us / 1000);
                s_fps_frame_count = 0;
                s_fps_flush_count = 0;
                s_fps_window_start = now;
            }
        }

        /* Tell LVGL the buffer is free — it can start rendering into it */
        lv_display_flush_ready(job.disp);
    }
}
#else
/* ── S3/Simulator: static draw buffer, partial rendering ── */
static uint8_t lvgl_buf[LVGL_BUF_SIZE];
#endif

/* ═══════════════════════════════════════════════
 * LVGL integration functions — board-specific flush & tick
 * ═══════════════════════════════════════════════ */

#if defined(DISPLAY_DRIVER_ST7701)
/* ── P4: Dual-core async flush.
 * Core 0 (LVGL) posts flush jobs to a queue.
 * Core 1 (flush_task) picks them up, runs PPA rotation, and signals flush_ready.
 * This overlaps LVGL rendering with PPA+cache_sync for ~2× throughput.
 */
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    flush_job_t job = {
        .disp = disp,
        .x1   = area->x1,
        .y1   = area->y1,
        .w    = (uint16_t)(area->x2 - area->x1 + 1),
        .h    = (uint16_t)(area->y2 - area->y1 + 1),
        .data = (const uint16_t *)px_map,
        .is_last_flush = lv_display_flush_is_last(disp),
    };

    /* Post to core 1 — returns immediately so LVGL can render into the other buffer.
     * portMAX_DELAY is safe because core 1 processes jobs fast (~7ms). */
    xQueueSend(s_flush_queue, &job, portMAX_DELAY);
    /* NOTE: we do NOT call lv_display_flush_ready() here.
     * Core 1's flush_task calls it after PPA completes. */
}

#else
/* ── S3: standard esp_lcd_panel flush via bus (I80 / QSPI) ── */
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {

    lv_display_t *display = (lv_display_t *)user_ctx;
    lv_disp_flush_ready(display);
    return false;
}

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {

    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}
#endif /* DISPLAY_DRIVER_ST7701 */

static void increase_lvgl_tick(void *arg) {

    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_touch_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    /* Read touch controller data */
    esp_lcd_touch_handle_t tp = lv_indev_get_user_data(indev);
    esp_lcd_touch_read_data(tp);

    /* Get coordinates — still using deprecated API until esp_lcd_touch 2.0 migration */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);
#pragma GCC diagnostic pop

    if (touchpad_pressed && touchpad_cnt > 0) {
#if defined(TOUCH_DRIVER_GT911_P4)
        /*
         * P4 landscape touch remapping:
         * GT911 reports in physical 480×800 portrait coordinates.
         * LVGL display is 800×480 landscape.  The flush callback handles
         * the rotation to the physical DPI framebuffer.
         *
         * Mapping (physical GT911 → LVGL logical 800×480):
         *   lvgl_x = (799 - phys_y)   → phys_y [0..799] maps to x [799..0]
         *   lvgl_y = phys_x            → phys_x [0..479] maps to y [0..479]
         */
        int32_t phys_x = touchpad_x[0];
        int32_t phys_y = touchpad_y[0];

        data->point.x = (LCD_PHYS_V_RES - 1) - phys_y;   /* [0..799] */
        data->point.y = phys_x;                            /* [0..479] */

        /* Clamp to LVGL screen resolution */
        if (data->point.x < 0) data->point.x = 0;
        if (data->point.y < 0) data->point.y = 0;
        if (data->point.x >= LCD_H_RES) data->point.x = LCD_H_RES - 1;
        if (data->point.y >= LCD_V_RES) data->point.y = LCD_V_RES - 1;
#else
        data->point.x =  touchpad_x[0] >= LCD_H_RES ? LCD_H_RES-1 : touchpad_x[0];
        data->point.y = touchpad_y[0] >= LCD_V_RES ? LCD_V_RES-1 : touchpad_y[0];
#endif
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* ═══════════════════════════════════════════════
 * Display initialisation — board-specific
 * ═══════════════════════════════════════════════ */

#if defined(DISPLAY_DRIVER_ST7701)
/* ── JC4880P433: ST7701S MIPI-DSI (PPA hardware rotation) ──
 *
 * Unlike I80/QSPI boards, MIPI-DSI has a hardware DPI framebuffer.
 * LVGL works in 800×480 landscape; the PPA hardware engine rotates
 * each dirty rect to 480×800 portrait and writes directly to the
 * DPI framebuffer — no intermediate buffers needed.
 *
 * Returns NULL because the P4 flush callback doesn't use a panel handle.
 */
static esp_lcd_panel_handle_t init_display(lv_display_t *our_display)
{
    ESP_LOGI(TAG, "Initialise display — ST7701S MIPI-DSI (%dx%d phys, %dx%d LVGL landscape)",
             LCD_PHYS_H_RES, LCD_PHYS_V_RES, LCD_H_RES, LCD_V_RES);

    /* PPA engine — kept initialised for potential future use */
    ESP_ERROR_CHECK(ppa_engine_init());
    ESP_LOGI(TAG, "PPA engine initialised");

    /* ST7701 LCD via MIPI-DSI */
    ESP_ERROR_CHECK(st7701_lcd_init());
    ESP_LOGI(TAG, "ST7701 MIPI-DSI LCD initialised (%dx%d)", LCD_PHYS_H_RES, LCD_PHYS_V_RES);

    /* Clear screen to black before backlight on (prevents white flash) */
    st7701_lcd_fill_screen(0x0000);

    /* P4 flush does not use esp_lcd_panel_handle_t — return NULL */
    (void)our_display;
    return NULL;
}
#endif /* DISPLAY_DRIVER */

/* ═══════════════════════════════════════════════
 * Touch initialisation — board-specific
 * ═══════════════════════════════════════════════ */
static esp_lcd_touch_handle_t init_touch(void)
{
    esp_lcd_touch_handle_t tp = NULL;
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    /* ── GT911 on I2C bus ── */
    ESP_LOGI(TAG, "Initialize touch controller GT911");
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = 100000;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(g_i2c_bus_handle, &tp_io_config, &tp_io_handle));

    /* Physical panel is 480×800 portrait. Coordinate remapping to
     * 800×480 landscape is done in lvgl_touch_cb(), so we report raw
     * physical coords here (no swap, no mirror). */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_PHYS_H_RES,   /* 480 — physical width */
        .y_max = LCD_PHYS_V_RES,   /* 800 — physical height */
        .rst_gpio_num = TOUCH_RST_PIN,
        .int_gpio_num = TOUCH_INT_PIN,
        .flags = {
            .swap_xy  = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp));

    return tp;
}

/* Tasks Stuff Here... */
#if FILM_USE_LOG != 0
#define MEM_MSG_DISPLAY_TIME 20000
#else
#define MEM_MSG_DISPLAY_TIME portMAX_DELAY
#endif

static void sysMan( void *arg ) {

uint16_t  msg;

while(1) {  // This is a task which runs for ever
    /* This will time out after MEM_MSG_DISPLAY_TIME and print memory then wait again */
if( xQueueReceive( sys.sysActionQ, &msg, pdMS_TO_TICKS(MEM_MSG_DISPLAY_TIME) ) ) {
switch(msg) {

case SAVE_PROCESS_CONFIG:
ESP_LOGW("sysMan", "[sysMan] SAVE_PROCESS_CONFIG received — calling writeConfigFile");
writeConfigFile( FILENAME_SAVE, false);
ESP_LOGW("sysMan", "[sysMan] writeConfigFile completed");
break;

      // Add Further processor intensive tasks here to keep them out of the GUI execution path
case SAVE_MACHINE_STATS:
        LV_LOG_USER("Save On EEPROM!");
          writeMachineStats(&gui.page.tools.machineStats);
          break;

case EXPORT_CFG:
LV_LOG_USER("Backup FilMachine.cfg to FilMachine_Backup.cfg.");
if( !copyAndRenameFile( FILENAME_SAVE, FILENAME_BACKUP) ) {
LV_LOG_USER("Backup failed!" );
} else LV_LOG_USER("Backup successful!" );
break;

case RELOAD_CFG:
          LV_LOG_USER("Reload FilMachine.cfg from backup");
          if( copyAndRenameFile( FILENAME_BACKUP, FILENAME_SAVE) ) rebootBoard();
break;

      default:
          LV_LOG_USER( "Unknown System Manager Request!");
          break;
      }
    } else {
#if FILM_USE_LOG != 0
      LV_LOG_USER("\nTotal Free Heap:    %u bytes\n"
        "  SPIRAM Free Heap    %7zu bytes\n"
        "  INTERNAL Free Heap  %7zu bytes\n",
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM) + heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL)
      );
#endif
}
}
}

void motorMan(void *arg) {
    int8_t rotation = 1;
    int8_t prevRotation = rotation;
    uint16_t msg;

    while(!stopMotorManTask) {  // Questo è un task che gira per sempre
        TickType_t interval = pdMS_TO_TICKS(1000); // Intervallo di default

        if (xQueueReceive(sys.motorActionQ, &msg, interval)) {
            // Aggiungi qui le cose da fare ogni intervallo
        } else {
            switch(rotation) {
                case 1:
                    runMotorFW(MOTOR_IN1_PIN, MOTOR_IN2_PIN);
                    prevRotation = 1;
                    rotation = 0;
                    interval = pdMS_TO_TICKS(getRandomRotationInterval() * 1000); // Imposta l'intervallo per runMotorFW
                    break;
                case -1:
                    runMotorRV(MOTOR_IN1_PIN, MOTOR_IN2_PIN);
                    prevRotation = -1;
                    rotation = 0;
                    interval = pdMS_TO_TICKS(getRandomRotationInterval() * 1000); // Imposta l'intervallo per runMotorRV
                    break;
                case 0:
                default:
                    stopMotor(MOTOR_IN1_PIN, MOTOR_IN2_PIN);
                    if (prevRotation == 1) {
                        rotation = -1;
                    } else if (prevRotation == -1) {
                        rotation = 1;
                    } else {
                        rotation = 1; // Valore iniziale se prevRotation non è 1 o -1
                    }
                    prevRotation = 0;
                    interval = pdMS_TO_TICKS(1000); // Intervallo per stopMotor
                    break;
            }
        }

        // Aggiungi un ritardo per rispettare l'intervallo calcolato
        vTaskDelay(interval);
    }

    // Codice per pulire e fermare il task
    stopMotor(MOTOR_IN1_PIN, MOTOR_IN2_PIN);
    vTaskDelete(NULL); // Termina il task
}

void stopMotorTask() {
    stopMotorManTask = true;
}

void runMotorTask() {
    xTaskCreatePinnedToCore( motorMan, "motorMan", 4096, NULL, 8,  NULL, tskNO_AFFINITY); // This can run on any free core
    stopMotorManTask = false;
}

void app_main( void ) {

lv_display_t *our_display;

    initGlobals();
init_Pins_and_Buses();

ESP_LOGI(TAG, "Initialise LVGL library");
    lv_init();
#if defined(DISPLAY_DRIVER_ST7701)
    /* P4: LVGL in 800×480 landscape, PPA handles rotation to physical 480×800 */
    our_display = lv_display_create( LCD_H_RES, LCD_V_RES );
    ESP_LOGI(TAG, "Display: %dx%d logical (PPA rotates to %dx%d physical)",
             LCD_H_RES, LCD_V_RES, LCD_PHYS_H_RES, LCD_PHYS_V_RES);
#else
    our_display = lv_display_create( LCD_H_RES, LCD_V_RES );
#endif

    /* Board-specific display init */
    esp_lcd_panel_handle_t panel_handle = init_display(our_display);

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(LCD_BLK, LCD_BK_LIGHT_ON_LEVEL);

    /* Board-specific touch init */
    esp_lcd_touch_handle_t tp = init_touch();

#if defined(DISPLAY_DRIVER_ST7701)
    /* P4: Double-buffered partial rendering with small PSRAM buffers.
     *
     * Dual-core async flush: core 0 renders, core 1 flushes via PPA.
     * Buffer = 1/15 of screen (800×32 = 25600 px = 51200 bytes).
     * Compromise between flush speed and visual tearing:
     *   1/10 (76KB): 4ms/flush, 13-20 FPS, no flicker
     *   1/15 (51KB): ~3ms/flush target, less tearing than 1/20
     *   1/20 (38KB): 2ms/flush, 14-21 FPS, flicker at bottom
     *
     * NOTE: Internal SRAM causes freezes during step scroll —
     * the step rendering needs internal DMA memory for widgets.
     * PSRAM-only is the only stable path.
     */
    size_t buf_size = LCD_H_RES * (LCD_V_RES / 15) * BYTES_PER_PIXEL;
    lvgl_buf1 = heap_caps_aligned_alloc(64, buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    lvgl_buf2 = heap_caps_aligned_alloc(64, buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (!lvgl_buf1 || !lvgl_buf2) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffers (%d bytes each)", (int)buf_size);
    }
    ESP_LOGI(TAG, "LVGL buffers: %d bytes each (PSRAM, 1/15 screen, DMA-aligned)", (int)buf_size);
    lv_display_set_buffers(our_display, lvgl_buf1, lvgl_buf2, buf_size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* ── Dual-core async flush: create queue + task on core 1 ── */
    s_flush_queue = xQueueCreate(2, sizeof(flush_job_t));
    assert(s_flush_queue != NULL);
    s_flush_disp = our_display;

    /* Pin flush task to core 1, priority 10 (higher than LVGL on core 0).
     * Stack: 4KB is enough — flush_task just posts PPA jobs + cache sync. */
    xTaskCreatePinnedToCore(flush_task, "flush_ppa", 4096, NULL, 10, NULL, 1);
    ESP_LOGI(TAG, "Dual-core async flush: LVGL on core 0, PPA flush on core 1");
#else
    // initialize LVGL draw buffers (S3: single buffer, partial rendering)
    lv_display_set_buffers( our_display, lvgl_buf, NULL, LVGL_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data( our_display, panel_handle );
#endif

    lv_display_set_flush_cb( our_display,  disp_flush );

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    lv_indev_t *indev = lv_indev_create();
lv_indev_set_type( indev, LV_INDEV_TYPE_POINTER );
lv_indev_set_read_cb( indev, lvgl_touch_cb ); /*This function will be called periodically (by the library) to get the mouse position and state*/
    lv_indev_set_user_data( indev, tp );

    /* Create System message queue */
    sys.sysActionQ = xQueueCreate( 16, sizeof( uint16_t ) );
    sys.motorActionQ = xQueueCreate( 16, sizeof( uint16_t ) );
    /* Create task to process external functions which will slow the GUI response */
    xTaskCreatePinnedToCore( sysMan, "sysMan", 16384, NULL, 8,  NULL, tskNO_AFFINITY ); // 16KB — FatFS LFN internals use deep stack in f_write/f_lseek

    ESP_LOGI(TAG, "Start LVGL on %s (%dx%d)", BOARD_NAME, LCD_H_RES, LCD_V_RES);
    /* LVGL GUI creation code is called here! */
create_keyboard();

    /* Pre-load settings so splash knows if random mode is enabled */
    readSettingsOnly(FILENAME_SAVE);

    /* Show splash screen — play button calls readConfigFile → menu → refreshSettingsUI */
    lv_obj_t * splash = splash_screen_create();
    lv_scr_load(splash);

    while (1) {
        // lv_timer_handler returns ms until next call is needed
        uint32_t time_till_next = lv_timer_handler();
        if (time_till_next < 1) time_till_next = 1;
        if (time_till_next > 50) time_till_next = 50;
        vTaskDelay(pdMS_TO_TICKS(time_till_next));
    }
}
