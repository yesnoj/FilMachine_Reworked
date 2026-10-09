/**
 * @file element_loadPopup.c
 *
 * Tools → "Load film" popup. Front end of the film loader (main/film_loader.c):
 * choose 135 or 120, press Start; the reel motor winds the film, the Hall
 * sensor counts the reel turns, at the end of the roll the blade cuts and the
 * tail is pulled in. Live status + a turns bar.
 *
 * Bottom buttons: Cancel/Close (left, red) · "Test blade" when idle, "Cut now"
 * while loading (centre, orange) · Start ↔ Stop (right, green/red).
 * The app (WebSocket load_start / load_stop / load_cut) opens and drives this
 * same popup, so the machine always shows what the app started.
 */

//ESSENTIAL INCLUDES
#include "FilMachine.h"

extern struct gui_components gui;

/* ── Layout ── */
#define LOAD_POPUP_W      600
#define LOAD_POPUP_H      420
#define LOAD_FMT_Y        (-112)  /* format buttons, offset from vertical centre */
#define LOAD_FMT_W        120
#define LOAD_FMT_H        48
#define LOAD_STATUS_Y     (-34)
#define LOAD_BAR_W        440
#define LOAD_BAR_H        22
#define LOAD_BAR_Y        44
#define LOAD_BTN_Y        10
#define LOAD_BTN_MX       15

static uint8_t s_format = LOAD_FMT_135;
static int     s_lastState = -1;
static bool    s_beeped = false;

static const char *load_status_text(int st)
{
    switch (st) {
        case LOAD_WINDING:     return loadStatusWinding_text;
        case LOAD_CUTTING:     return loadStatusCutting_text;
        case LOAD_TAIL:        return loadStatusTail_text;
        case LOAD_DONE:        return loadStatusDone_text;
        case LOAD_STOPPED:     return loadStatusStopped_text;
        case LOAD_ERR_NOSPIN:  return loadErrNoSpin_text;
        case LOAD_ERR_EARLY:   return loadErrEarly_text;
        case LOAD_ERR_LONG:    return loadErrLong_text;
        case LOAD_ERR_TIMEOUT: return loadErrTimeout_text;
        default:               return loadStatusReady_text;
    }
}

static bool load_running(int st) { return st == LOAD_WINDING || st == LOAD_CUTTING || st == LOAD_TAIL; }
static bool load_error(int st)   { return st >= LOAD_ERR_NOSPIN; }

static void set_button(lv_obj_t *btn, lv_obj_t *lbl, const char *text, uint32_t color)
{
    lv_label_set_text(lbl, text);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN);
}

static void style_format_buttons(bool enabled)
{
    struct sLoadPopup *lp = &gui.element.loadPopup;
    for (int i = 0; i < 2; i++) {
        bool sel = (i == s_format);
        lv_obj_set_style_bg_color(lp->fmtButton[i], lv_color_hex(sel ? LIGHT_BLUE_DARK : 0x3A3A3A), LV_PART_MAIN);
        lv_obj_set_style_border_width(lp->fmtButton[i], sel ? 3 : 0, LV_PART_MAIN);
        lv_obj_set_style_border_color(lp->fmtButton[i], lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);
        if (enabled) lv_obj_remove_state(lp->fmtButton[i], LV_STATE_DISABLED);
        else         lv_obj_add_state(lp->fmtButton[i], LV_STATE_DISABLED);
    }
}

/* Refresh everything from the loader state (2x per second while open). */
static void load_refresh(void)
{
    struct sLoadPopup *lp = &gui.element.loadPopup;
    if (lp->parent == NULL) return;

    int st = filmLoaderState();
    bool running = load_running(st);
    uint16_t p = filmLoaderPulses(), exp = filmLoaderExpectedPulses();

    lv_label_set_text(lp->statusLabel, load_status_text(st));
    lv_obj_set_style_text_color(lp->statusLabel,
        lv_color_hex(load_error(st) ? RED : (st == LOAD_DONE ? GREEN_LIGHT : WHITE)), 0);

    int pct = exp ? (int)p * 100 / exp : 0;
    if (pct > 100 || st == LOAD_DONE) pct = 100;
    lv_bar_set_value(lp->bar, (st == LOAD_IDLE) ? 0 : pct, LV_ANIM_ON);
    lv_label_set_text_fmt(lp->turnsLabel, loadTurns_text,
                          (unsigned)(p / LOAD_PULSES_PER_TURN),
                          (unsigned)((p % LOAD_PULSES_PER_TURN) * 10 / LOAD_PULSES_PER_TURN),
                          (unsigned)(exp / LOAD_PULSES_PER_TURN));

    if (running) {
        set_button(lp->actionButton, lp->actionButtonLabel, fillStop_text, RED_DARK);
        lv_label_set_text(lp->midButtonLabel, loadCutNow_text);
        if (st == LOAD_WINDING) lv_obj_remove_state(lp->midButton, LV_STATE_DISABLED);
        else                    lv_obj_add_state(lp->midButton, LV_STATE_DISABLED);
        lv_label_set_text(lp->cancelButtonLabel, fillCancel_text);
    } else {
        set_button(lp->actionButton, lp->actionButtonLabel, fillStart_text, GREEN_DARK);
        lv_label_set_text(lp->midButtonLabel, loadTestBlade_text);
        if (filmLoaderServoActive()) lv_obj_add_state(lp->midButton, LV_STATE_DISABLED);
        else                         lv_obj_remove_state(lp->midButton, LV_STATE_DISABLED);
        lv_label_set_text(lp->cancelButtonLabel, fillClose_text);
    }
    style_format_buttons(!running);

    /* one beep when the tank is ready */
    if (st != s_lastState) {
        if (st == LOAD_DONE && !s_beeped) { s_beeped = true; buzzer_beep(); }
        s_lastState = st;
    }
}

