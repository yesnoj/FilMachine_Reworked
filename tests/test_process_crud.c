/**
 * test_process_crud.c — Process Create/Modify/Delete Functional Tests
 *
 * Tests the full lifecycle of process management:
 * creation with name+steps, modification, deletion, duplication.
 */

#include "test_runner.h"
#include "lvgl.h"

/* ── Helper: open process detail for the N-th process (0-based) ── */
static processNode *get_process_at(int index)
{
    processNode *p = gui.page.processes.processElementsList.start;
    for (int i = 0; i < index && p != NULL; i++)
        p = p->next;
    return p;
}

/* ── Helper: open a specific process's detail page ── */
static bool open_process(processNode *proc)
{
    if (!proc || !proc->process.processElementSummary) return false;
    lv_obj_send_event(proc->process.processElementSummary, LV_EVENT_SHORT_CLICKED, NULL);
    test_pump(500);
    return (gui.tempProcessNode == proc &&
            gui.tempProcessNode->process.processDetails != NULL &&
            gui.tempProcessNode->process.processDetails->processDetailParent != NULL);
}

/* ── Helper: save and close current processDetail ── */
static void save_and_close_process(void)
{
    if (!gui.tempProcessNode || !gui.tempProcessNode->process.processDetails) return;
    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;

    /* Save if possible (needs steps > 0) */
    if (pd->stepElementsList.size > 0 && pd->processSaveButton != NULL) {
        /* Mark as changed so save button is enabled */
        pd->data.somethingChanged = true;
        lv_obj_send_event(pd->processSaveButton, LV_EVENT_REFRESH, NULL);
        test_pump(100);
        test_click_obj(pd->processSaveButton);
        test_pump(500);
    }

    /* Close */
    if (pd->processDetailCloseButton != NULL) {
        test_click_obj(pd->processDetailCloseButton);
        test_pump(500);
    }
}

/* ── Helper: close current processDetail WITHOUT saving ── */
static void close_process_nosave(void)
{
    if (!gui.tempProcessNode || !gui.tempProcessNode->process.processDetails) return;
    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;

    /* If somethingChanged, clear it so close button works */
    pd->data.somethingChanged = false;
    lv_obj_clear_state(pd->processDetailCloseButton, LV_STATE_DISABLED);

    if (pd->processDetailCloseButton != NULL) {
        test_click_obj(pd->processDetailCloseButton);
        test_pump(500);
    }
}


/* ═══════════════════════════════════════════════
 * Test 1: Full process creation
 * ═══════════════════════════════════════════════ */
static void test_create_process_full(void)
{
    TEST_BEGIN("Create process — full flow with name + step + save");

    int32_t old_size = gui.page.processes.processElementsList.size;

    /* 1. Click new process button */
    test_click_obj(gui.page.processes.newProcessButton);
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.tempProcessNode, "tempProcessNode should be set");
    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    TEST_ASSERT_NOT_NULL(pd, "processDetails should exist");

    /* 2. Set process name */
    lv_textarea_set_text(pd->processDetailNameTextArea, "AutoTest Process");
    test_pump(100);

    /* 3. Add a step via New Step button */
    test_click_obj(pd->processNewStepButton);
    test_pump(500);

    /* stepDetail should have opened — set step fields */
    TEST_ASSERT_NOT_NULL(gui.tempStepNode, "tempStepNode should be set");
    TEST_ASSERT_NOT_NULL(gui.tempStepNode->step.stepDetails, "step details should exist");

    /* Set step name and time directly on the node (roller popup is hard to automate) */
    gui.tempStepNode->step.stepDetails->data.timeMins = 5;
    gui.tempStepNode->step.stepDetails->data.timeSecs = 30;
    lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailNameTextArea, "Dev Step");
    /* Update the minutes/seconds textareas for the REFRESH handler */
    lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailMinTextArea, "5");
    lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailSecTextArea, "30");
    test_pump(100);

    /* Enable save by triggering refresh */
    lv_obj_send_event(gui.tempStepNode->step.stepDetails->stepSaveButton, LV_EVENT_REFRESH, NULL);
    test_pump(100);

    /* Save the step */
    test_click_obj(gui.tempStepNode->step.stepDetails->stepSaveButton);
    test_pump(500);

    /* Verify step was added */
    TEST_ASSERT(pd->stepElementsList.size > 0, "process should have at least 1 step");
    test_printf("         [INFO] Steps in new process: %d\n", (int)pd->stepElementsList.size);

    /* 4. Save the process */
    pd->data.somethingChanged = true;
    lv_obj_send_event(pd->processSaveButton, LV_EVENT_REFRESH, NULL);
    test_pump(100);
    test_click_obj(pd->processSaveButton);
    test_pump(500);

    /* 5. Close process detail */
    test_click_obj(pd->processDetailCloseButton);
    test_pump(500);

    /* 6. Verify process was added to list */
    int32_t new_size = gui.page.processes.processElementsList.size;
    test_printf("         [INFO] List size: before=%d, after=%d\n", (int)old_size, (int)new_size);
    TEST_ASSERT_EQ(new_size, old_size + 1, "process list should grow by 1");

    /* Verify the new process has the correct name */
    processNode *last = gui.page.processes.processElementsList.end;
    TEST_ASSERT_NOT_NULL(last, "last process should exist");
    TEST_ASSERT_STR_EQ(last->process.processDetails->data.processNameString,
                       "AutoTest Process", "new process should have correct name");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: Modify process name
 * ═══════════════════════════════════════════════ */
