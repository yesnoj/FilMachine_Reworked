/**
 * test_runner.h — FilMachine Automated Test Framework
 *
 * Lightweight test framework that simulates touch input on the LVGL simulator.
 * No external dependencies (no Unity, no Ruby) — just compile and run.
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include "FilMachine.h"

/* ═══════════════════════════════════════════════
 * Test Framework Macros
 * ═══════════════════════════════════════════════ */

/** Global test counters (defined in test_runner.c) */
extern int g_tests_passed;
extern int g_tests_failed;
extern int g_tests_total;
extern const char *g_current_test;

/** Log file handle — writes to test_results.txt (defined in test_helpers.c) */
extern FILE *g_log_file;

/** Dual-output printf: writes to both stdout and the log file */
static inline void test_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static inline void test_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);

    if (g_log_file) {
        va_start(ap, fmt);
        vfprintf(g_log_file, fmt, ap);
        va_end(ap);
        fflush(g_log_file);
    }
}

/** Begin a named test */
#define TEST_BEGIN(name) do { \
    g_current_test = (name); \
    g_tests_total++; \
    test_printf("  [RUN ] %s\n", g_current_test); \
} while(0)

/** End the current test (called after all asserts passed) */
#define TEST_END() do { \
    g_tests_passed++; \
    test_printf("  [PASS] %s\n", g_current_test); \
} while(0)

/** Assert a condition — logs FAIL and returns from test function on failure */
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        g_tests_failed++; \
        test_printf("  [FAIL] %s — %s\n", g_current_test, (msg)); \
        test_printf("         at %s:%d\n", __FILE__, __LINE__); \
        return; \
    } \
} while(0)

/** Assert two strings are equal */
#define TEST_ASSERT_STR_EQ(a, b, msg) do { \
    if ((a) == NULL || (b) == NULL || strcmp((a), (b)) != 0) { \
        g_tests_failed++; \
        test_printf("  [FAIL] %s — %s\n", g_current_test, (msg)); \
        test_printf("         expected: \"%s\"\n", (b) ? (b) : "(null)"); \
        test_printf("         got:      \"%s\"\n", (a) ? (a) : "(null)"); \
        test_printf("         at %s:%d\n", __FILE__, __LINE__); \
        return; \
    } \
} while(0)

/** Assert pointer is not NULL */
#define TEST_ASSERT_NOT_NULL(ptr, msg) do { \
    if ((ptr) == NULL) { \
        g_tests_failed++; \
        test_printf("  [FAIL] %s — %s (got NULL)\n", g_current_test, (msg)); \
        test_printf("         at %s:%d\n", __FILE__, __LINE__); \
        return; \
    } \
} while(0)

/** Assert pointer is NULL */
#define TEST_ASSERT_NULL(ptr, msg) do { \
    if ((ptr) != NULL) { \
        g_tests_failed++; \
        test_printf("  [FAIL] %s — %s (expected NULL)\n", g_current_test, (msg)); \
        test_printf("         at %s:%d\n", __FILE__, __LINE__); \
        return; \
    } \
} while(0)

/** Assert two integers are equal */
#define TEST_ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        g_tests_failed++; \
        test_printf("  [FAIL] %s — %s\n", g_current_test, (msg)); \
        test_printf("         expected: %d, got: %d\n", (int)(b), (int)(a)); \
        test_printf("         at %s:%d\n", __FILE__, __LINE__); \
        return; \
    } \
} while(0)

/** Print a test suite header */
#define TEST_SUITE(name) do { \
    test_printf("\n══════════════════════════════════════\n"); \
    test_printf("  Suite: %s\n", (name)); \
    test_printf("══════════════════════════════════════\n"); \
} while(0)


/* ═══════════════════════════════════════════════
 * UI Coordinate Constants (480x320 display)
 * ═══════════════════════════════════════════════ */

/* Home page — startButton is 60x60 at BOTTOM_RIGHT(-10,-7) → center (440,283) */
#define UI_BTN_START_X          440
#define UI_BTN_START_Y          283

/* Menu tabs (left sidebar, each ~130px wide, ~97px tall) */
#define UI_TAB_PROCESSES_X      70
#define UI_TAB_PROCESSES_Y      55
#define UI_TAB_SETTINGS_X       70
#define UI_TAB_SETTINGS_Y       158
#define UI_TAB_TOOLS_X          70
#define UI_TAB_TOOLS_Y          263

