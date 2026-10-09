/**
 * @file film_loader.c
 *
 * Film loading cycle of the tank with the built-in loader (135 cassette or
 * 120 roll in the universal cradle).
 *
 *   1. WINDING  the reel motor turns slowly; the Hall sensor counts the four
 *               magnets of the reel pulley (4 pulses per reel turn).
 *   2. The reel stops by itself at the end of the film: the 135 cassette spool
 *      (or the tape joining the 120 film to its paper) holds the film and the
 *      round belt slips — it is the torque limiter. "Stopped" = no Hall pulse
 *      for 3x the average pulse interval (never less than LOAD_STALL_MIN_MS).
 *   3. CUTTING  the motor keeps pulling (the belt slips and keeps the film taut
 *               on the blade) while the servo raises the Λ blade carrier and
 *               lowers it again. After a manual "Cut now" (film still moving)
 *               the motor stops for the cut instead.
 *   4. TAIL     half a turn to pull the cut tail inside the tank, motor off.
 *   5. DONE     beep; the tank is ready.
 *
 * Safety: refusing to cut when the reel stops too early (clip not hooked /
 * film jammed), a maximum number of turns (tail not held), a global timeout
 * and a "no rotation at all" check right after the start. The user can stop
 * at any time or cut immediately ("Cut now", e.g. when the 120 tape shows).
 *
 * Every state change happens in filmLoaderTickAt(), called every 20 ms by a
 * dedicated task on the board (or an LVGL timer in the simulator). The public
 * commands only post requests, so the LVGL thread and the WebSocket handler
 * never touch the motor or the servo directly.
 *
 * In the simulator (and the test runner) the Hall sensor, the motor and the
 * servo are modelled here: the "reel" produces a pulse every s_simPulseMs while
 * the motor runs, until the film runs out (then it stalls until the cut).
 */
#include "FilMachine.h"
#include "sensors.h"
#include "servo.h"

extern struct gui_components gui;

#if defined(BOARD_JC4880P433)
#include "esp_timer.h"
#include "freertos/task.h"
#endif

/* ── Timing and limits ─────────────────────────────────────────────── */
#define LOAD_TICK_MS            20
#define LOAD_NOSPIN_MS          5000     /* no pulse at all after the start → motor/belt problem   */
#define LOAD_STALL_MIN_MS       2500     /* shortest pause accepted as "the reel has stopped"      */
#define LOAD_STALL_FACTOR       3        /* ... or this many average pulse intervals               */
#define LOAD_TIMEOUT_MS         180000   /* whole winding phase                                    */
#define LOAD_TAIL_PULSES        2        /* half a turn after the cut                              */
#define LOAD_TAIL_MS            4000
#define CUT_UP_MS               900      /* blade up (ramp), hold, down (ramp), then servo off     */
#define CUT_HOLD_MS             300
#define CUT_DOWN_MS             600
#define SERVO_SETTLE_MS         400
#define CUTTER_TEST_HOLD_MS     4000     /* "cutter test" position is held this long, then released */

/* Expected / minimum / maximum Hall pulses per format (4 pulses = 1 reel turn).
 * 135 x 36: about 9 turns. 120: about 5 turns. Tune after the first real rolls:
 * every result is logged (and saved in the SD boot log). */
static const uint16_t k_expected[2] = { 36, 20 };
static const uint16_t k_min[2]      = { 16,  8 };
static const uint16_t k_max[2]      = { 60, 36 };

/* ── State ────────────────────────────────────────────────────────── */
static volatile int      s_state   = LOAD_IDLE;
static volatile uint8_t  s_format  = LOAD_FMT_135;
static volatile uint16_t s_pulses  = 0;          /* since the start of this load */
static volatile uint32_t s_reqSeq  = 0;          /* bumped by every request (debug) */

/* requests posted by the public API, consumed by the tick */
static volatile bool     s_reqStart = false;
static volatile uint8_t  s_reqFormat = LOAD_FMT_135;
static volatile bool     s_reqStop  = false;
static volatile bool     s_reqCut   = false;
static volatile bool     s_reqCycle = false;
static volatile int16_t  s_reqAngle = -1;        /* cutter test: 0..180, -1 = none */

