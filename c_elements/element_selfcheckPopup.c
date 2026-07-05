
/**
 * @file element_selfcheckPopup.c
 *
 * Self-check Diagnostic Process – guided hardware diagnostics.
 *
 * Logic:
 *  - "Start" begins current phase. Shows "Re-run" after completion.
 *  - "Stop" halts current phase. State is saved (shown as grey).
 *  - "Advance" moves to next phase. Disabled while running.
 *  - "X" (green, top-right) is the ONLY way to close (resets all).
 *  - Clicking a phase name in the left panel re-selects it (no auto-start).
 *  - Only one phase at a time.
 *  - "Self-check complete!" only when ALL phases are done/skipped/stopped.
 */
#include <string.h>
#include "FilMachine.h"
#include "esp_log.h"

static const char *TAG = "SelfCheck";

extern struct gui_components gui;

/* ── Phase configuration ─────────────────────────────────── */
#define SC_NUM_PHASES 8

#define SC_PENDING   0
#define SC_RUNNING   1
#define SC_DONE      2
#define SC_SKIPPED   3
#define SC_STOPPED   4

static const char *phaseNames[] = {
    "Temp. sensors", "Water pump", "Heater", "Valves",
    "Container C1", "Container C2", "Container C3",
    "Agitation motor"
};

static const char *phaseDescriptions[] = {
    "Reading water bath and\nchemistry temperature sensors.",
    "Running water recirculation\npump. Verify water flows.",
    "Heating test: verify the\ntemperature is rising.",
    "Opening/closing each valve\nsequentially. Listen for clicks.",
    "Pumping water through C1.\nVerify flow visually.",
    "Pumping water through C2.\nVerify flow visually.",
    "Pumping water through C3.\nVerify flow visually.",
    "Spinning motor forward then\nreverse. Verify rotation."
};

static const int32_t phaseDurations[] = { 5, 10, 30, 10, 10, 10, 10, 10 };

static uint8_t phaseState[SC_NUM_PHASES];
static char    phaseSavedStatus[SC_NUM_PHASES][40]; /* saved status text per phase */

/* ── Forward declarations ─────────────────────────────────── */
static void sc_timer_cb(lv_timer_t *timer);
static void sc_start_phase(void);
static void sc_complete_phase(void);
static void sc_select_phase(uint8_t phase);
static void sc_reset(void);
static void sc_update_left_icons(void);
static void sc_update_buttons(void);

/* ── Color for stopped state (grey) ────────────────────── */
#define SC_STOPPED_COLOR 0x888888

/* ── Valve relay indices for Self Check valve test ───── */
static const uint8_t sc_valve_relays[] = { C1_RLY, C2_RLY, C3_RLY, WB_RLY, WASTE_RLY };
#define SC_NUM_VALVES 5
static int8_t sc_last_valve = -1;  /* tracks which valve is currently open (-1 = none) */

/* ── Stop all hardware actuators (valves + pump) ──────── */
static void sc_stop_all_hw(void) {
    closeAllValves();
    cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
    pump_set_stop();
    motor_set_stop();
    sc_last_valve = -1;
    ESP_LOGI(TAG, "All HW stopped (valves closed, pump off, motor off)");
}

/* ── Check if all phases are non-pending ────────────────── */
static bool sc_all_finished(void) {
    for (int i = 0; i < SC_NUM_PHASES; i++)
        if (phaseState[i] == SC_PENDING) return false;
    return true;
}

/* ── Check if ALL phases are SC_DONE ────────────────────── */
static bool sc_all_done(void) {
    for (int i = 0; i < SC_NUM_PHASES; i++)
        if (phaseState[i] != SC_DONE) return false;
    return true;
}

/* ── Show completion message (green if all done, orange if mixed) ── */
static void sc_show_finish_message(void) {
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    if (sc_all_done()) {
        lv_label_set_text(sc->phaseTitle, selfCheckComplete_text);
        lv_obj_set_style_text_color(sc->phaseTitle, lv_color_hex(GREEN), 0);
    } else {
        lv_label_set_text(sc->phaseTitle, selfCheckFinished_text);
        lv_obj_set_style_text_color(sc->phaseTitle, lv_color_hex(ORANGE), 0);
    }
}

