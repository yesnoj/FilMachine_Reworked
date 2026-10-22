/**
 * test_mainboard_v12.c — Mainboard v1.2: no chemistry level sensors.
 *
 *  - heater guard: the heaters follow sim_setHeater() only while the bath MIN
 *    sensor reads water (2 s to cut, 5 s to come back), heaterBlocked in the
 *    state JSON;
 *  - "Fill chemistry" by volume: CHEM_CONTAINER_ML of bath water into C1
 *    through the tank, on the transfer engine, Stop included;
 *  - port B: B0-B5 are the spare GPIO of header J30.
 *
 * The guard and the transfer engine run on 1 s LVGL timers that read the real
 * clock, so these tests sleep (pumpSetTestParams makes the pump fast).
 */

#include "test_runner.h"
#include "ws_server.h"
#include "sensors.h"
#include "lvgl.h"
#include <unistd.h>

extern bool g_testHeaterHw;

static void pump_real(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 10) {
        usleep(10000);
        test_pump(10);
    }
}

static bool state_has(const char *needle)
{
    static char buf[8192];
    ws_debug_build_state(buf, sizeof(buf));
    return strstr(buf, needle) != NULL;
}

static void test_heater_guard(void)
{
    TEST_BEGIN("Heater guard — no heating with the bath below MIN");
    xferInit();                                   /* creates the guard timer too (idempotent) */
    sim_setHeater(false);
    sensors_sim_set_water_level(true, false);
    pump_real(6000);                              /* settle: bath reported above MIN */
    sim_setHeater(true);
    TEST_ASSERT(g_testHeaterHw, "bath above MIN: the request turns the heaters on");
    TEST_ASSERT(!heaterBlockedLowWater(), "not blocked");
    TEST_ASSERT(state_has("\"heaterBlocked\":false"), "state JSON: heaterBlocked false");

    sensors_sim_set_water_level(false, false);    /* the bath drops below MIN */
    pump_real(300);                               /* at most one 1 s sample so far */
    TEST_ASSERT(g_testHeaterHw, "one dry sample is not enough (debounce)");
    pump_real(3000);
    TEST_ASSERT(!g_testHeaterHw, "two dry samples (1-2 s): heaters off");
    TEST_ASSERT(heaterBlockedLowWater(), "blocked while heat is still requested");
    TEST_ASSERT(!heaterBathOk(), "bath reported below MIN");
    TEST_ASSERT(state_has("\"heaterBlocked\":true"), "state JSON: heaterBlocked true");

    sim_setHeater(true);                          /* the thermostat insists: still off */
    TEST_ASSERT(!g_testHeaterHw, "a new request while dry keeps them off");

    sensors_sim_set_water_level(true, false);     /* refilled */
    pump_real(2500);                              /* at most 3 wet samples */
    TEST_ASSERT(!g_testHeaterHw, "wet for a short time: still off");
    pump_real(4000);
    TEST_ASSERT(g_testHeaterHw, "five wet samples (4-5 s): the pending request turns them back on");
    TEST_ASSERT(!heaterBlockedLowWater(), "no longer blocked");

    sim_setHeater(false);
    TEST_ASSERT(!g_testHeaterHw, "request off: heaters off");

    TEST_ASSERT(heaterDebugSet(0, true), "diagnostics: heater 1 on by hand, bath above MIN");
    sensors_sim_set_water_level(false, false);
    pump_real(3300);
    TEST_ASSERT(!heaterDebugOn(0), "diagnostics heater cut by the guard when the bath drops");
    TEST_ASSERT(!heaterDebugSet(1, true), "diagnostics: refused with the bath below MIN");
    sensors_sim_set_water_level(true, false);
    pump_real(6000);
    sensors_sim_set_water_level(false, false);
    pump_real(3500);
    TEST_ASSERT(!heaterBlockedLowWater(), "dry bath without a heat request is not a block");
    sensors_sim_set_water_level(true, false);
    pump_real(6000);
    TEST_ASSERT(heaterBathOk(), "back to a wet bath for the other tests");
    TEST_END();
}

