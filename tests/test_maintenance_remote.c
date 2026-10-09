/**
 * test_maintenance_remote.c — Tools → Drain / Clean / Export driven by the app
 *
 * Feeds WebSocket commands with ws_debug_handle_command() (as if they came
 * from the app), runs the LVGL timers with test_pump() and checks the state
 * the app reads (drainTool*, cleanTool*, exportSeq) and the popups on the
 * display. Fill times are set to one or two seconds so a whole run is short.
 *
 * The SDL display driver makes LVGL read the real clock (SDL_GetTicks), so the
 * 1 s timers of the popups only advance with real time: pump_real() sleeps.
 */

#include "test_runner.h"
#include "ws_server.h"
#include "lvgl.h"
#include <unistd.h>

static uint16_t s_origChemSecs, s_origWbSecs;

static void fast_fill_times(void)
{
    struct machineSettings *S = &gui.page.settings.settingsParams;
    s_origChemSecs = S->chemCalibFillSecs;
    s_origWbSecs   = S->wbCalibFillSecs;
    S->chemCalibFillSecs = 1;     /* one fill or drain of a container = 1 s */
    S->wbCalibFillSecs   = 1;     /* water bath = 1 s */
}

static void restore_fill_times(void)
{
    struct machineSettings *S = &gui.page.settings.settingsParams;
    S->chemCalibFillSecs = s_origChemSecs;
    S->wbCalibFillSecs   = s_origWbSecs;
}

/* Run LVGL for `ms` of real time. */
static void pump_real(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 10) {
        usleep(10000);
        test_pump(10);
    }
}

static void cmd(const char *json)
{
    ws_debug_handle_command(json);
    test_pump(60);                /* lv_async_call runs on the next timer pass */
}

/* Pump in 200 ms steps until `done()` or `limitMs`; returns true if done. */
static bool pump_until(bool (*done)(void), uint32_t limitMs)
{
    for (uint32_t t = 0; t < limitMs; t += 100) {
        if (done()) return true;
        pump_real(100);
    }
    return done();
}

static bool drain_ended(void) { int s = drainToolState(); return s == DRAIN_TOOL_DONE || s == DRAIN_TOOL_STOPPED; }
static bool clean_ended(void) { int s = cleanToolState(); return s == CLEAN_TOOL_DONE || s == CLEAN_TOOL_STOPPED; }

static bool popup_visible(lv_obj_t *parent)
{
    return parent != NULL && !lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN);
}

static bool state_has(const char *needle)
{
    static char buf[8192];
    ws_debug_build_state(buf, sizeof(buf));
    return strstr(buf, needle) != NULL;
}

/* ── Drain ─────────────────────────────────────────────── */

static void test_drain_remote_full(void)
{
    TEST_BEGIN("Remote drain — start from the app, C1..WB, done, close");
    fast_fill_times();
    cmd("{\"cmd\":\"drain_start\"}");
    TEST_ASSERT_EQ(drainToolState(), DRAIN_TOOL_RUNNING, "draining after drain_start");
    TEST_ASSERT(popup_visible(gui.element.drainPopup.drainPopupParent), "drain popup shown on the display");
    TEST_ASSERT_EQ(drainToolTank(), 0, "starts with C1");
    TEST_ASSERT_EQ(drainToolRemainingSecs(), 4, "remaining = 3 containers x 1 s + bath 1 s");
    TEST_ASSERT(maintenanceBusy(), "maintenance busy while draining");
    TEST_ASSERT(state_has("\"drainToolState\":1"), "state JSON: drainToolState 1");

    pump_real(1500);
    TEST_ASSERT_EQ(drainToolTank(), 1, "then C2");

    TEST_ASSERT(pump_until(drain_ended, 15000), "drain ends");
    TEST_ASSERT_EQ(drainToolState(), DRAIN_TOOL_DONE, "DONE");
    TEST_ASSERT_EQ(drainToolLevelPct(), 0, "level 0 when done");
    TEST_ASSERT(!maintenanceBusy(), "not busy when done");
    TEST_ASSERT(state_has("\"drainToolState\":2"), "state JSON: drainToolState 2");

    cmd("{\"cmd\":\"drain_close\"}");
    TEST_ASSERT_EQ(drainToolState(), DRAIN_TOOL_IDLE, "IDLE after close");
    TEST_ASSERT(!popup_visible(gui.element.drainPopup.drainPopupParent), "popup closed");
    TEST_ASSERT(!alarm_is_active(), "alarm silenced by close");
    restore_fill_times();
    TEST_END();
}

