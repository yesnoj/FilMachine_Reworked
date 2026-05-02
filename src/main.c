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
#include <stdint.h>
#include <string.h>

#include "page_splash.h"
#include "SDL2/SDL.h"
#include "FilMachine.h"
#include "ws_server.h"
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
 * Simulator UI hover debug
 * F2 toggles hover debug on/off. Right mouse button dumps the pointed object.
 * ═══════════════════════════════════════════════ */
static lv_obj_t *sim_debug_hover_label = NULL;
static lv_obj_t *sim_debug_hover_box = NULL;
static lv_obj_t *sim_debug_hovered_obj = NULL;
static bool sim_debug_right_btn_prev = false;
static bool sim_debug_enabled = false;
static bool sim_debug_f2_prev = false;

static const char *sim_debug_known_name(lv_obj_t *obj)
{
    if(obj == NULL) return NULL;
#define UI_DEBUG_ENTRY(ptr_expr, name_str) do { if((ptr_expr) == obj) return (name_str); } while(0)
#include "../ui_debug_registry.inc"
#undef UI_DEBUG_ENTRY
    return NULL;
}

static const char *sim_debug_dynamic_name(lv_obj_t *obj)
{
    static char name_buf[256];
    if(obj == NULL) return NULL;

    int proc_idx = 0;
    for(processNode *proc = gui.page.processes.processElementsList.start; proc != NULL; proc = proc->next, ++proc_idx) {

        

    }

    proc_idx = 0;
    for(processNode *proc = gui.page.processes.processElementsList.start; proc != NULL; proc = proc->next, ++proc_idx) {
        if(proc->process.processElement == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].processElement", proc_idx); return name_buf; }
        if(proc->process.preferredIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].preferredIcon", proc_idx); return name_buf; }
        if(proc->process.processElementSummary == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].processElementSummary", proc_idx); return name_buf; }
        if(proc->process.processName == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].processName", proc_idx); return name_buf; }
        if(proc->process.processTemp == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].processTemp", proc_idx); return name_buf; }
        if(proc->process.processTempIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].processTempIcon", proc_idx); return name_buf; }
        if(proc->process.processTime == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].processTime", proc_idx); return name_buf; }
        if(proc->process.processTimeIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].processTimeIcon", proc_idx); return name_buf; }
        if(proc->process.processTypeIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].processTypeIcon", proc_idx); return name_buf; }
        if(proc->process.deleteButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].deleteButton", proc_idx); return name_buf; }
        if(proc->process.deleteButtonLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].deleteButtonLabel", proc_idx); return name_buf; }
        sProcessDetail *pd = proc->process.processDetails;
        if(pd != NULL) {
            if(pd->processesContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processesContainer", proc_idx); return name_buf; }
            if(pd->processDetailParent == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processDetailParent", proc_idx); return name_buf; }
            if(pd->processDetailContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processDetailContainer", proc_idx); return name_buf; }
            if(pd->processDetailNameContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processDetailNameContainer", proc_idx); return name_buf; }
            if(pd->processStepsContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processStepsContainer", proc_idx); return name_buf; }
            if(pd->processInfoContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processInfoContainer", proc_idx); return name_buf; }
            if(pd->processTempControlContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTempControlContainer", proc_idx); return name_buf; }
            if(pd->processTempContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTempContainer", proc_idx); return name_buf; }
            if(pd->processToleranceContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processToleranceContainer", proc_idx); return name_buf; }
            if(pd->processColorOrBnWContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processColorOrBnWContainer", proc_idx); return name_buf; }
            if(pd->processTotalTimeContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTotalTimeContainer", proc_idx); return name_buf; }
            if(pd->processDetailNameTextArea == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processDetailNameTextArea", proc_idx); return name_buf; }
            if(pd->processDetailStepsLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processDetailStepsLabel", proc_idx); return name_buf; }
            if(pd->processDetailInfoLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processDetailInfoLabel", proc_idx); return name_buf; }
            if(pd->processDetailCloseButtonLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processDetailCloseButtonLabel", proc_idx); return name_buf; }
            if(pd->processTempControlLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTempControlLabel", proc_idx); return name_buf; }
            if(pd->processTempLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTempLabel", proc_idx); return name_buf; }
            if(pd->processTempControlSwitch == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTempControlSwitch", proc_idx); return name_buf; }
            if(pd->processTempUnitLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTempUnitLabel", proc_idx); return name_buf; }
            if(pd->processToleranceLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processToleranceLabel", proc_idx); return name_buf; }
            if(pd->processColorLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processColorLabel", proc_idx); return name_buf; }
            if(pd->processBnWLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processBnWLabel", proc_idx); return name_buf; }
            if(pd->processPreferredLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processPreferredLabel", proc_idx); return name_buf; }
            if(pd->processSaveLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processSaveLabel", proc_idx); return name_buf; }
            if(pd->processRunLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processRunLabel", proc_idx); return name_buf; }
            if(pd->processNewStepLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processNewStepLabel", proc_idx); return name_buf; }
            if(pd->processTotalTimeLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTotalTimeLabel", proc_idx); return name_buf; }
            if(pd->processTotalTimeValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTotalTimeValue", proc_idx); return name_buf; }
            if(pd->processDetailCloseButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processDetailCloseButton", proc_idx); return name_buf; }
            if(pd->processRunButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processRunButton", proc_idx); return name_buf; }
            if(pd->processSaveButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processSaveButton", proc_idx); return name_buf; }
            if(pd->processNewStepButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processNewStepButton", proc_idx); return name_buf; }
            if(pd->processTempTextArea == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processTempTextArea", proc_idx); return name_buf; }
            if(pd->processToleranceTextArea == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].details.processToleranceTextArea", proc_idx); return name_buf; }
            if(pd->checkup != NULL) {
                sCheckup *ck = pd->checkup;
                if(ck->checkupParent == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupParent", proc_idx); return name_buf; }
                if(ck->checkupContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupContainer", proc_idx); return name_buf; }
                if(ck->checkupNextStepsContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupNextStepsContainer", proc_idx); return name_buf; }
                if(ck->checkupProcessNameContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupProcessNameContainer", proc_idx); return name_buf; }
                if(ck->checkupStepContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStepContainer", proc_idx); return name_buf; }
                if(ck->checkupWaterFillContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupWaterFillContainer", proc_idx); return name_buf; }
                if(ck->checkupReachTempContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupReachTempContainer", proc_idx); return name_buf; }
                if(ck->checkupTankAndFilmContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTankAndFilmContainer", proc_idx); return name_buf; }
                if(ck->checkupStepSourceContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStepSourceContainer", proc_idx); return name_buf; }
                if(ck->checkupTempControlContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTempControlContainer", proc_idx); return name_buf; }
                if(ck->checkupWaterTempContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupWaterTempContainer", proc_idx); return name_buf; }
                if(ck->checkupNextStepContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupNextStepContainer", proc_idx); return name_buf; }
                if(ck->checkupSelectTankChemistryContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupSelectTankChemistryContainer", proc_idx); return name_buf; }
                if(ck->checkupFillWaterContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupFillWaterContainer", proc_idx); return name_buf; }
                if(ck->checkupTargetTempsContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetTempsContainer", proc_idx); return name_buf; }
                if(ck->checkupTargetTempContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetTempContainer", proc_idx); return name_buf; }
                if(ck->checkupTargetWaterTempContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetWaterTempContainer", proc_idx); return name_buf; }
                if(ck->checkupTargetChemistryTempContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetChemistryTempContainer", proc_idx); return name_buf; }
                if(ck->checkupTankIsPresentContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTankIsPresentContainer", proc_idx); return name_buf; }
                if(ck->checkupFilmRotatingContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupFilmRotatingContainer", proc_idx); return name_buf; }
                if(ck->checkupFilmInPositionContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupFilmInPositionContainer", proc_idx); return name_buf; }
                if(ck->checkupProcessingContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupProcessingContainer", proc_idx); return name_buf; }
                if(ck->checkupTankSizeLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTankSizeLabel", proc_idx); return name_buf; }
                if(ck->checkupChemistryVolumeLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupChemistryVolumeLabel", proc_idx); return name_buf; }
                if(ck->checkupNextStepsLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupNextStepsLabel", proc_idx); return name_buf; }
                if(ck->checkupWaterFillLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupWaterFillLabel", proc_idx); return name_buf; }
                if(ck->checkupReachTempLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupReachTempLabel", proc_idx); return name_buf; }
                if(ck->checkupTankAndFilmLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTankAndFilmLabel", proc_idx); return name_buf; }
                if(ck->checkupMachineWillDoLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupMachineWillDoLabel", proc_idx); return name_buf; }
                if(ck->checkupCloseButtonLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupCloseButtonLabel", proc_idx); return name_buf; }
                if(ck->checkupStepSourceLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStepSourceLabel", proc_idx); return name_buf; }
                if(ck->checkupTempControlLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTempControlLabel", proc_idx); return name_buf; }
                if(ck->checkupWaterTempLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupWaterTempLabel", proc_idx); return name_buf; }
                if(ck->checkupNextStepLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupNextStepLabel", proc_idx); return name_buf; }
                if(ck->checkupStopAfterButtonLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStopAfterButtonLabel", proc_idx); return name_buf; }
                if(ck->checkupStopNowButtonLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStopNowButtonLabel", proc_idx); return name_buf; }
                if(ck->checkupStartButtonLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStartButtonLabel", proc_idx); return name_buf; }
                if(ck->checkupProcessReadyLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupProcessReadyLabel", proc_idx); return name_buf; }
                if(ck->checkupSelectBeforeStartLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupSelectBeforeStartLabel", proc_idx); return name_buf; }
                if(ck->checkupFillWaterLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupFillWaterLabel", proc_idx); return name_buf; }
                if(ck->checkupSkipButtonLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupSkipButtonLabel", proc_idx); return name_buf; }
                if(ck->checkupTargetTempLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetTempLabel", proc_idx); return name_buf; }
                if(ck->checkupTargetWaterTempLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetWaterTempLabel", proc_idx); return name_buf; }
                if(ck->checkupTargetChemistryTempLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetChemistryTempLabel", proc_idx); return name_buf; }
                if(ck->checkupTankIsPresentLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTankIsPresentLabel", proc_idx); return name_buf; }
                if(ck->checkupFilmRotatingLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupFilmRotatingLabel", proc_idx); return name_buf; }
                if(ck->checkupProcessCompleteLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupProcessCompleteLabel", proc_idx); return name_buf; }
                if(ck->checkupTargetTempValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetTempValue", proc_idx); return name_buf; }
                if(ck->checkupTargetToleranceTempValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetToleranceTempValue", proc_idx); return name_buf; }
                if(ck->checkupTargetWaterTempValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetWaterTempValue", proc_idx); return name_buf; }
                if(ck->checkupTargetChemistryTempValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTargetChemistryTempValue", proc_idx); return name_buf; }
                if(ck->checkupHeaterStatusLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupHeaterStatusLabel", proc_idx); return name_buf; }
                if(ck->checkupTempTimeoutLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTempTimeoutLabel", proc_idx); return name_buf; }
                if(ck->checkupStepSourceValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStepSourceValue", proc_idx); return name_buf; }
                if(ck->checkupTempControlValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTempControlValue", proc_idx); return name_buf; }
                if(ck->checkupWaterTempValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupWaterTempValue", proc_idx); return name_buf; }
                if(ck->checkupNextStepValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupNextStepValue", proc_idx); return name_buf; }
                if(ck->checkupProcessNameValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupProcessNameValue", proc_idx); return name_buf; }
                if(ck->checkupTankIsPresentValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTankIsPresentValue", proc_idx); return name_buf; }
                if(ck->checkupFilmRotatingValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupFilmRotatingValue", proc_idx); return name_buf; }
                if(ck->checkupStepTimeLeftValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStepTimeLeftValue", proc_idx); return name_buf; }
                if(ck->checkupProcessTimeLeftValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupProcessTimeLeftValue", proc_idx); return name_buf; }
                if(ck->checkupStepNameValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStepNameValue", proc_idx); return name_buf; }
                if(ck->checkupStepKindValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStepKindValue", proc_idx); return name_buf; }
                if(ck->checkupWaterFillStatusIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupWaterFillStatusIcon", proc_idx); return name_buf; }
                if(ck->checkupReachTempStatusIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupReachTempStatusIcon", proc_idx); return name_buf; }
                if(ck->checkupTankAndFilmStatusIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTankAndFilmStatusIcon", proc_idx); return name_buf; }
                if(ck->lowVolumeChemRadioButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.lowVolumeChemRadioButton", proc_idx); return name_buf; }
                if(ck->highVolumeChemRadioButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.highVolumeChemRadioButton", proc_idx); return name_buf; }
                if(ck->checkupSkipButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupSkipButton", proc_idx); return name_buf; }
                if(ck->checkupStartButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStartButton", proc_idx); return name_buf; }
                if(ck->checkupStopAfterButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStopAfterButton", proc_idx); return name_buf; }
                if(ck->checkupStopNowButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupStopNowButton", proc_idx); return name_buf; }
                if(ck->checkupCloseButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupCloseButton", proc_idx); return name_buf; }
                if(ck->stepArc == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.stepArc", proc_idx); return name_buf; }
                if(ck->processArc == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.processArc", proc_idx); return name_buf; }
                if(ck->pumpArc == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.pumpArc", proc_idx); return name_buf; }
                if(ck->checkupTankSizeTextArea == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupTankSizeTextArea", proc_idx); return name_buf; }
                if(ck->checkupVolumeTextArea == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].checkup.checkupVolumeTextArea", proc_idx); return name_buf; }
            }

            int step_idx = 0;
            for(stepNode *step = pd->stepElementsList.start; step != NULL; step = step->next, ++step_idx) {
                if(step->step.stepElement == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].stepElement", proc_idx, step_idx); return name_buf; }
                if(step->step.stepElementSummary == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].stepElementSummary", proc_idx, step_idx); return name_buf; }
                if(step->step.stepName == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].stepName", proc_idx, step_idx); return name_buf; }
                if(step->step.stepTime == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].stepTime", proc_idx, step_idx); return name_buf; }
                if(step->step.stepTimeIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].stepTimeIcon", proc_idx, step_idx); return name_buf; }
                if(step->step.stepTypeIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].stepTypeIcon", proc_idx, step_idx); return name_buf; }
                if(step->step.discardAfterIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].discardAfterIcon", proc_idx, step_idx); return name_buf; }
                if(step->step.sourceLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].sourceLabel", proc_idx, step_idx); return name_buf; }
                if(step->step.deleteButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].deleteButton", proc_idx, step_idx); return name_buf; }
                if(step->step.deleteButtonLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].deleteButtonLabel", proc_idx, step_idx); return name_buf; }
                if(step->step.editButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].editButton", proc_idx, step_idx); return name_buf; }
                if(step->step.editButtonLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].editButtonLabel", proc_idx, step_idx); return name_buf; }
                sStepDetail *sd = step->step.stepDetails;
                if(sd != NULL) {
                    if(sd->stepDetailParent == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDetailParent", proc_idx, step_idx); return name_buf; }
                    if(sd->mBoxStepPopupTitleLine == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.mBoxStepPopupTitleLine", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDetailNameContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDetailNameContainer", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDetailContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDetailContainer", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDurationContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDurationContainer", proc_idx, step_idx); return name_buf; }
                    if(sd->stepTypeContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepTypeContainer", proc_idx, step_idx); return name_buf; }
                    if(sd->stepSourceContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepSourceContainer", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDiscardAfterContainer == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDiscardAfterContainer", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDetailLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDetailLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDetailNamelLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDetailNamelLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDurationLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDurationLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDurationMinLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDurationMinLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepSaveLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepSaveLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepCancelLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepCancelLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepTypeLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepTypeLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepSourceLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepSourceLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepTypeHelpIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepTypeHelpIcon", proc_idx, step_idx); return name_buf; }
                    if(sd->stepSourceTempLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepSourceTempLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDiscardAfterLabel == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDiscardAfterLabel", proc_idx, step_idx); return name_buf; }
                    if(sd->stepSourceTempHelpIcon == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepSourceTempHelpIcon", proc_idx, step_idx); return name_buf; }
                    if(sd->stepSourceTempValue == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepSourceTempValue", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDiscardAfterSwitch == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDiscardAfterSwitch", proc_idx, step_idx); return name_buf; }
                    if(sd->stepSaveButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepSaveButton", proc_idx, step_idx); return name_buf; }
                    if(sd->stepCancelButton == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepCancelButton", proc_idx, step_idx); return name_buf; }
                    if(sd->stepSourceDropDownList == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepSourceDropDownList", proc_idx, step_idx); return name_buf; }
                    if(sd->stepTypeDropDownList == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepTypeDropDownList", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDetailSecTextArea == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDetailSecTextArea", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDetailMinTextArea == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDetailMinTextArea", proc_idx, step_idx); return name_buf; }
                    if(sd->stepDetailNameTextArea == obj) { snprintf(name_buf, sizeof(name_buf), "process[%d].step[%d].details.stepDetailNameTextArea", proc_idx, step_idx); return name_buf; }
                }
            }
        }
    }

    /* ---- Check the temporary step node (New Step popup, not yet in list) ---- */
    if(gui.tempStepNode != NULL) {
        sStepDetail *sd = gui.tempStepNode->step.stepDetails;
        if(sd != NULL && sd->stepDetailParent != NULL) {
            /* Determine process index for the label */
            int tmp_proc_idx = 0;
            processNode *tmp_parent = sd->parentProcess;
            if(tmp_parent != NULL) {
                int idx = 0;
                for(processNode *p = gui.page.processes.processElementsList.start; p != NULL; p = p->next, ++idx) {
                    if(p == tmp_parent) { tmp_proc_idx = idx; break; }
                }
            }
            /* Check whether this temp node is already in the step list (edit mode).
               If so, the loop above already handled it – skip to avoid duplicates. */
            bool already_in_list = false;
            if(tmp_parent != NULL && tmp_parent->process.processDetails != NULL) {
                for(stepNode *s = tmp_parent->process.processDetails->stepElementsList.start; s != NULL; s = s->next) {
                    if(s == gui.tempStepNode) { already_in_list = true; break; }
                }
            }
            if(!already_in_list) {
                const char *prefix_fmt = "process[%d].newStep.details.";
                char prefix[80];
                snprintf(prefix, sizeof(prefix), prefix_fmt, tmp_proc_idx);

                #define TEMP_STEP_CHECK(member, label) \
                    if(sd->member == obj) { snprintf(name_buf, sizeof(name_buf), "%s%s", prefix, label); return name_buf; }

                TEMP_STEP_CHECK(stepDetailParent, "stepDetailParent")
                TEMP_STEP_CHECK(mBoxStepPopupTitleLine, "mBoxStepPopupTitleLine")
                TEMP_STEP_CHECK(stepDetailContainer, "stepDetailContainer")
                TEMP_STEP_CHECK(stepDetailNameContainer, "stepDetailNameContainer")
                TEMP_STEP_CHECK(stepDurationContainer, "stepDurationContainer")
                TEMP_STEP_CHECK(stepTypeContainer, "stepTypeContainer")
                TEMP_STEP_CHECK(stepSourceContainer, "stepSourceContainer")
                TEMP_STEP_CHECK(stepDiscardAfterContainer, "stepDiscardAfterContainer")
                TEMP_STEP_CHECK(stepDetailLabel, "stepDetailLabel")
                TEMP_STEP_CHECK(stepDetailNamelLabel, "stepDetailNamelLabel")
                TEMP_STEP_CHECK(stepDurationLabel, "stepDurationLabel")
                TEMP_STEP_CHECK(stepDurationMinLabel, "stepDurationMinLabel")
                TEMP_STEP_CHECK(stepSaveLabel, "stepSaveLabel")
                TEMP_STEP_CHECK(stepCancelLabel, "stepCancelLabel")
                TEMP_STEP_CHECK(stepTypeLabel, "stepTypeLabel")
                TEMP_STEP_CHECK(stepSourceLabel, "stepSourceLabel")
                TEMP_STEP_CHECK(stepTypeHelpIcon, "stepTypeHelpIcon")
                TEMP_STEP_CHECK(stepSourceTempLabel, "stepSourceTempLabel")
                TEMP_STEP_CHECK(stepDiscardAfterLabel, "stepDiscardAfterLabel")
                TEMP_STEP_CHECK(stepSourceTempHelpIcon, "stepSourceTempHelpIcon")
                TEMP_STEP_CHECK(stepSourceTempValue, "stepSourceTempValue")
                TEMP_STEP_CHECK(stepDiscardAfterSwitch, "stepDiscardAfterSwitch")
                TEMP_STEP_CHECK(stepSaveButton, "stepSaveButton")
                TEMP_STEP_CHECK(stepCancelButton, "stepCancelButton")
                TEMP_STEP_CHECK(stepSourceDropDownList, "stepSourceDropDownList")
                TEMP_STEP_CHECK(stepTypeDropDownList, "stepTypeDropDownList")
                TEMP_STEP_CHECK(stepDetailSecTextArea, "stepDetailSecTextArea")
                TEMP_STEP_CHECK(stepDetailMinTextArea, "stepDetailMinTextArea")
                TEMP_STEP_CHECK(stepDetailNameTextArea, "stepDetailNameTextArea")

                #undef TEMP_STEP_CHECK
            }
        }
    }

    return NULL;
}


static bool sim_debug_obj_is_visible(lv_obj_t *obj)
{
    if(obj == NULL) return false;
    if(lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;
    return true;
}

static bool sim_debug_point_in_obj(lv_obj_t *obj, int32_t x, int32_t y)
{
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    return x >= a.x1 && x <= a.x2 && y >= a.y1 && y <= a.y2;
}

static bool sim_debug_obj_is_debug_overlay(lv_obj_t *obj)
{
    return obj == sim_debug_hover_label || obj == sim_debug_hover_box;
}

static lv_obj_t *sim_debug_find_obj_at(lv_obj_t *parent, int32_t x, int32_t y)
{
    if(parent == NULL || !sim_debug_obj_is_visible(parent)) return NULL;
    if(sim_debug_obj_is_debug_overlay(parent)) return NULL;
    if(!sim_debug_point_in_obj(parent, x, y)) return NULL;

    int32_t count = (int32_t)lv_obj_get_child_count(parent);
    for(int32_t i = count - 1; i >= 0; --i) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        lv_obj_t *hit = sim_debug_find_obj_at(child, x, y);
        if(hit) return hit;
    }

    return parent;
}

static lv_obj_t *sim_debug_find_hovered_obj(lv_display_t *display, int32_t x, int32_t y)
{
    /*
     * Search in visual Z order, from the highest overlay layer down to the
     * active screen. This is essential for roller/dropdown/popup objects,
     * otherwise the underlying page gets matched first.
     */
    lv_obj_t *roots[] = {
        lv_layer_top(),
        lv_display_get_layer_sys(display),
        lv_screen_active()
    };

    for(size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); ++i) {
        lv_obj_t *root = roots[i];
        if(root == NULL) continue;

        /* First search children from front to back (last child = topmost). */
        int32_t count = (int32_t)lv_obj_get_child_count(root);
        for(int32_t j = count - 1; j >= 0; --j) {
            lv_obj_t *child = lv_obj_get_child(root, j);
            lv_obj_t *hit = sim_debug_find_obj_at(child, x, y);
            if(hit) return hit;
        }

        /* Accept the root itself only for the active screen.
           Never stop on layer_top/layer_sys root, otherwise they mask popups. */
        if(root == lv_screen_active() && sim_debug_find_obj_at(root, x, y) == root) {
            return root;
        }
    }

    return NULL;
}

static int32_t sim_debug_child_index(lv_obj_t *obj)
{
    lv_obj_t *parent = lv_obj_get_parent(obj);
    if(parent == NULL) return -1;
    uint32_t count = lv_obj_get_child_count(parent);
    for(uint32_t i = 0; i < count; ++i) {
        if(lv_obj_get_child(parent, i) == obj) return (int32_t)i;
    }
    return -1;
}

static void sim_debug_build_path(lv_obj_t *obj, char *buf, size_t buf_sz)
{
    if(buf_sz == 0) return;
    buf[0] = '\0';
    if(obj == NULL) {
        snprintf(buf, buf_sz, "<null>");
        return;
    }

    const char *known = sim_debug_known_name(obj);
    if(known == NULL) known = sim_debug_dynamic_name(obj);
    if(known) {
        snprintf(buf, buf_sz, "%s", known);
        return;
    }

    lv_obj_t *stack[64];
    size_t n = 0;
    for(lv_obj_t *cur = obj; cur != NULL && n < 64; cur = lv_obj_get_parent(cur)) {
        stack[n++] = cur;
    }

    for(size_t rev = n; rev > 0; --rev) {
        lv_obj_t *cur = stack[rev - 1];
        const char *cur_known = sim_debug_known_name(cur);
        if(cur_known == NULL) cur_known = sim_debug_dynamic_name(cur);
        char part[96];
        if(cur_known) {
            snprintf(part, sizeof(part), "%s", cur_known);
        } else if(lv_obj_get_parent(cur) == NULL) {
            if(cur == lv_screen_active()) snprintf(part, sizeof(part), "screen_active");
            else if(cur == lv_layer_top()) snprintf(part, sizeof(part), "layer_top");
            else if(cur == lv_display_get_layer_sys(lv_display_get_default())) snprintf(part, sizeof(part), "layer_sys");
            else snprintf(part, sizeof(part), "root");
        } else {
            snprintf(part, sizeof(part), "child[%ld]", (long)sim_debug_child_index(cur));
        }

        if(buf[0] != '\0') strncat(buf, "/", buf_sz - strlen(buf) - 1);
        strncat(buf, part, buf_sz - strlen(buf) - 1);
    }
}

static void sim_debug_ensure_overlay(void)
{
    if(sim_debug_hover_label == NULL) {
        sim_debug_hover_label = lv_label_create(lv_layer_top());
        lv_obj_set_style_bg_opa(sim_debug_hover_label, LV_OPA_80, 0);
        lv_obj_set_style_bg_color(sim_debug_hover_label, lv_color_black(), 0);
        lv_obj_set_style_text_color(sim_debug_hover_label, lv_color_white(), 0);
        lv_obj_set_style_pad_all(sim_debug_hover_label, 4, 0);
        lv_obj_set_style_radius(sim_debug_hover_label, 4, 0);
        lv_obj_set_style_border_width(sim_debug_hover_label, 1, 0);
        lv_obj_set_style_border_color(sim_debug_hover_label, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_add_flag(sim_debug_hover_label, LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(sim_debug_hover_label);
    }

    if(sim_debug_hover_box == NULL) {
        sim_debug_hover_box = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(sim_debug_hover_box);
        lv_obj_set_style_bg_opa(sim_debug_hover_box, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sim_debug_hover_box, 2, 0);
        lv_obj_set_style_border_color(sim_debug_hover_box, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_add_flag(sim_debug_hover_box, LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(sim_debug_hover_box, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_move_foreground(sim_debug_hover_box);
    }
}

static void sim_debug_hide_overlay(void)
{
    if(sim_debug_hover_label) lv_obj_add_flag(sim_debug_hover_label, LV_OBJ_FLAG_HIDDEN);
    if(sim_debug_hover_box) lv_obj_add_flag(sim_debug_hover_box, LV_OBJ_FLAG_HIDDEN);
}

static void sim_debug_handle_toggle_key(void)
{
    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    bool f2_down = keys[SDL_SCANCODE_F2] != 0;

    if(f2_down && !sim_debug_f2_prev) {
        sim_debug_enabled = !sim_debug_enabled;
        printf("[SIM] UI debug %s (F2 toggle)\n", sim_debug_enabled ? "ENABLED" : "DISABLED");
        fflush(stdout);

        if(!sim_debug_enabled) {
            sim_debug_hovered_obj = NULL;
            sim_debug_right_btn_prev = false;
            sim_debug_hide_overlay();
        }
    }

    sim_debug_f2_prev = f2_down;
}

static void sim_debug_dump_obj(lv_obj_t *obj)
{
    char path[512];
    lv_area_t a;
    sim_debug_build_path(obj, path, sizeof(path));
    lv_obj_get_coords(obj, &a);
    printf("\n[SIM-UI-DEBUG] %s\n", path);
    printf("  ptr=%p parent=%p class=%p\n", (void *)obj, (void *)lv_obj_get_parent(obj), (void *)lv_obj_get_class(obj));
    int32_t rel_x = lv_obj_get_x(obj);
    int32_t rel_y = lv_obj_get_y(obj);
    printf("  x=%ld y=%ld w=%ld h=%ld\n", (long)rel_x, (long)rel_y,
           (long)(a.x2 - a.x1 + 1), (long)(a.y2 - a.y1 + 1));
    fflush(stdout);
}

static void sim_debug_update_overlay(lv_display_t *display)
{
    sim_debug_handle_toggle_key();
    sim_debug_ensure_overlay();

    if(!sim_debug_enabled) {
        sim_debug_hovered_obj = NULL;
        sim_debug_right_btn_prev = false;
        sim_debug_hide_overlay();
        return;
    }

    int mx = 0, my = 0;
    uint32_t buttons = SDL_GetMouseState(&mx, &my);
    bool right_down = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

    lv_obj_t *hit = sim_debug_find_hovered_obj(display, mx, my);
    if(hit != sim_debug_hovered_obj) {
        sim_debug_hovered_obj = hit;
    }

    if(sim_debug_hovered_obj) {
        char path[512];
        char info[640];
        lv_area_t a;
        sim_debug_build_path(sim_debug_hovered_obj, path, sizeof(path));
        lv_obj_get_coords(sim_debug_hovered_obj, &a);
        int32_t rel_x = lv_obj_get_x(sim_debug_hovered_obj);
        int32_t rel_y = lv_obj_get_y(sim_debug_hovered_obj);
        snprintf(info, sizeof(info), "%s\nx=%ld y=%ld w=%ld h=%ld",
                 path,
                 (long)rel_x, (long)rel_y,
                 (long)(a.x2 - a.x1 + 1),
                 (long)(a.y2 - a.y1 + 1));
        lv_label_set_text(sim_debug_hover_label, info);
        lv_obj_update_layout(sim_debug_hover_label);

        lv_coord_t tip_w = lv_obj_get_width(sim_debug_hover_label);
        lv_coord_t tip_h = lv_obj_get_height(sim_debug_hover_label);
        lv_coord_t tip_x = (lv_coord_t)(mx + 12);
        lv_coord_t tip_y = (lv_coord_t)(my + 12);
        if(tip_x + tip_w > LCD_H_RES) tip_x = LCD_H_RES - tip_w - 4;
        if(tip_y + tip_h > LCD_V_RES) tip_y = LCD_V_RES - tip_h - 4;
        if(tip_x < 0) tip_x = 0;
        if(tip_y < 0) tip_y = 0;
        lv_obj_set_pos(sim_debug_hover_label, tip_x, tip_y);
        lv_obj_clear_flag(sim_debug_hover_label, LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_pos(sim_debug_hover_box, a.x1, a.y1);
        lv_obj_set_size(sim_debug_hover_box, a.x2 - a.x1 + 1, a.y2 - a.y1 + 1);
        lv_obj_clear_flag(sim_debug_hover_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(sim_debug_hover_box);
        lv_obj_move_foreground(sim_debug_hover_label);
    } else {
        sim_debug_hide_overlay();
    }

    if(right_down && !sim_debug_right_btn_prev && sim_debug_hovered_obj != NULL) {
        sim_debug_dump_obj(sim_debug_hovered_obj);
    }
    sim_debug_right_btn_prev = right_down;
}

/* ═══════════════════════════════════════════════
 * Main entry point
 * ═══════════════════════════════════════════════ */
int main(int argc, char *argv[]) {

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   FilMachine Simulator                   ║\n");
    printf("║   Resolution: %dx%d                    ║\n", LCD_H_RES, LCD_V_RES);
    printf("║ Mouse = Touch | F2 = UI debug | Right click ║\n");
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
       LVGL 9.5 renamed gesture_limit → gesture_min_distance */
    mouse->gesture_min_distance = 30;   /* was 50 — pixels of movement to trigger GESTURE */
    mouse->gesture_min_velocity = 2;    /* was 3  — min per-frame velocity to keep accumulating */

    /* Create FreeRTOS queues (stub implementation) */
    sys.sysActionQ = xQueueCreate(16, sizeof(uint16_t));
    sys.motorActionQ = xQueueCreate(16, sizeof(uint16_t));

    /* Create shared keyboard */
    create_keyboard();

    /* Pre-load settings so splash knows if random mode is enabled */
    readSettingsOnly(FILENAME_SAVE);

    /* Show splash screen — play button calls readConfigFile → menu → refreshSettingsUI */
    lv_obj_t * splash = splash_screen_create();
    lv_scr_load(splash);

    /* WS server starts after splash Play — see splash_play_event_cb() */

    printf("[SIM] GUI initialized — entering main loop\n");
    printf("[SIM] Press F2 to enable/disable UI debug overlay\n[SIM] When enabled: hover highlights widgets, right click dumps details\n");

    /* Main loop */
    uint32_t last_tick = get_time_ms();

    while (1) {
        uint32_t now = get_time_ms();
        uint32_t elapsed = now - last_tick;
        last_tick = now;

        lv_tick_inc(elapsed);
        lv_timer_handler();
        sim_debug_update_overlay(display);

        /* Process any pending system actions (save, export, import, …) */
        sim_drain_sysActionQ();

        usleep(5000); /* ~5ms */
    }

    return 0;
}
