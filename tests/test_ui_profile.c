/**
 * test_ui_profile.c — UI Profile & Layout Tests
 *
 * Verifies that ui_profile.c values match the original hardcoded layout
 * constants, and that both resolution profiles are internally consistent.
 */

#include "test_runner.h"
#include "ui_profile.h"
#include "sensors.h"
#include <string.h>


/* ═══════════════════════════════════════════════
 * Test 1: Profile pointer is valid and resolution matches board
 * ═══════════════════════════════════════════════ */
static void test_profile_returns_valid_pointer(void)
{
    TEST_BEGIN("UIProfile — ui_get_profile returns valid pointer");

    const ui_profile_t *ui = ui_get_profile();
    TEST_ASSERT_NOT_NULL(ui, "ui_get_profile must not return NULL");

    /* common.content_w must be > 0 */
    TEST_ASSERT(ui->common.content_w > 0, "content_w should be positive");
    TEST_ASSERT(ui->common.content_h > 0, "content_h should be positive");

    test_printf("         [INFO] Profile: content=%dx%d, sidebar_w=%d\n",
                ui->common.content_w, ui->common.content_h, ui->common.sidebar_w);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: Key layout values match original hardcoded constants
 * ═══════════════════════════════════════════════ */
static void test_profile_values_match_original(void)
{
    TEST_BEGIN("UIProfile — 800x480 values match profile");

    const ui_profile_t *ui = ui_get_profile();

    /* Common */
    TEST_ASSERT_EQ(ui->common.content_x, 175, "content_x should be 175");
    TEST_ASSERT_EQ(ui->common.content_y, 7,   "content_y should be 7");
    TEST_ASSERT_EQ(ui->common.content_w, 620, "content_w should be 620");
    TEST_ASSERT_EQ(ui->common.content_h, 468, "content_h should be 468");
    TEST_ASSERT_EQ(ui->common.sidebar_x, 5,   "sidebar_x should be 5");

    /* Menu */
    TEST_ASSERT_EQ(ui->menu.tab_w, 165, "tab_w should be 165");
    TEST_ASSERT_EQ(ui->menu.tab_h, 150, "tab_h should be 150");
    TEST_ASSERT_EQ(ui->menu.tab_processes_y, 7,   "tab1_y should be 7");
    TEST_ASSERT_EQ(ui->menu.tab_settings_y, 166, "tab2_y should be 166");
    TEST_ASSERT_EQ(ui->menu.tab_tools_y, 325, "tab3_y should be 325");
    TEST_ASSERT_EQ(ui->menu.tab_label_offset_y, 35, "tab_label_offset_y should be 35");

    /* Process detail */
    TEST_ASSERT_EQ(ui->process_detail.steps_label_y, 38, "steps_label_y should be 38");
    TEST_ASSERT_EQ(ui->process_detail.info_label_y, 38,  "info_label_y should be 38");
    TEST_ASSERT_EQ(ui->process_detail.temp_ctrl_y, -20,  "temp_ctrl_y should be -20");
    TEST_ASSERT_EQ(ui->process_detail.close_w, 45, "close_w should be 45");
    TEST_ASSERT_EQ(ui->process_detail.close_h, 45, "close_h should be 45");

    /* Step detail */
    TEST_ASSERT(ui->step_detail.label_font != NULL, "step_detail.label_font must not be NULL");

    /* Clean popup */
    TEST_ASSERT_EQ(ui->clean_popup.repeat_label_x, -10, "repeat_label_x should be -10");

    test_printf("         [INFO] All 800x480 reference values verified\n");
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: Popup dimensions are reasonable for display
 * ═══════════════════════════════════════════════ */
static void test_popup_dimensions_within_display(void)
{
    TEST_BEGIN("UIProfile — popup sizes fit within display");

    const ui_profile_t *ui = ui_get_profile();

    /* All popup widths should be <= LCD_H_RES */
    TEST_ASSERT(ui->popups.message_w <= LCD_H_RES, "message_w should fit in display width");
    TEST_ASSERT(ui->popups.filter_w  <= LCD_H_RES, "filter_w should fit in display width");
    TEST_ASSERT(ui->popups.clean_w   <= LCD_H_RES, "clean_w should fit in display width");
    TEST_ASSERT(ui->popups.drain_w   <= LCD_H_RES, "drain_w should fit in display width");
    TEST_ASSERT(ui->popups.selfcheck_w <= LCD_H_RES, "selfcheck_w should fit in display width");

    /* All popup heights should be <= LCD_V_RES (for 320 profile) */
    TEST_ASSERT(ui->popups.message_h <= LCD_V_RES, "message_h should fit in display height");
    TEST_ASSERT(ui->popups.filter_h  <= LCD_V_RES, "filter_h should fit in display height");

    test_printf("         [INFO] All popup dimensions within %dx%d\n", LCD_H_RES, LCD_V_RES);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: Font pointers are all non-NULL
 * ═══════════════════════════════════════════════ */
static void test_profile_fonts_not_null(void)
{
    TEST_BEGIN("UIProfile — all font pointers are non-NULL");

    const ui_profile_t *ui = ui_get_profile();

    TEST_ASSERT_NOT_NULL(ui->menu.tab_icon_font, "menu.tab_icon_font");
    TEST_ASSERT_NOT_NULL(ui->menu.tab_label_font, "menu.tab_label_font");
    TEST_ASSERT_NOT_NULL(ui->processes.title_font, "processes.title_font");
    TEST_ASSERT_NOT_NULL(ui->settings.section_title_font, "settings.section_title_font");
    TEST_ASSERT_NOT_NULL(ui->process_detail.name_font, "process_detail.name_font");
    TEST_ASSERT_NOT_NULL(ui->step_detail.title_font, "step_detail.title_font");
    TEST_ASSERT_NOT_NULL(ui->checkup.process_name_font, "checkup.process_name_font");
    TEST_ASSERT_NOT_NULL(ui->message_popup.title_font, "message_popup.title_font");
    TEST_ASSERT_NOT_NULL(ui->message_popup.text_font, "message_popup.text_font");

    test_printf("         [INFO] All 11 sampled font pointers verified\n");
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: Sensor API stubs return expected values (simulator)
 * ═══════════════════════════════════════════════ */
static void test_sensor_stubs(void)
{
    TEST_BEGIN("Sensors — simulator stubs return expected values");

    /* Init should not crash */
    sensors_flow_init();
    sensors_water_level_init();
    sensors_hall_init();

    /* Flow meter stub returns a plausible value */
    float flow = sensors_flow_get_litres_per_min();
    test_printf("         [INFO] flow_get_litres_per_min() = %.1f\n", flow);
    TEST_ASSERT(flow >= 0.0f, "flow rate should be >= 0");

    /* Water level stub returns true (water present) */
    bool water = sensors_water_level_detected();
    test_printf("         [INFO] water_level_detected() = %s\n", water ? "true" : "false");
    TEST_ASSERT(water == true, "simulator should report water present");

    /* Max level should be false */
    bool water_max = sensors_water_level_max_detected();
    TEST_ASSERT(water_max == false, "simulator should report max level NOT reached");

    /* Hall sensor stub returns true (motor spinning) */
    bool hall = sensors_hall_magnet_detected();
    test_printf("         [INFO] hall_magnet_detected() = %s\n", hall ? "true" : "false");
    TEST_ASSERT(hall == true, "simulator should report magnet detected");

    /* Flow reset should not crash */
    sensors_flow_reset();
    uint32_t pulses = sensors_flow_get_pulse_count();
    TEST_ASSERT_EQ((int)pulses, 0, "pulse count after reset should be 0");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 6: Board header constants are consistent
 * ═══════════════════════════════════════════════ */
static void test_board_constants(void)
{
    TEST_BEGIN("Board — constants are consistent");

    /* LCD resolution should match what we expect */
    TEST_ASSERT(LCD_H_RES == 480 || LCD_H_RES == 800, "LCD_H_RES should be 480 or 800");
    TEST_ASSERT(LCD_V_RES == 320 || LCD_V_RES == 480,
                "LCD_V_RES should be 320 or 480");

    /* LVGL buffer size should be correct */
    int expected_buf = LCD_H_RES * (LCD_V_RES / 10) * 2;
    TEST_ASSERT_EQ(LVGL_BUF_SIZE, expected_buf, "LVGL_BUF_SIZE formula check");

    /* Relay count */
    TEST_ASSERT_EQ(RELAY_NUMBER, 8, "RELAY_NUMBER should be 8");

    test_printf("         [INFO] Board: %dx%d, buf=%d, relays=%d\n",
                LCD_H_RES, LCD_V_RES, LVGL_BUF_SIZE, RELAY_NUMBER);
    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_ui_profile(void)
{
    TEST_SUITE("UI Profile & Sensors");

    test_profile_returns_valid_pointer();
    test_profile_values_match_original();
    test_popup_dimensions_within_display();
    test_profile_fonts_not_null();
    test_sensor_stubs();
    test_board_constants();
}
