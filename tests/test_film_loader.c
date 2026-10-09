/**
 * test_film_loader.c — Film loader (Tools → Load film)
 *
 * Drives the loader state machine by hand (filmLoaderTickAt with a synthetic
 * clock) against the simulated reel of film_loader.c: the "reel" produces one
 * Hall pulse every pulseMs while the motor runs and stalls after filmPulses,
 * until the blade has cut the film.
 */

#include "test_runner.h"
#include "lvgl.h"

static uint32_t s_now;
static uint16_t s_maxServo;
static bool     s_sawCutting;

/* Fresh loader, manual ticks, simulated roll of `filmPulses` pulses. */
static void setup(uint16_t filmPulses, uint16_t pulseMs)
{
    filmLoaderSimManualTicks();
    filmLoaderSimReset();
    filmLoaderSimSetup(false, filmPulses, pulseMs);
    s_now = 1000;
    s_maxServo = 0;
    s_sawCutting = false;
}

static void tick(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 20) {
        s_now += 20;
        filmLoaderTickAt(s_now);
        uint16_t us = filmLoaderSimServoUs();
        if (us > s_maxServo) s_maxServo = us;
        if (filmLoaderState() == LOAD_CUTTING) s_sawCutting = true;
    }
}

/* Tick until the load is over (any state that is not winding/cutting/tail). */
static int run_to_end(uint32_t limitMs)
{
    uint32_t start = s_now;
    while (filmLoaderBusy() && s_now - start < limitMs) tick(20);
    tick(2000);          /* let the servo settle and switch off */
    return filmLoaderState();
}

static void test_loader_defaults(void)
{
    TEST_BEGIN("Film loader — default settings");
    struct machineSettings s;
    memset(&s, 0, sizeof(s));
    filmLoaderApplyDefaults(&s);
    TEST_ASSERT_EQ(s.loadSpeed, LOAD_SPEED_DEFAULT, "loadSpeed default");
    TEST_ASSERT_EQ(s.cutterRestUs, LOAD_CUTTER_REST_US_DEFAULT, "cutterRestUs default");
    TEST_ASSERT_EQ(s.cutterCutUs, LOAD_CUTTER_CUT_US_DEFAULT, "cutterCutUs default");
    TEST_ASSERT(gui.page.settings.settingsParams.loadSpeed >= 10, "live settings carry a valid loadSpeed");
    TEST_END();
}

static void test_loader_135_full_cycle(void)
{
    TEST_BEGIN("Film loader — 135: wind, stall, cut, tail, done");
    setup(37, 400);
    TEST_ASSERT(filmLoaderStart(LOAD_FMT_135), "start accepted");
    tick(20);
    TEST_ASSERT_EQ(filmLoaderState(), LOAD_WINDING, "winding after start");
    TEST_ASSERT(filmLoaderSimMotorOn(), "motor on while winding");
    TEST_ASSERT(!filmLoaderStart(LOAD_FMT_120), "second start refused while busy");
    int end = run_to_end(120000);
    TEST_ASSERT_EQ(end, LOAD_DONE, "load ends DONE");
    TEST_ASSERT_EQ(filmLoaderPulses(), 37, "reported film length = pulses before the cut");
    TEST_ASSERT(s_sawCutting, "went through CUTTING");
    TEST_ASSERT(s_maxServo >= LOAD_CUTTER_CUT_US_DEFAULT - 5, "blade reached the cut position");
    TEST_ASSERT_EQ(filmLoaderSimServoUs(), 0, "servo released at the end");
    TEST_ASSERT(!filmLoaderSimMotorOn(), "motor off at the end");
    TEST_ASSERT_EQ(filmLoaderFormat(), LOAD_FMT_135, "format kept");
    TEST_END();
}

static void test_loader_120_full_cycle(void)
{
    TEST_BEGIN("Film loader — 120: shorter roll, done");
    setup(21, 600);
    filmLoaderStart(LOAD_FMT_120);
    int end = run_to_end(120000);
    TEST_ASSERT_EQ(end, LOAD_DONE, "120 ends DONE");
    TEST_ASSERT_EQ(filmLoaderPulses(), 21, "120 length");
    TEST_ASSERT_EQ(filmLoaderExpectedPulses(), 20, "120 expected pulses");
    TEST_END();
}

static void test_loader_early_stall_no_cut(void)
{
    TEST_BEGIN("Film loader — reel stops too early: error, no cut");
    setup(10, 400);
    filmLoaderStart(LOAD_FMT_135);
    int end = run_to_end(60000);
    TEST_ASSERT_EQ(end, LOAD_ERR_EARLY, "early stall reported");
    TEST_ASSERT_EQ(s_maxServo, 0, "blade never moved");
    TEST_ASSERT(!filmLoaderSimMotorOn(), "motor off");
    TEST_END();
}

static void test_loader_no_spin(void)
{
    TEST_BEGIN("Film loader — no Hall pulse at all: motor/belt error");
    setup(37, 60000);                         /* first pulse would come after 60 s */
    filmLoaderStart(LOAD_FMT_135);
    int end = run_to_end(60000);
    TEST_ASSERT_EQ(end, LOAD_ERR_NOSPIN, "no-spin reported");
    TEST_ASSERT(s_now - 1000 < 9000, "detected within a few seconds");
    TEST_END();
}

