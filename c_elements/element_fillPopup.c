/**
 * @file element_fillPopup.c
 *
 * Maintenance "Fill bath" popup. Opens the water-bath valve (via
 * machineFillStart; the pressurized inlet needs no pump) and shows a live
 * status plus the inlet flow rate. The fill stops automatically when the MAX
 * water-level sensor reads FULL, on a safety timeout, if the flow meter sees no
 * water, or when the user presses Stop. Bottom buttons: Cancel (left) closes the
 * popup; Run (right, green) starts the fill and reads "Stop" while filling.
 *
 * This is completely independent of the process state machine.
 */

//ESSENTIAL INCLUDES
#include "FilMachine.h"

extern struct gui_components gui;

/* ── Layout ── */
#define FILL_POPUP_W      520
#define FILL_POPUP_H      360
#define FILL_STATUS_Y     (-58)  /* status label offset from vertical centre */
#define FILL_BAR_W        380
#define FILL_BAR_H        26
#define FILL_BAR_Y        (-8)   /* bargraph offset from vertical centre */
#define FILL_BTN_Y        10     /* buttons' bottom margin (matches Clean) */
#define FILL_BTN_MX       15     /* buttons' side margin (matches Clean) */

static uint8_t s_target = FILL_TARGET_WB;   /* WB or chem container */

static const char *fill_status_text(int state)
{
    switch (state) {
        case FILL_FULL:       return fillStatusFull_text;
        case FILL_STOPPED:    return fillStatusStopped_text;
        case FILL_TIMEOUT:    return fillStatusTimeout_text;
        case FILL_NOFLOW:     return fillStatusNoFlow_text;
        case FILL_NOLEVEL:    return fillStatusNoLevel_text;
        case FILL_DONE_NOMAX: return fillStatusDoneNoMax_text;
        default:              return fillStatusRunning_text;   /* IDLE / RUNNING */
    }
}

static void fill_popup_close(void)
{
    struct sFillPopup *fp = &gui.element.fillPopup;
    if (fp->liveTimer) { lv_timer_delete(fp->liveTimer); fp->liveTimer = NULL; }
    lv_style_reset(&fp->style_titleLine);
    lv_style_reset(&fp->style_barIndic);
    lv_msgbox_close(fp->parent);
    fp->parent = NULL;
}

/* Manual fill (no inlet): status is driven by the floats, not by volume. */
static const char *fill_manual_status(int state)
{
    switch (state) {
        case FILL_FULL:    return fillStatusFull_text;
        case FILL_STOPPED: return fillStatusStopped_text;
        case FILL_TIMEOUT: return fillStatusTimeout_text;
        default:
            if (s_target == FILL_TARGET_CHEM) return fillChemFilling_text;   /* pump-driven */
            return (machineFillLevelPct() >= 50) ? fillManualFilling_text
                                                 : fillManualPour_text;
    }
}

/* Popup phase: waiting for Start, filling, or finished. */
#define FP_READY    0
#define FP_RUNNING  1
#define FP_DONE     2
static int     s_phase  = FP_READY;
static bool    s_manual = false;   /* true = level-based display (floats only) */

static void fill_set_button(lv_obj_t *btn, lv_obj_t *lbl, const char *text, uint32_t color)
{
    lv_label_set_text(lbl, text);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN);
}

/* Refresh the status line ~2x/second; flip the button Start → Stop → Close. */
static void fill_live_cb(lv_timer_t *t)
{
    (void)t;
    struct sFillPopup *fp = &gui.element.fillPopup;
    if (fp->parent == NULL) return;

    if (s_phase != FP_RUNNING) return;   /* only refresh while actually filling */

    int state = machineFillState();

    /* Bargraph is always level-based (MIN/MAX floats) — there is no volume
     * target, the sensor decides when it's full. */
    lv_label_set_text(fp->statusLabel, s_manual ? fill_manual_status(state) : fill_status_text(state));
    lv_bar_set_value(fp->levelBar, machineFillLevelPct(), LV_ANIM_ON);

    /* Metered volume + flow only for the WB inlet (the only tank with a flow
     * meter); the chem container and manual WB have no flow reading. */
    if (!s_manual)
        lv_label_set_text_fmt(fp->flowLabel, fillInfo_fmt,
                              (unsigned long)machineFillVolumeMl(), machineFillFlowLpm());

    if (state != FILL_RUNNING) {
        s_phase = FP_DONE;
        fill_set_button(fp->actionButton, fp->actionButtonLabel, fillStart_text, GREEN_DARK);
    }
}

