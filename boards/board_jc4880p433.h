/**
 * @file board_jc4880p433.h
 * @brief Pin definitions for the ESP32-P4 board — 4.3" ST7701S 480×800 → landscape 480×320
 *
 * Board: GUITION JC4880P433
 * MCU:   ESP32-P4 RISC-V dual-core @ 400 MHz, 32 MB PSRAM (HEX), 16 MB Flash
 * Display: ST7701S 4.3" IPS 480×800, MIPI-DSI 2-lane
 *          Used in landscape mode: LVGL runs at 480×320, PPA rotates 90° and
 *          scales ×1.5 → 480×720 centred on 480×800 (40 px border top/bottom)
 * Touch: GT911 capacitive, I2C
 * SD: SDMMC 4-bit (much faster than SPI)
 * Audio: ES8311 codec via I2S + PA enable
 *
 * References:
 *   https://github.com/giltal/RetroESP32-P4  (driver source)
 *   https://it.aliexpress.com/item/1005009621821443.html
 */

#ifndef BOARD_JC4880P433_H
#define BOARD_JC4880P433_H

#define BOARD_NAME                  "JC4880P433-P4"

/* ═══════════════════════════════════════════════
 * Display — ST7701S, MIPI-DSI (2-lane)
 *
 * Physical panel: 480×800 portrait (MIPI-DSI, no user-facing GPIO except BLK/RST)
 * LVGL resolution: 480×320 landscape  (same as Makerfabs S3)
 * PPA handles rotation 90° + scale ×1.5 → 480×720 centred on 480×800
 * ═══════════════════════════════════════════════ */
#define DISPLAY_DRIVER_ST7701       1
#define DISPLAY_BUS_MIPI_DSI        1

/* LVGL logical resolution — what FilMachine UI sees */
#define LCD_H_RES                   480
#define LCD_V_RES                   320

/* Physical panel resolution — used internally by ST7701 driver & PPA */
#define LCD_PHYS_H_RES              480
#define LCD_PHYS_V_RES              800

/* PPA landscape configuration */
#define PPA_LANDSCAPE_ROTATION      90      /* degrees */
#define PPA_LANDSCAPE_SCALE         1.5f    /* ×1.5 → 480×720 on 480×800 */

#define LCD_BLK                     23      /* Backlight enable (active high) */
#define LCD_RST_PIN                 5       /* LCD reset (active low), -1 if not used */

/* ═══════════════════════════════════════════════
 * Touch — GT911 capacitive, I2C
 * Uses the standard esp_lcd_touch_gt911 driver.
 * Physical touch area: 480×800 portrait.
 * Coordinate remapping to 480×320 landscape is done
 * in the custom touch callback (see FilMachine.c).
 * ═══════════════════════════════════════════════ */
#define TOUCH_DRIVER_GT911          1
#define TOUCH_DRIVER_GT911_P4       1       /* Enables P4-specific coord remapping */

#define I2C_SDA                     7       /* Touch/peripheral I2C data */
#define I2C_SCL                     8       /* Touch/peripheral I2C clock */
#define TOUCH_INT_PIN               (-1)    /* GT911 interrupt (not wired) */
#define TOUCH_RST_PIN               (-1)    /* GT911 reset (not wired) */

/* ═══════════════════════════════════════════════
 * SD Card — SDMMC 4-bit (not SPI!)
 * Much faster than SPI: up to 40 MB/s vs ~4 MB/s
 * ═══════════════════════════════════════════════ */
#define SD_BUS_SDMMC                1       /* Flag: use SDMMC driver instead of SPI */
#define SD_MMC_CLK                  43
#define SD_MMC_CMD                  44
#define SD_MMC_D0                   39
#define SD_MMC_D1                   40
#define SD_MMC_D2                   41
#define SD_MMC_D3                   42

/* ═══════════════════════════════════════════════
 * Audio — ES8311 codec via I2S + power amplifier
 * ═══════════════════════════════════════════════ */
#define HAS_SPEAKER                 1
#define HAS_AUDIO_CODEC             1       /* ES8311 — much richer than raw I2S */
#define SPEAKER_I2S                 1
#define I2S_MCLK_PIN                13
#define I2S_BCLK_PIN                12
#define I2S_WS_PIN                  10      /* Word select / LRCLK */
#define I2S_DOUT_PIN                9       /* ESP32 → ES8311 SDIN */
#define I2S_DIN_PIN                 48      /* ES8311 DOUT → ESP32 (mic/line-in) */
#define AUDIO_PA_PIN                11      /* Power Amplifier enable (active high) */

/* ═══════════════════════════════════════════════
 * Motor — DC motor with H-bridge
 * Pin assignment provisional — depends on expansion header wiring.
 * ESP32-P4 GPIOs 24-38 are generally available.
 * ═══════════════════════════════════════════════ */
#define MOTOR_PIN_NUMBER            3
#define MOTOR_ENA_PIN               24      /* PWM speed control (or PCA9685) */
#define MOTOR_IN1_PIN               25      /* H-bridge direction A */
#define MOTOR_IN2_PIN               26      /* H-bridge direction B */
/* NOTE: provisional — final assignment depends on expansion PCB wiring */

/* ═══════════════════════════════════════════════
 * Temperature — DS18B20 OneWire
 * ═══════════════════════════════════════════════ */
#define TEMPERATURE_BUS_PIN         27      /* OneWire data */
#define TEMPERATURE_SENSOR_BATH     0
#define TEMPERATURE_SENSOR_CHEMICAL 1

/* ═══════════════════════════════════════════════
 * Relay board — MCP23017 I2C GPIO expander
 * Connected on the shared I2C bus (GPIO 7/8)
 * Same relay mapping as all other boards.
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
 * Connected on the shared I2C bus (GPIO 7/8)
 * ═══════════════════════════════════════════════ */
#define PUMP_PCA9685_CHANNEL        0

/* ═══════════════════════════════════════════════
 * Additional sensors
 * Same sensor support as JC4827W543 board.
 * GPIO assignment provisional — depends on expansion wiring.
 * ═══════════════════════════════════════════════ */
#define HAS_FLOW_METER              1
#define FLOW_METER_PIN              28      /* YF-S201 pulse output */

#define HAS_WATER_LEVEL_SENSOR      1
#define WATER_LEVEL_PIN             29      /* XKC-Y21 (min level) */
#define WATER_LEVEL_2_PIN           30      /* Second level (max, optional) */

#define HAS_HALL_SENSOR             1
#define HALL_SENSOR_PIN             31      /* KY-003 / A3144 */

/* ═══════════════════════════════════════════════
 * Spare pins (available for future use)
 * ESP32-P4 has GPIOs 0-54. With the above assignments,
 * many pins remain free: 0-6, 14-22, 32-38, 45-47, 49-54.
 * ═══════════════════════════════════════════════ */

#define TEST_PIN                    32

#endif /* BOARD_JC4880P433_H */
