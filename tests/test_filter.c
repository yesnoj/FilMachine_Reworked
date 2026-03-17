/**
 * test_filter.c — Process Filter Functionality Tests
 *
 * Tests the filter popup creation, filtering by name,
 * by film type, by preferred flag, and reset behavior.
 */

#include "test_runner.h"
#include "lvgl.h"


/* ═══════════════════════════════════════════════
 * Test 1: Filter popup creation
 * ═══════════════════════════════════════════════ */
static void test_filter_popup_create(void)
{
    TEST_BEGIN("Filter — popup creation via filterPopupCreate()");

    /* Ensure we're on processes tab */
    test_click_obj(gui.page.menu.processesTab);
    test_pump(300);

    /* Create filter popup */
    filterPopupCreate();
    test_pump(500);

    TEST_ASSERT_NOT_NULL(gui.element.filterPopup.mBoxFilterPopupParent,
                         "filter popup parent should exist");
    TEST_ASSERT_NOT_NULL(gui.element.filterPopup.mBoxContainer,
                         "filter container should exist");
    TEST_ASSERT_NOT_NULL(gui.element.filterPopup.mBoxApplyFilterLabel,
                         "apply filter label should exist");
    TEST_ASSERT_NOT_NULL(gui.element.filterPopup.mBoxResetFilterLabel,
                         "reset filter label should exist");

    test_printf("         [INFO] Filter popup created at %p\n",
           (void *)gui.element.filterPopup.mBoxFilterPopupParent);

    /* Close the filter popup (delete it) */
    lv_obj_delete(gui.element.filterPopup.mBoxFilterPopupParent);
    gui.element.filterPopup.mBoxFilterPopupParent = NULL;
    test_pump(200);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: Filter by name — direct API test
 * ═══════════════════════════════════════════════ */
static void test_filter_by_name(void)
{
    TEST_BEGIN("Filter — filterAndDisplayProcesses excludes non-matching");

    int32_t total = gui.page.processes.processElementsList.size;
    TEST_ASSERT(total >= 2, "need at least 2 processes for filter test");

    /* Set filter: name = "Test" — matches "Test B&W", "Quick Test",
       but not "Modified Name" (which was renamed by CRUD tests) */
    gui.page.processes.isFiltered = false;
    snprintf(gui.element.filterPopup.filterName, sizeof(gui.element.filterPopup.filterName), "%s", "Test");
    gui.element.filterPopup.isColorFilter  = false;
    gui.element.filterPopup.isBnWFilter    = false;
    gui.element.filterPopup.preferredOnly  = false;

    filterAndDisplayProcesses();
    test_pump(300);

    /* Count matched processes (isFiltered==true means included by filter) */
    int matched = 0;
    processNode *p = gui.page.processes.processElementsList.start;
    while (p != NULL) {
        if (p->process.isFiltered)
            matched++;
        p = p->next;
    }

    test_printf("         [INFO] Filter 'Test': %d matched / %d total\n", matched, (int)total);

    /* At least one should be excluded (not all contain "Test") */
    TEST_ASSERT(matched < total, "filter should exclude some processes");
    TEST_ASSERT(matched > 0, "filter should include at least one match");

    /* Reset filter */
    removeFiltersAndDisplayAllProcesses();
    gui.page.processes.isFiltered = false;
    test_pump(300);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: Filter by film type
 * ═══════════════════════════════════════════════ */
static void test_filter_by_film_type(void)
{
    TEST_BEGIN("Filter — by film type (B&W only)");

    int32_t total = gui.page.processes.processElementsList.size;
    TEST_ASSERT(total >= 2, "need at least 2 processes");

    /* Count B&W processes manually */
    int bnw_count = 0;
    processNode *p = gui.page.processes.processElementsList.start;
    while (p != NULL) {
        if (p->process.processDetails->data.filmType == BLACK_AND_WHITE_FILM)
            bnw_count++;
        p = p->next;
    }
    test_printf("         [INFO] B&W processes: %d / %d total\n", bnw_count, (int)total);

    /* Set filter: B&W only */
    gui.page.processes.isFiltered = false;
    gui.element.filterPopup.filterName[0]  = '\0';
    gui.element.filterPopup.isColorFilter  = false;
    gui.element.filterPopup.isBnWFilter    = true;
    gui.element.filterPopup.preferredOnly  = false;

    filterAndDisplayProcesses();
    test_pump(300);

    /* Count matched processes (isFiltered==true means included) */
    int matched = 0;
    p = gui.page.processes.processElementsList.start;
    while (p != NULL) {
        if (p->process.isFiltered)
            matched++;
        p = p->next;
    }

    test_printf("         [INFO] B&W filter: %d matched\n", matched);
    TEST_ASSERT_EQ(matched, bnw_count, "matched should equal B&W count");

    /* Reset */
    removeFiltersAndDisplayAllProcesses();
    gui.page.processes.isFiltered = false;
    test_pump(300);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: Filter by preferred
 * ═══════════════════════════════════════════════ */
static void test_filter_by_preferred(void)
{
    TEST_BEGIN("Filter — preferred only");

    int32_t total = gui.page.processes.processElementsList.size;

    /* Count preferred processes */
    int pref_count = 0;
    processNode *p = gui.page.processes.processElementsList.start;
    while (p != NULL) {
        if (p->process.processDetails->data.isPreferred)
            pref_count++;
        p = p->next;
    }
    test_printf("         [INFO] Preferred: %d / %d total\n", pref_count, (int)total);

    /* Set filter: preferred only */
    gui.page.processes.isFiltered = false;
    gui.element.filterPopup.filterName[0]  = '\0';
    gui.element.filterPopup.isColorFilter  = false;
    gui.element.filterPopup.isBnWFilter    = false;
    gui.element.filterPopup.preferredOnly  = true;

    filterAndDisplayProcesses();
    test_pump(300);

    /* Count matched (isFiltered==true means included) */
    int matched = 0;
    p = gui.page.processes.processElementsList.start;
    while (p != NULL) {
        if (p->process.isFiltered)
            matched++;
        p = p->next;
    }

    test_printf("         [INFO] Preferred filter: %d matched\n", matched);
    TEST_ASSERT_EQ(matched, pref_count, "matched should equal preferred count");

    /* Reset */
    removeFiltersAndDisplayAllProcesses();
    gui.page.processes.isFiltered = false;
    test_pump(300);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: Reset filter restores all
 * ═══════════════════════════════════════════════ */
static void test_filter_reset(void)
{
    TEST_BEGIN("Filter — reset shows all processes");

    int32_t total = gui.page.processes.processElementsList.size;

    /* Apply a restrictive filter first */
    gui.page.processes.isFiltered = false;
    snprintf(gui.element.filterPopup.filterName, sizeof(gui.element.filterPopup.filterName), "%s", "ZZZZZ_NO_MATCH");
    gui.element.filterPopup.isColorFilter  = false;
    gui.element.filterPopup.isBnWFilter    = false;
    gui.element.filterPopup.preferredOnly  = false;

    filterAndDisplayProcesses();
    test_pump(300);

    /* Count matched — should be 0 (nothing matches "ZZZZZ_NO_MATCH") */
    int matched_filtered = 0;
    processNode *p = gui.page.processes.processElementsList.start;
    while (p != NULL) {
        if (p->process.isFiltered)
            matched_filtered++;
        p = p->next;
    }
    test_printf("         [INFO] After impossible filter: %d matched\n", matched_filtered);
    TEST_ASSERT_EQ(matched_filtered, 0, "impossible filter should match none");

    /* Reset filter */
    removeFiltersAndDisplayAllProcesses();
    gui.page.processes.isFiltered = false;
    test_pump(300);

    /* After reset, container should have all processes re-created */
    uint32_t child_count = lv_obj_get_child_count(gui.page.processes.processesListContainer);
    test_printf("         [INFO] After reset: %u children / %d total\n",
                (unsigned)child_count, (int)total);
    TEST_ASSERT_EQ((int)child_count, (int)total, "reset should show all processes");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_filter(void)
{
    TEST_SUITE("Filter");

    test_filter_popup_create();
    test_filter_by_name();
    test_filter_by_film_type();
    test_filter_by_preferred();
    test_filter_reset();
}