/* winding bookkeeping (tick only) */
static uint32_t s_pulseBase, s_t0, s_lastPulseMs, s_avgIvMs, s_tailT0;
static uint16_t s_tailP0;
static uint16_t s_cutPulses;       /* film length = pulses when the cut started */

/* servo sequencer (tick only) */
#define SEQ_NONE     0
#define SEQ_CUT      1   /* up, hold, down, settle → off */
#define SEQ_HOLD     2   /* cutter test: hold an angle, then off */
#define SEQ_RETRACT  3   /* back to rest, settle → off */
static int      s_seq = SEQ_NONE;
static uint32_t s_seqT0;
static uint16_t s_holdUs;

static bool     s_motorOn = false;
static bool     s_inited  = false;

/* ── Settings helpers ─────────────────────────────────────────────── */
static struct machineSettings *ld_settings(void) { return &gui.page.settings.settingsParams; }

static uint16_t rest_us(void)
{
    uint16_t v = ld_settings()->cutterRestUs;
    return (v >= SERVO_US_MIN_ABS && v <= SERVO_US_MAX_ABS) ? v : LOAD_CUTTER_REST_US_DEFAULT;
}

static uint16_t cut_us(void)
{
    uint16_t v = ld_settings()->cutterCutUs;
    return (v >= SERVO_US_MIN_ABS && v <= SERVO_US_MAX_ABS) ? v : LOAD_CUTTER_CUT_US_DEFAULT;
}

static uint8_t motor_duty(void)
{
    uint8_t pct = ld_settings()->loadSpeed;
    if (pct < 10 || pct > 100) pct = LOAD_SPEED_DEFAULT;
    return mapPercentageToValue(pct, 10, 100);
}

void filmLoaderApplyDefaults(struct machineSettings *s)
{
    s->loadSpeed    = LOAD_SPEED_DEFAULT;
    s->cutterRestUs = LOAD_CUTTER_REST_US_DEFAULT;
    s->cutterCutUs  = LOAD_CUTTER_CUT_US_DEFAULT;
}

/* ── Hardware abstraction ─────────────────────────────────────────── */
#if defined(BOARD_JC4880P433)

static uint32_t hal_pulses(void) { return sensors_hall_pulse_count(); }

static void hal_motor(bool on)
{
    if (on == s_motorOn) return;
    s_motorOn = on;
    if (on) motor_start_kicked(true, motor_duty());
    else    motor_set_stop();
}

static void hal_servo_us(uint16_t us) { servo_write_us(us); }
static void hal_servo_off(void)       { servo_off(); }

#else /* simulator + test runner: modelled reel, motor and servo */

static uint32_t s_simPulses = 0;        /* total "Hall" pulses produced by the model */
static uint32_t s_simLastPulseMs = 0;
static uint16_t s_simPulseMs = 400;     /* one pulse every 400 ms (fast, for demos) */
static uint16_t s_simFilmPulses = 0;    /* reel stalls after this many pulses (0 = by format) */
static bool     s_simCut = false;       /* film cut: the reel turns freely again */
static bool     s_simAutoTick = true;
static uint16_t s_simServoUs = 0;       /* 0 = no pulses */
static uint32_t s_simNow = 0;

static uint32_t hal_pulses(void) { return s_simPulses; }

static void hal_motor(bool on)
{
    if (on == s_motorOn) return;
    s_motorOn = on;
    s_simLastPulseMs = s_simNow;
    if (on) motor_set_forward(motor_duty());
    else    motor_set_stop();
}

static void hal_servo_us(uint16_t us) { s_simServoUs = us; }
static void hal_servo_off(void)       { s_simServoUs = 0; }

