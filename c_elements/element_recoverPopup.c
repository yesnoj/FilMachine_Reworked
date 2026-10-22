/**
 * @file element_recoverPopup.c
 *
 * Tools -> "Empty Cx into the bottle" (machine v7).
 *
 * Moves the whole content of one chemistry container into its recovery bottle
 * through the RECOVERY outlet (valve A6, seventh port of the collector), in
 * four phases, all of them tank-mediated transfers (see xfer* in accessories.c):
 *
 *   1. wash the line: RECOVER_WASH_ML of bath water through the line to waste
 *   2. head: the first RECOVER_HEAD_ML of the chemistry to waste, they carry
 *      away the film of water left by the wash
 *   3. transfer: CHEM_CONTAINER_ML of the chemistry into the bottle, cycle
 *      after cycle (no level sensor since Mainboard v1.2: once the container
 *      is empty the pump just moves air, which is harmless)
 *   4. wash the line again, so the next use finds it clean
 *
 * Nothing but that chemistry (and a few tenths of a millilitre of water) can
 * reach the bottle: every other liquid was washed out of the line before and
 * the head of the chemistry itself was discarded.
 *
 * The popup shows the flow: source bar -> tank bar -> destination bar with a
 * drop running along the pipe, the four phases as dots underneath, the
 * remaining time, and Stop (graceful: the tank is emptied to where the liquid
 * was going, then the run ends).
 */
#include <string.h>
#include "FilMachine.h"
#include "esp_log.h"
#if defined(DISPLAY_DRIVER_ST7701)
#include "st7701_lcd.h"
#endif

static const char *TAG = "RecoverProc";

extern struct gui_components gui;

static const char    *containerNames[] = { "C1", "C2", "C3" };
static const uint32_t containerColor[] = { ORANGE_LIGHT, GREEN_LIGHT, LIGHT_BLUE };
#define COLOR_WATER   BLUE
#define COLOR_WASTE   0x6A6F76
#define COLOR_DOT_OFF 0x4A4F56
#define COLOR_PICK_OFF 0x2B2F36
#define NO_CONTAINER  255

static uint16_t s_movedMl = 0;      /* chemistry delivered to the bottle in the current/last run */

/* ── Forward declarations ───────────────────────────────── */
static void recover_reset(void);
static void recover_start(void);
static void recover_timer_cb(lv_timer_t *timer);
static void recover_set_drop(int8_t dir);

/* ── Helpers ────────────────────────────────────────────── */
static uint8_t     cidx(void)   { uint8_t c = gui.element.recoverPopup.container; return c > 2 ? 0 : c; }
static const char *cname(void) { return containerNames[cidx()]; }
static uint32_t    ccolor(void) { return containerColor[cidx()]; }

/* Confirm page: the chosen container is highlighted, Start enabled only with one chosen. */
static void recover_draw_choice(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    for (uint8_t i = 0; i < 3; i++) {
        bool on = (rp->container == i);
        lv_obj_set_style_bg_color(rp->pickButton[i], lv_color_hex(on ? containerColor[i] : COLOR_PICK_OFF), LV_PART_MAIN);
        lv_obj_set_style_text_color(rp->pickButton[i], lv_color_hex(on ? 0x101214 : WHITE), LV_PART_MAIN);
        if (on) lv_obj_add_state(rp->pickButton[i], LV_STATE_CHECKED);
        else    lv_obj_remove_state(rp->pickButton[i], LV_STATE_CHECKED);
    }
    if (rp->container > 2) {
        lv_obj_add_state(rp->recoverStartButton, LV_STATE_DISABLED);
        lv_label_set_text(rp->recoverTitle, recoverPopupTitle_text);
    } else {
        lv_obj_remove_state(rp->recoverStartButton, LV_STATE_DISABLED);
        lv_label_set_text_fmt(rp->recoverTitle, recoverPopupTitleFmt_text, cname());
    }
}

static uint16_t phase_ml(uint8_t phase) {
    switch (phase) {
        case RECOVER_PH_WASH1: case RECOVER_PH_WASH2: return RECOVER_WASH_ML;
        case RECOVER_PH_HEAD: return RECOVER_HEAD_ML;
        default: return CHEM_CONTAINER_ML;
    }
}

static uint32_t total_estimate(void) {
    uint32_t t = 0;
    for (uint8_t p = 0; p < 4; p++) t += xferTransferSecs(phase_ml(p));
    return t;
}

