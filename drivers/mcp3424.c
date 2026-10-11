/**
 * @file mcp3424.c
 * @brief Minimal MCP3424 driver (ESP32-P4 i2c_master API). See mcp3424.h.
 */
#include "include/mcp3424.h"

#if defined(BOARD_JC4880P433)
#include <string.h>
#include "esp_log.h"

#define TAG              "MCP3424"
#define I2C_TIMEOUT_MS   50

/* Configuration byte: RDY | C1 C0 | O/C | S1 S0 | G1 G0
 *   RDY = 1 on write  -> start a one-shot conversion
 *   O/C = 0           -> one-shot mode
 *   S1S0 = 01         -> 14 bit, 60 SPS (LSB = 250 uV)
 *   G1G0 = 00         -> PGA x1 */
#define CFG_START        0x80
#define CFG_14BIT        0x04
#define CFG_RDY_BIT      0x80
#define LSB_14BIT_V      (2.048f / 8192.0f)

esp_err_t mcp3424_init(mcp3424_t *dev, i2c_master_bus_handle_t bus, uint8_t addr)
{
    memset(dev, 0, sizeof(*dev));
    dev->addr = addr;

    /* Probe first: with the separate-module setup nothing answers here and we
     * must not register a device that is not there. */
    if (i2c_master_probe(bus, addr, I2C_TIMEOUT_MS) != ESP_OK) {
        ESP_LOGI(TAG, "no MCP3424 at 0x%02X (rail monitor disabled)", addr);
        return ESP_ERR_NOT_FOUND;
    }

    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = 400000,
    };
    esp_err_t ret = i2c_master_bus_add_device(bus, &cfg, &dev->dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "add device 0x%02X failed: %s", addr, esp_err_to_name(ret));
        dev->dev_handle = NULL;
        return ret;
    }
    ESP_LOGI(TAG, "MCP3424 found at 0x%02X", addr);
    return ESP_OK;
}

esp_err_t mcp3424_start(mcp3424_t *dev, uint8_t ch)
{
    if (!dev->dev_handle || ch > 3) return ESP_ERR_INVALID_STATE;
    uint8_t cfg = CFG_START | (uint8_t)(ch << 5) | CFG_14BIT;
    dev->channel = ch;
    return i2c_master_transmit(dev->dev_handle, &cfg, 1, I2C_TIMEOUT_MS);
}

esp_err_t mcp3424_read(mcp3424_t *dev, float *volts, bool *ready)
{
    *ready = false;
    if (!dev->dev_handle) return ESP_ERR_INVALID_STATE;
    uint8_t buf[3] = { 0 };                 /* data MSB, data LSB, config */
    esp_err_t ret = i2c_master_receive(dev->dev_handle, buf, sizeof buf, I2C_TIMEOUT_MS);
    if (ret != ESP_OK) return ret;
    if (buf[2] & CFG_RDY_BIT) return ESP_OK;   /* still converting */
    /* 12..16-bit results are already sign-extended to 16 bits by the chip. */
    int16_t raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    *volts = (float)raw * LSB_14BIT_V;
    *ready = true;
    return ESP_OK;
}
#endif /* BOARD_JC4880P433 */
