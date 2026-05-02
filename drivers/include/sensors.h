/**
 * @file sensors.h
 * @brief Public API for additional sensors (flow meter, water level, hall).
 *
 * Each sensor has an init function and one or more read functions.
 * On boards without the sensor hardware, the functions compile to
 * safe no-ops / default values.
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Flow meter (YF-S201) ── */
void     sensors_flow_init(void);
float    sensors_flow_get_litres_per_min(void);
uint32_t sensors_flow_get_pulse_count(void);
void     sensors_flow_reset(void);

/* ── Water level (XKC-Y21) ── */
void sensors_water_level_init(void);
bool sensors_water_level_detected(void);      /* true = water at min level */
bool sensors_water_level_max_detected(void);  /* true = water at max level */

/* ── Hall effect (KY-003 / A3144) ── */
void sensors_hall_init(void);
bool sensors_hall_magnet_detected(void);      /* true = motor shaft magnet near */

#ifdef __cplusplus
}
#endif

#endif /* SENSORS_H */