/* Start the engine for `phase`; false if the pump is still busy elsewhere. */
static bool recover_start_phase(uint8_t phase) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    uint8_t cx = getValueForChemicalSource(rp->container);   /* C1..C3 = relay 0..2 */
    switch (phase) {
        case RECOVER_PH_WASH1:
        case RECOVER_PH_WASH2:
            return xferStartVolume(getValueForChemicalSource(WB), getValueForChemicalSource(WASTE), RECOVER_WASH_ML, NULL);
        case RECOVER_PH_HEAD:
            return xferStartVolume(cx, getValueForChemicalSource(WASTE), RECOVER_HEAD_ML, NULL);
        default:
#if defined(RECOVERY_RLY)
            return xferStartVolume(cx, RECOVERY_RLY, CHEM_CONTAINER_ML, NULL);   /* no level sensor: the nominal volume */
#else
            return false;
#endif
    }
}

static void bar_color(lv_obj_t *bar, uint32_t rgb) {
    lv_obj_set_style_bg_color(bar, lv_color_hex(rgb), LV_PART_INDICATOR);
    lv_obj_set_style_border_color(bar, lv_color_hex(rgb), LV_PART_MAIN);
}

/* The three bars and their labels for the current phase and engine state. */
static void recover_draw_flow(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    bool done = (rp->result == RECOVER_TOOL_DONE);
    bool wash = !done && (rp->phase == RECOVER_PH_WASH1 || rp->phase == RECOVER_PH_WASH2);
    bool toBottle = done || (rp->phase == RECOVER_PH_XFER);   /* at the end: the container empty, the bottle full */

    /* Source: bath water during the washes, the container otherwise */
    if (wash) {
        lv_label_set_text(rp->srcLabel, "WB");
        bar_color(rp->srcBar, COLOR_WATER);
        lv_bar_set_value(rp->srcBar, 100, LV_ANIM_OFF);
    } else {
        lv_label_set_text(rp->srcLabel, cname());
        bar_color(rp->srcBar, ccolor());
        /* what is left in the container: full minus the head, then down to 0 as the transfer proceeds */
        int32_t left = 100;
        if (rp->phase == RECOVER_PH_HEAD) left = 97;
        else if (toBottle) {
            uint16_t target = xferTargetMl() ? xferTargetMl() : CHEM_CONTAINER_ML;
            uint32_t moved = xferMovedMl();
            if (xferPhase() == 1) moved += (uint32_t)xferPhaseSecs() * XFER_CYCLE_ML / (xferCycleSecs() ? xferCycleSecs() : 1);
            if (moved > target) moved = target;
            left = 100 - (int32_t)(moved * 100 / target);
            if (left < 3) left = 3;
            if (rp->result == RECOVER_TOOL_DONE) left = 0;
        }
        lv_bar_set_value(rp->srcBar, left, LV_ANIM_ON);
    }

    /* Tank: fills over the cycle, empties over the drain */
    uint32_t liquid = wash ? COLOR_WATER : ccolor();
    bar_color(rp->tankBar, liquid);
    int32_t tank = 0;
    uint16_t tot = xferPhaseTotalSecs();
    if (xferPhase() == 1 && tot) tank = (int32_t)((uint32_t)xferPhaseSecs() * 100 / tot);
    else if (xferPhase() == 2) {
        uint16_t liq = tot > pumpPurgeSecs() ? (uint16_t)(tot - pumpPurgeSecs()) : 1;
        tank = xferPhaseSecs() >= liq ? 0 : 100 - (int32_t)((uint32_t)xferPhaseSecs() * 100 / liq);
    }
    if (tank > 100) tank = 100;
    lv_bar_set_value(rp->tankBar, tank, LV_ANIM_ON);

    /* Destination: the bottle (filling up with the chemistry) or the waste */
    if (toBottle) {
        lv_label_set_text(rp->dstLabel, recoverLblBottle_text);
        bar_color(rp->dstBar, ccolor());
        uint16_t target = xferTargetMl() ? xferTargetMl() : CHEM_CONTAINER_ML;
        int32_t got = (int32_t)((uint32_t)xferMovedMl() * 100 / target);
        if (rp->result == RECOVER_TOOL_DONE) got = 100;
        lv_bar_set_value(rp->dstBar, got > 100 ? 100 : got, LV_ANIM_ON);
    } else {
        lv_label_set_text(rp->dstLabel, recoverLblWaste_text);
        bar_color(rp->dstBar, COLOR_WASTE);
        lv_bar_set_value(rp->dstBar, 0, LV_ANIM_OFF);
    }

    /* Phase dots: done = green, current = liquid colour, pending = grey */
    for (uint8_t i = 0; i < 4; i++) {
        uint32_t c = COLOR_DOT_OFF;
        if (rp->result == RECOVER_TOOL_DONE || i < rp->phase) c = GREEN;
        else if (i == rp->phase && rp->isRunning) c = (i == RECOVER_PH_XFER) ? ccolor() : COLOR_WATER;
        lv_obj_set_style_bg_color(rp->phaseDot[i], lv_color_hex(c), 0);
        lv_obj_set_style_text_color(rp->phaseDotLabel[i], lv_color_hex(c == COLOR_DOT_OFF ? 0x9AA0A6 : WHITE), 0);
    }

    /* The drop follows the pump */
    if (!rp->isRunning) recover_set_drop(0);
    else recover_set_drop((int8_t)xferPhase());
}