static void sim_model(uint32_t now)
{
    s_simNow = now;
    if (!s_motorOn) return;
    uint16_t film = s_simFilmPulses ? s_simFilmPulses : (uint16_t)(k_expected[s_format] + 1);
    bool turning = s_simCut || (s_simPulses - s_pulseBase) < film;
    if (turning && (now - s_simLastPulseMs) >= s_simPulseMs) {
        s_simPulses++;
        s_simLastPulseMs = now;
    }
}

void filmLoaderSimSetup(bool autoTick, uint16_t filmPulses, uint16_t pulseMs)
{
    s_simAutoTick   = autoTick;
    s_simFilmPulses = filmPulses;
    if (pulseMs) s_simPulseMs = pulseMs;
}

uint16_t filmLoaderSimServoUs(void) { return s_simServoUs; }
bool     filmLoaderSimMotorOn(void) { return s_motorOn; }

#endif

/* ── Servo sequencer ──────────────────────────────────────────────── */
static void seq_start(int seq, uint32_t now)
{
    s_seq = seq;
    s_seqT0 = now;
    if (seq == SEQ_RETRACT) hal_servo_us(rest_us());
}

static void seq_tick(uint32_t now)
{
    uint32_t t = now - s_seqT0;
    int32_t r = rest_us(), c = cut_us();
    switch (s_seq) {
    case SEQ_CUT:
        if (t < CUT_UP_MS) {
            hal_servo_us((uint16_t)(r + (c - r) * (int32_t)t / CUT_UP_MS));
        } else if (t < CUT_UP_MS + CUT_HOLD_MS) {
            hal_servo_us((uint16_t)c);
#if !defined(BOARD_JC4880P433)
            s_simCut = true;                       /* model: the blade went through the film */
#endif
        } else if (t < CUT_UP_MS + CUT_HOLD_MS + CUT_DOWN_MS) {
            uint32_t d = t - CUT_UP_MS - CUT_HOLD_MS;
            hal_servo_us((uint16_t)(c + (r - c) * (int32_t)d / CUT_DOWN_MS));
        } else if (t < CUT_UP_MS + CUT_HOLD_MS + CUT_DOWN_MS + SERVO_SETTLE_MS) {
            hal_servo_us((uint16_t)r);
        } else {
            hal_servo_off();
            s_seq = SEQ_NONE;
        }
        break;
    case SEQ_HOLD:
        hal_servo_us(s_holdUs);
        if (t >= CUTTER_TEST_HOLD_MS) seq_start(SEQ_RETRACT, now);
        break;
    case SEQ_RETRACT:
        hal_servo_us((uint16_t)r);
        if (t >= SERVO_SETTLE_MS + 200) { hal_servo_off(); s_seq = SEQ_NONE; }
        break;
    default:
        break;
    }
}

/* ── State machine ────────────────────────────────────────────────── */
static bool is_running(int st) { return st == LOAD_WINDING || st == LOAD_CUTTING || st == LOAD_TAIL; }

static void finish(int st, uint32_t now)
{
    hal_motor(false);
    if (s_seq == SEQ_CUT || s_seq == SEQ_HOLD) seq_start(SEQ_RETRACT, now);
    s_state = st;
    LV_LOG_USER("FILM LOAD: end state=%d format=%s pulses=%u (%u.%u turns, expected %u)",
                st, s_format == LOAD_FMT_120 ? "120" : "135", (unsigned)s_pulses,
                (unsigned)(s_pulses / LOAD_PULSES_PER_TURN), (unsigned)((s_pulses % LOAD_PULSES_PER_TURN) * 10 / LOAD_PULSES_PER_TURN),
                (unsigned)k_expected[s_format]);
}

static void begin_cut(uint32_t now, bool manual)
{
    LV_LOG_USER("FILM LOAD: %s cut after %u pulses", manual ? "manual" : "automatic", (unsigned)s_pulses);
    /* Automatic cut: the reel has stopped and the slipping belt keeps the film
     * taut, so the motor stays on. Manual cut: the film is still moving, so the
     * reel stops for the cut and restarts afterwards to pull the tail in. */
    if (manual) hal_motor(false);
    s_cutPulses = s_pulses;
    s_state = LOAD_CUTTING;
    seq_start(SEQ_CUT, now);
}