static void load_live_cb(lv_timer_t *t)
{
    (void)t;
    load_refresh();
}

static void load_popup_close(void)
{
    struct sLoadPopup *lp = &gui.element.loadPopup;
    if (lp->liveTimer) { lv_timer_delete(lp->liveTimer); lp->liveTimer = NULL; }
    lv_style_reset(&lp->style_titleLine);
    lv_style_reset(&lp->style_barIndic);
    lv_msgbox_close(lp->parent);
    lp->parent = NULL;
}

static void load_start(void)
{
    s_beeped = false;
    filmLoaderStart(s_format);
    load_refresh();
}

/* ── Events ── */
static void event_loadFormat(lv_event_t *e)
{
    struct sLoadPopup *lp = &gui.element.loadPopup;
    lv_obj_t *obj = lv_event_get_target(e);
    if (filmLoaderBusy()) return;
    s_format = (obj == lp->fmtButton[1]) ? LOAD_FMT_120 : LOAD_FMT_135;
    style_format_buttons(true);
}

static void event_loadAction(lv_event_t *e)
{
    (void)e;
    if (filmLoaderBusy()) filmLoaderStop();
    else                  load_start();
    load_refresh();
}

static void event_loadMid(lv_event_t *e)
{
    (void)e;
    if (filmLoaderBusy()) filmLoaderCutNow();
    else                  filmLoaderCutterCycle();
    load_refresh();
}

static void event_loadCancel(lv_event_t *e)
{
    (void)e;
    if (filmLoaderBusy()) filmLoaderStop();     /* motor off, blade back down */
    load_popup_close();
}

/* ── Remote control (app over WebSocket) ── */
void loadPopupRemoteStart(uint8_t format)
{
    struct sLoadPopup *lp = &gui.element.loadPopup;
    if (filmLoaderBusy()) { LV_LOG_USER("Remote load_start ignored: already loading"); return; }
    if (lp->parent == NULL) loadPopupCreate();
    s_format = (format == LOAD_FMT_120) ? LOAD_FMT_120 : LOAD_FMT_135;
    if (lp->parent != NULL) style_format_buttons(true);
    load_start();       /* even if another popup owns the screen, the loader runs */
}

void loadPopupRemoteStop(void)
{
    filmLoaderStop();
    load_refresh();
}

void loadPopupRemoteCut(void)
{
    filmLoaderCutNow();
    load_refresh();
}

