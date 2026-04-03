/**
 * test_persistence.c — Config File Persistence Tests
 *
 * Tests the write/read cycle of the configuration file (FatFS stub → sd/FilMachine.cfg).
 * Verifies that processes, steps, and settings survive a save/load round-trip.
 *
 * IMPORTANT: These tests use emptyList() instead of deleteProcessElement() to
 * clear the process list.  deleteProcessElement() calls lv_obj_delete() on the
 * processElement LVGL object, which is NULL for nodes created by readConfigFile
 * or test_generate_data (they have no UI).  lv_obj_delete(NULL) dereferences a
 * NULL pointer (LVGL 9 has no NULL guard), causing a segfault.
 *
 * emptyList() frees nodes via process_node_destroy() which only touches data,
 * styles, and timers — never LVGL widget objects.
 */

#include "test_runner.h"
#include "lvgl.h"
#include <unistd.h>

/* Test-specific config filename to avoid corrupting the main one */
#define TEST_CONFIG_FILE "/FilMachine_Test.cfg"


/* ═══════════════════════════════════════════════
 * Helper: safely clear the process list and reload from a config file.
 *
 * readConfigFile() resets list pointers (start=NULL, end=NULL, size=0) BEFORE
 * reading, which leaks any existing nodes.  We call emptyList() first to
 * properly free the old nodes and avoid heap leaks.
 * ═══════════════════════════════════════════════ */
static void safe_clear_and_reload(const char *path)
{
    processList *list = &gui.page.processes.processElementsList;
    emptyList(list, PROCESS_NODE);
    readConfigFile(path, false);
}


/* ═══════════════════════════════════════════════
 * Test 1: Write and read config — processes survive round-trip
 * ═══════════════════════════════════════════════ */
static void test_config_write_read(void)
{
    TEST_BEGIN("Persistence — write/read config preserves processes");

    processList *list = &gui.page.processes.processElementsList;
    int32_t orig_count = list->size;
    TEST_ASSERT(orig_count > 0, "need at least one process");

    /* Save first process name and step count for later comparison */
    processNode *first = list->start;
    char saved_name[MAX_PROC_NAME_LEN + 1];
    snprintf(saved_name, sizeof(saved_name), "%s", first->process.processDetails->data.processNameString);
    int32_t saved_step_count = first->process.processDetails->stepElementsList.size;

    test_printf("         [INFO] Saving %d processes (first: \"%s\", %d steps)\n",
           (int)orig_count, saved_name, (int)saved_step_count);

    /* Write current state to test config file */
    writeConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);

    /* Clear all processes safely (no LVGL access) and read back */
    safe_clear_and_reload(TEST_CONFIG_FILE);
    test_pump(100);

    /* Verify process count restored */
    int32_t restored_count = list->size;
    test_printf("         [INFO] Restored %d processes\n", (int)restored_count);
    TEST_ASSERT_EQ((int)restored_count, (int)orig_count,
                   "process count should match after read");

    /* Verify first process name */
    processNode *restored_first = list->start;
    TEST_ASSERT_NOT_NULL(restored_first, "first restored process should exist");
    TEST_ASSERT_STR_EQ(restored_first->process.processDetails->data.processNameString,
                       saved_name, "first process name should match");

    /* Verify step count */
    int32_t restored_steps = restored_first->process.processDetails->stepElementsList.size;
    test_printf("         [INFO] First process steps: saved=%d, restored=%d\n",
           (int)saved_step_count, (int)restored_steps);
    TEST_ASSERT_EQ((int)restored_steps, (int)saved_step_count,
                   "step count should match after read");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: ALL 10 settings fields survive config round-trip
 * ═══════════════════════════════════════════════ */
