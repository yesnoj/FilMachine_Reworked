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

/* FilMachine.h pulls in board.h which defines DISPLAY_DRIVER_xxx
 * and TOUCH_DRIVER_xxx — must come BEFORE the conditional includes. */
#include "FilMachine.h"

/* ═══════════════════════════════════════════════
 * Board-specific display and touch driver includes
 * ═══════════════════════════════════════════════ */
#if defined(DISPLAY_DRIVER_ILI9488)
    #include "esp_lcd_ili9488.h"
#elif defined(DISPLAY_DRIVER_NV3041A)
    #include "esp_lcd_nv3041.h"
    #include "driver/spi_master.h"
#elif defined(DISPLAY_DRIVER_ST7701)
    #include "st7701_lcd.h"
    #include "ppa_engine.h"
    #include "esp_heap_caps.h"
#else
    #error "No display driver defined — check board.h"
#endif

#if defined(TOUCH_DRIVER_FT6236)
    #include "esp_lcd_touch_ft5x06.h"
#elif defined(TOUCH_DRIVER_GT911)
    #include "esp_lcd_touch_gt911.h"
#else
    #error "No touch driver defined — check board.h"
#endif

/* Global Stuff */
struct gui_components gui;
struct sys_components sys;

static const char *TAG = "FilMachine"; /* ESP Debug Message Tag */
bool stopMotorManTask = false;
uint8_t initErrors = 0;

#if defined(DISPLAY_DRIVER_ST7701)
/* ── P4: PPA-based flush needs PSRAM buffers (double-buffered, full-frame) ── */
static uint8_t *lvgl_buf1 = NULL;
static uint8_t *lvgl_buf2 = NULL;

/* PPA output buffer for rotated+scaled frame */
#define PPA_BUF_ALIGN    64
#define PPA_OUT_PIXELS   (LCD_PHYS_H_RES * LCD_PHYS_V_RES)
#define PPA_OUT_BYTES    (PPA_OUT_PIXELS * BYTES_PER_PIXEL)
#define PPA_OUT_ALIGNED  ((PPA_OUT_BYTES + PPA_BUF_ALIGN - 1) & ~(PPA_BUF_ALIGN - 1))
static void *ppa_out_buf = NULL;
#else
/* ── S3/Simulator: static draw buffer, partial rendering ── */
static uint8_t lvgl_buf[LVGL_BUF_SIZE];
#endif

/* ═══════════════════════════════════════════════
 * LVGL integration functions — board-specific flush & tick
 * ═══════════════════════════════════════════════ */

