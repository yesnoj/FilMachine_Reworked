/**
 * @file audio.c
 * ES8311 audio codec bring-up + simple tone/volume playback.
 *
 * Path: ESP32-P4 I2S (STD, 16-bit) -> ES8311 codec (I2C control) -> power amp (GPIO).
 * Built via the espressif/esp_codec_dev component. Board-only source (the simulator
 * CMakeLists compiles its own driver subset and never includes this file).
 */

#include "audio.h"
#include "board.h"

#include <math.h>
#include <string.h>

#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

/* I2C_NUM lives in FilMachine.h; keep a local fallback so this stays self-contained. */
#ifndef I2C_NUM
#define I2C_NUM   0
#endif

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_TAG           "audio"

static esp_codec_dev_handle_t s_dev    = NULL;
static uint8_t                s_volume = 60;   /* default output volume % */

esp_err_t audio_init(i2c_master_bus_handle_t bus)
{
    /* ── 1. I2S TX channel (master) ── */
    i2s_chan_handle_t tx = NULL;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan_cfg, &tx, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(AUDIO_TAG, "i2s_new_channel: %s", esp_err_to_name(err));
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK_PIN,
            .bclk = I2S_BCLK_PIN,
            .ws   = I2S_WS_PIN,
            .dout = I2S_DOUT_PIN,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    err = i2s_channel_init_std_mode(tx, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(AUDIO_TAG, "i2s_channel_init_std_mode: %s", esp_err_to_name(err));
        return err;
    }

    /* ── 2. esp_codec_dev interfaces ── */
    audio_codec_i2s_cfg_t i2s_if_cfg = {
        .port       = I2S_NUM_0,
        .tx_handle  = tx,
        .rx_handle  = NULL,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_if_cfg);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port       = I2C_NUM,
        .addr       = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = bus,
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    if (!data_if || !ctrl_if || !gpio_if) {
        ESP_LOGE(AUDIO_TAG, "codec interface creation failed");
        return ESP_FAIL;
    }

    /* ── 3. ES8311 codec (output only) ── */
    esp_codec_dev_hw_gain_t gain = { .pa_voltage = 5.0f, .codec_dac_voltage = 3.3f };
    es8311_codec_cfg_t es_cfg = {
        .ctrl_if       = ctrl_if,
        .gpio_if       = gpio_if,
        .codec_mode    = ESP_CODEC_DEV_WORK_MODE_DAC,   /* speaker out */
        .pa_pin        = AUDIO_PA_PIN,
        .pa_reverted   = false,                          /* PA enable is active-high */
        .master_mode   = false,
        .use_mclk      = true,
        .digital_mic   = false,
        .invert_mclk   = false,
        .invert_sclk   = false,
        .hw_gain       = gain,
    };
    const audio_codec_if_t *codec_if = es8311_codec_new(&es_cfg);
    if (codec_if == NULL) {
        ESP_LOGE(AUDIO_TAG, "es8311_codec_new failed");
        return ESP_FAIL;
    }

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if  = data_if,
    };
    s_dev = esp_codec_dev_new(&dev_cfg);
    if (s_dev == NULL) {
        ESP_LOGE(AUDIO_TAG, "esp_codec_dev_new failed");
        return ESP_FAIL;
    }

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel         = 2,
        .channel_mask    = 0,
        .sample_rate     = AUDIO_SAMPLE_RATE,
    };
    err = esp_codec_dev_open(s_dev, &fs);
    if (err != ESP_OK) {
        ESP_LOGE(AUDIO_TAG, "esp_codec_dev_open: %s", esp_err_to_name(err));
        return err;
    }

    esp_codec_dev_set_out_vol(s_dev, s_volume);
    ESP_LOGI(AUDIO_TAG, "ES8311 ready (I2S MCLK=%d BCLK=%d WS=%d DOUT=%d, PA=%d, vol=%d%%)",
             I2S_MCLK_PIN, I2S_BCLK_PIN, I2S_WS_PIN, I2S_DOUT_PIN, AUDIO_PA_PIN, s_volume);
    return ESP_OK;
}

void audio_set_volume(uint8_t vol_0_100)
{
    if (vol_0_100 > 100) vol_0_100 = 100;
    s_volume = vol_0_100;
    if (s_dev) {
        esp_codec_dev_set_out_vol(s_dev, (int)vol_0_100);
    }
}

void audio_play_tone(uint32_t freq_hz, uint32_t ms)
{
    if (s_dev == NULL || freq_hz == 0 || ms == 0) return;

    const uint32_t total_frames = (AUDIO_SAMPLE_RATE * ms) / 1000U;
    const float    step         = 2.0f * (float)M_PI * (float)freq_hz / (float)AUDIO_SAMPLE_RATE;

    int16_t  buf[128 * 2];   /* stereo interleaved, 128 frames per chunk */
    uint32_t done  = 0;
    float    phase = 0.0f;

    while (done < total_frames) {
        int frames = 0;
        for (; frames < 128 && done < total_frames; frames++, done++) {
            int16_t s = (int16_t)(sinf(phase) * 12000.0f);   /* ~0.37 full scale */
            phase += step;
            if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
            buf[frames * 2]     = s;   /* L */
            buf[frames * 2 + 1] = s;   /* R */
        }
        esp_codec_dev_write(s_dev, buf, frames * 2 * (int)sizeof(int16_t));
    }
}
