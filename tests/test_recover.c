/**
 * test_recover.c — machine v7: Tools → "Empty Cx into the bottle", the
 * tank-mediated transfer engine, the Kamoer timings and the bath circulator.
 *
 * The engine runs on a 1 s LVGL timer and LVGL reads the real clock (SDL), so
 * these tests sleep; pumpSetTestParams() makes the pump a thousand times
 * faster and the air purge one second, so a whole recovery (four phases,
 * 1000 ml) takes about twenty seconds instead of six minutes.
 */

#include "test_runner.h"
#include "ws_server.h"
#include "lvgl.h"
#include <unistd.h>

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
    test_pump(60);                /* queued LVGL action runs on the next timer pass */
}

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

static void wait_pump_idle(void)
{
    for (int t = 0; t < 600 && xferBusy(); t++) pump_real(50);
}

static void fast_pump(void)    { pumpSetTestParams(60000, 1); }   /* 1000 ml/s, 1 s of air */
static void real_pump(void)    { wait_pump_idle(); pumpSetTestParams(0, 0); }

/* ── Kamoer timings ───────────────────────────────────────── */

static void test_pump_timings(void)
{
    TEST_BEGIN("Kamoer — times from the volume, linear duty, purge");
    uint8_t spd = gui.page.settings.settingsParams.pumpSpeed;
    gui.page.settings.settingsParams.pumpSpeed = 100;
    TEST_ASSERT_EQ(calculateFillTime(600, 100), 60, "600 ml at 100 % = 60 s (600 ml/min)");
    TEST_ASSERT_EQ(calculateFillTime(350, 100), 35, "350 ml (medium tank, low volume) = 35 s");
    TEST_ASSERT_EQ(calculateFillTime(350, 50), 70, "half speed = twice the time");
    TEST_ASSERT_EQ(calculateFillTime(12800, 100), 1280, "12.8 l bath: no overflow in the arithmetic");
    TEST_ASSERT_EQ(tankVolumeMl(2, 2), 700, "High volume, medium tank = 700 ml");
    TEST_ASSERT_EQ(tankFillSeconds(2, 2), 70, "...filled in 70 s");
    TEST_ASSERT_EQ(tankFillSeconds(1, 1), 25, "Low volume, small tank: 250 ml in 25 s");
    TEST_ASSERT_EQ(pumpPurgeSecs(), PUMP_PURGE_SECS, "purge at 100 % = PUMP_PURGE_SECS");
    gui.page.settings.settingsParams.pumpSpeed = 50;
    TEST_ASSERT_EQ(pumpPurgeSecs(), 2 * PUMP_PURGE_SECS, "purge at 50 % = twice as long");
    TEST_ASSERT_EQ(pumpPercentToDuty(100), PUMP_MAX_ANALOG_VAL, "100 % = max duty");
    TEST_ASSERT_EQ(pumpPercentToDuty(50), PUMP_MAX_ANALOG_VAL / 2, "50 % = half duty (linear)");
    TEST_ASSERT_EQ(pumpPercentToDuty(10), PUMP_MIN_ANALOG_VAL, "10 % is floored so the motor starts");
    gui.page.settings.settingsParams.pumpSpeed = 100;
    /* through the tank: twice the pump time plus one purge per 300 ml cycle */
    TEST_ASSERT_EQ(xferTransferSecs(300), 2 * 30 + PUMP_PURGE_SECS, "one cycle: 30 s in, 30 s out, air");
    TEST_ASSERT_EQ(xferTransferSecs(1000), 2 * 100 + 4 * PUMP_PURGE_SECS, "1000 ml: 4 cycles");
    gui.page.settings.settingsParams.pumpSpeed = spd;
    TEST_END();
}

/* ── Transfer engine ─────────────────────────────────────── */