#if defined(DISPLAY_DRIVER_ST7701)
/* ── P4: PPA rotate+scale flush ──
 * LVGL renders 480×320 (full frame).  PPA rotates 90° and scales ×1.5
 * producing 480×720 centred on the 480×800 physical panel (40 px border top/bottom).
 */
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (!ppa_out_buf) {
        ppa_out_buf = heap_caps_aligned_calloc(
            PPA_BUF_ALIGN, 1, PPA_OUT_ALIGNED,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
        if (!ppa_out_buf) {
            ESP_LOGE(TAG, "PPA output buffer alloc failed (%d bytes)", PPA_OUT_ALIGNED);
            lv_display_flush_ready(disp);
            return;
        }
    }

    uint32_t out_w = 0, out_h = 0;
    esp_err_t ret = ppa_rotate_scale_rgb565_to(
        (const uint16_t *)px_map,
        LCD_H_RES, LCD_V_RES,          /* 480×320 LVGL input */
        PPA_LANDSCAPE_ROTATION,         /* 90° rotation */
        PPA_LANDSCAPE_SCALE,            /* ×1.5 horizontal */
        PPA_LANDSCAPE_SCALE,            /* ×1.5 vertical */
        ppa_out_buf, PPA_OUT_ALIGNED,
        &out_w, &out_h,
        false);                         /* rgb_swap = false */

    if (ret == ESP_OK && out_w > 0 && out_h > 0) {
        /* Centre the scaled output on the physical 480×800 panel */
        uint16_t x_off = (LCD_PHYS_H_RES - (uint16_t)out_w) / 2;
        uint16_t y_off = (LCD_PHYS_V_RES - (uint16_t)out_h) / 2;
        st7701_lcd_draw_rgb_bitmap(x_off, y_off, out_w, out_h,
                                   (const uint16_t *)ppa_out_buf);
    } else {
        ESP_LOGE(TAG, "PPA rotate+scale failed (0x%x), out=%lux%lu", ret, out_w, out_h);
    }

    lv_display_flush_ready(disp);
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
         * Physical GT911 reports in 480×800 portrait coordinates.
         * PPA rotates the 480×320 LVGL buffer 90° → 320×480, then scales ×1.5 → 480×720.
         * This 480×720 image is centred on the 480×800 panel with 40 px borders top/bottom.
         *
         * On-screen output after rotation+scale:
         *   out_w  = LCD_V_RES × scale = 320×1.5 = 480  (fills physical width)
         *   out_h  = LCD_H_RES × scale = 480×1.5 = 720  (centred in physical 800)
         *
         * Inverse mapping (physical touch → LVGL 480×320):
         *   phys_y runs along the on-screen height (720 px) → maps to lvgl_x (0..479)
         *   phys_x runs along the on-screen width  (480 px) → maps to lvgl_y (0..319, inverted)
         *
         * May need fine-tuning on real hardware (rotation direction, axis inversion).
         */
        int32_t phys_x = touchpad_x[0];
        int32_t phys_y = touchpad_y[0];
        int32_t out_w  = (int32_t)(LCD_V_RES * PPA_LANDSCAPE_SCALE);   /* 320×1.5 = 480 */
        int32_t out_h  = (int32_t)(LCD_H_RES * PPA_LANDSCAPE_SCALE);   /* 480×1.5 = 720 */
        int32_t x_off  = (LCD_PHYS_H_RES - out_w) / 2;                /* (480-480)/2 = 0 */
        int32_t y_off  = (LCD_PHYS_V_RES - out_h) / 2;                /* (800-720)/2 = 40 */

        data->point.x = (phys_y - y_off) * LCD_H_RES / out_h;         /* phys_y [40..759] → lvgl_x [0..479] */
        data->point.y = (LCD_PHYS_H_RES - x_off - phys_x) * LCD_V_RES / out_w; /* phys_x [0..479] → lvgl_y [319..0] */

        /* Clamp to LVGL resolution */
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

#if defined(DISPLAY_DRIVER_ILI9488)
/* ── Makerfabs: ILI9488 16-bit parallel (I80) ── */
static esp_lcd_panel_handle_t init_display(lv_display_t *our_display)
{
    ESP_LOGI(TAG, "Initialise LCD bus — ILI9488 16-bit parallel");
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_PLL240M,
        .dc_gpio_num = LCD_RS,
        .wr_gpio_num = LCD_WR,
        .data_gpio_nums = {
            LCD_D0,  LCD_D1,  LCD_D2,  LCD_D3,
            LCD_D4,  LCD_D5,  LCD_D6,  LCD_D7,
            LCD_D8,  LCD_D9,  LCD_D10, LCD_D11,
            LCD_D12, LCD_D13, LCD_D14, LCD_D15,
        },
        .bus_width = 16,
        .max_transfer_bytes = LVGL_BUF_SIZE
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = our_display,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install ILI9488 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(io_handle, &panel_config, 0, &panel_handle));

    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, false);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);

    return panel_handle;
}

