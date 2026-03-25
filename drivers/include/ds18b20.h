/**
 * @file ds18b20.h
 * @brief ESP-IDF driver for DS18B20 OneWire temperature sensor
 *
 * Supports MULTIPLE sensors on the SAME OneWire bus using ROM addressing.
 * Each DS18B20 has a unique 64-bit ROM code used for individual addressing.
 *
 * Pin assignment:
 *   - TEMPERATURE_PIN (GPIO17) — shared OneWire bus for both sensors
 *
 * Wiring: Both DS18B20 data pins → GPIO17 with single 4.7kΩ pull-up to 3.3V
 */

#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DS18B20_MAX_SENSORS  4   /**< Max sensors we can discover on one bus */

/** Single sensor handle (with ROM address) */
typedef struct {
    uint8_t rom[8];         /**< 64-bit unique ROM code */
    bool    valid;          /**< true if ROM was read successfully */
} ds18b20_sensor_t;

/** OneWire bus handle (one GPIO, multiple sensors) */
typedef struct {
    int                gpio;                            /**< GPIO pin number */
    bool               initialized;                     /**< true after successful init */
    uint8_t            sensor_count;                    /**< Number of sensors found */
    ds18b20_sensor_t   sensors[DS18B20_MAX_SENSORS];    /**< Discovered sensors */
} ds18b20_bus_t;

/**
 * @brief Initialize OneWire bus and discover all DS18B20 sensors
 * @param bus   Pointer to bus handle
 * @param gpio  GPIO pin number (must have external 4.7kΩ pull-up)
 * @return ESP_OK if at least one sensor found, ESP_FAIL if none detected
 */
esp_err_t ds18b20_init(ds18b20_bus_t *bus, int gpio);

/**
 * @brief Read temperature from a specific sensor by index
 * @param bus     Pointer to initialized bus handle
 * @param index   Sensor index (0-based, from discovery order)
 * @param temp_c  Pointer to store temperature in °C (resolution ±0.0625°C)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t ds18b20_read_temp(ds18b20_bus_t *bus, uint8_t index, float *temp_c);

/**
 * @brief Get the number of sensors on the bus
 */
static inline uint8_t ds18b20_get_count(ds18b20_bus_t *bus) {
    return bus ? bus->sensor_count : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* DS18B20_H */
