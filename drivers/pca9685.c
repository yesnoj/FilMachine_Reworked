/**
 * @file pca9685.c
 * @brief ESP-IDF driver for PCA9685 16-channel 12-bit PWM controller
 *
 * ESP32-P4 (BOARD_JC4880P433): uses the new i2c_master API
 * All other targets: uses the legacy driver/i2c.h API
 */

#include "pca9685.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

/* FreeRTOS pdMS_TO_TICKS — on ESP-IDF this comes from FreeRTOS headers.
 * We define it locally if not already available (e.g. simulator build). */
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((uint32_t)(ms))
#endif

static const char *TAG = "PCA9685";

/* ── Low-level I2C helpers ── */

#if defined(BOARD_JC4880P433)
/* ────── New I2C master API (ESP32-P4) ────── */

#define I2C_TIMEOUT_MS  100

static esp_err_t pca9685_write_reg(pca9685_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    return i2c_master_transmit(dev->dev_handle, buf, 2, I2C_TIMEOUT_MS);
}

static esp_err_t pca9685_read_reg(pca9685_t *dev, uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(dev->dev_handle, &reg, 1, value, 1, I2C_TIMEOUT_MS);
}

/**
 * Write 4 bytes (ON_L, ON_H, OFF_L, OFF_H) to a channel register block.
 * Uses auto-increment (MODE1.AI must be set).
 */
static esp_err_t pca9685_write_channel(pca9685_t *dev, uint8_t base_reg,
                                        uint16_t on, uint16_t off)
{
    uint8_t buf[5] = {
        base_reg,
        (uint8_t)(on & 0xFF),
        (uint8_t)((on >> 8) & 0x1F),
        (uint8_t)(off & 0xFF),
        (uint8_t)((off >> 8) & 0x1F),
    };
    return i2c_master_transmit(dev->dev_handle, buf, 5, I2C_TIMEOUT_MS);
}

esp_err_t pca9685_init(pca9685_t *dev, i2c_master_bus_handle_t bus, uint8_t addr, uint32_t freq_hz)
{
    dev->addr = addr;
    dev->initialized = false;
    dev->pwm_freq = 0;

    /* Register device on the I2C bus */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &dev->dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add PCA9685 device at 0x%02X: %s", addr, esp_err_to_name(ret));
        return ret;
    }

    /* Probe: try to read MODE1 */
    uint8_t mode1 = 0;
    ret = pca9685_read_reg(dev, PCA9685_REG_MODE1, &mode1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PCA9685 not found at 0x%02X (err=%s)", addr, esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "PCA9685 found at 0x%02X (MODE1=0x%02X)", addr, mode1);

    /* Software reset: sleep → set prescaler → wake */
    ret = pca9685_write_reg(dev, PCA9685_REG_MODE1,
                             PCA9685_MODE1_SLEEP | PCA9685_MODE1_AI);
    if (ret != ESP_OK) return ret;

    if (freq_hz < 24) freq_hz = 24;
    if (freq_hz > 1526) freq_hz = 1526;

    uint8_t prescale = (uint8_t)(((uint32_t)PCA9685_OSC_FREQ + (2048UL * freq_hz))
                                  / (4096UL * freq_hz) - 1);
    if (prescale < 3) prescale = 3;

    ret = pca9685_write_reg(dev, PCA9685_REG_PRESCALE, prescale);
    if (ret != ESP_OK) return ret;
    ESP_LOGI(TAG, "Prescaler=%u → PWM freq ~%lu Hz", prescale,
             (unsigned long)(PCA9685_OSC_FREQ / (4096UL * (prescale + 1))));

    ret = pca9685_write_reg(dev, PCA9685_REG_MODE1, PCA9685_MODE1_AI);
    if (ret != ESP_OK) return ret;

    esp_rom_delay_us(600);

    ret = pca9685_write_reg(dev, PCA9685_REG_MODE1,
                             PCA9685_MODE1_RESTART | PCA9685_MODE1_AI);
    if (ret != ESP_OK) return ret;

    ret = pca9685_write_reg(dev, PCA9685_REG_MODE2, PCA9685_MODE2_OUTDRV);
    if (ret != ESP_OK) return ret;

    pca9685_all_off(dev);

    dev->pwm_freq = freq_hz;
    dev->initialized = true;
    ESP_LOGI(TAG, "Init OK — %u channels, %lu Hz PWM",
             PCA9685_MAX_CHANNELS, (unsigned long)freq_hz);
    return ESP_OK;
}

#else
/* ────── Legacy I2C API (ESP32-S3 etc.) ────── */

