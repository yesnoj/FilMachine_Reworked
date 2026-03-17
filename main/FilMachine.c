#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "esp_timer.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9488.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "FilMachine.h"

/* Global Stuff */
struct gui_components gui;
struct sys_components sys;

static const char *TAG = "FilMachine"; /* ESP Debug Message Tag */
static uint8_t lvgl_buf[LVGL_BUF_SIZE]; /* Draw buffer used by LVGL */
bool stopMotorManTask = false;
uint8_t initErrors = 0;

/* LVGL integration functions */
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

    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x =  touchpad_x[0] >= LCD_H_RES ? LCD_H_RES-1 : touchpad_x[0]; // The touch driver returns values higher than the resolution sometimes so this stops LVGL errors by limiting the values
        data->point.y = touchpad_y[0] >= LCD_V_RES ? LCD_V_RES-1 : touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
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

    ESP_LOGI(TAG, "Initialise LCD bus");
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_PLL240M,
        .dc_gpio_num = LCD_RS,
        .wr_gpio_num = LCD_WR,
        .data_gpio_nums = {
            LCD_D0,
            LCD_D1,
            LCD_D2,
            LCD_D3,
            LCD_D4,
            LCD_D5,
            LCD_D6,
            LCD_D7,
            LCD_D8,
            LCD_D9,
            LCD_D10,
            LCD_D11,
            LCD_D12,
            LCD_D13,
            LCD_D14,
            LCD_D15,
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

    ESP_LOGI(TAG, "Install driver for ILI9488 in 16-bit parallel mode");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(io_handle, &panel_config, 0, &panel_handle));

    esp_lcd_panel_swap_xy( panel_handle, true);
    esp_lcd_panel_mirror( panel_handle, true, false);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(LCD_BLK, LCD_BK_LIGHT_ON_LEVEL);

    esp_lcd_touch_handle_t tp = NULL;
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    ESP_LOGI(TAG, "Initialize touch controller (I2C)");
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    /* Touch IO handle */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_V_RES,
        .y_max = LCD_H_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };
    /* Initialize touch */
    ESP_LOGI(TAG, "Initialize touch controller FT6236 (uses FT5X06 driver)");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));

    // initialize LVGL draw buffers
    lv_display_set_buffers( our_display, lvgl_buf, NULL, LVGL_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_flush_cb( our_display,  disp_flush );
    lv_display_set_user_data( our_display, panel_handle );

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

    ESP_LOGI(TAG, "Start LVGL");
    /* LVGL GUI creation code is called here! */
create_keyboard();
homePage();

readConfigFile(FILENAME_SAVE, false);

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(5));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}