static void recover_draw_status(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (rp->stopping) {
        lv_label_set_text(rp->recoverStatusLabel, recoverStopping_text);
    } else if (rp->waitingPump) {
        lv_label_set_text(rp->recoverStatusLabel, recoverWaiting_text);
    } else switch (rp->phase) {
        case RECOVER_PH_WASH1: lv_label_set_text(rp->recoverStatusLabel, recoverPhaseWash1_text); break;
        case RECOVER_PH_HEAD:  lv_label_set_text(rp->recoverStatusLabel, recoverPhaseHead_text);  break;
        case RECOVER_PH_XFER: {
            uint16_t cycle = (uint16_t)(xferMovedMl() / XFER_CYCLE_ML) + 1;
            if (cycle > rp->xferCycles) cycle = rp->xferCycles;
            lv_label_set_text_fmt(rp->recoverStatusLabel, recoverPhaseXferFmt_text, cname(), (int)cycle, (int)rp->xferCycles);
            break;
        }
        default: lv_label_set_text(rp->recoverStatusLabel, recoverPhaseWash2_text); break;
    }
    if (rp->stopping) { lv_label_set_text(rp->recoverTimeLabel, ""); return; }
    uint32_t rem = rp->estimate > rp->elapsed ? rp->estimate - rp->elapsed : 0;
    if (rem >= 60) lv_label_set_text_fmt(rp->recoverTimeLabel, timeLeftMinFmt_text, (long)(rem / 60), (long)(rem % 60));
    else           lv_label_set_text_fmt(rp->recoverTimeLabel, timeLeftSecFmt_text, (long)rem);
}

/* ── The drop on the pipe ───────────────────────────────── */
static void drop_anim_cb(void *var, int32_t v) { lv_obj_set_x((lv_obj_t *)var, v); }

static void recover_set_drop(int8_t dir) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    const ui_recover_popup_layout_t *ui = &ui_get_profile()->recover_popup;
    if (rp->dropDir == dir) return;
    rp->dropDir = dir;
    lv_anim_delete(rp->drop, drop_anim_cb);
    if (dir == 0) { lv_obj_add_flag(rp->drop, LV_OBJ_FLAG_HIDDEN); return; }
    lv_obj_remove_flag(rp->drop, LV_OBJ_FLAG_HIDDEN);
    int32_t half = ui->drop_size / 2;
    int32_t x0 = (dir == 1 ? ui->bar_src_x + ui->bar_w : ui->bar_tank_x + ui->bar_w) + 2;
    int32_t x1 = (dir == 1 ? ui->bar_tank_x : ui->bar_dst_x) - ui->drop_size - 2;
    (void)half;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, rp->drop);
    lv_anim_set_values(&a, x0, x1);
    lv_anim_set_duration(&a, 900);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, drop_anim_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);
}

/* ── End of a run ───────────────────────────────────────── */
static void recover_show_end(uint8_t result) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    safeTimerDelete(&rp->recoverTimer);
    rp->isRunning = false;
    rp->stopping = false;
    rp->waitingPump = false;
    rp->result = result;
    recover_set_drop(0);
    lv_bar_set_value(rp->tankBar, 0, LV_ANIM_ON);
    if (result == RECOVER_TOOL_DONE) {
        lv_label_set_text_fmt(rp->recoverStatusLabel, recoverCompleteFmt_text, cname());
        lv_obj_set_style_text_color(rp->recoverStatusLabel, lv_color_hex(GREEN), LV_PART_MAIN);
        gui.page.tools.machineStats.clean++;             /* counts as a maintenance cycle */
        qSysAction(SAVE_MACHINE_STATS);
        ESP_LOGI(TAG, "=== RECOVERY OF %s FINISHED ===", cname());
    } else {
        lv_label_set_text(rp->recoverStatusLabel, recoverStopped_text);
        ESP_LOGW(TAG, "=== RECOVERY OF %s STOPPED ===", cname());
    }
    recover_draw_flow();
    lv_label_set_text(rp->recoverTimeLabel, "");
    lv_label_set_text(rp->recoverStopButtonLabel, buttonClose_text);
    lv_obj_set_style_bg_color(rp->recoverStopButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
#if defined(DISPLAY_DRIVER_ST7701)
    st7701_lcd_set_dim_inhibit(false);
#endif
    alarm_start_persistent();
}