static esp_err_t pca9685_write_reg(pca9685_t *dev, uint8_t reg, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(dev->port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t pca9685_read_reg(pca9685_t *dev, uint8_t reg, uint8_t *value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);  /* Repeated start */
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, value, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(dev->port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * Write 4 bytes (ON_L, ON_H, OFF_L, OFF_H) to a channel register block.
 * Uses auto-increment (MODE1.AI must be set).
 */
static esp_err_t pca9685_write_channel(pca9685_t *dev, uint8_t base_reg,
                                        uint16_t on, uint16_t off)
{
    uint8_t data[4] = {
        (uint8_t)(on & 0xFF),
        (uint8_t)((on >> 8) & 0x1F),
        (uint8_t)(off & 0xFF),
        (uint8_t)((off >> 8) & 0x1F),
    };

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, base_reg, true);
    for (int i = 0; i < 4; i++) {
        i2c_master_write_byte(cmd, data[i], true);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(dev->port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t pca9685_init(pca9685_t *dev, i2c_port_t port, uint8_t addr, uint32_t freq_hz)
{
    dev->addr = addr;
    dev->port = port;
    dev->initialized = false;
    dev->pwm_freq = 0;

    /* Probe: try to read MODE1 */
    uint8_t mode1 = 0;
    esp_err_t ret = pca9685_read_reg(dev, PCA9685_REG_MODE1, &mode1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PCA9685 not found at 0x%02X (err=%s)", addr, esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "PCA9685 found at 0x%02X (MODE1=0x%02X)", addr, mode1);

    /* Software reset: sleep → set prescaler → wake */
    ret = pca9685_write_reg(dev, PCA9685_REG_MODE1,
                             PCA9685_MODE1_SLEEP | PCA9685_MODE1_AI);
    if (ret != ESP_OK) return ret;

    if (freq_hz < 24) freq_hz = 24;
    if (freq_hz > 1526) freq_hz = 1526;

    uint8_t prescale = (uint8_t)(((uint32_t)PCA9685_OSC_FREQ + (2048UL * freq_hz))
                                  / (4096UL * freq_hz) - 1);
    if (prescale < 3) prescale = 3;

    ret = pca9685_write_reg(dev, PCA9685_REG_PRESCALE, prescale);
    if (ret != ESP_OK) return ret;
    ESP_LOGI(TAG, "Prescaler=%u → PWM freq ~%lu Hz", prescale,
             (unsigned long)(PCA9685_OSC_FREQ / (4096UL * (prescale + 1))));

    ret = pca9685_write_reg(dev, PCA9685_REG_MODE1, PCA9685_MODE1_AI);
    if (ret != ESP_OK) return ret;

    esp_rom_delay_us(600);

    ret = pca9685_write_reg(dev, PCA9685_REG_MODE1,
                             PCA9685_MODE1_RESTART | PCA9685_MODE1_AI);
    if (ret != ESP_OK) return ret;

    ret = pca9685_write_reg(dev, PCA9685_REG_MODE2, PCA9685_MODE2_OUTDRV);
    if (ret != ESP_OK) return ret;

    pca9685_all_off(dev);

    dev->pwm_freq = freq_hz;
    dev->initialized = true;
    ESP_LOGI(TAG, "Init OK — %u channels, %lu Hz PWM",
             PCA9685_MAX_CHANNELS, (unsigned long)freq_hz);
    return ESP_OK;
}

#endif /* BOARD_JC4880P433 */

/* ── Public API (common to both I2C backends) ── */

esp_err_t pca9685_set_duty(pca9685_t *dev, uint8_t channel, uint16_t duty)
{
    if (channel >= PCA9685_MAX_CHANNELS) {
        ESP_LOGE(TAG, "Invalid channel %u (max %u)", channel, PCA9685_MAX_CHANNELS - 1);
        return ESP_ERR_INVALID_ARG;
    }
    if (duty > PCA9685_MAX_VALUE) duty = PCA9685_MAX_VALUE;

    uint8_t base = PCA9685_REG_LED0_ON_L + 4 * channel;

    if (duty == 0) {
        /* Fully off — set OFF full-on bit */
        return pca9685_write_channel(dev, base, 0x0000, 0x1000);
    }
    if (duty >= PCA9685_MAX_VALUE) {
        /* Fully on — set ON full-on bit */
        return pca9685_write_channel(dev, base, 0x1000, 0x0000);
    }

    /* Normal PWM: ON at tick 0, OFF at tick = duty */
    return pca9685_write_channel(dev, base, 0, duty);
}

esp_err_t pca9685_set_full_on(pca9685_t *dev, uint8_t channel)
{
    return pca9685_set_duty(dev, channel, PCA9685_MAX_VALUE);
}

esp_err_t pca9685_set_full_off(pca9685_t *dev, uint8_t channel)
{
    return pca9685_set_duty(dev, channel, 0);
}

esp_err_t pca9685_all_off(pca9685_t *dev)
{
    /* Use ALL_LED registers for a single-shot all-off */
    return pca9685_write_channel(dev, PCA9685_REG_ALL_ON_L, 0x0000, 0x1000);
}

esp_err_t pca9685_set_frequency(pca9685_t *dev, uint32_t freq_hz)
{
    if (freq_hz < 24) freq_hz = 24;
    if (freq_hz > 1526) freq_hz = 1526;

    /* Must go to sleep to change prescaler */
    uint8_t mode1 = 0;
    esp_err_t ret = pca9685_read_reg(dev, PCA9685_REG_MODE1, &mode1);
    if (ret != ESP_OK) return ret;

    /* Sleep */
    ret = pca9685_write_reg(dev, PCA9685_REG_MODE1,
                             (mode1 & ~PCA9685_MODE1_RESTART) | PCA9685_MODE1_SLEEP);
    if (ret != ESP_OK) return ret;

    /* Write prescaler */
    uint8_t prescale = (uint8_t)(((uint32_t)PCA9685_OSC_FREQ + (2048UL * freq_hz))
                                  / (4096UL * freq_hz) - 1);
    if (prescale < 3) prescale = 3;

    ret = pca9685_write_reg(dev, PCA9685_REG_PRESCALE, prescale);
    if (ret != ESP_OK) return ret;

    /* Wake up (restore old mode, clear sleep) */
    ret = pca9685_write_reg(dev, PCA9685_REG_MODE1, mode1 & ~PCA9685_MODE1_SLEEP);
    if (ret != ESP_OK) return ret;

    esp_rom_delay_us(600);

    /* Restart */
    ret = pca9685_write_reg(dev, PCA9685_REG_MODE1, mode1 | PCA9685_MODE1_RESTART);

    dev->pwm_freq = freq_hz;
    ESP_LOGI(TAG, "Frequency changed to ~%lu Hz (prescale=%u)",
             (unsigned long)(PCA9685_OSC_FREQ / (4096UL * (prescale + 1))), prescale);
    return ret;
}
