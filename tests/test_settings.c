/**
 * test_settings.c — Settings Test Suite
 *
 * Requires: previous suites have run (we're on the menu screen).
 *
 * Tests:
 * 1. Switch to Settings tab — UI elements exist
 * 2. Temperature unit radio buttons exist
 * 3. Sliders exist and have valid ranges
 * 4. Switches exist and are functional
 */

#include "test_runner.h"
#include "lvgl.h"

/* ── Test 1: Settings tab UI elements exist ── */
static void test_settings_elements_exist(void)
{
    TEST_BEGIN("Settings tab — UI elements exist");

    /* Switch to settings tab using actual LVGL object */
    test_click_obj(gui.page.menu.settingsTab);
    test_pump(300);

    TEST_ASSERT_EQ(gui.page.menu.newSelection, TAB_SETTINGS,
                   "should be on Settings tab");

    /* Check main container */
    TEST_ASSERT_NOT_NULL(gui.page.settings.settingsSection,
                         "settingsSection should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.settingsContainer,
                         "settingsContainer should exist");

    /* Temperature unit */
    TEST_ASSERT_NOT_NULL(gui.page.settings.tempUnitCelsiusRadioButton,
                         "Celsius radio button should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.tempUnitFahrenheitRadioButton,
                         "Fahrenheit radio button should exist");

    /* Sliders */
    TEST_ASSERT_NOT_NULL(gui.page.settings.filmRotationSpeedSlider,
                         "rotation speed slider should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.filmRotationInversionIntervalSlider,
                         "inversion interval slider should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.filmRandomSlider,
                         "random slider should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.drainFillTimeSlider,
                         "drain/fill time slider should exist");

    /* Switches */
    TEST_ASSERT_NOT_NULL(gui.page.settings.waterInletSwitch,
                         "water inlet switch should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.persistentAlarmSwitch,
                         "persistent alarm switch should exist");
    TEST_ASSERT_NOT_NULL(gui.page.settings.autostartSwitch,
                         "autostart switch should exist");

    TEST_END();
}

/* ── Test 2: Settings params have reasonable defaults ── */
static void test_settings_defaults(void)
{
    TEST_BEGIN("Settings — params have reasonable defaults");

    struct machineSettings *s = &gui.page.settings.settingsParams;

    /* Temperature unit should be Celsius (0) or Fahrenheit (1) */
    TEST_ASSERT(s->tempUnit == CELSIUS_TEMP || s->tempUnit == FAHRENHEIT_TEMP,
                "tempUnit should be Celsius or Fahrenheit");

    /* Rotation speed: 0-100 range */
    test_printf("         [INFO] Rotation speed: %d\n", s->filmRotationSpeedSetpoint);
    TEST_ASSERT(s->filmRotationSpeedSetpoint >= 0 && s->filmRotationSpeedSetpoint <= 100,
                "rotation speed should be in valid range");

    /* Inversion interval: 0-100 range */
    test_printf("         [INFO] Inversion interval: %d\n", s->rotationIntervalSetpoint);
    TEST_ASSERT(s->rotationIntervalSetpoint >= 0 && s->rotationIntervalSetpoint <= 100,
                "inversion interval should be in valid range");

    /* Randomness: 0-100% */
    test_printf("         [INFO] Randomness: %d\n", s->randomSetpoint);
    TEST_ASSERT(s->randomSetpoint >= 0 && s->randomSetpoint <= 100,
                "randomness should be in valid range");

    TEST_END();
}

/* ── Test 3: Change temperature unit ── */
static void test_change_temp_unit(void)
{
    TEST_BEGIN("Settings — change temperature unit");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    tempUnit_t old_unit = s->tempUnit;
    test_printf("         [INFO] Current tempUnit: %d\n", (int)old_unit);

    /* Toggle the temperature unit */
    tempUnit_t new_unit = (old_unit == CELSIUS_TEMP) ? FAHRENHEIT_TEMP : CELSIUS_TEMP;
    s->tempUnit = new_unit;

    /* Verify it was set */
    TEST_ASSERT_EQ((int)s->tempUnit, (int)new_unit,
                   "tempUnit should be updated");
    test_printf("         [INFO] New tempUnit: %d\n", (int)s->tempUnit);

    /* Restore original */
    s->tempUnit = old_unit;

    TEST_END();
}

