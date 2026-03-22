
/**
 * @file element_drainPopup.c
 *
 * Drain Machine – sequentially drains C1, C2, C3 and the Water Bath
 * to waste, showing a visual tank-level animation for each container.
 */
#include <string.h>
#include "FilMachine.h"

extern struct gui_components gui;

/* ── Tank configuration ─────────────────────────────────── */
#define NUM_TANKS  4

static const char    *tankNames[]   = { "C1",  "C2",  "C3",  "WB" };
static const uint32_t tankColor[]   = { ORANGE_LIGHT, GREEN_LIGHT, LIGHT_BLUE, BLUE };
static const uint8_t  tankSource[]  = { SOURCE_C1, SOURCE_C2, SOURCE_C3, SOURCE_WB };

static int32_t getTankTime(int tank) {
    if (tank < 3) return (int32_t)getContainerFillTime();
    return (int32_t)getWbFillTime();
}

static int32_t totalDrainTime(void) {
    int32_t t = 0;
    for (int i = 0; i < NUM_TANKS; i++) t += getTankTime(i);
    return t;
}

/* ── Forward declarations ───────────────────────────────── */
static void drain_reset(void);
static void drain_start(void);
static void drain_timer_cb(lv_timer_t *timer);

/* ── Timer callback (called every 1 s while draining) ──── */
static void drain_timer_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);
    struct sDrainPopup *dp = &gui.element.drainPopup;

    /* --- STOP requested --------------------------------- */
    if (dp->stopNowPressed) {
        cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
        safeTimerDelete(&dp->drainTimer);
        dp->isDraining = false;

        lv_label_set_text(dp->drainStatusLabel, drainStopped_text);
        lv_label_set_text(dp->drainTimeLabel, "");
        lv_label_set_text(dp->drainWasteLabel, "");
        lv_label_set_text(dp->drainStopButtonLabel, buttonClose_text);
        lv_obj_set_style_bg_color(dp->drainStopButton,
                                  lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        return;
    }

    /* --- Advance elapsed counters ----------------------- */
    dp->tankElapsed++;
    dp->totalElapsed++;

    uint8_t  tank = dp->currentTank;
    int32_t  tt   = getTankTime(tank);

    /* --- Update current tank bar (100 → 0) -------------- */
    int32_t pct = ((tt - dp->tankElapsed) * 100) / tt;
    if (pct < 0) pct = 0;
    lv_bar_set_value(dp->tankBar[tank], pct, LV_ANIM_ON);

    /* --- Update status labels --------------------------- */
    int32_t rem = totalDrainTime() - dp->totalElapsed;
    if (rem < 0) rem = 0;
    lv_label_set_text_fmt(dp->drainStatusLabel, "Draining: %s", tankNames[tank]);
    if (rem >= 60)
        lv_label_set_text_fmt(dp->drainTimeLabel,
                              "Time left: %ldm %02lds", (long)(rem / 60), (long)(rem % 60));
    else
        lv_label_set_text_fmt(dp->drainTimeLabel,
                              "Time left: %lds", (long)rem);

    /* --- Current tank finished? ------------------------- */
    if (dp->tankElapsed >= tt) {
        cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
        lv_bar_set_value(dp->tankBar[tank], 0, LV_ANIM_OFF);

        /* Mark done – green label */
        lv_obj_set_style_text_color(dp->tankLabel[tank],
                                    lv_color_hex(GREEN), LV_PART_MAIN);

        dp->currentTank++;
        dp->tankElapsed = 0;

        if (dp->currentTank >= NUM_TANKS) {
            /* ── All tanks done ── */
            safeTimerDelete(&dp->drainTimer);
            dp->isDraining = false;

            lv_label_set_text(dp->drainStatusLabel, drainComplete_text);
            lv_obj_set_style_text_color(dp->drainStatusLabel,
                                        lv_color_hex(GREEN), LV_PART_MAIN);
            lv_label_set_text(dp->drainWasteLabel, "");
            lv_label_set_text(dp->drainTimeLabel, "");
            lv_label_set_text(dp->drainStopButtonLabel, buttonClose_text);
            lv_obj_set_style_bg_color(dp->drainStopButton,
                                      lv_color_hex(GREEN_DARK), LV_PART_MAIN);

            /* Start persistent alarm after all containers are drained */
            alarm_start_persistent();

            LV_LOG_USER("Drain complete – all containers drained");
        } else {
            /* ── Start next tank ── */
            uint8_t next = dp->currentTank;
            cleanRelayManager(
                getValueForChemicalSource(tankSource[next]),
                getValueForChemicalSource(SOURCE_WASTE),
                PUMP_IN_RLY, true);
        }
    }
}

