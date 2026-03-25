/**
 * @file element_splashPopup.c
 * @brief Splash Screen configuration popup
 *
 * Logic:
 *   - "Use Default" switch ON  → standard Deep Ocean splash, options hidden
 *   - "Random" switch ON       → randomize palette/shape/complexity/seed each boot,
 *                                 options hidden, Default switch disabled
 *   - Both OFF                 → user manually picks palette, shape, complexity
 *
 * Palette and Shape use the standard roller-popup pattern (same as Tank Size etc.)
 */

#include "FilMachine.h"

extern struct gui_components gui;

/* ── Palette & shape names — newline-separated for rollerPopupCreate ── */
const char *splashPaletteList =
    "Cyberpunk\n"
    "Aurora\n"
    "Lava\n"
    "Deep Ocean\n"
    "Forest\n"
    "Sunset\n"
    "Machinery\n"
    "Arctic\n"
    "Neon\n"
    "Pastel";

const char *splashShapeList =
    "Overlapping Rects\n"
    "Circles & Arcs\n"
    "Mixed Shapes\n"
    "Diagonal Bands\n"
    "Grid Blocks\n"
    "Radial Arcs";

/* Arrays to look up display name by index */
const char * const palette_display[] = {
    "Cyberpunk", "Aurora", "Lava", "Deep Ocean", "Forest",
    "Sunset", "Machinery", "Arctic", "Neon", "Pastel"
};
#define PALETTE_DISPLAY_COUNT  (sizeof(palette_display) / sizeof(palette_display[0]))

const char * const shape_display[] = {
    "Overlapping Rects", "Circles & Arcs", "Mixed Shapes",
    "Diagonal Bands", "Grid Blocks", "Radial Arcs"
};
#define SHAPE_DISPLAY_COUNT  (sizeof(shape_display) / sizeof(shape_display[0]))

/* ── Forward declarations ── */
static void event_splashPopup(lv_event_t * e);
static void update_ui_state(void);

/* ═══════════════════════════════════════════════
 * Update UI state based on Default / Random switches
 * ═══════════════════════════════════════════════ */