#elif defined(DISPLAY_DRIVER_NV3041A)
/* ── JC4827W543: NV3041A QSPI ── */
static esp_lcd_panel_handle_t init_display(lv_display_t *our_display)
{
    ESP_LOGI(TAG, "Initialise LCD bus — NV3041A QSPI");

    /* QSPI bus: 4 data lines + clock */
    spi_bus_config_t bus_config = {
        .sclk_io_num     = LCD_CLK_PIN,
        .data0_io_num    = LCD_D0_PIN,
        .data1_io_num    = LCD_D1_PIN,
        .data2_io_num    = LCD_D2_PIN,
        .data3_io_num    = LCD_D3_PIN,
        .max_transfer_sz = LVGL_BUF_SIZE,
        .flags           = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_QUAD,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num         = LCD_CS_PIN,
        .dc_gpio_num         = -1,      /* QSPI: no DC pin — cmd/data in address phase */
        .spi_mode            = 0,
        .pclk_hz             = 40 * 1000 * 1000,
        .trans_queue_depth   = 10,
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx            = our_display,
        .lcd_cmd_bits        = 32,
        .lcd_param_bits      = 8,
        .flags = {
            .quad_mode = true,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install NV3041A panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,           /* No RST pin on JC4827W543 */
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_nv3041(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    return panel_handle;
}

#elif defined(DISPLAY_DRIVER_ST7701)
/* ── JC4880P433: ST7701S MIPI-DSI + PPA rotation/scale ──
 *
 * Unlike I80/QSPI boards, MIPI-DSI has a hardware framebuffer — we don't use
 * esp_lcd_panel_draw_bitmap().  Instead the PPA-based flush callback writes
 * directly to the ST7701 framebuffer via st7701_lcd_draw_rgb_bitmap().
 *
 * init_display() initialises the PPA engine and the MIPI-DSI LCD, then
 * clears the screen to black before the backlight comes on.
 * Returns NULL because the P4 flush callback doesn't use a panel handle.
 */
static esp_lcd_panel_handle_t init_display(lv_display_t *our_display)
{
    ESP_LOGI(TAG, "Initialise display — ST7701S MIPI-DSI (%dx%d phys, %dx%d LVGL landscape)",
             LCD_PHYS_H_RES, LCD_PHYS_V_RES, LCD_H_RES, LCD_V_RES);

    /* 1. PPA engine — needed for rotation + scale in the flush callback */
    ESP_ERROR_CHECK(ppa_engine_init());
    ESP_LOGI(TAG, "PPA engine initialised");

    /* 2. ST7701 LCD via MIPI-DSI */
    ESP_ERROR_CHECK(st7701_lcd_init());
    ESP_LOGI(TAG, "ST7701 MIPI-DSI LCD initialised (%dx%d)", LCD_PHYS_H_RES, LCD_PHYS_V_RES);

    /* 3. Clear screen to black before backlight on (prevents white flash) */
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

#if defined(TOUCH_DRIVER_FT6236)
    /* ── Makerfabs: FT6236 (FT5x06 family) on shared I2C bus ── */
    ESP_LOGI(TAG, "Initialize touch controller FT6236 (FT5x06 driver)");
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_V_RES,
        .y_max = LCD_H_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy  = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));

#elif defined(TOUCH_DRIVER_GT911)
    /* ── GT911 on shared I2C bus ── */
    ESP_LOGI(TAG, "Initialize touch controller GT911");
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM, &tp_io_config, &tp_io_handle));

#if defined(TOUCH_DRIVER_GT911_P4)
    /* P4: Physical panel is 480×800 portrait.  Coordinate remapping to
     * 480×320 landscape is done in lvgl_touch_cb(), so we report raw
     * physical coords here (no swap, no mirror). */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_PHYS_H_RES,   /* 480 — physical width */
        .y_max = LCD_PHYS_V_RES,   /* 800 — physical height */
        .rst_gpio_num = TOUCH_RST_PIN,
        .int_gpio_num = TOUCH_INT_PIN,
        .flags = {
            .swap_xy  = 0,  /* Raw coords — remapping in touch callback */
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
#else
    /* Default GT911: Native landscape — no remapping needed */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = TOUCH_RST_PIN,
        .int_gpio_num = TOUCH_INT_PIN,
        .flags = {
            .swap_xy  = 0,  /* Native landscape — no swap needed */
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
#endif
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp));
#endif

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
writeConfigFile( FILENAME_SAVE, false);
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
    our_display = lv_display_create( LCD_H_RES, LCD_V_RES );

    /* Board-specific display init */
    esp_lcd_panel_handle_t panel_handle = init_display(our_display);

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(LCD_BLK, LCD_BK_LIGHT_ON_LEVEL);

    /* Board-specific touch init */
    esp_lcd_touch_handle_t tp = init_touch();

#if defined(DISPLAY_DRIVER_ST7701)
    /* P4: Double-buffered FULL rendering in PSRAM.
     * Full-frame mode is required because the PPA flush rotates+scales
     * the entire frame — partial dirty rects cannot be rotated individually.
     * With 32 MB PSRAM the two 480×320 buffers (600 KB total) are trivial. */
    size_t buf_size = LCD_H_RES * LCD_V_RES * BYTES_PER_PIXEL;
    lvgl_buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    lvgl_buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    if (!lvgl_buf1 || !lvgl_buf2) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffers in PSRAM (%d bytes each)", buf_size);
    }
    lv_display_set_buffers(our_display, lvgl_buf1, lvgl_buf2, buf_size,
                           LV_DISPLAY_RENDER_MODE_FULL);
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
    xTaskCreatePinnedToCore( sysMan, "sysMan", 4096, NULL, 8,  NULL, tskNO_AFFINITY ); // This can run on any free core

    ESP_LOGI(TAG, "Start LVGL on %s (%dx%d)", BOARD_NAME, LCD_H_RES, LCD_V_RES);
    /* LVGL GUI creation code is called here! */
create_keyboard();

    /* Pre-load settings so splash knows if random mode is enabled */
    readSettingsOnly(FILENAME_SAVE);

    /* Show splash screen — play button calls readConfigFile → menu → refreshSettingsUI */
    lv_obj_t * splash = splash_screen_create();
    lv_scr_load(splash);

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(5));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}
