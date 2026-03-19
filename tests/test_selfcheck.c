/**
 * test_selfcheck.c — Self-check Popup Tests
 */

#include "test_runner.h"
#include "lvgl.h"

static void go_to_tools(void)
{
    test_click_obj(gui.page.menu.toolsTab);
    test_pump(300);
}

static void open_selfcheck(void)
{
    go_to_tools();
    test_click_obj(gui.page.tools.toolsSelfcheckButton);
    test_pump(500);
}

static void close_selfcheck(void)
{
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    if (sc->closeButton != NULL)
        test_click_obj(sc->closeButton);
    test_pump(300);
}


/* Test 1: Popup creates with all UI elements */
static void test_selfcheck_create(void)
{
    TEST_BEGIN("Self-check — popup creates with all UI elements");

    open_selfcheck();
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;

    TEST_ASSERT_NOT_NULL(sc->selfcheckPopupParent, "popup parent");
    TEST_ASSERT(!lv_obj_has_flag(sc->selfcheckPopupParent, LV_OBJ_FLAG_HIDDEN), "popup visible");
    TEST_ASSERT_NOT_NULL(sc->leftPanel, "left panel");
    TEST_ASSERT_NOT_NULL(sc->rightPanel, "right panel");
    TEST_ASSERT_NOT_NULL(sc->stopButton, "stop button");
    TEST_ASSERT_NOT_NULL(sc->startButton, "start button");
    TEST_ASSERT_NOT_NULL(sc->advanceButton, "advance button");
    TEST_ASSERT_NOT_NULL(sc->closeButton, "close button");
    TEST_ASSERT_NOT_NULL(sc->progressBar, "progress bar");

    for (int i = 0; i < 7; i++) {
        TEST_ASSERT_NOT_NULL(sc->phaseIcon[i], "phase icon");
        TEST_ASSERT_NOT_NULL(sc->phaseNameLabel[i], "phase name");
    }

    test_printf("         [INFO] All self-check UI elements verified\n");
    close_selfcheck();
    TEST_END();
}


/* Test 2: Initial state */
static void test_selfcheck_initial_state(void)
{
    TEST_BEGIN("Self-check — initial state is phase 0");

    open_selfcheck();
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;

    TEST_ASSERT_EQ(sc->currentPhase, 0, "phase 0");
    TEST_ASSERT(!sc->isRunning, "not running");
    TEST_ASSERT(lv_obj_has_flag(sc->progressBar, LV_OBJ_FLAG_HIDDEN), "progress hidden");

    const char *title = lv_label_get_text(sc->phaseTitle);
    TEST_ASSERT_STR_EQ(title, "Temp. sensors", "title");

    test_printf("         [INFO] Initial state correct\n");
    close_selfcheck();
    TEST_END();
}


/* Test 3: Start shows running */
static void test_selfcheck_start_phase(void)
{
    TEST_BEGIN("Self-check — start phase shows running state");

    open_selfcheck();
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;

    test_click_obj(sc->startButton);
    test_pump(500);

    TEST_ASSERT(sc->isRunning, "running after Start");
    TEST_ASSERT(!lv_obj_has_flag(sc->progressBar, LV_OBJ_FLAG_HIDDEN), "progress visible");

    test_printf("         [INFO] Phase running OK\n");

    test_click_obj(sc->stopButton);
    test_pump(300);
    close_selfcheck();
    TEST_END();
}


/* Test 4: Next disabled while running */
static void test_selfcheck_next_disabled_while_running(void)
{
    TEST_BEGIN("Self-check — Next disabled while running");

    open_selfcheck();
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;

    test_click_obj(sc->startButton);
    test_pump(500);

    TEST_ASSERT(lv_obj_has_state(sc->advanceButton, LV_STATE_DISABLED),
                "advance disabled while running");
    test_printf("         [INFO] Next correctly disabled\n");

    test_click_obj(sc->stopButton);
    test_pump(300);
    close_selfcheck();
    TEST_END();
}


/* Test 5: Stop saves stopped state */
static void test_selfcheck_stop_state(void)
{
    TEST_BEGIN("Self-check — stop saves grey state");

    open_selfcheck();
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;

    test_click_obj(sc->startButton);
    test_pump(500);
    TEST_ASSERT(sc->isRunning, "should be running");

    test_click_obj(sc->stopButton);
    test_pump(300);
    TEST_ASSERT(!sc->isRunning, "should stop");

    const char *status = lv_label_get_text(sc->phaseStatus);
    TEST_ASSERT_STR_EQ(status, "Stopped", "status shows Stopped");

    /* Start button should say Re-run */
    const char *startLabel = lv_label_get_text(sc->startButtonLabel);
    TEST_ASSERT_STR_EQ(startLabel, "Re-run", "button says Re-run");

    test_printf("         [INFO] Stopped state saved correctly\n");
    close_selfcheck();
    TEST_END();
}


/* Test 6: Close resets */
static void test_selfcheck_close_resets(void)
{
    TEST_BEGIN("Self-check — close resets all state");

    open_selfcheck();
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;

    /* Start and stop to get a non-pristine state */
    test_click_obj(sc->startButton);
    test_pump(500);
    test_click_obj(sc->stopButton);
    test_pump(300);

    close_selfcheck();

    /* Reopen */
    test_click_obj(gui.page.tools.toolsSelfcheckButton);
    test_pump(500);

    TEST_ASSERT_EQ(sc->currentPhase, 0, "reset to phase 0");
    TEST_ASSERT(!sc->isRunning, "not running");

    const char *startLabel = lv_label_get_text(sc->startButtonLabel);
    TEST_ASSERT_STR_EQ(startLabel, buttonStart_text, "button says Start");

    test_printf("         [INFO] Reset correct\n");
    close_selfcheck();
    TEST_END();
}


/* Test 7: Button labels */
static void test_selfcheck_button_labels(void)
{
    TEST_BEGIN("Self-check — button labels correct");

    open_selfcheck();
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;

    TEST_ASSERT_STR_EQ(lv_label_get_text(sc->stopButtonLabel), buttonStop_text, "Stop");
    TEST_ASSERT_STR_EQ(lv_label_get_text(sc->startButtonLabel), buttonStart_text, "Start");
    TEST_ASSERT_STR_EQ(lv_label_get_text(sc->advanceButtonLabel), selfCheckNext_text, "Next");

    test_printf("         [INFO] Labels: Stop, Start, Next\n");
    close_selfcheck();
    TEST_END();
}


/* Test 8: 7 phase names are correct */
static void test_selfcheck_phase_names(void)
{
    TEST_BEGIN("Self-check — 7 phase names displayed");

    open_selfcheck();
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;

    const char *expected[] = {
        "Temp. sensors", "Water pump", "Heater", "Valves",
        "Container C1", "Container C2", "Container C3"
    };

    for (int i = 0; i < 7; i++) {
        const char *name = lv_label_get_text(sc->phaseNameLabel[i]);
        TEST_ASSERT_STR_EQ(name, expected[i], "phase name");
    }

    test_printf("         [INFO] All 7 phase names correct\n");
    close_selfcheck();
    TEST_END();
}


void test_suite_selfcheck(void)
{
    TEST_SUITE("Self-check");

    test_selfcheck_create();
    test_selfcheck_initial_state();
    test_selfcheck_start_phase();
    test_selfcheck_next_disabled_while_running();
    test_selfcheck_stop_state();
    test_selfcheck_close_resets();
    test_selfcheck_button_labels();
    test_selfcheck_phase_names();
}
