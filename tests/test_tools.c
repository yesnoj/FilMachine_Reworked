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
 * Test 7: Return to Processes after tools
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
    test_tools_return_to_processes();
}