void filmLoaderTickAt(uint32_t now)
{
#if !defined(BOARD_JC4880P433)
    sim_model(now);
#endif
    /* ── requests ── */
    if (s_reqStop) {
        s_reqStop = false; s_reqStart = false; s_reqCut = false;
        if (is_running(s_state)) finish(LOAD_STOPPED, now);
        else if (s_seq != SEQ_NONE && s_seq != SEQ_RETRACT) seq_start(SEQ_RETRACT, now);
    }
    if (s_reqStart) {
        s_reqStart = false;
        if (!is_running(s_state)) {
            s_format      = s_reqFormat;
            s_pulses      = 0;
            s_pulseBase   = hal_pulses();
            s_t0          = now;
            s_lastPulseMs = now;
            s_avgIvMs     = 0;
            s_reqCut      = false;
#if !defined(BOARD_JC4880P433)
            s_simCut      = false;
#endif
            if (s_seq != SEQ_NONE) { hal_servo_us(rest_us()); s_seq = SEQ_NONE; }
            hal_servo_off();
            s_state = LOAD_WINDING;
            hal_motor(true);
            LV_LOG_USER("FILM LOAD: start format=%s duty=%u", s_format == LOAD_FMT_120 ? "120" : "135", (unsigned)motor_duty());
        }
    }
    if (s_reqAngle >= 0) {
        int16_t a = s_reqAngle; s_reqAngle = -1;
        if (!is_running(s_state)) {
            int32_t r = rest_us(), c = cut_us();
            s_holdUs = (uint16_t)(r + (c - r) * a / 180);
            seq_start(SEQ_HOLD, now);
        }
    }
    if (s_reqCycle) {
        s_reqCycle = false;
        if (!is_running(s_state)) seq_start(SEQ_CUT, now);    /* test cut: blade only, no motor */
    }

    /* ── winding ── */
    if (s_state == LOAD_WINDING) {
        uint16_t p = (uint16_t)(hal_pulses() - s_pulseBase);
        if (p != s_pulses) {
            if (s_pulses > 0) {
                uint32_t iv = now - s_lastPulseMs;
                s_avgIvMs = s_avgIvMs ? (s_avgIvMs * 3 + iv) / 4 : iv;
            }
            s_pulses = p;
            s_lastPulseMs = now;
        }
        uint32_t stall = s_avgIvMs * LOAD_STALL_FACTOR;
        if (stall < LOAD_STALL_MIN_MS) stall = LOAD_STALL_MIN_MS;

        if (s_reqCut) { s_reqCut = false; begin_cut(now, true); }
        else if (s_pulses == 0 && now - s_t0 >= LOAD_NOSPIN_MS) finish(LOAD_ERR_NOSPIN, now);
        else if (s_pulses > k_max[s_format])                     finish(LOAD_ERR_LONG, now);
        else if (now - s_t0 >= LOAD_TIMEOUT_MS)                  finish(LOAD_ERR_TIMEOUT, now);
        else if (s_pulses > 0 && now - s_lastPulseMs >= stall) {
            if (s_pulses < k_min[s_format]) finish(LOAD_ERR_EARLY, now);
            else                            begin_cut(now, false);
        }
    }

    /* ── servo ── */
    if (s_seq != SEQ_NONE) seq_tick(now);

    /* ── cut finished → pull the tail in ── */
    if (s_state == LOAD_CUTTING && s_seq == SEQ_NONE) {
        s_state  = LOAD_TAIL;
        s_tailT0 = now;
        s_tailP0 = (uint16_t)(hal_pulses() - s_pulseBase);
        hal_motor(true);                  /* no-op after an automatic cut (motor still on) */
    }
    if (s_state == LOAD_TAIL) {
        uint16_t p = (uint16_t)(hal_pulses() - s_pulseBase);
        if (p - s_tailP0 >= LOAD_TAIL_PULSES || now - s_tailT0 >= LOAD_TAIL_MS) {
            s_pulses = s_cutPulses;       /* report the film length, not the tail */
            finish(LOAD_DONE, now);
        }
    }
}