static lv_obj_t *bottom_button(lv_obj_t *parent, lv_align_t align, int32_t x, uint32_t color,
                               lv_event_cb_t cb, lv_obj_t **label)
{
    const lv_font_t *btnFont = ui_get_profile()->clean_popup.button_font;
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(b, align, x, LOAD_BTN_Y);
    lv_obj_set_style_bg_color(b, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    *label = lv_label_create(b);
    lv_obj_set_style_text_font(*label, btnFont, 0);
    lv_obj_align(*label, LV_ALIGN_CENTER, 0, 0);
    return b;
}

void loadPopupCreate(void)
{
    struct sLoadPopup *lp = &gui.element.loadPopup;
    const ui_roller_popup_layout_t *ui = &ui_get_profile()->roller_popup;

    if (lp->parent != NULL) {
        LV_LOG_USER("Load popup already open, skipping duplicate");
        return;
    }
    filmLoaderInit();
    if (!filmLoaderBusy()) s_format = (uint8_t)filmLoaderFormat();
    s_lastState = filmLoaderState();
    s_beeped = (s_lastState == LOAD_DONE);

    createPopupBackdrop(&lp->parent, &lp->container, LOAD_POPUP_W, LOAD_POPUP_H);

    /* Title + underline */
    lp->title = lv_label_create(lp->container);
    lv_label_set_text(lp->title, loadPopupTitle_text);
    lv_obj_set_style_text_font(lp->title, ui->title_font, 0);
    lv_obj_align(lp->title, LV_ALIGN_TOP_MID, 0, ui->title_y);

    lv_style_init(&lp->style_titleLine);
    lv_style_set_line_width(&lp->style_titleLine, ui_get_profile()->title_line_width);
    lv_style_set_line_rounded(&lp->style_titleLine, true);
    lp->titleLinePoints[0].x = 0; lp->titleLinePoints[0].y = 0;
    lp->titleLinePoints[1].x = ui_get_profile()->popups.roller_title_line_w; lp->titleLinePoints[1].y = 0;
    lv_obj_t *line = lv_line_create(lp->container);
    lv_line_set_points(line, lp->titleLinePoints, 2);
    lv_obj_add_style(line, &lp->style_titleLine, 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, ui->title_line_y);

    /* Format: 135 / 120 */
    static const char *fmtText[2] = { "135", "120" };
    for (int i = 0; i < 2; i++) {
        lp->fmtButton[i] = lv_button_create(lp->container);
        lv_obj_set_size(lp->fmtButton[i], LOAD_FMT_W, LOAD_FMT_H);
        lv_obj_align(lp->fmtButton[i], LV_ALIGN_CENTER, (i == 0 ? -1 : 1) * (LOAD_FMT_W / 2 + 10), LOAD_FMT_Y);
        lv_obj_add_event_cb(lp->fmtButton[i], event_loadFormat, LV_EVENT_CLICKED, NULL);
        lv_obj_t *l = lv_label_create(lp->fmtButton[i]);
        lv_label_set_text(l, fmtText[i]);
        lv_obj_set_style_text_font(l, ui->confirm_btn_font, 0);
        lv_obj_align(l, LV_ALIGN_CENTER, 0, 0);
    }

    /* Live status */
    lp->statusLabel = lv_label_create(lp->container);
    lv_obj_set_style_text_font(lp->statusLabel, ui->confirm_btn_font, 0);
    lv_obj_set_width(lp->statusLabel, LOAD_POPUP_W - 60);
    lv_obj_set_style_text_align(lp->statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(lp->statusLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(lp->statusLabel, LV_ALIGN_CENTER, 0, LOAD_STATUS_Y);

    /* Turns bar */
    lv_style_init(&lp->style_barIndic);
    lv_style_set_bg_opa(&lp->style_barIndic, LV_OPA_COVER);
    lv_style_set_bg_color(&lp->style_barIndic, lv_color_hex(ORANGE));
    lv_style_set_radius(&lp->style_barIndic, LV_RADIUS_CIRCLE);

    lp->bar = lv_bar_create(lp->container);
    lv_obj_set_size(lp->bar, LOAD_BAR_W, LOAD_BAR_H);
    lv_obj_align(lp->bar, LV_ALIGN_CENTER, 0, LOAD_BAR_Y);
    lv_bar_set_range(lp->bar, 0, 100);
    lv_obj_set_style_bg_opa(lp->bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lp->bar, lv_color_hex(0x3A3A3A), LV_PART_MAIN);
    lv_obj_set_style_radius(lp->bar, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_add_style(lp->bar, &lp->style_barIndic, LV_PART_INDICATOR);

    lp->turnsLabel = lv_label_create(lp->container);
    lv_obj_set_style_text_font(lp->turnsLabel, ui->confirm_btn_font, 0);
    lv_obj_set_width(lp->turnsLabel, LOAD_BAR_W);
    lv_obj_set_style_text_align(lp->turnsLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lp->turnsLabel, LV_ALIGN_CENTER, 0, LOAD_BAR_Y + LOAD_BAR_H / 2 + 22);

    /* Bottom buttons */
    lv_obj_t *cancelBtn = bottom_button(lp->container, LV_ALIGN_BOTTOM_LEFT, LOAD_BTN_MX, RED_DARK, event_loadCancel, &lp->cancelButtonLabel);
    (void)cancelBtn;
    lp->midButton    = bottom_button(lp->container, LV_ALIGN_BOTTOM_MID, 0, ORANGE_DARK, event_loadMid, &lp->midButtonLabel);
    lp->actionButton = bottom_button(lp->container, LV_ALIGN_BOTTOM_RIGHT, -LOAD_BTN_MX, GREEN_DARK, event_loadAction, &lp->actionButtonLabel);

    load_refresh();
    lp->liveTimer = lv_timer_create(load_live_cb, 500, NULL);
}