static void test_drain_remote_stop(void)
{
    TEST_BEGIN("Remote drain — Stop from the app");
    fast_fill_times();
    gui.page.settings.settingsParams.chemCalibFillSecs = 3;
    cmd("{\"cmd\":\"drain_start\"}");
    pump_real(1200);
    cmd("{\"cmd\":\"drain_stop\"}");
    TEST_ASSERT(pump_until(drain_ended, 3000), "stops within a tick");
    TEST_ASSERT_EQ(drainToolState(), DRAIN_TOOL_STOPPED, "STOPPED");
    cmd("{\"cmd\":\"drain_close\"}");
    TEST_ASSERT_EQ(drainToolState(), DRAIN_TOOL_IDLE, "IDLE after close");
    restore_fill_times();
    TEST_END();
}

static void test_busy_rejections(void)
{
    TEST_BEGIN("Remote tools — one at a time, no process while draining");
    fast_fill_times();
    cmd("{\"cmd\":\"drain_start\"}");
    TEST_ASSERT(drainToolBusy(), "draining");

    ws_debug_handle_command("{\"cmd\":\"clean_start\",\"mask\":1,\"cycles\":1}");
    TEST_ASSERT_STR_EQ(ws_debug_last_event(), "clean_rejected", "clean refused while draining");
    ws_debug_handle_command("{\"cmd\":\"drain_start\"}");
    TEST_ASSERT_STR_EQ(ws_debug_last_event(), "drain_rejected", "second drain refused");
    ws_debug_handle_command("{\"cmd\":\"start_process\",\"index\":0}");
    TEST_ASSERT_STR_EQ(ws_debug_last_event(), "start_rejected", "process refused while draining");
    test_pump(60);
    TEST_ASSERT(!cleanToolBusy(), "clean did not start");

    cmd("{\"cmd\":\"drain_stop\"}");
    pump_until(drain_ended, 3000);
    cmd("{\"cmd\":\"drain_close\"}");
    restore_fill_times();
    TEST_END();
}

/* ── Clean ─────────────────────────────────────────────── */

static int s_seen[3];
static bool s_sawWaste, s_fillingInWaste, s_arcMismatch;
static bool clean_ended_tracking(void)
{
    int st = cleanToolState();
    if (st == CLEAN_TOOL_RUNNING) s_seen[cleanToolContainer()]++;
    if (st == CLEAN_TOOL_WASTE) {
        s_sawWaste = true;
        if (cleanToolFilling()) s_fillingInWaste = true;
    }
    /* The app draws the arcs from these values: they must be the display's. */
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    if (cleanToolProcessArc() != lv_arc_get_value(cp->cleanProcessArc) ||
        cleanToolCycleArc()   != lv_arc_get_value(cp->cleanCycleArc) ||
        cleanToolPumpArc()    != lv_arc_get_value(cp->cleanPumpArc)) s_arcMismatch = true;
    return clean_ended();
}

