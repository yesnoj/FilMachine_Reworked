/**
 * @file element_splashPopup.c
 * @brief Splash Screen configuration popup with live background preview
 *
 * Layout:
 *   [X] close button                    (top-right, always visible)
 *   Title "Splash Screen" + orange line
 *   [Use Default]  switch
 *   [Random next boot] switch           (hidden when Default ON)
 *   Palette / Shape Style / Complexity  (hidden when Default ON or Random ON)
 *   [Random] button                     (bottom-center, hidden when Default ON or Random ON)
 *
 * Background: live preview of splash shapes behind a dark overlay.
 *
 * Logic:
 *   - Default ON  → Random hidden, options hidden; preview = Deep Ocean
 *   - Default OFF, Random ON  → options hidden; preview = random shapes (new seed each open)
 *   - Default OFF, Random OFF → options+button visible; preview = custom params live
 *   - Toggling Default ON forces Random OFF
 *   - Toggling Random ON forces Default OFF
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
static void refresh_preview_internal(void);

/* ═══════════════════════════════════════════════
 * Regenerate preview background (shapes only)
 * ═══════════════════════════════════════════════ */
static void refresh_preview_internal(void)
{
    lv_obj_t *prev = gui.element.splashPopup.previewContainer;
    if (!prev) return;

    /* Delete old shapes */
    lv_obj_clean(prev);

    if (gui.page.settings.settingsParams.splashDefault) {
        splash_standard_preview_generate(prev);
    } else {
        uint32_t seed = gui.page.settings.settingsParams.splashSeed;
        if (seed == 0) seed = 1;
        splash_preview_generate(
            prev, seed,
            gui.page.settings.settingsParams.splashPalette,
            gui.page.settings.settingsParams.splashShapeStyle,
            gui.page.settings.settingsParams.splashComplexity
        );
    }
}

/* Public wrapper — called by roller popup after palette/shape change */
void splashPopupRefreshPreview(void)
{
    uint32_t tick = lv_tick_get();
    gui.page.settings.settingsParams.splashSeed =
        tick * 1103515245u + 12345u;
    refresh_preview_internal();
}

/* ═══════════════════════════════════════════════
 * Update visibility based on Default/Random state
 *
 *  Default ON  → hide Random row, hide options, hide random button
 *  Default OFF, Random ON  → show Random row, hide options, hide random button
 *  Default OFF, Random OFF → show Random row, show options, show random button
 * ═══════════════════════════════════════════════ */
