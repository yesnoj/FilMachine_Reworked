/**
 * @file board_jc4880p433.h
 * @brief Pin definitions for the ESP32-P4 board — 4.3" ST7701S 480×800 → landscape 800×480
 *
 * Board: GUITION JC4880P433
 * MCU:   ESP32-P4 RISC-V dual-core @ 400 MHz, 32 MB PSRAM (HEX), 16 MB Flash
 * Display: ST7701S 4.3" IPS 480×800, MIPI-DSI 2-lane
 *          Used in landscape mode: LVGL runs at 800×480, PPA rotates 90°
 *          to fill the full 480×800 physical panel
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
 * LVGL resolution: 800×480 landscape (full panel, no scaling)
 * PPA handles rotation 90° only → fills 480×800 physical panel
 * ═══════════════════════════════════════════════ */
#define DISPLAY_DRIVER_ST7701       1
#define DISPLAY_BUS_MIPI_DSI        1

/* LVGL logical resolution — what FilMachine UI sees (landscape) */
#define LCD_H_RES                   800
#define LCD_V_RES                   480

/* Physical panel resolution — used internally by ST7701 driver & PPA */
#define LCD_PHYS_H_RES              480
#define LCD_PHYS_V_RES              800

/* PPA landscape configuration */
#define PPA_LANDSCAPE_ROTATION      90      /* degrees */
#define PPA_LANDSCAPE_SCALE         1.0f    /* no scaling — 800×480 fills 480×800 */

#define LCD_BLK                     23      /* Backlight enable (active high) */
#define LCD_RST_PIN                 5       /* LCD reset (active low), -1 if not used */

/* ═══════════════════════════════════════════════
 * Touch — GT911 capacitive, I2C
 * Uses the standard esp_lcd_touch_gt911 driver.
 * Physical touch area: 480×800 portrait.
 * Coordinate remapping to 800×480 landscape is done
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
 * Motor — DC motor with H-bridge (DBH-12V channel A)
 * All pins routed via the Expand IO header (JP1).
 * ═══════════════════════════════════════════════ */
#define MOTOR_PIN_NUMBER            3
#define MOTOR_ENA_PIN               32      /* PWM speed control — JP1 pin 19 */
#define MOTOR_IN1_PIN               33      /* H-bridge direction A — JP1 pin 8 */
#define MOTOR_IN2_PIN               34      /* H-bridge direction B — JP1 pin 17 */

/* ═══════════════════════════════════════════════
 * Temperature — DS18B20 OneWire
 * ═══════════════════════════════════════════════ */
#define TEMPERATURE_BUS_PIN         35      /* OneWire data — JP1 pin 15 */
#define TEMPERATURE_SENSOR_BATH     0
#define TEMPERATURE_SENSOR_CHEMICAL 1

/* ═══════════════════════════════════════════════
 * Solenoid driver — Adafruit I2C 8-Ch Solenoid Driver (#6318)
 * MCP23017-based, I2C address 0x20, on shared bus (GPIO 7/8).
 * Connected via JP1 pins 23 (SDA) / 25 (SCL) + 3.3V/GND.
 * Drop-in replacement for discrete relay board — same chip,
 * same address, same register layout.
 * ═══════════════════════════════════════════════ */
/* Port A — solenoid valves + heater relay via Adafruit #6318 MOSFET channels (3.6A max)
 * Pins 0-4: solenoid valves, Pins 5-6: free, Pin 7: heater relay trigger */
#define RELAY_NUMBER                5       /* Valve channels on MCP23017 port A (pins 0-4) */
#define C1_RLY                      0
#define C2_RLY                      1
#define C3_RLY                      2
#define WB_RLY                      3
#define WASTE_RLY                   4
#define HEATER_RLY                  7       /* Port A pin 7 — drives external relay for heater */

/* Port B — external relay board (10A per channel, for high-power loads)
 * 4-channel relay board connected to MCP23017 port B pins 8-11.
 * Active HIGH (relay board jumpered to high-level trigger).
 * JD-VCC on relay board powered from 12V (separate from logic VCC).
 * Heater moved to Port A pin 7 — all 4 Port B channels are spare. */
#define PORTB_RELAY_START           8       /* First port B pin */
#define PORTB_RELAY_COUNT           4       /* 4-channel relay board */
#define RELAY_SPARE_1               8       /* Port B pin 0 — relay ch.1: spare */
#define RELAY_SPARE_2               9       /* Port B pin 1 — relay ch.2: spare */
#define RELAY_SPARE_3               10      /* Port B pin 2 — relay ch.3: spare */
#define RELAY_SPARE_4               11      /* Port B pin 3 — relay ch.4: spare */

/* ── Port B repurposed: chemistry level sensors + heater MOSFETs ──
 * MCP23017 pin numbering: Port B = pins 8..15 (B0=8 … B7=15). Port B pins are
 * raw push-pull GPIO (broken out on the Adafruit #6318 bottom edge), unlike
 * Port A which is behind the board's MOSFET drivers.
 *
 * B0-B5: 3 chemistry containers × (min, max) XKC-Y21 sensors (INPUT + pull-up,
 *        LOW = water). B6-B7: 2 heater MOSFET modules (OUTPUT, HIGH = on). */
#define HAS_CHEM_LEVEL_SENSORS      1
#define CHEM_LEVEL_CONTAINERS       3
#define CHEM1_MIN_PIN               8       /* B0 */
#define CHEM1_MAX_PIN               9       /* B1 */
#define CHEM2_MIN_PIN               10      /* B2 */
#define CHEM2_MAX_PIN               11      /* B3 */
#define CHEM3_MIN_PIN               12      /* B4 */
#define CHEM3_MAX_PIN               13      /* B5 */

