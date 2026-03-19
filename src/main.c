/**
 * main.c — FilMachine PC Simulator Entry Point
 *
 * This file REPLACES FilMachine.c (the ESP32 firmware entry point).
 * It defines the same global variables and provides stubs for
 * hardware-specific functions that were in FilMachine.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

#include "SDL2/SDL.h"
#include "FilMachine.h"
#include "src/indev/lv_indev_private.h"   /* access gesture_limit / gesture_min_velocity */

/* ═══════════════════════════════════════════════
 * Global variables (originally in FilMachine.c)
 * ═══════════════════════════════════════════════ */
struct gui_components   gui;
struct sys_components   sys;
bool                    stopMotorManTask = false;
uint8_t                 initErrors = 0;

/* ═══════════════════════════════════════════════
 * Stubs for functions defined in FilMachine.c
 * ═══════════════════════════════════════════════ */
void stopMotorTask(void) {
    stopMotorManTask = true;
    printf("[SIM] Motor task stopped\n");
}

void runMotorTask(void) {
    printf("[SIM] Motor task started (stub — not running in simulator)\n");
    stopMotorManTask = false;
}

/* ═══════════════════════════════════════════════
 * Weak stubs for hardware functions.
 * If accessories.c already defines them (even with empty #if 0 bodies),
 * the linker will prefer that version. Otherwise these are used.
 * ═══════════════════════════════════════════════ */
__attribute__((weak)) void stopMotor(uint8_t pin1, uint8_t pin2) {
    printf("[SIM] Motor stop\n");
}
__attribute__((weak)) void runMotorFW(uint8_t pin1, uint8_t pin2) {
    printf("[SIM] Motor forward\n");
}
__attribute__((weak)) void runMotorRV(uint8_t pin1, uint8_t pin2) {
    printf("[SIM] Motor reverse\n");
}
__attribute__((weak)) void setMotorSpeed(uint8_t pin, uint8_t spd) { }
__attribute__((weak)) void setMotorSpeedUp(uint8_t pin, uint8_t spd) { }
__attribute__((weak)) void setMotorSpeedDown(uint8_t pin, uint8_t spd) { }
__attribute__((weak)) void enableMotor(uint8_t pin) { }
__attribute__((weak)) void testPin(uint8_t pin) { }
__attribute__((weak)) void initializeRelayPins(void) { }
__attribute__((weak)) void initializeMotorPins(void) { }
__attribute__((weak)) void initializeTemperatureSensor(void) { }
__attribute__((weak)) void printTemperature(float temp) { }
__attribute__((weak)) void init_Pins_and_Buses(void) { }
__attribute__((weak)) void initMCP23017Pins(void) { }
__attribute__((weak)) void pwmLedTest(void) { }
__attribute__((weak)) void cleanRelayManager(uint8_t a, uint8_t b, uint8_t c, bool d) { }
__attribute__((weak)) void sendValueToRelay(uint8_t a, uint8_t b, bool c) { }
__attribute__((weak)) void rebootBoard(void) {
    printf("[SIM] Reboot requested — ignoring in simulator\n");
}

/* ═══════════════════════════════════════════════
 * Simulated temperature sensor & heater control
 * ═══════════════════════════════════════════════ */
static float sim_water_temp    = 20.0f;
static float sim_chem_temp     = 20.0f;
static bool  sim_heater_active = false;

#define SIM_HEAT_RATE   0.5f   /* °C per call when heater ON  */
#define SIM_COOL_RATE   0.1f   /* °C per call when heater OFF */
#define SIM_AMBIENT     20.0f  /* Room temperature             */

float sim_getTemperature(uint8_t sensorIndex) {
    /* Update simulation each time temperature is read */
    if (sim_heater_active) {
        sim_water_temp += SIM_HEAT_RATE;
        sim_chem_temp  += SIM_HEAT_RATE * 0.8f;  /* chemistry heats slower */
    } else {
        if (sim_water_temp > SIM_AMBIENT)
            sim_water_temp -= SIM_COOL_RATE;
        if (sim_chem_temp > SIM_AMBIENT)
            sim_chem_temp -= SIM_COOL_RATE;
    }

    /* Apply calibration offset to all temperature readings */
    extern struct gui_components gui;
    float offset = gui.page.settings.settingsParams.tempCalibOffset / 10.0f;

    if (sensorIndex == TEMPERATURE_SENSOR_BATH)
        return sim_water_temp + offset;
    else if (sensorIndex == TEMPERATURE_SENSOR_CHEMICAL)
        return sim_chem_temp + offset;
    return SIM_AMBIENT + offset;
}

