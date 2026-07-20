/**
 * @file element_speedPopup.c
 *
 * Dedicated popup to tune Pump / Motor speed. Styled to match the temperature roller.
 *   - roller to pick a percentage (10..100 %, step 5)
 *   - a "Test" switch that runs the pump/motor live at the selected speed
 *   - a "SET" button that stops the motor/pump, saves the value, and shows a confirmation popup
 *
 * Uses the non-blocking pump_set / motor_set helpers (safe from LVGL callbacks).
 */

//ESSENTIAL INCLUDES
#include "FilMachine.h"

extern struct gui_components gui;
extern struct sys_components sys;

/* ── Layout (adjust here if rows are too tight/loose) ── */
#define SPEED_ROLLER_TO_SWITCH   12   /* gap roller -> Test switch */
#define SPEED_SWITCH_TO_SET      14   /* gap Test switch -> SET button */
#define SPEED_POPUP_BOTTOM       96   /* Test row + gaps + margins (SET button height added separately) */
#define TUNE_BTN_MX              15   /* button side margin (matches Clean popup) */
#define TUNE_BTN_MY              10   /* button bottom margin (matches Clean popup) */

/* Apply the currently selected speed to the real hardware (live test). */
static void speed_apply_live(void)
{
    struct sSpeedPopup *sp = &gui.element.speedPopup;
    if (sp->isPump)
        pump_set_forward(pumpPercentToDuty(sp->percent));
    else
        motor_set_forward(mapPercentageToValue(sp->percent, 10, 100));
}

/* Stop the pump/motor. */
static void speed_stop(void)
{
    struct sSpeedPopup *sp = &gui.element.speedPopup;
    if (sp->isPump)
        pump_set_stop();
    else
        motor_set_stop();
}

/* Close and destroy the popup (always stops the hardware first). */
static void speed_popup_close(void)
{
    struct sSpeedPopup *sp = &gui.element.speedPopup;
    speed_stop();
    lv_style_reset(&sp->style_titleLine);
    lv_style_reset(&sp->style_roller);
    lv_msgbox_close(sp->parent);
    sp->parent = NULL;
}

/* Roller moved: update selected %, and if the test switch is on, apply it live. */
static void event_speedRoller(lv_event_t *e)
{
    (void)e;
    struct sSpeedPopup *sp = &gui.element.speedPopup;
    uint32_t idx = lv_roller_get_selected(sp->roller);
    sp->percent = (uint8_t)(10 + idx * 5);
    if (lv_obj_has_state(sp->testSwitch, LV_STATE_CHECKED))
        speed_apply_live();
}

/* Test switch toggled: start or stop the live run. */
static void event_speedTestSwitch(lv_event_t *e)
{
    (void)e;
    struct sSpeedPopup *sp = &gui.element.speedPopup;
    if (lv_obj_has_state(sp->testSwitch, LV_STATE_CHECKED)) {
        if (!sp->isPump)
            motor_start_kicked(true, mapPercentageToValue(sp->percent, 10, 100));  /* breakaway kick from rest */
        else
            speed_apply_live();
    } else {
        speed_stop();
    }
}

/* SET pressed: stop, save, refresh the settings row, persist, close, then confirm. */
static void event_speedSet(lv_event_t *e)
{
    (void)e;
    struct sSpeedPopup *sp = &gui.element.speedPopup;

    bool    isPump  = sp->isPump;
    uint8_t percent = sp->percent;

    speed_stop();

    if (isPump) {
        gui.page.settings.settingsParams.pumpSpeed = percent;
    } else {
        gui.page.settings.settingsParams.filmRotationSpeedSetpoint = percent;
        sys.analogVal_rotationSpeedPercent = mapPercentageToValue(percent, 10, 100);
    }

    if (sp->targetValueLabel)
        lv_label_set_text_fmt(sp->targetValueLabel, "%d%%", percent);

    LV_LOG_USER("Speed SET: %s = %d%%", isPump ? "pump" : "motor", percent);
    qSysAction(SAVE_PROCESS_CONFIG);

    speed_popup_close();

    /* Confirmation popup — same approach as the temperature calibration SET. */
    {
        static char msg[64];
        snprintf(msg, sizeof(msg), "%s set to %d%%",
                 isPump ? pumpSpeed_text : rotationSpeed_text, percent);
        messagePopupCreate(speedSetPopupTitle_text, msg, NULL, NULL, NULL);
    }
}