/* Process list area (right panel) */
#define UI_PROC_LIST_X          290
#define UI_PROC_LIST_BASE_Y     38
#define UI_PROC_ELEMENT_H       70
#define UI_PROC_Y(i)            (UI_PROC_LIST_BASE_Y + 35 + (i) * UI_PROC_ELEMENT_H)

/* New Process button (top right of process list) */
#define UI_BTN_NEW_PROC_X       437
#define UI_BTN_NEW_PROC_Y       20

/* Filter button */
#define UI_BTN_FILTER_X         395
#define UI_BTN_FILTER_Y         20

/* Process detail page */
#define UI_BTN_CLOSE_X          460
#define UI_BTN_CLOSE_Y          20
#define UI_BTN_NEW_STEP_X       212
#define UI_BTN_NEW_STEP_Y       55
#define UI_BTN_SAVE_PROC_X      150
#define UI_BTN_SAVE_PROC_Y      295
#define UI_BTN_RUN_PROC_X       370
#define UI_BTN_RUN_PROC_Y       295

/* Step elements inside process detail */
#define UI_STEP_X               255
#define UI_STEP_ELEMENT_H       70
#define UI_STEP_Y(i)            (-13 + 35 + (i) * UI_STEP_ELEMENT_H)

/* Step detail popup (centered 350x300) */
#define UI_STEP_DETAIL_SAVE_X   175
#define UI_STEP_DETAIL_SAVE_Y   280
#define UI_STEP_DETAIL_CANCEL_X 305
#define UI_STEP_DETAIL_CANCEL_Y 280

/* Message popup buttons (centered popup) */
#define UI_POPUP_BTN1_X         190
#define UI_POPUP_BTN1_Y         200
#define UI_POPUP_BTN2_X         290
#define UI_POPUP_BTN2_Y         200
#define UI_POPUP_CLOSE_X        350
#define UI_POPUP_CLOSE_Y        105

/* Swipe parameters */
#define UI_SWIPE_DISTANCE       80


/* ═══════════════════════════════════════════════
 * Test Helper Functions (test_helpers.c)
 * ═══════════════════════════════════════════════ */

/** Initialize the test input device. Call once after lv_init() + display creation. */
void test_indev_init(void);

/** Process LVGL timers for the given number of milliseconds (no input injection) */
void test_pump(uint32_t ms);

/** Simulate a single click at (x, y) */
void test_click(int32_t x, int32_t y);

/** Simulate a click at the center of an LVGL object (auto-computes coordinates) */
void test_click_obj(lv_obj_t *obj);

/** Simulate a long press at (x, y) for duration_ms */
void test_long_press(int32_t x, int32_t y, uint32_t duration_ms);

/** Simulate a swipe from (x1,y1) to (x2,y2) over the given number of intermediate steps */
void test_swipe(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int steps);

/** Print final test summary. Returns 0 if all passed, 1 if any failed. */
int test_summary(void);


/* ═══════════════════════════════════════════════
 * Test Suite Entry Points
 * ═══════════════════════════════════════════════ */

/** test_navigation.c — basic nav & home page */
void test_suite_navigation(void);

/** test_processes.c — process list UI verification */
void test_suite_processes(void);

/** test_process_crud.c — full process CRUD (create/modify/delete/duplicate) */
void test_suite_process_crud(void);

/** test_steps.c — step list UI verification */
void test_suite_steps(void);

/** test_step_crud.c — full step CRUD (create/edit/delete/multiple/time) */
void test_suite_step_crud(void);

/** test_execution.c — process execution via checkup */
void test_suite_execution(void);

/** test_persistence.c — config file write/read round-trip */
void test_suite_persistence(void);

/** test_settings.c — settings UI + functional tests */
void test_suite_settings(void);

/** test_filter.c — process filter popup and filtering */
void test_suite_filter(void);

/** test_tools.c — tools page UI, popups, statistics */
void test_suite_tools(void);

/** test_edge_cases.c — utilities, deep copy, data integrity */
void test_suite_edge_cases(void);

/** test_utilities.c — pure utility functions, mappings, conversions */
void test_suite_utilities(void);

/** test_destroy_and_lifecycle.c — destroy functions, lifecycle, clone-in-place */
void test_suite_destroy_and_lifecycle(void);


/* ═══════════════════════════════════════════════
 * External globals (defined in main/src)
 * ═══════════════════════════════════════════════ */
extern struct gui_components gui;
extern struct sys_components sys;
extern bool   stopMotorManTask;
extern uint8_t initErrors;

#endif /* TEST_RUNNER_H */
