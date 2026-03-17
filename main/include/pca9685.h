/**
 * @file pca9685.h
 * @brief ESP-IDF driver for PCA9685 16-channel 12-bit PWM controller (I2C)
 *
 * Daisy-chained on the same I2C bus as MCP23017 (via Adafruit 6318
 * pass-through connector). Provides individual channel duty-cycle
 * control (0–4095) and configurable PWM frequency (24–1526 Hz).
 *
 * Typical use: pump speed control via PWM on channel 0.
 */

#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── PCA9685 Register addresses ── */
#define PCA9685_REG_MODE1       0x00
#define PCA9685_REG_MODE2       0x01
#define PCA9685_REG_LED0_ON_L   0x06    /* First channel ON low byte */
#define PCA9685_REG_LED0_ON_H   0x07
#define PCA9685_REG_LED0_OFF_L  0x08    /* First channel OFF low byte */
#define PCA9685_REG_LED0_OFF_H  0x09
#define PCA9685_REG_ALL_ON_L    0xFA    /* All channels ON low byte */
#define PCA9685_REG_ALL_ON_H    0xFB
#define PCA9685_REG_ALL_OFF_L   0xFC
#define PCA9685_REG_ALL_OFF_H   0xFD
#define PCA9685_REG_PRESCALE    0xFE    /* Prescaler register (only writable in sleep) */

/* ── MODE1 bits ── */
#define PCA9685_MODE1_ALLCALL   0x01    /* Respond to all-call address */
#define PCA9685_MODE1_SLEEP     0x10    /* Low-power mode (oscillator off) */
#define PCA9685_MODE1_AI        0x20    /* Auto-increment register pointer */
#define PCA9685_MODE1_RESTART   0x80    /* Restart enabled */

/* ── MODE2 bits ── */
#define PCA9685_MODE2_OUTDRV    0x04    /* Totem-pole output (vs open-drain) */

/* ── Defaults ── */
#define PCA9685_DEFAULT_ADDR    0x40    /* 7-bit I2C address (A0-A5 all LOW) */
#define PCA9685_OSC_FREQ        25000000  /* Internal oscillator 25 MHz */
#define PCA9685_MAX_CHANNELS    16
#define PCA9685_MAX_VALUE       4095    /* 12-bit max */

/* ── Handle ── */
typedef struct {
    uint8_t     addr;       /* 7-bit I2C address */
    i2c_port_t  port;       /* I2C port number */
    bool        initialized;
    uint32_t    pwm_freq;   /* Current PWM frequency in Hz */
} pca9685_t;

/**
 * @brief Initialize the PCA9685
 *
 * Resets the device, enables auto-increment, sets totem-pole output,
 * and configures the PWM frequency.
 *
 * @param dev       Pointer to device handle (will be populated)
 * @param port      I2C port number (e.g. I2C_NUM_0)
 * @param addr      7-bit I2C address (default 0x40)
 * @param freq_hz   Desired PWM frequency in Hz (24–1526)
 * @return ESP_OK on success
 */
esp_err_t pca9685_init(pca9685_t *dev, i2c_port_t port, uint8_t addr, uint32_t freq_hz);

/**
 * @brief Set PWM duty cycle for a single channel
 *
 * @param dev       Device handle
 * @param channel   Channel number 0–15
 * @param duty      Duty cycle 0–4095 (0 = fully off, 4095 = fully on)
 * @return ESP_OK on success
 */
esp_err_t pca9685_set_duty(pca9685_t *dev, uint8_t channel, uint16_t duty);

/**
 * @brief Set a channel fully ON (100% duty, bypasses PWM)
 */
esp_err_t pca9685_set_full_on(pca9685_t *dev, uint8_t channel);

/**
 * @brief Set a channel fully OFF (0% duty, bypasses PWM)
 */
esp_err_t pca9685_set_full_off(pca9685_t *dev, uint8_t channel);

/**
 * @brief Turn all channels off
 */
esp_err_t pca9685_all_off(pca9685_t *dev);

/**
 * @brief Change the PWM frequency
 *
 * @param dev       Device handle
 * @param freq_hz   Desired PWM frequency in Hz (24–1526)
 * @return ESP_OK on success
 */
esp_err_t pca9685_set_frequency(pca9685_t *dev, uint32_t freq_hz);

/**
 * @brief Convert 8-bit duty (0–255) to 12-bit (0–4095)
 *
 * Convenience for code that uses 8-bit duty values (like LEDC).
 */
static inline uint16_t pca9685_duty_from_8bit(uint8_t duty8)
{
    /* Map 0→0, 255→4095 with rounding */
    return (uint16_t)(((uint32_t)duty8 * 4095 + 127) / 255);
}

#ifdef __cplusplus
}
#endif

#endif /* PCA9685_H */