/* ── Reset UI to initial (confirm) state ─────────────── */
static void drain_reset(void) {
    struct sDrainPopup *dp = &gui.element.drainPopup;

    /* Stop alarm if it was ringing */
    alarm_stop();

    /* Stop timer & relays if still active */
    if (dp->isDraining) {
        cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
        safeTimerDelete(&dp->drainTimer);
    }

    /* Bars back to 100 % */
    for (int i = 0; i < NUM_TANKS; i++) {
        lv_bar_set_value(dp->tankBar[i], 100, LV_ANIM_OFF);
        lv_obj_set_style_text_color(dp->tankLabel[i],
                                    lv_color_hex(WHITE), LV_PART_MAIN);
    }

    dp->currentTank   = 0;
    dp->tankElapsed   = 0;
    dp->totalElapsed  = 0;
    dp->isDraining    = false;
    dp->stopNowPressed = false;

    lv_label_set_text(dp->drainStatusLabel, "");
    lv_obj_set_style_text_color(dp->drainStatusLabel,
                                lv_color_hex(WHITE), LV_PART_MAIN);
    lv_label_set_text(dp->drainWasteLabel, "");
    lv_label_set_text(dp->drainTimeLabel, "");
    lv_label_set_text(dp->drainStopButtonLabel, buttonStop_text);
    lv_obj_set_style_bg_color(dp->drainStopButton,
                              lv_color_hex(RED_DARK), LV_PART_MAIN);

    /* Show confirm, hide process */
    lv_obj_remove_flag(dp->drainConfirmContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dp->drainProcessContainer, LV_OBJ_FLAG_HIDDEN);
}