static void test_clean_only_selected(void)
{
    TEST_BEGIN("Remote clean — only C2, 1 cycle: never touches C1/C3");
    fast_fill_times();
    memset(s_seen, 0, sizeof(s_seen));
    s_sawWaste = s_fillingInWaste = s_arcMismatch = false;
    uint32_t cleans = gui.page.tools.machineStats.clean;

    cmd("{\"cmd\":\"clean_start\",\"mask\":2,\"cycles\":1,\"drainWb\":false}");
    TEST_ASSERT_EQ(cleanToolState(), CLEAN_TOOL_RUNNING, "running after clean_start");
    TEST_ASSERT(popup_visible(gui.element.cleanPopup.cleanPopupParent), "clean popup shown on the display");
    TEST_ASSERT_EQ(cleanToolMask(), 2, "mask C2");
    TEST_ASSERT(lv_obj_has_state(gui.element.cleanPopup.cleanSelectC2CheckBox, LV_STATE_CHECKED), "C2 ticked on the display");
    TEST_ASSERT(!lv_obj_has_state(gui.element.cleanPopup.cleanSelectC1CheckBox, LV_STATE_CHECKED), "C1 not ticked");
    TEST_ASSERT_EQ(cleanToolContainer(), 1, "starts with C2 (was always C1)");

    TEST_ASSERT(pump_until(clean_ended_tracking, 15000), "clean ends");
    TEST_ASSERT_EQ(s_seen[0], 0, "C1 never cleaned");
    TEST_ASSERT_EQ(s_seen[2], 0, "C3 never cleaned");
    TEST_ASSERT(s_seen[1] > 0, "C2 cleaned");
    TEST_ASSERT(!s_sawWaste, "no bath drain");
    TEST_ASSERT_EQ(cleanToolState(), CLEAN_TOOL_DONE, "DONE");
    TEST_ASSERT_EQ(cleanToolContainer(), 1, "still reports C2 at the end (was C3)");
    TEST_ASSERT(!cleanToolFilling(), "not filling after the end");
    TEST_ASSERT(!s_arcMismatch, "arcs for the app = arcs on the display");
    TEST_ASSERT_EQ(cleanToolPercent(), 100, "100 % when done");
    TEST_ASSERT_EQ(gui.page.tools.machineStats.clean, cleans + 1, "clean cycles statistic +1");

    cmd("{\"cmd\":\"clean_close\"}");
    TEST_ASSERT_EQ(cleanToolState(), CLEAN_TOOL_IDLE, "IDLE after close");
    TEST_ASSERT(!popup_visible(gui.element.cleanPopup.cleanPopupParent), "popup closed");
    restore_fill_times();
    TEST_END();
}

static void test_clean_c1_c3_two_cycles_drain_bath(void)
{
    TEST_BEGIN("Remote clean — C1 + C3, 2 cycles, then bath to waste");
    fast_fill_times();
    memset(s_seen, 0, sizeof(s_seen));
    s_sawWaste = s_fillingInWaste = s_arcMismatch = false;

    cmd("{\"cmd\":\"clean_start\",\"mask\":5,\"cycles\":2,\"drainWb\":true}");
    TEST_ASSERT_EQ(cleanToolCycles(), 2, "2 cycles");
    TEST_ASSERT(cleanToolDrainWb(), "bath drain on");
    TEST_ASSERT_EQ(cleanToolRemainingSecs(), 8, "remaining = 2 cycles x (fill + drain) x 1 s x 2 containers");
    TEST_ASSERT(state_has("\"cleanToolMask\":5"), "state JSON: mask 5");

    TEST_ASSERT(pump_until(clean_ended_tracking, 40000), "clean ends");
    TEST_ASSERT(s_seen[0] > 0 && s_seen[2] > 0, "C1 and C3 cleaned");
    TEST_ASSERT_EQ(s_seen[1], 0, "C2 skipped");
    TEST_ASSERT(s_sawWaste, "bath drained to waste at the end");
    TEST_ASSERT(!s_fillingInWaste, "bath to waste reported as draining, as on the display");
    TEST_ASSERT(!s_arcMismatch, "arcs for the app = arcs on the display");
    TEST_ASSERT_EQ(cleanToolState(), CLEAN_TOOL_DONE, "DONE");
    cmd("{\"cmd\":\"clean_close\"}");
    restore_fill_times();
    TEST_END();
}