static void update_ui_state(void)
{
    bool isDefault = gui.page.settings.settingsParams.splashDefault;
    bool isRandom  = gui.page.settings.settingsParams.splashRandom;

    /* If Random is ON, force Default OFF and disable it */
    if (isRandom) {
        if (isDefault) {
            gui.page.settings.settingsParams.splashDefault = false;
            lv_obj_remove_state(gui.element.splashPopup.defaultSwitch, LV_STATE_CHECKED);
        }
        lv_obj_add_state(gui.element.splashPopup.defaultSwitch, LV_STATE_DISABLED);
        lv_obj_add_flag(gui.element.splashPopup.optionsContainer, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_state(gui.element.splashPopup.defaultSwitch, LV_STATE_DISABLED);

        if (isDefault) {
            /* Default ON → hide options */
            lv_obj_add_flag(gui.element.splashPopup.optionsContainer, LV_OBJ_FLAG_HIDDEN);
        } else {
            /* Both OFF → show options for manual pick */
            lv_obj_remove_flag(gui.element.splashPopup.optionsContainer, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ═══════════════════════════════════════════════
 * Event handler for all popup interactions
 * ═══════════════════════════════════════════════ */
static void event_splashPopup(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    /* ── Default switch toggled ── */
    if (target == gui.element.splashPopup.defaultSwitch && code == LV_EVENT_VALUE_CHANGED) {
        gui.page.settings.settingsParams.splashDefault =
            lv_obj_has_state(target, LV_STATE_CHECKED);
        update_ui_state();
        qSysAction(SAVE_PROCESS_CONFIG);
    }

    /* ── Random switch toggled ── */
    if (target == gui.element.splashPopup.randomSwitch && code == LV_EVENT_VALUE_CHANGED) {
        gui.page.settings.settingsParams.splashRandom =
            lv_obj_has_state(target, LV_STATE_CHECKED);
        update_ui_state();
        qSysAction(SAVE_PROCESS_CONFIG);
    }

    /* ── Palette text area focused → open roller popup ── */
    if (target == gui.element.splashPopup.paletteTextArea && code == LV_EVENT_FOCUSED) {
        if (gui.element.rollerPopup.mBoxRollerParent != NULL) return;
        rollerPopupCreate(splashPaletteList, "Palette",
                          gui.element.splashPopup.paletteTextArea,
                          gui.page.settings.settingsParams.splashPalette, ORANGE);
    }

    /* ── Shape text area focused → open roller popup ── */
    if (target == gui.element.splashPopup.shapeTextArea && code == LV_EVENT_FOCUSED) {
        if (gui.element.rollerPopup.mBoxRollerParent != NULL) return;
        rollerPopupCreate(splashShapeList, "Shape Style",
                          gui.element.splashPopup.shapeTextArea,
                          gui.page.settings.settingsParams.splashShapeStyle, ORANGE);
    }

    /* ── Complexity slider (20–100, step 20) ── */
    if (target == gui.element.splashPopup.complexitySlider) {
        if (code == LV_EVENT_VALUE_CHANGED) {
            int32_t val = lv_slider_get_value(target);
            val = roundToStep(val, 20);
            if (val < 20) val = 20;
            if (val > 100) val = 100;
            lv_slider_set_value(target, val, LV_ANIM_OFF);
            gui.page.settings.settingsParams.splashComplexity = (uint8_t)val;
        }
        if (code == LV_EVENT_RELEASED) {
            qSysAction(SAVE_PROCESS_CONFIG);
        }
    }

    /* ── Close button ── */
    if (target == gui.element.splashPopup.closeButton && code == LV_EVENT_CLICKED) {
        lv_obj_add_flag(gui.element.splashPopup.splashPopupParent, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ═══════════════════════════════════════════════
 * Create the splash settings popup
 * ═══════════════════════════════════════════════ */
void splashPopupCreate(void)
{
    const ui_splash_popup_layout_t *ui = &ui_get_profile()->splash_popup;
    const ui_settings_layout_t *ui_s = &ui_get_profile()->settings;

    if (gui.element.splashPopup.splashPopupParent != NULL) return;

    /* ── Backdrop + main container ── */
    createPopupBackdrop(
        &gui.element.splashPopup.splashPopupParent,
        &gui.element.splashPopup.splashContainer,
        ui_get_profile()->popups.splash_w,
        ui_get_profile()->popups.splash_h
    );

    lv_obj_t *cont = gui.element.splashPopup.splashContainer;

    /* ── Title ── */
    gui.element.splashPopup.splashTitle = lv_label_create(cont);
    lv_label_set_text(gui.element.splashPopup.splashTitle, "Splash Screen");
    lv_obj_set_style_text_font(gui.element.splashPopup.splashTitle, ui->title_font, 0);
    lv_obj_align(gui.element.splashPopup.splashTitle, LV_ALIGN_TOP_MID, 0, ui->title_y);

    /* ── Title line ── */
    initTitleLineStyle(&gui.element.splashPopup.style_titleLine, ORANGE);
    gui.element.splashPopup.splashTitleLine = lv_line_create(cont);
    gui.element.splashPopup.titleLinePoints[0].x = 0;
    gui.element.splashPopup.titleLinePoints[0].y = 0;
    gui.element.splashPopup.titleLinePoints[1].x = ui_get_profile()->popups.splash_title_line_w;
    gui.element.splashPopup.titleLinePoints[1].y = 0;
    lv_line_set_points(gui.element.splashPopup.splashTitleLine,
                       gui.element.splashPopup.titleLinePoints, 2);
    lv_obj_add_style(gui.element.splashPopup.splashTitleLine,
                     &gui.element.splashPopup.style_titleLine, 0);
    lv_obj_align(gui.element.splashPopup.splashTitleLine,
                 LV_ALIGN_TOP_MID, 0, ui->title_line_y);

    /* ── Default switch row ── */
    lv_obj_t *def_row = lv_obj_create(cont);
    lv_obj_remove_style_all(def_row);
    lv_obj_set_size(def_row, lv_pct(90), ui->row_h);
    lv_obj_align(def_row, LV_ALIGN_TOP_MID, 0, ui->default_switch_y);
    lv_obj_remove_flag(def_row, LV_OBJ_FLAG_SCROLLABLE);

    gui.element.splashPopup.defaultLabel = lv_label_create(def_row);
    lv_label_set_text(gui.element.splashPopup.defaultLabel, "Use Default");
    lv_obj_set_style_text_font(gui.element.splashPopup.defaultLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.defaultLabel, LV_ALIGN_LEFT_MID, ui->label_x, 0);

    gui.element.splashPopup.defaultSwitch = lv_switch_create(def_row);
    lv_obj_set_size(gui.element.splashPopup.defaultSwitch,
                    ui_s->toggle_switch_w, ui_s->toggle_switch_h);
    lv_obj_align(gui.element.splashPopup.defaultSwitch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(gui.element.splashPopup.defaultSwitch,
                              lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gui.element.splashPopup.defaultSwitch,
                              lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gui.element.splashPopup.defaultSwitch,
                              lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (gui.page.settings.settingsParams.splashDefault) {
        lv_obj_add_state(gui.element.splashPopup.defaultSwitch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(gui.element.splashPopup.defaultSwitch,
                        event_splashPopup, LV_EVENT_VALUE_CHANGED,
                        gui.element.splashPopup.defaultSwitch);

    /* ── Random switch row ── */
    lv_obj_t *sw_row = lv_obj_create(cont);
    lv_obj_remove_style_all(sw_row);
    lv_obj_set_size(sw_row, lv_pct(90), ui->row_h);
    lv_obj_align(sw_row, LV_ALIGN_TOP_MID, 0, ui->random_switch_y);
    lv_obj_remove_flag(sw_row, LV_OBJ_FLAG_SCROLLABLE);

    gui.element.splashPopup.randomLabel = lv_label_create(sw_row);
    lv_label_set_text(gui.element.splashPopup.randomLabel, "Random");
    lv_obj_set_style_text_font(gui.element.splashPopup.randomLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.randomLabel, LV_ALIGN_LEFT_MID, ui->label_x, 0);

    gui.element.splashPopup.randomSwitch = lv_switch_create(sw_row);
    lv_obj_set_size(gui.element.splashPopup.randomSwitch,
                    ui_s->toggle_switch_w, ui_s->toggle_switch_h);
    lv_obj_align(gui.element.splashPopup.randomSwitch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(gui.element.splashPopup.randomSwitch,
                              lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gui.element.splashPopup.randomSwitch,
                              lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gui.element.splashPopup.randomSwitch,
                              lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (gui.page.settings.settingsParams.splashRandom) {
        lv_obj_add_state(gui.element.splashPopup.randomSwitch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(gui.element.splashPopup.randomSwitch,
                        event_splashPopup, LV_EVENT_VALUE_CHANGED,
                        gui.element.splashPopup.randomSwitch);

    /* ── Options container (visible only when Random=OFF and Default=OFF) ── */
    gui.element.splashPopup.optionsContainer = lv_obj_create(cont);
    lv_obj_set_size(gui.element.splashPopup.optionsContainer, lv_pct(92), ui->options_h);
    lv_obj_align(gui.element.splashPopup.optionsContainer,
                 LV_ALIGN_TOP_MID, 0, ui->options_y);
    lv_obj_remove_flag(gui.element.splashPopup.optionsContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_opa(gui.element.splashPopup.optionsContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(gui.element.splashPopup.optionsContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(gui.element.splashPopup.optionsContainer, 0, 0);

    lv_obj_t *opts = gui.element.splashPopup.optionsContainer;
    int y = 0;

    /* ── Row 1: Palette (text area → opens roller popup) ── */
    gui.element.splashPopup.paletteLabel = lv_label_create(opts);
    lv_label_set_text(gui.element.splashPopup.paletteLabel, "Palette");
    lv_obj_set_style_text_font(gui.element.splashPopup.paletteLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.paletteLabel, LV_ALIGN_TOP_LEFT, ui->label_x, y + 6);

    gui.element.splashPopup.paletteTextArea = lv_textarea_create(opts);
    lv_obj_set_size(gui.element.splashPopup.paletteTextArea, ui->roller_w, ui->roller_h);
    lv_obj_align(gui.element.splashPopup.paletteTextArea, LV_ALIGN_TOP_RIGHT, ui->roller_x, y);
    lv_textarea_set_one_line(gui.element.splashPopup.paletteTextArea, true);
    lv_obj_set_scrollbar_mode(gui.element.splashPopup.paletteTextArea, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(gui.element.splashPopup.paletteTextArea,
                              lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_align(gui.element.splashPopup.paletteTextArea, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(gui.element.splashPopup.paletteTextArea, ui->value_font, 0);
    lv_obj_set_style_border_color(gui.element.splashPopup.paletteTextArea,
                                  lv_color_hex(ORANGE), 0);
    {
        uint8_t idx = gui.page.settings.settingsParams.splashPalette;
        if (idx >= PALETTE_DISPLAY_COUNT) idx = 0;
        lv_textarea_set_text(gui.element.splashPopup.paletteTextArea, palette_display[idx]);
        lv_textarea_set_cursor_pos(gui.element.splashPopup.paletteTextArea, 0);
    }
    lv_obj_add_event_cb(gui.element.splashPopup.paletteTextArea,
                        event_splashPopup, LV_EVENT_FOCUSED,
                        gui.element.splashPopup.paletteTextArea);

    y += ui->row_h + ui->row_gap;

    /* ── Row 2: Shape Style (text area → opens roller popup) ── */
    gui.element.splashPopup.shapeLabel = lv_label_create(opts);
    lv_label_set_text(gui.element.splashPopup.shapeLabel, "Shape Style");
    lv_obj_set_style_text_font(gui.element.splashPopup.shapeLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.shapeLabel, LV_ALIGN_TOP_LEFT, ui->label_x, y + 6);

    gui.element.splashPopup.shapeTextArea = lv_textarea_create(opts);
    lv_obj_set_size(gui.element.splashPopup.shapeTextArea, ui->roller_w, ui->roller_h);
    lv_obj_align(gui.element.splashPopup.shapeTextArea, LV_ALIGN_TOP_RIGHT, ui->roller_x, y);
    lv_textarea_set_one_line(gui.element.splashPopup.shapeTextArea, true);
    lv_obj_set_scrollbar_mode(gui.element.splashPopup.shapeTextArea, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(gui.element.splashPopup.shapeTextArea,
                              lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_text_align(gui.element.splashPopup.shapeTextArea, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(gui.element.splashPopup.shapeTextArea, ui->value_font, 0);
    lv_obj_set_style_border_color(gui.element.splashPopup.shapeTextArea,
                                  lv_color_hex(ORANGE), 0);
    {
        uint8_t idx = gui.page.settings.settingsParams.splashShapeStyle;
        if (idx >= SHAPE_DISPLAY_COUNT) idx = 0;
        lv_textarea_set_text(gui.element.splashPopup.shapeTextArea, shape_display[idx]);
        lv_textarea_set_cursor_pos(gui.element.splashPopup.shapeTextArea, 0);
    }
    lv_obj_add_event_cb(gui.element.splashPopup.shapeTextArea,
                        event_splashPopup, LV_EVENT_FOCUSED,
                        gui.element.splashPopup.shapeTextArea);

    y += ui->row_h + ui->row_gap;

    /* ── Row 3: Complexity (20–100, step 20) ── */
    gui.element.splashPopup.complexityLabel = lv_label_create(opts);
    lv_label_set_text(gui.element.splashPopup.complexityLabel, "Complexity");
    lv_obj_set_style_text_font(gui.element.splashPopup.complexityLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.complexityLabel, LV_ALIGN_TOP_LEFT, ui->label_x, y + 6);

    gui.element.splashPopup.complexitySlider = lv_slider_create(opts);
    lv_obj_set_width(gui.element.splashPopup.complexitySlider, ui->slider_w);
    lv_obj_align(gui.element.splashPopup.complexitySlider, LV_ALIGN_TOP_RIGHT, ui->slider_x, y + 10);
    lv_slider_set_range(gui.element.splashPopup.complexitySlider, 20, 100);
    {
        int32_t val = gui.page.settings.settingsParams.splashComplexity;
        val = roundToStep(val, 20);
        if (val < 20) val = 20;
        if (val > 100) val = 100;
        lv_slider_set_value(gui.element.splashPopup.complexitySlider, val, LV_ANIM_OFF);
    }
    lv_obj_set_style_bg_color(gui.element.splashPopup.complexitySlider,
                              lv_color_hex(ORANGE), LV_PART_KNOB);
    lv_obj_set_style_bg_color(gui.element.splashPopup.complexitySlider,
                              lv_color_hex(ORANGE_LIGHT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(gui.element.splashPopup.complexitySlider,
                              lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_add_event_cb(gui.element.splashPopup.complexitySlider,
                        event_splashPopup, LV_EVENT_VALUE_CHANGED,
                        gui.element.splashPopup.complexitySlider);
    lv_obj_add_event_cb(gui.element.splashPopup.complexitySlider,
                        event_splashPopup, LV_EVENT_RELEASED,
                        gui.element.splashPopup.complexitySlider);

    /* ── Close button ── */
    gui.element.splashPopup.closeButton = lv_button_create(cont);
    lv_obj_set_size(gui.element.splashPopup.closeButton, ui->close_btn_w, ui->close_btn_h);
    lv_obj_align(gui.element.splashPopup.closeButton,
                 LV_ALIGN_BOTTOM_MID, 0, -(ui->close_btn_y));
    lv_obj_set_style_bg_color(gui.element.splashPopup.closeButton,
                              lv_color_hex(ORANGE), LV_PART_MAIN);
    lv_obj_add_event_cb(gui.element.splashPopup.closeButton,
                        event_splashPopup, LV_EVENT_CLICKED,
                        gui.element.splashPopup.closeButton);

    gui.element.splashPopup.closeButtonLabel = lv_label_create(gui.element.splashPopup.closeButton);
    lv_label_set_text(gui.element.splashPopup.closeButtonLabel, "Close");
    lv_obj_set_style_text_font(gui.element.splashPopup.closeButtonLabel, ui->button_font, 0);
    lv_obj_center(gui.element.splashPopup.closeButtonLabel);

    /* ── Initial visibility ── */
    update_ui_state();
}
