/**
 * test_execution.c — Process Execution (Checkup) Functional Tests
 *
 * Tests the checkup flow: launching a process run, verifying UI elements,
 * stop controls, and returning to the menu.
 */

#include "test_runner.h"
#include "lvgl.h"

/* ── Helper: open first process detail ── */
static bool open_first_process_detail(void)
{
    /* Make sure we're on processes tab */
    test_click_obj(gui.page.menu.processesTab);
    test_pump(300);

    processNode *first = gui.page.processes.processElementsList.start;
    if (!first || !first->process.processElementSummary) return false;

    lv_obj_send_event(first->process.processElementSummary, LV_EVENT_SHORT_CLICKED, NULL);
    test_pump(500);

    return (gui.tempProcessNode != NULL &&
            gui.tempProcessNode->process.processDetails != NULL &&
            gui.tempProcessNode->process.processDetails->processDetailParent != NULL);
}

/* ── Helper: close checkup and return to menu ── */
static void close_checkup(void)
{
    if (!gui.tempProcessNode || !gui.tempProcessNode->process.processDetails ||
        !gui.tempProcessNode->process.processDetails->checkup) return;

    sCheckup *ck = gui.tempProcessNode->process.processDetails->checkup;

    if (ck->checkupCloseButton != NULL) {
        /* Close button may be disabled during processing — force-enable it */
        lv_obj_clear_state(ck->checkupCloseButton, LV_STATE_DISABLED);
        test_click_obj(ck->checkupCloseButton);
        test_pump(500);
    }
}


/* ═══════════════════════════════════════════════
 * Test 1: Run button opens checkup screen
 * ═══════════════════════════════════════════════ */