static void test_clean_stop_while_filling(void)
{
    TEST_BEGIN("Remote clean — Stop while filling: pumps it back, then STOPPED");
    fast_fill_times();
    gui.page.settings.settingsParams.chemCalibFillSecs = 4;
    cmd("{\"cmd\":\"clean_start\",\"mask\":1,\"cycles\":1,\"drainWb\":false}");
    pump_real(2100);                                   /* ~2 s into the fill */
    TEST_ASSERT(cleanToolFilling(), "filling C1");
    cmd("{\"cmd\":\"clean_stop\"}");
    TEST_ASSERT_EQ(cleanToolState(), CLEAN_TOOL_STOPPING, "STOPPING: draining back");
    TEST_ASSERT(!cleanToolFilling(), "now draining");
    uint32_t back = cleanToolRemainingSecs();
    TEST_ASSERT(back >= 1 && back <= 3, "drains back for about the time it filled");
    TEST_ASSERT(pump_until(clean_ended, 6000), "ends");
    TEST_ASSERT_EQ(cleanToolState(), CLEAN_TOOL_STOPPED, "STOPPED");
    cmd("{\"cmd\":\"clean_close\"}");
    TEST_ASSERT_EQ(cleanToolState(), CLEAN_TOOL_IDLE, "IDLE after close");
    restore_fill_times();
    TEST_END();
}

static void test_clean_no_container(void)
{
    TEST_BEGIN("Remote clean — no container selected is refused");
    ws_debug_handle_command("{\"cmd\":\"clean_start\",\"mask\":0,\"cycles\":1}");
    TEST_ASSERT_STR_EQ(ws_debug_last_event(), "clean_rejected", "clean_rejected");
    test_pump(60);
    TEST_ASSERT(!cleanToolBusy(), "not started");
    TEST_END();
}

/* ── Export ────────────────────────────────────────────── */

static void test_export_remote(void)
{
    TEST_BEGIN("Remote export — queued for the system task, result in the state");
    uint16_t msg;
    while (xQueueReceive(sys.sysActionQ, &msg, 0) == pdTRUE) { }   /* empty the queue */
    ws_debug_handle_command("{\"cmd\":\"export_config\"}");
    bool queued = false;
    while (xQueueReceive(sys.sysActionQ, &msg, 0) == pdTRUE) if (msg == EXPORT_CFG) queued = true;
    TEST_ASSERT(queued, "EXPORT_CFG queued");
    TEST_ASSERT(state_has("\"exportSeq\":"), "state JSON has exportSeq");
    TEST_ASSERT(state_has("\"exportOk\":"), "state JSON has exportOk");
    TEST_END();
}

static void test_state_fields(void)
{
    TEST_BEGIN("State JSON — drain / clean / export fields present");
    const char *keys[] = { "drainToolState", "drainToolTank", "drainToolLevelPct", "drainToolRemaining",
                           "cleanToolState", "cleanToolContainer", "cleanToolCycle", "cleanToolCycles",
                           "cleanToolFilling", "cleanToolPct", "cleanToolRemaining", "cleanToolMask",
                           "cleanToolDrainWb", "cleanToolProcessArc", "cleanToolCycleArc", "cleanToolPumpArc",
                           "exportSeq", "exportOk" };
    char k[64];
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        snprintf(k, sizeof(k), "\"%s\":", keys[i]);
        TEST_ASSERT(state_has(k), keys[i]);
    }
    TEST_END();
}

void test_suite_maintenance_remote(void)
{
    TEST_SUITE("Maintenance from the app (Drain / Clean / Export)");
    test_state_fields();
    test_drain_remote_full();
    test_drain_remote_stop();
    test_busy_rejections();
    test_clean_only_selected();
    test_clean_c1_c3_two_cycles_drain_bath();
    test_clean_stop_while_filling();
    test_clean_no_container();
    test_export_remote();
}
