/**
 * @file board_makerfabs_s3.h
 * @brief Pin definitions for the ORIGINAL board — 3.5" ILI9488 480×320
 *
 * Board: Makerfabs MaTouch ESP32-S3 Parallel TFT with Touch 3.5" ILI9488
 * MCU:   ESP32-S3-WROOM-1-N16R2 (16MB Flash, 2MB PSRAM)
 * Display: ILI9488 3.5" 480×320, 16-bit parallel interface
 * Touch: FT6236 capacitive, I2C
 * SD: SPI (custom pins)
 * Product: https://www.makerfabs.com/esp32-s3-parallel-tft-with-touch-ili9488.html
 */

#ifndef BOARD_MAKERFABS_S3_H
#define BOARD_MAKERFABS_S3_H

#define BOARD_NAME                  "Makerfabs-S3-ILI9488"

/* ═══════════════════════════════════════════════
 * Display — ILI9488, 16-bit parallel
 * ═══════════════════════════════════════════════ */
#define DISPLAY_DRIVER_ILI9488      1
#define DISPLAY_BUS_PARALLEL16      1

#define LCD_H_RES                   480
#define LCD_V_RES                   320

#define LCD_BLK                     GPIO_NUM_45
#define LCD_WR                      GPIO_NUM_35
#define LCD_RD                      GPIO_NUM_48
#define LCD_RS                      GPIO_NUM_36
#define LCD_CS                      GPIO_NUM_37
#define LCD_RST                     GPIO_NUM_NC
#define LCD_BSY                     GPIO_NUM_NC

#define LCD_D0                      GPIO_NUM_47
#define LCD_D1                      GPIO_NUM_21
#define LCD_D2                      GPIO_NUM_14
#define LCD_D3                      GPIO_NUM_13
#define LCD_D4                      GPIO_NUM_12
#define LCD_D5                      GPIO_NUM_11
#define LCD_D6                      GPIO_NUM_10
#define LCD_D7                      GPIO_NUM_9
#define LCD_D8                      GPIO_NUM_3
#define LCD_D9                      GPIO_NUM_8
#define LCD_D10                     GPIO_NUM_16
#define LCD_D11                     GPIO_NUM_15
#define LCD_D12                     GPIO_NUM_7
#define LCD_D13                     GPIO_NUM_6
#define LCD_D14                     GPIO_NUM_5
#define LCD_D15                     GPIO_NUM_4

/* ═══════════════════════════════════════════════
 * Touch — FT6236 capacitive, I2C
 * ═══════════════════════════════════════════════ */
#define TOUCH_DRIVER_FT6236         1

#define I2C_SDA                     GPIO_NUM_38
#define I2C_SCL                     GPIO_NUM_39
#define I2C_INT                     GPIO_NUM_40

/* ═══════════════════════════════════════════════
 * SD Card — SPI
 * ═══════════════════════════════════════════════ */
#define SDSPI_HOST_ID               SPI3_HOST
#define SD_CS                       GPIO_NUM_1
#define SD_MOSI                     GPIO_NUM_2
#define SD_MISO                     GPIO_NUM_41
#define SD_SCLK                     GPIO_NUM_42

/* ═══════════════════════════════════════════════
 * Motor — DC motor with H-bridge
 * ═══════════════════════════════════════════════ */
#define MOTOR_PIN_NUMBER            3
#define MOTOR_ENA_PIN               18
#define MOTOR_IN1_PIN               8
#define MOTOR_IN2_PIN               9

/* ═══════════════════════════════════════════════
 * Temperature — DS18B20 OneWire
 * ═══════════════════════════════════════════════ */
#define TEMPERATURE_BUS_PIN         17
#define TEMPERATURE_SENSOR_BATH     0
#define TEMPERATURE_SENSOR_CHEMICAL 1

/* ═══════════════════════════════════════════════
 * Relay board — MCP23017 I2C GPIO expander
 * ═══════════════════════════════════════════════ */
#define RELAY_NUMBER                8
#define HEATER_RLY                  0
#define C1_RLY                      1
#define C2_RLY                      2
#define C3_RLY                      3
#define WB_RLY                      4
#define WASTE_RLY                   5
#define PUMP_IN_RLY                 6
#define PUMP_OUT_RLY                7
#define INVALID_RELAY               255

/* ═══════════════════════════════════════════════
 * Pump speed — PCA9685 PWM controller
 * ═══════════════════════════════════════════════ */
#define PUMP_PCA9685_CHANNEL        0

/* ═══════════════════════════════════════════════
 * Speaker / Buzzer
 * ═══════════════════════════════════════════════ */
#define HAS_SPEAKER                 0   /* No speaker on this board */
/* #define SPEAKER_PIN              — not available */

/* ═══════════════════════════════════════════════
 * Additional sensors (not present on original board)
 * ═══════════════════════════════════════════════ */
#define HAS_FLOW_METER              0
#define HAS_WATER_LEVEL_SENSOR      0
#define HAS_HALL_SENSOR             0

#define TEST_PIN                    15

#endif /* BOARD_MAKERFABS_S3_H */
