/**
 * @file servo.c
 * @brief 50 Hz servo PWM on LEDC timer 3 / channel 3 (film-loader cutter).
 *
 * LEDC allocation on the JC4880P433:
 *   timer 0 / channel 0 = film motor ENA (5 kHz)
 *   timer 1 / channel 1 = LCD backlight
 *   timer 2 / channel 2 = pump ENA (16 kHz)
 *   timer 3 / channel 3 = cutter servo (50 Hz)   ← this file
 */
#include "FilMachine.h"
#include "servo.h"

#if !defined(BOARD_SIMULATOR) && defined(HAS_CUTTER_SERVO) && HAS_CUTTER_SERVO

#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "servo";

#define SERVO_LEDC_MODE      LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_TIMER     LEDC_TIMER_3
#define SERVO_LEDC_CHANNEL   LEDC_CHANNEL_3
#define SERVO_LEDC_RES       LEDC_TIMER_14_BIT
#define SERVO_LEDC_MAX       ((1u << 14) - 1u)

static bool s_ok = false;

void servo_init(void)
{
    if (s_ok) return;
    ledc_timer_config_t t = {
        .speed_mode      = SERVO_LEDC_MODE,
        .timer_num       = SERVO_LEDC_TIMER,
        .duty_resolution = SERVO_LEDC_RES,
        .freq_hz         = 1000000 / SERVO_PERIOD_US,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    if (ledc_timer_config(&t) != ESP_OK) { ESP_LOGE(TAG, "LEDC timer config failed"); return; }
    ledc_channel_config_t c = {
        .speed_mode = SERVO_LEDC_MODE,
        .channel    = SERVO_LEDC_CHANNEL,
        .timer_sel  = SERVO_LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = CUTTER_SERVO_PIN,
        .duty       = 0,
        .hpoint     = 0,
    };
    if (ledc_channel_config(&c) != ESP_OK) { ESP_LOGE(TAG, "LEDC channel config failed"); return; }
    s_ok = true;
    ESP_LOGI(TAG, "Cutter servo on GPIO %d (LEDC timer 3 / ch 3, 50 Hz)", CUTTER_SERVO_PIN);
}

void servo_write_us(uint16_t us)
{
    if (!s_ok) return;
    if (us < SERVO_US_MIN_ABS) us = SERVO_US_MIN_ABS;
    if (us > SERVO_US_MAX_ABS) us = SERVO_US_MAX_ABS;
    uint32_t duty = ((uint32_t)us * (SERVO_LEDC_MAX + 1u)) / SERVO_PERIOD_US;
    ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
    ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
}

void servo_off(void)
{
    if (!s_ok) return;
    ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, 0);
    ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
}

bool servo_available(void) { return s_ok; }

#else  /* simulator / board without the servo */

void servo_init(void) {}
void servo_write_us(uint16_t us) { (void)us; }
void servo_off(void) {}
bool servo_available(void) { return false; }

#endif
