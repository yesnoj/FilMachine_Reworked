/**
 * @file mcp3424.h
 * @brief Minimal ESP-IDF driver for the Microchip MCP3424 4-channel delta-sigma ADC (I2C)
 *
 * Used on the FilMachine Mainboard for the 12 V / 5 V rails and the pump current.
 * Non-blocking use: mcp3424_start() launches a one-shot conversion on a channel,
 * mcp3424_read() fetches it later (reports not-ready if it is still converting).
 * 14-bit mode (60 SPS, ~17 ms per conversion), PGA x1, full scale ±2.048 V.
 */
#ifndef MCP3424_H
#define MCP3424_H

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
    uint8_t                 channel;    /* channel of the last started conversion (0..3) */
} mcp3424_t;

/** Probe @p addr on @p bus; on success register the device. ESP_ERR_NOT_FOUND if absent. */
esp_err_t mcp3424_init(mcp3424_t *dev, i2c_master_bus_handle_t bus, uint8_t addr);

/** Start a one-shot 14-bit conversion on channel @p ch (0 = CH1 … 3 = CH4). */
esp_err_t mcp3424_start(mcp3424_t *dev, uint8_t ch);

/** Read the last conversion. *ready = false while still converting. */
esp_err_t mcp3424_read(mcp3424_t *dev, float *volts, bool *ready);

#ifdef __cplusplus
}
#endif
#endif /* BOARD_JC4880P433 */
#endif /* MCP3424_H */
