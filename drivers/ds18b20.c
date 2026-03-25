/**
 * @file ds18b20.c
 * @brief ESP-IDF driver for DS18B20 OneWire temperature sensor
 *
 * Bit-banged OneWire using GPIO + esp_rom_delay_us() for precise timing.
 * Supports MULTIPLE sensors on the same bus via ROM SEARCH algorithm.
 */

#include "include/ds18b20.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"   /* esp_rom_delay_us */
#include <string.h>

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

/* DS18B20 ROM commands */
#define DS18B20_CMD_SEARCH_ROM  0xF0
#define DS18B20_CMD_READ_ROM    0x33
#define DS18B20_CMD_MATCH_ROM   0x55
#define DS18B20_CMD_SKIP_ROM    0xCC
#define DS18B20_CMD_CONVERT_T   0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE

/* DS18B20 family code */
#define DS18B20_FAMILY_CODE     0x28

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

/* ── CRC-8 for ROM validation (Dallas/Maxim polynomial x^8+x^5+x^4+1) ── */

static uint8_t ow_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            byte >>= 1;
        }
    }
    return crc;
}

/* ── ROM Search algorithm (finds all devices on bus) ── */

/**
 * @brief OneWire ROM Search — discovers all devices on the bus.
 *
 * Implements the standard 1-Wire search algorithm.
 * Each call to ow_search_next() finds the next device ROM.
 *
 * @param gpio     OneWire bus GPIO
 * @param rom      8-byte buffer for found ROM code
 * @param last_discrepancy  State variable (pass 0 for first call)
 * @return new last_discrepancy value (0 = search complete, -1 = error)
 */
static int ow_search_next(int gpio, uint8_t *rom, int last_discrepancy)
{
    if (!ow_reset(gpio)) return -1;

    ow_write_byte(gpio, DS18B20_CMD_SEARCH_ROM);

    int last_zero = 0;

    for (int bit_index = 1; bit_index <= 64; bit_index++) {
        uint8_t id_bit   = ow_read_bit(gpio);
        uint8_t cmp_bit  = ow_read_bit(gpio);

        if (id_bit == 1 && cmp_bit == 1) {
            /* No devices responding */
            return -1;
        }

        uint8_t direction;
        if (id_bit != cmp_bit) {
            /* All devices have the same bit — no conflict */
            direction = id_bit;
        } else {
            /* Conflict: both 0 and 1 present on bus */
            if (bit_index == last_discrepancy) {
                direction = 1;  /* Take the 1 path this time */
            } else if (bit_index > last_discrepancy) {
                direction = 0;  /* Take the 0 path (first time) */
            } else {
                /* Use same direction as previous search */
                int byte_idx = (bit_index - 1) / 8;
                int bit_pos  = (bit_index - 1) % 8;
                direction = (rom[byte_idx] >> bit_pos) & 0x01;
            }
            if (direction == 0) {
                last_zero = bit_index;
            }
        }

        /* Store the chosen bit in ROM */
        {
            int byte_idx = (bit_index - 1) / 8;
            int bit_pos  = (bit_index - 1) % 8;
            if (direction) {
                rom[byte_idx] |= (1 << bit_pos);
            } else {
                rom[byte_idx] &= ~(1 << bit_pos);
            }
        }

        /* Write chosen direction back to bus */
        ow_write_bit(gpio, direction);
    }

    return last_zero;
}


/* ── Public API ── */