static void test_loader_too_long(void)
{
    TEST_BEGIN("Film loader — too many turns: error, no cut");
    setup(500, 300);
    filmLoaderStart(LOAD_FMT_135);
    int end = run_to_end(120000);
    TEST_ASSERT_EQ(end, LOAD_ERR_LONG, "too-long reported");
    TEST_ASSERT_EQ(s_maxServo, 0, "blade never moved");
    TEST_END();
}

static void test_loader_manual_cut(void)
{
    TEST_BEGIN("Film loader — Cut now while winding");
    setup(37, 400);
    filmLoaderStart(LOAD_FMT_120);
    tick(4100);                                /* about 10 pulses */
    uint16_t p = filmLoaderPulses();
    TEST_ASSERT(p >= 8 && p <= 11, "some turns wound before the manual cut");
    filmLoaderCutNow();
    tick(20);
    TEST_ASSERT_EQ(filmLoaderState(), LOAD_CUTTING, "cutting right after Cut now");
    int end = run_to_end(30000);
    TEST_ASSERT_EQ(end, LOAD_DONE, "manual cut ends DONE");
    TEST_ASSERT_EQ(filmLoaderPulses(), p, "length = pulses at the moment of the cut");
    TEST_END();
}

static void test_loader_stop(void)
{
    TEST_BEGIN("Film loader — Stop while winding");
    setup(37, 400);
    filmLoaderStart(LOAD_FMT_135);
    tick(3000);
    filmLoaderStop();
    tick(1000);
    TEST_ASSERT_EQ(filmLoaderState(), LOAD_STOPPED, "stopped");
    TEST_ASSERT(!filmLoaderSimMotorOn(), "motor off");
    TEST_ASSERT(!filmLoaderBusy(), "not busy");
    TEST_END();
}

static void test_loader_stop_during_cut(void)
{
    TEST_BEGIN("Film loader — Stop during the cut: blade back to rest");
    setup(37, 400);
    filmLoaderStart(LOAD_FMT_135);
    tick(2000);
    filmLoaderCutNow();
    tick(500);                                 /* blade half way up */
    TEST_ASSERT_EQ(filmLoaderState(), LOAD_CUTTING, "cutting");
    filmLoaderStop();
    tick(40);
    TEST_ASSERT_EQ(filmLoaderSimServoUs(), LOAD_CUTTER_REST_US_DEFAULT, "blade commanded back to rest");
    tick(2000);
    TEST_ASSERT_EQ(filmLoaderSimServoUs(), 0, "servo released");
    TEST_ASSERT_EQ(filmLoaderState(), LOAD_STOPPED, "stopped");
    TEST_END();
}

static void test_loader_cutter_test(void)
{
    TEST_BEGIN("Film loader — cutter test holds the angle, then releases");
    setup(37, 400);
    filmLoaderCutterTest(90);
    tick(200);
    uint16_t mid = (LOAD_CUTTER_REST_US_DEFAULT + LOAD_CUTTER_CUT_US_DEFAULT) / 2;
    TEST_ASSERT_EQ(filmLoaderSimServoUs(), mid, "90 deg = middle pulse");
    TEST_ASSERT(!filmLoaderSimMotorOn(), "motor stays off");
    tick(6000);
    TEST_ASSERT_EQ(filmLoaderSimServoUs(), 0, "released after the hold time");
    filmLoaderCutterCycle();
    tick(400);
    TEST_ASSERT(filmLoaderSimServoUs() > LOAD_CUTTER_REST_US_DEFAULT, "test cycle raises the blade");
    TEST_ASSERT(!filmLoaderSimMotorOn(), "test cycle: motor off");
    tick(3000);
    TEST_ASSERT_EQ(filmLoaderSimServoUs(), 0, "test cycle ends released");
    TEST_ASSERT_EQ(filmLoaderState(), LOAD_IDLE, "state untouched by the test cycle");
    TEST_END();
}

static void test_loader_popup(void)
{
    TEST_BEGIN("Film loader — popup opens from the app command");
    setup(37, 400);
    struct sLoadPopup *lp = &gui.element.loadPopup;
    loadPopupRemoteStart(LOAD_FMT_120);
    test_pump(100);
    TEST_ASSERT_NOT_NULL(lp->parent, "popup open");
    TEST_ASSERT_EQ(filmLoaderFormat(), LOAD_FMT_120, "format from the app");
    TEST_ASSERT(filmLoaderBusy(), "loading started");
    loadPopupRemoteStop();
    tick(100);
    TEST_ASSERT(!filmLoaderBusy(), "stopped from the app");
    if (lp->liveTimer) { lv_timer_delete(lp->liveTimer); lp->liveTimer = NULL; }
    lv_style_reset(&lp->style_titleLine);
    lv_style_reset(&lp->style_barIndic);
    lv_msgbox_close(lp->parent);
    lp->parent = NULL;
    test_pump(100);
    TEST_END();
}

void test_suite_film_loader(void)
{
    TEST_SUITE("Film loader");
    test_loader_defaults();
    test_loader_135_full_cycle();
    test_loader_120_full_cycle();
    test_loader_early_stall_no_cut();
    test_loader_no_spin();
    test_loader_too_long();
    test_loader_manual_cut();
    test_loader_stop();
    test_loader_stop_during_cut();
    test_loader_cutter_test();
    test_loader_popup();
    filmLoaderSimReset();
}