static void test_xfer_engine(void)
{
    TEST_BEGIN("Transfer engine — fill, drain + air, volume target, graceful stop");
    fast_pump();
    TEST_ASSERT(!xferBusy(), "idle at start");
    TEST_ASSERT(xferStartVolume(C2_RLY, WASTE_RLY, 300, NULL), "starts");
    TEST_ASSERT(xferBusy(), "busy");
    TEST_ASSERT_EQ(xferPhase(), 1, "filling the tank first");
    TEST_ASSERT_EQ((int)xferFrom(), C2_RLY, "from C2");
    TEST_ASSERT_EQ((int)xferTo(), WASTE_RLY, "to waste");
    TEST_ASSERT(!xferStartVolume(C1_RLY, WASTE_RLY, 100, NULL), "a second start is refused while busy");
    pump_real(1300);
    TEST_ASSERT_EQ(xferPhase(), 2, "then draining it to the destination");
    pump_real(2300);
    TEST_ASSERT(!xferBusy(), "300 ml done: one cycle, idle again");
    TEST_ASSERT_EQ((int)xferMovedMl(), 300, "300 ml moved");

    /* until-stopped transfer with a graceful stop while filling */
    cleanRelayManager(WB_RLY, C1_RLY, PUMP_IN_RLY, true);
    TEST_ASSERT_EQ(xferPhase(), 1, "clean-style transfer filling");
    pump_real(300);
    cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
    TEST_ASSERT(xferBusy(), "stop is graceful: still busy");
    TEST_ASSERT(xferFinishing(), "...finishing");
    TEST_ASSERT_EQ(xferPhase(), 2, "what went in is drained to the destination");
    TEST_ASSERT_EQ((int)xferTo(), C1_RLY, "...which is C1");
    wait_pump_idle();
    TEST_ASSERT(!xferBusy(), "idle after the final drain");

    /* a start requested while finishing waits for the drain */
    cleanRelayManager(WB_RLY, C3_RLY, PUMP_IN_RLY, true);
    pump_real(1200);
    cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
    cleanRelayManager(C3_RLY, WB_RLY, PUMP_OUT_RLY, true);
    TEST_ASSERT_EQ((int)xferTo(), C3_RLY, "still draining to C3 first");
    for (int t = 0; t < 100 && !(xferPhase() == 1 && xferFrom() == C3_RLY); t++) pump_real(50);
    TEST_ASSERT(xferPhase() == 1 && xferFrom() == C3_RLY, "then the queued C3 -> WB transfer starts");
    xferAbort();
    TEST_ASSERT(!xferBusy(), "abort stops at once");
    real_pump();
    TEST_END();
}

/* ── The recovery tool from the app ─────────────────────── */

static bool recover_ended(void) { int s = recoverToolState(); return s == RECOVER_TOOL_DONE || s == RECOVER_TOOL_STOPPED; }

static int  s_seenFrom[8], s_seenTo[8];      /* valve pins seen per phase, bitmasks */
static bool s_badPhase;
static void track_phase(void)
{
    int ph = recoverToolPhase();
    if (ph < 0 || ph > 3) { s_badPhase = true; return; }
    if (xferBusy()) {
        s_seenFrom[ph] |= 1 << xferFrom();
        s_seenTo[ph]   |= 1 << xferTo();
    }
}
static bool recover_ended_tracking(void) { track_phase(); return recover_ended(); }
static bool pump_until(bool (*done)(void), uint32_t limitMs)
{
    for (uint32_t t = 0; t < limitMs; t += 100) {
        if (done()) return true;
        pump_real(100);
    }
    return done();
}