/* Run (ready/done) ↔ Stop (running). */
static void event_fillAction(lv_event_t *e)
{
    (void)e;
    struct sFillPopup *fp = &gui.element.fillPopup;

    if (s_phase == FP_RUNNING) {
        machineFillStop();    /* Stop → the fill ends and the button returns to Run */
    } else {                  /* READY or DONE → (re)start the fill */
        s_phase = FP_RUNNING;
        machineFillStart(s_target);   /* WB (valve/meter/floats) or chem (pump WB→C1) */
        lv_label_set_text(fp->statusLabel,
                          s_manual ? fill_manual_status(FILL_RUNNING) : fill_status_text(FILL_RUNNING));
        fill_set_button(fp->actionButton, fp->actionButtonLabel, fillStop_text, RED_DARK);
    }
}

/* Cancel: leave the popup at any time (stops the fill first). */
static void event_fillCancel(lv_event_t *e)
{
    (void)e;
    if (s_phase == FP_RUNNING) machineFillStop();   /* close the valve on the way out */
    fill_popup_close();
}

void fillPopupCreate(uint8_t target)
{
    struct sFillPopup *fp = &gui.element.fillPopup;
    const ui_roller_popup_layout_t *ui = &ui_get_profile()->roller_popup;

    if (fp->parent != NULL) {
        LV_LOG_USER("Fill popup already open, skipping duplicate");
        return;
    }

    s_target = target;
    machineFillSetTarget(target);   /* so the target-aware helpers below are correct */

    createPopupBackdrop(&fp->parent, &fp->container, FILL_POPUP_W, FILL_POPUP_H);

    /* Mode: with the water inlet connected we meter volume via the flow sensor;
     * without it the bath is filled by hand and we use the floats only. */
    s_manual = machineFillManual();

    /* If the bath is already full, skip Start and show "full" straight away. */
    bool alreadyFull = machineFillBathFull();
    s_phase = alreadyFull ? FP_DONE : FP_READY;

    /* Title + underline */
    fp->title = lv_label_create(fp->container);
    lv_label_set_text(fp->title, (target == FILL_TARGET_CHEM) ? fillChemPopupTitle_text : fillPopupTitle_text);
    lv_obj_set_style_text_font(fp->title, ui->title_font, 0);
    lv_obj_align(fp->title, LV_ALIGN_TOP_MID, 0, ui->title_y);

    lv_style_init(&fp->style_titleLine);
    lv_style_set_line_width(&fp->style_titleLine, ui_get_profile()->title_line_width);
    lv_style_set_line_rounded(&fp->style_titleLine, true);
    fp->titleLinePoints[0].x = 0; fp->titleLinePoints[0].y = 0;
    fp->titleLinePoints[1].x = ui_get_profile()->popups.roller_title_line_w; fp->titleLinePoints[1].y = 0;
    lv_obj_t *line = lv_line_create(fp->container);
    lv_line_set_points(line, fp->titleLinePoints, 2);
    lv_obj_add_style(line, &fp->style_titleLine, 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, ui->title_line_y);

    /* Live status */
    fp->statusLabel = lv_label_create(fp->container);
    lv_obj_set_style_text_font(fp->statusLabel, ui->confirm_btn_font, 0);
    lv_obj_set_width(fp->statusLabel, FILL_POPUP_W - 80);
    lv_obj_set_style_text_align(fp->statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(fp->statusLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(fp->statusLabel, alreadyFull ? fillStatusFull_text : fillStatusReady_text);
    lv_obj_align(fp->statusLabel, LV_ALIGN_CENTER, 0, FILL_STATUS_Y);

    /* Water-bath level bargraph */
    lv_style_init(&fp->style_barIndic);
    lv_style_set_bg_opa(&fp->style_barIndic, LV_OPA_COVER);
    lv_style_set_bg_color(&fp->style_barIndic, lv_color_hex(ORANGE));
    lv_style_set_radius(&fp->style_barIndic, LV_RADIUS_CIRCLE);

    fp->levelBar = lv_bar_create(fp->container);
    lv_obj_set_size(fp->levelBar, FILL_BAR_W, FILL_BAR_H);
    lv_obj_align(fp->levelBar, LV_ALIGN_CENTER, 0, FILL_BAR_Y);
    lv_bar_set_range(fp->levelBar, 0, 100);
    lv_bar_set_value(fp->levelBar, alreadyFull ? 100 : machineFillLevelPct(), LV_ANIM_OFF);
    lv_obj_set_style_bg_opa(fp->levelBar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(fp->levelBar, lv_color_hex(0x3A3A3A), LV_PART_MAIN);
    lv_obj_set_style_radius(fp->levelBar, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_add_style(fp->levelBar, &fp->style_barIndic, LV_PART_INDICATOR);

    /* Metered volume + live inlet flow rate (from the flow meter). Not shown in
     * manual mode, where there is no flow meter reading. */
    fp->flowLabel = lv_label_create(fp->container);
    lv_obj_set_style_text_font(fp->flowLabel, ui->confirm_btn_font, 0);
    lv_label_set_text_fmt(fp->flowLabel, fillInfo_fmt, 0UL, 0.0f);
    lv_obj_align_to(fp->flowLabel, fp->levelBar, LV_ALIGN_OUT_BOTTOM_MID, 0, 16);
    if (s_manual) lv_obj_add_flag(fp->flowLabel, LV_OBJ_FLAG_HIDDEN);

    /* Bottom buttons — same size/font/position/colours as the Clean popup:
     * Cancel (left, red) closes; Run (right, green) starts and becomes Stop. */
    const lv_font_t *btnFont = ui_get_profile()->clean_popup.button_font;

    lv_obj_t *cancelBtn = lv_button_create(fp->container);
    lv_obj_set_size(cancelBtn, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(cancelBtn, LV_ALIGN_BOTTOM_LEFT, FILL_BTN_MX, FILL_BTN_Y);
    lv_obj_set_style_bg_color(cancelBtn, lv_color_hex(RED_DARK), LV_PART_MAIN);
    lv_obj_add_event_cb(cancelBtn, event_fillCancel, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancelLbl = lv_label_create(cancelBtn);
    lv_label_set_text(cancelLbl, fillCancel_text);
    lv_obj_set_style_text_font(cancelLbl, btnFont, 0);
    lv_obj_align(cancelLbl, LV_ALIGN_CENTER, 0, 0);

    fp->actionButton = lv_button_create(fp->container);
    lv_obj_set_size(fp->actionButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(fp->actionButton, LV_ALIGN_BOTTOM_RIGHT, -FILL_BTN_MX, FILL_BTN_Y);
    lv_obj_add_event_cb(fp->actionButton, event_fillAction, LV_EVENT_CLICKED, NULL);

    fp->actionButtonLabel = lv_label_create(fp->actionButton);
    lv_obj_set_style_text_font(fp->actionButtonLabel, btnFont, 0);
    lv_obj_align(fp->actionButtonLabel, LV_ALIGN_CENTER, 0, 0);
    fill_set_button(fp->actionButton, fp->actionButtonLabel, fillStart_text, GREEN_DARK);

    /* Do NOT start filling yet — wait for the user to press Start.
     * The timer only refreshes the live readout once filling is running. */
    fp->liveTimer = lv_timer_create(fill_live_cb, 500, NULL);
}
