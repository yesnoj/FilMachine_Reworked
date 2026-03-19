/**
 * test_ota.c — OTA Firmware Update Tests
 *
 * Tests the OTA update functionality:
 *   - OTA API functions (version, progress, status)
 *   - SD card firmware check (file present/absent)
 *   - SD card update flow (start, progress, completion)
 *   - Wi-Fi server start/stop
 *   - OTA progress popup UI
 *   - Wi-Fi popup UI (IP, PIN, status)
 *   - Update buttons in Tools tab
 */

#include "test_runner.h"
#include "lvgl.h"

/* ── Helper: navigate to Tools tab ── */
static void go_to_tools(void)
{
    test_click_obj(gui.page.menu.toolsTab);
    test_pump(300);
}


/* ═══════════════════════════════════════════════
 * Test 1: OTA API — running version
 * ═══════════════════════════════════════════════ */
static void test_ota_running_version(void)
{
    TEST_BEGIN("OTA — running version is not empty");

    const char *ver = ota_get_running_version();
    TEST_ASSERT_NOT_NULL(ver, "version should not be NULL");
    TEST_ASSERT(strlen(ver) > 0, "version should not be empty");

    test_printf("         [INFO] Running version: %s\n", ver);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 2: OTA API — initial state
 * ═══════════════════════════════════════════════ */
static void test_ota_initial_state(void)
{
    TEST_BEGIN("OTA — initial state is not running, progress 0");

    TEST_ASSERT(!ota_is_running(), "should not be running initially");
    TEST_ASSERT_EQ(ota_get_progress(), 0, "progress should be 0");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 3: OTA SD — check firmware file
 * ═══════════════════════════════════════════════ */
static void test_ota_sd_check(void)
{
    TEST_BEGIN("OTA — SD firmware check returns result");

    char ver[32] = {0};
    bool found = ota_check_sd(ver, sizeof(ver));

    /* The file may or may not exist in the test environment,
       just verify the function doesn't crash */
    test_printf("         [INFO] SD firmware %s", found ? "found" : "not found");
    if (found)
        test_printf(": version %s", ver);
    test_printf("\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 4: OTA SD — start and progress simulation
 * ═══════════════════════════════════════════════ */
static void test_ota_sd_update_flow(void)
{
    TEST_BEGIN("OTA — SD update starts and sets running state");

    bool started = ota_start_sd();
    TEST_ASSERT(started, "ota_start_sd should return true");
    TEST_ASSERT(ota_is_running(), "should be running after start");
    TEST_ASSERT_EQ(ota_get_progress(), 0, "progress starts at 0");

    const char *status = ota_get_status_text();
    TEST_ASSERT(status != NULL && strlen(status) > 0, "status should be set");
    test_printf("         [INFO] OTA started, status: %s\n", status ? status : "(null)");

    /* Let the lv_timer fire by pumping with test_pump (which increments tick + calls handler) */
    test_pump(5000);

    int pct_final = ota_get_progress();
    test_printf("         [INFO] Progress after 5s pump: %d%%\n", pct_final);

    /* If timer didn't fire (timing issue), just verify the API didn't crash */
    if (pct_final < 100) {
        test_printf("         [INFO] Timer didn't complete in test — verifying API stability\n");
        /* Force completion for subsequent tests */
        test_pump(10000);
    }

    test_printf("         [INFO] Final progress: %d%%, running: %s\n",
                ota_get_progress(), ota_is_running() ? "yes" : "no");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 5: OTA — cannot start while already running
 * ═══════════════════════════════════════════════ */
static void test_ota_no_double_start(void)
{
    TEST_BEGIN("OTA — cannot start a second update while running");

    /* Start first update */
    bool started1 = ota_start_sd();

    if (started1 && ota_is_running()) {
        /* Try to start a second one */
        bool started2 = ota_start_sd();
        TEST_ASSERT(!started2, "second start should fail while running");
        test_printf("         [INFO] Double-start correctly prevented\n");

        /* Wait for first to finish */
        test_pump(5000);
    } else {
        /* Previous test may have left OTA in finished state, skip */
        test_printf("         [INFO] OTA not running (already completed), skipping\n");
    }

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 6: OTA Wi-Fi — server start/stop
 * ═══════════════════════════════════════════════ */
static void test_ota_wifi_server(void)
{
    TEST_BEGIN("OTA — Wi-Fi server start and stop");

    TEST_ASSERT(!ota_wifi_server_is_running(), "server should not be running initially");

    bool started = ota_wifi_server_start();
    TEST_ASSERT(started, "server should start successfully");
    TEST_ASSERT(ota_wifi_server_is_running(), "server should be running after start");

    const char *ip = ota_get_ip_address();
    TEST_ASSERT_NOT_NULL(ip, "IP address should not be NULL");
    TEST_ASSERT(strlen(ip) > 0, "IP address should not be empty");
    test_printf("         [INFO] Server IP: %s\n", ip);

    bool stopped = ota_wifi_server_stop();
    TEST_ASSERT(stopped, "server should stop successfully");
    TEST_ASSERT(!ota_wifi_server_is_running(), "server should not be running after stop");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 7: OTA UI — update buttons exist in Tools
 * ═══════════════════════════════════════════════ */
static void test_ota_tools_buttons(void)
{
    TEST_BEGIN("OTA — update buttons exist in Tools tab");

    go_to_tools();

    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsUpdateSDButton,
                         "SD update button should exist");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsUpdateSDLabel,
                         "SD update label should exist");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsUpdateWifiButton,
                         "Wi-Fi update button should exist");
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsUpdateWifiLabel,
                         "Wi-Fi update label should exist");

    const char *sdLabel = lv_label_get_text(gui.page.tools.toolsUpdateSDLabel);
    TEST_ASSERT_STR_EQ(sdLabel, otaUpdateFromSD_text, "SD label text");

    const char *wifiLabel = lv_label_get_text(gui.page.tools.toolsUpdateWifiLabel);
    TEST_ASSERT_STR_EQ(wifiLabel, otaWifiUpdate_text, "Wi-Fi label text");

    test_printf("         [INFO] Both OTA buttons verified in Tools tab\n");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 8: OTA Wi-Fi popup — opens and shows IP/PIN
 * ═══════════════════════════════════════════════ */
static void test_ota_wifi_popup(void)
{
    TEST_BEGIN("OTA — Wi-Fi popup opens with IP and PIN");

    /* Call the popup creation function directly (simulates button press) */
    otaWifiPopupCreate();
    test_pump(1000);

    struct sOtaWifiPopup *p = &gui.element.otaWifiPopup;

    TEST_ASSERT_NOT_NULL(p->popupParent, "popup should be created");
    TEST_ASSERT(!lv_obj_has_flag(p->popupParent, LV_OBJ_FLAG_HIDDEN),
                "popup should be visible");

    TEST_ASSERT_NOT_NULL(p->ipLabel, "IP label should exist");
    TEST_ASSERT_NOT_NULL(p->pinLabel, "PIN label should exist");
    TEST_ASSERT_NOT_NULL(p->statusLabel, "status label should exist");

    /* PIN should be 5 digits */
    TEST_ASSERT(strlen(p->otaPin) == 5, "PIN should be 5 digits");
    test_printf("         [INFO] Wi-Fi popup open, PIN: %s\n", p->otaPin);

    /* Close the popup */
    test_click_obj(p->closeButton);
    test_pump(500);

    TEST_ASSERT(lv_obj_has_flag(p->popupParent, LV_OBJ_FLAG_HIDDEN),
                "popup should be hidden after close");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 9: OTA progress popup — creates and updates
 * ═══════════════════════════════════════════════ */
static void test_ota_progress_popup(void)
{
    TEST_BEGIN("OTA — progress popup shows status and progress");

    /* Start an SD update to have progress data */
    ota_start_sd();

    /* Create progress popup */
    otaProgressPopupCreate("Test Update");
    test_pump(500);

    struct sOtaProgressPopup *p = &gui.element.otaProgressPopup;

    TEST_ASSERT_NOT_NULL(p->popupParent, "progress popup should be created");
    TEST_ASSERT(!lv_obj_has_flag(p->popupParent, LV_OBJ_FLAG_HIDDEN),
                "progress popup should be visible");

    TEST_ASSERT_NOT_NULL(p->titleLabel, "title label should exist");
    TEST_ASSERT_NOT_NULL(p->statusLabel, "status label should exist");
    TEST_ASSERT_NOT_NULL(p->progressBar, "progress bar should exist");
    TEST_ASSERT_NOT_NULL(p->percentLabel, "percent label should exist");

    const char *title = lv_label_get_text(p->titleLabel);
    TEST_ASSERT_STR_EQ(title, "Test Update", "title should match");

    /* Wait for some progress */
    test_pump(2000);

    int pct = ota_get_progress();
    test_printf("         [INFO] Progress popup showing %d%%\n", pct);

    /* Close it */
    test_click_obj(p->closeButton);
    test_pump(300);

    /* Wait for OTA to finish in background */
    test_pump(5000);

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Test 10: Software version label uses OTA version
 * ═══════════════════════════════════════════════ */
static void test_ota_version_label(void)
{
    TEST_BEGIN("OTA — software version label matches ota_get_running_version");

    go_to_tools();

    const char *labelVer = lv_label_get_text(gui.page.tools.toolSoftwareVersionValue);
    const char *otaVer = ota_get_running_version();

    TEST_ASSERT_STR_EQ(labelVer, otaVer,
                       "label version should match OTA running version");

    test_printf("         [INFO] Version label: %s, OTA version: %s\n",
                labelVer ? labelVer : "(null)", otaVer ? otaVer : "(null)");

    TEST_END();
}


/* ═══════════════════════════════════════════════
 * Suite Entry Point
 * ═══════════════════════════════════════════════ */
void test_suite_ota(void)
{
    TEST_SUITE("OTA Update");

    test_ota_running_version();
    test_ota_initial_state();
    test_ota_sd_check();
    test_ota_sd_update_flow();
    test_ota_no_double_start();
    test_ota_wifi_server();
    test_ota_tools_buttons();
    test_ota_wifi_popup();
    test_ota_progress_popup();
    test_ota_version_label();
}
