/**
 * @file audio.h
 * ES8311 audio codec (I2S output + power amplifier) — minimal tone/volume API.
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bring up the ES8311 codec on the shared I2C bus: I2S TX channel, codec device,
 * PA enable, initial volume. Returns ESP_OK on success. */
esp_err_t audio_init(i2c_master_bus_handle_t bus);

/* Set output volume, 0-100 %. */
void audio_set_volume(uint8_t vol_0_100);

/* Play a blocking sine tone (frequency in Hz, duration in ms). */
void audio_play_tone(uint32_t freq_hz, uint32_t ms);

#ifdef __cplusplus
}
#endif