void sim_setHeater(bool on) {
    sim_heater_active = on;
    printf("[SIM] Heater %s\n", on ? "ON" : "OFF");
}

void sim_resetTemperatures(void) {
    sim_water_temp    = SIM_AMBIENT;
    sim_chem_temp     = SIM_AMBIENT;
    sim_heater_active = false;
}

/* ═══════════════════════════════════════════════
 * Persistent alarm sound (SDL audio)
 * ═══════════════════════════════════════════════ */
static SDL_AudioDeviceID sim_audio_dev = 0;
static lv_timer_t *alarm_timer = NULL;

static void sim_init_audio(void) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        printf("[SIM] WARNING: SDL_INIT_AUDIO failed: %s\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec spec = {0};
    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 1;
    spec.samples = 2048;
    spec.callback = NULL;
    sim_audio_dev = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    if (sim_audio_dev) {
        SDL_PauseAudioDevice(sim_audio_dev, 0);
        printf("[SIM] Audio device initialized (ID: %u)\n", sim_audio_dev);
    } else {
        printf("[SIM] WARNING: Failed to open audio device: %s\n", SDL_GetError());
    }
}

void buzzer_beep(void) {
    if (!sim_audio_dev) return;

    int samples = 44100 * 200 / 1000; /* 200ms */
    int16_t *buf = malloc(samples * sizeof(int16_t));
    if (!buf) return;

    /* Generate 880Hz square wave */
    for (int i = 0; i < samples; i++) {
        buf[i] = (i / (44100/880) % 2) ? 8000 : -8000;
    }

    SDL_ClearQueuedAudio(sim_audio_dev);
    SDL_QueueAudio(sim_audio_dev, buf, samples * sizeof(int16_t));
    free(buf);
    printf("[SIM] BEEP!\n");
}

static void alarm_timer_cb(lv_timer_t *t) {
    (void)t;
    buzzer_beep();
}

void alarm_start_persistent(void) {
    buzzer_beep(); /* immediate beep */
    if (gui.page.settings.settingsParams.isPersistentAlarm) {
        if (alarm_timer == NULL) {
            alarm_timer = lv_timer_create(alarm_timer_cb, 10000, NULL);
        } else {
            lv_timer_resume(alarm_timer);
        }
    }
}

void alarm_stop(void) {
    if (alarm_timer != NULL) {
        lv_timer_pause(alarm_timer);
    }
}

bool alarm_is_active(void) {
    return alarm_timer != NULL && lv_timer_get_paused(alarm_timer) == false;
}

/* ═══════════════════════════════════════════════
 * Demo data generator — used when no config file found
 * ═══════════════════════════════════════════════ */
static void sim_add_step(processNode *proc, const char *name,
                         uint8_t mins, uint8_t secs,
                         uint8_t type, uint8_t source, uint8_t discard)
{
    stepNode *s = (stepNode *)allocateAndInitializeNode(STEP_NODE);
    if (!s) return;
    snprintf(s->step.stepDetails->data.stepNameString, sizeof(s->step.stepDetails->data.stepNameString), "%s", name);
    s->step.stepDetails->data.timeMins = mins;
    s->step.stepDetails->data.timeSecs = secs;
    s->step.stepDetails->data.type     = type;
    s->step.stepDetails->data.source   = source;
    s->step.stepDetails->data.discardAfterProc = discard;
    s->prev = NULL;
    s->next = NULL;

    stepList *list = &proc->process.processDetails->stepElementsList;
    if (list->start == NULL) {
        list->start = s;
    } else {
        list->end->next = s;
        s->prev = list->end;
    }
    list->end = s;
    list->size++;
}

static void sim_add_process(processList *list, processNode *p)
{
    p->prev = NULL;
    p->next = NULL;
    if (list->start == NULL) {
        list->start = p;
    } else {
        list->end->next = p;
        p->prev = list->end;
    }
    list->end = p;
    list->size++;
}

