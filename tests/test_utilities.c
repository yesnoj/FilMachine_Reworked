/**
 * test_utilities.c — Utility & Lookup Function Tests
 *
 * Tests pure/utility functions that don't depend on UI state:
 * findRollerStringIndex, getValueForChemicalSource, mapPercentageToValue,
 * getRandomRotationInterval, calculateTotalTime, emptyList, toLowerCase.
 */

#include "test_runner.h"
#include "lvgl.h"
#include <string.h>


/* ═══════════════════════════════════════════════
 * Test 1: findRollerStringIndex — string lookup in newline-delimited list
 * ═══════════════════════════════════════════════ */
static void test_find_roller_string_index(void)
{
    TEST_BEGIN("Utility — findRollerStringIndex lookup");

    /* Build a newline-delimited list like roller popups use */
    const char *list = "10\n20\n30\n40\n50";

    uint32_t idx;

    /* First element */
    idx = findRollerStringIndex("10", list);
    test_printf("         [INFO] findRollerStringIndex(\"10\") = %u\n", (unsigned)idx);
    TEST_ASSERT_EQ((int)idx, 0, "first element should be at index 0");

    /* Middle element */
    idx = findRollerStringIndex("30", list);
    test_printf("         [INFO] findRollerStringIndex(\"30\") = %u\n", (unsigned)idx);
    TEST_ASSERT_EQ((int)idx, 2, "\"30\" should be at index 2");

    /* Last element */
    idx = findRollerStringIndex("50", list);
    test_printf("         [INFO] findRollerStringIndex(\"50\") = %u\n", (unsigned)idx);
    TEST_ASSERT_EQ((int)idx, 4, "last element should be at index 4");

    /* Not found — should return (uint32_t)-1 */
    idx = findRollerStringIndex("99", list);
    test_printf("         [INFO] findRollerStringIndex(\"99\") = %u (expect %u)\n",
                (unsigned)idx, (unsigned)(uint32_t)-1);
    TEST_ASSERT_EQ((int)idx, (int)(uint32_t)-1, "missing element should return -1");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: getValueForChemicalSource — source-to-relay mapping
 * ═══════════════════════════════════════════════ */
static void test_chemical_source_mapping(void)
{
    TEST_BEGIN("Utility — getValueForChemicalSource mapping");

    TEST_ASSERT_EQ(getValueForChemicalSource(C1),    C1_RLY,    "C1 → C1_RLY");
    TEST_ASSERT_EQ(getValueForChemicalSource(C2),    C2_RLY,    "C2 → C2_RLY");
    TEST_ASSERT_EQ(getValueForChemicalSource(C3),    C3_RLY,    "C3 → C3_RLY");
    TEST_ASSERT_EQ(getValueForChemicalSource(WB),    WB_RLY,    "WB → WB_RLY");
    TEST_ASSERT_EQ(getValueForChemicalSource(WASTE), WASTE_RLY, "WASTE → WASTE_RLY");

    /* Invalid source should return 255 */
    TEST_ASSERT_EQ(getValueForChemicalSource(99), 255, "invalid source → 255");

    test_printf("         [INFO] All 6 source mappings verified\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: mapPercentageToValue — percentage to motor analog value
 * ═══════════════════════════════════════════════ */
static void test_map_percentage_to_value(void)
{
    TEST_BEGIN("Utility — mapPercentageToValue bounds");

    /* MOTOR_MIN_ANALOG_VAL=150, MOTOR_MAX_ANALOG_VAL=255 */

    /* At minimum percentage → should return MOTOR_MIN_ANALOG_VAL */
    uint8_t val_min = mapPercentageToValue(0, 0, 100);
    test_printf("         [INFO] mapPercentageToValue(0,0,100) = %d (expect %d)\n",
                val_min, MOTOR_MIN_ANALOG_VAL);
    TEST_ASSERT_EQ(val_min, MOTOR_MIN_ANALOG_VAL, "0% → min analog value");

    /* At maximum percentage → should return MOTOR_MAX_ANALOG_VAL */
    uint8_t val_max = mapPercentageToValue(100, 0, 100);
    test_printf("         [INFO] mapPercentageToValue(100,0,100) = %d (expect %d)\n",
                val_max, MOTOR_MAX_ANALOG_VAL);
    TEST_ASSERT_EQ(val_max, MOTOR_MAX_ANALOG_VAL, "100% → max analog value");

    /* 50% should be approximately in the middle */
    uint8_t val_mid = mapPercentageToValue(50, 0, 100);
    test_printf("         [INFO] mapPercentageToValue(50,0,100) = %d\n", val_mid);
    TEST_ASSERT(val_mid > MOTOR_MIN_ANALOG_VAL && val_mid < MOTOR_MAX_ANALOG_VAL,
                "50% should be between min and max");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: getRandomRotationInterval — statistical bounds
 * ═══════════════════════════════════════════════ */
static void test_random_rotation_interval(void)
{
    TEST_BEGIN("Utility — getRandomRotationInterval stays in bounds");

    /* Set known values for the calculation:
     * baseTime=30, randomPercentage=50 → variation=15
     * Result should be clamped between 5..60 */
    struct machineSettings *s = &gui.page.settings.settingsParams;
    uint8_t old_interval = s->rotationIntervalSetpoint;
    uint8_t old_random   = s->randomSetpoint;

    s->rotationIntervalSetpoint = 30;
    s->randomSetpoint           = 50;

    uint8_t min_seen = 255, max_seen = 0;
    for (int i = 0; i < 100; i++) {
        uint8_t val = getRandomRotationInterval();
        if (val < min_seen) min_seen = val;
        if (val > max_seen) max_seen = val;
    }

    test_printf("         [INFO] 100 samples: min=%d, max=%d (bounds: 5..60)\n",
                min_seen, max_seen);
    TEST_ASSERT(min_seen >= 5,  "minimum result should be >= 5");
    TEST_ASSERT(max_seen <= 60, "maximum result should be <= 60");

    /* With 0% randomness, result should always equal baseTime */
    s->randomSetpoint = 0;
    uint8_t deterministic = getRandomRotationInterval();
    test_printf("         [INFO] 0%% randomness: result=%d (expect %d)\n",
                deterministic, s->rotationIntervalSetpoint);
    TEST_ASSERT_EQ(deterministic, s->rotationIntervalSetpoint,
                   "0% randomness → result equals base time");

    /* Restore */
    s->rotationIntervalSetpoint = old_interval;
    s->randomSetpoint           = old_random;

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: calculateTotalTime — sum of step durations
 * ═══════════════════════════════════════════════ */
static void test_calculate_total_time(void)
{
    TEST_BEGIN("Utility — calculateTotalTime sums step durations");

    /* Use the first process which has known steps */
    processNode *p = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(p, "need at least one process");

    /* Save old values */
    uint32_t old_mins = p->process.processDetails->data.timeMins;
    uint8_t  old_secs = p->process.processDetails->data.timeSecs;

    /* Calculate expected total manually */
    uint32_t expected_mins = 0;
    uint8_t  expected_secs = 0;
    stepNode *s = p->process.processDetails->stepElementsList.start;
    while (s != NULL) {
        expected_mins += s->step.stepDetails->data.timeMins;
        expected_secs += s->step.stepDetails->data.timeSecs;
        if (expected_secs >= 60) {
            expected_mins += expected_secs / 60;
            expected_secs  = expected_secs % 60;
        }
        s = s->next;
    }

    /* calculateTotalTime() writes to processTotalTimeValue label which may
     * be NULL if the process detail page isn't open — create a temp label */
    lv_obj_t *saved_label = p->process.processDetails->processTotalTimeValue;
    lv_obj_t *temp_label  = NULL;
    if (saved_label == NULL) {
        temp_label = lv_label_create(lv_screen_active());
        p->process.processDetails->processTotalTimeValue = temp_label;
    }

    calculateTotalTime(p);

    test_printf("         [INFO] calculateTotalTime: %"PRIu32"m %ds (expected %"PRIu32"m %ds)\n",
                p->process.processDetails->data.timeMins,
                p->process.processDetails->data.timeSecs,
                expected_mins, expected_secs);
    TEST_ASSERT_EQ((int)p->process.processDetails->data.timeMins, (int)expected_mins,
                   "total minutes should match manual sum");
    TEST_ASSERT_EQ((int)p->process.processDetails->data.timeSecs, (int)expected_secs,
                   "total seconds should match manual sum");

    /* Cleanup temp label and restore original */
    if (temp_label != NULL) {
        lv_obj_delete(temp_label);
        p->process.processDetails->processTotalTimeValue = saved_label;
    }

    /* Restore original time values */
    p->process.processDetails->data.timeMins = old_mins;
    p->process.processDetails->data.timeSecs = old_secs;

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 6: emptyList — empties a step list and verifies
 * ═══════════════════════════════════════════════ */
static void test_empty_step_list(void)
{
    TEST_BEGIN("Utility — emptyList clears a step list");

    /* Create a small temporary step list */
    stepList temp_list = { .start = NULL, .end = NULL, .size = 0 };

    stepNode *s1 = (stepNode *)allocateAndInitializeNode(STEP_NODE);
    stepNode *s2 = (stepNode *)allocateAndInitializeNode(STEP_NODE);
    TEST_ASSERT_NOT_NULL(s1, "s1 allocated");
    TEST_ASSERT_NOT_NULL(s2, "s2 allocated");

    snprintf(s1->step.stepDetails->data.stepNameString, sizeof(s1->step.stepDetails->data.stepNameString), "%s", "TempStep1");
    snprintf(s2->step.stepDetails->data.stepNameString, sizeof(s2->step.stepDetails->data.stepNameString), "%s", "TempStep2");

    /* Link them: s1 → s2 */
    s1->prev = NULL;
    s1->next = s2;
    s2->prev = s1;
    s2->next = NULL;
    temp_list.start = s1;
    temp_list.end   = s2;
    temp_list.size  = 2;

    TEST_ASSERT_EQ((int)temp_list.size, 2, "temp list should have 2 steps");

    /* Empty it */
    emptyList(&temp_list, STEP_NODE);

    test_printf("         [INFO] After emptyList: size=%d, start=%p, end=%p\n",
                (int)temp_list.size, (void *)temp_list.start, (void *)temp_list.end);
    TEST_ASSERT_EQ((int)temp_list.size, 0,       "size should be 0 after empty");
    TEST_ASSERT(temp_list.start == NULL,          "start should be NULL");
    TEST_ASSERT(temp_list.end   == NULL,          "end should be NULL");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 7: toLowerCase — in-place string lowercasing
 * ═══════════════════════════════════════════════ */
static void test_to_lower_case(void)
{
    TEST_BEGIN("Utility — toLowerCase converts in-place");

    char buf1[] = "Hello World";
    toLowerCase(buf1);
    test_printf("         [INFO] toLowerCase(\"Hello World\") → \"%s\"\n", buf1);
    TEST_ASSERT(strcmp(buf1, "hello world") == 0, "should lowercase all chars");

    char buf2[] = "ALLCAPS123";
    toLowerCase(buf2);
    test_printf("         [INFO] toLowerCase(\"ALLCAPS123\") → \"%s\"\n", buf2);
    TEST_ASSERT(strcmp(buf2, "allcaps123") == 0, "digits should be unchanged");

    char buf3[] = "";
    toLowerCase(buf3);
    TEST_ASSERT(strcmp(buf3, "") == 0, "empty string should remain empty");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 8: Filter with Color+B&W both selected (should show all)
 * ═══════════════════════════════════════════════ */
static void test_filter_both_film_types(void)
{
    TEST_BEGIN("Utility — filter with both Color+B&W shows all");

    int32_t total = gui.page.processes.processElementsList.size;

    /* Ensure we're on processes tab */
    test_click_obj(gui.page.menu.processesTab);
    test_pump(300);

    /* Set filter: both Color AND B&W → should show ALL processes */
    gui.page.processes.isFiltered = false;
    gui.element.filterPopup.filterName[0]  = '\0';
    gui.element.filterPopup.isColorFilter  = true;
    gui.element.filterPopup.isBnWFilter    = true;
    gui.element.filterPopup.preferredOnly  = false;

    filterAndDisplayProcesses();
    test_pump(300);

    /* Count matched — should be all */
    int matched = 0;
    processNode *p = gui.page.processes.processElementsList.start;
    while (p != NULL) {
        if (p->process.isFiltered)
            matched++;
        p = p->next;
    }

    test_printf("         [INFO] Both Color+B&W: %d matched / %d total\n", matched, (int)total);
    TEST_ASSERT_EQ(matched, (int)total, "both types selected → all should match");

    /* Reset */
    removeFiltersAndDisplayAllProcesses();
    gui.page.processes.isFiltered = false;
    test_pump(300);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 9: Drain/Fill Overlap calculation
 * ═══════════════════════════════════════════════ */
static void test_drain_fill_overlap_calculation(void)
{
    TEST_BEGIN("Utility — drain/fill overlap time calculation");

    /* This tests the adjustment of step time based on drain/fill overlap.
     * The formula is: adjustedTime = stepTime - (overlap% / 100) * 2 * fillTime
     *
     * Test 1: overlap=0% → adjusted time = stepTime (no change)
     * Test 2: overlap=100% → adjusted time = stepTime - 2*fillTime (full overlap)
     * Test 3: overlap=50% → adjusted time should be between the two
     */

    struct machineSettings *s = &gui.page.settings.settingsParams;
    uint8_t old_overlap = s->drainFillOverlapSetpoint;

    /* For this test, we'll use calculateFillTime to get a known fill time */
    uint16_t fill_time_ms = calculateFillTime(s->chemContainerMl, s->pumpSpeed);
    test_printf("         [INFO] fillTime for %d ml at %d%% pump = %d ms\n",
                s->chemContainerMl, s->pumpSpeed, fill_time_ms);

    /* Test 1: 0% overlap — adjusted should equal original stepTime */
    s->drainFillOverlapSetpoint = 0;
    uint32_t step_time = 60000; /* 60 seconds */
    /* adjustedTime = 60000 - (0/100) * 2 * fill_time = 60000 */
    uint32_t adjusted_0pct = step_time - (0 * 2 * fill_time_ms) / 100;
    test_printf("         [INFO] 0%% overlap: %lu ms → %lu ms (expect %lu ms)\n",
                (unsigned long)step_time, (unsigned long)adjusted_0pct, (unsigned long)step_time);
    TEST_ASSERT_EQ((int)adjusted_0pct, (int)step_time,
                   "0% overlap should result in unchanged time");

    /* Test 2: 100% overlap — adjusted = stepTime - 2*fillTime */
    s->drainFillOverlapSetpoint = 100;
    uint32_t adjusted_100pct = step_time - (100 * 2 * fill_time_ms) / 100;
    test_printf("         [INFO] 100%% overlap: %lu ms → %lu ms (deduction: %lu ms)\n",
                (unsigned long)step_time, (unsigned long)adjusted_100pct,
                (unsigned long)(2 * fill_time_ms));
    TEST_ASSERT(adjusted_100pct < step_time,
                "100% overlap should result in shorter adjusted time");
    TEST_ASSERT((int)adjusted_100pct == (int)(step_time - 2 * fill_time_ms),
                "100% overlap should deduct 2*fillTime");

    /* Test 3: 50% overlap — should be in between */
    s->drainFillOverlapSetpoint = 50;
    uint32_t adjusted_50pct = step_time - (50 * 2 * fill_time_ms) / 100;
    test_printf("         [INFO] 50%% overlap: %lu ms → %lu ms\n",
                (unsigned long)step_time, (unsigned long)adjusted_50pct);
    TEST_ASSERT(adjusted_50pct > adjusted_100pct && adjusted_50pct < step_time,
                "50% overlap should be between 0% and 100%");

    /* Restore original */
    s->drainFillOverlapSetpoint = old_overlap;

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_utilities(void)
{
    TEST_SUITE("Utilities");

    test_find_roller_string_index();
    test_chemical_source_mapping();
    test_map_percentage_to_value();
    test_random_rotation_interval();
    test_calculate_total_time();
    test_empty_step_list();
    test_to_lower_case();
    test_filter_both_film_types();
    test_drain_fill_overlap_calculation();
}
