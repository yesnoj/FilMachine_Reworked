/**
 * test_processes.c — Process CRUD Test Suite
 *
 * Requires: navigation tests have run (we're on the Processes tab).
 *
 * Tests:
 * 1. Process list is populated from config
 * 2. Click on a process opens process detail
 * 3. gui.tempProcessNode points to the correct process
 * 4. Close button returns to process list
 * 5. Create new process
 */

#include "test_runner.h"
#include "lvgl.h"

/* ── Test 1: Process list is populated ── */
static void test_process_list_populated(void)
{
    TEST_BEGIN("Process list — populated from config/generated data");

    /* The processes list should exist */
    TEST_ASSERT_NOT_NULL(gui.page.processes.processesListContainer,
                         "processesListContainer should exist");

    /* Check that at least 1 process was loaded (from config or generated) */
    int32_t size = gui.page.processes.processElementsList.size;
    test_printf("         [INFO] Process list size: %d\n", (int)size);

    TEST_ASSERT(size > 0, "process list should have at least 1 process");

    /* The start node should be valid */
    TEST_ASSERT_NOT_NULL(gui.page.processes.processElementsList.start,
                         "process list start should not be NULL");

    /* Verify each process node has a UI element */
    processNode *p = gui.page.processes.processElementsList.start;
    int with_ui = 0;
    while (p != NULL) {
        if (p->process.processElement != NULL) with_ui++;
        p = p->next;
    }
    test_printf("         [INFO] Processes with UI elements: %d / %d\n", with_ui, (int)size);

    TEST_END();
}

/* ── Test 2: Create a new process ── */
static void test_create_new_process(void)
{
    TEST_BEGIN("New process — click new process button opens detail");

    /* Click the "new process" button using the actual LVGL object */
    TEST_ASSERT_NOT_NULL(gui.page.processes.newProcessButton,
                         "newProcessButton should exist");
    test_click_obj(gui.page.processes.newProcessButton);
    test_pump(500);

    /* Clicking "new process" opens processDetail for a new (empty) process.
     * The process is NOT added to the list until saved.
     * Verify that processDetail opened correctly. */
    TEST_ASSERT_NOT_NULL(gui.tempProcessNode,
                         "tempProcessNode should be set after new process click");
    TEST_ASSERT_NOT_NULL(gui.tempProcessNode->process.processDetails,
                         "processDetails should exist");
    TEST_ASSERT_NOT_NULL(gui.tempProcessNode->process.processDetails->processDetailParent,
                         "processDetail page should be created");

    test_printf("         [INFO] New process detail opened at %p\n",
           (void *)gui.tempProcessNode->process.processDetails->processDetailParent);

    /* Close the process detail to return to the menu
     * (new process was not saved — just verifying the UI opens) */
    lv_obj_t *closeBtn = gui.tempProcessNode->process.processDetails->processDetailCloseButton;
    TEST_ASSERT_NOT_NULL(closeBtn, "close button should exist");
    test_click_obj(closeBtn);
    test_pump(500);

    /* Verify we're back on the menu screen */
    TEST_ASSERT(lv_screen_active() == gui.page.menu.screen_mainMenu,
                "should be back on menu after closing new process detail");

    TEST_END();
}

/* ── Test 3: Click on a process opens detail ── */
static void test_click_process_opens_detail(void)
{
    TEST_BEGIN("Process click — opens process detail page");

    /* We should have at least one process at this point */
    TEST_ASSERT(gui.page.processes.processElementsList.size > 0,
                "need at least one process to test");

    /* Click on the first process's summary element.
     * Process elements use LV_EVENT_SHORT_CLICKED on processElementSummary
     * (which bubbles to processElement via LV_OBJ_FLAG_EVENT_BUBBLE).
     * The handler checks: obj == processElementSummary && swipedLeft == true */
    processNode *first = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(first, "first process node should exist");
    TEST_ASSERT_NOT_NULL(first->process.processElementSummary,
                         "first process should have a summary element");

    test_printf("         [DBG] Sending SHORT_CLICKED to processElementSummary %p\n",
           (void *)first->process.processElementSummary);
    lv_obj_send_event(first->process.processElementSummary, LV_EVENT_SHORT_CLICKED, NULL);
    test_pump(500);

    /* gui.tempProcessNode should now point to a valid process */
    TEST_ASSERT_NOT_NULL(gui.tempProcessNode,
                         "tempProcessNode should be set after click");

    /* Process detail should be created */
    TEST_ASSERT_NOT_NULL(gui.tempProcessNode->process.processDetails,
                         "processDetails should exist");
    TEST_ASSERT_NOT_NULL(gui.tempProcessNode->process.processDetails->processDetailParent,
                         "processDetailParent LVGL widget should exist");

    TEST_END();
}

/* ── Test 4: Process detail has expected UI elements ── */
static void test_process_detail_elements(void)
{
    TEST_BEGIN("Process detail — UI elements exist");

    /* We should be in process detail mode */
    TEST_ASSERT_NOT_NULL(gui.tempProcessNode, "tempProcessNode should be set");

    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    TEST_ASSERT_NOT_NULL(pd, "processDetails should exist");

    /* Check key UI elements exist */
    TEST_ASSERT_NOT_NULL(pd->processDetailCloseButton, "close button should exist");
    TEST_ASSERT_NOT_NULL(pd->processNewStepButton, "new step button should exist");
    TEST_ASSERT_NOT_NULL(pd->processSaveButton, "save button should exist");
    TEST_ASSERT_NOT_NULL(pd->processDetailNameTextArea, "name text area should exist");
    TEST_ASSERT_NOT_NULL(pd->processStepsContainer, "steps container should exist");

    TEST_END();
}

/* ── Test 5: Close button returns to process list ── */
static void test_close_process_detail(void)
{
    TEST_BEGIN("Process detail — close button returns to list");

    /* Get the close button from the process detail */
    TEST_ASSERT_NOT_NULL(gui.tempProcessNode, "tempProcessNode should be set");
    sProcessDetail *pd = gui.tempProcessNode->process.processDetails;
    TEST_ASSERT_NOT_NULL(pd, "processDetails should exist");
    TEST_ASSERT_NOT_NULL(pd->processDetailCloseButton, "close button should exist");

    /* Click the close button using the actual LVGL object */
    test_click_obj(pd->processDetailCloseButton);
    test_pump(500);

    /* We should still be on the menu screen */
    TEST_ASSERT(lv_screen_active() == gui.page.menu.screen_mainMenu,
                "should still be on menu screen after closing detail");

    TEST_END();
}

/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_processes(void)
{
    TEST_SUITE("Processes");

    test_process_list_populated();
    test_create_new_process();
    test_click_process_opens_detail();
    test_process_detail_elements();
    test_close_process_detail();
}