static void test_fill_chem_by_volume(void)
{
    TEST_BEGIN("Fill chemistry — CHEM_CONTAINER_ML into C1 by volume, no sensors");
    uint8_t spd = gui.page.settings.settingsParams.pumpSpeed;
    gui.page.settings.settingsParams.pumpSpeed = 100;
    pumpSetTestParams(60000, 1);                  /* 1000 ml/s, 1 s of air */
    for (int t = 0; t < 600 && xferBusy(); t++) pump_real(50);

    machineFillSetTarget(FILL_TARGET_CHEM);
    TEST_ASSERT(!machineFillBathFull(), "C1 has no sensor: never reported full before starting");
    TEST_ASSERT(machineFillManual(), "level-style display (no flow meter)");
    machineFillStart(FILL_TARGET_CHEM);
    TEST_ASSERT_EQ(machineFillState(), FILL_RUNNING, "running");
    TEST_ASSERT(xferBusy(), "on the transfer engine");
    TEST_ASSERT_EQ(xferFrom(), getValueForChemicalSource(WB), "from the bath (WB)");
    TEST_ASSERT_EQ(xferTo(), getValueForChemicalSource(0), "to C1");
    TEST_ASSERT_EQ(xferTargetMl(), CHEM_CONTAINER_ML, "the nominal container volume");
    for (int t = 0; t < 600 && machineFillState() == FILL_RUNNING; t++) pump_real(50);
    TEST_ASSERT_EQ(machineFillState(), FILL_FULL, "ends by itself: FULL");
    TEST_ASSERT_EQ((int)machineFillVolumeMl(), CHEM_CONTAINER_ML, "the whole volume went into C1");
    TEST_ASSERT_EQ(machineFillLevelPct(), 100, "bargraph at 100 %");
    TEST_ASSERT(!xferBusy(), "pump idle");

    machineFillStart(FILL_TARGET_CHEM);           /* again, then Stop */
    pump_real(600);
    machineFillStop();
    for (int t = 0; t < 600 && machineFillState() == FILL_RUNNING; t++) pump_real(50);
    TEST_ASSERT_EQ(machineFillState(), FILL_STOPPED, "Stop: the tank is emptied into C1, then STOPPED");
    TEST_ASSERT(!xferBusy(), "pump idle after Stop");

    machineFillStart(FILL_TARGET_CHEM);           /* a process start aborts it half way */
    pump_real(600);
    xferAbort();
    TEST_ASSERT_EQ(machineFillState(), FILL_STOPPED, "aborted: STOPPED, never 'C1 filled'");

    machineFillSetTarget(FILL_TARGET_WB);
    pumpSetTestParams(0, 0);
    gui.page.settings.settingsParams.pumpSpeed = spd;
    TEST_END();
}

static void test_port_b_spare(void)
{
    TEST_BEGIN("Port B — B0-B5 spare GPIO (J30), B6/B7 heaters");
    TEST_ASSERT_EQ(GPIO_SPARE_B_FIRST, 8, "B0 = MCP pin 8");
    TEST_ASSERT_EQ(GPIO_SPARE_B_COUNT, 6, "six spare pins B0-B5");
    TEST_ASSERT(GPIO_SPARE_B_FIRST + GPIO_SPARE_B_COUNT <= HEATER1_PIN, "spare pins stop before the heaters (B6)");
    TEST_ASSERT_EQ(CHEM_CONTAINER_ML, 500, "v7 containers of 500 ml");
    TEST_END();
}

void test_suite_mainboard_v12(void)
{
    TEST_SUITE("Mainboard v1.2: heater guard, Fill chemistry by volume, spare GPIO");
    test_heater_guard();
    test_fill_chem_by_volume();
    test_port_b_spare();
}
