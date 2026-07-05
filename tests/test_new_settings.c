/**
 * test_new_settings.c — Tests for New Settings Fields
 *
 * Tests the settings added in the latest development cycle:
 *   - Pump speed slider
 *   - Tank size selector (S/M/L)
 *   - Container size textarea
 *   - Water bath size textarea
 *   - Chemistry volume selector (Low/High)
 *   - Wi-Fi SSID and password fields
 */

#include "test_runner.h"
#include "lvgl.h"

/* ── Helper: switch to Settings tab ── */
static void go_to_settings(void)
{
    test_click_obj(gui.page.menu.settingsTab);
    test_pump(300);
}


/* ═══════════════════════════════════════════════
 * Test 1: Pump speed UI elements exist
 * ═══════════════════════════════════════════════ */
static void test_settings_pump_speed_exists(void)
{
    TEST_BEGIN("Settings — pump speed slider exists");

    go_to_settings();

    TEST_ASSERT_NOT_NULL(gui.page.settings.pumpSpeedContainer,
                         "pump speed container should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.pumpSpeedLabel,
                         "pump speed label should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.pumpSpeedSlider,
                         "pump speed slider should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.pumpSpeedValueLabel,
                         "pump speed value label should exist");

    const char *label = lv_label_get_text(gui.page.settings.pumpSpeedLabel);
    TEST_ASSERT_STR_EQ(label, pumpSpeed_text, "label should say 'Pump speed'");

    int32_t val = lv_slider_get_value(gui.page.settings.pumpSpeedSlider);
    test_printf("         [INFO] Pump speed slider value: %d\n", (int)val);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: Tank size UI elements exist
 * ═══════════════════════════════════════════════ */
static void test_settings_tank_size_exists(void)
{
    TEST_BEGIN("Settings — tank size selector exists");

    TEST_ASSERT_NOT_NULL(gui.page.settings.tankSizeContainer,
                         "tank size container should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.tankSizeLabel,
                         "tank size label should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.tankSizeTextArea,
                         "tank size text area should exist");

    const char *label = lv_label_get_text(gui.page.settings.tankSizeLabel);
    TEST_ASSERT_STR_EQ(label, tankSize_text, "label should say 'Tank size'");

    const char *value = lv_textarea_get_text(gui.page.settings.tankSizeTextArea);
    TEST_ASSERT_NOT_NULL(value, "tank size value should not be NULL");
    test_printf("         [INFO] Tank size value: %s\n", value);

    TEST_END();
}


/* Tests 3 & 4 (container-size / water-bath-size textareas) were removed:
 * those Settings rows no longer exist — capacity was replaced by the calibrated
 * fill flow (TUNE / Maintenance → Fill), so there is nothing to assert here. */


/* ═══════════════════════════════════════════════
 * Test 5: Chemistry volume UI elements exist
 * ═══════════════════════════════════════════════ */
static void test_settings_chem_volume_exists(void)
{
    TEST_BEGIN("Settings — chemistry volume selector exists");

    TEST_ASSERT_NOT_NULL(gui.page.settings.chemVolumeContainer,
                         "chemistry volume container should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.chemVolumeLabel,
                         "chemistry volume label should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.chemVolumeTextArea,
                         "chemistry volume text area should exist");

    const char *label = lv_label_get_text(gui.page.settings.chemVolumeLabel);
    TEST_ASSERT_STR_EQ(label, chemistryVolume_text, "label should say 'Chemistry size'");

    const char *value = lv_textarea_get_text(gui.page.settings.chemVolumeTextArea);
    TEST_ASSERT_NOT_NULL(value, "chemistry volume value should not be NULL");
    test_printf("         [INFO] Chemistry volume value: %s\n", value);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 6: Settings params — new fields have valid defaults
 * ═══════════════════════════════════════════════ */
static void test_settings_new_defaults(void)
{
    TEST_BEGIN("Settings — new fields have valid default values");

    struct machineSettings *s = &gui.page.settings.settingsParams;

    /* Pump speed: 10–100 */
    TEST_ASSERT(s->pumpSpeed >= 10 && s->pumpSpeed <= 100,
                "pump speed should be in range 10–100");
    test_printf("         [INFO] pumpSpeed=%d\n", s->pumpSpeed);

    /* Tank size: 1=S, 2=M, 3=L */
    TEST_ASSERT(s->tankSize >= 1 && s->tankSize <= 3,
                "tank size should be 1, 2, or 3");
    test_printf("         [INFO] tankSize=%d\n", s->tankSize);

    /* Chem calibrated fill time: 0 (uncalibrated) up to 3600 s */
    TEST_ASSERT(s->chemCalibFillSecs <= 3600,
                "chem calib fill secs should be 0–3600");
    test_printf("         [INFO] chemCalibFillSecs=%d\n", s->chemCalibFillSecs);

    /* WB calibrated fill time: 0 (uncalibrated) up to 3600 s */
    TEST_ASSERT(s->wbCalibFillSecs <= 3600,
                "WB calib fill secs should be 0–3600");
    test_printf("         [INFO] wbCalibFillSecs=%d\n", s->wbCalibFillSecs);

    /* Chemistry volume: 1=Low, 2=High */
    TEST_ASSERT(s->chemistryVolume >= 1 && s->chemistryVolume <= 2,
                "chemistry volume should be 1 or 2");
    test_printf("         [INFO] chemistryVolume=%d\n", s->chemistryVolume);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 7: Selfcheck button exists in Tools
 * ═══════════════════════════════════════════════ */
static void test_settings_selfcheck_button(void)
{
    TEST_BEGIN("Tools — self-check button exists");

    test_click_obj(gui.page.menu.toolsTab);
    test_pump(300);

    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsSelfcheckButton,
                         "selfcheck button should exist");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsSelfcheckLabel,
                         "selfcheck label should exist");

    const char *label = lv_label_get_text(gui.page.tools.toolsSelfcheckLabel);
    TEST_ASSERT_STR_EQ(label, selfCheck_text, "label should say 'Self-check'");

    test_printf("         [INFO] Self-check button verified in Tools tab\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_new_settings(void)
{
    TEST_SUITE("New Settings & Features");

    test_settings_pump_speed_exists();
    test_settings_tank_size_exists();
    /* container-size / wb-size row tests removed (rows no longer exist) */
    test_settings_chem_volume_exists();
    test_settings_new_defaults();
    test_settings_selfcheck_button();
}