static void test_settings_persistence(void)
{
    TEST_BEGIN("Persistence — ALL 10 settings fields survive write/read cycle");

    struct machineSettings *s = &gui.page.settings.settingsParams;

    /* Save originals to restore later */
    struct machineSettings saved = *s;

    /* Set ALL 10 fields to distinctive, non-default values */
    s->tempUnit                  = FAHRENHEIT_TEMP;   /* 1 */
    s->waterInlet                = true;              /* 2 */
    s->calibratedTemp            = 22;                /* 3 */
    s->filmRotationSpeedSetpoint = 77;                /* 4 */
    s->rotationIntervalSetpoint  = 42;                /* 5 */
    s->randomSetpoint            = 33;                /* 6 */
    s->isPersistentAlarm         = true;              /* 7 */
    s->isProcessAutostart        = true;              /* 8 */
    s->drainFillOverlapSetpoint  = 55;                /* 9 */
    s->multiRinseTime            = 120;               /* 10 */

    test_printf("         [INFO] Set all 10 fields: tempUnit=%d water=%d cal=%d speed=%d "
                "interval=%d random=%d alarm=%d autostart=%d overlap=%d rinse=%d\n",
           s->tempUnit, s->waterInlet, s->calibratedTemp,
           s->filmRotationSpeedSetpoint, s->rotationIntervalSetpoint,
           s->randomSetpoint, s->isPersistentAlarm, s->isProcessAutostart,
           s->drainFillOverlapSetpoint, s->multiRinseTime);

    /* Write config */
    writeConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);

    /* Clobber ALL 10 fields to opposite values */
    s->tempUnit                  = CELSIUS_TEMP;
    s->waterInlet                = false;
    s->calibratedTemp            = 0;
    s->filmRotationSpeedSetpoint = 0;
    s->rotationIntervalSetpoint  = 0;
    s->randomSetpoint            = 0;
    s->isPersistentAlarm         = false;
    s->isProcessAutostart        = false;
    s->drainFillOverlapSetpoint  = 0;
    s->multiRinseTime            = 0;

    /* Read back (safe_clear_and_reload frees old process nodes first) */
    safe_clear_and_reload(TEST_CONFIG_FILE);
    test_pump(100);

    /* Verify ALL 10 fields restored */
    TEST_ASSERT_EQ((int)s->tempUnit, (int)FAHRENHEIT_TEMP,
                   "tempUnit should be restored to Fahrenheit");
    TEST_ASSERT_EQ((int)s->waterInlet, 1,
                   "waterInlet should be restored to true");
    TEST_ASSERT_EQ((int)s->calibratedTemp, 22,
                   "calibratedTemp should be restored to 22");
    TEST_ASSERT_EQ((int)s->filmRotationSpeedSetpoint, 77,
                   "rotation speed should be restored to 77");
    TEST_ASSERT_EQ((int)s->rotationIntervalSetpoint, 42,
                   "rotation interval should be restored to 42");
    TEST_ASSERT_EQ((int)s->randomSetpoint, 33,
                   "random setpoint should be restored to 33");
    TEST_ASSERT_EQ((int)s->isPersistentAlarm, 1,
                   "persistent alarm should be restored to true");
    TEST_ASSERT_EQ((int)s->isProcessAutostart, 1,
                   "autostart should be restored to true");
    TEST_ASSERT_EQ((int)s->drainFillOverlapSetpoint, 55,
                   "drain/fill overlap should be restored to 55");
    TEST_ASSERT_EQ((int)s->multiRinseTime, 120,
                   "multiRinseTime should be restored to 120");

    test_printf("         [INFO] All 10 settings fields verified OK\n");

    /* Restore original settings for subsequent tests */
    *s = saved;

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2b: Per-process data fields survive round-trip
 * ═══════════════════════════════════════════════ */
