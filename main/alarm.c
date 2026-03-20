#include "FilMachine.h"

extern struct gui_components gui;

/*
 * Common alarm implementation for ESP-IDF target.
 * The simulator keeps its own implementation in src/main.c.
 */

static lv_timer_t *s_alarm_timer = NULL;

void buzzer_beep(void)
{
    /*
     * Hardware buzzer implementation is not wired here yet.
     * Keep this as a safe no-op so alarm logic links and runs.
     */
}

static void alarm_timer_cb(lv_timer_t *t)
{
    (void)t;
    buzzer_beep();
}

void alarm_start_persistent(void)
{
    buzzer_beep();

    if (gui.page.settings.settingsParams.isPersistentAlarm)
    {
        if (s_alarm_timer == NULL)
        {
            s_alarm_timer = lv_timer_create(alarm_timer_cb, 10000, NULL);
        }
        else
        {
            lv_timer_resume(s_alarm_timer);
        }
    }
}

void alarm_stop(void)
{
    if (s_alarm_timer != NULL)
    {
        lv_timer_pause(s_alarm_timer);
    }
}

bool alarm_is_active(void)
{
    return (s_alarm_timer != NULL) && (lv_timer_get_paused(s_alarm_timer) == false);
}