/* ── Start the drain sequence ────────────────────────── */
static void drain_start(void) {
    struct sDrainPopup *dp = &gui.element.drainPopup;

    /* Reset bars */
    for (int i = 0; i < NUM_TANKS; i++) {
        lv_bar_set_value(dp->tankBar[i], 100, LV_ANIM_OFF);
        lv_obj_set_style_text_color(dp->tankLabel[i],
                                    lv_color_hex(WHITE), LV_PART_MAIN);
    }

    dp->currentTank    = 0;
    dp->tankElapsed    = 0;
    dp->totalElapsed   = 0;
    dp->isDraining     = true;
    dp->stopNowPressed = false;

    /* Switch view */
    lv_obj_add_flag(dp->drainConfirmContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(dp->drainProcessContainer, LV_OBJ_FLAG_HIDDEN);

    /* Initial labels */
    lv_label_set_text(dp->drainStatusLabel, "Draining: C1");
    lv_obj_set_style_text_color(dp->drainStatusLabel,
                                lv_color_hex(WHITE), LV_PART_MAIN);
    int32_t tot = totalDrainTime();
    lv_label_set_text_fmt(dp->drainTimeLabel,
                          "Time left: %ldm %02lds", (long)(tot / 60), (long)(tot % 60));
    lv_label_set_text(dp->drainWasteLabel, "");
    lv_obj_add_flag(dp->drainWasteLabel, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(dp->drainStopButtonLabel, buttonStop_text);
    lv_obj_set_style_bg_color(dp->drainStopButton,
                              lv_color_hex(RED_DARK), LV_PART_MAIN);

    /* Start pump for C1 */
    cleanRelayManager(
        getValueForChemicalSource(SOURCE_C1),
        getValueForChemicalSource(SOURCE_WASTE),
        PUMP_IN_RLY, true);

    /* Create 1-second timer */
    dp->drainTimer = lv_timer_create(drain_timer_cb, 1000, NULL);
}

/* ══════════════════════════════════════════════════════════
 *  EVENT  HANDLER
 * ══════════════════════════════════════════════════════════ */
void event_drainPopup(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t       *obj  = lv_event_get_target(e);
    struct sDrainPopup *dp = &gui.element.drainPopup;

    if (code == LV_EVENT_CLICKED) {

        /* Cancel */
        if (obj == dp->drainCancelButton) {
            lv_obj_add_flag(dp->drainPopupParent, LV_OBJ_FLAG_HIDDEN);
        }
        /* Start */
        else if (obj == dp->drainStartButton) {
            drain_start();
        }
        /* Stop / Close */
        else if (obj == dp->drainStopButton) {
            if (dp->isDraining) {
                dp->stopNowPressed = true;
            } else {
                drain_reset();
                lv_obj_add_flag(dp->drainPopupParent, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════
 *  UI  CREATION
 * ══════════════════════════════════════════════════════════ */
void drainPopupCreate(void) {
    struct sDrainPopup *dp = &gui.element.drainPopup;
    const ui_drain_popup_layout_t *ui = &ui_get_profile()->drain_popup;

    if (dp->drainPopupParent != NULL) return;   /* already created */

    /* ═══ BACKDROP ═══ */
    createPopupBackdrop(&dp->drainPopupParent, &dp->drainContainer, ui_get_profile()->popups.drain_w, ui_get_profile()->popups.drain_h);

    /* Title */
    dp->drainTitle = lv_label_create(dp->drainContainer);
    lv_label_set_text(dp->drainTitle, drainMachine_text);
    lv_obj_set_style_text_font(dp->drainTitle, ui->title_font, 0);
    lv_obj_align(dp->drainTitle, LV_ALIGN_TOP_MID, 0, ui->title_y);

    /* White underline */
    initTitleLineStyle(&dp->style_drainTitleLine, WHITE);

    dp->drainTitleLine = lv_line_create(dp->drainContainer);
    lv_line_set_points(dp->drainTitleLine, dp->titleLinePoints, 2);
    lv_obj_add_style(dp->drainTitleLine, &dp->style_drainTitleLine, 0);
    lv_obj_align(dp->drainTitleLine, LV_ALIGN_TOP_MID, 0, ui->title_line_y);

    /* ═══════════════════════════════════════════════════════
     *  PHASE 1 – CONFIRM CONTAINER
     * ═══════════════════════════════════════════════════════ */
    dp->drainConfirmContainer = lv_obj_create(dp->drainPopupParent);
    lv_obj_align(dp->drainConfirmContainer, LV_ALIGN_TOP_MID, 0, ui->confirm_y);
    lv_obj_set_size(dp->drainConfirmContainer, ui_get_profile()->popups.drain_confirm_w, ui_get_profile()->popups.drain_confirm_h);
    lv_obj_remove_flag(dp->drainConfirmContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_opa(dp->drainConfirmContainer,
                                LV_OPA_TRANSP, 0);

    /* Info text */
    dp->drainInfoLabel = lv_label_create(dp->drainConfirmContainer);
    lv_label_set_text(dp->drainInfoLabel,
        "All containers and the water\n"
        "bath will be drained to waste.\n\n"
        "Containers:  C1   C2   C3\n"
        "Water Bath:  WB");
    lv_obj_set_style_text_font(dp->drainInfoLabel,
                               ui->info_font, 0);
    lv_obj_set_style_text_align(dp->drainInfoLabel,
                                LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(dp->drainInfoLabel, ui->info_w);
    lv_label_set_long_mode(dp->drainInfoLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(dp->drainInfoLabel, LV_ALIGN_TOP_MID, 0, ui->info_y);

    /* Start button (green, bottom-right) */
    dp->drainStartButton = lv_button_create(dp->drainConfirmContainer);
    lv_obj_set_size(dp->drainStartButton,
                    BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(dp->drainStartButton, LV_ALIGN_BOTTOM_RIGHT, -ui->button_x, ui->button_y);
    lv_obj_add_event_cb(dp->drainStartButton,
                        event_drainPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(dp->drainStartButton,
                              lv_color_hex(GREEN_DARK), LV_PART_MAIN);

    dp->drainStartButtonLabel = lv_label_create(dp->drainStartButton);
    lv_label_set_text(dp->drainStartButtonLabel, buttonStart_text);
    lv_obj_set_style_text_font(dp->drainStartButtonLabel,
                               ui->button_font, 0);
    lv_obj_align(dp->drainStartButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* Cancel button (red, bottom-left) */
    dp->drainCancelButton = lv_button_create(dp->drainConfirmContainer);
    lv_obj_set_size(dp->drainCancelButton,
                    BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(dp->drainCancelButton, LV_ALIGN_BOTTOM_LEFT, ui->button_x, ui->button_y);
    lv_obj_add_event_cb(dp->drainCancelButton,
                        event_drainPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(dp->drainCancelButton,
                              lv_color_hex(RED_DARK), LV_PART_MAIN);

    dp->drainCancelButtonLabel = lv_label_create(dp->drainCancelButton);
    lv_label_set_text(dp->drainCancelButtonLabel, buttonCancel_text);
    lv_obj_set_style_text_font(dp->drainCancelButtonLabel,
                               ui->button_font, 0);
    lv_obj_align(dp->drainCancelButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* ═══════════════════════════════════════════════════════
     *  PHASE 2 – PROCESS CONTAINER  (hidden initially)
     * ═══════════════════════════════════════════════════════ */
    dp->drainProcessContainer = lv_obj_create(dp->drainPopupParent);
    lv_obj_align(dp->drainProcessContainer, LV_ALIGN_TOP_MID, 0, ui->process_y);
    lv_obj_set_size(dp->drainProcessContainer, ui_get_profile()->popups.drain_process_w, ui_get_profile()->popups.drain_process_h);
    lv_obj_remove_flag(dp->drainProcessContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dp->drainProcessContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_border_opa(dp->drainProcessContainer,
                                LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(dp->drainProcessContainer, ui->process_pad, 0);

    /* ── Tank bars ─────────────────────────────────────── */
    const int bar_w = ui->bar_w, bar_h = ui->bar_h, gap = ui->bar_gap;
    const int total_w = NUM_TANKS * bar_w + (NUM_TANKS - 1) * gap;
    const int start_x = (ui->bars_area_w - total_w) / 2;

    for (int i = 0; i < NUM_TANKS; i++) {
        int x = start_x + i * (bar_w + gap);

        /* Bar (vertical – height > width) */
        dp->tankBar[i] = lv_bar_create(dp->drainProcessContainer);
        lv_obj_set_size(dp->tankBar[i], bar_w, bar_h);
        lv_bar_set_range(dp->tankBar[i], 0, 100);
        lv_bar_set_value(dp->tankBar[i], 100, LV_ANIM_OFF);
        lv_obj_set_pos(dp->tankBar[i], x, ui->bar_y);

        /* Indicator colour (filled) */
        lv_obj_set_style_bg_color(dp->tankBar[i],
            lv_color_hex(tankColor[i]), LV_PART_INDICATOR);
        lv_obj_set_style_radius(dp->tankBar[i], ui->bar_radius, LV_PART_INDICATOR);

        /* Background colour (empty) */
        lv_obj_set_style_bg_color(dp->tankBar[i],
            lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_obj_set_style_radius(dp->tankBar[i], ui->bar_radius, LV_PART_MAIN);

        /* Subtle coloured border */
        lv_obj_set_style_border_color(dp->tankBar[i],
            lv_color_hex(tankColor[i]), LV_PART_MAIN);
        lv_obj_set_style_border_width(dp->tankBar[i], 2, LV_PART_MAIN);
        lv_obj_set_style_border_opa(dp->tankBar[i],
            LV_OPA_50, LV_PART_MAIN);

        /* Label under bar */
        dp->tankLabel[i] = lv_label_create(dp->drainProcessContainer);
        lv_label_set_text(dp->tankLabel[i], tankNames[i]);
        lv_obj_set_style_text_font(dp->tankLabel[i],
                                   ui->bar_label_font, 0);
        lv_obj_set_width(dp->tankLabel[i], bar_w);
        lv_obj_set_style_text_align(dp->tankLabel[i],
                                    LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(dp->tankLabel[i], x, bar_h + ui->label_y);
    }

    /* ── Status labels (below bars) ────────────────────── */
    dp->drainStatusLabel = lv_label_create(dp->drainProcessContainer);
    lv_label_set_text(dp->drainStatusLabel, "");
    lv_obj_set_style_text_font(dp->drainStatusLabel,
                               ui->status_font, 0);
    lv_obj_set_width(dp->drainStatusLabel, ui->status_w);
    lv_obj_set_style_text_align(dp->drainStatusLabel,
                                LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(dp->drainStatusLabel, LV_ALIGN_TOP_MID, 0, ui->status_y);

    dp->drainWasteLabel = lv_label_create(dp->drainProcessContainer);
    lv_label_set_text(dp->drainWasteLabel, "");
    lv_obj_set_style_text_font(dp->drainWasteLabel,
                               ui->secondary_font, 0);
    lv_obj_set_width(dp->drainWasteLabel, ui->status_w);
    lv_obj_set_style_text_align(dp->drainWasteLabel,
                                LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(dp->drainWasteLabel, LV_ALIGN_TOP_MID, 0, ui->waste_y);

    dp->drainTimeLabel = lv_label_create(dp->drainProcessContainer);
    lv_label_set_text(dp->drainTimeLabel, "");
    lv_obj_set_style_text_font(dp->drainTimeLabel,
                               ui->secondary_font, 0);
    lv_obj_set_width(dp->drainTimeLabel, ui->status_w);
    lv_obj_set_style_text_align(dp->drainTimeLabel,
                                LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(dp->drainTimeLabel, LV_ALIGN_TOP_MID, 0, ui->time_y);

    /* ── Stop / Close button ───────────────────────────── */
    dp->drainStopButton = lv_button_create(dp->drainProcessContainer);
    lv_obj_set_size(dp->drainStopButton,
                    BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(dp->drainStopButton, LV_ALIGN_TOP_MID, 0, ui->stop_y);
    lv_obj_add_event_cb(dp->drainStopButton,
                        event_drainPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(dp->drainStopButton,
                              lv_color_hex(RED_DARK), LV_PART_MAIN);

    dp->drainStopButtonLabel = lv_label_create(dp->drainStopButton);
    lv_label_set_text(dp->drainStopButtonLabel, buttonStop_text);
    lv_obj_set_style_text_font(dp->drainStopButtonLabel,
                               ui->button_font, 0);
    lv_obj_align(dp->drainStopButtonLabel, LV_ALIGN_CENTER, 0, 0);
}