static void test_process_data_persistence(void)
{
    TEST_BEGIN("Persistence — per-process fields survive write/read cycle");

    processList *list = &gui.page.processes.processElementsList;
    TEST_ASSERT(list->size > 0, "need at least one process");

    processNode *orig = list->start;
    sProcessData *od = &orig->process.processDetails->data;

    /* Snapshot all data fields of first process */
    char saved_name[MAX_PROC_NAME_LEN + 1];
    snprintf(saved_name, sizeof(saved_name), "%s", od->processNameString);
    int32_t saved_temp    = od->temp;
    float   saved_tol     = od->tempTolerance;
    uint8_t saved_tc      = od->isTempControlled;
    uint8_t saved_pref    = od->isPreferred;
    uint8_t saved_ft      = od->filmType;
    uint32_t saved_mins   = od->timeMins;
    uint8_t saved_secs    = od->timeSecs;

    test_printf("         [INFO] Snapshot: \"%s\" temp=%d tol=%.1f tc=%d pref=%d ft=%d %dm%ds\n",
                saved_name, (int)saved_temp, (double)saved_tol,
                saved_tc, saved_pref, saved_ft, (int)saved_mins, saved_secs);

    /* Write */
    writeConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);

    /* Clear safely and reload */
    safe_clear_and_reload(TEST_CONFIG_FILE);
    test_pump(100);

    TEST_ASSERT(list->size > 0, "at least one process restored");
    processNode *rest = list->start;
    sProcessData *rd = &rest->process.processDetails->data;

    TEST_ASSERT_STR_EQ(rd->processNameString, saved_name, "name matches");
    TEST_ASSERT_EQ((int)rd->temp, (int)saved_temp, "temp matches");
    TEST_ASSERT(rd->tempTolerance == saved_tol, "tempTolerance matches");
    TEST_ASSERT_EQ((int)rd->isTempControlled, (int)saved_tc, "isTempControlled matches");
    TEST_ASSERT_EQ((int)rd->isPreferred, (int)saved_pref, "isPreferred matches");
    TEST_ASSERT_EQ((int)rd->filmType, (int)saved_ft, "filmType matches");
    TEST_ASSERT_EQ((int)rd->timeMins, (int)saved_mins, "timeMins matches");
    TEST_ASSERT_EQ((int)rd->timeSecs, (int)saved_secs, "timeSecs matches");

    test_printf("         [INFO] All 8 process data fields verified OK\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2c: Per-step data fields survive round-trip
 * ═══════════════════════════════════════════════ */
static void test_step_data_persistence(void)
{
    TEST_BEGIN("Persistence — per-step fields survive write/read cycle");

    processList *list = &gui.page.processes.processElementsList;
    TEST_ASSERT(list->size > 0, "need at least one process");

    processNode *proc = list->start;
    stepList *sl = &proc->process.processDetails->stepElementsList;
    TEST_ASSERT(sl->size > 0, "first process needs at least one step");

    /* Snapshot ALL step data for first process */
    int32_t n = sl->size;
    /* Allocate temp arrays on stack (max 30 steps) */
    char    sn_names[MAX_STEP_ELEMENTS][MAX_PROC_NAME_LEN + 1];
    uint8_t sn_mins[MAX_STEP_ELEMENTS];
    uint8_t sn_secs[MAX_STEP_ELEMENTS];
    uint8_t sn_type[MAX_STEP_ELEMENTS];
    uint8_t sn_src[MAX_STEP_ELEMENTS];
    uint8_t sn_disc[MAX_STEP_ELEMENTS];

    stepNode *s = sl->start;
    for (int i = 0; i < n && s != NULL; i++, s = s->next) {
        snprintf(sn_names[i], sizeof(sn_names[i]), "%s", s->step.stepDetails->data.stepNameString);
        sn_mins[i] = s->step.stepDetails->data.timeMins;
        sn_secs[i] = s->step.stepDetails->data.timeSecs;
        sn_type[i] = s->step.stepDetails->data.type;
        sn_src[i]  = s->step.stepDetails->data.source;
        sn_disc[i] = s->step.stepDetails->data.discardAfterProc;
    }

    test_printf("         [INFO] Snapshot %d steps from \"%s\"\n",
                (int)n, proc->process.processDetails->data.processNameString);

    /* Write, clear safely, read back */
    writeConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);
    safe_clear_and_reload(TEST_CONFIG_FILE);
    test_pump(100);

    TEST_ASSERT(list->size > 0, "process list restored");
    processNode *rproc = list->start;
    stepList *rsl = &rproc->process.processDetails->stepElementsList;
    TEST_ASSERT_EQ((int)rsl->size, (int)n, "step count matches");

    /* Verify each step field-by-field */
    stepNode *rs = rsl->start;
    int verified = 0;
    for (int i = 0; i < n && rs != NULL; i++, rs = rs->next) {
        sStepData *d = &rs->step.stepDetails->data;
        TEST_ASSERT_STR_EQ(d->stepNameString, sn_names[i], "step name matches");
        TEST_ASSERT_EQ(d->timeMins, sn_mins[i], "step timeMins matches");
        TEST_ASSERT_EQ(d->timeSecs, sn_secs[i], "step timeSecs matches");
        TEST_ASSERT_EQ(d->type,     sn_type[i], "step type matches");
        TEST_ASSERT_EQ(d->source,   sn_src[i],  "step source matches");
        TEST_ASSERT_EQ(d->discardAfterProc, sn_disc[i], "step discard matches");
        verified++;
    }

    test_printf("         [INFO] All 6 fields x %d steps = %d checks verified OK\n",
                verified, verified * 6);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: Reading missing config file doesn't crash
 * ═══════════════════════════════════════════════ */
static void test_missing_config_no_crash(void)
{
    TEST_BEGIN("Persistence — missing config file doesn't crash");

    /* Save current state first so we can restore it */
    int32_t saved_count = gui.page.processes.processElementsList.size;

    /* Try to read a file that doesn't exist */
    readConfigFile("/NonExistentFile_12345.cfg", false);
    test_pump(100);

    /* The function should return gracefully — we just verify no crash
     * and that the process list is still intact (readConfigFile should
     * not corrupt existing data when the file doesn't exist) */
    test_printf("         [INFO] Process count after reading missing file: %d\n",
           (int)gui.page.processes.processElementsList.size);

    /* Process count should be unchanged (read failed, nothing loaded) */
    TEST_ASSERT_EQ((int)gui.page.processes.processElementsList.size,
                   (int)saved_count,
                   "process list should be unchanged after failed read");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: Statistics persistence (completed, totalMins, stopped, clean)
 * ═══════════════════════════════════════════════ */
static void test_stats_persistence(void)
{
    TEST_BEGIN("Persistence — statistics survive write/read cycle");

    machineStatistics *stats = &gui.page.tools.machineStats;

    /* Save originals */
    uint32_t orig_completed = stats->completed;
    uint64_t orig_totalMins = stats->totalMins;
    uint32_t orig_totalSecs = stats->totalSecs;
    uint32_t orig_stopped   = stats->stopped;
    uint32_t orig_clean     = stats->clean;

    /* Set distinctive values */
    stats->completed = 5;
    stats->totalMins = 120;
    stats->totalSecs = 45;
    stats->stopped   = 2;
    stats->clean     = 3;

    test_printf("         [INFO] Set stats: completed=%d, totalMins=%llu, totalSecs=%u, stopped=%d, clean=%d\n",
                stats->completed, (unsigned long long)stats->totalMins,
                (unsigned)stats->totalSecs, stats->stopped, stats->clean);

    /* Write config */
    writeConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);

    /* Clobber stats to 0 */
    stats->completed = 0;
    stats->totalMins = 0;
    stats->totalSecs = 0;
    stats->stopped   = 0;
    stats->clean     = 0;

    /* Load back */
    safe_clear_and_reload(TEST_CONFIG_FILE);
    test_pump(100);

    /* Verify all stats restored */
    test_printf("         [INFO] Restored stats: completed=%d, totalMins=%llu, totalSecs=%u, stopped=%d, clean=%d\n",
                stats->completed, (unsigned long long)stats->totalMins,
                (unsigned)stats->totalSecs, stats->stopped, stats->clean);
    TEST_ASSERT_EQ((int)stats->completed, 5, "completed count should be restored");
    TEST_ASSERT_EQ((int)stats->totalMins, 120, "totalMins should be restored");
    TEST_ASSERT_EQ((int)stats->totalSecs, 45, "totalSecs should be restored");
    TEST_ASSERT_EQ((int)stats->stopped, 2, "stopped count should be restored");
    TEST_ASSERT_EQ((int)stats->clean, 3, "clean count should be restored");

    /* Restore originals */
    stats->completed = orig_completed;
    stats->totalMins = orig_totalMins;
    stats->totalSecs = orig_totalSecs;
    stats->stopped   = orig_stopped;
    stats->clean     = orig_clean;

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: Temperature calibration offset persistence
 * ═══════════════════════════════════════════════ */
static void test_tempCalibOffset_persistence(void)
{
    TEST_BEGIN("Persistence — tempCalibOffset survives write/read cycle");

    struct machineSettings *s = &gui.page.settings.settingsParams;

    /* Save original */
    int8_t orig_offset = s->tempCalibOffset;

    /* Set to a known value (-15 = -1.5°C) */
    s->tempCalibOffset = -15;
    test_printf("         [INFO] Set tempCalibOffset = %d (= -1.5°C)\n", s->tempCalibOffset);

    /* Write config */
    writeConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);

    /* Clear to 0 */
    s->tempCalibOffset = 0;

    /* Load back */
    safe_clear_and_reload(TEST_CONFIG_FILE);
    test_pump(100);

    /* Verify restored */
    test_printf("         [INFO] Restored tempCalibOffset = %d\n", s->tempCalibOffset);
    TEST_ASSERT_EQ((int)s->tempCalibOffset, -15,
                   "tempCalibOffset should be restored to -15");

    /* Restore original */
    s->tempCalibOffset = orig_offset;

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_persistence(void)
{
    TEST_SUITE("Persistence");

    test_config_write_read();
    test_settings_persistence();
    test_process_data_persistence();
    test_step_data_persistence();
    test_missing_config_no_crash();
    test_stats_persistence();
    test_tempCalibOffset_persistence();
}