static void test_recover_full_run(void)
{
    TEST_BEGIN("Empty C2 into the bottle — four phases, right valves, done, close");
    fast_pump();
    memset(s_seenFrom, 0, sizeof(s_seenFrom));
    memset(s_seenTo, 0, sizeof(s_seenTo));
    s_badPhase = false;
    uint32_t cleans = gui.page.tools.machineStats.clean;

    cmd("{\"cmd\":\"recover_start\",\"container\":1}");
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_RUNNING, "running after recover_start");
    TEST_ASSERT_EQ(recoverToolContainer(), 1, "C2");
    TEST_ASSERT_EQ(recoverToolPhase(), RECOVER_PH_WASH1, "starts washing the line");
    TEST_ASSERT(popup_visible(gui.element.recoverPopup.recoverPopupParent), "popup shown on the display");
    TEST_ASSERT(!lv_obj_has_flag(gui.element.recoverPopup.recoverProcessContainer, LV_OBJ_FLAG_HIDDEN), "on its process page");
    TEST_ASSERT(maintenanceBusy(), "maintenance busy");
    TEST_ASSERT(state_has("\"recoverToolState\":1"), "state JSON: running");
    TEST_ASSERT(state_has("\"recoverToolContainer\":1"), "state JSON: container");
    TEST_ASSERT(recoverToolRemainingSecs() > 0, "has an estimate");

    ws_debug_handle_command("{\"cmd\":\"recover_start\",\"container\":0}");
    TEST_ASSERT_STR_EQ(ws_debug_last_event(), "recover_rejected", "a second one is refused");
    ws_debug_handle_command("{\"cmd\":\"start_process\",\"index\":0}");
    TEST_ASSERT_STR_EQ(ws_debug_last_event(), "start_rejected", "no process while recovering");

    TEST_ASSERT(pump_until(recover_ended_tracking, 90000), "ends");
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_DONE, "DONE");
    TEST_ASSERT(!s_badPhase, "phase always 0..3");
    TEST_ASSERT_EQ(s_seenFrom[RECOVER_PH_WASH1], 1 << WB_RLY, "wash 1: water from the bath only");
    TEST_ASSERT_EQ(s_seenTo[RECOVER_PH_WASH1], 1 << WASTE_RLY, "wash 1: to waste only");
    TEST_ASSERT_EQ(s_seenFrom[RECOVER_PH_HEAD], 1 << C2_RLY, "head: C2 only");
    TEST_ASSERT_EQ(s_seenTo[RECOVER_PH_HEAD], 1 << WASTE_RLY, "head: to waste");
    TEST_ASSERT_EQ(s_seenFrom[RECOVER_PH_XFER], 1 << C2_RLY, "transfer: C2 only");
    TEST_ASSERT_EQ(s_seenTo[RECOVER_PH_XFER], 1 << RECOVERY_RLY, "transfer: to the recovery outlet only");
    TEST_ASSERT_EQ(s_seenFrom[RECOVER_PH_WASH2], 1 << WB_RLY, "wash 2: water from the bath");
    TEST_ASSERT_EQ(s_seenTo[RECOVER_PH_WASH2], 1 << WASTE_RLY, "wash 2: to waste");
    TEST_ASSERT_EQ((int)recoverToolMovedMl(), CHEM_CONTAINER_ML, "the whole container went into the bottle");
    TEST_ASSERT_EQ(recoverToolPercent(), 100, "100 %");
    TEST_ASSERT_EQ(gui.page.tools.machineStats.clean, cleans + 1, "counted as a maintenance cycle");
    TEST_ASSERT(alarm_is_active() == gui.page.settings.settingsParams.isPersistentAlarm, "persistent alarm at the end if enabled");
    TEST_ASSERT(!xferBusy(), "pump idle");
    TEST_ASSERT(state_has("\"recoverToolState\":3"), "state JSON: done");

    cmd("{\"cmd\":\"recover_close\"}");
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_IDLE, "IDLE after close");
    TEST_ASSERT(!popup_visible(gui.element.recoverPopup.recoverPopupParent), "popup closed");
    TEST_ASSERT(!alarm_is_active(), "alarm silenced by close");
    real_pump();
    TEST_END();
}

static void test_recover_stop(void)
{
    TEST_BEGIN("Empty C1 — Stop while transferring: tank emptied to the bottle, STOPPED");
    fast_pump();
    cmd("{\"cmd\":\"recover_start\",\"container\":0}");
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_RUNNING, "running");
    /* let it reach the transfer phase */
    for (int t = 0; t < 400 && recoverToolPhase() != RECOVER_PH_XFER; t++) pump_real(100);
    TEST_ASSERT_EQ(recoverToolPhase(), RECOVER_PH_XFER, "transferring");
    pump_real(400);
    cmd("{\"cmd\":\"recover_stop\"}");
    pump_real(1100);                                       /* the popup timer ticks once a second */
    int st = recoverToolState();
    TEST_ASSERT(st == RECOVER_TOOL_STOPPING || st == RECOVER_TOOL_STOPPED, "stopping (draining the tank) or already stopped");
    if (st == RECOVER_TOOL_STOPPING) {
        TEST_ASSERT(state_has("\"recoverToolState\":2"), "state JSON: stopping");
        TEST_ASSERT_EQ((int)xferTo(), RECOVERY_RLY, "...the tank is drained to the bottle");
    }
    TEST_ASSERT(pump_until(recover_ended, 20000), "ends");
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_STOPPED, "STOPPED");
    TEST_ASSERT(!xferBusy(), "pump idle: nothing left in the tank");
    cmd("{\"cmd\":\"recover_close\"}");
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_IDLE, "IDLE after close");
    real_pump();
    TEST_END();
}