bool filmLoaderNeedsTick(void)
{
    return s_reqStart || s_reqStop || s_reqCut || s_reqCycle || s_reqAngle >= 0 ||
           is_running(s_state) || s_seq != SEQ_NONE;
}

/* ── Runner ───────────────────────────────────────────────────────── */
#if defined(BOARD_JC4880P433)
static void loader_task(void *arg)
{
    (void)arg;
    for (;;) {
        if (filmLoaderNeedsTick()) {
            filmLoaderTickAt((uint32_t)(esp_timer_get_time() / 1000));
            vTaskDelay(pdMS_TO_TICKS(LOAD_TICK_MS));
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));                /* idle: cheap poll */
        }
    }
}
#else
static lv_timer_t *s_simTimer = NULL;
static void loader_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (filmLoaderNeedsTick() || s_motorOn) filmLoaderTickAt(lv_tick_get());
}
#endif

void filmLoaderInit(void)
{
    if (s_inited) return;
    s_inited = true;
    servo_init();
    servo_off();
#if defined(BOARD_JC4880P433)
    xTaskCreate(loader_task, "filmload", 3072, NULL, 5, NULL);
#else
    if (s_simAutoTick && s_simTimer == NULL) s_simTimer = lv_timer_create(loader_timer_cb, LOAD_TICK_MS, NULL);
#endif
}

#if !defined(BOARD_JC4880P433)
/* Test runner: drive the state machine by hand (no LVGL timer). */
void filmLoaderSimManualTicks(void)
{
    s_simAutoTick = false;
    if (s_simTimer) { lv_timer_delete(s_simTimer); s_simTimer = NULL; }
    s_inited = true;
}

/* Test runner: back to a clean idle state. */
void filmLoaderSimReset(void)
{
    s_state = LOAD_IDLE; s_pulses = 0; s_seq = SEQ_NONE; s_motorOn = false;
    s_reqStart = s_reqStop = s_reqCut = s_reqCycle = false; s_reqAngle = -1;
    s_simPulses = 0; s_simCut = false; s_simServoUs = 0; s_simNow = 0; s_simLastPulseMs = 0;
}
#endif

/* ── Public commands (any thread) ─────────────────────────────────── */
bool filmLoaderStart(uint8_t format)
{
    filmLoaderInit();
    if (is_running(s_state) || s_reqStart) return false;
    s_reqFormat = (format == LOAD_FMT_120) ? LOAD_FMT_120 : LOAD_FMT_135;
    s_format    = s_reqFormat;     /* so the UI shows the right format at once */
    s_reqStart  = true;
    s_reqSeq++;
    return true;
}

void filmLoaderStop(void)        { filmLoaderInit(); s_reqStop = true; s_reqSeq++; }
void filmLoaderCutNow(void)      { filmLoaderInit(); if (is_running(s_state) || s_reqStart) s_reqCut = true; s_reqSeq++; }
void filmLoaderCutterCycle(void) { filmLoaderInit(); s_reqCycle = true; s_reqSeq++; }

void filmLoaderCutterTest(uint8_t angle)
{
    filmLoaderInit();
    s_reqAngle = (int16_t)(angle > 180 ? 180 : angle);
    s_reqSeq++;
}

/* ── Status (any thread) ──────────────────────────────────────────── */
int      filmLoaderState(void)    { return (s_reqStart && !is_running(s_state)) ? LOAD_WINDING : s_state; }
int      filmLoaderFormat(void)   { return s_format; }
uint16_t filmLoaderPulses(void)   { return s_pulses; }
uint16_t filmLoaderExpectedPulses(void) { return k_expected[s_format]; }
bool     filmLoaderBusy(void)     { return s_reqStart || is_running(s_state); }
bool     filmLoaderServoActive(void) { return s_seq != SEQ_NONE || s_reqCycle || s_reqAngle >= 0; }
