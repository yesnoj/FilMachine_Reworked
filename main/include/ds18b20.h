/**
 * @file ds18b20.h
 * @brief ESP-IDF driver for DS18B20 OneWire temperature sensor
 *
 * Uses bit-banged OneWire protocol with microsecond delays.
 * Each sensor uses its own GPIO pin (no multi-drop).
 *
 * Pin assignment:
 *   - TEMPERATURE_CHEMICAL_PIN (GPIO17) — chemical sensor
 *   - TEMPERATURE_BATH_PIN     (GPIO19) — bath sensor
 *
 * Wiring: DS18B20 data pin → GPIOxx with 4.7kΩ pull-up to 3.3V
 */

#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Sensor handle */
typedef struct {
    int gpio;           /**< GPIO pin number */
    bool initialized;   /**< true after successful init */
} ds18b20_t;

/**
 * @brief Initialize a DS18B20 sensor on the given GPIO
 * @param dev  Pointer to sensor handle
 * @param gpio GPIO pin number (must have external 4.7kΩ pull-up)
 * @return ESP_OK on success, ESP_FAIL if sensor not detected
 */
esp_err_t ds18b20_init(ds18b20_t *dev, int gpio);

/**
 * @brief Read temperature from DS18B20
 * @param dev  Pointer to initialized sensor handle
 * @param temp_c  Pointer to store temperature in °C (resolution ±0.0625°C)
 * @return ESP_OK on success, ESP_FAIL on communication error
 */
esp_err_t ds18b20_read_temp(ds18b20_t *dev, float *temp_c);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20_H */
