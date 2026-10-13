/**
 * @file ads1115.c
 * @brief Minimal ADS1115 driver (ESP32-P4 i2c_master API). See ads1115.h.
 */
#include "include/ads1115.h"

#if defined(BOARD_JC4880P433)
#include <string.h>
#include "esp_log.h"

#define TAG              "ADS1115"
#define I2C_TIMEOUT_MS   50

/* Register pointer values */
#define REG_CONVERSION   0x00
#define REG_CONFIG       0x01

/* Config register (16 bit):
 *   bit 15    OS        write 1 = start a single conversion; read 1 = idle, 0 = converting
 *   bits14:12 MUX       100..111 = AIN0..AIN3 single-ended (vs GND)
 *   bits11:9  PGA       010 = +-2.048 V full scale (LSB 62.5 uV)
 *   bit 8     MODE      1 = single-shot / power-down
 *   bits 7:5  DR        100 = 128 SPS (~7.8 ms per conversion)
 *   bits 4:0  comparator disabled (COMP_QUE = 11) */
#define CFG_OS_START     0x8000
#define CFG_MUX_SE(ch)   ((uint16_t)(0x4000 | ((uint16_t)(ch) << 12)))
#define CFG_PGA_2V048    0x0400
#define CFG_MODE_SINGLE  0x0100
#define CFG_DR_128SPS    0x0080
#define CFG_COMP_OFF     0x0003
#define LSB_VOLTS        (2.048f / 32768.0f)

static esp_err_t write_reg(ads1115_t *dev, uint8_t reg, uint16_t val)
{
    uint8_t buf[3] = { reg, (uint8_t)(val >> 8), (uint8_t)val };
    return i2c_master_transmit(dev->dev_handle, buf, sizeof buf, I2C_TIMEOUT_MS);
}

static esp_err_t read_reg(ads1115_t *dev, uint8_t reg, uint16_t *val)
{
    uint8_t rx[2] = { 0, 0 };
    esp_err_t ret = i2c_master_transmit_receive(dev->dev_handle, &reg, 1, rx, sizeof rx, I2C_TIMEOUT_MS);
    if (ret == ESP_OK) *val = (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);
    return ret;
}

esp_err_t ads1115_init(ads1115_t *dev, i2c_master_bus_handle_t bus, uint8_t addr)
{
    memset(dev, 0, sizeof(*dev));
    dev->addr = addr;

    /* Probe first: with the separate-module setup nothing answers here and we
     * must not register a device that is not there. */
    if (i2c_master_probe(bus, addr, I2C_TIMEOUT_MS) != ESP_OK) {
        ESP_LOGI(TAG, "no ADS1115 at 0x%02X (rail monitor disabled)", addr);
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
    /* Sanity check that it really is an ADS1115: the config register must read
     * back as idle single-shot with the comparator bits we write. */
    uint16_t cfgv = 0;
    ret = write_reg(dev, REG_CONFIG, CFG_MUX_SE(0) | CFG_PGA_2V048 | CFG_MODE_SINGLE | CFG_DR_128SPS | CFG_COMP_OFF);
    if (ret == ESP_OK) ret = read_reg(dev, REG_CONFIG, &cfgv);
    if (ret != ESP_OK || (cfgv & 0x7FFF) != (CFG_MUX_SE(0) | CFG_PGA_2V048 | CFG_MODE_SINGLE | CFG_DR_128SPS | CFG_COMP_OFF)) {
        ESP_LOGW(TAG, "device at 0x%02X does not behave like an ADS1115 (cfg 0x%04X): rail monitor disabled", addr, cfgv);
        i2c_master_bus_rm_device(dev->dev_handle);
        dev->dev_handle = NULL;
        return ESP_ERR_NOT_SUPPORTED;
    }
    ESP_LOGI(TAG, "ADS1115 found at 0x%02X", addr);
    return ESP_OK;
}

esp_err_t ads1115_start(ads1115_t *dev, uint8_t ch)
{
    if (!dev->dev_handle || ch > 3) return ESP_ERR_INVALID_STATE;
    dev->channel = ch;
    return write_reg(dev, REG_CONFIG,
                     CFG_OS_START | CFG_MUX_SE(ch) | CFG_PGA_2V048 | CFG_MODE_SINGLE | CFG_DR_128SPS | CFG_COMP_OFF);
}

esp_err_t ads1115_read(ads1115_t *dev, float *volts, bool *ready)
{
    *ready = false;
    if (!dev->dev_handle) return ESP_ERR_INVALID_STATE;
    uint16_t cfg = 0;
    esp_err_t ret = read_reg(dev, REG_CONFIG, &cfg);
    if (ret != ESP_OK) return ret;
    if (!(cfg & CFG_OS_START)) return ESP_OK;       /* OS = 0: still converting */
    uint16_t raw = 0;
    ret = read_reg(dev, REG_CONVERSION, &raw);
    if (ret != ESP_OK) return ret;
    *volts = (float)(int16_t)raw * LSB_VOLTS;       /* single-ended: 0..+2.048 V (small negative = noise) */
    *ready = true;
    return ESP_OK;
}
#endif /* BOARD_JC4880P433 */