/* ── Timer callback (every second while running) ────────── */
static void recover_timer_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (!rp->isRunning) return;
    rp->elapsed++;

    if (rp->stopNowPressed && !rp->stopping) {
        rp->stopping = true;
        rp->waitingPump = false;
        xferStop();                                        /* drains the tank to where the liquid was going */
        ESP_LOGW(TAG, "Recovery STOPPED by user: finishing the drain");
    }
    if (rp->stopping) {
        if (rp->phase == RECOVER_PH_XFER) s_movedMl = xferMovedMl();
        if (!xferBusy()) { recover_show_end(RECOVER_TOOL_STOPPED); return; }
        recover_draw_flow();
        recover_draw_status();
        return;
    }

    if (rp->waitingPump) {
        /* Another tool is still draining the tank: start when the pump is free */
        if (recover_start_phase(rp->phase)) rp->waitingPump = false;
    } else if (!xferBusy()) {
        /* Phase over: the next one, or done */
        if (rp->phase == RECOVER_PH_XFER) s_movedMl = xferMovedMl();
        if (rp->phase >= RECOVER_PH_WASH2) { recover_show_end(RECOVER_TOOL_DONE); return; }
        rp->phase++;
        ESP_LOGI(TAG, "Recovery of %s: phase %d", cname(), rp->phase + 1);
        if (!recover_start_phase(rp->phase)) rp->waitingPump = true;
    }
    recover_draw_flow();
    recover_draw_status();
}