static void test_recover_display(void)
{
    TEST_BEGIN("Empty chemistry — from the Tools page: choose C3, Start, Stop, Close");
    fast_pump();
    TEST_ASSERT_NOT_NULL(gui.page.tools.toolsRecoverButton, "Tools page has the Empty chemistry row");
    recoverPopupCreate();                                  /* what the row button does */
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    TEST_ASSERT(popup_visible(rp->recoverPopupParent), "popup open");
    TEST_ASSERT(!lv_obj_has_flag(rp->recoverConfirmContainer, LV_OBJ_FLAG_HIDDEN), "on the confirm page");
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_IDLE, "not running yet");
    TEST_ASSERT_EQ(recoverToolContainer(), -1, "nothing chosen yet");
    TEST_ASSERT(lv_obj_has_state(rp->recoverStartButton, LV_STATE_DISABLED), "Start disabled until a container is chosen");
    TEST_ASSERT_STR_EQ(lv_label_get_text(rp->recoverTitle), recoverPopupTitle_text, "generic title");

    test_click_obj(rp->recoverStartButton);
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_IDLE, "Start does nothing without a choice");
    test_click_obj(rp->pickButton[2]);
    TEST_ASSERT_EQ(recoverToolContainer(), 2, "C3 chosen");
    TEST_ASSERT(lv_obj_has_state(rp->pickButton[2], LV_STATE_CHECKED), "C3 button checked");
    TEST_ASSERT(!lv_obj_has_state(rp->pickButton[0], LV_STATE_CHECKED), "C1 not");
    TEST_ASSERT(!lv_obj_has_state(rp->recoverStartButton, LV_STATE_DISABLED), "Start enabled");
    TEST_ASSERT(strstr(lv_label_get_text(rp->recoverTitle), "C3") != NULL, "title names C3");
    test_click_obj(rp->pickButton[0]);
    TEST_ASSERT_EQ(recoverToolContainer(), 0, "changed mind: C1");
    TEST_ASSERT(!lv_obj_has_state(rp->pickButton[2], LV_STATE_CHECKED), "C3 unchecked");
    test_click_obj(rp->pickButton[2]);

    test_click_obj(rp->recoverStartButton);
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_RUNNING, "Start runs it");
    TEST_ASSERT_EQ(recoverToolContainer(), 2, "...for C3");
    TEST_ASSERT(!lv_obj_has_flag(rp->recoverProcessContainer, LV_OBJ_FLAG_HIDDEN), "process page shown");
    TEST_ASSERT(!lv_obj_has_flag(rp->drop, LV_OBJ_FLAG_HIDDEN), "the drop is moving");
    pump_real(600);
    test_click_obj(rp->recoverStopButton);
    TEST_ASSERT(pump_until(recover_ended, 20000), "Stop ends it");
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_STOPPED, "STOPPED");
    TEST_ASSERT(lv_obj_has_flag(rp->drop, LV_OBJ_FLAG_HIDDEN), "the drop stops");
    test_click_obj(rp->recoverStopButton);                 /* now "Close" */
    TEST_ASSERT(!popup_visible(rp->recoverPopupParent), "closed");
    TEST_ASSERT_EQ(recoverToolState(), RECOVER_TOOL_IDLE, "IDLE");
    real_pump();
    TEST_END();
}

static void test_recover_state_fields(void)
{
    TEST_BEGIN("State JSON — recover / transfer / circulator fields present");
    const char *keys[] = { "recoverToolState", "recoverToolContainer", "recoverToolPhase", "recoverToolPct",
                           "recoverToolRemaining", "recoverToolMovedMl", "xferPhase", "bathCirculator" };
    char k[64];
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        snprintf(k, sizeof(k), "\"%s\":", keys[i]);
        TEST_ASSERT(state_has(k), keys[i]);
    }
    TEST_END();
}

/* ── Bath circulator ─────────────────────────────────────── */

static void test_circulator(void)
{
    TEST_BEGIN("Bath circulator — off by default, on/off, in the state JSON");
    TEST_ASSERT(!circulatorOn(), "off at boot");
    TEST_ASSERT(state_has("\"bathCirculator\":false"), "state: false");
    circulatorSet(true);
    TEST_ASSERT(circulatorOn(), "on");
    TEST_ASSERT(state_has("\"bathCirculator\":true"), "state: true");
    circulatorSet(false);
    TEST_ASSERT(!circulatorOn(), "off again");
    TEST_END();
}

void test_suite_recover(void)
{
    TEST_SUITE("Machine v7: Kamoer timings, transfer engine, Empty chemistry, circulator");
    test_pump_timings();
    test_xfer_engine();
    test_recover_state_fields();
    test_recover_full_run();
    test_recover_stop();
    test_recover_display();
    test_circulator();
}
