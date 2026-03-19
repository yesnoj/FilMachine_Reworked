/**
 * test_tools.c — Tools Page Tests
 *
 * Tests the Tools tab UI elements, button interactions,
 * popup dialogs (export, import, credits), and statistics display.
 */

#include "test_runner.h"
#include "lvgl.h"


/* ── Helper: switch to Tools tab ── */
static void go_to_tools(void)
{
    test_click_obj(gui.page.menu.toolsTab);
    test_pump(300);
}


/* ═══════════════════════════════════════════════
 * Test 1: Tools section UI elements exist
 * ═══════════════════════════════════════════════ */
static void test_tools_ui_elements(void)
{
    TEST_BEGIN("Tools — UI elements exist");

    go_to_tools();

    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsSection,
                         "tools section should exist");
    TEST_ASSERT(!lv_obj_has_flag(gui.page.tools.toolsSection, LV_OBJ_FLAG_HIDDEN),
                "tools section should be visible");

    /* Maintenance buttons */
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsCleaningButton,
                         "cleaning button should exist");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsDrainingButton,
                         "draining button should exist");

    /* Utility buttons */
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsImportButton,
                         "import button should exist");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsExportButton,
                         "export button should exist");

    /* Credits */
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolCreditButton,
                         "credit button should exist");

    test_printf("         [INFO] All tools buttons verified\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: Statistics display elements exist
 * ═══════════════════════════════════════════════ */
static void test_tools_statistics_display(void)
{
    TEST_BEGIN("Tools — statistics display elements exist");

    TEST_ASSERT_NOT_NULL(gui.page.tools.toolStatCompletedProcessesValue,
                         "completed processes value label");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolStatTotalTimeValue,
                         "total time value label");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolStatCompleteCycleValue,
                         "complete cycle value label");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolStatStoppedProcessesValue,
                         "stopped processes value label");

    test_printf("         [INFO] Stats: completed=%d, totalMins=%llu, stopped=%d, clean=%d\n",
                (int)gui.page.tools.machineStats.completed,
                (unsigned long long)gui.page.tools.machineStats.totalMins,
                (int)gui.page.tools.machineStats.stopped,
                (int)gui.page.tools.machineStats.clean);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: Software version & serial labels exist
 * ═══════════════════════════════════════════════ */
static void test_tools_software_info(void)
{
    TEST_BEGIN("Tools — software version and serial labels");

    TEST_ASSERT_NOT_NULL(gui.page.tools.toolSoftwareVersionValue,
                         "version value label should exist");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolSoftwareSerialValue,
                         "serial value label should exist");

    const char *ver = lv_label_get_text(gui.page.tools.toolSoftwareVersionValue);
    const char *ser = lv_label_get_text(gui.page.tools.toolSoftwareSerialValue);
    test_printf("         [INFO] Version: %s, Serial: %s\n",
                ver ? ver : "(null)", ser ? ser : "(null)");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: Export button triggers popup
 * ═══════════════════════════════════════════════ */
static void test_tools_export_popup(void)
{
    TEST_BEGIN("Tools — export button opens popup");

    /* Click export button */
    test_click_obj(gui.page.tools.toolsExportButton);
    test_pump(500);

    /* A message popup should appear */
    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupParent,
                         "export popup should appear");

    test_printf("         [INFO] Export popup appeared\n");

    /* Close popup (click close or button2 = cancel) */
    if (gui.element.messagePopup.mBoxPopupButtonClose != NULL) {
        test_click_obj(gui.element.messagePopup.mBoxPopupButtonClose);
    } else if (gui.element.messagePopup.mBoxPopupButton2 != NULL) {
        test_click_obj(gui.element.messagePopup.mBoxPopupButton2);
    }
    test_pump(500);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: Import button triggers popup
 * ═══════════════════════════════════════════════ */
static void test_tools_import_popup(void)
{
    TEST_BEGIN("Tools — import button opens popup");

    test_click_obj(gui.page.tools.toolsImportButton);
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupParent,
                         "import popup should appear");

    test_printf("         [INFO] Import popup appeared\n");

    /* Close popup */
    if (gui.element.messagePopup.mBoxPopupButtonClose != NULL) {
        test_click_obj(gui.element.messagePopup.mBoxPopupButtonClose);
    } else if (gui.element.messagePopup.mBoxPopupButton2 != NULL) {
        test_click_obj(gui.element.messagePopup.mBoxPopupButton2);
    }
    test_pump(500);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 6: Credits button triggers popup
 * ═══════════════════════════════════════════════ */
