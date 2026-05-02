/**
 * @file mcp23017.h
 * @brief ESP-IDF driver for MCP23017 I2C 16-bit I/O expander
 *
 * Provides Arduino-compatible API: mcp23017_pinMode, mcp23017_digitalWrite,
 * mcp23017_digitalRead — mapped to I2C register-level operations.
 *
 * Pin numbering: 0-7 = GPA0-GPA7,  8-15 = GPB0-GPB7
 */

#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#if defined(BOARD_JC4880P433)
    #include "driver/i2c_master.h"
#else
    #include "driver/i2c.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ── MCP23017 Register addresses (IOCON.BANK = 0, default) ── */
#define MCP23017_IODIRA     0x00    /* I/O direction register A (1=input, 0=output) */
#define MCP23017_IODIRB     0x01    /* I/O direction register B */
#define MCP23017_IPOLA      0x02    /* Input polarity register A */
#define MCP23017_IPOLB      0x03    /* Input polarity register B */
#define MCP23017_GPINTENA   0x04    /* Interrupt-on-change enable A */
#define MCP23017_GPINTENB   0x05    /* Interrupt-on-change enable B */
#define MCP23017_DEFVALA    0x06    /* Default compare value A */
#define MCP23017_DEFVALB    0x07    /* Default compare value B */
#define MCP23017_INTCONA    0x08    /* Interrupt control register A */
#define MCP23017_INTCONB    0x09    /* Interrupt control register B */
#define MCP23017_IOCON      0x0A    /* Configuration register */
#define MCP23017_GPPUA      0x0C    /* Pull-up resistor register A */
#define MCP23017_GPPUB      0x0D    /* Pull-up resistor register B */
#define MCP23017_INTFA      0x0E    /* Interrupt flag register A */
#define MCP23017_INTFB      0x0F    /* Interrupt flag register B */
#define MCP23017_INTCAPA    0x10    /* Interrupt captured value A */
#define MCP23017_INTCAPB    0x11    /* Interrupt captured value B */
#define MCP23017_GPIOA      0x12    /* GPIO register A */
#define MCP23017_GPIOB      0x13    /* GPIO register B */
#define MCP23017_OLATA      0x14    /* Output latch register A */
#define MCP23017_OLATB      0x15    /* Output latch register B */

/* ── Pin mode constants (Arduino-compatible) ── */
#define MCP23017_INPUT      1
#define MCP23017_OUTPUT     0
#define MCP23017_INPUT_PULLUP  2

/* ── Default I2C address ── */
#define MCP23017_DEFAULT_ADDR   0x20

/* ── Handle ── */
typedef struct {
    uint8_t     addr;       /* 7-bit I2C address */
#if defined(BOARD_JC4880P433)
    i2c_master_dev_handle_t dev_handle;  /* New I2C master device handle */
#else
    i2c_port_t  port;       /* I2C port number (legacy API) */
#endif
    uint8_t     iodir[2];   /* Cached IODIR registers [A, B] */
    uint8_t     olat[2];    /* Cached output latch [A, B] */
    uint8_t     gppu[2];    /* Cached pull-up registers [A, B] */
} mcp23017_t;

/**
 * @brief Initialize the MCP23017
 * @param dev       Pointer to device handle (will be populated)
 * @param port      I2C port number (e.g. I2C_NUM_0) — ignored on P4
 * @param addr      7-bit I2C address (default 0x20)
 * @return ESP_OK on success, ESP_FAIL if device not responding
 */
#if defined(BOARD_JC4880P433)
esp_err_t mcp23017_init(mcp23017_t *dev, i2c_master_bus_handle_t bus, uint8_t addr);
#else
esp_err_t mcp23017_init(mcp23017_t *dev, i2c_port_t port, uint8_t addr);
#endif

/**
 * @brief Set pin direction
 * @param dev   Device handle
 * @param pin   Pin number 0-15 (0-7 = GPA, 8-15 = GPB)
 * @param mode  MCP23017_INPUT, MCP23017_OUTPUT, or MCP23017_INPUT_PULLUP
 */
esp_err_t mcp23017_pinMode(mcp23017_t *dev, uint8_t pin, uint8_t mode);

/**
 * @brief Write a digital value to an output pin
 * @param dev   Device handle
 * @param pin   Pin number 0-15
 * @param value 0 = LOW, non-zero = HIGH
 */
esp_err_t mcp23017_digitalWrite(mcp23017_t *dev, uint8_t pin, uint8_t value);

/**
 * @brief Read a digital value from a pin
 * @param dev   Device handle
 * @param pin   Pin number 0-15
 * @return 0 or 1, or -1 on error
 */
int mcp23017_digitalRead(mcp23017_t *dev, uint8_t pin);

/**
 * @brief Write a full 8-bit value to port A or B
 * @param dev   Device handle
 * @param port_ab  0 = port A (pins 0-7), 1 = port B (pins 8-15)
 * @param value    8-bit value to write
 */
esp_err_t mcp23017_writePort(mcp23017_t *dev, uint8_t port_ab, uint8_t value);

/**
 * @brief Read a full 8-bit value from port A or B
 * @param dev   Device handle
 * @param port_ab  0 = port A, 1 = port B
 * @return 8-bit value, or -1 on error
 */
int mcp23017_readPort(mcp23017_t *dev, uint8_t port_ab);

/**
 * @brief Set all 16 pins as output and all LOW (safe reset)
 */
esp_err_t mcp23017_allOutputLow(mcp23017_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* MCP23017_H */