/* ── Test 4: Modify slider value ── */
static void test_modify_slider(void)
{
    TEST_BEGIN("Settings — slider value change updates param");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    uint8_t old_speed = s->filmRotationSpeedSetpoint;
    test_printf("         [INFO] Old rotation speed: %d\n", old_speed);

    /* Set slider to a known value (must be multiple of 10 for rounding) */
    lv_slider_set_value(gui.page.settings.filmRotationSpeedSlider, 50, LV_ANIM_OFF);
    test_pump(50);

    /* Trigger the VALUE_CHANGED event — the handler reads the slider value
     * and updates settingsParams.filmRotationSpeedSetpoint.
     * User_data for this event is the value label (set during registration). */
    lv_obj_send_event(gui.page.settings.filmRotationSpeedSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    test_printf("         [INFO] New rotation speed: %d\n", s->filmRotationSpeedSetpoint);
    TEST_ASSERT_EQ((int)s->filmRotationSpeedSetpoint, 50,
                   "rotation speed should be 50 after slider change");

    /* Restore original value */
    lv_slider_set_value(gui.page.settings.filmRotationSpeedSlider, old_speed, LV_ANIM_OFF);
    lv_obj_send_event(gui.page.settings.filmRotationSpeedSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    TEST_END();
}

/* ── Test 5: Toggle switch ── */
static void test_toggle_switch(void)
{
    TEST_BEGIN("Settings — switch toggle updates param");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    bool old_inlet = s->waterInlet;
    test_printf("         [INFO] Old waterInlet: %d\n", (int)old_inlet);

    /* Toggle the switch state */
    if (old_inlet) {
        lv_obj_remove_state(gui.page.settings.waterInletSwitch, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(gui.page.settings.waterInletSwitch, LV_STATE_CHECKED);
    }
    test_pump(50);

    /* Trigger VALUE_CHANGED — the handler reads lv_obj_has_state and updates param */
    lv_obj_send_event(gui.page.settings.waterInletSwitch,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    test_printf("         [INFO] New waterInlet: %d\n", (int)s->waterInlet);
    TEST_ASSERT(s->waterInlet != old_inlet,
                "waterInlet should have toggled");

    /* Restore original state */
    if (old_inlet) {
        lv_obj_add_state(gui.page.settings.waterInletSwitch, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(gui.page.settings.waterInletSwitch, LV_STATE_CHECKED);
    }
    lv_obj_send_event(gui.page.settings.waterInletSwitch,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    TEST_END();
}

/* ── Test 6: Inversion interval slider ── */
static void test_inversion_interval_slider(void)
{
    TEST_BEGIN("Settings — inversion interval slider updates param");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    uint8_t old_val = s->rotationIntervalSetpoint;
    test_printf("         [INFO] Old interval: %d\n", old_val);

    /* Set to 30 (must be multiple of 10 for rounding) */
    lv_slider_set_value(gui.page.settings.filmRotationInversionIntervalSlider, 30, LV_ANIM_OFF);
    test_pump(50);
    lv_obj_send_event(gui.page.settings.filmRotationInversionIntervalSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    test_printf("         [INFO] New interval: %d\n", s->rotationIntervalSetpoint);
    TEST_ASSERT_EQ((int)s->rotationIntervalSetpoint, 30,
                   "interval should be 30 after slider change");

    /* Restore */
    lv_slider_set_value(gui.page.settings.filmRotationInversionIntervalSlider, old_val, LV_ANIM_OFF);
    lv_obj_send_event(gui.page.settings.filmRotationInversionIntervalSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    TEST_END();
}

/* ── Test 7: Randomness slider ── */
static void test_randomness_slider(void)
{
    TEST_BEGIN("Settings — randomness slider updates param");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    uint8_t old_val = s->randomSetpoint;
    test_printf("         [INFO] Old randomness: %d\n", old_val);

    /* Set to 60 (rounds to nearest 20 → 60) */
    lv_slider_set_value(gui.page.settings.filmRandomSlider, 60, LV_ANIM_OFF);
    test_pump(50);
    lv_obj_send_event(gui.page.settings.filmRandomSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    test_printf("         [INFO] New randomness: %d\n", s->randomSetpoint);
    TEST_ASSERT_EQ((int)s->randomSetpoint, 60,
                   "randomness should be 60 after slider change");

    /* Restore */
    lv_slider_set_value(gui.page.settings.filmRandomSlider, old_val, LV_ANIM_OFF);
    lv_obj_send_event(gui.page.settings.filmRandomSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    TEST_END();
}

/* ── Test 8: Persistent alarm switch toggle ── */
static void test_persistent_alarm_switch(void)
{
    TEST_BEGIN("Settings — persistent alarm switch toggle");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    bool old_val = s->isPersistentAlarm;
    test_printf("         [INFO] Old persistent alarm: %d\n", (int)old_val);

    /* Toggle */
    if (old_val) {
        lv_obj_remove_state(gui.page.settings.persistentAlarmSwitch, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(gui.page.settings.persistentAlarmSwitch, LV_STATE_CHECKED);
    }
    test_pump(50);
    lv_obj_send_event(gui.page.settings.persistentAlarmSwitch,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    test_printf("         [INFO] New persistent alarm: %d\n", (int)s->isPersistentAlarm);
    TEST_ASSERT(s->isPersistentAlarm != old_val,
                "persistent alarm should have toggled");

    /* Restore */
    if (old_val) {
        lv_obj_add_state(gui.page.settings.persistentAlarmSwitch, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(gui.page.settings.persistentAlarmSwitch, LV_STATE_CHECKED);
    }
    lv_obj_send_event(gui.page.settings.persistentAlarmSwitch,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    TEST_END();
}

/* ── Test 9: Autostart switch toggle ── */
static void test_autostart_switch(void)
{
    TEST_BEGIN("Settings — autostart switch toggle");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    bool old_val = s->isProcessAutostart;
    test_printf("         [INFO] Old autostart: %d\n", (int)old_val);

    /* Toggle */
    if (old_val) {
        lv_obj_remove_state(gui.page.settings.autostartSwitch, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(gui.page.settings.autostartSwitch, LV_STATE_CHECKED);
    }
    test_pump(50);
    lv_obj_send_event(gui.page.settings.autostartSwitch,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    test_printf("         [INFO] New autostart: %d\n", (int)s->isProcessAutostart);
    TEST_ASSERT(s->isProcessAutostart != old_val,
                "autostart should have toggled");

    /* Restore */
    if (old_val) {
        lv_obj_add_state(gui.page.settings.autostartSwitch, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(gui.page.settings.autostartSwitch, LV_STATE_CHECKED);
    }
    lv_obj_send_event(gui.page.settings.autostartSwitch,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    TEST_END();
}

/* ── Test 10: Drain/fill overlap slider ── */
static void test_drain_fill_slider(void)
{
    TEST_BEGIN("Settings — drain/fill overlap slider updates param");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    uint8_t old_val = s->drainFillOverlapSetpoint;
    test_printf("         [INFO] Old drain/fill overlap: %d\n", old_val);

    /* Set to 50 (rounds to nearest 50) */
    lv_slider_set_value(gui.page.settings.drainFillTimeSlider, 50, LV_ANIM_OFF);
    test_pump(50);
    lv_obj_send_event(gui.page.settings.drainFillTimeSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    test_printf("         [INFO] New drain/fill overlap: %d\n", s->drainFillOverlapSetpoint);
    TEST_ASSERT_EQ((int)s->drainFillOverlapSetpoint, 50,
                   "drain/fill overlap should be 50");

    /* Restore */
    lv_slider_set_value(gui.page.settings.drainFillTimeSlider, old_val, LV_ANIM_OFF);
    lv_obj_send_event(gui.page.settings.drainFillTimeSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    TEST_END();
}

/* ── Test 11: Multi-rinse time slider (rounds to 30, range 60-180) ── */
static void test_multi_rinse_time_slider(void)
{
    TEST_BEGIN("Settings — multi-rinse time slider updates param");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    uint8_t old_val = s->multiRinseTime;
    test_printf("         [INFO] Old multi-rinse time: %d\n", old_val);

    /* Set to 120 (must be multiple of 30, within 60-180 range) */
    lv_slider_set_value(gui.page.settings.multiRinseTimeSlider, 120, LV_ANIM_OFF);
    test_pump(50);
    lv_obj_send_event(gui.page.settings.multiRinseTimeSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    test_printf("         [INFO] New multi-rinse time: %d\n", s->multiRinseTime);
    TEST_ASSERT_EQ((int)s->multiRinseTime, 120,
                   "multi-rinse should be 120 after slider change");

    /* Restore */
    lv_slider_set_value(gui.page.settings.multiRinseTimeSlider, old_val, LV_ANIM_OFF);
    lv_obj_send_event(gui.page.settings.multiRinseTimeSlider,
                      LV_EVENT_VALUE_CHANGED, NULL);
    test_pump(100);

    TEST_END();
}

/* ── Test 12: Default settings values after initGlobals ── */
static void test_default_settings_values(void)
{
    TEST_BEGIN("Settings — default values verified");

    struct machineSettings *s = &gui.page.settings.settingsParams;

    /* According to requirements, after initGlobals() these should be:
     * tankSize == 2 (Medium)
     * pumpSpeed == 30
     * chemContainerMl == 500
     * wbContainerMl == 2000
     * drainFillOverlapSetpoint == 100 */

    test_printf("         [INFO] tankSize=%d, pumpSpeed=%d, chemMl=%d, wbMl=%d, overlap=%d\n",
                s->tankSize, s->pumpSpeed, s->chemContainerMl, s->wbContainerMl,
                s->drainFillOverlapSetpoint);

    TEST_ASSERT_EQ((int)s->tankSize, 2,
                   "tankSize should default to 2 (Medium)");
    TEST_ASSERT_EQ((int)s->pumpSpeed, 30,
                   "pumpSpeed should default to 30");
    TEST_ASSERT_EQ((int)s->chemContainerMl, 500,
                   "chemContainerMl should default to 500");
    TEST_ASSERT_EQ((int)s->wbContainerMl, 2000,
                   "wbContainerMl should default to 2000");
    TEST_ASSERT_EQ((int)s->drainFillOverlapSetpoint, 100,
                   "drainFillOverlapSetpoint should default to 100");

    TEST_END();
}


/* ── Test 13: Temperature calibration offset is accessible ── */
static void test_tempCalibOffset_accessible(void)
{
    TEST_BEGIN("Settings — tempCalibOffset field is accessible");

    struct machineSettings *s = &gui.page.settings.settingsParams;
    int8_t old_offset = s->tempCalibOffset;

    /* Verify we can set and read the offset */
    s->tempCalibOffset = -20;
    test_printf("         [INFO] Set tempCalibOffset to -20\n");
    TEST_ASSERT_EQ((int)s->tempCalibOffset, -20,
                   "tempCalibOffset should be set to -20");

    /* Set a positive value */
    s->tempCalibOffset = 10;
    test_printf("         [INFO] Set tempCalibOffset to 10\n");
    TEST_ASSERT_EQ((int)s->tempCalibOffset, 10,
                   "tempCalibOffset should be set to 10");

    /* Restore original */
    s->tempCalibOffset = old_offset;

    TEST_END();
}


/* ── Test 14: Switch back to processes for next tests ── */
static void test_settings_return_to_processes(void)
{
    TEST_BEGIN("Settings — return to Processes tab");

    test_click_obj(gui.page.menu.processesTab);
    test_pump(300);

    TEST_ASSERT_EQ(gui.page.menu.newSelection, TAB_PROCESSES,
                   "should be back on Processes tab");

    TEST_END();
}

/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_settings(void)
{
    TEST_SUITE("Settings");

    test_settings_elements_exist();
    test_settings_defaults();
    test_change_temp_unit();
    test_modify_slider();
    test_toggle_switch();
    test_inversion_interval_slider();
    test_randomness_slider();
    test_persistent_alarm_switch();
    test_autostart_switch();
    test_drain_fill_slider();
    test_multi_rinse_time_slider();
    test_default_settings_values();
    test_tempCalibOffset_accessible();
    test_settings_return_to_processes();
}