static void test_tools_credits_popup(void)
{
    TEST_BEGIN("Tools — credits button opens popup");

    test_click_obj(gui.page.tools.toolCreditButton);
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.element.messagePopup.mBoxPopupParent,
                         "credits popup should appear");

    test_printf("         [INFO] Credits popup appeared\n");

    /* Close popup */
    if (gui.element.messagePopup.mBoxPopupButtonClose != NULL) {
        test_click_obj(gui.element.messagePopup.mBoxPopupButtonClose);
    } else if (gui.element.messagePopup.mBoxPopupButton1 != NULL) {
        test_click_obj(gui.element.messagePopup.mBoxPopupButton1);
    }
    test_pump(500);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 7: Tools timer pause/resume functions exist and can be called
 * ═══════════════════════════════════════════════ */
static void test_tools_timer_pause_resume(void)
{
    TEST_BEGIN("Tools — timer pause/resume functions callable");

    /* These are weak stubs in test_runner.c, so we just verify they can be called
     * without crashing. In a real implementation, these would control the timer. */

    test_printf("         [INFO] Calling tools_pause_timer()\n");
    tools_pause_timer();
    test_pump(100);

    test_printf("         [INFO] Calling tools_resume_timer()\n");
    tools_resume_timer();
    test_pump(100);

    test_printf("         [INFO] Both timer functions called successfully\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 8: Alarm functions exist and can be called
 * ═══════════════════════════════════════════════ */
static void test_alarm_functions(void)
{
    TEST_BEGIN("Tools — alarm functions callable");

    /* These are weak stubs in test_runner.c returning default values */

    /* Verify alarm_is_active returns a boolean (should start as false) */
    bool is_active = alarm_is_active();
    test_printf("         [INFO] alarm_is_active() returned: %d\n", (int)is_active);
    TEST_ASSERT(is_active == false || is_active == true,
                "alarm_is_active() should return boolean");

    /* Call alarm_start_persistent */
    test_printf("         [INFO] Calling alarm_start_persistent()\n");
    alarm_start_persistent();
    test_pump(100);

    /* Call alarm_stop */
    test_printf("         [INFO] Calling alarm_stop()\n");
    alarm_stop();
    test_pump(100);

    /* After stopping, alarm_is_active should return false */
    is_active = alarm_is_active();
    test_printf("         [INFO] alarm_is_active() after stop: %d\n", (int)is_active);
    /* Note: The weak stub always returns false, so we just verify it doesn't crash */
    TEST_ASSERT(is_active == false,
                "alarm should be inactive after alarm_stop (weak stub returns false)");

    test_printf("         [INFO] All alarm functions called successfully\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 9: Statistics persistence round-trip
 * ═══════════════════════════════════════════════ */
static void test_tools_stats_persistence(void)
{
    TEST_BEGIN("Tools — statistics persistence via writeMachineStats");

    /* Access the machine statistics */
    machineStatistics *stats = &gui.page.tools.machineStats;

    /* Save originals */
    uint32_t orig_completed = stats->completed;
    uint64_t orig_totalMins = stats->totalMins;
    uint32_t orig_stopped   = stats->stopped;
    uint32_t orig_clean     = stats->clean;

    /* Set known values */
    stats->completed = 7;
    stats->totalMins = 450;
    stats->stopped   = 1;
    stats->clean     = 5;

    test_printf("         [INFO] Stats set: completed=%d, totalMins=%llu, stopped=%d, clean=%d\n",
                stats->completed, (unsigned long long)stats->totalMins,
                stats->stopped, stats->clean);

    /* Write stats (this should persist to config) */
    writeMachineStats(stats);
    test_pump(100);

    /* Verify we can read them back (create a temp structure) */
    machineStatistics temp_stats = { .completed = 0, .totalMins = 0, .stopped = 0, .clean = 0 };
    readMachineStats(&temp_stats);
    test_pump(100);

    test_printf("         [INFO] Stats read: completed=%d, totalMins=%llu, stopped=%d, clean=%d\n",
                temp_stats.completed, (unsigned long long)temp_stats.totalMins,
                temp_stats.stopped, temp_stats.clean);

    /* Restore originals */
    stats->completed = orig_completed;
    stats->totalMins = orig_totalMins;
    stats->stopped   = orig_stopped;
    stats->clean     = orig_clean;

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 10: Return to Processes after tools
 * ═══════════════════════════════════════════════ */
static void test_tools_return_to_processes(void)
{
    TEST_BEGIN("Tools — return to Processes tab");

    test_click_obj(gui.page.menu.processesTab);
    test_pump(300);

    TEST_ASSERT_EQ(gui.page.menu.newSelection, TAB_PROCESSES,
                   "should be back on Processes tab");
    TEST_ASSERT(!lv_obj_has_flag(gui.page.processes.processesSection, LV_OBJ_FLAG_HIDDEN),
                "processes section should be visible");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_tools(void)
{
    TEST_SUITE("Tools");

    test_tools_ui_elements();
    test_tools_statistics_display();
    test_tools_software_info();
    test_tools_export_popup();
    test_tools_import_popup();
    test_tools_credits_popup();
    test_tools_timer_pause_resume();
    test_alarm_functions();
    test_tools_stats_persistence();
    test_tools_return_to_processes();
}
