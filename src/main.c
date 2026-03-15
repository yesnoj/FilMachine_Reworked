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

float sim_getTemperature(uint8_t sensorPin) {
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

    if (sensorPin == TEMPERATURE_BATH_PIN)
        return sim_water_temp;
    else if (sensorPin == TEMPERATURE_CHEMICAL_PIN)
        return sim_chem_temp;
    return SIM_AMBIENT;
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
 * Demo data generator — used when no config file found
 * ═══════════════════════════════════════════════ */
static void sim_add_step(processNode *proc, const char *name,
                         uint8_t mins, uint8_t secs,
                         uint8_t type, uint8_t source, uint8_t discard)
{
    stepNode *s = (stepNode *)allocateAndInitializeNode(STEP_NODE);
    if (!s) return;
    strncpy(s->step.stepDetails->stepNameString, name, MAX_PROC_NAME_LEN);
    s->step.stepDetails->timeMins = mins;
    s->step.stepDetails->timeSecs = secs;
    s->step.stepDetails->type     = type;
    s->step.stepDetails->source   = source;
    s->step.stepDetails->discardAfterProc = discard;
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
    strncpy(p1->process.processDetails->processNameString, "C41 Color", MAX_PROC_NAME_LEN);
    p1->process.processDetails->temp             = 38;
    p1->process.processDetails->tempTolerance    = 0.3f;
    p1->process.processDetails->isTempControlled = true;
    p1->process.processDetails->isPreferred      = true;
    p1->process.processDetails->filmType         = COLOR_FILM;
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
    strncpy(p2->process.processDetails->processNameString, "B&W Classic", MAX_PROC_NAME_LEN);
    p2->process.processDetails->temp             = 20;
    p2->process.processDetails->tempTolerance    = 0.5f;
    p2->process.processDetails->isTempControlled = true;
    p2->process.processDetails->isPreferred      = true;
    p2->process.processDetails->filmType         = BLACK_AND_WHITE_FILM;
    sim_add_step(p2, "Developer",  8, 0,  0, 0, 1);
    sim_add_step(p2, "Stop Bath",  1, 0,  0, 1, 0);
    sim_add_step(p2, "Fixer",      5, 0,  0, 2, 0);
    sim_add_step(p2, "Wash",       5, 0,  2, 3, 1);
    calculateTotalTime(p2);
    sim_add_process(list, p2);

    /* Process 3: "Quick Rinse" — no temp control */
    processNode *p3 = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    strncpy(p3->process.processDetails->processNameString, "Quick Rinse", MAX_PROC_NAME_LEN);
    p3->process.processDetails->temp             = 24;
    p3->process.processDetails->tempTolerance    = 1.0f;
    p3->process.processDetails->isTempControlled = false;
    p3->process.processDetails->isPreferred      = false;
    p3->process.processDetails->filmType         = BLACK_AND_WHITE_FILM;
    sim_add_step(p3, "Developer",  5, 0, 0, 0, 1);
    sim_add_step(p3, "Fixer",      3, 0, 0, 1, 0);
    sim_add_step(p3, "Rinse",      2, 0, 1, 3, 1);
    calculateTotalTime(p3);
    sim_add_process(list, p3);

    printf("[SIM] Generated %d demo processes\n", (int)list->size);
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

        usleep(5000); /* ~5ms */
    }

    return 0;
}