/* ── Reset UI to initial (confirm) state ─────────────── */
static void recover_reset(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    alarm_stop();
    if (rp->isRunning) {
        xferStop();
        safeTimerDelete(&rp->recoverTimer);
    }
#if defined(DISPLAY_DRIVER_ST7701)
    st7701_lcd_set_dim_inhibit(false);
#endif
    rp->isRunning = false;
    rp->stopNowPressed = false;
    rp->stopping = false;
    rp->waitingPump = false;
    rp->result = 0;
    rp->phase = RECOVER_PH_WASH1;
    rp->elapsed = 0;
    rp->container = NO_CONTAINER;
    recover_set_drop(0);

    lv_label_set_text(rp->recoverInfoLabel, recoverConfirmBody_text);
    recover_draw_choice();
    lv_label_set_text(rp->recoverStatusLabel, "");
    lv_obj_set_style_text_color(rp->recoverStatusLabel, lv_color_hex(WHITE), LV_PART_MAIN);
    lv_label_set_text(rp->recoverTimeLabel, "");
    lv_label_set_text(rp->recoverStopButtonLabel, buttonStop_text);
    lv_obj_set_style_bg_color(rp->recoverStopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
    lv_bar_set_value(rp->srcBar, 100, LV_ANIM_OFF);
    lv_bar_set_value(rp->tankBar, 0, LV_ANIM_OFF);
    lv_bar_set_value(rp->dstBar, 0, LV_ANIM_OFF);

    lv_obj_remove_flag(rp->recoverConfirmContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(rp->recoverStartButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(rp->recoverCancelButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(rp->recoverProcessContainer, LV_OBJ_FLAG_HIDDEN);
}

/* ── Start the sequence ──────────────────────────────── */
static void recover_start(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (rp->container > 2) return;                         /* nothing chosen */
    lv_label_set_text_fmt(rp->recoverTitle, recoverPopupTitleFmt_text, cname());
    rp->isRunning = true;
    rp->stopNowPressed = false;
    rp->stopping = false;
    rp->result = 0;
    rp->phase = RECOVER_PH_WASH1;
    rp->elapsed = 0;
    s_movedMl = 0;
    rp->estimate = total_estimate();
    rp->xferCycles = (uint16_t)((CHEM_CONTAINER_ML + XFER_CYCLE_ML - 1) / XFER_CYCLE_ML);

    lv_obj_add_flag(rp->recoverConfirmContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(rp->recoverStartButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(rp->recoverCancelButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(rp->recoverProcessContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(rp->recoverStatusLabel, lv_color_hex(WHITE), LV_PART_MAIN);
    lv_label_set_text(rp->recoverStopButtonLabel, buttonStop_text);
    lv_obj_set_style_bg_color(rp->recoverStopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);

    ESP_LOGI(TAG, "=== RECOVERY OF %s STARTED (about %lu s) ===", cname(), (unsigned long)rp->estimate);
#if defined(DISPLAY_DRIVER_ST7701)
    st7701_lcd_set_dim_inhibit(true);
#endif
    rp->waitingPump = !recover_start_phase(rp->phase);
    recover_draw_flow();
    recover_draw_status();
    rp->recoverTimer = lv_timer_create(recover_timer_cb, 1000, NULL);
}

/* ══════════════════════════════════════════════════════════
 *  STATE FOR THE APP  +  REMOTE CONTROL (WebSocket)
 * ══════════════════════════════════════════════════════════ */
int recoverToolState(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (rp->isRunning) return rp->stopping ? RECOVER_TOOL_STOPPING : RECOVER_TOOL_RUNNING;
    return rp->result;
}
bool recoverToolBusy(void)        { return gui.element.recoverPopup.isRunning; }
int  recoverToolContainer(void)   { uint8_t c = gui.element.recoverPopup.container; return c > 2 ? -1 : (int)c; }
int  recoverToolPhase(void)       { return gui.element.recoverPopup.phase; }
uint16_t recoverToolMovedMl(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (rp->isRunning && rp->phase == RECOVER_PH_XFER) return xferMovedMl();
    return s_movedMl;
}
int recoverToolPercent(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (rp->result == RECOVER_TOOL_DONE) return 100;
    if (!rp->isRunning || rp->estimate == 0) return 0;
    uint32_t p = rp->elapsed * 100 / rp->estimate;
    return p > 99 ? 99 : (int)p;
}
uint32_t recoverToolRemainingSecs(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (!rp->isRunning) return 0;
    return rp->estimate > rp->elapsed ? rp->estimate - rp->elapsed : 0;
}

bool recoverPopupRemoteStart(uint8_t container) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (rp->isRunning || container > 2) return false;
    recoverPopupCreate();                                  /* confirm page, nothing chosen */
    if (rp->recoverPopupParent == NULL) return false;
    rp->container = container;                             /* the app chose */
    recover_draw_choice();
    recover_start();
    return rp->isRunning;
}

void recoverPopupRemoteStop(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (rp->isRunning) rp->stopNowPressed = true;
}

void recoverPopupRemoteClose(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    if (rp->recoverPopupParent == NULL || rp->isRunning) return;
    recover_reset();
    lv_obj_add_flag(rp->recoverPopupParent, LV_OBJ_FLAG_HIDDEN);
}

/* ══════════════════════════════════════════════════════════
 *  EVENT  HANDLER
 * ══════════════════════════════════════════════════════════ */
void event_recoverPopup(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t       *obj  = lv_event_get_target(e);
    struct sRecoverPopup *rp = &gui.element.recoverPopup;

    if (code != LV_EVENT_CLICKED) return;

    for (uint8_t i = 0; i < 3; i++) {
        if (obj == rp->pickButton[i] && !rp->isRunning) {
            rp->container = i;
            recover_draw_choice();
            return;
        }
    }
    if (obj == rp->recoverCancelButton) {
        lv_obj_add_flag(rp->recoverPopupParent, LV_OBJ_FLAG_HIDDEN);
    } else if (obj == rp->recoverStartButton) {
        if (rp->container > 2) return;
        if (maintenanceBusy() && !xferBusy()) return;    /* another tool (fill, load...) is running */
        recover_start();
    } else if (obj == rp->recoverStopButton) {
        if (rp->isRunning) {
            rp->stopNowPressed = true;
        } else {
            recover_reset();
            lv_obj_add_flag(rp->recoverPopupParent, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ══════════════════════════════════════════════════════════
 *  UI  CREATION  (created once; reopened for each container)
 * ══════════════════════════════════════════════════════════ */
static lv_obj_t *make_bar(lv_obj_t *parent, int x, int y, int w, int h, uint32_t rgb, int radius) {
    lv_obj_t *b = lv_bar_create(parent);
    lv_obj_set_size(b, w, h);
    lv_bar_set_range(b, 0, 100);
    lv_bar_set_value(b, 0, LV_ANIM_OFF);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_style_bg_color(b, lv_color_hex(rgb), LV_PART_INDICATOR);
    lv_obj_set_style_radius(b, radius, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(b, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_radius(b, radius, LV_PART_MAIN);
    lv_obj_set_style_border_color(b, lv_color_hex(rgb), LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 2, LV_PART_MAIN);
    lv_obj_set_style_border_opa(b, LV_OPA_50, LV_PART_MAIN);
    return b;
}

static lv_obj_t *make_bar_label(lv_obj_t *parent, int x, int y, int w, const lv_font_t *font, const char *txt) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_width(l, w);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(l, x, y);
    return l;
}

void recoverPopupCreate(void) {
    struct sRecoverPopup *rp = &gui.element.recoverPopup;
    const ui_drain_popup_layout_t   *ui  = &ui_get_profile()->drain_popup;    /* same frame as the drain popup */
    const ui_recover_popup_layout_t *uir = &ui_get_profile()->recover_popup;

    if (rp->recoverPopupParent != NULL) {
        /* Already built: reopen (on the confirm page unless a run is going on) */
        if (!rp->isRunning) recover_reset();
        lv_obj_remove_flag(rp->recoverPopupParent, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    rp->container = NO_CONTAINER;

    /* ═══ BACKDROP + TITLE ═══ */
    createPopupBackdrop(&rp->recoverPopupParent, &rp->recoverContainer, ui_get_profile()->popups.drain_w, ui_get_profile()->popups.drain_h);

    rp->recoverTitle = lv_label_create(rp->recoverContainer);
    lv_label_set_text(rp->recoverTitle, recoverPopupTitle_text);
    lv_obj_set_style_text_font(rp->recoverTitle, ui->title_font, 0);
    lv_obj_align(rp->recoverTitle, LV_ALIGN_TOP_MID, ui->title_x, ui->title_y);

    initTitleLineStyle(&rp->style_recoverTitleLine, WHITE);
    rp->titleLinePoints[1].x = ui_get_profile()->popups.drain_title_line_w;
    rp->recoverTitleLine = lv_line_create(rp->recoverContainer);
    lv_line_set_points(rp->recoverTitleLine, rp->titleLinePoints, 2);
    lv_obj_add_style(rp->recoverTitleLine, &rp->style_recoverTitleLine, 0);
    lv_obj_align(rp->recoverTitleLine, LV_ALIGN_TOP_MID, ui->title_line_x, ui->title_line_y);

    /* ═══ PHASE 1 – CONFIRM ═══ */
    rp->recoverConfirmContainer = lv_obj_create(rp->recoverPopupParent);
    lv_obj_align(rp->recoverConfirmContainer, LV_ALIGN_TOP_MID, ui->confirm_x, ui->confirm_y);
    lv_obj_set_size(rp->recoverConfirmContainer, ui_get_profile()->popups.drain_confirm_w, ui_get_profile()->popups.drain_confirm_h);
    lv_obj_remove_flag(rp->recoverConfirmContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_opa(rp->recoverConfirmContainer, LV_OPA_TRANSP, 0);

    rp->recoverInfoLabel = lv_label_create(rp->recoverConfirmContainer);
    lv_label_set_text(rp->recoverInfoLabel, recoverConfirmBody_text);
    lv_obj_set_style_text_font(rp->recoverInfoLabel, ui->info_font, 0);
    lv_obj_set_style_text_align(rp->recoverInfoLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(rp->recoverInfoLabel, ui->confirm_info_w);
    lv_label_set_long_mode(rp->recoverInfoLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(rp->recoverInfoLabel, LV_ALIGN_TOP_MID, ui->confirm_info_x, ui->confirm_info_y);

    /* C1 / C2 / C3: one of them, then Start */
    for (uint8_t i = 0; i < 3; i++) {
        rp->pickButton[i] = lv_button_create(rp->recoverConfirmContainer);
        lv_obj_set_size(rp->pickButton[i], uir->pick_w, uir->pick_h);
        lv_obj_align(rp->pickButton[i], LV_ALIGN_TOP_MID, (i - 1) * uir->pick_pitch, uir->pick_y);
        lv_obj_add_flag(rp->pickButton[i], LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_style_bg_color(rp->pickButton[i], lv_color_hex(COLOR_PICK_OFF), LV_PART_MAIN);
        lv_obj_set_style_border_color(rp->pickButton[i], lv_color_hex(containerColor[i]), LV_PART_MAIN);
        lv_obj_set_style_border_width(rp->pickButton[i], 3, LV_PART_MAIN);
        lv_obj_set_style_border_opa(rp->pickButton[i], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(rp->pickButton[i], lv_color_hex(containerColor[i]), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_event_cb(rp->pickButton[i], event_recoverPopup, LV_EVENT_CLICKED, NULL);
        lv_obj_t *l = lv_label_create(rp->pickButton[i]);
        lv_label_set_text(l, containerNames[i]);
        lv_obj_set_style_text_font(l, ui->button_font, 0);
        lv_obj_center(l);
    }

    rp->recoverStartButton = lv_button_create(rp->recoverContainer);
    lv_obj_set_size(rp->recoverStartButton, ui->action_btn_w, ui->action_btn_h);
    lv_obj_align(rp->recoverStartButton, LV_ALIGN_BOTTOM_RIGHT, -ui->start_cancel_btn_x, ui->start_cancel_btn_y);
    lv_obj_add_event_cb(rp->recoverStartButton, event_recoverPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(rp->recoverStartButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
    rp->recoverStartButtonLabel = lv_label_create(rp->recoverStartButton);
    lv_label_set_text(rp->recoverStartButtonLabel, buttonStart_text);
    lv_obj_set_style_text_font(rp->recoverStartButtonLabel, ui->button_font, 0);
    lv_obj_align(rp->recoverStartButtonLabel, LV_ALIGN_CENTER, 0, 0);

    rp->recoverCancelButton = lv_button_create(rp->recoverContainer);
    lv_obj_set_size(rp->recoverCancelButton, ui->action_btn_w, ui->action_btn_h);
    lv_obj_align(rp->recoverCancelButton, LV_ALIGN_BOTTOM_LEFT, ui->start_cancel_btn_x, ui->start_cancel_btn_y);
    lv_obj_add_event_cb(rp->recoverCancelButton, event_recoverPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(rp->recoverCancelButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
    rp->recoverCancelButtonLabel = lv_label_create(rp->recoverCancelButton);
    lv_label_set_text(rp->recoverCancelButtonLabel, buttonCancel_text);
    lv_obj_set_style_text_font(rp->recoverCancelButtonLabel, ui->button_font, 0);
    lv_obj_align(rp->recoverCancelButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* ═══ PHASE 2 – PROCESS (hidden initially) ═══ */
    rp->recoverProcessContainer = lv_obj_create(rp->recoverPopupParent);
    lv_obj_align(rp->recoverProcessContainer, LV_ALIGN_TOP_MID, ui->progress_container_x, ui->progress_container_y);
    lv_obj_set_size(rp->recoverProcessContainer, ui_get_profile()->popups.drain_process_w, ui_get_profile()->popups.drain_process_h);
    lv_obj_remove_flag(rp->recoverProcessContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(rp->recoverProcessContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_border_opa(rp->recoverProcessContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(rp->recoverProcessContainer, ui->progress_container_pad, 0);

    /* The pipe: a thin line at bar mid-height from the source to the destination */
    rp->pipe = lv_obj_create(rp->recoverProcessContainer);
    lv_obj_remove_style_all(rp->pipe);
    lv_obj_set_size(rp->pipe, uir->bar_dst_x - uir->bar_src_x - uir->bar_w, 4);
    lv_obj_set_pos(rp->pipe, uir->bar_src_x + uir->bar_w, uir->pipe_y - 2);
    lv_obj_set_style_bg_opa(rp->pipe, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(rp->pipe, lv_color_hex(0x8A9099), 0);
    lv_obj_set_style_radius(rp->pipe, 2, 0);

    /* Source, tank, destination bars (vertical, filled from the bottom) */
    rp->srcBar  = make_bar(rp->recoverProcessContainer, uir->bar_src_x,  uir->bar_y, uir->bar_w, uir->bar_h, ccolor(), ui->tank_bar_radius);
    rp->tankBar = make_bar(rp->recoverProcessContainer, uir->bar_tank_x, uir->bar_y, uir->bar_w, uir->bar_h, ccolor(), ui->tank_bar_radius);
    rp->dstBar  = make_bar(rp->recoverProcessContainer, uir->bar_dst_x,  uir->bar_y, uir->bar_w, uir->bar_h, COLOR_WASTE, ui->tank_bar_radius);
    lv_bar_set_value(rp->srcBar, 100, LV_ANIM_OFF);
    rp->srcLabel  = make_bar_label(rp->recoverProcessContainer, uir->bar_src_x - 20,  uir->bar_label_y, uir->bar_w + 40, ui->bar_label_font, cname());
    rp->tankLabel = make_bar_label(rp->recoverProcessContainer, uir->bar_tank_x - 20, uir->bar_label_y, uir->bar_w + 40, ui->bar_label_font, recoverLblTank_text);
    rp->dstLabel  = make_bar_label(rp->recoverProcessContainer, uir->bar_dst_x - 30,  uir->bar_label_y, uir->bar_w + 60, ui->bar_label_font, recoverLblWaste_text);

    /* The drop that runs along the pipe */
    rp->drop = lv_obj_create(rp->recoverProcessContainer);
    lv_obj_remove_style_all(rp->drop);
    lv_obj_set_size(rp->drop, uir->drop_size, uir->drop_size);
    lv_obj_set_pos(rp->drop, uir->bar_src_x + uir->bar_w + 2, uir->pipe_y - uir->drop_size / 2);
    lv_obj_set_style_radius(rp->drop, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(rp->drop, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(rp->drop, lv_color_hex(WHITE), 0);
    lv_obj_add_flag(rp->drop, LV_OBJ_FLAG_HIDDEN);
    rp->dropDir = 0;

    /* The four phases */
    {
        char names[64];
        strncpy(names, recoverStepNames, sizeof(names) - 1);
        names[sizeof(names) - 1] = '\0';
        char *save = NULL;
        char *tok = strtok_r(names, "\n", &save);
        for (uint8_t i = 0; i < 4; i++) {
            int x = uir->dot_x0 + i * uir->dot_pitch;
            rp->phaseDot[i] = lv_obj_create(rp->recoverProcessContainer);
            lv_obj_remove_style_all(rp->phaseDot[i]);
            lv_obj_set_size(rp->phaseDot[i], uir->dot_size, uir->dot_size);
            lv_obj_set_pos(rp->phaseDot[i], x - uir->dot_size / 2, uir->dot_y);
            lv_obj_set_style_radius(rp->phaseDot[i], LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_opa(rp->phaseDot[i], LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(rp->phaseDot[i], lv_color_hex(COLOR_DOT_OFF), 0);
            rp->phaseDotLabel[i] = make_bar_label(rp->recoverProcessContainer, x - uir->dot_pitch / 2, uir->dot_label_y, uir->dot_pitch, uir->dot_font, tok ? tok : "");
            tok = tok ? strtok_r(NULL, "\n", &save) : NULL;
        }
    }

    /* Status + time */
    rp->recoverStatusLabel = lv_label_create(rp->recoverProcessContainer);
    lv_label_set_text(rp->recoverStatusLabel, "");
    lv_obj_set_style_text_font(rp->recoverStatusLabel, uir->status_font, 0);
    lv_obj_set_width(rp->recoverStatusLabel, uir->status_w);
    lv_obj_set_style_text_align(rp->recoverStatusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(rp->recoverStatusLabel, LV_ALIGN_TOP_MID, 0, uir->status_y);

    rp->recoverTimeLabel = lv_label_create(rp->recoverProcessContainer);
    lv_label_set_text(rp->recoverTimeLabel, "");
    lv_obj_set_style_text_font(rp->recoverTimeLabel, uir->status_font, 0);
    lv_obj_set_width(rp->recoverTimeLabel, uir->status_w);
    lv_obj_set_style_text_align(rp->recoverTimeLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(rp->recoverTimeLabel, LV_ALIGN_TOP_MID, 0, uir->time_y);

    /* Stop / Close */
    rp->recoverStopButton = lv_button_create(rp->recoverProcessContainer);
    lv_obj_set_size(rp->recoverStopButton, ui->stop_btn_w, ui->stop_btn_h);
    lv_obj_align(rp->recoverStopButton, LV_ALIGN_TOP_MID, ui->stop_btn_x, ui->stop_btn_y);
    lv_obj_add_event_cb(rp->recoverStopButton, event_recoverPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(rp->recoverStopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
    rp->recoverStopButtonLabel = lv_label_create(rp->recoverStopButton);
    lv_label_set_text(rp->recoverStopButtonLabel, buttonStop_text);
    lv_obj_set_style_text_font(rp->recoverStopButtonLabel, ui->button_font, 0);
    lv_obj_align(rp->recoverStopButtonLabel, LV_ALIGN_CENTER, 0, 0);

    recover_reset();
}