#define HAS_DUAL_HEATER             1
#define HEATER1_PIN                 14      /* B6 — heater MOSFET 1 */
#define HEATER2_PIN                 15      /* B7 — heater MOSFET 2 */

/* ── Power-rail monitoring (future custom board) ──
 * The 12V/5V/3.3V rails must be scaled below the ADC full-scale (~2.5V @ 12dB)
 * with resistor dividers, each feeding an ADC1 channel. When the custom board
 * provides them, set HAS_RAIL_MONITOR to 1 and fill in the channels + divider
 * ratios (ratio = (R_top + R_bottom) / R_bottom). The diagnostics "Power rails"
 * panel then shows live voltages; until then it shows "n/a". */
#define HAS_RAIL_MONITOR            0
#if HAS_RAIL_MONITOR
#define RAIL_12V_ADC_CH             ADC_CHANNEL_0
#define RAIL_12V_DIVIDER            6.0f     /* e.g. 100k/20k → 6.0 */
#define RAIL_5V_ADC_CH              ADC_CHANNEL_1
#define RAIL_5V_DIVIDER             3.0f     /* e.g. 20k/10k → 3.0 */
#define RAIL_3V3_ADC_CH             ADC_CHANNEL_2
#define RAIL_3V3_DIVIDER            2.0f     /* e.g. 10k/10k → 2.0 */
#endif

/* Pump direction is now handled by DBH-12V H-bridge channel B.
 * These constants are kept as logical direction markers so that
 * page_checkup.c and element_cleanPopup.c need no changes. */
#define PUMP_IN_RLY                 254     /* Logical: pump forward (filling) */
#define PUMP_OUT_RLY                253     /* Logical: pump reverse (draining) */
#define INVALID_RELAY               255

/* ═══════════════════════════════════════════════
 * Pump — DBH-12V H-bridge channel B
 *
 * The DBH-12V dual DC motor driver handles both the agitation
 * motor (channel A, see MOTOR_* defines above) and the pump
 * (channel B, below). Pump direction (fill/drain) is controlled
 * by reversing the H-bridge instead of using solenoid valves.
 *
 * Control scheme (same as motor channel A):
 *   IN1=HIGH, IN2=LOW  → forward  (filling)
 *   IN1=LOW,  IN2=HIGH → reverse  (draining)
 *   ENA = LEDC PWM     → speed control
 *
 * NOTE: Some DBH-12V variants use IN1/IN2 as direct PWM lines
 * (forward PWM on IN1, reverse PWM on IN2, EN=enable). If your
 * board works that way, swap PUMP_ENA_PIN with PUMP_IN1/IN2_PIN
 * and adjust pump_run() in accessories.c accordingly.
 *
 * Pins allocated from JP1 Expand IO header.
 * ═══════════════════════════════════════════════ */
#define HAS_PUMP_HBRIDGE            1
#define PUMP_IN1_PIN                49      /* JP1 pin 13 — pump direction A */
#define PUMP_IN2_PIN                50      /* JP1 pin 11 — pump direction B */
#define PUMP_ENA_PIN                51      /* JP1 pin 9  — pump PWM speed  */

/* ═══════════════════════════════════════════════
 * Additional sensors
 * All pins routed via the Expand IO header (JP1).
 * ═══════════════════════════════════════════════ */
#define HAS_FLOW_METER              1
#define FLOW_METER_PIN              52      /* YF-S201 pulse output — JP1 pin 7 */

#define HAS_WATER_LEVEL_SENSOR      1
#define WATER_LEVEL_PIN             29      /* XKC-Y21 (min level) — JP1 pin 14 */
#define WATER_LEVEL_2_PIN           30      /* Second level (max) — JP1 pin 12 */

#define HAS_HALL_SENSOR             1
#define HALL_SENSOR_PIN             31      /* KY-003 / A3144 — JP1 pin 10 */

/* ═══════════════════════════════════════════════
 * Wi-Fi — ESP32-C6 companion chip via SDIO (ESP-Hosted)
 *
 * The JC-ESP32P4-M3 module integrates an ESP32-C6-MINI
 * connected to the P4 over SDIO for Wi-Fi & BLE.
 * Uses the esp_hosted + esp_wifi_remote components in
 * ESP-IDF 5.5+ to provide standard esp_wifi API on P4.
 * ═══════════════════════════════════════════════ */
#define HAS_WIFI_REMOTE             1       /* Wi-Fi via C6 companion chip */
#define WIFI_HOSTED_SDIO            1       /* Transport: SDIO (not SPI) */
#define WIFI_SDIO_CLK               18
#define WIFI_SDIO_CMD               19
#define WIFI_SDIO_D0                14
#define WIFI_SDIO_D1                15
#define WIFI_SDIO_D2                16
#define WIFI_SDIO_D3                17
#define WIFI_C6_RESET_PIN           54      /* P4 GPIO to reset ESP32-C6 */

/* ═══════════════════════════════════════════════
 * Spare pins on Expand IO header (JP1):
 *   GPIO 28 (pin 21) — currently assigned as TEST_PIN
 *   All other JP1 GPIO pins are allocated.
 *
 * Not on JP1 header but free on ESP32-P4:
 *   GPIO 20, 21, 22, 25, 26, 27, 36, 37, 38, 45, 46, 47, 53
 * ═══════════════════════════════════════════════ */

#define TEST_PIN                    28      /* JP1 pin 21 — spare on header */

#endif /* BOARD_JC4880P433_H */