/* Cancel: stop the hardware (in case Test was on) and close without saving. */
static void event_speedCancel(lv_event_t *e)
{
    (void)e;
    speed_popup_close();
}

void speedPopupCreate(bool isPump, uint8_t currentPercent)
{
    struct sSpeedPopup *sp = &gui.element.speedPopup;
    const ui_roller_popup_layout_t *ui = &ui_get_profile()->roller_popup;
    const int      rollerH = ui_get_profile()->popups.roller_wheel_h;
    const uint32_t accent  = ORANGE;

    /* Guard: only one popup at a time. */
    if (sp->parent != NULL) {
        LV_LOG_USER("Speed popup already open, skipping duplicate");
        return;
    }

    /* Clamp + snap to step-5 grid. */
    if (currentPercent < 10)  currentPercent = 10;
    if (currentPercent > 100) currentPercent = 100;
    currentPercent = (uint8_t)(((currentPercent - 10) / 5) * 5 + 10);

    sp->isPump  = isPump;
    sp->percent = currentPercent;
    sp->targetValueLabel = isPump ? gui.page.settings.pumpSpeedValueLabel
                                  : gui.page.settings.filmRotationSpeedValueLabel;

    /* Build roller options "10%\n15%\n...\n100%". */
    {
        char  *p   = sp->options;
        size_t rem = sizeof(sp->options);
        for (int v = 10; v <= 100; v += 5) {
            int n = snprintf(p, rem, "%d%%%s", v, (v < 100) ? "\n" : "");
            if (n < 0 || (size_t)n >= rem) break;
            p += n;
            rem -= (size_t)n;
        }
    }

    /* Popup height sized to the content: title area + roller + Test row + SET. */
    createPopupBackdrop(&sp->parent, &sp->container,
                        ui_get_profile()->popups.roller_w,
                        ui->wheel_container_y + rollerH + BUTTON_MBOX_HEIGHT + SPEED_POPUP_BOTTOM);

    /* Title */
    sp->title = lv_label_create(sp->container);
    lv_label_set_text(sp->title, isPump ? pumpSpeed_text : rotationSpeed_text);
    lv_obj_set_style_text_font(sp->title, ui->title_font, 0);
    lv_obj_align(sp->title, LV_ALIGN_TOP_MID, ui->title_x, ui->title_y);

    /* Title underline (same style as the temperature popup) */
    lv_style_init(&sp->style_titleLine);
    lv_style_set_line_width(&sp->style_titleLine, ui_get_profile()->title_line_width);
    lv_style_set_line_rounded(&sp->style_titleLine, true);
    sp->titleLinePoints[0].x = 0;
    sp->titleLinePoints[0].y = 0;
    sp->titleLinePoints[1].x = ui_get_profile()->popups.roller_title_line_w;
    sp->titleLinePoints[1].y = 0;
    lv_obj_t *titleLine = lv_line_create(sp->container);
    lv_line_set_points(titleLine, sp->titleLinePoints, 2);
    lv_obj_add_style(titleLine, &sp->style_titleLine, 0);
    lv_obj_align(titleLine, LV_ALIGN_TOP_MID, ui->title_line_x, ui->title_line_y);

    /* Roller — styled like the temperature roller (big selected font, accent bar, white box).
       Placed directly in the container (no padding wrapper) so the Test row can hug it. */
    lv_style_init(&sp->style_roller);
    lv_style_set_text_font(&sp->style_roller, ui->wheel_font);
    lv_style_set_bg_color(&sp->style_roller, lv_color_hex(accent));
    lv_style_set_border_width(&sp->style_roller, ui_get_profile()->title_line_width);
    lv_style_set_border_color(&sp->style_roller, lv_color_hex(accent));

    sp->roller = lv_roller_create(sp->container);
    lv_roller_set_options(sp->roller, sp->options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(sp->roller, ui->wheel_visible_rows);
    lv_obj_set_width(sp->roller, ui->wheel_w);
    lv_obj_set_height(sp->roller, rollerH);
    lv_obj_align(sp->roller, LV_ALIGN_TOP_MID, ui->wheel_container_x, ui->wheel_container_y);
    lv_roller_set_selected(sp->roller, (uint16_t)((currentPercent - 10) / 5), LV_ANIM_OFF);
    lv_obj_add_event_cb(sp->roller, event_speedRoller, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_style(sp->roller, &sp->style_roller, LV_PART_SELECTED);
    lv_obj_set_style_text_font(sp->roller, ui->wheel_normal_font, LV_PART_MAIN);
    lv_obj_set_style_border_color(sp->roller, lv_color_hex(WHITE), LV_PART_MAIN);

    /* Test switch — hugs the roller bottom + label to its left */
    sp->testSwitch = lv_switch_create(sp->container);
    lv_obj_align_to(sp->testSwitch, sp->roller, LV_ALIGN_OUT_BOTTOM_MID, 24, SPEED_ROLLER_TO_SWITCH);
    lv_obj_add_event_cb(sp->testSwitch, event_speedTestSwitch, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *testLbl = lv_label_create(sp->container);
    lv_label_set_text(testLbl, speedTestSwitch_text);
    lv_obj_set_style_text_font(testLbl, ui->title_font, 0);
    lv_obj_align_to(testLbl, sp->testSwitch, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    /* Bottom buttons — same size/font/position/colours as the Clean popup:
       Cancel (left, red) closes without saving; Set (right, green) applies. */
    const lv_font_t *btnFont = ui_get_profile()->clean_popup.button_font;

    lv_obj_t *cancelBtn = lv_button_create(sp->container);
    lv_obj_set_size(cancelBtn, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(cancelBtn, LV_ALIGN_BOTTOM_LEFT, TUNE_BTN_MX, TUNE_BTN_MY);
    lv_obj_set_style_bg_color(cancelBtn, lv_color_hex(RED_DARK), LV_PART_MAIN);
    lv_obj_add_event_cb(cancelBtn, event_speedCancel, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancelLbl = lv_label_create(cancelBtn);
    lv_label_set_text(cancelLbl, buttonCancel_text);
    lv_obj_set_style_text_font(cancelLbl, btnFont, 0);
    lv_obj_align(cancelLbl, LV_ALIGN_CENTER, 0, 0);

    sp->setButton = lv_button_create(sp->container);
    lv_obj_set_size(sp->setButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(sp->setButton, LV_ALIGN_BOTTOM_RIGHT, -TUNE_BTN_MX, TUNE_BTN_MY);
    lv_obj_set_style_bg_color(sp->setButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
    lv_obj_add_event_cb(sp->setButton, event_speedSet, LV_EVENT_CLICKED, NULL);

    lv_obj_t *setLbl = lv_label_create(sp->setButton);
    lv_label_set_text(setLbl, tuneRollerButton_text);   /* "Set" */
    lv_obj_set_style_text_font(setLbl, btnFont, 0);
    lv_obj_align(setLbl, LV_ALIGN_CENTER, 0, 0);
}

/* ═══════════════════════════════════════════════════════════════
 *  Volume popup — same roller styling, no Test switch.
 *  The tone plays on every roller change so you hear the volume.
 *  Reuses the shared speedPopup struct (only one popup open at a time).
 * ═══════════════════════════════════════════════════════════════ */
#ifndef SIMULATOR_BUILD
#include "audio.h"
#endif

static void volume_apply(uint8_t pct)
{
#ifndef SIMULATOR_BUILD
    audio_set_volume(pct);
    audio_play_tone(1000, 90);
#else
    (void)pct;   /* simulator: no ES8311, popup is silent */
#endif
}

static void event_volumeRoller(lv_event_t *e)
{
    (void)e;
    struct sSpeedPopup *sp = &gui.element.speedPopup;
    uint32_t idx = lv_roller_get_selected(sp->roller);
    sp->percent = (uint8_t)(idx * 5);          /* 0..100 % step 5 */
    volume_apply(sp->percent);
}

static void event_volumeSet(lv_event_t *e)
{
    (void)e;
    struct sSpeedPopup *sp = &gui.element.speedPopup;
    uint8_t percent = sp->percent;

    gui.page.settings.settingsParams.volume = percent;
    if (sp->targetValueLabel)
        lv_label_set_text_fmt(sp->targetValueLabel, "%d%%", percent);

    LV_LOG_USER("Volume SET: %d%%", percent);
    qSysAction(SAVE_PROCESS_CONFIG);

#ifndef SIMULATOR_BUILD
    audio_set_volume(percent);   /* keep the saved level active */
    audio_stop();                /* silence the preview tone on close */
#endif

    lv_style_reset(&sp->style_titleLine);
    lv_style_reset(&sp->style_roller);
    lv_msgbox_close(sp->parent);
    sp->parent = NULL;

    {
        static char msg[48];
        snprintf(msg, sizeof(msg), "Volume set to %d%%", percent);
        messagePopupCreate(volumeSetPopupTitle_text, msg, NULL, NULL, NULL);
    }
}

/* Cancel: restore the real output volume to the saved value (the roller
 * previews it live) and close without saving. */
static void event_volumeCancel(lv_event_t *e)
{
    (void)e;
    struct sSpeedPopup *sp = &gui.element.speedPopup;
#ifndef SIMULATOR_BUILD
    audio_set_volume(gui.page.settings.settingsParams.volume);
    audio_stop();                /* silence the preview tone on close */
#endif
    lv_style_reset(&sp->style_titleLine);
    lv_style_reset(&sp->style_roller);
    lv_msgbox_close(sp->parent);
    sp->parent = NULL;
}

void volumePopupCreate(uint8_t currentPercent)
{
    struct sSpeedPopup *sp = &gui.element.speedPopup;
    const ui_roller_popup_layout_t *ui = &ui_get_profile()->roller_popup;
    const int      rollerH = ui_get_profile()->popups.roller_wheel_h;
    const uint32_t accent  = ORANGE;

    if (sp->parent != NULL) {
        LV_LOG_USER("Volume popup already open, skipping duplicate");
        return;
    }

    if (currentPercent > 100) currentPercent = 100;
    currentPercent = (uint8_t)((currentPercent / 5) * 5);   /* snap to step 5 */

    sp->percent = currentPercent;
    sp->targetValueLabel = gui.page.settings.volumeValueLabel;

    /* Build roller options "0%\n5%\n...\n100%". */
    {
        char  *p   = sp->options;
        size_t rem = sizeof(sp->options);
        for (int v = 0; v <= 100; v += 5) {
            int n = snprintf(p, rem, "%d%%%s", v, (v < 100) ? "\n" : "");
            if (n < 0 || (size_t)n >= rem) break;
            p += n; rem -= (size_t)n;
        }
    }

    createPopupBackdrop(&sp->parent, &sp->container,
                        ui_get_profile()->popups.roller_w,
                        ui->wheel_container_y + rollerH + BUTTON_MBOX_HEIGHT + 60);

    sp->title = lv_label_create(sp->container);
    lv_label_set_text(sp->title, volume_text);
    lv_obj_set_style_text_font(sp->title, ui->title_font, 0);
    lv_obj_align(sp->title, LV_ALIGN_TOP_MID, ui->title_x, ui->title_y);

    lv_style_init(&sp->style_titleLine);
    lv_style_set_line_width(&sp->style_titleLine, ui_get_profile()->title_line_width);
    lv_style_set_line_rounded(&sp->style_titleLine, true);
    sp->titleLinePoints[0].x = 0; sp->titleLinePoints[0].y = 0;
    sp->titleLinePoints[1].x = ui_get_profile()->popups.roller_title_line_w; sp->titleLinePoints[1].y = 0;
    lv_obj_t *titleLine = lv_line_create(sp->container);
    lv_line_set_points(titleLine, sp->titleLinePoints, 2);
    lv_obj_add_style(titleLine, &sp->style_titleLine, 0);
    lv_obj_align(titleLine, LV_ALIGN_TOP_MID, ui->title_line_x, ui->title_line_y);

    lv_style_init(&sp->style_roller);
    lv_style_set_text_font(&sp->style_roller, ui->wheel_font);
    lv_style_set_bg_color(&sp->style_roller, lv_color_hex(accent));
    lv_style_set_border_width(&sp->style_roller, ui_get_profile()->title_line_width);
    lv_style_set_border_color(&sp->style_roller, lv_color_hex(accent));

    sp->roller = lv_roller_create(sp->container);
    lv_roller_set_options(sp->roller, sp->options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(sp->roller, ui->wheel_visible_rows);
    lv_obj_set_width(sp->roller, ui->wheel_w);
    lv_obj_set_height(sp->roller, rollerH);
    lv_obj_align(sp->roller, LV_ALIGN_TOP_MID, ui->wheel_container_x, ui->wheel_container_y);
    lv_roller_set_selected(sp->roller, (uint16_t)(currentPercent / 5), LV_ANIM_OFF);
    lv_obj_add_event_cb(sp->roller, event_volumeRoller, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_style(sp->roller, &sp->style_roller, LV_PART_SELECTED);
    lv_obj_set_style_text_font(sp->roller, ui->wheel_normal_font, LV_PART_MAIN);
    lv_obj_set_style_border_color(sp->roller, lv_color_hex(WHITE), LV_PART_MAIN);

    /* Bottom buttons — same size/font/position/colours as the Clean popup. */
    const lv_font_t *btnFont = ui_get_profile()->clean_popup.button_font;

    lv_obj_t *volCancelBtn = lv_button_create(sp->container);
    lv_obj_set_size(volCancelBtn, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(volCancelBtn, LV_ALIGN_BOTTOM_LEFT, TUNE_BTN_MX, TUNE_BTN_MY);
    lv_obj_set_style_bg_color(volCancelBtn, lv_color_hex(RED_DARK), LV_PART_MAIN);
    lv_obj_add_event_cb(volCancelBtn, event_volumeCancel, LV_EVENT_CLICKED, NULL);
    lv_obj_t *volCancelLbl = lv_label_create(volCancelBtn);
    lv_label_set_text(volCancelLbl, buttonCancel_text);
    lv_obj_set_style_text_font(volCancelLbl, btnFont, 0);
    lv_obj_align(volCancelLbl, LV_ALIGN_CENTER, 0, 0);

    sp->setButton = lv_button_create(sp->container);
    lv_obj_set_size(sp->setButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
    lv_obj_align(sp->setButton, LV_ALIGN_BOTTOM_RIGHT, -TUNE_BTN_MX, TUNE_BTN_MY);
    lv_obj_set_style_bg_color(sp->setButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
    lv_obj_add_event_cb(sp->setButton, event_volumeSet, LV_EVENT_CLICKED, NULL);

    lv_obj_t *volSetLbl = lv_label_create(sp->setButton);
    lv_label_set_text(volSetLbl, tuneRollerButton_text);   /* "Set" */
    lv_obj_set_style_text_font(volSetLbl, btnFont, 0);
    lv_obj_align(volSetLbl, LV_ALIGN_CENTER, 0, 0);
}
