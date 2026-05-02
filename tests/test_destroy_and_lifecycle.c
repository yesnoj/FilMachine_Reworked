/**
 * test_destroy_and_lifecycle.c — Destroy Functions, Lifecycle & Clone Tests
 *
 * Tests all destroy/cleanup functions, safeTimerDelete, clone-in-place,
 * allocateAndInitializeNode, parentProcess back-reference, and
 * checkup.currentStep field introduced during the review refactoring.
 */

#include "test_runner.h"
#include "lvgl.h"
#include <string.h>


/* ═══════════════════════════════════════════════
 * Test 1: All destroy functions accept NULL safely
 * ═══════════════════════════════════════════════ */
static void test_destroy_null_safety(void)
{
    TEST_BEGIN("Destroy — all destroy functions accept NULL without crash");

    /* These must not crash */
    step_detail_destroy(NULL);
    checkup_destroy(NULL);
    process_detail_destroy(NULL);
    step_node_destroy(NULL);
    process_node_destroy(NULL);

    test_printf("         [INFO] 5 destroy(NULL) calls completed without crash\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: step_detail_destroy — allocate then destroy
 * ═══════════════════════════════════════════════ */
static void test_step_detail_destroy_basic(void)
{
    TEST_BEGIN("Destroy — step_detail_destroy frees sStepDetail");

    sStepDetail *sd = (sStepDetail *)malloc(sizeof(sStepDetail));
    TEST_ASSERT_NOT_NULL(sd, "malloc sStepDetail should succeed");
    memset(sd, 0, sizeof(sStepDetail));

    /* Initialize the style like the real code does */
    lv_style_init(&sd->style_mBoxStepPopupTitleLine);

    /* Set some data to make it non-trivial */
    snprintf(sd->data.stepNameString, sizeof(sd->data.stepNameString), "DestroyTest");
    sd->data.timeMins = 5;
    sd->data.timeSecs = 30;

    /* Destroy — should reset style and free */
    step_detail_destroy(sd);

    test_printf("         [INFO] step_detail_destroy completed without crash\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: checkup_destroy — allocate then destroy
 * ═══════════════════════════════════════════════ */
static void test_checkup_destroy_basic(void)
{
    TEST_BEGIN("Destroy — checkup_destroy frees sCheckup");

    sCheckup *ckup = (sCheckup *)malloc(sizeof(sCheckup));
    TEST_ASSERT_NOT_NULL(ckup, "malloc sCheckup should succeed");
    memset(ckup, 0, sizeof(sCheckup));

    /* Initialize the style */
    lv_style_init(&ckup->textAreaStyleCheckup);

    /* Timers are NULL (memset 0) → safeTimerDelete should handle it */
    ckup->data.processStep = 3;
    ckup->data.isProcessing = true;

    /* Destroy — should call safeTimerDelete on all 3 timers, reset style, free */
    checkup_destroy(ckup);

    test_printf("         [INFO] checkup_destroy completed without crash\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: step_node_destroy — allocate via API then destroy
 * ═══════════════════════════════════════════════ */
static void test_step_node_destroy_full(void)
{
    TEST_BEGIN("Destroy — step_node_destroy cleans step + stepDetails");

    stepNode *sn = (stepNode *)allocateAndInitializeNode(STEP_NODE);
    TEST_ASSERT_NOT_NULL(sn, "allocateAndInitializeNode(STEP_NODE) should succeed");
    TEST_ASSERT_NOT_NULL(sn->step.stepDetails, "stepDetails should be allocated");

    /* Set data */
    snprintf(sn->step.stepDetails->data.stepNameString,
             sizeof(sn->step.stepDetails->data.stepNameString), "NodeDestroyTest");
    sn->step.stepDetails->data.timeMins = 10;

    /* Destroy — should free stepDetails then free node */
    step_node_destroy(sn);

    test_printf("         [INFO] step_node_destroy completed without crash\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: process_node_destroy — allocate via API then destroy
 * ═══════════════════════════════════════════════ */
static void test_process_node_destroy_full(void)
{
    TEST_BEGIN("Destroy — process_node_destroy cascading cleanup");

    processNode *pn = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    TEST_ASSERT_NOT_NULL(pn, "allocateAndInitializeNode(PROCESS_NODE) should succeed");
    TEST_ASSERT_NOT_NULL(pn->process.processDetails, "processDetails allocated");
    TEST_ASSERT_NOT_NULL(pn->process.processDetails->checkup, "checkup allocated");

    /* Set data */
    snprintf(pn->process.processDetails->data.processNameString,
             sizeof(pn->process.processDetails->data.processNameString), "ProcDestroyTest");
    pn->process.processDetails->data.temp = 38;

    /* Destroy — cascades: process_detail_destroy → checkup_destroy → free */
    process_node_destroy(pn);

    test_printf("         [INFO] process_node_destroy cascading cleanup completed\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 6: process_detail_destroy with steps — cascading
 * ═══════════════════════════════════════════════ */
static void test_process_detail_destroy_with_steps(void)
{
    TEST_BEGIN("Destroy — process_detail_destroy frees all steps in list");

    processNode *pn = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    TEST_ASSERT_NOT_NULL(pn, "process node allocated");

    sProcessDetail *pd = pn->process.processDetails;

    /* Add 3 steps to the step list */
    for (int i = 0; i < 3; i++) {
        stepNode *sn = (stepNode *)allocateAndInitializeNode(STEP_NODE);
        TEST_ASSERT_NOT_NULL(sn, "step node allocated");
        snprintf(sn->step.stepDetails->data.stepNameString,
                 sizeof(sn->step.stepDetails->data.stepNameString), "Step%d", i);
        sn->step.stepDetails->data.timeMins = (uint8_t)(i + 1);
        sn->prev = NULL;
        sn->next = NULL;

        if (pd->stepElementsList.start == NULL) {
            pd->stepElementsList.start = sn;
        } else {
            pd->stepElementsList.end->next = sn;
            sn->prev = pd->stepElementsList.end;
        }
        pd->stepElementsList.end = sn;
        pd->stepElementsList.size++;
    }

    TEST_ASSERT_EQ((int)pd->stepElementsList.size, 3, "should have 3 steps");

    /* Destroy the entire process node — steps, checkup, details, node */
    process_node_destroy(pn);

    test_printf("         [INFO] process_node_destroy with 3 steps completed\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 7: safeTimerDelete — NULL safety
 * ═══════════════════════════════════════════════ */
static void test_safe_timer_delete_null(void)
{
    TEST_BEGIN("Lifecycle — safeTimerDelete handles NULL safely");

    /* Pass NULL pointer-to-pointer */
    safeTimerDelete(NULL);

    /* Pass pointer to NULL timer */
    lv_timer_t *timer = NULL;
    safeTimerDelete(&timer);
    TEST_ASSERT_NULL(timer, "timer should remain NULL after safeTimerDelete");

    test_printf("         [INFO] safeTimerDelete(NULL) and safeTimerDelete(&NULL) safe\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 8: allocateAndInitializeNode — STEP_NODE zero-init
 * ═══════════════════════════════════════════════ */
static void test_allocate_step_node_zeroed(void)
{
    TEST_BEGIN("Lifecycle — allocateAndInitializeNode(STEP_NODE) zeroes all fields");

    stepNode *sn = (stepNode *)allocateAndInitializeNode(STEP_NODE);
    TEST_ASSERT_NOT_NULL(sn, "allocation should succeed");

    /* Node pointers should be NULL */
    TEST_ASSERT_NULL(sn->next, "next should be NULL");
    TEST_ASSERT_NULL(sn->prev, "prev should be NULL");

    /* stepDetails should be allocated but data zeroed */
    TEST_ASSERT_NOT_NULL(sn->step.stepDetails, "stepDetails should be allocated");
    TEST_ASSERT_EQ(sn->step.stepDetails->data.timeMins, 0, "timeMins should be 0");
    TEST_ASSERT_EQ(sn->step.stepDetails->data.timeSecs, 0, "timeSecs should be 0");
    TEST_ASSERT_EQ(sn->step.stepDetails->data.type, 0, "type should be 0");
    TEST_ASSERT_EQ(sn->step.stepDetails->data.source, 0, "source should be 0");
    TEST_ASSERT_EQ(sn->step.stepDetails->data.somethingChanged, false, "somethingChanged false");
    TEST_ASSERT_EQ(sn->step.stepDetails->data.isEditMode, false, "isEditMode false");
    TEST_ASSERT_EQ((int)strlen(sn->step.stepDetails->data.stepNameString), 0, "name should be empty");

    /* UI pointers should be NULL */
    TEST_ASSERT_NULL(sn->step.stepDetails->stepDetailParent, "stepDetailParent NULL");
    TEST_ASSERT_NULL(sn->step.stepDetails->stepSaveButton, "stepSaveButton NULL");

    test_printf("         [INFO] STEP_NODE: all fields zeroed correctly\n");

    step_node_destroy(sn);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 9: allocateAndInitializeNode — PROCESS_NODE init
 * ═══════════════════════════════════════════════ */
static void test_allocate_process_node_init(void)
{
    TEST_BEGIN("Lifecycle — allocateAndInitializeNode(PROCESS_NODE) full init");

    processNode *pn = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    TEST_ASSERT_NOT_NULL(pn, "allocation should succeed");

    /* Node pointers */
    TEST_ASSERT_NULL(pn->next, "next should be NULL");
    TEST_ASSERT_NULL(pn->prev, "prev should be NULL");

    /* processDetails allocated and zeroed */
    sProcessDetail *pd = pn->process.processDetails;
    TEST_ASSERT_NOT_NULL(pd, "processDetails should be allocated");
    TEST_ASSERT_EQ(pd->data.temp, 0, "temp should be 0");
    TEST_ASSERT_EQ(pd->data.filmType, 0, "filmType should be 0");
    TEST_ASSERT_EQ(pd->data.isPreferred, 0, "isPreferred should be 0");
    TEST_ASSERT_EQ((int)pd->stepElementsList.size, 0, "step list should be empty");
    TEST_ASSERT_NULL(pd->stepElementsList.start, "step list start NULL");
    TEST_ASSERT_NULL(pd->processDetailParent, "processDetailParent NULL");

    /* checkup allocated with default tankSize */
    sCheckup *ckup = pd->checkup;
    TEST_ASSERT_NOT_NULL(ckup, "checkup should be allocated");
    TEST_ASSERT_EQ(ckup->data.tankSize, 2, "default tankSize should be 2 (Medium)");
    TEST_ASSERT_EQ(ckup->data.processStep, 0, "processStep should be 0");
    TEST_ASSERT_EQ(ckup->data.isProcessing, false, "isProcessing should be false");
    TEST_ASSERT_NULL(ckup->checkupParent, "checkupParent NULL");
    TEST_ASSERT_NULL(ckup->processTimer, "processTimer NULL");

    test_printf("         [INFO] PROCESS_NODE: details, checkup allocated, defaults correct\n");

    process_node_destroy(pn);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 10: allocateAndInitializeNode — invalid type returns NULL
 * ═══════════════════════════════════════════════ */
static void test_allocate_invalid_type(void)
{
    TEST_BEGIN("Lifecycle — allocateAndInitializeNode(invalid) returns NULL");

    void *result = allocateAndInitializeNode(99);
    TEST_ASSERT_NULL(result, "invalid type should return NULL");

    test_printf("         [INFO] allocateAndInitializeNode(99) = NULL — OK\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 11: single_step_clone — actual data copy
 * ═══════════════════════════════════════════════ */
static void test_single_step_clone_data(void)
{
    TEST_BEGIN("Clone — single_step_clone copies data into target");

    /* Get a real step from the test data */
    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");
    stepNode *orig_node = proc->process.processDetails->stepElementsList.start;
    TEST_ASSERT_NOT_NULL(orig_node, "first step should exist");

    singleStep *src = &orig_node->step;
    singleStep dst;
    memset(&dst, 0xFF, sizeof(singleStep));  /* Fill with garbage */

    bool ok = single_step_clone(src, &dst);
    TEST_ASSERT_EQ(ok, true, "clone should succeed");

    /* Verify data was copied */
    TEST_ASSERT_NOT_NULL(dst.stepDetails, "dst.stepDetails should be allocated");
    TEST_ASSERT_STR_EQ(dst.stepDetails->data.stepNameString,
                       src->stepDetails->data.stepNameString,
                       "step name should match");
    TEST_ASSERT_EQ(dst.stepDetails->data.timeMins,
                   src->stepDetails->data.timeMins,
                   "timeMins should match");
    TEST_ASSERT_EQ(dst.stepDetails->data.timeSecs,
                   src->stepDetails->data.timeSecs,
                   "timeSecs should match");
    TEST_ASSERT_EQ(dst.stepDetails->data.type,
                   src->stepDetails->data.type,
                   "type should match");
    TEST_ASSERT_EQ(dst.stepDetails->data.source,
                   src->stepDetails->data.source,
                   "source should match");

    /* Verify dst.stepDetails is a DIFFERENT pointer than src */
    TEST_ASSERT(dst.stepDetails != src->stepDetails,
                "dst stepDetails must be a different allocation");

    test_printf("         [INFO] single_step_clone: \"%s\" cloned OK\n",
                dst.stepDetails->data.stepNameString);

    /* Clean up the cloned stepDetails */
    step_detail_destroy(dst.stepDetails);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 12: single_process_clone — actual data copy
 * ═══════════════════════════════════════════════ */
static void test_single_process_clone_data(void)
{
    TEST_BEGIN("Clone — single_process_clone copies data into target");

    processNode *orig = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(orig, "first process should exist");

    singleProcess *src = &orig->process;
    singleProcess dst;
    memset(&dst, 0xFF, sizeof(singleProcess));  /* Fill with garbage */

    bool ok = single_process_clone(src, &dst);
    TEST_ASSERT_EQ(ok, true, "clone should succeed");

    /* Verify data was copied */
    TEST_ASSERT_NOT_NULL(dst.processDetails, "dst.processDetails should be allocated");
    TEST_ASSERT_STR_EQ(dst.processDetails->data.processNameString,
                       src->processDetails->data.processNameString,
                       "process name should match");
    TEST_ASSERT_EQ(dst.processDetails->data.temp,
                   src->processDetails->data.temp,
                   "temp should match");
    TEST_ASSERT_EQ(dst.processDetails->data.filmType,
                   src->processDetails->data.filmType,
                   "filmType should match");
    TEST_ASSERT_EQ(dst.processDetails->data.isPreferred,
                   src->processDetails->data.isPreferred,
                   "isPreferred should match");

    /* Step count should match */
    TEST_ASSERT_EQ((int)dst.processDetails->stepElementsList.size,
                   (int)src->processDetails->stepElementsList.size,
                   "step count should match");

    /* Different pointer */
    TEST_ASSERT(dst.processDetails != src->processDetails,
                "dst processDetails must be a different allocation");

    test_printf("         [INFO] single_process_clone: \"%s\" (%d steps) cloned OK\n",
                dst.processDetails->data.processNameString,
                (int)dst.processDetails->stepElementsList.size);

    /* Clean up */
    process_detail_destroy(dst.processDetails);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 13: single_step_clone — isolation from original
 * ═══════════════════════════════════════════════ */
static void test_single_step_clone_isolation(void)
{
    TEST_BEGIN("Clone — single_step_clone target is isolated from source");

    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");
    stepNode *orig_node = proc->process.processDetails->stepElementsList.start;
    TEST_ASSERT_NOT_NULL(orig_node, "first step should exist");

    /* Save original values */
    uint8_t orig_mins = orig_node->step.stepDetails->data.timeMins;
    char orig_name[MAX_PROC_NAME_LEN + 1];
    snprintf(orig_name, sizeof(orig_name), "%s",
             orig_node->step.stepDetails->data.stepNameString);

    /* Clone */
    singleStep dst;
    bool ok = single_step_clone(&orig_node->step, &dst);
    TEST_ASSERT_EQ(ok, true, "clone should succeed");

    /* Modify clone */
    dst.stepDetails->data.timeMins = 99;
    snprintf(dst.stepDetails->data.stepNameString,
             sizeof(dst.stepDetails->data.stepNameString), "MODIFIED_CLONE");

    /* Verify original is unchanged */
    TEST_ASSERT_EQ(orig_node->step.stepDetails->data.timeMins, orig_mins,
                   "original timeMins unchanged after modifying clone");
    TEST_ASSERT_STR_EQ(orig_node->step.stepDetails->data.stepNameString, orig_name,
                       "original name unchanged after modifying clone");

    test_printf("         [INFO] Clone modified to 99min, original still %dmin — isolated\n",
                (int)orig_mins);

    step_detail_destroy(dst.stepDetails);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 14: parentProcess back-reference — field accessible
 * ═══════════════════════════════════════════════ */
static void test_parent_process_backref(void)
{
    TEST_BEGIN("Struct — parentProcess back-reference in sStepDetail");

    /* Allocate a fresh step and process */
    processNode *pn = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    TEST_ASSERT_NOT_NULL(pn, "process node allocated");

    stepNode *sn = (stepNode *)allocateAndInitializeNode(STEP_NODE);
    TEST_ASSERT_NOT_NULL(sn, "step node allocated");

    /* parentProcess should be NULL initially (memset 0) */
    TEST_ASSERT_NULL(sn->step.stepDetails->parentProcess,
                     "parentProcess should be NULL after allocation");

    /* Set back-reference */
    sn->step.stepDetails->parentProcess = pn;
    TEST_ASSERT(sn->step.stepDetails->parentProcess == pn,
                "parentProcess should point to the process node");

    /* Verify we can traverse back to the process data */
    TEST_ASSERT_STR_EQ(
        sn->step.stepDetails->parentProcess->process.processDetails->data.processNameString,
        pn->process.processDetails->data.processNameString,
        "traversing parentProcess should reach process data");

    test_printf("         [INFO] parentProcess back-reference set and traversed OK\n");

    /* Clean up */
    step_node_destroy(sn);
    process_node_destroy(pn);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 15: checkup.currentStep — field accessible
 * ═══════════════════════════════════════════════ */
static void test_checkup_current_step(void)
{
    TEST_BEGIN("Struct — checkup.currentStep field read/write");

    processNode *pn = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    TEST_ASSERT_NOT_NULL(pn, "process node allocated");

    sCheckup *ckup = pn->process.processDetails->checkup;
    TEST_ASSERT_NOT_NULL(ckup, "checkup allocated");

    /* Should be NULL initially */
    TEST_ASSERT_NULL(ckup->currentStep, "currentStep should be NULL initially");

    /* Create a step and set it as currentStep */
    stepNode *sn = (stepNode *)allocateAndInitializeNode(STEP_NODE);
    TEST_ASSERT_NOT_NULL(sn, "step allocated");
    snprintf(sn->step.stepDetails->data.stepNameString,
             sizeof(sn->step.stepDetails->data.stepNameString), "CurrentStepTest");

    ckup->currentStep = sn;
    TEST_ASSERT(ckup->currentStep == sn, "currentStep should point to the step");
    TEST_ASSERT_STR_EQ(ckup->currentStep->step.stepDetails->data.stepNameString,
                       "CurrentStepTest",
                       "should access step data via currentStep");

    test_printf("         [INFO] checkup.currentStep set and read OK\n");

    /* Clean up */
    step_node_destroy(sn);
    process_node_destroy(pn);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 16: emptyList process list — create and empty
 * ═══════════════════════════════════════════════ */
static void test_empty_process_list(void)
{
    TEST_BEGIN("Lifecycle — emptyList clears a process list");

    /* Create a temporary process list with 2 processes */
    processList temp_list = { .start = NULL, .end = NULL, .size = 0 };

    processNode *p1 = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    processNode *p2 = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    TEST_ASSERT_NOT_NULL(p1, "p1 allocated");
    TEST_ASSERT_NOT_NULL(p2, "p2 allocated");

    snprintf(p1->process.processDetails->data.processNameString,
             sizeof(p1->process.processDetails->data.processNameString), "EmptyTest1");
    snprintf(p2->process.processDetails->data.processNameString,
             sizeof(p2->process.processDetails->data.processNameString), "EmptyTest2");

    /* Link: p1 → p2 */
    p1->prev = NULL;
    p1->next = p2;
    p2->prev = p1;
    p2->next = NULL;
    temp_list.start = p1;
    temp_list.end   = p2;
    temp_list.size  = 2;

    TEST_ASSERT_EQ((int)temp_list.size, 2, "temp list should have 2 processes");

    /* Empty it */
    emptyList(&temp_list, PROCESS_NODE);

    TEST_ASSERT_EQ((int)temp_list.size, 0, "size should be 0 after empty");
    TEST_ASSERT_NULL(temp_list.start, "start should be NULL");
    TEST_ASSERT_NULL(temp_list.end, "end should be NULL");

    test_printf("         [INFO] emptyList(processList) cleared 2 processes OK\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 17: Double destroy via step_node_destroy(NULL) after destroy
 * ═══════════════════════════════════════════════ */
static void test_destroy_then_null_safe(void)
{
    TEST_BEGIN("Destroy — destroy + NULL re-call pattern is safe");

    stepNode *sn = (stepNode *)allocateAndInitializeNode(STEP_NODE);
    TEST_ASSERT_NOT_NULL(sn, "step allocated");

    /* Destroy it */
    step_node_destroy(sn);

    /* Calling destroy on NULL is the safe pattern (not on the freed pointer!) */
    step_node_destroy(NULL);
    process_node_destroy(NULL);
    step_detail_destroy(NULL);
    checkup_destroy(NULL);
    process_detail_destroy(NULL);

    test_printf("         [INFO] destroy(freed) + destroy(NULL) pattern safe\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 18: isNodeInList — finds node in list, NULL safety
 * ═══════════════════════════════════════════════ */
static void test_is_node_in_list(void)
{
    TEST_BEGIN("Lifecycle — isNodeInList finds node and handles NULL");

    processList *list = &gui.page.processes.processElementsList;
    processNode *first = list->start;
    TEST_ASSERT_NOT_NULL(first, "first process should exist");

    /* First node should be found */
    void *found = isNodeInList((void *)list, first, PROCESS_NODE);
    TEST_ASSERT(found == first, "first node should be found in list");

    /* A freshly allocated node should NOT be in the list */
    processNode *orphan = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    TEST_ASSERT_NOT_NULL(orphan, "orphan allocated");

    void *not_found = isNodeInList((void *)list, orphan, PROCESS_NODE);
    TEST_ASSERT_NULL(not_found, "orphan should NOT be found in list");

    /* NULL inputs should return NULL */
    TEST_ASSERT_NULL(isNodeInList(NULL, first, PROCESS_NODE),
                     "NULL list should return NULL");
    TEST_ASSERT_NULL(isNodeInList((void *)list, NULL, PROCESS_NODE),
                     "NULL node should return NULL");

    test_printf("         [INFO] isNodeInList: found, not_found, NULL checks OK\n");

    process_node_destroy(orphan);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 19: Deep copy preserves parentProcess as NULL
 * ═══════════════════════════════════════════════ */
static void test_deep_copy_clears_parent_process(void)
{
    TEST_BEGIN("Clone — deep copy step zeroes parentProcess pointer");

    processNode *proc = gui.page.processes.processElementsList.start;
    TEST_ASSERT_NOT_NULL(proc, "first process should exist");
    stepNode *orig = proc->process.processDetails->stepElementsList.start;
    TEST_ASSERT_NOT_NULL(orig, "first step should exist");

    /* Set parentProcess on original (simulating real usage) */
    orig->step.stepDetails->parentProcess = proc;

    /* Deep copy the step */
    stepNode *copy = deepCopyStepNode(orig);
    TEST_ASSERT_NOT_NULL(copy, "deep copy should succeed");

    /* parentProcess in copy should be NULL (memset zeroed during deep copy) */
    TEST_ASSERT_NULL(copy->step.stepDetails->parentProcess,
                     "copied parentProcess should be NULL (not shared)");

    test_printf("         [INFO] parentProcess in original=%p, in copy=NULL — OK\n",
                (void *)orig->step.stepDetails->parentProcess);

    /* Clean original back-reference (was only set for this test) */
    orig->step.stepDetails->parentProcess = NULL;

    step_node_destroy(copy);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 20: Checkup default values after allocation
 * ═══════════════════════════════════════════════ */
static void test_checkup_defaults_after_alloc(void)
{
    TEST_BEGIN("Lifecycle — checkup has correct defaults after PROCESS_NODE alloc");

    processNode *pn = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
    TEST_ASSERT_NOT_NULL(pn, "process node allocated");

    sCheckup *ckup = pn->process.processDetails->checkup;
    TEST_ASSERT_NOT_NULL(ckup, "checkup allocated");

    /* Verify all data fields are zero/false except tankSize */
    TEST_ASSERT_EQ(ckup->data.isProcessing, false, "isProcessing default false");
    TEST_ASSERT_EQ(ckup->data.processStep, 0, "processStep default 0");
    TEST_ASSERT_EQ(ckup->data.activeVolume_index, 0, "activeVolume_index default 0");
    TEST_ASSERT_EQ(ckup->data.tankSize, 2, "tankSize default 2 (Medium)");
    TEST_ASSERT_EQ(ckup->data.stopNow, false, "stopNow default false");
    TEST_ASSERT_EQ(ckup->data.stopAfter, false, "stopAfter default false");
    TEST_ASSERT_EQ(ckup->data.isFilling, false, "isFilling default false");
    TEST_ASSERT_EQ(ckup->data.isDeveloping, false, "isDeveloping default false");
    TEST_ASSERT_EQ(ckup->data.heaterOn, false, "heaterOn default false");
    TEST_ASSERT_EQ(ckup->data.tempTimeoutCounter, 0, "tempTimeoutCounter default 0");

    /* Timer pointers should be NULL */
    TEST_ASSERT_NULL(ckup->processTimer, "processTimer NULL");
    TEST_ASSERT_NULL(ckup->pumpTimer, "pumpTimer NULL");
    TEST_ASSERT_NULL(ckup->tempTimer, "tempTimer NULL");

    /* currentStep should be NULL */
    TEST_ASSERT_NULL(ckup->currentStep, "currentStep default NULL");

    test_printf("         [INFO] All checkup defaults verified (14 checks)\n");

    process_node_destroy(pn);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_destroy_and_lifecycle(void)
{
    TEST_SUITE("Destroy & Lifecycle");

    test_destroy_null_safety();
    test_step_detail_destroy_basic();
    test_checkup_destroy_basic();
    test_step_node_destroy_full();
    test_process_node_destroy_full();
    test_process_detail_destroy_with_steps();
    test_safe_timer_delete_null();
    test_allocate_step_node_zeroed();
    test_allocate_process_node_init();
    test_allocate_invalid_type();
    test_single_step_clone_data();
    test_single_process_clone_data();
    test_single_step_clone_isolation();
    test_parent_process_backref();
    test_checkup_current_step();
    test_empty_process_list();
    test_destroy_then_null_safe();
    test_is_node_in_list();
    test_deep_copy_clears_parent_process();
    test_checkup_defaults_after_alloc();
}
