/**
 * @file ds18b20.c
 * @brief ESP-IDF driver for DS18B20 OneWire temperature sensor
 *
 * Bit-banged OneWire using GPIO + esp_rom_delay_us() for precise timing.
 * Single-sensor per GPIO (skip ROM addressing).
 */

#include "include/ds18b20.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"   /* esp_rom_delay_us */

#define TAG "DS18B20"

/* ── OneWire timing (microseconds) ── */
#define OW_RESET_PULSE_US       480
#define OW_RESET_WAIT_US        70
#define OW_RESET_READ_US        410
#define OW_WRITE1_LOW_US        6
#define OW_WRITE1_HIGH_US       64
#define OW_WRITE0_LOW_US        60
#define OW_WRITE0_HIGH_US       10
#define OW_READ_INIT_US         6
#define OW_READ_SAMPLE_US       9
#define OW_READ_REST_US         55

/* DS18B20 commands */
#define DS18B20_CMD_CONVERT_T   0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE
#define DS18B20_CMD_SKIP_ROM    0xCC

/* ── Low-level OneWire helpers ── */

static void ow_pin_output(int gpio)
{
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
}

static void ow_pin_input(int gpio)
{
    gpio_set_direction(gpio, GPIO_MODE_INPUT);
}

static void ow_pin_low(int gpio)
{
    gpio_set_level(gpio, 0);
}

static void ow_pin_high(int gpio)
{
    gpio_set_level(gpio, 1);
}

static int ow_pin_read(int gpio)
{
    return gpio_get_level(gpio);
}

/**
 * @brief OneWire reset pulse — detects sensor presence
 * @return true if device present, false if no device
 */
static bool ow_reset(int gpio)
{
    bool present = false;

    ow_pin_output(gpio);
    ow_pin_low(gpio);
    esp_rom_delay_us(OW_RESET_PULSE_US);

    ow_pin_input(gpio);
    esp_rom_delay_us(OW_RESET_WAIT_US);

    present = (ow_pin_read(gpio) == 0);
    esp_rom_delay_us(OW_RESET_READ_US);

    return present;
}

/**
 * @brief Write a single bit on OneWire bus
 */
static void ow_write_bit(int gpio, uint8_t bit)
{
    ow_pin_output(gpio);
    ow_pin_low(gpio);

    if (bit) {
        esp_rom_delay_us(OW_WRITE1_LOW_US);
        ow_pin_input(gpio);    /* release — pulled HIGH by external resistor */
        esp_rom_delay_us(OW_WRITE1_HIGH_US);
    } else {
        esp_rom_delay_us(OW_WRITE0_LOW_US);
        ow_pin_input(gpio);    /* release */
        esp_rom_delay_us(OW_WRITE0_HIGH_US);
    }
}

/**
 * @brief Read a single bit from OneWire bus
 */
static uint8_t ow_read_bit(int gpio)
{
    uint8_t bit;

    ow_pin_output(gpio);
    ow_pin_low(gpio);
    esp_rom_delay_us(OW_READ_INIT_US);

    ow_pin_input(gpio);
    esp_rom_delay_us(OW_READ_SAMPLE_US);

    bit = ow_pin_read(gpio) ? 1 : 0;
    esp_rom_delay_us(OW_READ_REST_US);

    return bit;
}

/**
 * @brief Write a byte on OneWire bus (LSB first)
 */
static void ow_write_byte(int gpio, uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(gpio, data & 0x01);
        data >>= 1;
    }
}

/**
 * @brief Read a byte from OneWire bus (LSB first)
 */
static uint8_t ow_read_byte(int gpio)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data |= (ow_read_bit(gpio) << i);
    }
    return data;
}

/* ── Public API ── */

esp_err_t ds18b20_init(ds18b20_t *dev, int gpio)
{
    dev->gpio = gpio;
    dev->initialized = false;

    /* Configure GPIO as open-drain with pull-up (external 4.7kΩ required) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    /* Try reset — check if sensor is present */
    if (!ow_reset(gpio)) {
        ESP_LOGE(TAG, "DS18B20 not found on GPIO%d", gpio);
        return ESP_FAIL;
    }

    dev->initialized = true;
    ESP_LOGI(TAG, "DS18B20 found on GPIO%d", gpio);
    return ESP_OK;
}

esp_err_t ds18b20_read_temp(ds18b20_t *dev, float *temp_c)
{
    if (!dev->initialized) return ESP_FAIL;

    int gpio = dev->gpio;

    /* Step 1: Start conversion */
    if (!ow_reset(gpio)) {
        ESP_LOGE(TAG, "GPIO%d: reset failed before convert", gpio);
        return ESP_FAIL;
    }
    ow_write_byte(gpio, DS18B20_CMD_SKIP_ROM);
    ow_write_byte(gpio, DS18B20_CMD_CONVERT_T);

    /* Wait for conversion — default 12-bit takes max 750ms.
     * Poll the bus: DS18B20 pulls LOW while converting, releases when done. */
    int timeout = 80000;  /* ~800ms in 10us increments */
    while (timeout > 0) {
        esp_rom_delay_us(10);
        timeout--;
        if (ow_read_bit(gpio) == 1) break;  /* conversion done */
    }
    if (timeout <= 0) {
        ESP_LOGE(TAG, "GPIO%d: conversion timeout", gpio);
        return ESP_FAIL;
    }

    /* Step 2: Read scratchpad */
    if (!ow_reset(gpio)) {
        ESP_LOGE(TAG, "GPIO%d: reset failed before read", gpio);
        return ESP_FAIL;
    }
    ow_write_byte(gpio, DS18B20_CMD_SKIP_ROM);
    ow_write_byte(gpio, DS18B20_CMD_READ_SCRATCH);

    uint8_t lsb = ow_read_byte(gpio);
    uint8_t msb = ow_read_byte(gpio);

    /* Convert raw 16-bit value to temperature.
     * DS18B20 12-bit: resolution = 0.0625°C, signed two's complement. */
    int16_t raw = (int16_t)((msb << 8) | lsb);
    *temp_c = raw * 0.0625f;

    ESP_LOGD(TAG, "GPIO%d: raw=0x%04X temp=%.2f°C", gpio, (uint16_t)raw, *temp_c);
    return ESP_OK;
}
