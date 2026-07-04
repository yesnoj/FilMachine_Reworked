/**
 * @file element_calibPopup.c
 *
 * Dual temperature calibration popup: two side-by-side rollers (Bath | Chemical),
 * each with the live sensor reading shown above it. SET computes a per-sensor
 * offset = (true temp you set) - (raw sensor reading) and stores them
 * (bath offset -> config, chemical offset -> NVS).
 */

//ESSENTIAL INCLUDES
#include "FilMachine.h"

extern struct gui_components gui;

/* ── Layout (tweak here if columns/roller/button don't sit right) ── */
#define CALIB_POPUP_W     600
#define CALIB_POPUP_H     420
#define CALIB_COL_DX      140    /* horizontal offset of each column from centre */
#define CALIB_NAME_Y      56     /* Y of the "Bath"/"Chemical" name labels */
#define CALIB_ROLLER_Y    96     /* Y of the rollers (moved up to free the bottom) */
#define CALIB_ROLLER_W    150
#define CALIB_ROLLER_H    165
#define CALIB_READ_DY     12     /* gap roller -> current-temperature label below it */
#define CALIB_SET_Y       (-16)  /* SET button offset from bottom */

static void set_read_label(lv_obj_t *lbl, float t)
{
    if (t > -100.0f) lv_label_set_text_fmt(lbl, "%.1f C", t);
    else             lv_label_set_text(lbl, "--");
}

static uint16_t temp_to_sel(float reading)
{
    int t = (reading > -100.0f) ? (int)(reading + 0.5f) : 20;
    if (t < TEMP_ROLLER_MIN) t = TEMP_ROLLER_MIN;
    if (t > TEMP_ROLLER_MAX) t = TEMP_ROLLER_MAX;
    return (uint16_t)(t - TEMP_ROLLER_MIN);
}

