/**
 * @file servo.h
 * @brief Hobby-servo output (50 Hz PWM) for the film-loader cutter.
 *
 * On the JC4880P433 the servo signal is CUTTER_SERVO_PIN (GPIO 28, JP1 pin 21),
 * driven by LEDC timer 3 / channel 3. On boards without HAS_CUTTER_SERVO (and in
 * the simulator) every function is a no-op.
 */
#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SERVO_PERIOD_US   20000   /* 50 Hz */
#define SERVO_US_MIN_ABS  400     /* hard limits accepted by servo_write_us() */
#define SERVO_US_MAX_ABS  2600

/** Configure the LEDC timer/channel. The output stays LOW (no pulses). */
void servo_init(void);

/** Output a pulse of `us` microseconds every 20 ms (clamped to the hard limits). */
void servo_write_us(uint16_t us);

/** Stop the pulses (signal held LOW): the servo goes limp and stays quiet. */
void servo_off(void);

/** true after servo_init() succeeded on real hardware. */
bool servo_available(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVO_H */
