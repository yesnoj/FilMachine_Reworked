/**
 * @file board_simulator.h
 * @brief "Board" definitions for the desktop simulator (SDL2 + LVGL)
 *
 * The simulator doesn't use real GPIO — all hardware is stubbed.
 * This header defines the logical constants (resolution, sensor indices,
 * relay numbers) so the application code compiles identically.
 *
 * Resolution matches the JC4880P433 physical panel: 800×480.
 */

#ifndef BOARD_SIMULATOR_H
#define BOARD_SIMULATOR_H

#define BOARD_NAME                  "Simulator"

/* ═══════════════════════════════════════════════
 * Resolution — matches JC4880P433 (800×480)
 * ═══════════════════════════════════════════════ */
#define LCD_H_RES                   800
#define LCD_V_RES                   480

/* ═══════════════════════════════════════════════
 * Display — simulated via SDL2 (no real driver)
 * Dummy defines for LCD pins referenced in accessories.c
 * (the actual GPIO init is skipped in the simulator).
 * ═══════════════════════════════════════════════ */
#define DISPLAY_DRIVER_SDL          1
#define DISPLAY_BUS_NONE            1

#define LCD_BLK                     0
#define LCD_WR                      0
#define LCD_RD                      0
#define LCD_RS                      0
#define LCD_CS                      0
#define LCD_RST                     0
#define LCD_BSY                     0
#define LCD_D0                      0
#define LCD_D1                      0
#define LCD_D2                      0
#define LCD_D3                      0
#define LCD_D4                      0
#define LCD_D5                      0
#define LCD_D6                      0
#define LCD_D7                      0
#define LCD_D8                      0
#define LCD_D9                      0
#define LCD_D10                     0
#define LCD_D11                     0
#define LCD_D12                     0
#define LCD_D13                     0
#define LCD_D14                     0
#define LCD_D15                     0

/* ═══════════════════════════════════════════════
 * Touch — simulated via SDL2 mouse
 * ═══════════════════════════════════════════════ */
#define TOUCH_DRIVER_SDL            1

/* Dummy I2C / touch pins (never used, but code references them) */
#define I2C_SDA                     0
#define I2C_SCL                     0
#define I2C_INT                     0

/* ═══════════════════════════════════════════════
 * SD Card — mapped to POSIX filesystem via stubs
 * ═══════════════════════════════════════════════ */
/* No real SPI — FatFS stubs redirect to sd/ folder.
 * Dummy defines needed because accessories.c references them at compile time.
 * SDSPI_HOST_ID is already defined in esp_stubs.h, so skip it here. */
#ifndef SDSPI_HOST_ID
#define SDSPI_HOST_ID               0
#endif
#define SD_CS                       0
#define SD_MOSI                     0
#define SD_MISO                     0
#define SD_SCLK                     0

/* ═══════════════════════════════════════════════
 * Motor — printf stubs
 * ═══════════════════════════════════════════════ */
#define MOTOR_PIN_NUMBER            3
#define MOTOR_ENA_PIN               0
#define MOTOR_IN1_PIN               0
#define MOTOR_IN2_PIN               0

/* ═══════════════════════════════════════════════
 * Temperature — simulated (20°C ambient + heater model)
 * ═══════════════════════════════════════════════ */
#define TEMPERATURE_BUS_PIN         0
#define TEMPERATURE_SENSOR_BATH     0
#define TEMPERATURE_SENSOR_CHEMICAL 1

/* ═══════════════════════════════════════════════
 * Relay board — stubs (printf only)
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
 * Pump speed — stub
 * ═══════════════════════════════════════════════ */
#define PUMP_PCA9685_CHANNEL        0

/* ═══════════════════════════════════════════════
 * Speaker — SDL2 audio
 * ═══════════════════════════════════════════════ */
#define HAS_SPEAKER                 1
#define SPEAKER_SDL                 1       /* SDL audio backend */

/* ═══════════════════════════════════════════════
 * Additional sensors — all simulated
 * ═══════════════════════════════════════════════ */
#define HAS_FLOW_METER              1       /* simulated */
#define FLOW_METER_PIN              0

#define HAS_WATER_LEVEL_SENSOR      1       /* simulated — always OK */
#define WATER_LEVEL_PIN             0

#define HAS_HALL_SENSOR             1       /* simulated — always spinning */
#define HALL_SENSOR_PIN             0

#define TEST_PIN                    0

#endif /* BOARD_SIMULATOR_H */
