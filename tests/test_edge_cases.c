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
    int32_t f0 = convertCelsiusToFahrenheit(0);
    test_printf("         [INFO] 0°C → %d°F (expected 32)\n", (int)f0);
    TEST_ASSERT_EQ(f0, 32, "0°C should be 32°F");

    /* 100°C = 212°F */
    int32_t f100 = convertCelsiusToFahrenheit(100);
    test_printf("         [INFO] 100°C → %d°F (expected 212)\n", (int)f100);
    TEST_ASSERT_EQ(f100, 212, "100°C should be 212°F");

    /* 20°C = 68°F */
    int32_t f20 = convertCelsiusToFahrenheit(20);
    test_printf("         [INFO] 20°C → %d°F (expected 68)\n", (int)f20);
    TEST_ASSERT_EQ(f20, 68, "20°C should be 68°F");

    /* 38°C = 100°F (rounded: 38*1.8+32 = 100.4 → 101 with +0.5 rounding) */
    int32_t f38 = convertCelsiusToFahrenheit(38);
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
    TEST_ASSERT_STR_EQ(copy->step.stepDetails->data.stepNameString,
                       orig->step.stepDetails->data.stepNameString,
                       "copied step name should match original");
    TEST_ASSERT_EQ(copy->step.stepDetails->data.timeMins,
                   orig->step.stepDetails->data.timeMins,
                   "copied timeMins should match");
    TEST_ASSERT_EQ(copy->step.stepDetails->data.timeSecs,
                   orig->step.stepDetails->data.timeSecs,
                   "copied timeSecs should match");
    TEST_ASSERT_EQ(copy->step.stepDetails->data.type,
                   orig->step.stepDetails->data.type,
                   "copied type should match");
    TEST_ASSERT_EQ(copy->step.stepDetails->data.source,
                   orig->step.stepDetails->data.source,
                   "copied source should match");

    /* Verify it's a separate allocation (different pointers) */
    TEST_ASSERT(copy != orig, "copy should be a different pointer");
    TEST_ASSERT(copy->step.stepDetails != orig->step.stepDetails,
                "copy stepDetails should be different pointer");

    test_printf("         [INFO] Deep copy step verified: \"%s\" (%d:%02d)\n",
                copy->step.stepDetails->data.stepNameString,
                (int)copy->step.stepDetails->data.timeMins,
                (int)copy->step.stepDetails->data.timeSecs);

    /* Clean up — destroy the copy (not in any list) */
    step_node_destroy(copy);

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
    TEST_ASSERT_STR_EQ(copy->process.processDetails->data.processNameString,
                       orig->process.processDetails->data.processNameString,
                       "copied name should match original");

    /* Verify temperature fields */
    TEST_ASSERT_EQ(copy->process.processDetails->data.temp,
                   orig->process.processDetails->data.temp,
                   "copied temp should match");
    TEST_ASSERT_EQ(copy->process.processDetails->data.filmType,
                   orig->process.processDetails->data.filmType,
                   "copied filmType should match");

    /* Verify step count matches */
    int32_t orig_steps = orig->process.processDetails->stepElementsList.size;
    int32_t copy_steps = copy->process.processDetails->stepElementsList.size;
    test_printf("         [INFO] Original steps: %d, Copy steps: %d\n",
                (int)orig_steps, (int)copy_steps);
    TEST_ASSERT_EQ(copy_steps, orig_steps, "step count should match");

    /* Different pointers */
    TEST_ASSERT(copy != orig, "copy should be different pointer");

    /* Clean up — use centralized destroy (frees steps, checkup, details, node) */
    process_node_destroy(copy);

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
        uint8_t type = s->step.stepDetails->data.type;
        uint8_t source = s->step.stepDetails->data.source;

        /* Type should be 0=CHEMISTRY, 1=RINSE, or 2=MULTI_RINSE */
        TEST_ASSERT(type <= 2, "step type should be 0-2");

        /* Source should be 0-4 (C1, C2, C3, WB, WASTE) */
        TEST_ASSERT(source <= 4, "step source should be 0-4");

        test_printf("         [INFO] Step %d: type=%d source=%d discard=%d\n",
                    step_idx, type, source, s->step.stepDetails->data.discardAfterProc);

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
    snprintf(original_name, sizeof(original_name), "%s", proc->process.processDetails->data.processNameString);

    /* Set name to exactly MAX_PROC_NAME_LEN chars */
    char max_name[MAX_PROC_NAME_LEN + 1];
    memset(max_name, 'A', MAX_PROC_NAME_LEN);
    max_name[MAX_PROC_NAME_LEN] = '\0';

    snprintf(proc->process.processDetails->data.processNameString, sizeof(proc->process.processDetails->data.processNameString), "%s", max_name);
    test_printf("         [INFO] Set %d-char name\n", (int)strlen(max_name));

    /* Verify it was stored */
    int len = strlen(proc->process.processDetails->data.processNameString);
    TEST_ASSERT(len <= MAX_PROC_NAME_LEN, "stored name should not exceed max");
    test_printf("         [INFO] Stored name length: %d (max=%d)\n", len, MAX_PROC_NAME_LEN);

    /* Restore original name */
    snprintf(proc->process.processDetails->data.processNameString, sizeof(proc->process.processDetails->data.processNameString), "%s", original_name);

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
 * Test 10: Deep copy step — modifying copy doesn't affect original
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_step_isolation(void)
{
    TEST_BEGIN("Deep copy — step copy is isolated from original");

    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");
    stepNode *orig = proc->process.processDetails->stepElementsList.start;
    TEST_ASSERT_NOT_NULL(orig, "first step should exist");

    /* Save original values */
    uint8_t orig_mins = orig->step.stepDetails->data.timeMins;
    uint8_t orig_secs = orig->step.stepDetails->data.timeSecs;
    uint8_t orig_type = orig->step.stepDetails->data.type;
    char orig_name[MAX_PROC_NAME_LEN + 1];
    snprintf(orig_name, sizeof(orig_name), "%s", orig->step.stepDetails->data.stepNameString);

    /* Deep copy */
    stepNode *copy = deepCopyStepNode(orig);
    TEST_ASSERT_NOT_NULL(copy, "deep copy should succeed");

    /* Modify ALL data fields in the copy */
    copy->step.stepDetails->data.timeMins = 99;
    copy->step.stepDetails->data.timeSecs = 59;
    copy->step.stepDetails->data.type = MULTI_RINSE;
    copy->step.stepDetails->data.source = SOURCE_WASTE;
    copy->step.stepDetails->data.discardAfterProc = true;
    copy->step.stepDetails->data.somethingChanged = true;
    copy->step.stepDetails->data.isEditMode = true;
    snprintf(copy->step.stepDetails->data.stepNameString,
             sizeof(copy->step.stepDetails->data.stepNameString), "MODIFIED");

    /* Verify original is UNCHANGED */
    TEST_ASSERT_EQ(orig->step.stepDetails->data.timeMins, orig_mins,
                   "original timeMins must be unchanged");
    TEST_ASSERT_EQ(orig->step.stepDetails->data.timeSecs, orig_secs,
                   "original timeSecs must be unchanged");
    TEST_ASSERT_EQ(orig->step.stepDetails->data.type, orig_type,
                   "original type must be unchanged");
    TEST_ASSERT_STR_EQ(orig->step.stepDetails->data.stepNameString, orig_name,
                       "original name must be unchanged");

    test_printf("         [INFO] Modified copy (99:59), original still (%d:%02d) — isolated OK\n",
                (int)orig_mins, (int)orig_secs);

    step_node_destroy(copy);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 11: Deep copy process — modifying copy doesn't affect original
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_process_isolation(void)
{
    TEST_BEGIN("Deep copy — process copy is isolated from original");

    processNode *orig = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(orig, "first process should exist");

    /* Save original values */
    uint32_t orig_temp = orig->process.processDetails->data.temp;
    bool orig_preferred = orig->process.processDetails->data.isPreferred;
    uint8_t orig_film = orig->process.processDetails->data.filmType;
    char orig_name[MAX_PROC_NAME_LEN + 1];
    snprintf(orig_name, sizeof(orig_name), "%s",
             orig->process.processDetails->data.processNameString);

    /* Deep copy */
    processNode *copy = deepCopyProcessNode(orig);
    TEST_ASSERT_NOT_NULL(copy, "deep copy should succeed");

    /* Modify ALL data fields in the copy */
    copy->process.processDetails->data.temp = 99;
    copy->process.processDetails->data.tempTolerance = 9.9f;
    copy->process.processDetails->data.isTempControlled = !orig->process.processDetails->data.isTempControlled;
    copy->process.processDetails->data.isPreferred = !orig_preferred;
    copy->process.processDetails->data.filmType = (orig_film == 0) ? 1 : 0;
    copy->process.processDetails->data.somethingChanged = true;
    copy->process.processDetails->data.timeMins = 999;
    copy->process.processDetails->data.timeSecs = 59;
    snprintf(copy->process.processDetails->data.processNameString,
             sizeof(copy->process.processDetails->data.processNameString), "MODIFIED");

    /* Verify original is UNCHANGED */
    TEST_ASSERT_EQ(orig->process.processDetails->data.temp, orig_temp,
                   "original temp must be unchanged");
    TEST_ASSERT_EQ(orig->process.processDetails->data.isPreferred, orig_preferred,
                   "original isPreferred must be unchanged");
    TEST_ASSERT_EQ(orig->process.processDetails->data.filmType, orig_film,
                   "original filmType must be unchanged");
    TEST_ASSERT_STR_EQ(orig->process.processDetails->data.processNameString, orig_name,
                       "original name must be unchanged");

    test_printf("         [INFO] Modified copy temp=99, original temp=%d — isolated OK\n",
                (int)orig_temp);

    /* Cleanup — use centralized destroy */
    process_node_destroy(copy);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 12: Deep copy step — UI pointers are NULL after copy
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_step_ui_null(void)
{
    TEST_BEGIN("Deep copy — step UI pointers are NULL in copy");

    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");
    stepNode *orig = proc->process.processDetails->stepElementsList.start;
    TEST_ASSERT_NOT_NULL(orig, "first step should exist");

    stepNode *copy = deepCopyStepNode(orig);
    TEST_ASSERT_NOT_NULL(copy, "deep copy should succeed");

    /* The memset(0) in deepCopyStepDetail zeroes ALL LVGL pointers */
    TEST_ASSERT_NULL(copy->step.stepDetails->stepDetailParent,
                     "copied stepDetailParent should be NULL");
    TEST_ASSERT_NULL(copy->step.stepDetails->stepSaveButton,
                     "copied stepSaveButton should be NULL");
    TEST_ASSERT_NULL(copy->step.stepDetails->stepCancelButton,
                     "copied stepCancelButton should be NULL");
    TEST_ASSERT_NULL(copy->step.stepDetails->stepDetailNameTextArea,
                     "copied stepDetailNameTextArea should be NULL");
    TEST_ASSERT_NULL(copy->step.stepDetails->stepTypeDropDownList,
                     "copied stepTypeDropDownList should be NULL");
    TEST_ASSERT_NULL(copy->step.stepDetails->stepSourceDropDownList,
                     "copied stepSourceDropDownList should be NULL");

    /* But data fields ARE copied */
    TEST_ASSERT_STR_EQ(copy->step.stepDetails->data.stepNameString,
                       orig->step.stepDetails->data.stepNameString,
                       "data.stepNameString should still be copied");
    TEST_ASSERT_EQ(copy->step.stepDetails->data.timeMins,
                   orig->step.stepDetails->data.timeMins,
                   "data.timeMins should still be copied");

    test_printf("         [INFO] 6 UI pointers NULL, data fields preserved — OK\n");

    step_node_destroy(copy);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 13: Deep copy process — UI pointers are NULL after copy
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_process_ui_null(void)
{
    TEST_BEGIN("Deep copy — process UI pointers are NULL in copy");

    processNode *orig = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(orig, "first process should exist");

    processNode *copy = deepCopyProcessNode(orig);
    TEST_ASSERT_NOT_NULL(copy, "deep copy should succeed");

    /* processDetail LVGL pointers should all be NULL */
    TEST_ASSERT_NULL(copy->process.processDetails->processDetailParent,
                     "processDetailParent should be NULL");
    TEST_ASSERT_NULL(copy->process.processDetails->processStepsContainer,
                     "processStepsContainer should be NULL");
    TEST_ASSERT_NULL(copy->process.processDetails->processSaveButton,
                     "processSaveButton should be NULL");
    TEST_ASSERT_NULL(copy->process.processDetails->processRunButton,
                     "processRunButton should be NULL");
    TEST_ASSERT_NULL(copy->process.processDetails->processDetailNameTextArea,
                     "processDetailNameTextArea should be NULL");

    /* Data fields ARE preserved */
    TEST_ASSERT_STR_EQ(copy->process.processDetails->data.processNameString,
                       orig->process.processDetails->data.processNameString,
                       "data.processNameString should be copied");
    TEST_ASSERT_EQ(copy->process.processDetails->data.temp,
                   orig->process.processDetails->data.temp,
                   "data.temp should be copied");

    test_printf("         [INFO] 5 UI pointers NULL, data fields preserved — OK\n");

    /* Cleanup — use centralized destroy */
    process_node_destroy(copy);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 14: Deep copy checkup — data preserved, UI NULL
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_checkup(void)
{
    TEST_BEGIN("Deep copy — checkup data preserved, UI pointers NULL");

    /* Create a checkup with known data values */
    sCheckup original;
    memset(&original, 0, sizeof(sCheckup));
    original.data.isProcessing      = true;
    original.data.processStep       = 3;
    original.data.activeVolume_index = 1;
    original.data.tankSize          = 2;
    original.data.stopNow           = false;
    original.data.stopAfter         = true;
    original.data.isFilling         = true;
    original.data.isAlreadyPumping  = false;
    original.data.isDeveloping      = true;
    original.data.stepFillWaterStatus  = 1;
    original.data.stepReachTempStatus  = 2;
    original.data.stepCheckFilmStatus  = 0;
    original.data.currentWaterTemp  = 38.5f;
    original.data.currentChemTemp   = 37.2f;
    original.data.heaterOn          = true;
    original.data.tempTimeoutCounter = 42;
    /* Set a fake UI pointer to verify it's NOT copied */
    original.checkupParent = (lv_obj_t *)0xDEADBEEF;

    sCheckup *copy = deepCopyCheckup(&original);
    TEST_ASSERT_NOT_NULL(copy, "deepCopyCheckup should succeed");

    /* Verify ALL 16 data fields */
    TEST_ASSERT_EQ(copy->data.isProcessing, true, "isProcessing");
    TEST_ASSERT_EQ(copy->data.processStep, 3, "processStep");
    TEST_ASSERT_EQ(copy->data.activeVolume_index, 1, "activeVolume_index");
    TEST_ASSERT_EQ(copy->data.tankSize, 2, "tankSize");
    TEST_ASSERT_EQ(copy->data.stopNow, false, "stopNow");
    TEST_ASSERT_EQ(copy->data.stopAfter, true, "stopAfter");
    TEST_ASSERT_EQ(copy->data.isFilling, true, "isFilling");
    TEST_ASSERT_EQ(copy->data.isAlreadyPumping, false, "isAlreadyPumping");
    TEST_ASSERT_EQ(copy->data.isDeveloping, true, "isDeveloping");
    TEST_ASSERT_EQ(copy->data.stepFillWaterStatus, 1, "stepFillWaterStatus");
    TEST_ASSERT_EQ(copy->data.stepReachTempStatus, 2, "stepReachTempStatus");
    TEST_ASSERT_EQ(copy->data.stepCheckFilmStatus, 0, "stepCheckFilmStatus");
    TEST_ASSERT(copy->data.currentWaterTemp > 38.4f && copy->data.currentWaterTemp < 38.6f,
                "currentWaterTemp ~38.5");
    TEST_ASSERT(copy->data.currentChemTemp > 37.1f && copy->data.currentChemTemp < 37.3f,
                "currentChemTemp ~37.2");
    TEST_ASSERT_EQ(copy->data.heaterOn, true, "heaterOn");
    TEST_ASSERT_EQ(copy->data.tempTimeoutCounter, 42, "tempTimeoutCounter");

    /* UI pointers must be NULL (memset zeroed them) */
    TEST_ASSERT_NULL(copy->checkupParent, "checkupParent should be NULL (not 0xDEADBEEF)");
    TEST_ASSERT_NULL(copy->checkupContainer, "checkupContainer should be NULL");
    TEST_ASSERT_NULL(copy->processTimer, "processTimer should be NULL");
    TEST_ASSERT_NULL(copy->checkupStartButton, "checkupStartButton should be NULL");

    test_printf("         [INFO] All 16 checkup data fields preserved, UI pointers NULL — OK\n");

    free(copy);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 15: Deep copy NULL safety — all functions handle NULL
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_null_safety(void)
{
    TEST_BEGIN("Deep copy — NULL inputs return NULL safely");

    /* Every deepCopy function should return NULL on NULL input */
    sStepDetail *sd = deepCopyStepDetail(NULL);
    TEST_ASSERT_NULL(sd, "deepCopyStepDetail(NULL) should return NULL");

    singleStep dummyStep;
    TEST_ASSERT_EQ(single_step_clone(NULL, &dummyStep), false, "single_step_clone(NULL, dst) should return false");
    TEST_ASSERT_EQ(single_step_clone(&dummyStep, NULL), false, "single_step_clone(src, NULL) should return false");

    stepNode *sn = deepCopyStepNode(NULL);
    TEST_ASSERT_NULL(sn, "deepCopyStepNode(NULL) should return NULL");

    sCheckup *ck = deepCopyCheckup(NULL);
    TEST_ASSERT_NULL(ck, "deepCopyCheckup(NULL) should return NULL");

    sProcessDetail *pd = deepCopyProcessDetail(NULL);
    TEST_ASSERT_NULL(pd, "deepCopyProcessDetail(NULL) should return NULL");

    singleProcess dummyProc;
    TEST_ASSERT_EQ(single_process_clone(NULL, &dummyProc), false, "single_process_clone(NULL, dst) should return false");
    TEST_ASSERT_EQ(single_process_clone(&dummyProc, NULL), false, "single_process_clone(src, NULL) should return false");

    processNode *pn = deepCopyProcessNode(NULL);
    TEST_ASSERT_NULL(pn, "deepCopyProcessNode(NULL) should return NULL");

    /* deepCopyStepList with empty list should return empty list */
    stepList empty_list = {.start = NULL, .end = NULL, .size = 0};
    stepList copied_empty = deepCopyStepList(empty_list);
    TEST_ASSERT_EQ(copied_empty.size, 0, "empty list copy should have size 0");
    TEST_ASSERT_NULL(copied_empty.start, "empty list copy start should be NULL");
    TEST_ASSERT_NULL(copied_empty.end, "empty list copy end should be NULL");

    test_printf("         [INFO] All 8 NULL-safety checks passed\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 16: Deep copy step list — doubly-linked integrity
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_step_list_integrity(void)
{
    TEST_BEGIN("Deep copy — step list maintains doubly-linked integrity");

    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");

    stepList orig_list = proc->process.processDetails->stepElementsList;
    TEST_ASSERT(orig_list.size > 0, "need at least one step");

    /* Deep copy the step list */
    stepList copy_list = deepCopyStepList(orig_list);
    TEST_ASSERT_EQ(copy_list.size, orig_list.size, "copied size should match");

    /* Walk forward */
    int fwd = 0;
    stepNode *s = copy_list.start;
    stepNode *last = NULL;
    while (s != NULL) {
        /* Verify prev pointer is correct */
        TEST_ASSERT(s->prev == last, "prev pointer should point to previous node");
        last = s;
        fwd++;
        s = s->next;
    }
    TEST_ASSERT_EQ(fwd, (int)copy_list.size, "forward walk should match size");
    TEST_ASSERT(last == copy_list.end, "last node should be list end");

    /* Walk backward */
    int bwd = 0;
    s = copy_list.end;
    while (s != NULL) {
        bwd++;
        s = s->prev;
    }
    TEST_ASSERT_EQ(bwd, (int)copy_list.size, "backward walk should match size");

    /* Verify data is preserved for each node */
    stepNode *orig_node = orig_list.start;
    stepNode *copy_node = copy_list.start;
    int idx = 0;
    while (orig_node != NULL && copy_node != NULL) {
        TEST_ASSERT(copy_node != orig_node, "copy nodes must be different pointers");
        TEST_ASSERT_STR_EQ(copy_node->step.stepDetails->data.stepNameString,
                           orig_node->step.stepDetails->data.stepNameString,
                           "step names should match");
        TEST_ASSERT_EQ(copy_node->step.stepDetails->data.timeMins,
                       orig_node->step.stepDetails->data.timeMins,
                       "timeMins should match");
        idx++;
        orig_node = orig_node->next;
        copy_node = copy_node->next;
    }

    test_printf("         [INFO] Copied list: fwd=%d, bwd=%d, %d nodes data-verified — OK\n",
                fwd, bwd, idx);

    /* Cleanup */
    s = copy_list.start;
    while (s != NULL) {
        stepNode *next = s->next;
        step_node_destroy(s);
        s = next;
    }

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 17: sStepData sub-struct — all fields accessible via .data
 * ═══════════════════════════════════════════════ */
static void test_step_data_substruct(void)
{
    TEST_BEGIN("Sub-struct — sStepData field access and assignment");

    /* Create two sStepData structs and verify assignment copies all fields */
    sStepData a;
    memset(&a, 0, sizeof(sStepData));
    snprintf(a.stepNameString, sizeof(a.stepNameString), "TestStep");
    a.somethingChanged = true;
    a.isEditMode = true;
    a.timeMins = 12;
    a.timeSecs = 45;
    a.type = RINSE;
    a.source = SOURCE_C3;
    a.discardAfterProc = true;

    /* Simple struct assignment — this is what deepCopyStepDetail uses */
    sStepData b = a;

    TEST_ASSERT_STR_EQ(b.stepNameString, "TestStep", "name copied via assignment");
    TEST_ASSERT_EQ(b.somethingChanged, true, "somethingChanged copied");
    TEST_ASSERT_EQ(b.isEditMode, true, "isEditMode copied");
    TEST_ASSERT_EQ(b.timeMins, 12, "timeMins copied");
    TEST_ASSERT_EQ(b.timeSecs, 45, "timeSecs copied");
    TEST_ASSERT_EQ(b.type, RINSE, "type copied");
    TEST_ASSERT_EQ(b.source, SOURCE_C3, "source copied");
    TEST_ASSERT_EQ(b.discardAfterProc, true, "discardAfterProc copied");

    /* Modify b, verify a is unchanged */
    b.timeMins = 0;
    b.type = CHEMISTRY;
    snprintf(b.stepNameString, sizeof(b.stepNameString), "Changed");
    TEST_ASSERT_EQ(a.timeMins, 12, "original timeMins unchanged after modifying copy");
    TEST_ASSERT_EQ(a.type, RINSE, "original type unchanged");
    TEST_ASSERT_STR_EQ(a.stepNameString, "TestStep", "original name unchanged");

    test_printf("         [INFO] All 8 sStepData fields: assign + isolate verified\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 18: sProcessData sub-struct — all fields accessible via .data
 * ═══════════════════════════════════════════════ */
static void test_process_data_substruct(void)
{
    TEST_BEGIN("Sub-struct — sProcessData field access and assignment");

    sProcessData a;
    memset(&a, 0, sizeof(sProcessData));
    snprintf(a.processNameString, sizeof(a.processNameString), "TestProc");
    a.temp = 38;
    a.tempTolerance = 0.3f;
    a.isTempControlled = true;
    a.isPreferred = true;
    a.somethingChanged = false;
    a.filmType = BLACK_AND_WHITE_FILM;
    a.timeMins = 20;
    a.timeSecs = 30;

    sProcessData b = a;

    TEST_ASSERT_STR_EQ(b.processNameString, "TestProc", "name copied");
    TEST_ASSERT_EQ(b.temp, 38, "temp copied");
    TEST_ASSERT(b.tempTolerance > 0.29f && b.tempTolerance < 0.31f, "tempTolerance copied");
    TEST_ASSERT_EQ(b.isTempControlled, true, "isTempControlled copied");
    TEST_ASSERT_EQ(b.isPreferred, true, "isPreferred copied");
    TEST_ASSERT_EQ(b.somethingChanged, false, "somethingChanged copied");
    TEST_ASSERT_EQ(b.filmType, 0, "filmType copied");
    TEST_ASSERT_EQ(b.timeMins, 20, "timeMins copied");
    TEST_ASSERT_EQ(b.timeSecs, 30, "timeSecs copied");

    /* Modify b, verify a unchanged */
    b.temp = 0;
    b.isPreferred = false;
    TEST_ASSERT_EQ(a.temp, 38, "original temp unchanged");
    TEST_ASSERT_EQ(a.isPreferred, true, "original isPreferred unchanged");

    test_printf("         [INFO] All 9 sProcessData fields: assign + isolate verified\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 19: sCheckupData sub-struct — all fields accessible via .data
 * ═══════════════════════════════════════════════ */
static void test_checkup_data_substruct(void)
{
    TEST_BEGIN("Sub-struct — sCheckupData field access and assignment");

    sCheckupData a;
    memset(&a, 0, sizeof(sCheckupData));
    a.isProcessing      = true;
    a.processStep       = 5;
    a.activeVolume_index = 1;
    a.tankSize          = 3;
    a.stopNow           = false;
    a.stopAfter         = true;
    a.isFilling         = true;
    a.isAlreadyPumping  = false;
    a.isDeveloping      = true;
    a.stepFillWaterStatus  = 2;
    a.stepReachTempStatus  = 1;
    a.stepCheckFilmStatus  = 0;
    a.currentWaterTemp  = 25.5f;
    a.currentChemTemp   = 24.0f;
    a.heaterOn          = true;
    a.tempTimeoutCounter = 100;

    sCheckupData b = a;

    TEST_ASSERT_EQ(b.isProcessing, true, "isProcessing");
    TEST_ASSERT_EQ(b.processStep, 5, "processStep");
    TEST_ASSERT_EQ(b.activeVolume_index, 1, "activeVolume_index");
    TEST_ASSERT_EQ(b.tankSize, 3, "tankSize");
    TEST_ASSERT_EQ(b.stopNow, false, "stopNow");
    TEST_ASSERT_EQ(b.stopAfter, true, "stopAfter");
    TEST_ASSERT_EQ(b.isFilling, true, "isFilling");
    TEST_ASSERT_EQ(b.isAlreadyPumping, false, "isAlreadyPumping");
    TEST_ASSERT_EQ(b.isDeveloping, true, "isDeveloping");
    TEST_ASSERT_EQ(b.stepFillWaterStatus, 2, "stepFillWaterStatus");
    TEST_ASSERT_EQ(b.stepReachTempStatus, 1, "stepReachTempStatus");
    TEST_ASSERT_EQ(b.stepCheckFilmStatus, 0, "stepCheckFilmStatus");
    TEST_ASSERT(b.currentWaterTemp > 25.4f && b.currentWaterTemp < 25.6f, "currentWaterTemp");
    TEST_ASSERT(b.currentChemTemp > 23.9f && b.currentChemTemp < 24.1f, "currentChemTemp");
    TEST_ASSERT_EQ(b.heaterOn, true, "heaterOn");
    TEST_ASSERT_EQ(b.tempTimeoutCounter, 100, "tempTimeoutCounter");

    /* Modify b, verify a unchanged */
    b.processStep = 0;
    b.currentWaterTemp = 0.0f;
    b.heaterOn = false;
    TEST_ASSERT_EQ(a.processStep, 5, "original processStep unchanged");
    TEST_ASSERT(a.currentWaterTemp > 25.4f, "original currentWaterTemp unchanged");
    TEST_ASSERT_EQ(a.heaterOn, true, "original heaterOn unchanged");

    test_printf("         [INFO] All 16 sCheckupData fields: assign + isolate verified\n");

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

    /* New tests for Points 4/5/16/17 refactoring */
    test_deep_copy_step_isolation();
    test_deep_copy_process_isolation();
    test_deep_copy_step_ui_null();
    test_deep_copy_process_ui_null();
    test_deep_copy_checkup();
    test_deep_copy_null_safety();
    test_deep_copy_step_list_integrity();
    test_step_data_substruct();
    test_process_data_substruct();
    test_checkup_data_substruct();
}
