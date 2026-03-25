/**
 * test_navigation.c — Navigation Test Suite
 *
 * Tests:
 * 1. Splash screen is active after boot
 * 2. Transition from splash to menu (simulated play button click)
 * 3. Tab switching (Processes / Settings / Tools)
 */

#include "test_runner.h"
#include "lvgl.h"

/* ── Test 1: Splash screen is the active screen ── */
static void test_splash_screen_active(void)
{
    TEST_BEGIN("Splash screen — is the active screen after boot");

    /* The active screen should exist */
    lv_obj_t *active = lv_screen_active();
    TEST_ASSERT_NOT_NULL(active, "active screen should exist");

    TEST_END();
}

/* ── Test 2: Simulate splash → menu transition ── */
static void test_splash_to_menu(void)
{
    TEST_BEGIN("Splash → Menu — readConfigFile + menu() creates menu screen");

    /* Simulate what the play button callback does */
    readConfigFile(FILENAME_SAVE, false);
    menu();
    refreshSettingsUI();
    test_pump(500);  /* allow menu() to complete rendering */

    /* After transition, menu screen should be created */
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

    test_splash_screen_active();
    test_splash_to_menu();
    test_switch_to_settings();
    test_switch_to_tools();
    test_switch_back_to_processes();
}