static void sim_generate_demo_data(void)
{
    processList *list = &gui.page.processes.processElementsList;

    /* Process 1: "C41 Color" — temperature controlled */
    processNode *p1 = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    snprintf(p1->process.processDetails->data.processNameString, sizeof(p1->process.processDetails->data.processNameString), "%s", "C41 Color");
    p1->process.processDetails->data.temp             = 38;
    p1->process.processDetails->data.tempTolerance    = 0.3f;
    p1->process.processDetails->data.isTempControlled = true;
    p1->process.processDetails->data.isPreferred      = true;
    p1->process.processDetails->data.filmType         = COLOR_FILM;
    sim_add_step(p1, "Pre-wash",   1, 0,  1, 3, 1);
    sim_add_step(p1, "Developer",  3, 15, 0, 0, 0);
    sim_add_step(p1, "Bleach",     6, 30, 0, 1, 0);
    sim_add_step(p1, "Wash",       3, 0,  2, 3, 1);
    sim_add_step(p1, "Fix",        6, 30, 0, 2, 0);
    sim_add_step(p1, "Final Wash", 3, 0,  2, 3, 1);
    calculateTotalTime(p1);
    sim_add_process(list, p1);

    /* Process 2: "B&W Classic" — temperature controlled */
    processNode *p2 = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    snprintf(p2->process.processDetails->data.processNameString, sizeof(p2->process.processDetails->data.processNameString), "%s", "B&W Classic");
    p2->process.processDetails->data.temp             = 20;
    p2->process.processDetails->data.tempTolerance    = 0.5f;
    p2->process.processDetails->data.isTempControlled = true;
    p2->process.processDetails->data.isPreferred      = true;
    p2->process.processDetails->data.filmType         = BLACK_AND_WHITE_FILM;
    sim_add_step(p2, "Developer",  8, 0,  0, 0, 1);
    sim_add_step(p2, "Stop Bath",  1, 0,  0, 1, 0);
    sim_add_step(p2, "Fixer",      5, 0,  0, 2, 0);
    sim_add_step(p2, "Wash",       5, 0,  2, 3, 1);
    calculateTotalTime(p2);
    sim_add_process(list, p2);

    /* Process 3: "Quick Rinse" — no temp control */
    processNode *p3 = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    snprintf(p3->process.processDetails->data.processNameString, sizeof(p3->process.processDetails->data.processNameString), "%s", "Quick Rinse");
    p3->process.processDetails->data.temp             = 24;
    p3->process.processDetails->data.tempTolerance    = 1.0f;
    p3->process.processDetails->data.isTempControlled = false;
    p3->process.processDetails->data.isPreferred      = false;
    p3->process.processDetails->data.filmType         = BLACK_AND_WHITE_FILM;
    sim_add_step(p3, "Developer",  5, 0, 0, 0, 1);
    sim_add_step(p3, "Fixer",      3, 0, 0, 1, 0);
    sim_add_step(p3, "Rinse",      2, 0, 1, 3, 1);
    calculateTotalTime(p3);
    sim_add_process(list, p3);

    printf("[SIM] Generated %d demo processes\n", (int)list->size);
}

/* ═══════════════════════════════════════════════
 * System queue drain — replaces the sysMan FreeRTOS task.
 *
 * On real hardware, sysMan() runs as a separate FreeRTOS task and
 * processes messages from sys.sysActionQ in a while(1) loop.
 * In the simulator there are no real threads, so we drain the
 * queue once per main-loop iteration instead.
 * ═══════════════════════════════════════════════ */
