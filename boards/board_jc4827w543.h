/**
 * @file board_jc4827w543.h
 * @brief Pin definitions for the NEW board — 4.3" NV3041A 480×272
 *
 * Board: Guition JC4827W543C (capacitive touch variant)
 * MCU:   ESP32-S3-WROOM-1-N4R8 (4MB Flash, 8MB PSRAM)
 * Display: NV3041A 4.3" IPS 480×272, QSPI interface
 * Touch: GT911 capacitive, I2C
 * SD: SPI (shared bus)
 * Speaker: I2S (3 pins)
 *
 * IO Pin Distribution (from official Guition Excel):
 * ┌────────┬────────────────────────────────────────┐
 * │ GPIO   │ Function                               │
 * ├────────┼────────────────────────────────────────┤
 * │ IO0    │ LCD_TE (boot strapping)                │
 * │ IO1    │ BL_CTRL (backlight PWM)                │
 * │ IO2    │ SPECK_LRCLK (I2S speaker)              │
 * │ IO3    │ CTP_INT (touch interrupt)              │
 * │ IO4    │ CTP_SCL (I2C clock — shared bus)       │
 * │ IO5    │ FREE → DS18B20 OneWire                 │
 * │ IO6    │ FREE → Flow meter (YF-S201)            │
 * │ IO7    │ FREE → Water level sensor (XKC-Y21)    │
 * │ IO8    │ CTP_SDA (I2C data — shared bus)        │
 * │ IO9    │ FREE → Hall sensor (KY-003)            │
 * │ IO10   │ TF_CS (SD card)                        │
 * │ IO11   │ TF_MISO (SD SPI)                       │
 * │ IO12   │ TF_CLK (SD SPI)                        │
 * │ IO13   │ TF_MOSI (SD SPI)                       │
 * │ IO14   │ FREE → Water level #2 (optional)       │
 * │ IO15   │ FREE → Spare                           │
 * │ IO16   │ FREE → Spare                           │
 * │ IO17   │ UART1 TX (spare UART)                  │
 * │ IO18   │ UART1 RX (spare UART)                  │
 * │ IO19   │ USB_D+                                 │
 * │ IO20   │ USB_D-                                 │
 * │ IO21   │ LCD_A0 (QSPI D0)                      │
 * │ IO35   │ NOT AVAILABLE                          │
 * │ IO36   │ FREE → Spare                           │
 * │ IO37   │ FREE → Spare                           │
 * │ IO38   │ RTP_CS (free with CTP variant)         │
 * │ IO39   │ LCD_A3 (QSPI D3)                      │
 * │ IO40   │ LCD_A2 (QSPI D2)                      │
 * │ IO41   │ SPECK_DIN (I2S speaker)                │
 * │ IO42   │ SPECK_BCLK (I2S speaker)               │
 * │ IO43   │ UART0 TX (console/debug)               │
 * │ IO44   │ UART0 RX (console/debug)               │
 * │ IO45   │ LCD_CS (QSPI)                          │
 * │ IO46   │ FREE (strapping pin — care at boot)    │
 * │ IO47   │ LCD_CLK (QSPI)                         │
 * │ IO48   │ LCD_A1 (QSPI D1)                      │
 * └────────┴────────────────────────────────────────┘
 */

#ifndef BOARD_JC4827W543_H
#define BOARD_JC4827W543_H

#define BOARD_NAME                  "JC4827W543"

/* ═══════════════════════════════════════════════
 * Display — NV3041A, QSPI
 * ═══════════════════════════════════════════════ */
#define DISPLAY_DRIVER_NV3041A      1
#define DISPLAY_BUS_QSPI            1

#define LCD_H_RES                   480
#define LCD_V_RES                   272

#define LCD_BLK                     1       /* IO1 — backlight PWM */
#define LCD_CS_PIN                  45      /* IO45 — QSPI chip select */
#define LCD_CLK_PIN                 47      /* IO47 — QSPI clock */
#define LCD_D0_PIN                  21      /* IO21 — QSPI data 0 */
#define LCD_D1_PIN                  48      /* IO48 — QSPI data 1 */
#define LCD_D2_PIN                  40      /* IO40 — QSPI data 2 */
#define LCD_D3_PIN                  39      /* IO39 — QSPI data 3 */
#define LCD_TE_PIN                  0       /* IO0  — tearing effect (optional) */

