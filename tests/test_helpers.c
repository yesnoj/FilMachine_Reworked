/**
 * test_helpers.c — Touch Simulation & LVGL Pump Helpers
 *
 * Provides functions to simulate touch input by creating a custom
 * lv_indev with a controllable read callback. The test framework
 * controls what coordinates and press/release state the callback reports.
 */

#include "test_runner.h"
#include "lvgl.h"
#include "src/indev/lv_indev_private.h"   /* access gesture_limit / gesture_min_velocity */

/* ═══════════════════════════════════════════════
 * Log File Handle (used by test_printf in test_runner.h)
 * ═══════════════════════════════════════════════ */
FILE *g_log_file = NULL;

/* ═══════════════════════════════════════════════
 * Mock Input Device State
 * ═══════════════════════════════════════════════ */
static lv_indev_t *s_test_indev = NULL;

/** Current simulated pointer state */
static volatile int32_t          s_mouse_x     = 0;
static volatile int32_t          s_mouse_y     = 0;
static volatile lv_indev_state_t s_mouse_state = LV_INDEV_STATE_RELEASED;

/**
 * Read callback for the test input device.
 * LVGL calls this every timer cycle to get the "touch" state.
 */
static void test_indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point.x = s_mouse_x;
    data->point.y = s_mouse_y;
    data->state   = s_mouse_state;
    data->continue_reading = false;
}

/* ═══════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════ */

void test_indev_init(void)
{
    s_test_indev = lv_indev_create();
    lv_indev_set_type(s_test_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_test_indev, test_indev_read_cb);

    /* Lower gesture thresholds to match the simulator settings */
    s_test_indev->gesture_min_distance = 30;
    s_test_indev->gesture_min_velocity = 2;

    test_printf("[TEST] Input device initialized\n");
}

void test_pump(uint32_t ms)
{
    /* Process LVGL timers in small increments (5ms each, like the real main loop) */
    uint32_t elapsed = 0;
    while (elapsed < ms) {
        uint32_t step = (ms - elapsed) >= 5 ? 5 : (ms - elapsed);
        lv_tick_inc(step);
        lv_timer_handler();
        elapsed += step;
    }
}

void test_click(int32_t x, int32_t y)
{
    /* Move to position */
    s_mouse_x = x;
    s_mouse_y = y;
    s_mouse_state = LV_INDEV_STATE_RELEASED;
    test_pump(50);  /* let LVGL see the position (needs ≥1 indev read at 33ms period) */

    /* Press */
    s_mouse_state = LV_INDEV_STATE_PRESSED;
    test_pump(120); /* hold pressed ~120ms — guarantees ≥3 indev reads for reliable click */

    /* Release */
    s_mouse_state = LV_INDEV_STATE_RELEASED;
    test_pump(200); /* process release + any animations/events */
}

void test_click_obj(lv_obj_t *obj)
{
    if (obj == NULL) {
        test_printf("         [WARN] test_click_obj: NULL object — skipping\n");
        return;
    }

    /*
     * Use lv_obj_send_event() to fire CLICKED directly on the LVGL object.
     *
     * Touch-simulation via test_click(x,y) doesn't work reliably because
     * LVGL's indev processing enters "scroll detection mode" when the
     * parent of the target object is scrollable (screens are scrollable by
     * default in LVGL 9).  The scroll-watch logic delays or suppresses
     * click events when the pointer stays still, which is exactly what
     * our simulated touch does.
     *
     * lv_obj_send_event() bypasses indev processing entirely and fires
     * the event on the correct object with the correct user_data (set
     * during lv_obj_add_event_cb registration).  This is the standard
     * approach for LVGL unit/integration tests.
     *
     * Touch-based test_click(x,y) is still available for coordinate-based
     * interactions (popup buttons at hardcoded positions, swipe gestures).
     */
    test_printf("         [DBG] click_obj → send LV_EVENT_CLICKED to %p\n", (void *)obj);
    lv_obj_send_event(obj, LV_EVENT_CLICKED, NULL);
    test_pump(100); /* process any resulting screen loads / animations */
}

void test_long_press(int32_t x, int32_t y, uint32_t duration_ms)
{
    /* Move to position */
    s_mouse_x = x;
    s_mouse_y = y;
    s_mouse_state = LV_INDEV_STATE_RELEASED;
    test_pump(30);

    /* Press and hold */
    s_mouse_state = LV_INDEV_STATE_PRESSED;
    test_pump(duration_ms);

    /* Release */
    s_mouse_state = LV_INDEV_STATE_RELEASED;
    test_pump(100);
}

void test_swipe(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int steps)
{
    if (steps < 2) steps = 2;

    /* Move to start */
    s_mouse_x = x1;
    s_mouse_y = y1;
    s_mouse_state = LV_INDEV_STATE_RELEASED;
    test_pump(30);

    /* Press at start */
    s_mouse_state = LV_INDEV_STATE_PRESSED;
    test_pump(30);

    /* Move incrementally */
    for (int i = 1; i <= steps; i++) {
        s_mouse_x = x1 + (x2 - x1) * i / steps;
        s_mouse_y = y1 + (y2 - y1) * i / steps;
        test_pump(15);  /* ~15ms per step for realistic swipe speed */
    }

    /* Release */
    s_mouse_state = LV_INDEV_STATE_RELEASED;
    test_pump(100);
}

int test_summary(void)
{
    test_printf("\n══════════════════════════════════════\n");
    test_printf("  TEST SUMMARY\n");
    test_printf("══════════════════════════════════════\n");
    test_printf("  Total:  %d\n", g_tests_total);
    test_printf("  Passed: %d\n", g_tests_passed);
    test_printf("  Failed: %d\n", g_tests_failed);
    test_printf("══════════════════════════════════════\n");

    if (g_tests_failed == 0) {
        test_printf("  >>> ALL TESTS PASSED <<<\n");
    } else {
        test_printf("  >>> %d TEST(S) FAILED <<<\n", g_tests_failed);
    }
    test_printf("══════════════════════════════════════\n\n");

    return (g_tests_failed > 0) ? 1 : 0;
}