static void update_ui_state(void)
{
    bool isDefault = gui.page.settings.settingsParams.splashDefault;
    bool isRandom  = gui.page.settings.settingsParams.splashRandom;

    /* Random switch row (= parent of randomSwitch): visible when Default is OFF */
    lv_obj_t *rnd_row = lv_obj_get_parent(gui.element.splashPopup.randomSwitch);
    if (isDefault) {
        lv_obj_add_flag(rnd_row, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(rnd_row, LV_OBJ_FLAG_HIDDEN);
    }

    /* Options + random button: visible when Default OFF AND Random OFF */
    if (isDefault || isRandom) {
        lv_obj_add_flag(gui.element.splashPopup.optionsContainer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(gui.element.splashPopup.randomButton, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(gui.element.splashPopup.optionsContainer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(gui.element.splashPopup.randomButton, LV_OBJ_FLAG_HIDDEN);
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
        bool checked = lv_obj_has_state(target, LV_STATE_CHECKED);
        gui.page.settings.settingsParams.splashDefault = checked;
        /* Default ON → force Random OFF */
        if (checked) {
            gui.page.settings.settingsParams.splashRandom = false;
            lv_obj_remove_state(gui.element.splashPopup.randomSwitch, LV_STATE_CHECKED);
        }
        update_ui_state();
        refresh_preview_internal();
        qSysAction(SAVE_PROCESS_CONFIG);
    }

    /* ── Random switch toggled ── */
    if (target == gui.element.splashPopup.randomSwitch && code == LV_EVENT_VALUE_CHANGED) {
        bool checked = lv_obj_has_state(target, LV_STATE_CHECKED);
        gui.page.settings.settingsParams.splashRandom = checked;
        /* Random ON → force Default OFF */
        if (checked) {
            gui.page.settings.settingsParams.splashDefault = false;
            lv_obj_remove_state(gui.element.splashPopup.defaultSwitch, LV_STATE_CHECKED);
        }
        update_ui_state();
        refresh_preview_internal();
        qSysAction(SAVE_PROCESS_CONFIG);
    }

    /* ── Random button clicked → randomise params for custom mode ── */
    if (target == gui.element.splashPopup.randomButton && code == LV_EVENT_CLICKED) {
        uint32_t tick = lv_tick_get();
        uint32_t rng  = tick * 1103515245u + 12345u;

        uint8_t rpal = (uint8_t)(rng % PALETTE_DISPLAY_COUNT);
        rng = rng * 1103515245u + 12345u;
        uint8_t rsty = (uint8_t)(rng % SHAPE_DISPLAY_COUNT);
        rng = rng * 1103515245u + 12345u;
        uint8_t rcmx = (uint8_t)(((rng % 5) + 1) * 20);  /* 20,40,60,80,100 */
        rng = rng * 1103515245u + 12345u;

        gui.page.settings.settingsParams.splashPalette    = rpal;
        gui.page.settings.settingsParams.splashShapeStyle = rsty;
        gui.page.settings.settingsParams.splashComplexity = rcmx;
        gui.page.settings.settingsParams.splashSeed       = rng;

        /* Update UI controls */
        if (rpal < PALETTE_DISPLAY_COUNT)
            lv_textarea_set_text(gui.element.splashPopup.paletteTextArea, palette_display[rpal]);
        if (rsty < SHAPE_DISPLAY_COUNT)
            lv_textarea_set_text(gui.element.splashPopup.shapeTextArea, shape_display[rsty]);
        lv_slider_set_value(gui.element.splashPopup.complexitySlider, rcmx, LV_ANIM_OFF);

        refresh_preview_internal();
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

    /* ── Complexity slider ── */
    if (target == gui.element.splashPopup.complexitySlider) {
        if (code == LV_EVENT_VALUE_CHANGED) {
            int32_t val = lv_slider_get_value(target);
            val = roundToStep(val, 20);
            if (val < 20)  val = 20;
            if (val > 100) val = 100;
            lv_slider_set_value(target, val, LV_ANIM_OFF);
            gui.page.settings.settingsParams.splashComplexity = (uint8_t)val;
        }
        if (code == LV_EVENT_RELEASED) {
            uint32_t tick = lv_tick_get();
            gui.page.settings.settingsParams.splashSeed =
                tick * 1103515245u + 12345u;
            refresh_preview_internal();
            qSysAction(SAVE_PROCESS_CONFIG);
        }
    }

    /* ── X close button (top-right) ── */
    if (target == gui.element.splashPopup.xCloseButton && code == LV_EVENT_CLICKED) {
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

    /* Ensure valid defaults for preview */
    if (gui.page.settings.settingsParams.splashSeed == 0 &&
        !gui.page.settings.settingsParams.splashDefault) {
        gui.page.settings.settingsParams.splashSeed =
            lv_tick_get() * 1103515245u + 12345u;
    }
    if (gui.page.settings.settingsParams.splashComplexity == 0) {
        gui.page.settings.settingsParams.splashComplexity = 40;
    }

    int popup_w = ui_get_profile()->popups.splash_w;
    int popup_h = ui_get_profile()->popups.splash_h;

    /* ── Backdrop + main container ── */
    createPopupBackdrop(
        &gui.element.splashPopup.splashPopupParent,
        &gui.element.splashPopup.splashContainer,
        popup_w, popup_h
    );

    lv_obj_t *cont = gui.element.splashPopup.splashContainer;

    /* Clip children to the rounded corners */
    lv_obj_set_style_clip_corner(cont, true, 0);

    /* Get the default padding applied by the theme — preview/overlay
     * use negative offsets to extend into this padding area so the
     * background fills the entire popup including rounded corners,
     * while all UI elements keep their existing positions. */
    int pad_l = lv_obj_get_style_pad_left(cont, 0);
    int pad_t = lv_obj_get_style_pad_top(cont, 0);

    /* ── Preview container (fills entire popup, first child = lowest z-order) ── */
    gui.element.splashPopup.previewContainer = lv_obj_create(cont);
    lv_obj_remove_style_all(gui.element.splashPopup.previewContainer);
    lv_obj_set_size(gui.element.splashPopup.previewContainer, popup_w, popup_h);
    lv_obj_set_pos(gui.element.splashPopup.previewContainer, -pad_l, -pad_t);
    lv_obj_remove_flag(gui.element.splashPopup.previewContainer, LV_OBJ_FLAG_SCROLLABLE);

    /* Generate initial preview */
    refresh_preview_internal();

    /* ── Dark overlay for readability ── */
    gui.element.splashPopup.overlayRect = lv_obj_create(cont);
    lv_obj_remove_style_all(gui.element.splashPopup.overlayRect);
    lv_obj_set_size(gui.element.splashPopup.overlayRect, popup_w, popup_h);
    lv_obj_set_pos(gui.element.splashPopup.overlayRect, -pad_l, -pad_t);
    lv_obj_set_style_bg_color(gui.element.splashPopup.overlayRect,
                              lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(gui.element.splashPopup.overlayRect,
                            (uint8_t)ui->overlay_opa, 0);
    lv_obj_remove_flag(gui.element.splashPopup.overlayRect, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(gui.element.splashPopup.overlayRect, LV_OBJ_FLAG_CLICKABLE);

    /* ── Title ── */
    gui.element.splashPopup.splashTitle = lv_label_create(cont);
    lv_label_set_text(gui.element.splashPopup.splashTitle, splashPopupTitle_text);
    lv_obj_set_style_text_font(gui.element.splashPopup.splashTitle, ui->title_font, 0);
    lv_obj_align(gui.element.splashPopup.splashTitle, LV_ALIGN_TOP_MID, ui->title_x, ui->title_y);

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
                 LV_ALIGN_TOP_MID, ui->title_line_x, ui->title_line_y);

    /* ── Default switch row ── */
    lv_obj_t *def_row = lv_obj_create(cont);
    lv_obj_remove_style_all(def_row);
    lv_obj_set_size(def_row, ui->switch_row_w, ui->row_h);
    lv_obj_align(def_row, LV_ALIGN_TOP_MID, ui->switch_row_x, ui->default_switch_y);
    lv_obj_remove_flag(def_row, LV_OBJ_FLAG_SCROLLABLE);

    gui.element.splashPopup.defaultLabel = lv_label_create(def_row);
    lv_label_set_text(gui.element.splashPopup.defaultLabel, splashPopupUseDefault_text);
    lv_obj_set_style_text_font(gui.element.splashPopup.defaultLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.defaultLabel, LV_ALIGN_LEFT_MID, ui->label_x, ui->switch_label_y);

    gui.element.splashPopup.defaultSwitch = lv_switch_create(def_row);
    lv_obj_set_size(gui.element.splashPopup.defaultSwitch,
                    ui_s->toggle_switch_w, ui_s->toggle_switch_h);
    lv_obj_align(gui.element.splashPopup.defaultSwitch, LV_ALIGN_RIGHT_MID, ui->switch_x, ui->switch_y);
    lv_obj_set_style_bg_color(gui.element.splashPopup.defaultSwitch,
                              lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gui.element.splashPopup.defaultSwitch,
                              lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gui.element.splashPopup.defaultSwitch,
                              lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (gui.page.settings.settingsParams.splashDefault)
        lv_obj_add_state(gui.element.splashPopup.defaultSwitch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(gui.element.splashPopup.defaultSwitch,
                        event_splashPopup, LV_EVENT_VALUE_CHANGED,
                        gui.element.splashPopup.defaultSwitch);

    /* ── Random switch row ── */
    lv_obj_t *rnd_row = lv_obj_create(cont);
    lv_obj_remove_style_all(rnd_row);
    lv_obj_set_size(rnd_row, ui->switch_row_w, ui->row_h);
    lv_obj_align(rnd_row, LV_ALIGN_TOP_MID, ui->switch_row_x, ui->random_switch_y);
    lv_obj_remove_flag(rnd_row, LV_OBJ_FLAG_SCROLLABLE);

    gui.element.splashPopup.randomLabel = lv_label_create(rnd_row);
    lv_label_set_text(gui.element.splashPopup.randomLabel, splashPopupRandom_text);
    lv_obj_set_style_text_font(gui.element.splashPopup.randomLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.randomLabel, LV_ALIGN_LEFT_MID, ui->label_x, ui->switch_label_y);

    gui.element.splashPopup.randomSwitch = lv_switch_create(rnd_row);
    lv_obj_set_size(gui.element.splashPopup.randomSwitch,
                    ui_s->toggle_switch_w, ui_s->toggle_switch_h);
    lv_obj_align(gui.element.splashPopup.randomSwitch, LV_ALIGN_RIGHT_MID, ui->switch_x, ui->switch_y);
    lv_obj_set_style_bg_color(gui.element.splashPopup.randomSwitch,
                              lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gui.element.splashPopup.randomSwitch,
                              lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(gui.element.splashPopup.randomSwitch,
                              lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (gui.page.settings.settingsParams.splashRandom)
        lv_obj_add_state(gui.element.splashPopup.randomSwitch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(gui.element.splashPopup.randomSwitch,
                        event_splashPopup, LV_EVENT_VALUE_CHANGED,
                        gui.element.splashPopup.randomSwitch);

    /* ── Options container (Palette / Shape Style / Complexity) ── */
    gui.element.splashPopup.optionsContainer = lv_obj_create(cont);
    lv_obj_set_size(gui.element.splashPopup.optionsContainer, ui->options_w, ui->options_h);
    lv_obj_align(gui.element.splashPopup.optionsContainer,
                 LV_ALIGN_TOP_MID, ui->options_x, ui->options_y);
    lv_obj_remove_flag(gui.element.splashPopup.optionsContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_opa(gui.element.splashPopup.optionsContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(gui.element.splashPopup.optionsContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(gui.element.splashPopup.optionsContainer, 0, 0);

    lv_obj_t *opts = gui.element.splashPopup.optionsContainer;
    int y = 0;

    /* ── Row 1: Palette ── */
    gui.element.splashPopup.paletteLabel = lv_label_create(opts);
    lv_label_set_text(gui.element.splashPopup.paletteLabel, splashPopupPalette_text);
    lv_obj_set_style_text_font(gui.element.splashPopup.paletteLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.paletteLabel, LV_ALIGN_TOP_LEFT, ui->label_x, y + ui->label_y_offset);

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
    lv_obj_set_style_opa(gui.element.splashPopup.paletteTextArea, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_width(gui.element.splashPopup.paletteTextArea, 0, LV_PART_CURSOR);
    {
        uint8_t idx = gui.page.settings.settingsParams.splashPalette;
        if (idx >= PALETTE_DISPLAY_COUNT) idx = 0;
        lv_textarea_set_text(gui.element.splashPopup.paletteTextArea, palette_display[idx]);
        lv_textarea_set_cursor_pos(gui.element.splashPopup.paletteTextArea, LV_TEXTAREA_CURSOR_LAST);
    }
    lv_obj_add_event_cb(gui.element.splashPopup.paletteTextArea,
                        event_splashPopup, LV_EVENT_FOCUSED,
                        gui.element.splashPopup.paletteTextArea);

    y += ui->row_h + ui->row_gap;

    /* ── Row 2: Shape Style ── */
    gui.element.splashPopup.shapeLabel = lv_label_create(opts);
    lv_label_set_text(gui.element.splashPopup.shapeLabel, splashPopupShapeStyle_text);
    lv_obj_set_style_text_font(gui.element.splashPopup.shapeLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.shapeLabel, LV_ALIGN_TOP_LEFT, ui->label_x, y + ui->label_y_offset);

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
    lv_obj_set_style_opa(gui.element.splashPopup.shapeTextArea, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_width(gui.element.splashPopup.shapeTextArea, 0, LV_PART_CURSOR);
    {
        uint8_t idx = gui.page.settings.settingsParams.splashShapeStyle;
        if (idx >= SHAPE_DISPLAY_COUNT) idx = 0;
        lv_textarea_set_text(gui.element.splashPopup.shapeTextArea, shape_display[idx]);
        lv_textarea_set_cursor_pos(gui.element.splashPopup.shapeTextArea, LV_TEXTAREA_CURSOR_LAST);
    }
    lv_obj_add_event_cb(gui.element.splashPopup.shapeTextArea,
                        event_splashPopup, LV_EVENT_FOCUSED,
                        gui.element.splashPopup.shapeTextArea);

    y += ui->row_h + ui->row_gap;

    /* ── Row 3: Complexity (20–100, step 20) ── */
    gui.element.splashPopup.complexityLabel = lv_label_create(opts);
    lv_label_set_text(gui.element.splashPopup.complexityLabel, splashPopupComplexity_text);
    lv_obj_set_style_text_font(gui.element.splashPopup.complexityLabel, ui->label_font, 0);
    lv_obj_align(gui.element.splashPopup.complexityLabel, LV_ALIGN_TOP_LEFT, ui->label_x, y + ui->label_y_offset);

    gui.element.splashPopup.complexitySlider = lv_slider_create(opts);
    lv_obj_set_width(gui.element.splashPopup.complexitySlider, ui->slider_w);
    lv_obj_align(gui.element.splashPopup.complexitySlider, LV_ALIGN_TOP_RIGHT, ui->slider_x, y + ui->slider_y_offset);
    lv_slider_set_range(gui.element.splashPopup.complexitySlider, 20, 100);
    {
        int32_t val = gui.page.settings.settingsParams.splashComplexity;
        val = roundToStep(val, 20);
        if (val < 20)  val = 20;
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

    /* ── X close button (top-right, like other popups) ── */
    gui.element.splashPopup.xCloseButton = lv_button_create(cont);
    lv_obj_set_size(gui.element.splashPopup.xCloseButton, ui->x_close_w, ui->x_close_h);
    lv_obj_align(gui.element.splashPopup.xCloseButton,
                 LV_ALIGN_TOP_RIGHT, ui->x_close_x, ui->x_close_y);
    lv_obj_add_event_cb(gui.element.splashPopup.xCloseButton,
                        event_splashPopup, LV_EVENT_CLICKED,
                        gui.element.splashPopup.xCloseButton);
    lv_obj_set_style_bg_color(gui.element.splashPopup.xCloseButton,
                              lv_color_hex(ORANGE), LV_PART_MAIN);
    lv_obj_move_foreground(gui.element.splashPopup.xCloseButton);

    gui.element.splashPopup.xCloseButtonLabel = lv_label_create(gui.element.splashPopup.xCloseButton);
    lv_label_set_text(gui.element.splashPopup.xCloseButtonLabel, closePopup_icon);
    lv_obj_set_style_text_font(gui.element.splashPopup.xCloseButtonLabel, ui->close_icon_font, 0);
    lv_obj_align(gui.element.splashPopup.xCloseButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* ── Random button (bottom-center, replaces old Close) ── */
    gui.element.splashPopup.randomButton = lv_button_create(cont);
    lv_obj_set_size(gui.element.splashPopup.randomButton, ui->close_btn_w, ui->close_btn_h);
    lv_obj_align(gui.element.splashPopup.randomButton,
                 LV_ALIGN_BOTTOM_MID, ui->close_btn_x, -(ui->close_btn_y));
    lv_obj_set_style_bg_color(gui.element.splashPopup.randomButton,
                              lv_color_hex(ORANGE), LV_PART_MAIN);
    lv_obj_add_event_cb(gui.element.splashPopup.randomButton,
                        event_splashPopup, LV_EVENT_CLICKED,
                        gui.element.splashPopup.randomButton);

    gui.element.splashPopup.randomButtonLabel = lv_label_create(gui.element.splashPopup.randomButton);
    lv_label_set_text(gui.element.splashPopup.randomButtonLabel, splashPopupRandomBtn_text);
    lv_obj_set_style_text_font(gui.element.splashPopup.randomButtonLabel, ui->button_font, 0);
    lv_obj_center(gui.element.splashPopup.randomButtonLabel);

    /* ── Apply initial visibility ── */
    update_ui_state();
}
