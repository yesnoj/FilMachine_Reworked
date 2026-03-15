/**
 * test_edge_cases.c — Edge Cases, Utility Functions & Data Integrity Tests
 *
 * Tests: temperature conversion, case-insensitive search, percentage
 * calculation, deep copy, process name max length, step field types,
 * and data integrity after operations.
 */

#include "test_runner.h"
#include "lvgl.h"


/* ═══════════════════════════════════════════════
 * Test 1: Temperature conversion — known values
 * ═══════════════════════════════════════════════ */
static void test_temp_conversion(void)
{
    TEST_BEGIN("Utility — Celsius to Fahrenheit conversion");

    /* 0°C = 32°F */
    int32_t f0 = convertCelsiusoToFahrenheit(0);
    test_printf("         [INFO] 0°C → %d°F (expected 32)\n", (int)f0);
    TEST_ASSERT_EQ(f0, 32, "0°C should be 32°F");

    /* 100°C = 212°F */
    int32_t f100 = convertCelsiusoToFahrenheit(100);
    test_printf("         [INFO] 100°C → %d°F (expected 212)\n", (int)f100);
    TEST_ASSERT_EQ(f100, 212, "100°C should be 212°F");

    /* 20°C = 68°F */
    int32_t f20 = convertCelsiusoToFahrenheit(20);
    test_printf("         [INFO] 20°C → %d°F (expected 68)\n", (int)f20);
    TEST_ASSERT_EQ(f20, 68, "20°C should be 68°F");

    /* 38°C = 100°F (rounded: 38*1.8+32 = 100.4 → 101 with +0.5 rounding) */
    int32_t f38 = convertCelsiusoToFahrenheit(38);
    test_printf("         [INFO] 38°C → %d°F (expected ~100-101)\n", (int)f38);
    TEST_ASSERT(f38 >= 100 && f38 <= 101, "38°C should be ~100-101°F");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: Case-insensitive string search
 * ═══════════════════════════════════════════════ */
static void test_case_insensitive_search(void)
{
    TEST_BEGIN("Utility — caseInsensitiveStrstr");

    /* Exact match */
    uint8_t r1 = caseInsensitiveStrstr("Hello World", "Hello");
    TEST_ASSERT_EQ(r1, 1, "exact match should return 1");

    /* Case-insensitive match */
    uint8_t r2 = caseInsensitiveStrstr("Hello World", "hello");
    TEST_ASSERT_EQ(r2, 1, "lowercase needle should match uppercase haystack");

    /* Partial match */
    uint8_t r3 = caseInsensitiveStrstr("Test C41 Process", "c41");
    TEST_ASSERT_EQ(r3, 1, "'c41' should match 'Test C41 Process'");

    /* No match */
    uint8_t r4 = caseInsensitiveStrstr("Hello World", "xyz");
    TEST_ASSERT_EQ(r4, 0, "non-matching should return 0");

    /* Empty needle */
    uint8_t r5 = caseInsensitiveStrstr("Hello", "");
    TEST_ASSERT_EQ(r5, 1, "empty needle should match everything");

    test_printf("         [INFO] All 5 search cases passed\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: Percentage calculation
 * ═══════════════════════════════════════════════ */
static void test_percentage_calculation(void)
{
    TEST_BEGIN("Utility — calculatePercentage");

    /* 50% case: 30 sec out of 1 min */
    uint8_t p1 = calculatePercentage(0, 30, 1, 0);
    test_printf("         [INFO] 30s / 60s = %d%% (expected 50)\n", (int)p1);
    TEST_ASSERT_EQ(p1, 50, "30s of 60s should be 50%");

    /* 0% case */
    uint8_t p2 = calculatePercentage(0, 0, 5, 0);
    test_printf("         [INFO] 0s / 300s = %d%% (expected 0)\n", (int)p2);
    TEST_ASSERT_EQ(p2, 0, "0 elapsed should be 0%");

    /* 100% case */
    uint8_t p3 = calculatePercentage(5, 0, 5, 0);
    test_printf("         [INFO] 300s / 300s = %d%% (expected 100)\n", (int)p3);
    TEST_ASSERT_EQ(p3, 100, "equal times should be 100%");

    /* Overflow case: more elapsed than total → capped at 100 */
    uint8_t p4 = calculatePercentage(10, 0, 5, 0);
    test_printf("         [INFO] 600s / 300s = %d%% (expected 100, capped)\n", (int)p4);
    TEST_ASSERT_EQ(p4, 100, "overflow should cap at 100%");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: Deep copy step node
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_step(void)
{
    TEST_BEGIN("Deep copy — step node preserves all fields");

    /* Get first process's first step */
    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");
    stepNode *orig = proc->process.processDetails->stepElementsList.start;
    TEST_ASSERT_NOT_NULL(orig, "first step should exist");

    /* Deep copy */
    stepNode *copy = deepCopyStepNode(orig);
    TEST_ASSERT_NOT_NULL(copy, "deep copy should not return NULL");

    /* Verify fields match */
    TEST_ASSERT_STR_EQ(copy->step.stepDetails->stepNameString,
                       orig->step.stepDetails->stepNameString,
                       "copied step name should match original");
    TEST_ASSERT_EQ(copy->step.stepDetails->timeMins,
                   orig->step.stepDetails->timeMins,
                   "copied timeMins should match");
    TEST_ASSERT_EQ(copy->step.stepDetails->timeSecs,
                   orig->step.stepDetails->timeSecs,
                   "copied timeSecs should match");
    TEST_ASSERT_EQ(copy->step.stepDetails->type,
                   orig->step.stepDetails->type,
                   "copied type should match");
    TEST_ASSERT_EQ(copy->step.stepDetails->source,
                   orig->step.stepDetails->source,
                   "copied source should match");

    /* Verify it's a separate allocation (different pointers) */
    TEST_ASSERT(copy != orig, "copy should be a different pointer");
    TEST_ASSERT(copy->step.stepDetails != orig->step.stepDetails,
                "copy stepDetails should be different pointer");

    test_printf("         [INFO] Deep copy step verified: \"%s\" (%d:%02d)\n",
                copy->step.stepDetails->stepNameString,
                (int)copy->step.stepDetails->timeMins,
                (int)copy->step.stepDetails->timeSecs);

    /* Clean up — free the copy (manually, since it's not in any list) */
    free(copy->step.stepDetails);
    free(copy);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: Deep copy process node
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_process(void)
{
    TEST_BEGIN("Deep copy — process node preserves name, steps, temp");

    processNode *orig = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(orig, "first process should exist");

    struct processNode *copy = deepCopyProcessNode(orig);
    TEST_ASSERT_NOT_NULL(copy, "deep copy should not return NULL");

    /* Verify name */
    TEST_ASSERT_STR_EQ(copy->process.processDetails->processNameString,
                       orig->process.processDetails->processNameString,
                       "copied name should match original");

    /* Verify temperature fields */
    TEST_ASSERT_EQ(copy->process.processDetails->temp,
                   orig->process.processDetails->temp,
                   "copied temp should match");
    TEST_ASSERT_EQ(copy->process.processDetails->filmType,
                   orig->process.processDetails->filmType,
                   "copied filmType should match");

    /* Verify step count matches */
    int32_t orig_steps = orig->process.processDetails->stepElementsList.size;
    int32_t copy_steps = copy->process.processDetails->stepElementsList.size;
    test_printf("         [INFO] Original steps: %d, Copy steps: %d\n",
                (int)orig_steps, (int)copy_steps);
    TEST_ASSERT_EQ(copy_steps, orig_steps, "step count should match");

    /* Different pointers */
    TEST_ASSERT(copy != orig, "copy should be different pointer");

    /* Clean up — free copied process steps then process */
    stepNode *s = copy->process.processDetails->stepElementsList.start;
    while (s != NULL) {
        stepNode *next = s->next;
        if (s->step.stepDetails) free(s->step.stepDetails);
        free(s);
        s = next;
    }
    free(copy->process.processDetails);
    free(copy);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 6: Step types and sources stored correctly
 * ═══════════════════════════════════════════════ */
static void test_step_type_source_fields(void)
{
    TEST_BEGIN("Data — step type and source fields persist correctly");

    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");

    stepNode *s = proc->process.processDetails->stepElementsList.start;
    TEST_ASSERT_NOT_NULL(s, "first step should exist");

    /* Walk all steps and verify type/source are within valid ranges */
    int step_idx = 0;
    while (s != NULL) {
        uint8_t type = s->step.stepDetails->type;
        uint8_t source = s->step.stepDetails->source;

        /* Type should be 0=CHEMISTRY, 1=RINSE, or 2=MULTI_RINSE */
        TEST_ASSERT(type <= 2, "step type should be 0-2");

        /* Source should be 0-4 (C1, C2, C3, WB, WASTE) */
        TEST_ASSERT(source <= 4, "step source should be 0-4");

        test_printf("         [INFO] Step %d: type=%d source=%d discard=%d\n",
                    step_idx, type, source, s->step.stepDetails->discardAfterProc);

        s = s->next;
        step_idx++;
    }
    test_printf("         [INFO] Verified %d steps\n", step_idx);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 7: Process name max length handling
 * ═══════════════════════════════════════════════ */
static void test_process_name_max_length(void)
{
    TEST_BEGIN("Edge — process name at MAX_PROC_NAME_LEN boundary");

    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");

    /* Save original name */
    char original_name[MAX_PROC_NAME_LEN + 1];
    strncpy(original_name, proc->process.processDetails->processNameString, MAX_PROC_NAME_LEN);
    original_name[MAX_PROC_NAME_LEN] = '\0';

    /* Set name to exactly MAX_PROC_NAME_LEN chars */
    char max_name[MAX_PROC_NAME_LEN + 1];
    memset(max_name, 'A', MAX_PROC_NAME_LEN);
    max_name[MAX_PROC_NAME_LEN] = '\0';

    strncpy(proc->process.processDetails->processNameString, max_name, MAX_PROC_NAME_LEN);
    test_printf("         [INFO] Set %d-char name\n", (int)strlen(max_name));

    /* Verify it was stored */
    int len = strlen(proc->process.processDetails->processNameString);
    TEST_ASSERT(len <= MAX_PROC_NAME_LEN, "stored name should not exceed max");
    test_printf("         [INFO] Stored name length: %d (max=%d)\n", len, MAX_PROC_NAME_LEN);

    /* Restore original name */
    strncpy(proc->process.processDetails->processNameString, original_name, MAX_PROC_NAME_LEN);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 8: Process order preserved after operations
 * ═══════════════════════════════════════════════ */
static void test_process_order_integrity(void)
{
    TEST_BEGIN("Data — process list doubly-linked integrity");

    processList *list = &gui.page.processes.processElementsList;
    int32_t count = list->size;
    TEST_ASSERT(count > 0, "need at least one process");

    /* Walk forward, counting nodes */
    int fwd = 0;
    processNode *p = list->start;
    processNode *last_forward = NULL;
    while (p != NULL) {
        last_forward = p;
        fwd++;
        p = p->next;
    }
    TEST_ASSERT_EQ(fwd, (int)count, "forward walk count should match size");
    TEST_ASSERT(last_forward == list->end, "last forward node should be list->end");

    /* Walk backward from end */
    int bwd = 0;
    p = list->end;
    processNode *first_backward = NULL;
    while (p != NULL) {
        first_backward = p;
        bwd++;
        p = p->prev;
    }
    TEST_ASSERT_EQ(bwd, (int)count, "backward walk count should match size");
    TEST_ASSERT(first_backward == list->start, "first backward node should be list->start");

    test_printf("         [INFO] List integrity: fwd=%d, bwd=%d, size=%d\n",
                fwd, bwd, (int)count);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 9: Step list doubly-linked integrity
 * ═══════════════════════════════════════════════ */
static void test_step_list_integrity(void)
{
    TEST_BEGIN("Data — step list doubly-linked integrity");

    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");

    stepList *list = &proc->process.processDetails->stepElementsList;
    int32_t count = list->size;
    TEST_ASSERT(count > 0, "need at least one step");

    /* Walk forward */
    int fwd = 0;
    stepNode *s = list->start;
    stepNode *last = NULL;
    while (s != NULL) {
        last = s;
        fwd++;
        s = s->next;
    }
    TEST_ASSERT_EQ(fwd, (int)count, "forward step walk should match size");
    TEST_ASSERT(last == list->end, "last step should be list->end");

    /* Walk backward */
    int bwd = 0;
    s = list->end;
    stepNode *first = NULL;
    while (s != NULL) {
        first = s;
        bwd++;
        s = s->prev;
    }
    TEST_ASSERT_EQ(bwd, (int)count, "backward step walk should match size");
    TEST_ASSERT(first == list->start, "first step should be list->start");

    test_printf("         [INFO] Step list: fwd=%d, bwd=%d, size=%d\n",
                fwd, bwd, (int)count);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_edge_cases(void)
{
    TEST_SUITE("Edge Cases & Utilities");

    test_temp_conversion();
    test_case_insensitive_search();
    test_percentage_calculation();
    test_deep_copy_step();
    test_deep_copy_process();
    test_step_type_source_fields();
    test_process_name_max_length();
    test_process_order_integrity();
    test_step_list_integrity();
}