static void sim_drain_sysActionQ(void)
{
    uint16_t msg;
    /* Process ALL pending messages (there could be more than one) */
    while (xQueueReceive(sys.sysActionQ, &msg, 0)) {
        switch (msg) {

        case SAVE_PROCESS_CONFIG:
            printf("[SIM] sysAction: saving config → %s\n", FILENAME_SAVE);
            writeConfigFile(FILENAME_SAVE, false);
            break;

        case SAVE_MACHINE_STATS:
            printf("[SIM] sysAction: saving machine stats\n");
            writeMachineStats(&gui.page.tools.machineStats);
            break;

        case EXPORT_CFG:
            printf("[SIM] sysAction: export (backup) %s → %s\n",
                   FILENAME_SAVE, FILENAME_BACKUP);
            if (!copyAndRenameFile(FILENAME_SAVE, FILENAME_BACKUP)) {
                printf("[SIM] Export/backup FAILED\n");
            } else {
                printf("[SIM] Export/backup successful\n");
            }
            break;

        case RELOAD_CFG:
            printf("[SIM] sysAction: import (restore) %s → %s\n",
                   FILENAME_BACKUP, FILENAME_SAVE);
            if (copyAndRenameFile(FILENAME_BACKUP, FILENAME_SAVE)) {
                printf("[SIM] Import successful — reboot requested (ignored in sim)\n");
                rebootBoard();
            } else {
                printf("[SIM] Import FAILED — backup file missing?\n");
            }
            break;

        default:
            printf("[SIM] sysAction: unknown message 0x%04X\n", msg);
            break;
        }
    }
}

/* ═══════════════════════════════════════════════
 * Time helper
 * ═══════════════════════════════════════════════ */
static uint32_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/* ═══════════════════════════════════════════════
 * Main entry point
 * ═══════════════════════════════════════════════ */
int main(int argc, char *argv[]) {

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   FilMachine Simulator                   ║\n");
    printf("║   Resolution: %dx%d                    ║\n", LCD_H_RES, LCD_V_RES);
    printf("║   Mouse = Touch | Close window to quit   ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    srand((unsigned int)time(NULL));

    /* Initialize globals (zeroes out gui and sys structs) */
    initGlobals();

    /* Initialize LVGL */
    lv_init();

    /* Create SDL display (LVGL 9 built-in SDL driver) */
    lv_display_t *display = lv_sdl_window_create(LCD_H_RES, LCD_V_RES);
    if (!display) {
        fprintf(stderr, "ERROR: Failed to create SDL display!\n");
        return 1;
    }

    /* Initialize SDL audio for buzzer/alarm */
    sim_init_audio();

    /* Create SDL mouse input (simulates touchscreen) */
    lv_indev_t *mouse = lv_sdl_mouse_create();
    if (!mouse) {
        fprintf(stderr, "ERROR: Failed to create SDL mouse input!\n");
        return 1;
    }

    /* Lower gesture thresholds so short/slow swipes are detected more reliably.
       Defaults: gesture_limit=50, gesture_min_velocity=3 */
    mouse->gesture_limit        = 30;   /* was 50 — pixels of movement to trigger GESTURE */
    mouse->gesture_min_velocity = 2;    /* was 3  — min per-frame velocity to keep accumulating */

    /* Create FreeRTOS queues (stub implementation) */
    sys.sysActionQ = xQueueCreate(16, sizeof(uint16_t));
    sys.motorActionQ = xQueueCreate(16, sizeof(uint16_t));

    /* Create shared keyboard */
    create_keyboard();

    /* Launch the home page — the real FilMachine GUI! */
    homePage();

    /* Load saved configuration from sd/FilMachine.cfg */
    readConfigFile(FILENAME_SAVE, false);

    /* Sync settings widgets with the values just loaded from config */
    refreshSettingsUI();

    /* If no processes were loaded (missing or corrupt config), generate demo data */
    if (gui.page.processes.processElementsList.size == 0) {
        printf("[SIM] No config file found — generating demo processes\n");
        sim_generate_demo_data();
    } else {
        printf("[SIM] Loaded %d processes from config\n",
               (int)gui.page.processes.processElementsList.size);
    }

    printf("[SIM] GUI initialized — entering main loop\n");
    printf("[SIM] Use mouse to interact, close window to quit\n");

    /* Main loop */
    uint32_t last_tick = get_time_ms();

    while (1) {
        uint32_t now = get_time_ms();
        uint32_t elapsed = now - last_tick;
        last_tick = now;

        lv_tick_inc(elapsed);
        lv_timer_handler();

        /* Process any pending system actions (save, export, import, …) */
        sim_drain_sysActionQ();

        usleep(5000); /* ~5ms */
    }

    return 0;
}
