/**
 * @file mcp23017.c
 * @brief ESP-IDF driver for MCP23017 I2C 16-bit I/O expander
 *
 * ESP32-P4 (BOARD_JC4880P433): uses the new i2c_master API
 * All other targets: uses the legacy driver/i2c.h API
 */

#include "include/mcp23017.h"
#include "esp_log.h"
#include <string.h>

#define TAG "MCP23017"

/* ── Low-level I2C helpers ── */

#if defined(BOARD_JC4880P433)
/* ────── New I2C master API (ESP32-P4) ────── */

#define I2C_TIMEOUT_MS  100

static esp_err_t mcp23017_write_reg(mcp23017_t *dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    esp_err_t ret = i2c_master_transmit(dev->dev_handle, buf, 2, I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write reg 0x%02X failed: %s", reg, esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t mcp23017_read_reg(mcp23017_t *dev, uint8_t reg, uint8_t *val)
{
    esp_err_t ret = i2c_master_transmit_receive(dev->dev_handle, &reg, 1, val, 1, I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read reg 0x%02X failed: %s", reg, esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t mcp23017_init(mcp23017_t *dev, i2c_master_bus_handle_t bus, uint8_t addr)
{
    memset(dev, 0, sizeof(mcp23017_t));
    dev->addr = addr;

    /* Register device on the I2C bus */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &dev->dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add MCP23017 device at 0x%02X: %s", addr, esp_err_to_name(ret));
        return ret;
    }

    /* Probe: try to read MODE register via a simple transmit_receive */
    uint8_t probe_reg = MCP23017_IOCON;
    uint8_t probe_val = 0;
    ret = i2c_master_transmit_receive(dev->dev_handle, &probe_reg, 1, &probe_val, 1, I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MCP23017 not found at 0x%02X", addr);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MCP23017 found at 0x%02X", addr);

    /* Set IOCON: BANK=0 (sequential addresses), MIRROR=0, SEQOP=0 */
    mcp23017_write_reg(dev, MCP23017_IOCON, 0x00);

    /* Read current register state into cache */
    mcp23017_read_reg(dev, MCP23017_IODIRA, &dev->iodir[0]);
    mcp23017_read_reg(dev, MCP23017_IODIRB, &dev->iodir[1]);
    mcp23017_read_reg(dev, MCP23017_OLATA,  &dev->olat[0]);
    mcp23017_read_reg(dev, MCP23017_OLATB,  &dev->olat[1]);
    mcp23017_read_reg(dev, MCP23017_GPPUA,  &dev->gppu[0]);
    mcp23017_read_reg(dev, MCP23017_GPPUB,  &dev->gppu[1]);

    ESP_LOGI(TAG, "Init OK — IODIR: A=0x%02X B=0x%02X, OLAT: A=0x%02X B=0x%02X",
             dev->iodir[0], dev->iodir[1], dev->olat[0], dev->olat[1]);

    return ESP_OK;
}

#else
/* ────── Legacy I2C API (ESP32-S3 etc.) ────── */

#define I2C_TIMEOUT_TICKS  50   /* ~50 ms at default 1 kHz tick rate */

static esp_err_t mcp23017_write_reg(mcp23017_t *dev, uint8_t reg, uint8_t val)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, val, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(dev->port, cmd, I2C_TIMEOUT_TICKS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write reg 0x%02X failed: %s", reg, esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t mcp23017_read_reg(mcp23017_t *dev, uint8_t reg, uint8_t *val)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);  /* Repeated start */
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, val, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(dev->port, cmd, I2C_TIMEOUT_TICKS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read reg 0x%02X failed: %s", reg, esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t mcp23017_init(mcp23017_t *dev, i2c_port_t port, uint8_t addr)
{
    memset(dev, 0, sizeof(mcp23017_t));
    dev->addr = addr;
    dev->port = port;

    /* Verify device is present */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(port, cmd, I2C_TIMEOUT_TICKS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MCP23017 not found at 0x%02X", addr);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MCP23017 found at 0x%02X", addr);

    /* Set IOCON: BANK=0 (sequential addresses), MIRROR=0, SEQOP=0 */
    mcp23017_write_reg(dev, MCP23017_IOCON, 0x00);

    /* Read current register state into cache */
    mcp23017_read_reg(dev, MCP23017_IODIRA, &dev->iodir[0]);
    mcp23017_read_reg(dev, MCP23017_IODIRB, &dev->iodir[1]);
    mcp23017_read_reg(dev, MCP23017_OLATA,  &dev->olat[0]);
    mcp23017_read_reg(dev, MCP23017_OLATB,  &dev->olat[1]);
    mcp23017_read_reg(dev, MCP23017_GPPUA,  &dev->gppu[0]);
    mcp23017_read_reg(dev, MCP23017_GPPUB,  &dev->gppu[1]);

    ESP_LOGI(TAG, "Init OK — IODIR: A=0x%02X B=0x%02X, OLAT: A=0x%02X B=0x%02X",
             dev->iodir[0], dev->iodir[1], dev->olat[0], dev->olat[1]);

    return ESP_OK;
}

#endif /* BOARD_JC4880P433 */

/* ── Public API (common to both I2C backends) ── */

esp_err_t mcp23017_pinMode(mcp23017_t *dev, uint8_t pin, uint8_t mode)
{
    if (pin > 15) return ESP_ERR_INVALID_ARG;

    uint8_t port_idx = (pin < 8) ? 0 : 1;
    uint8_t bit = pin % 8;

    /* Set direction */
    if (mode == MCP23017_OUTPUT) {
        dev->iodir[port_idx] &= ~(1 << bit);   /* 0 = output */
    } else {
        dev->iodir[port_idx] |= (1 << bit);    /* 1 = input */
    }

    /* Set pull-up */
    if (mode == MCP23017_INPUT_PULLUP) {
        dev->gppu[port_idx] |= (1 << bit);
    } else {
        dev->gppu[port_idx] &= ~(1 << bit);
    }

    esp_err_t ret = mcp23017_write_reg(dev,
        port_idx == 0 ? MCP23017_IODIRA : MCP23017_IODIRB,
        dev->iodir[port_idx]);
    if (ret != ESP_OK) return ret;

    ret = mcp23017_write_reg(dev,
        port_idx == 0 ? MCP23017_GPPUA : MCP23017_GPPUB,
        dev->gppu[port_idx]);

    return ret;
}

esp_err_t mcp23017_digitalWrite(mcp23017_t *dev, uint8_t pin, uint8_t value)
{
    if (pin > 15) return ESP_ERR_INVALID_ARG;

    uint8_t port_idx = (pin < 8) ? 0 : 1;
    uint8_t bit = pin % 8;

    if (value) {
        dev->olat[port_idx] |= (1 << bit);
    } else {
        dev->olat[port_idx] &= ~(1 << bit);
    }

    return mcp23017_write_reg(dev,
        port_idx == 0 ? MCP23017_OLATA : MCP23017_OLATB,
        dev->olat[port_idx]);
}

int mcp23017_digitalRead(mcp23017_t *dev, uint8_t pin)
{
    if (pin > 15) return -1;

    uint8_t port_idx = (pin < 8) ? 0 : 1;
    uint8_t bit = pin % 8;
    uint8_t val = 0;

    esp_err_t ret = mcp23017_read_reg(dev,
        port_idx == 0 ? MCP23017_GPIOA : MCP23017_GPIOB,
        &val);

    if (ret != ESP_OK) return -1;

    return (val >> bit) & 0x01;
}

esp_err_t mcp23017_writePort(mcp23017_t *dev, uint8_t port_ab, uint8_t value)
{
    if (port_ab > 1) return ESP_ERR_INVALID_ARG;

    dev->olat[port_ab] = value;
    return mcp23017_write_reg(dev,
        port_ab == 0 ? MCP23017_OLATA : MCP23017_OLATB,
        value);
}

int mcp23017_readPort(mcp23017_t *dev, uint8_t port_ab)
{
    if (port_ab > 1) return -1;

    uint8_t val = 0;
    esp_err_t ret = mcp23017_read_reg(dev,
        port_ab == 0 ? MCP23017_GPIOA : MCP23017_GPIOB,
        &val);

    return (ret == ESP_OK) ? val : -1;
}

esp_err_t mcp23017_allOutputLow(mcp23017_t *dev)
{
    esp_err_t ret;

    /* Set all pins as output */
    dev->iodir[0] = 0x00;
    dev->iodir[1] = 0x00;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRA, 0x00);
    if (ret != ESP_OK) return ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRB, 0x00);
    if (ret != ESP_OK) return ret;

    /* Set all outputs LOW */
    dev->olat[0] = 0x00;
    dev->olat[1] = 0x00;
    ret = mcp23017_write_reg(dev, MCP23017_OLATA, 0x00);
    if (ret != ESP_OK) return ret;
    ret = mcp23017_write_reg(dev, MCP23017_OLATB, 0x00);

    return ret;
}
