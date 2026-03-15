/**
 * test_navigation.c — Navigation Test Suite
 *
 * Tests:
 * 1. Home page creation and start button
 * 2. Transition from home to menu
 * 3. Tab switching (Processes / Settings / Tools)
 */

#include "test_runner.h"
#include "lvgl.h"

/* ── Test 1: Home page has splash and start button ── */
static void test_home_page_created(void)
{
    TEST_BEGIN("Home page — splash screen and start button exist");

    TEST_ASSERT_NOT_NULL(gui.page.home.screen_home, "screen_home should exist");
    TEST_ASSERT_NOT_NULL(gui.page.home.startButton, "startButton should exist");
    TEST_ASSERT_NOT_NULL(gui.page.home.splashImage, "splashImage should exist");

    /* Verify start button is on screen */
    lv_obj_update_layout(gui.page.home.startButton);
    lv_area_t coords;
    lv_obj_get_coords(gui.page.home.startButton, &coords);
    test_printf("         [INFO] Start button area: (%d,%d)→(%d,%d)\n",
           (int)coords.x1, (int)coords.y1, (int)coords.x2, (int)coords.y2);

    TEST_END();
}

/* ── Test 2: Click Start button → transitions to menu ── */
static void test_start_button_opens_menu(void)
{
    TEST_BEGIN("Start button — click opens menu");

    /* Click the actual LVGL start button object (auto-computes center) */
    test_click_obj(gui.page.home.startButton);
    test_pump(500);  /* allow menu() to complete rendering */

    /* After clicking start, menu screen should be created */
    TEST_ASSERT_NOT_NULL(gui.page.menu.screen_mainMenu, "menu screen should exist");

    /* The active screen should be the menu */
    TEST_ASSERT(lv_screen_active() == gui.page.menu.screen_mainMenu,
                "active screen should be menu");

    /* All 3 tabs should exist */
    TEST_ASSERT_NOT_NULL(gui.page.menu.processesTab, "processesTab should exist");
    TEST_ASSERT_NOT_NULL(gui.page.menu.settingsTab, "settingsTab should exist");
    TEST_ASSERT_NOT_NULL(gui.page.menu.toolsTab, "toolsTab should exist");

    /* Processes tab should be active by default (menu() triggers initial click) */
    TEST_ASSERT_EQ(gui.page.menu.newSelection, TAB_PROCESSES,
                   "default tab should be Processes");

    /* Processes section should be visible */
    TEST_ASSERT_NOT_NULL(gui.page.processes.processesSection, "processesSection should exist");
    TEST_ASSERT(!lv_obj_has_flag(gui.page.processes.processesSection, LV_OBJ_FLAG_HIDDEN),
                "processes section should be visible");

    TEST_END();
}

/* ── Test 3: Switch to Settings tab ── */
static void test_switch_to_settings(void)
{
    TEST_BEGIN("Tab switch — click Settings tab");

    /* Click actual Settings tab object */
    test_click_obj(gui.page.menu.settingsTab);
    test_pump(300);

    /* Settings should now be the active selection */
    TEST_ASSERT_EQ(gui.page.menu.newSelection, TAB_SETTINGS,
                   "newSelection should be TAB_SETTINGS");

    /* Settings section visible, processes hidden */
    TEST_ASSERT(!lv_obj_has_flag(gui.page.settings.settingsSection, LV_OBJ_FLAG_HIDDEN),
                "settings section should be visible");
    TEST_ASSERT(lv_obj_has_flag(gui.page.processes.processesSection, LV_OBJ_FLAG_HIDDEN),
                "processes section should be hidden");

    TEST_END();
}

/* ── Test 4: Switch to Tools tab ── */
static void test_switch_to_tools(void)
{
    TEST_BEGIN("Tab switch — click Tools tab");

    test_click_obj(gui.page.menu.toolsTab);
    test_pump(300);

    TEST_ASSERT_EQ(gui.page.menu.newSelection, TAB_TOOLS,
                   "newSelection should be TAB_TOOLS");

    TEST_ASSERT(!lv_obj_has_flag(gui.page.tools.toolsSection, LV_OBJ_FLAG_HIDDEN),
                "tools section should be visible");
    TEST_ASSERT(lv_obj_has_flag(gui.page.settings.settingsSection, LV_OBJ_FLAG_HIDDEN),
                "settings section should be hidden");

    TEST_END();
}

/* ── Test 5: Switch back to Processes tab ── */
static void test_switch_back_to_processes(void)
{
    TEST_BEGIN("Tab switch — return to Processes tab");

    test_click_obj(gui.page.menu.processesTab);
    test_pump(300);

    TEST_ASSERT_EQ(gui.page.menu.newSelection, TAB_PROCESSES,
                   "newSelection should be TAB_PROCESSES");

    TEST_ASSERT(!lv_obj_has_flag(gui.page.processes.processesSection, LV_OBJ_FLAG_HIDDEN),
                "processes section should be visible");
    TEST_ASSERT(lv_obj_has_flag(gui.page.tools.toolsSection, LV_OBJ_FLAG_HIDDEN),
                "tools section should be hidden");

    TEST_END();
}

/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_navigation(void)
{
    TEST_SUITE("Navigation");

    test_home_page_created();
    test_start_button_opens_menu();
    test_switch_to_settings();
    test_switch_to_tools();
    test_switch_back_to_processes();
}