esp_err_t ds18b20_init(ds18b20_bus_t *bus, int gpio)
{
    memset(bus, 0, sizeof(*bus));
    bus->gpio = gpio;
    bus->initialized = false;

    /* Configure GPIO as input with pull-up (external 4.7kΩ required) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    /* Check if any device is present */
    if (!ow_reset(gpio)) {
        ESP_LOGE(TAG, "No OneWire device found on GPIO%d", gpio);
        return ESP_FAIL;
    }

    /* Discover all sensors via ROM Search */
    uint8_t rom[8];
    int last_discrepancy = 0;
    int found = 0;

    memset(rom, 0, sizeof(rom));

    do {
        last_discrepancy = ow_search_next(gpio, rom, last_discrepancy);

        if (last_discrepancy < 0) break;

        /* Validate CRC */
        if (ow_crc8(rom, 7) != rom[7]) {
            ESP_LOGW(TAG, "GPIO%d: ROM CRC mismatch, skipping", gpio);
            continue;
        }

        /* Check family code (DS18B20 = 0x28) */
        if (rom[0] != DS18B20_FAMILY_CODE) {
            ESP_LOGW(TAG, "GPIO%d: non-DS18B20 device (family=0x%02X), skipping", gpio, rom[0]);
            continue;
        }

        if (found < DS18B20_MAX_SENSORS) {
            memcpy(bus->sensors[found].rom, rom, 8);
            bus->sensors[found].valid = true;
            ESP_LOGI(TAG, "GPIO%d: sensor[%d] ROM=%02X%02X%02X%02X%02X%02X%02X%02X",
                     gpio, found,
                     rom[0], rom[1], rom[2], rom[3],
                     rom[4], rom[5], rom[6], rom[7]);
            found++;
        }
    } while (last_discrepancy != 0 && found < DS18B20_MAX_SENSORS);

    bus->sensor_count = found;

    if (found == 0) {
        ESP_LOGE(TAG, "GPIO%d: no DS18B20 sensors found", gpio);
        return ESP_FAIL;
    }

    bus->initialized = true;
    ESP_LOGI(TAG, "GPIO%d: %d DS18B20 sensor(s) discovered", gpio, found);
    return ESP_OK;
}


esp_err_t ds18b20_read_temp(ds18b20_bus_t *bus, uint8_t index, float *temp_c)
{
    if (!bus || !bus->initialized) return ESP_FAIL;
    if (index >= bus->sensor_count) return ESP_FAIL;
    if (!bus->sensors[index].valid) return ESP_FAIL;

    int gpio = bus->gpio;
    uint8_t *rom = bus->sensors[index].rom;

    /* Step 1: Start conversion for this specific sensor */
    if (!ow_reset(gpio)) {
        ESP_LOGE(TAG, "GPIO%d: reset failed before convert", gpio);
        return ESP_FAIL;
    }

    /* MATCH ROM — address a specific sensor */
    ow_write_byte(gpio, DS18B20_CMD_MATCH_ROM);
    for (int i = 0; i < 8; i++) {
        ow_write_byte(gpio, rom[i]);
    }
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
        ESP_LOGE(TAG, "GPIO%d[%d]: conversion timeout", gpio, index);
        return ESP_FAIL;
    }

    /* Step 2: Read scratchpad from this sensor */
    if (!ow_reset(gpio)) {
        ESP_LOGE(TAG, "GPIO%d[%d]: reset failed before read", gpio, index);
        return ESP_FAIL;
    }

    /* MATCH ROM again for read */
    ow_write_byte(gpio, DS18B20_CMD_MATCH_ROM);
    for (int i = 0; i < 8; i++) {
        ow_write_byte(gpio, rom[i]);
    }
    ow_write_byte(gpio, DS18B20_CMD_READ_SCRATCH);

    uint8_t lsb = ow_read_byte(gpio);
    uint8_t msb = ow_read_byte(gpio);

    /* Convert raw 16-bit value to temperature.
     * DS18B20 12-bit: resolution = 0.0625°C, signed two's complement. */
    int16_t raw = (int16_t)((msb << 8) | lsb);
    *temp_c = raw * 0.0625f;

    ESP_LOGD(TAG, "GPIO%d[%d]: raw=0x%04X temp=%.2f°C", gpio, index, (uint16_t)raw, *temp_c);
    return ESP_OK;
}
