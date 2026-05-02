/**
 * @file sensors.c
 * @brief Drivers for additional sensors (flow, water level, hall effect).
 *
 * All functions are guarded by HAS_xxx defines from the active board header,
 * so they compile to no-ops on boards that lack the sensor hardware.
 *
 * Sensors:
 *   - YF-S201 flow meter       (pulse counter on FLOW_METER_PIN)
 *   - XKC-Y21 water level      (digital input on WATER_LEVEL_PIN / WATER_LEVEL_2_PIN)
 *   - KY-003  hall sensor       (digital input on HALL_SENSOR_PIN)
 */

#include "FilMachine.h"
#include "sensors.h"

#ifndef BOARD_SIMULATOR
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char __attribute__((unused)) *TAG = "sensors";

/* ═══════════════════════════════════════════════
 * YF-S201 Flow Meter
 * Outputs ~450 pulses per litre.
 * Flow rate (L/min) = pulse_count / 450 / elapsed_minutes
 * ═══════════════════════════════════════════════ */
#if HAS_FLOW_METER

#define FLOW_PULSES_PER_LITRE   450

static volatile uint32_t flow_pulse_count = 0;
static int64_t            flow_last_read_us = 0;

static void IRAM_ATTR flow_meter_isr(void *arg)
{
    flow_pulse_count++;
}

void sensors_flow_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << FLOW_METER_PIN,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_POSEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(FLOW_METER_PIN, flow_meter_isr, NULL));

    flow_pulse_count = 0;
    flow_last_read_us = esp_timer_get_time();
    ESP_LOGI(TAG, "Flow meter initialised on GPIO %d", FLOW_METER_PIN);
}

float sensors_flow_get_litres_per_min(void)
{
    int64_t now = esp_timer_get_time();
    int64_t elapsed_us = now - flow_last_read_us;
    if (elapsed_us <= 0) return 0.0f;

    uint32_t pulses = flow_pulse_count;
    flow_pulse_count = 0;
    flow_last_read_us = now;

    float elapsed_min = (float)elapsed_us / 60000000.0f;
    float litres = (float)pulses / FLOW_PULSES_PER_LITRE;
    return litres / elapsed_min;
}

uint32_t sensors_flow_get_pulse_count(void)
{
    return flow_pulse_count;
}

void sensors_flow_reset(void)
{
    flow_pulse_count = 0;
    flow_last_read_us = esp_timer_get_time();
}

#else /* !HAS_FLOW_METER */
void     sensors_flow_init(void) {}
float    sensors_flow_get_litres_per_min(void) { return 0.0f; }
uint32_t sensors_flow_get_pulse_count(void) { return 0; }
void     sensors_flow_reset(void) {}
#endif


/* ═══════════════════════════════════════════════
 * XKC-Y21 Water Level Sensor
 * Non-contact capacitive sensor.
 * Output: LOW = water detected, HIGH = no water.
 * ═══════════════════════════════════════════════ */
#if HAS_WATER_LEVEL_SENSOR

void sensors_water_level_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << WATER_LEVEL_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
#ifdef WATER_LEVEL_2_PIN
    io_conf.pin_bit_mask |= (1ULL << WATER_LEVEL_2_PIN);
#endif
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_LOGI(TAG, "Water level sensor initialised on GPIO %d", WATER_LEVEL_PIN);
}

bool sensors_water_level_detected(void)
{
    /* XKC-Y21 NPN: LOW when water is present */
    return gpio_get_level(WATER_LEVEL_PIN) == 0;
}

bool sensors_water_level_max_detected(void)
{
#ifdef WATER_LEVEL_2_PIN
    return gpio_get_level(WATER_LEVEL_2_PIN) == 0;
#else
    return false;
#endif
}

#else /* !HAS_WATER_LEVEL_SENSOR */
void sensors_water_level_init(void) {}
bool sensors_water_level_detected(void) { return true; }     /* assume OK */
bool sensors_water_level_max_detected(void) { return false; }
#endif


/* ═══════════════════════════════════════════════
 * KY-003 / A3144 Hall Effect Sensor
 * Detects magnetic field → confirms motor shaft is spinning.
 * Output: LOW = magnet detected, HIGH = no magnet.
 * ═══════════════════════════════════════════════ */
#if HAS_HALL_SENSOR

void sensors_hall_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << HALL_SENSOR_PIN,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_LOGI(TAG, "Hall sensor initialised on GPIO %d", HALL_SENSOR_PIN);
}

bool sensors_hall_magnet_detected(void)
{
    /* A3144: active LOW when magnet is near */
    return gpio_get_level(HALL_SENSOR_PIN) == 0;
}

#else /* !HAS_HALL_SENSOR */
void sensors_hall_init(void) {}
bool sensors_hall_magnet_detected(void) { return true; }  /* assume spinning */
#endif

#else /* BOARD_SIMULATOR */
/* ═══════════════════════════════════════════════
 * Simulator stubs — return plausible values
 * ═══════════════════════════════════════════════ */
void     sensors_flow_init(void) {}
float    sensors_flow_get_litres_per_min(void) { return 2.5f; }
uint32_t sensors_flow_get_pulse_count(void) { return 0; }
void     sensors_flow_reset(void) {}

void sensors_water_level_init(void) {}
bool sensors_water_level_detected(void) { return true; }
bool sensors_water_level_max_detected(void) { return false; }

void sensors_hall_init(void) {}
bool sensors_hall_magnet_detected(void) { return true; }
#endif /* BOARD_SIMULATOR */