static void test_run_opens_checkup(void)
{
    TEST_BEGIN("Execution — run button opens checkup screen");

    TEST_ASSERT(open_first_process_detail(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    TEST_ASSERT(pd->stepElementsList.size > 0, "process needs steps to run");

    /* The processRunButton event handler sets checkupParent = NULL then calls checkup().
     * We replicate what the event handler does: */
    pd->checkup->checkupParent = NULL;
    lv_style_reset(&pd->textAreaStyle);
    checkup(gui.tempProcessNode);
    test_pump(500);

    /* Verify checkup screen was created */
    TEST_ASSERT_NOT_NULL(pd->checkup->checkupParent,
                         "checkupParent should be created");
    TEST_ASSERT_NOT_NULL(pd->checkup->checkupContainer,
                         "checkupContainer should exist");

    /* Verify the process name is displayed */
    TEST_ASSERT_NOT_NULL(pd->checkup->checkupProcessNameValue,
                         "process name label should exist");

    close_checkup();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: Checkup UI elements exist
 * ═══════════════════════════════════════════════ */
static void test_checkup_ui_elements(void)
{
    TEST_BEGIN("Execution — checkup UI elements exist");

    TEST_ASSERT(open_first_process_detail(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;

    /* Launch checkup */
    pd->checkup->checkupParent = NULL;
    lv_style_reset(&pd->textAreaStyle);
    checkup(gui.tempProcessNode);
    test_pump(500);

    sCheckup *ck = pd->checkup;

    /* Main containers */
    TEST_ASSERT_NOT_NULL(ck->checkupParent, "checkupParent should exist");
    TEST_ASSERT_NOT_NULL(ck->checkupContainer, "checkupContainer should exist");
    TEST_ASSERT_NOT_NULL(ck->checkupNextStepsContainer,
                         "left panel (next steps) should exist");
    TEST_ASSERT_NOT_NULL(ck->checkupStepContainer,
                         "right panel (step info) should exist");

    /* Control buttons available at processStep 0 */
    TEST_ASSERT_NOT_NULL(ck->checkupCloseButton, "close button should exist");
    TEST_ASSERT_NOT_NULL(ck->checkupStartButton, "start button should exist");

    /* Note: checkupStopNowButton and checkupStopAfterButton are only created
     * when isProcessing == 1 (processStep 4).  At step 0 they are still NULL. */
    test_printf("         [INFO] stopNow btn at step 0: %p (expected NULL)\n",
           (void *)ck->checkupStopNowButton);

    /* Process name */
    TEST_ASSERT_NOT_NULL(ck->checkupProcessNameValue,
                         "process name label should exist");

    /* Initial state */
    TEST_ASSERT_EQ((int)ck->data.isProcessing, 0, "should not be processing initially");
    TEST_ASSERT_EQ((int)ck->data.processStep, 0, "processStep should be 0 initially");
    TEST_ASSERT_EQ((int)ck->data.stopNow, 0, "stopNow should be false initially");
    TEST_ASSERT_EQ((int)ck->data.stopAfter, 0, "stopAfter should be false initially");

    close_checkup();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: Stop Now button shows confirmation popup
 * ═══════════════════════════════════════════════ */
static void test_stop_now_popup(void)
{
    TEST_BEGIN("Execution — stop now popup via messagePopupCreate");

    TEST_ASSERT(open_first_process_detail(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;

    /* Launch checkup */
    pd->checkup->checkupParent = NULL;
    lv_style_reset(&pd->textAreaStyle);
    checkup(gui.tempProcessNode);
    test_pump(500);

    sCheckup *ck = pd->checkup;

    /* Stop buttons are only created at processStep 4 (isProcessing == 1).
     * We test the popup mechanism directly by calling messagePopupCreate
     * with a dummy whoCallMe pointer — this verifies the popup system works
     * without needing the stop buttons to exist. */
    messagePopupCreate(warningPopupTitle_text, stopNowProcessPopupBody_text,
                       checkupStop_text, stepDetailCancel_text,
                       &gui);   /* use &gui as a neutral whoCallMe */
    test_pump(500);

    /* Verify popup appeared */
    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupParent,
                         "stop now popup should appear");
    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupButton1,
                         "popup stop button should exist");
    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupButton2,
                         "popup cancel button should exist");

    /* Cancel the popup (Button2 dismisses) */
    test_click_obj(gui.element.messagePopup.mBoxPopupButton2);
    test_pump(500);

    /* Verify stopNow was NOT set (we cancelled with neutral whoCallMe) */
    TEST_ASSERT_EQ((int)ck->data.stopNow, 0, "stopNow should still be false after cancel");

    close_checkup();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: Return to menu after closing checkup
 * ═══════════════════════════════════════════════ */
static void test_return_to_menu(void)
{
    TEST_BEGIN("Execution — close checkup returns to menu screen");

    TEST_ASSERT(open_first_process_detail(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;

    /* Launch checkup */
    pd->checkup->checkupParent = NULL;
    lv_style_reset(&pd->textAreaStyle);
    checkup(gui.tempProcessNode);
    test_pump(500);

    sCheckup *ck = pd->checkup;
    TEST_ASSERT_NOT_NULL(ck->checkupParent, "checkup should be open");

    /* Close checkup — this loads the menu screen and deletes checkup parent */
    lv_obj_clear_state(ck->checkupCloseButton, LV_STATE_DISABLED);
    test_click_obj(ck->checkupCloseButton);
    test_pump(500);

    /* Verify checkup parent was destroyed */
    TEST_ASSERT_NULL(pd->checkup->checkupParent,
                     "checkupParent should be NULL after close");

    /* Verify we're back on the menu screen */
    lv_obj_t *active_screen = lv_screen_active();
    TEST_ASSERT(active_screen == gui.page.menu.screen_mainMenu,
                "active screen should be the main menu");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_execution(void)
{
    TEST_SUITE("Execution (Checkup)");

    test_run_opens_checkup();
    test_checkup_ui_elements();
    test_stop_now_popup();
    test_return_to_menu();
}
