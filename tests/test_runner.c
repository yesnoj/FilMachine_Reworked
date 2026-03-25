/**
 * test_runner.c — FilMachine Test Entry Point
 *
 * Replaces src/main.c for the test build. Initializes LVGL + SDL display,
 * registers a test input device, loads config, then runs all test suites.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#include "test_runner.h"
#include "page_splash.h"
#include "lvgl.h"

/* Need access to gesture fields on the SDL mouse indev */
#include "src/indev/lv_indev_private.h"

/* ═══════════════════════════════════════════════
 * Global Test Counters
 * ═══════════════════════════════════════════════ */
int         g_tests_passed  = 0;
int         g_tests_failed  = 0;
int         g_tests_total   = 0;
const char *g_current_test  = "";

/* ═══════════════════════════════════════════════
 * Global Variables (same as main.c — needed by FilMachine code)
 * ═══════════════════════════════════════════════ */
struct gui_components   gui;
struct sys_components   sys;
bool                    stopMotorManTask = false;
uint8_t                 initErrors = 0;

/* ═══════════════════════════════════════════════
 * Stubs (same as main.c)
 * ═══════════════════════════════════════════════ */
void stopMotorTask(void) {
    stopMotorManTask = true;
}

void runMotorTask(void) {
    stopMotorManTask = false;
}

__attribute__((weak)) void stopMotor(uint8_t pin1, uint8_t pin2) { }
__attribute__((weak)) void runMotorFW(uint8_t pin1, uint8_t pin2) { }
__attribute__((weak)) void runMotorRV(uint8_t pin1, uint8_t pin2) { }
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
__attribute__((weak)) void rebootBoard(void) { }
__attribute__((weak)) void buzzer_beep(void) { }
__attribute__((weak)) void alarm_start_persistent(void) { }
__attribute__((weak)) void alarm_stop(void) { }
__attribute__((weak)) bool alarm_is_active(void) { return false; }
__attribute__((weak)) void tools_pause_timer(void) { }
__attribute__((weak)) void tools_resume_timer(void) { }

/* Temperature control stubs for tests (no simulation needed) */
float sim_getTemperature(uint8_t sensorIndex) { return 20.0f; }
void  sim_setHeater(bool on) { }
void  sim_resetTemperatures(void) { }

/* ═══════════════════════════════════════════════
 * Test Data Generator — creates processes when no config file is present
 * ═══════════════════════════════════════════════ */