/* ── Update left panel icons ────────────────────────────── */
static void sc_update_left_icons(void) {
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    for (int i = 0; i < SC_NUM_PHASES; i++) {
        uint32_t color;
        const char *icon;

        if (i == sc->currentPhase && sc->isRunning) {
            /* Currently running: blue arrow */
            icon = arrowStep_icon; color = LIGHT_BLUE;
        } else if (i == sc->currentPhase && phaseState[i] == SC_PENDING) {
            /* Current phase, pending: white arrow */
            icon = arrowStep_icon; color = WHITE;
        } else {
            /* All other cases: show actual state */
            switch (phaseState[i]) {
                case SC_DONE:    icon = checkStep_icon; color = GREEN; break;
                case SC_SKIPPED: icon = dotStep_icon;   color = ORANGE; break;
                case SC_STOPPED: icon = dotStep_icon;   color = SC_STOPPED_COLOR; break;
                default:         icon = dotStep_icon;   color = WHITE; break;
            }
        }

        lv_label_set_text(sc->phaseIcon[i], icon);
        lv_obj_set_style_text_color(sc->phaseIcon[i], lv_color_hex(color), 0);
        lv_obj_set_style_text_color(sc->phaseNameLabel[i], lv_color_hex(color), 0);
    }
}

/* ── Update button states ───────────────────────────────── */
static void sc_update_buttons(void) {
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    uint8_t st = phaseState[sc->currentPhase];

    if (sc->isRunning) {
        /* Running: Stop enabled, Start disabled, Advance disabled, X disabled */
        lv_obj_clear_state(sc->stopButton, LV_STATE_DISABLED);
        lv_obj_add_state(sc->startButton, LV_STATE_DISABLED);
        lv_obj_add_state(sc->advanceButton, LV_STATE_DISABLED);
        lv_obj_add_state(sc->closeButton, LV_STATE_DISABLED);
    } else {
        /* Not running: Stop disabled (nothing to stop), Start enabled, X enabled */
        lv_obj_add_state(sc->stopButton, LV_STATE_DISABLED);
        lv_obj_clear_state(sc->closeButton, LV_STATE_DISABLED);

        /* Hide Next on last phase, show otherwise */
        if (sc->currentPhase >= SC_NUM_PHASES - 1) {
            lv_obj_add_flag(sc->advanceButton, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(sc->advanceButton, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(sc->advanceButton, LV_STATE_DISABLED);
        }

        if (st == SC_DONE || st == SC_SKIPPED || st == SC_STOPPED) {
            lv_label_set_text(sc->startButtonLabel, selfCheckRerun_text);
            lv_obj_clear_state(sc->startButton, LV_STATE_DISABLED);
        } else {
            lv_label_set_text(sc->startButtonLabel, buttonStart_text);
            lv_obj_clear_state(sc->startButton, LV_STATE_DISABLED);
        }
    }
}

/* ── Select a phase (show info, do NOT auto-start) ──────── */
static void sc_select_phase(uint8_t phase) {
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    sc->currentPhase = phase;

    /* Title and description */
    lv_label_set_text(sc->phaseTitle, phaseNames[phase]);
    lv_obj_set_style_text_color(sc->phaseTitle, lv_color_hex(WHITE), 0);
    lv_label_set_text(sc->phaseDescription, phaseDescriptions[phase]);

    /* Restore saved status for this phase */
    uint8_t st = phaseState[phase];
    if (st == SC_DONE) {
        lv_label_set_text(sc->phaseStatus, phaseSavedStatus[phase]);
        lv_obj_set_style_text_color(sc->phaseStatus, lv_color_hex(GREEN), 0);
    } else if (st == SC_STOPPED) {
        lv_label_set_text(sc->phaseStatus, selfCheckStopped_text);
        lv_obj_set_style_text_color(sc->phaseStatus, lv_color_hex(SC_STOPPED_COLOR), 0);
    } else if (st == SC_SKIPPED) {
        lv_label_set_text(sc->phaseStatus, selfCheckSkipped_text);
        lv_obj_set_style_text_color(sc->phaseStatus, lv_color_hex(ORANGE), 0);
    } else {
        lv_label_set_text(sc->phaseStatus, "");
        lv_obj_set_style_text_color(sc->phaseStatus, lv_color_hex(WHITE), 0);
    }
    lv_label_set_text(sc->phaseTimer, "");

    /* Progress bar: hidden until task starts. Show filled if done. */
    if (st == SC_DONE) {
        lv_obj_remove_flag(sc->progressBar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(sc->progressBar, 100, LV_ANIM_OFF);
    } else {
        lv_obj_add_flag(sc->progressBar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(sc->progressBar, 0, LV_ANIM_OFF);
    }

    /* Check if all finished */
    if (sc_all_finished()) {
        sc_show_finish_message();
    }

    sc_update_left_icons();
    sc_update_buttons();
}

/* ── Reset all state ────────────────────────────────────── */
static void sc_reset(void) {
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    if (sc->isRunning) {
        safeTimerDelete(&sc->checkTimer);
        sc->isRunning = false;
    }
    sc_stop_all_hw();   /* Safety: ensure all valves closed + pump off on reset */
    memset(phaseState, SC_PENDING, sizeof(phaseState));
    memset(phaseSavedStatus, 0, sizeof(phaseSavedStatus));
    sc->currentPhase = 0;
    sc->phaseElapsed = 0;
    sc_select_phase(0);
}

/* ── Timer callback ─────────────────────────────────────── */
static void sc_timer_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    if (!sc->isRunning) return;

    sc->phaseElapsed++;
    uint8_t phase = sc->currentPhase;
    int32_t dur = phaseDurations[phase];
    int32_t rem = dur - sc->phaseElapsed;
    if (rem < 0) rem = 0;

    lv_label_set_text_fmt(sc->phaseTimer, selfCheckTimeFmt_text, (long)rem);

    /* Progress bar for ALL phases */
    {
        int32_t pct = (sc->phaseElapsed * 100) / dur;
        if (pct > 100) pct = 100;
        lv_bar_set_value(sc->progressBar, pct, LV_ANIM_ON);
    }

    /* Phase-specific status */
    switch (phase) {
        case 0: {
            /* Real DS18B20 readings (cached, non-blocking). "--" if a sensor is
             * absent / not reading. */
            float wt = getCachedTemperature(TEMPERATURE_SENSOR_BATH);
            float ct = getCachedTemperature(TEMPERATURE_SENSOR_CHEMICAL);
            char wb[10], cb[10];
            if (wt > -100.0f) snprintf(wb, sizeof(wb), "%.1f C", wt); else snprintf(wb, sizeof(wb), "--");
            if (ct > -100.0f) snprintf(cb, sizeof(cb), "%.1f C", ct); else snprintf(cb, sizeof(cb), "--");
            lv_label_set_text_fmt(sc->phaseStatus, "Water: %s\nChemistry: %s", wb, cb);
            snprintf(phaseSavedStatus[phase], sizeof(phaseSavedStatus[phase]),
                     "Water: %s\nChemistry: %s", wb, cb);
            break;
        }
        case 1: {
            /* Pump test: 1-4s forward, 5s pause, 6-9s reverse, 10s done */
            int sub = sc->phaseElapsed;
            #define SC_PUMP_TEST_DUTY 180  /* ~70% speed for test */
            if (sub <= 4) {
                if (sub == 1) {
                    pump_set_forward(SC_PUMP_TEST_DUTY);
                    ESP_LOGI(TAG, "Pump test: FORWARD at duty %d", SC_PUMP_TEST_DUTY);
                }
                lv_label_set_text(sc->phaseStatus, "Pump forward...");
            } else if (sub == 5) {
                pump_set_stop();
                lv_label_set_text(sc->phaseStatus, "Pausing...");
                ESP_LOGI(TAG, "Pump test: PAUSE");
            } else if (sub <= 9) {
                if (sub == 6) {
                    pump_set_reverse(SC_PUMP_TEST_DUTY);
                    ESP_LOGI(TAG, "Pump test: REVERSE at duty %d", SC_PUMP_TEST_DUTY);
                }
                lv_label_set_text(sc->phaseStatus, "Pump reverse...");
            } else {
                pump_set_stop();
                lv_label_set_text(sc->phaseStatus, "Pump OK");
                ESP_LOGI(TAG, "Pump test: STOPPED");
            }
            snprintf(phaseSavedStatus[phase], sizeof(phaseSavedStatus[phase]), "Pump OK");
            break;
        }
        case 2: {
            /* Heater phase: show the real bath temperature. (Actual heater
             * actuation + rising-check comes with the 2-MOSFET heater work.) */
            float t = getCachedTemperature(TEMPERATURE_SENSOR_BATH);
            char tb[12];
            if (t > -100.0f) snprintf(tb, sizeof(tb), "%.1f C", t); else snprintf(tb, sizeof(tb), "--");
            lv_label_set_text_fmt(sc->phaseStatus, "Water bath: %s", tb);
            snprintf(phaseSavedStatus[phase], sizeof(phaseSavedStatus[phase]), "Water bath: %s", tb);
            break;
        }
        case 3: {
            const char *valves[] = {"C1", "C2", "C3", "WB", "WASTE"};
            int vi = sc->phaseElapsed / 2;
            if (vi >= SC_NUM_VALVES) vi = SC_NUM_VALVES - 1;
            lv_label_set_text_fmt(sc->phaseStatus, selfCheckValveFmt_text, valves[vi]);
            snprintf(phaseSavedStatus[phase], sizeof(phaseSavedStatus[phase]), "All valves OK");

            /* ── Actually toggle valves via public API ── */
            if (vi != sc_last_valve) {
                /* Close previous valve */
                if (sc_last_valve >= 0 && sc_last_valve < SC_NUM_VALVES) {
                    setValveState(sc_valve_relays[sc_last_valve], false);
                    ESP_LOGI(TAG, "Valve %s OFF (relay %d)", valves[sc_last_valve], sc_valve_relays[sc_last_valve]);
                }
                /* Open new valve */
                setValveState(sc_valve_relays[vi], true);
                sc_last_valve = vi;
                ESP_LOGI(TAG, "Valve %s ON  (relay %d)", valves[vi], sc_valve_relays[vi]);
            }
            break;
        }
        case 4: case 5: case 6: {
            int sub = sc->phaseElapsed;
            /* Map phase 4→C1, 5→C2, 6→C3 */
            uint8_t containerSrc = (phase == 4) ? C1 : (phase == 5) ? C2 : C3;
            const char *cName    = (phase == 4) ? "C1" : (phase == 5) ? "C2" : "C3";
            const char *msg;

            if (sub <= 3) {
                msg = "Filling from WB...";
                /* Pump WB → Container (forward) */
                if (sub == 1) {
                    cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
                    cleanRelayManager(getValueForChemicalSource(WB),
                                      getValueForChemicalSource(containerSrc),
                                      PUMP_IN_RLY, true);
                    ESP_LOGI(TAG, "%s: pumping WB → %s (fill)", cName, cName);
                }
            } else if (sub <= 6) {
                msg = "Draining back to WB...";
                /* Pump Container → WB (reverse) */
                if (sub == 4) {
                    cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
                    cleanRelayManager(getValueForChemicalSource(containerSrc),
                                      getValueForChemicalSource(WB),
                                      PUMP_OUT_RLY, true);
                    ESP_LOGI(TAG, "%s: pumping %s → WB (drain back)", cName, cName);
                }
            } else {
                msg = "Draining to waste...";
                /* Pump WB → Waste */
                if (sub == 7) {
                    cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
                    cleanRelayManager(getValueForChemicalSource(WB),
                                      getValueForChemicalSource(WASTE),
                                      PUMP_IN_RLY, true);
                    ESP_LOGI(TAG, "%s: pumping WB → WASTE", cName);
                }
            }
            lv_label_set_text(sc->phaseStatus, msg);
            snprintf(phaseSavedStatus[phase], sizeof(phaseSavedStatus[phase]), "Flow OK");
            break;
        }
        case 7: {
            /* Agitation motor test: 1-4s forward, 5s pause, 6-9s reverse, 10s done */
            int sub = sc->phaseElapsed;
            #define SC_MOTOR_TEST_DUTY 180  /* ~70% speed for test */
            if (sub <= 4) {
                if (sub == 1) {
                    motor_set_forward(SC_MOTOR_TEST_DUTY);
                    ESP_LOGI(TAG, "Motor test: FORWARD at duty %d", SC_MOTOR_TEST_DUTY);
                }
                lv_label_set_text(sc->phaseStatus, "Forward...");
            } else if (sub == 5) {
                motor_set_stop();
                lv_label_set_text(sc->phaseStatus, "Pausing...");
                ESP_LOGI(TAG, "Motor test: PAUSE");
            } else if (sub <= 9) {
                if (sub == 6) {
                    motor_set_reverse(SC_MOTOR_TEST_DUTY);
                    ESP_LOGI(TAG, "Motor test: REVERSE at duty %d", SC_MOTOR_TEST_DUTY);
                }
                lv_label_set_text(sc->phaseStatus, "Reverse...");
            } else {
                motor_set_stop();
                lv_label_set_text(sc->phaseStatus, "Motor OK");
                ESP_LOGI(TAG, "Motor test: STOPPED");
            }
            snprintf(phaseSavedStatus[phase], sizeof(phaseSavedStatus[phase]), "Motor OK");
            break;
        }
    }

    if (sc->phaseElapsed >= dur) {
        sc_complete_phase();
    }
}

/* ── Start current phase ────────────────────────────────── */
static void sc_start_phase(void) {
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    sc->phaseElapsed = 0;
    sc->isRunning = true;
    phaseState[sc->currentPhase] = SC_RUNNING;

    lv_label_set_text(sc->phaseStatus, selfCheckRunning_text);
    lv_obj_set_style_text_color(sc->phaseStatus, lv_color_hex(WHITE), 0);

    /* Show progress bar for ALL phases when running */
    lv_obj_remove_flag(sc->progressBar, LV_OBJ_FLAG_HIDDEN);
    lv_bar_set_value(sc->progressBar, 0, LV_ANIM_OFF);

    sc_update_left_icons();
    sc_update_buttons();

    sc->checkTimer = lv_timer_create(sc_timer_cb, 1000, NULL);
    LV_LOG_USER("Self-check: Started phase %d (%s)", sc->currentPhase, phaseNames[sc->currentPhase]);
}

/* ── Complete current phase ─────────────────────────────── */
static void sc_complete_phase(void) {
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    safeTimerDelete(&sc->checkTimer);
    sc->isRunning = false;
    phaseState[sc->currentPhase] = SC_DONE;

    /* Stop any hardware that was running in this phase */
    sc_stop_all_hw();

    /* Show Done immediately with green */
    lv_label_set_text(sc->phaseStatus, selfCheckDone_text);
    lv_obj_set_style_text_color(sc->phaseStatus, lv_color_hex(GREEN), 0);
    lv_label_set_text(sc->phaseTimer, "");

    lv_bar_set_value(sc->progressBar, 100, LV_ANIM_ON);

    /* Update icons immediately so green check shows */
    sc_update_left_icons();
    sc_update_buttons();

    /* Check if all finished */
    if (sc_all_finished()) {
        sc_show_finish_message();
    }

    ESP_LOGI(TAG, "Phase %d (%s) complete", sc->currentPhase,
             phaseNames[sc->currentPhase]);
}

/* ══════════════════════════════════════════════════════════
 *  EVENT HANDLER
 * ══════════════════════════════════════════════════════════ */
void event_selfcheckPopup(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;

    if (code != LV_EVENT_CLICKED) return;

    /* ── X close button: always close + reset ── */
    if (obj == sc->closeButton) {
        sc_reset();
        lv_obj_add_flag(sc->selfcheckPopupParent, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    /* ── Stop: only stops running task ── */
    if (obj == sc->stopButton) {
        if (sc->isRunning) {
            safeTimerDelete(&sc->checkTimer);
            sc->isRunning = false;
            sc_stop_all_hw();   /* Close valves, stop pump */
            phaseState[sc->currentPhase] = SC_STOPPED;
            lv_label_set_text(sc->phaseStatus, selfCheckStopped_text);
            lv_obj_set_style_text_color(sc->phaseStatus, lv_color_hex(SC_STOPPED_COLOR), 0);
            lv_label_set_text(sc->phaseTimer, "");
            sc_update_left_icons();
            sc_update_buttons();
            ESP_LOGW(TAG, "Phase %d STOPPED by user", sc->currentPhase);
        }
        return;
    }

    /* ── Start / Re-run ── */
    if (obj == sc->startButton) {
        if (!sc->isRunning) {
            sc_start_phase();
        }
        return;
    }

    /* ── Advance: move to next phase (only when not running) ── */
    if (obj == sc->advanceButton) {
        if (sc->isRunning) return; /* blocked while running */

        /* If current phase is still pending, mark as skipped */
        if (phaseState[sc->currentPhase] == SC_PENDING)
            phaseState[sc->currentPhase] = SC_SKIPPED;

        /* Find next phase */
        if (sc->currentPhase < SC_NUM_PHASES - 1) {
            sc_select_phase(sc->currentPhase + 1);
        } else {
            /* Already at last phase */
            sc_update_left_icons();
            if (sc_all_finished()) {
                sc_show_finish_message();
                lv_label_set_text(sc->phaseDescription, "");
                lv_label_set_text(sc->phaseStatus, "");
                lv_label_set_text(sc->phaseTimer, "");
                lv_obj_add_flag(sc->progressBar, LV_OBJ_FLAG_HIDDEN);
            }
            sc_update_buttons();
        }
        return;
    }

    /* ── Click on phase name: re-select (only if not running) ── */
    if (!sc->isRunning) {
        for (int i = 0; i < SC_NUM_PHASES; i++) {
            if (obj == sc->phaseNameLabel[i] || obj == sc->phaseIcon[i]) {
                sc_select_phase(i);
                return;
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════
 *  UI CREATION
 * ══════════════════════════════════════════════════════════ */
void selfcheckPopupCreate(void) {
    const ui_selfcheck_popup_layout_t *ui = &ui_get_profile()->selfcheck_popup;
    struct sSelfcheckPopup *sc = &gui.element.selfcheckPopup;
    if (sc->selfcheckPopupParent != NULL) return;

    createPopupBackdrop(&sc->selfcheckPopupParent, &sc->selfcheckContainer, ui_get_profile()->popups.selfcheck_w, ui_get_profile()->popups.selfcheck_h);

    /* Title */
    sc->selfcheckTitle = lv_label_create(sc->selfcheckContainer);
    lv_label_set_text(sc->selfcheckTitle, selfCheck_text);
    lv_obj_set_style_text_font(sc->selfcheckTitle, ui->title_font, 0);
    lv_obj_align(sc->selfcheckTitle, LV_ALIGN_TOP_MID, ui->title_x, ui->title_y);

    /* White underline */
    initTitleLineStyle(&sc->style_selfcheckTitleLine, WHITE);
    sc->selfcheckTitleLine = lv_line_create(sc->selfcheckContainer);
    lv_line_set_points(sc->selfcheckTitleLine, sc->titleLinePoints, 2);
    lv_obj_add_style(sc->selfcheckTitleLine, &sc->style_selfcheckTitleLine, 0);
    lv_obj_align(sc->selfcheckTitleLine, LV_ALIGN_TOP_MID, ui->title_line_x, ui->title_line_y);

    /* X close button (green, top-right) */
    sc->closeButton = lv_button_create(sc->selfcheckContainer);
    lv_obj_set_size(sc->closeButton, ui->close_w, ui->close_h);
    lv_obj_align(sc->closeButton, LV_ALIGN_TOP_RIGHT, ui->close_x, ui->close_y);
    lv_obj_add_event_cb(sc->closeButton, event_selfcheckPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(sc->closeButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
    lv_obj_move_foreground(sc->closeButton);

    sc->closeButtonLabel = lv_label_create(sc->closeButton);
    lv_label_set_text(sc->closeButtonLabel, closePopup_icon);
    lv_obj_set_style_text_font(sc->closeButtonLabel, ui->close_icon_font, 0);
    lv_obj_align(sc->closeButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* ── LEFT PANEL ── */
    sc->leftPanel = lv_obj_create(sc->selfcheckContainer);
    lv_obj_set_pos(sc->leftPanel, ui->left_panel_x, ui->left_panel_y);
    lv_obj_set_size(sc->leftPanel, ui->left_panel_w, ui->left_panel_h);
    lv_obj_remove_flag(sc->leftPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_opa(sc->leftPanel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(sc->leftPanel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(sc->leftPanel, 0, 0);

    sc->tasksLabel = lv_label_create(sc->leftPanel);
    lv_label_set_text(sc->tasksLabel, selfCheckTasks_text);
    lv_obj_set_style_text_font(sc->tasksLabel, ui->phase_title_font, 0);
    lv_obj_set_pos(sc->tasksLabel, ui->tasks_label_x, ui->tasks_label_y);

    for (int i = 0; i < SC_NUM_PHASES; i++) {
        int y = ui->phase_row_y + (i * ui->phase_row_gap);
        sc->phaseNameLabel[i] = lv_label_create(sc->leftPanel);
        lv_label_set_text(sc->phaseNameLabel[i], phaseNames[i]);
        lv_obj_set_style_text_font(sc->phaseNameLabel[i], ui->phase_name_font, 0);
        lv_obj_set_style_text_color(sc->phaseNameLabel[i], lv_color_hex(WHITE), 0);
        lv_obj_set_pos(sc->phaseNameLabel[i], ui->phase_name_x, y);
        lv_obj_add_flag(sc->phaseNameLabel[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(sc->phaseNameLabel[i], event_selfcheckPopup, LV_EVENT_CLICKED, NULL);

        sc->phaseIcon[i] = lv_label_create(sc->leftPanel);
        lv_label_set_text(sc->phaseIcon[i], dotStep_icon);
        lv_obj_set_style_text_font(sc->phaseIcon[i], ui->phase_icon_font, 0);
        lv_obj_set_style_text_color(sc->phaseIcon[i], lv_color_hex(WHITE), 0);
        lv_obj_align_to(sc->phaseIcon[i], sc->phaseNameLabel[i], LV_ALIGN_OUT_LEFT_MID, -6, 0);
        lv_obj_add_flag(sc->phaseIcon[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(sc->phaseIcon[i], event_selfcheckPopup, LV_EVENT_CLICKED, NULL);
    }

    /* ── RIGHT PANEL ── */
    sc->rightPanel = lv_obj_create(sc->selfcheckContainer);
    lv_obj_set_pos(sc->rightPanel, ui->right_panel_x, ui->right_panel_y);
    lv_obj_set_size(sc->rightPanel, ui->right_panel_w, ui->right_panel_h);
    lv_obj_remove_flag(sc->rightPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_opa(sc->rightPanel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(sc->rightPanel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(sc->rightPanel, 0, 0);

    sc->phaseTitle = lv_label_create(sc->rightPanel);
    lv_label_set_text(sc->phaseTitle, phaseNames[0]);
    lv_obj_set_style_text_font(sc->phaseTitle, ui->phase_title_font, 0);
    lv_obj_set_width(sc->phaseTitle, ui->phase_title_w);
    lv_label_set_long_mode(sc->phaseTitle, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(sc->phaseTitle, ui->phase_title_x, ui->phase_title_y);

    sc->phaseDescription = lv_label_create(sc->rightPanel);
    lv_label_set_text(sc->phaseDescription, phaseDescriptions[0]);
    lv_obj_set_style_text_font(sc->phaseDescription, ui->phase_name_font, 0);
    lv_obj_set_width(sc->phaseDescription, ui->phase_desc_w);
    lv_label_set_long_mode(sc->phaseDescription, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(sc->phaseDescription, ui->phase_desc_x, ui->phase_desc_y);

    sc->phaseStatus = lv_label_create(sc->rightPanel);
    lv_label_set_text(sc->phaseStatus, "");
    lv_obj_set_style_text_font(sc->phaseStatus, ui->tasks_font, 0);
    lv_obj_set_width(sc->phaseStatus, ui->phase_status_w);
    lv_label_set_long_mode(sc->phaseStatus, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(sc->phaseStatus, ui->phase_status_x, ui->phase_status_y);

    sc->phaseTimer = lv_label_create(sc->rightPanel);
    lv_label_set_text(sc->phaseTimer, "");
    lv_obj_set_style_text_font(sc->phaseTimer, ui->tasks_font, 0);
    lv_obj_set_pos(sc->phaseTimer, ui->phase_timer_x, ui->phase_timer_y);

    /* Progress bar (thick, green, hidden by default) */
    sc->progressBar = lv_bar_create(sc->rightPanel);
    lv_obj_set_size(sc->progressBar, ui->progress_bar_w, ui->progress_bar_h);
    lv_obj_set_pos(sc->progressBar, ui->progress_bar_x, ui->progress_bar_y);
    lv_bar_set_range(sc->progressBar, 0, 100);
    lv_bar_set_value(sc->progressBar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sc->progressBar, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sc->progressBar, lv_color_hex(GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_radius(sc->progressBar, ui_get_profile()->selfcheck_progress_radius, LV_PART_MAIN);
    lv_obj_set_style_radius(sc->progressBar, ui_get_profile()->selfcheck_progress_radius, LV_PART_INDICATOR);
    lv_obj_add_flag(sc->progressBar, LV_OBJ_FLAG_HIDDEN);

    /* ── 3 BUTTONS IN ONE ROW ── */
    
    /* Stop (red) */
    sc->stopButton = lv_button_create(sc->rightPanel);
    lv_obj_set_size(sc->stopButton, ui->control_btn_w, ui->control_btn_h);
    lv_obj_set_pos(sc->stopButton, ui->stop_button_x, ui->control_btn_y);
    lv_obj_add_event_cb(sc->stopButton, event_selfcheckPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(sc->stopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);

    sc->stopButtonLabel = lv_label_create(sc->stopButton);
    lv_label_set_text(sc->stopButtonLabel, buttonStop_text);
    lv_obj_set_style_text_font(sc->stopButtonLabel, ui->control_btn_font, 0);
    lv_obj_align(sc->stopButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* Start (green) */
    sc->startButton = lv_button_create(sc->rightPanel);
    lv_obj_set_size(sc->startButton, ui->control_btn_w, ui->control_btn_h);
    lv_obj_set_pos(sc->startButton, ui->start_button_x, ui->control_btn_y);
    lv_obj_add_event_cb(sc->startButton, event_selfcheckPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(sc->startButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);

    sc->startButtonLabel = lv_label_create(sc->startButton);
    lv_label_set_text(sc->startButtonLabel, buttonStart_text);
    lv_obj_set_style_text_font(sc->startButtonLabel, ui->control_btn_font, 0);
    lv_obj_align(sc->startButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* Advance (light blue) */
    sc->advanceButton = lv_button_create(sc->rightPanel);
    lv_obj_set_size(sc->advanceButton, ui->control_btn_w, ui->control_btn_h);
    lv_obj_set_pos(sc->advanceButton, ui->advance_button_x, ui->control_btn_y);
    lv_obj_add_event_cb(sc->advanceButton, event_selfcheckPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(sc->advanceButton, lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);

    sc->advanceButtonLabel = lv_label_create(sc->advanceButton);
    lv_label_set_text(sc->advanceButtonLabel, selfCheckNext_text);
    lv_obj_set_style_text_font(sc->advanceButtonLabel, ui->advance_button_font, 0);
    lv_obj_align(sc->advanceButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* Initialize */
    memset(phaseState, SC_PENDING, sizeof(phaseState));
    memset(phaseSavedStatus, 0, sizeof(phaseSavedStatus));
    sc->currentPhase = 0;
    sc->phaseElapsed = 0;
    sc->isRunning = false;
    sc->checkTimer = NULL;

    sc_select_phase(0);
    LV_LOG_USER("Self-check popup created");
}
