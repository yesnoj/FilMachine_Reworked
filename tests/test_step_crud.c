/**
 * test_step_crud.c — Step Create/Edit/Delete Functional Tests
 *
 * Tests the full lifecycle of step management within a process:
 * creation with full details, editing, deletion, multiple steps, validation.
 */

#include "test_runner.h"
#include "lvgl.h"

/* ── Helper: open first process detail ── */
static bool open_first_process_for_steps(void)
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

/* ── Helper: save and close current processDetail ── */
static void save_close(void)
{
    if (!gui.tempProcessNode || !gui.tempProcessNode->process.processDetails) return;
    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;

    if (pd->stepElementsList.size > 0 && pd->processSaveButton) {
        pd->somethingChanged = true;
        lv_obj_send_event(pd->processSaveButton, LV_EVENT_REFRESH, NULL);
        test_pump(100);
        test_click_obj(pd->processSaveButton);
        test_pump(500);
    }
    if (pd->processDetailCloseButton) {
        test_click_obj(pd->processDetailCloseButton);
        test_pump(500);
    }
}


/* ═══════════════════════════════════════════════
 * Test 1: Create step with full details
 * ═══════════════════════════════════════════════ */
static void test_create_step_full(void)
{
    TEST_BEGIN("Create step — full details (name, time, type, source)");

    TEST_ASSERT(open_first_process_for_steps(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    int32_t old_step_count = pd->stepElementsList.size;
    test_printf("         [INFO] Steps before: %d\n", (int)old_step_count);

    /* Click New Step */
    test_click_obj(pd->processNewStepButton);
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.tempStepNode, "tempStepNode should be set");
    TEST_ASSERT_NOT_NULL(gui.tempStepNode->step.stepDetails, "step details should exist");

    sStepDetail *sd = gui.tempStepNode->step.stepDetails;

    /* Set step fields */
    lv_textarea_set_text(sd->stepDetailNamelTextArea, "Test Bleach");
    gui.tempStepNode->step.stepDetails->timeMins = 8;
    gui.tempStepNode->step.stepDetails->timeSecs = 45;
    lv_textarea_set_text(sd->stepDetailMinTextArea, "8");
    lv_textarea_set_text(sd->stepDetailSecTextArea, "45");

    /* Set type to CHEMISTRY (0) */
    lv_dropdown_set_selected(sd->stepTypeDropDownList, 0);
    sd->type = CHEMISTRY;

    /* Set source */
    lv_dropdown_set_selected(sd->stepSourceDropDownList, 1);
    sd->source = 1;

    test_pump(100);

    /* Enable save via refresh */
    lv_obj_send_event(sd->stepSaveButton, LV_EVENT_REFRESH, NULL);
    test_pump(100);

    /* Save step */
    test_click_obj(sd->stepSaveButton);
    test_pump(500);

    /* Verify step count increased */
    int32_t new_step_count = pd->stepElementsList.size;
    test_printf("         [INFO] Steps after: %d\n", (int)new_step_count);
    TEST_ASSERT_EQ(new_step_count, old_step_count + 1,
                   "step count should increase by 1");

    /* Verify step data */
    stepNode *last_step = pd->stepElementsList.end;
    TEST_ASSERT_NOT_NULL(last_step, "last step should exist");
    TEST_ASSERT_STR_EQ(last_step->step.stepDetails->stepNameString,
                       "Test Bleach", "step name should match");

    save_close();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: Edit existing step
 * ═══════════════════════════════════════════════ */
static void test_edit_step(void)
{
    TEST_BEGIN("Edit step — modify name and time via stepDetail");

    TEST_ASSERT(open_first_process_for_steps(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    TEST_ASSERT(pd->stepElementsList.size > 0, "need at least one step");

    stepNode *first_step = pd->stepElementsList.start;
    TEST_ASSERT_NOT_NULL(first_step, "first step should exist");

    /* Open step detail directly (event_stepElement returns if indev==NULL,
     * so we call stepDetail() directly as a workaround) */
    stepDetail(gui.tempProcessNode, first_step);
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.tempStepNode, "tempStepNode should be set");
    sStepDetail *sd = gui.tempStepNode->step.stepDetails;
    TEST_ASSERT_NOT_NULL(sd, "step details should exist");

    /* Modify name and time */
    lv_textarea_set_text(sd->stepDetailNamelTextArea, "Edited Step");
    sd->timeMins = 10;
    sd->timeSecs = 0;
    lv_textarea_set_text(sd->stepDetailMinTextArea, "10");
    lv_textarea_set_text(sd->stepDetailSecTextArea, "0");
    test_pump(100);

    /* Save */
    lv_obj_send_event(sd->stepSaveButton, LV_EVENT_REFRESH, NULL);
    test_pump(100);
    test_click_obj(sd->stepSaveButton);
    test_pump(500);

    /* Verify name was updated */
    TEST_ASSERT_STR_EQ(first_step->step.stepDetails->stepNameString,
                       "Edited Step", "step name should be updated");

    save_close();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: Delete step (direct API workaround)
 * ═══════════════════════════════════════════════ */
static void test_delete_step(void)
{
    TEST_BEGIN("Delete step — remove from process list");

    TEST_ASSERT(open_first_process_for_steps(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    int32_t old_count = pd->stepElementsList.size;
    TEST_ASSERT(old_count > 1, "need at least 2 steps to delete one safely");
    test_printf("         [INFO] Steps before delete: %d\n", (int)old_count);

    /* Get the last step (least disruptive to delete) */
    stepNode *last_step = pd->stepElementsList.end;
    TEST_ASSERT_NOT_NULL(last_step, "last step should exist");

    /* Call deleteStepElement directly (event_stepElement won't work without indev) */
    bool result = deleteStepElement(last_step, gui.tempProcessNode, false);
    test_pump(500);

    int32_t new_count = pd->stepElementsList.size;
    test_printf("         [INFO] Steps after delete: %d\n", (int)new_count);
    TEST_ASSERT_EQ(new_count, old_count - 1, "step count should decrease by 1");

    save_close();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: Create multiple steps in sequence
 * ═══════════════════════════════════════════════ */
static void test_create_multiple_steps(void)
{
    TEST_BEGIN("Create steps — add 3 steps in sequence");

    TEST_ASSERT(open_first_process_for_steps(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    int32_t base_count = pd->stepElementsList.size;

    const char *names[] = {"Step A", "Step B", "Step C"};
    uint8_t mins[] = {2, 3, 1};

    for (int i = 0; i < 3; i++) {
        test_click_obj(pd->processNewStepButton);
        test_pump(500);

        TEST_ASSERT_NOT_NULL(gui.tempStepNode, "tempStepNode should be set");
        sStepDetail *sd = gui.tempStepNode->step.stepDetails;

        lv_textarea_set_text(sd->stepDetailNamelTextArea, names[i]);
        sd->timeMins = mins[i];
        sd->timeSecs = 0;
        lv_textarea_set_text(sd->stepDetailMinTextArea, mins[i] == 2 ? "2" : mins[i] == 3 ? "3" : "1");
        lv_textarea_set_text(sd->stepDetailSecTextArea, "0");
        test_pump(100);

        lv_obj_send_event(sd->stepSaveButton, LV_EVENT_REFRESH, NULL);
        test_pump(100);
        test_click_obj(sd->stepSaveButton);
        test_pump(500);
    }

    int32_t final_count = pd->stepElementsList.size;
    test_printf("         [INFO] Steps: base=%d, final=%d\n", (int)base_count, (int)final_count);
    TEST_ASSERT_EQ(final_count, base_count + 3, "should have added 3 steps");

    /* Verify each new step has a UI element */
    stepNode *s = pd->stepElementsList.start;
    int with_ui = 0;
    while (s != NULL) {
        if (s->step.stepElement != NULL) with_ui++;
        s = s->next;
    }
    TEST_ASSERT_EQ(with_ui, (int)final_count, "all steps should have UI elements");

    save_close();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: Step without name/time doesn't save (cancel instead)
 * ═══════════════════════════════════════════════ */
static void test_step_cancel(void)
{
    TEST_BEGIN("Step cancel — discard unsaved step");

    TEST_ASSERT(open_first_process_for_steps(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    int32_t old_count = pd->stepElementsList.size;

    /* Click New Step */
    test_click_obj(pd->processNewStepButton);
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.tempStepNode, "tempStepNode should be set");
    sStepDetail *sd = gui.tempStepNode->step.stepDetails;

    /* Don't set any fields — click cancel */
    TEST_ASSERT_NOT_NULL(sd->stepCancelButton, "cancel button should exist");
    test_click_obj(sd->stepCancelButton);
    test_pump(500);

    /* Step count should be unchanged */
    int32_t new_count = pd->stepElementsList.size;
    TEST_ASSERT_EQ(new_count, old_count, "canceling should not add a step");

    save_close();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 6: Verify total time calculation
 * ═══════════════════════════════════════════════ */
static void test_total_time_calculation(void)
{
    TEST_BEGIN("Total time — calculated from all steps");

    TEST_ASSERT(open_first_process_for_steps(), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;

    /* Sum up all step times manually */
    uint32_t total_mins = 0;
    uint32_t total_secs = 0;
    stepNode *s = pd->stepElementsList.start;
    while (s != NULL) {
        total_mins += s->step.stepDetails->timeMins;
        total_secs += s->step.stepDetails->timeSecs;
        s = s->next;
    }
    /* Normalize seconds overflow */
    total_mins += total_secs / 60;
    total_secs = total_secs % 60;

    test_printf("         [INFO] Calculated total: %dmin %dsec\n",
           (int)total_mins, (int)total_secs);
    test_printf("         [INFO] Process reports: %dmin %dsec\n",
           (int)pd->timeMins, (int)pd->timeSecs);

    TEST_ASSERT_EQ((int)pd->timeMins, (int)total_mins,
                   "process timeMins should match sum of step minutes");
    TEST_ASSERT_EQ((int)pd->timeSecs, (int)total_secs,
                   "process timeSecs should match sum of step seconds");

    /* Close without saving */
    pd->somethingChanged = false;
    lv_obj_clear_state(pd->processDetailCloseButton, LV_STATE_DISABLED);
    test_click_obj(pd->processDetailCloseButton);
    test_pump(500);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_step_crud(void)
{
    TEST_SUITE("Step CRUD");

    test_create_step_full();
    test_edit_step();
    test_delete_step();
    test_create_multiple_steps();
    test_step_cancel();
    test_total_time_calculation();
}