static void add_step_to_process(processNode *proc, const char *name,
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

static void add_process_to_list(processList *list, processNode *p)
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

void test_generate_data(void)
{
    processList *list = &gui.page.processes.processElementsList;

    /* ── Process 1: "Test C41" (Color) ── */
    processNode *p1 = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    snprintf(p1->process.processDetails->data.processNameString, sizeof(p1->process.processDetails->data.processNameString), "%s", "Test C41");
    p1->process.processDetails->data.temp            = 38;
    p1->process.processDetails->data.tempTolerance   = 0.3f;
    p1->process.processDetails->data.isTempControlled = 1;
    p1->process.processDetails->data.isPreferred     = 1;
    p1->process.processDetails->data.filmType        = COLOR_FILM;
    p1->process.processDetails->data.timeMins        = 20;
    p1->process.processDetails->data.timeSecs        = 0;
    add_step_to_process(p1, "Pre-wash",  1, 0,  1, 3, 1);
    add_step_to_process(p1, "Developer", 3, 15, 0, 0, 0);
    add_step_to_process(p1, "Bleach",    6, 30, 0, 1, 0);
    add_step_to_process(p1, "Wash",      3, 0,  2, 3, 1);
    add_step_to_process(p1, "Fix",       6, 30, 0, 2, 0);
    add_step_to_process(p1, "Final Wash",3, 0,  2, 3, 1);
    add_process_to_list(list, p1);

    /* ── Process 2: "Test B&W" (B&W) ── */
    processNode *p2 = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    snprintf(p2->process.processDetails->data.processNameString, sizeof(p2->process.processDetails->data.processNameString), "%s", "Test B&W");
    p2->process.processDetails->data.temp            = 20;
    p2->process.processDetails->data.tempTolerance   = 0.5f;
    p2->process.processDetails->data.isTempControlled = 1;
    p2->process.processDetails->data.isPreferred     = 1;
    p2->process.processDetails->data.filmType        = BLACK_AND_WHITE_FILM;
    p2->process.processDetails->data.timeMins        = 15;
    p2->process.processDetails->data.timeSecs        = 0;
    add_step_to_process(p2, "Developer", 8, 0,  0, 0, 1);
    add_step_to_process(p2, "Stop Bath", 1, 0,  0, 1, 0);
    add_step_to_process(p2, "Fixer",     5, 0,  0, 2, 0);
    add_step_to_process(p2, "Wash",      5, 0,  2, 3, 1);
    add_process_to_list(list, p2);

    /* ── Process 3: "Quick Test" (B&W, no temp control) ── */
    processNode *p3 = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    snprintf(p3->process.processDetails->data.processNameString, sizeof(p3->process.processDetails->data.processNameString), "%s", "Quick Test");
    p3->process.processDetails->data.temp            = 24;
    p3->process.processDetails->data.tempTolerance   = 1.0f;
    p3->process.processDetails->data.isTempControlled = 0;
    p3->process.processDetails->data.isPreferred     = 0;
    p3->process.processDetails->data.filmType        = BLACK_AND_WHITE_FILM;
    p3->process.processDetails->data.timeMins        = 10;
    p3->process.processDetails->data.timeSecs        = 0;
    add_step_to_process(p3, "Developer", 5, 0, 0, 0, 1);
    add_step_to_process(p3, "Fixer",     3, 0, 0, 1, 0);
    add_step_to_process(p3, "Rinse",     2, 0, 1, 3, 1);
    add_process_to_list(list, p3);

    test_printf("[TEST] Generated %d test processes\n", (int)list->size);
}

/* ═══════════════════════════════════════════════
 * Main — Test Entry Point
 * ═══════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    /* Build log filename with timestamp in dedicated test_results/ directory.
     * TEST_RESULTS_DIR is defined by CMake to point to <source>/test_results/
     * so results survive clean builds (they don't live in the build directory). */
    char log_filename[256];
    {
#ifdef TEST_RESULTS_DIR
        /* Create directory if it doesn't exist (no error if it already does) */
        mkdir(TEST_RESULTS_DIR, 0755);
#endif
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
#ifdef TEST_RESULTS_DIR
        snprintf(log_filename, sizeof(log_filename),
                 TEST_RESULTS_DIR "/test_results_%04d%02d%02d_%02d%02d%02d.txt",
                 tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                 tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
#else
        snprintf(log_filename, sizeof(log_filename),
                 "test_results_%04d%02d%02d_%02d%02d%02d.txt",
                 tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                 tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
#endif
    }

    /* Open log file — all test_printf() calls will mirror output here */
    g_log_file = fopen(log_filename, "w");
    if (!g_log_file) {
        fprintf(stderr, "WARNING: Could not open %s for writing\n", log_filename);
    }

    test_printf("╔══════════════════════════════════════════╗\n");
    test_printf("║   FilMachine Automated Test Runner       ║\n");
    test_printf("║   Resolution: %dx%d                    ║\n", LCD_H_RES, LCD_V_RES);
    test_printf("╚══════════════════════════════════════════╝\n\n");

    srand((unsigned int)time(NULL));

    /* Initialize globals */
    initGlobals();

    /* Initialize LVGL */
    lv_init();

    /* Create SDL display (needed for rendering even in test mode) */
    lv_display_t *display = lv_sdl_window_create(LCD_H_RES, LCD_V_RES);
    if (!display) {
        fprintf(stderr, "ERROR: Failed to create SDL display!\n");
        return 1;
    }

    /* We do NOT create an SDL mouse — we use our own test indev instead */
    test_indev_init();

    /* Create FreeRTOS queues (stub) */
    sys.sysActionQ   = xQueueCreate(16, sizeof(uint16_t));
    sys.motorActionQ = xQueueCreate(16, sizeof(uint16_t));

    /* Create shared keyboard */
    create_keyboard();

    /* Launch splash screen (play callback → menu) */
    lv_obj_t *splash = splash_screen_create();
    lv_scr_load(splash);

    /* Load saved configuration */
    readConfigFile(FILENAME_SAVE, false);

    /* If no processes loaded (missing sd/FilMachine.cfg), generate test data */
    if (gui.page.processes.processElementsList.size == 0) {
        test_printf("[TEST] No config file found — generating test data\n");
        test_generate_data();
    } else {
        test_printf("[TEST] Loaded %d processes from config\n",
                    (int)gui.page.processes.processElementsList.size);
    }

    test_printf("[TEST] Setup complete — running test suites\n\n");

    /* Pump LVGL to let the home page render fully */
    test_pump(500);

    /* ── Run all test suites ── */

    test_suite_navigation();
    test_suite_processes();
    test_suite_process_crud();
    test_suite_steps();
    test_suite_step_crud();
    test_suite_execution();
    test_suite_persistence();
    test_suite_settings();
    test_suite_filter();
    test_suite_tools();
    test_suite_new_settings();
    test_suite_selfcheck();
    test_suite_ota();
    test_suite_ui_profile();
    test_suite_edge_cases();
    test_suite_utilities();
    test_suite_destroy_and_lifecycle();

    /* ── Summary ── */
    int result = test_summary();

    test_printf("[TEST] Done — exiting with code %d\n", result);

    /* Close log file */
    if (g_log_file) {
        test_printf("[TEST] Results saved to: %s\n", log_filename);
        fclose(g_log_file);
        g_log_file = NULL;
    }

    return result;
}