static void test_modify_process_name(void)
{
    TEST_BEGIN("Modify process — change name and save");

    processNode *first = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(first, "first process should exist");
    TEST_ASSERT(open_process(first), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;

    /* Change the name */
    lv_textarea_set_text(pd->processDetailNameTextArea, "Modified Name");
    pd->data.somethingChanged = true;
    lv_obj_send_event(pd->processSaveButton, LV_EVENT_REFRESH, NULL);
    test_pump(100);

    /* Save */
    test_click_obj(pd->processSaveButton);
    test_pump(500);

    /* Verify name was updated in the node */
    TEST_ASSERT_STR_EQ(pd->data.processNameString, "Modified Name",
                       "process name should be updated after save");

    /* Close */
    test_click_obj(pd->processDetailCloseButton);
    test_pump(500);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: Change film type
 * ═══════════════════════════════════════════════ */
static void test_change_film_type(void)
{
    TEST_BEGIN("Modify process — change film type");

    processNode *first = gui.page.processes.processElementsList.start;
    TEST_ASSERT(open_process(first), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    uint8_t old_type = pd->data.filmType;

    /* Click the opposite film type label */
    if (old_type == COLOR_FILM) {
        TEST_ASSERT_NOT_NULL(pd->processBnWLabel, "B&W label should exist");
        test_click_obj(pd->processBnWLabel);
    } else {
        TEST_ASSERT_NOT_NULL(pd->processColorLabel, "Color label should exist");
        test_click_obj(pd->processColorLabel);
    }
    test_pump(200);

    /* Verify film type changed */
    TEST_ASSERT(pd->data.filmType != old_type, "film type should have changed");
    test_printf("         [INFO] Film type: %d → %d\n", old_type, pd->data.filmType);

    save_and_close_process();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: Toggle preferred
 * ═══════════════════════════════════════════════ */
static void test_toggle_preferred(void)
{
    TEST_BEGIN("Modify process — toggle preferred");

    processNode *first = gui.page.processes.processElementsList.start;
    TEST_ASSERT(open_process(first), "should open process detail");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    bool old_pref = pd->data.isPreferred;

    TEST_ASSERT_NOT_NULL(pd->processPreferredLabel, "preferred label should exist");
    test_click_obj(pd->processPreferredLabel);
    test_pump(200);

    /* Verify preferred toggled */
    TEST_ASSERT(pd->data.isPreferred != old_pref, "preferred should have toggled");
    test_printf("         [INFO] Preferred: %d → %d\n", old_pref, pd->data.isPreferred);

    save_and_close_process();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: Delete process (workaround — no swipe)
 * ═══════════════════════════════════════════════ */
static void test_delete_process(void)
{
    TEST_BEGIN("Delete process — confirm deletion via popup");

    int32_t old_size = gui.page.processes.processElementsList.size;
    TEST_ASSERT(old_size > 0, "need at least one process");

    /* Get the last process (so we don't disturb other tests) */
    processNode *target = gui.page.processes.processElementsList.end;
    TEST_ASSERT_NOT_NULL(target, "target process should exist");
    TEST_ASSERT_NOT_NULL(target->process.deleteButton, "delete button should exist");

    /* Workaround: manually set state as if swiped right (since gesture sim doesn't work) */
    target->process.swipedRight = true;
    target->process.swipedLeft = false;
    gui.tempProcessNode = target;

    /* Show delete button and send SHORT_CLICKED to it */
    lv_obj_remove_flag(target->process.deleteButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_send_event(target->process.deleteButton, LV_EVENT_SHORT_CLICKED, NULL);
    test_pump(500);

    /* Verify popup appeared */
    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupParent,
                         "delete confirmation popup should appear");

    /* Click confirm (Button1 = delete) */
    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupButton1,
                         "popup confirm button should exist");
    test_click_obj(gui.element.messagePopup.mBoxPopupButton1);
    test_pump(500);

    /* Verify process was removed */
    int32_t new_size = gui.page.processes.processElementsList.size;
    test_printf("         [INFO] List size: before=%d, after=%d\n", (int)old_size, (int)new_size);
    TEST_ASSERT_EQ(new_size, old_size - 1, "process list should shrink by 1");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 6: Cancel process deletion
 * ═══════════════════════════════════════════════ */
static void test_cancel_delete_process(void)
{
    TEST_BEGIN("Delete process — cancel keeps process");

    int32_t old_size = gui.page.processes.processElementsList.size;
    TEST_ASSERT(old_size > 0, "need at least one process");

    processNode *target = gui.page.processes.processElementsList.end;
    TEST_ASSERT_NOT_NULL(target, "target process should exist");

    /* Set up as if swiped right */
    target->process.swipedRight = true;
    target->process.swipedLeft = false;
    gui.tempProcessNode = target;
    lv_obj_remove_flag(target->process.deleteButton, LV_OBJ_FLAG_HIDDEN);

    lv_obj_send_event(target->process.deleteButton, LV_EVENT_SHORT_CLICKED, NULL);
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupParent,
                         "popup should appear");

    /* Click cancel (Button2) */
    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupButton2,
                         "popup cancel button should exist");
    test_click_obj(gui.element.messagePopup.mBoxPopupButton2);
    test_pump(500);

    /* Verify process still exists */
    int32_t new_size = gui.page.processes.processElementsList.size;
    TEST_ASSERT_EQ(new_size, old_size, "process list should be unchanged");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 7: Duplicate process
 * ═══════════════════════════════════════════════ */
static void test_duplicate_process(void)
{
    TEST_BEGIN("Duplicate process — via popup confirmation");

    int32_t old_size = gui.page.processes.processElementsList.size;
    TEST_ASSERT(old_size > 0, "need at least one process");

    processNode *target = gui.page.processes.processElementsList.start;
    gui.tempProcessNode = target;

    /* Simulate long press popup for duplication.
     * Long press handler sets longPressHandled=true and calls messagePopupCreate.
     * We replicate that sequence directly. */
    target->process.longPressHandled = true;
    target->process.swipedLeft = true;
    messagePopupCreate(duplicatePopupTitle_text, duplicateProcessPopupBody_text,
                       checkupNo_text, checkupYes_text, target);
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupParent,
                         "duplicate popup should appear");

    /* Button2 = "Yes" = confirm duplicate (in the popup handler) */
    test_click_obj(gui.element.messagePopup.mBoxPopupButton2);
    test_pump(500);

    int32_t new_size = gui.page.processes.processElementsList.size;
    test_printf("         [INFO] List size: before=%d, after=%d\n", (int)old_size, (int)new_size);
    TEST_ASSERT_EQ(new_size, old_size + 1, "process list should grow by 1 after duplicate");

    /* Reset long press state */
    target->process.longPressHandled = false;

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 8: Process without steps cannot be saved to list
 * ═══════════════════════════════════════════════ */
static void test_process_no_steps_no_save(void)
{
    TEST_BEGIN("Create process — no steps prevents save to list");

    int32_t old_size = gui.page.processes.processElementsList.size;

    /* Open new process */
    test_click_obj(gui.page.processes.newProcessButton);
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.tempProcessNode, "tempProcessNode should be set");
    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;

    /* Set name but don't add any steps */
    lv_textarea_set_text(pd->processDetailNameTextArea, "Empty Process");
    test_pump(100);

    /* Try to save — the handler checks stepElementsList.size > 0 */
    test_click_obj(pd->processSaveButton);
    test_pump(500);

    /* Process should NOT be added to list */
    int32_t new_size = gui.page.processes.processElementsList.size;
    TEST_ASSERT_EQ(new_size, old_size,
                   "process without steps should not be added to list");

    /* Close without saving */
    close_process_nosave();

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_process_crud(void)
{
    TEST_SUITE("Process CRUD");

    test_create_process_full();
    test_modify_process_name();
    test_change_film_type();
    test_toggle_preferred();
    test_delete_process();
    test_cancel_delete_process();
    test_duplicate_process();
    test_process_no_steps_no_save();
}
