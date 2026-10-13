/**
 * @file ads1115.h
 * @brief Minimal ESP-IDF driver for the TI ADS1115 4-channel 16-bit delta-sigma ADC (I2C)
 *
 * Used on the FilMachine Mainboard (U4, ADDR = GND -> 0x48) for the 12 V / 5 V rails
 * and the pump current (BTN8962 IS pins). Non-blocking use: ads1115_start() launches a
 * one-shot single-ended conversion on a channel, ads1115_read() fetches it later and
 * reports not-ready while it is still converting.
 * Fixed setup: PGA +-2.048 V (LSB 62.5 uV), 128 SPS (one conversion ~ 8 ms).
 */
#ifndef ADS1115_H
#define ADS1115_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#if defined(BOARD_JC4880P433)
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_master_dev_handle_t dev_handle;
    uint8_t                 addr;
    uint8_t                 channel;    /* channel of the last started conversion (0..3 = AIN0..AIN3) */
} ads1115_t;

/** Probe @p addr on @p bus; on success register the device. ESP_ERR_NOT_FOUND if absent. */
esp_err_t ads1115_init(ads1115_t *dev, i2c_master_bus_handle_t bus, uint8_t addr);

/** Start a one-shot conversion of AINn against GND, n = @p ch (0..3). */
esp_err_t ads1115_start(ads1115_t *dev, uint8_t ch);

/** Read the last conversion. *ready = false while still converting. */
esp_err_t ads1115_read(ads1115_t *dev, float *volts, bool *ready);

#ifdef __cplusplus
}
#endif
#endif /* BOARD_JC4880P433 */
#endif /* ADS1115_H */