/* ═══════════════════════════════════════════════
 * Touch — GT911 capacitive, I2C
 * The I2C bus (IO4/IO8) is SHARED with MCP23017
 * and PCA9685 — all on the same bus.
 * ═══════════════════════════════════════════════ */
#define TOUCH_DRIVER_GT911          1

#define I2C_SDA                     8       /* IO8  — shared I2C data */
#define I2C_SCL                     4       /* IO4  — shared I2C clock */
#define TOUCH_INT_PIN               3       /* IO3  — touch interrupt */
#define TOUCH_RST_PIN               38      /* IO38 — touch reset (CTP variant) */

/* ═══════════════════════════════════════════════
 * SD Card — SPI
 * ═══════════════════════════════════════════════ */
#define SDSPI_HOST_ID               SPI3_HOST
#define SD_CS                       10      /* IO10 */
#define SD_MISO                     11      /* IO11 */
#define SD_SCLK                     12      /* IO12 */
#define SD_MOSI                     13      /* IO13 */

/* ═══════════════════════════════════════════════
 * Speaker — I2S digital audio (3 pins)
 * Much better than a simple buzzer!
 * ═══════════════════════════════════════════════ */
#define HAS_SPEAKER                 1
#define SPEAKER_I2S                 1       /* I2S interface (not PWM buzzer) */
#define SPEAKER_LRCLK_PIN          2       /* IO2  — I2S word select */
#define SPEAKER_DIN_PIN            41      /* IO41 — I2S data */
#define SPEAKER_BCLK_PIN           42      /* IO42 — I2S bit clock */

/* ═══════════════════════════════════════════════
 * Motor — DC motor with H-bridge
 * Uses MCP23017 outputs or direct GPIO.
 * Pin assignment TBD when hardware arrives.
 * For now, keep same logical interface.
 * ═══════════════════════════════════════════════ */
#define MOTOR_PIN_NUMBER            3
#define MOTOR_ENA_PIN               16      /* IO16 — PWM speed (or PCA9685) */
#define MOTOR_IN1_PIN               15      /* IO15 — direction A (or MCP23017) */
#define MOTOR_IN2_PIN               36      /* IO36 — direction B (or MCP23017) */
/* NOTE: these are provisional — final assignment depends on PCB design */

/* ═══════════════════════════════════════════════
 * Temperature — DS18B20 OneWire
 * ═══════════════════════════════════════════════ */
#define TEMPERATURE_BUS_PIN         5       /* IO5 — OneWire data */
#define TEMPERATURE_SENSOR_BATH     0
#define TEMPERATURE_SENSOR_CHEMICAL 1

/* ═══════════════════════════════════════════════
 * Relay board — MCP23017 I2C GPIO expander
 * Connected on the shared I2C bus (IO4/IO8)
 * Same relay mapping as original board.
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
 * Connected on the shared I2C bus (IO4/IO8)
 * ═══════════════════════════════════════════════ */
#define PUMP_PCA9685_CHANNEL        0

/* ═══════════════════════════════════════════════
 * Additional sensors — NEW on this board
 * ═══════════════════════════════════════════════ */
#define HAS_FLOW_METER              1
#define FLOW_METER_PIN              6       /* IO6 — YF-S201 pulse output */

#define HAS_WATER_LEVEL_SENSOR      1
#define WATER_LEVEL_PIN             7       /* IO7 — XKC-Y21 (min level) */
#define WATER_LEVEL_2_PIN           14      /* IO14 — second level (max, optional) */

#define HAS_HALL_SENSOR             1
#define HALL_SENSOR_PIN             9       /* IO9 — KY-003 / A3144 */

/* ═══════════════════════════════════════════════
 * Spare pins (available for future use)
 * ═══════════════════════════════════════════════ */
/* IO17 — UART1 TX (available if UART1 not needed) */
/* IO18 — UART1 RX (available if UART1 not needed) */
/* IO37 — completely free */
/* IO46 — free but strapping pin (careful at boot) */

#define TEST_PIN                    37

#endif /* BOARD_JC4827W543_H */
