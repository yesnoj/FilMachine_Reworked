/**
 * test_persistence.c — Config File Persistence Tests
 *
 * Tests the write/read cycle of the configuration file (FatFS stub → sd/FilMachine.cfg).
 * Verifies that processes, steps, and settings survive a save/load round-trip.
 */

#include "test_runner.h"
#include "lvgl.h"
#include <unistd.h>

/* Test-specific config filename to avoid corrupting the main one */
#define TEST_CONFIG_FILE "/FilMachine_Test.cfg"


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
    strncpy(saved_name, first->process.processDetails->processNameString, MAX_PROC_NAME_LEN);
    saved_name[MAX_PROC_NAME_LEN] = '\0';
    int32_t saved_step_count = first->process.processDetails->stepElementsList.size;

    test_printf("         [INFO] Saving %d processes (first: \"%s\", %d steps)\n",
           (int)orig_count, saved_name, (int)saved_step_count);

    /* Write current state to test config file */
    writeConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);

    /* Now clear ALL processes from the list by deleting from the end */
    while (list->size > 0) {
        processNode *last = list->end;
        deleteProcessElement(last);
    }
    TEST_ASSERT_EQ((int)list->size, 0, "list should be empty after clearing");

    /* Read back the config */
    readConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);

    /* Verify process count restored */
    int32_t restored_count = list->size;
    test_printf("         [INFO] Restored %d processes\n", (int)restored_count);
    TEST_ASSERT_EQ((int)restored_count, (int)orig_count,
                   "process count should match after read");

    /* Verify first process name */
    processNode *restored_first = list->start;
    TEST_ASSERT_NOT_NULL(restored_first, "first restored process should exist");
    TEST_ASSERT_STR_EQ(restored_first->process.processDetails->processNameString,
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
 * Test 2: Settings survive config round-trip
 * ═══════════════════════════════════════════════ */
static void test_settings_persistence(void)
{
    TEST_BEGIN("Persistence — settings survive write/read cycle");

    struct machineSettings *s = &gui.page.settings.settingsParams;

    /* Modify some settings to known values */
    s->tempUnit = FAHRENHEIT_TEMP;
    s->filmRotationSpeedSetpoint = 77;
    s->rotationIntervalSetpoint = 42;
    s->randomSetpoint = 33;
    s->isPersistentAlarm = true;
    s->isProcessAutostart = true;

    test_printf("         [INFO] Set tempUnit=%d, speed=%d, interval=%d, random=%d\n",
           s->tempUnit, s->filmRotationSpeedSetpoint,
           s->rotationIntervalSetpoint, s->randomSetpoint);

    /* Write config */
    writeConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);

    /* Alter the settings to different values */
    s->tempUnit = CELSIUS_TEMP;
    s->filmRotationSpeedSetpoint = 0;
    s->rotationIntervalSetpoint = 0;
    s->randomSetpoint = 0;
    s->isPersistentAlarm = false;
    s->isProcessAutostart = false;

    /* Read back */
    readConfigFile(TEST_CONFIG_FILE, false);
    test_pump(100);

    /* Verify settings were restored */
    TEST_ASSERT_EQ((int)s->tempUnit, (int)FAHRENHEIT_TEMP,
                   "tempUnit should be restored to Fahrenheit");
    TEST_ASSERT_EQ((int)s->filmRotationSpeedSetpoint, 77,
                   "rotation speed should be restored");
    TEST_ASSERT_EQ((int)s->rotationIntervalSetpoint, 42,
                   "rotation interval should be restored");
    TEST_ASSERT_EQ((int)s->randomSetpoint, 33,
                   "random setpoint should be restored");
    TEST_ASSERT_EQ((int)s->isPersistentAlarm, 1,
                   "persistent alarm should be restored");
    TEST_ASSERT_EQ((int)s->isProcessAutostart, 1,
                   "autostart should be restored");

    /* Reset settings back to sensible defaults for subsequent tests */
    s->tempUnit = CELSIUS_TEMP;
    s->isPersistentAlarm = false;
    s->isProcessAutostart = false;

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
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_persistence(void)
{
    TEST_SUITE("Persistence");

    test_config_write_read();
    test_settings_persistence();
    test_missing_config_no_crash();
}