static lv_obj_t *make_temp_roller(lv_obj_t *parent, lv_style_t *style, int dx, uint16_t sel)
{
    const ui_roller_popup_layout_t *ui = &ui_get_profile()->roller_popup;

    lv_style_init(style);
    lv_style_set_text_font(style, ui->wheel_font);
    lv_style_set_bg_color(style, lv_color_hex(ORANGE));
    lv_style_set_border_width(style, ui_get_profile()->title_line_width);
    lv_style_set_border_color(style, lv_color_hex(ORANGE));

    lv_obj_t *r = lv_roller_create(parent);
    lv_roller_set_options(r, gui.element.rollerPopup.tempCelsiusOptions, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(r, 3);
    lv_obj_set_width(r, CALIB_ROLLER_W);
    lv_obj_set_height(r, CALIB_ROLLER_H);
    lv_obj_align(r, LV_ALIGN_TOP_MID, dx, CALIB_ROLLER_Y);
    lv_roller_set_selected(r, sel, LV_ANIM_OFF);
    lv_obj_add_style(r, style, LV_PART_SELECTED);
    lv_obj_set_style_text_font(r, ui->wheel_normal_font, LV_PART_MAIN);
    lv_obj_set_style_border_color(r, lv_color_hex(WHITE), LV_PART_MAIN);
    return r;
}

static void calib_popup_close(void)
{
    struct sCalibPopup *cp = &gui.element.calibPopup;
    if (cp->liveTimer) { lv_timer_delete(cp->liveTimer); cp->liveTimer = NULL; }
    lv_style_reset(&cp->style_titleLine);
    lv_style_reset(&cp->style_rollerBath);
    lv_style_reset(&cp->style_rollerChem);
    lv_msgbox_close(cp->parent);
    cp->parent = NULL;
}

/* Refresh the two live readings ~1x/second. */
static void calib_live_cb(lv_timer_t *t)
{
    (void)t;
    struct sCalibPopup *cp = &gui.element.calibPopup;
    if (cp->parent == NULL) return;
    set_read_label(cp->bathReadLabel, getCachedTemperature(TEMPERATURE_SENSOR_BATH));
    set_read_label(cp->chemReadLabel, getCachedTemperature(TEMPERATURE_SENSOR_CHEMICAL));
}

/* SET: compute a per-sensor offset from the true temps entered, save, confirm. */
static void event_calibSet(lv_event_t *e)
{
    (void)e;
    struct sCalibPopup *cp = &gui.element.calibPopup;

    int trueBath = TEMP_ROLLER_MIN + (int)lv_roller_get_selected(cp->rollerBath);
    int trueChem = TEMP_ROLLER_MIN + (int)lv_roller_get_selected(cp->rollerChem);

    float bathReading = getCachedTemperature(TEMPERATURE_SENSOR_BATH);
    float chemReading = getCachedTemperature(TEMPERATURE_SENSOR_CHEMICAL);

    /* Only recalibrate a sensor that is actually reading. */
    if (bathReading > -100.0f) {
        float rawBath = bathReading - (gui.page.settings.settingsParams.tempCalibOffset / 10.0f);
        gui.page.settings.settingsParams.tempCalibOffset = (int8_t)((trueBath - rawBath) * 10.0f);
        gui.page.settings.settingsParams.calibratedTemp  = (uint8_t)trueBath;
    }
    if (chemReading > -100.0f) {
        float rawChem = chemReading - (getChemCalibOffset() / 10.0f);
        setChemCalibOffset((int8_t)((trueChem - rawChem) * 10.0f));   /* saved with the config */
    }

    qSysAction(SAVE_PROCESS_CONFIG);   /* persists the bath offset via the config */

    LV_LOG_USER("Calib SET: bath off=%d chem off=%d (tenths)",
                gui.page.settings.settingsParams.tempCalibOffset, getChemCalibOffset());

    /* Snapshot values before closing (close clears the popup state). */
    float bathOff = gui.page.settings.settingsParams.tempCalibOffset / 10.0f;
    float chemOff = getChemCalibOffset() / 10.0f;

    calib_popup_close();

    {
        static char msg[80];
        snprintf(msg, sizeof(msg), "Bath offset  %+.1f C\nChem offset  %+.1f C", bathOff, chemOff);
        messagePopupCreate(calibrationPopupTitle_text, msg, NULL, NULL, NULL);
    }
}

void calibPopupCreate(void)
{
    struct sCalibPopup *cp = &gui.element.calibPopup;
    const ui_roller_popup_layout_t *ui = &ui_get_profile()->roller_popup;

    if (cp->parent != NULL) {
        LV_LOG_USER("Calib popup already open, skipping duplicate");
        return;
    }

    float bRead = getCachedTemperature(TEMPERATURE_SENSOR_BATH);
    float cRead = getCachedTemperature(TEMPERATURE_SENSOR_CHEMICAL);

    createPopupBackdrop(&cp->parent, &cp->container, CALIB_POPUP_W, CALIB_POPUP_H);

    /* Title + underline */
    cp->title = lv_label_create(cp->container);
    lv_label_set_text(cp->title, calibrationPopupTitle_text);
    lv_obj_set_style_text_font(cp->title, ui->title_font, 0);
    lv_obj_align(cp->title, LV_ALIGN_TOP_MID, 0, ui->title_y);

    lv_style_init(&cp->style_titleLine);
    lv_style_set_line_width(&cp->style_titleLine, ui_get_profile()->title_line_width);
    lv_style_set_line_rounded(&cp->style_titleLine, true);
    cp->titleLinePoints[0].x = 0; cp->titleLinePoints[0].y = 0;
    cp->titleLinePoints[1].x = ui_get_profile()->popups.roller_title_line_w; cp->titleLinePoints[1].y = 0;
    lv_obj_t *line = lv_line_create(cp->container);
    lv_line_set_points(line, cp->titleLinePoints, 2);
    lv_obj_add_style(line, &cp->style_titleLine, 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, ui->title_line_y);

    /* Column name labels (static), moved up */
    lv_obj_t *bathName = lv_label_create(cp->container);
    lv_label_set_text(bathName, calibBath_text);
    lv_obj_set_style_text_font(bathName, ui->title_font, 0);
    lv_obj_align(bathName, LV_ALIGN_TOP_MID, -CALIB_COL_DX, CALIB_NAME_Y);

    lv_obj_t *chemName = lv_label_create(cp->container);
    lv_label_set_text(chemName, calibChem_text);
    lv_obj_set_style_text_font(chemName, ui->title_font, 0);
    lv_obj_align(chemName, LV_ALIGN_TOP_MID, CALIB_COL_DX, CALIB_NAME_Y);

    /* Rollers (true temperature to set for each sensor) */
    cp->rollerBath = make_temp_roller(cp->container, &cp->style_rollerBath, -CALIB_COL_DX, temp_to_sel(bRead));
    cp->rollerChem = make_temp_roller(cp->container, &cp->style_rollerChem,  CALIB_COL_DX, temp_to_sel(cRead));

    /* Current-temperature readings, in the freed space below each roller */
    cp->bathReadLabel = lv_label_create(cp->container);
    lv_obj_set_style_text_font(cp->bathReadLabel, ui->confirm_btn_font, 0);
    lv_obj_align_to(cp->bathReadLabel, cp->rollerBath, LV_ALIGN_OUT_BOTTOM_MID, 0, CALIB_READ_DY);
    set_read_label(cp->bathReadLabel, bRead);

    cp->chemReadLabel = lv_label_create(cp->container);
    lv_obj_set_style_text_font(cp->chemReadLabel, ui->confirm_btn_font, 0);
    lv_obj_align_to(cp->chemReadLabel, cp->rollerChem, LV_ALIGN_OUT_BOTTOM_MID, 0, CALIB_READ_DY);
    set_read_label(cp->chemReadLabel, cRead);

    /* SET button */
    cp->setButton = lv_button_create(cp->container);
    lv_obj_set_size(cp->setButton, ui->confirm_btn_w, ui->confirm_btn_h);
    lv_obj_align(cp->setButton, LV_ALIGN_BOTTOM_MID, 0, CALIB_SET_Y);
    lv_obj_add_event_cb(cp->setButton, event_calibSet, LV_EVENT_CLICKED, NULL);

    lv_obj_t *setLbl = lv_label_create(cp->setButton);
    lv_label_set_text(setLbl, tuneRollerButton_text);   /* "SET" */
    lv_obj_set_style_text_font(setLbl, ui->confirm_btn_font, 0);
    lv_obj_align(setLbl, LV_ALIGN_CENTER, 0, 0);

    /* Live reading refresh */
    cp->liveTimer = lv_timer_create(calib_live_cb, 1000, NULL);
}
