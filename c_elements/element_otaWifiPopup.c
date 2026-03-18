/**
 * @file element_otaWifiPopup.c
 *
 * OTA Wi-Fi Update popup.
 *
 * Shows the board's IP address and a random 5-digit PIN.
 * The user opens http://<ip> in a browser, enters the PIN,
 * and uploads the firmware .bin file.
 *
 * The popup has:
 *   - Title "Wi-Fi update"
 *   - X close button (stops server + closes)
 *   - IP address (large, prominent)
 *   - PIN code (5 digits)
 *   - Status label (Connecting... / Server running / Uploading... / Done)
 *   - Progress bar (hidden until upload starts)
 */

#include <string.h>
#include <stdlib.h>
#include "FilMachine.h"

extern struct gui_components gui;

/* ── Generate random 5-digit PIN ──────────────────────────── */
static void ota_generate_pin(char *pin, size_t len) {
    uint32_t r = (uint32_t)(lv_tick_get() * 7919 + 104729);
    /* Simple LCG-style mixing for variety */
    r = (r ^ (r >> 16)) * 0x45d9f3b;
    r = (r ^ (r >> 16)) * 0x45d9f3b;
    r = r ^ (r >> 16);
    snprintf(pin, len, "%05" PRIu32, r % 100000);
}

/* ── Timer to update status and progress ──────────────────── */
static lv_timer_t *ota_ui_timer = NULL;

static void ota_ui_timer_cb(lv_timer_t *timer) {
    (void)timer;
    struct sOtaWifiPopup *p = &gui.element.otaWifiPopup;

    /* Update IP once available */
    const char *ip = ota_get_ip_address();
    if (ip && ip[0] != '\0') {
        char buf[40];
        snprintf(buf, sizeof(buf), "http://%s", ip);
        lv_label_set_text(p->ipLabel, buf);
    }

    /* Update status */
    const char *status = ota_get_status_text();
    if (status && status[0] != '\0') {
        lv_label_set_text(p->statusLabel, status);
    }

    /* Update progress bar */
    int pct = ota_get_progress();
    if (pct > 0) {
        lv_obj_remove_flag(p->progressBar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(p->progressBar, pct, LV_ANIM_ON);
    }

    if (ota_is_running()) {
        lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(ORANGE), 0);
    } else if (pct >= 100) {
        lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(GREEN), 0);
    }
}

/* ── Event handler ────────────────────────────────────────── */
void event_otaWifiPopup(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    struct sOtaWifiPopup *p = &gui.element.otaWifiPopup;

    if (code != LV_EVENT_CLICKED) return;

    if (obj == p->closeButton) {
        /* Stop server, stop timer, hide popup */
        ota_wifi_server_stop();
        safeTimerDelete(&ota_ui_timer);
        lv_obj_add_flag(p->popupParent, LV_OBJ_FLAG_HIDDEN);
        LV_LOG_USER("[OTA] Wi-Fi popup closed");
    }
}

/* ── Create popup ─────────────────────────────────────────── */
void otaWifiPopupCreate(void) {
    struct sOtaWifiPopup *p = &gui.element.otaWifiPopup;

    /* Generate a fresh PIN */
    ota_generate_pin(p->otaPin, sizeof(p->otaPin));

    if (p->popupParent != NULL) {
        /* Popup already created — just show it with fresh PIN */
        char pin_buf[32];
        snprintf(pin_buf, sizeof(pin_buf), "PIN:  %s", p->otaPin);
        lv_label_set_text(p->pinLabel, pin_buf);

        lv_label_set_text(p->ipLabel, "Connecting...");
        lv_label_set_text(p->statusLabel, "Starting server...");
        lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(WHITE), 0);
        lv_obj_add_flag(p->progressBar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(p->progressBar, 0, LV_ANIM_OFF);

        lv_obj_remove_flag(p->popupParent, LV_OBJ_FLAG_HIDDEN);

        /* Start server + UI timer */
        ota_wifi_server_start();
        if (ota_ui_timer == NULL)
            ota_ui_timer = lv_timer_create(ota_ui_timer_cb, 500, NULL);
        return;
    }

    /* ── First-time creation ── */
    createPopupBackdrop(&p->popupParent, &p->popupContainer, 380, 260);

    /* Title */
    p->titleLabel = lv_label_create(p->popupContainer);
    lv_label_set_text(p->titleLabel, otaWifiUpdate_text);
    lv_obj_set_style_text_font(p->titleLabel, &lv_font_montserrat_22, 0);
    lv_obj_align(p->titleLabel, LV_ALIGN_TOP_MID, 0, -10);

    /* Underline */
    initTitleLineStyle(&p->style_titleLine, WHITE);
    p->titleLine = lv_line_create(p->popupContainer);
    lv_line_set_points(p->titleLine, p->titleLinePoints, 2);
    lv_obj_add_style(p->titleLine, &p->style_titleLine, 0);
    lv_obj_align(p->titleLine, LV_ALIGN_TOP_MID, 0, 23);

    /* X close button */
    p->closeButton = lv_button_create(p->popupContainer);
    lv_obj_set_size(p->closeButton, BUTTON_POPUP_CLOSE_WIDTH, BUTTON_POPUP_CLOSE_HEIGHT);
    lv_obj_align(p->closeButton, LV_ALIGN_TOP_RIGHT, 7, -10);
    lv_obj_add_event_cb(p->closeButton, event_otaWifiPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(p->closeButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
    lv_obj_move_foreground(p->closeButton);

    p->closeButtonLabel = lv_label_create(p->closeButton);
    lv_label_set_text(p->closeButtonLabel, closePopup_icon);
    lv_obj_set_style_text_font(p->closeButtonLabel, &FilMachineFontIcons_20, 0);
    lv_obj_align(p->closeButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* IP address — large and prominent */
    p->ipLabel = lv_label_create(p->popupContainer);
    lv_label_set_text(p->ipLabel, "Connecting...");
    lv_obj_set_style_text_font(p->ipLabel, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(p->ipLabel, lv_color_hex(LIGHT_BLUE), 0);
    lv_obj_set_width(p->ipLabel, 360);
    lv_obj_set_style_text_align(p->ipLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(p->ipLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(p->ipLabel, LV_ALIGN_TOP_MID, 0, 40);

    /* PIN label */
    char pin_buf[32];
    snprintf(pin_buf, sizeof(pin_buf), "PIN:  %s", p->otaPin);
    p->pinLabel = lv_label_create(p->popupContainer);
    lv_label_set_text(p->pinLabel, pin_buf);
    lv_obj_set_style_text_font(p->pinLabel, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(p->pinLabel, lv_color_hex(GREEN), 0);
    lv_obj_set_style_text_align(p->pinLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(p->pinLabel, 360);
    lv_obj_align(p->pinLabel, LV_ALIGN_TOP_MID, 0, 80);

    /* Status label */
    p->statusLabel = lv_label_create(p->popupContainer);
    lv_label_set_text(p->statusLabel, "Starting server...");
    lv_obj_set_style_text_font(p->statusLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(WHITE), 0);
    lv_obj_set_width(p->statusLabel, 360);
    lv_obj_set_style_text_align(p->statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(p->statusLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(p->statusLabel, LV_ALIGN_TOP_MID, 0, 130);

    /* Progress bar (hidden until upload starts) */
    p->progressBar = lv_bar_create(p->popupContainer);
    lv_obj_set_size(p->progressBar, 320, 25);
    lv_obj_align(p->progressBar, LV_ALIGN_TOP_MID, 0, 170);
    lv_bar_set_range(p->progressBar, 0, 100);
    lv_bar_set_value(p->progressBar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(p->progressBar, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_bg_color(p->progressBar, lv_color_hex(GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_radius(p->progressBar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(p->progressBar, 4, LV_PART_INDICATOR);
    lv_obj_add_flag(p->progressBar, LV_OBJ_FLAG_HIDDEN);

    /* Start the server */
    ota_wifi_server_start();

    /* Start UI update timer */
    ota_ui_timer = lv_timer_create(ota_ui_timer_cb, 500, NULL);

    LV_LOG_USER("[OTA] Wi-Fi popup created, PIN: %s", p->otaPin);
}

/* ══════════════════════════════════════════════════════════════
 *  OTA PROGRESS POPUP (used by SD update)
 *
 *  Shows: title, status text, progress bar with percentage, X close.
 * ══════════════════════════════════════════════════════════════ */

static lv_timer_t *ota_progress_timer = NULL;

static void ota_progress_timer_cb(lv_timer_t *timer) {
    (void)timer;
    struct sOtaProgressPopup *p = &gui.element.otaProgressPopup;

    const char *status = ota_get_status_text();
    if (status && status[0] != '\0')
        lv_label_set_text(p->statusLabel, status);

    int pct = ota_get_progress();
    lv_bar_set_value(p->progressBar, pct, LV_ANIM_ON);
    lv_label_set_text_fmt(p->percentLabel, "%d%%", pct);

    if (ota_is_running()) {
        lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(ORANGE), 0);
    } else if (pct >= 100) {
        lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(GREEN), 0);
        lv_obj_set_style_text_color(p->percentLabel, lv_color_hex(GREEN), 0);
    }
}

void event_otaProgressPopup(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    struct sOtaProgressPopup *p = &gui.element.otaProgressPopup;

    if (code != LV_EVENT_CLICKED) return;

    if (obj == p->closeButton) {
        safeTimerDelete(&ota_progress_timer);
        lv_obj_add_flag(p->popupParent, LV_OBJ_FLAG_HIDDEN);
        LV_LOG_USER("[OTA] Progress popup closed");
    }
}

void otaProgressPopupCreate(const char *title) {
    struct sOtaProgressPopup *p = &gui.element.otaProgressPopup;

    if (p->popupParent != NULL) {
        /* Already created — reset and show */
        lv_label_set_text(p->titleLabel, title);
        lv_label_set_text(p->statusLabel, "Starting...");
        lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(WHITE), 0);
        lv_bar_set_value(p->progressBar, 0, LV_ANIM_OFF);
        lv_label_set_text(p->percentLabel, "0%");
        lv_obj_set_style_text_color(p->percentLabel, lv_color_hex(WHITE), 0);
        lv_obj_remove_flag(p->popupParent, LV_OBJ_FLAG_HIDDEN);

        if (ota_progress_timer == NULL)
            ota_progress_timer = lv_timer_create(ota_progress_timer_cb, 200, NULL);
        return;
    }

    /* ── First-time creation ── */
    createPopupBackdrop(&p->popupParent, &p->popupContainer, 360, 200);

    /* Title */
    p->titleLabel = lv_label_create(p->popupContainer);
    lv_label_set_text(p->titleLabel, title);
    lv_obj_set_style_text_font(p->titleLabel, &lv_font_montserrat_22, 0);
    lv_obj_align(p->titleLabel, LV_ALIGN_TOP_MID, 0, -10);

    /* Underline */
    initTitleLineStyle(&p->style_titleLine, WHITE);
    p->titleLine = lv_line_create(p->popupContainer);
    lv_line_set_points(p->titleLine, p->titleLinePoints, 2);
    lv_obj_add_style(p->titleLine, &p->style_titleLine, 0);
    lv_obj_align(p->titleLine, LV_ALIGN_TOP_MID, 0, 23);

    /* X close button */
    p->closeButton = lv_button_create(p->popupContainer);
    lv_obj_set_size(p->closeButton, BUTTON_POPUP_CLOSE_WIDTH, BUTTON_POPUP_CLOSE_HEIGHT);
    lv_obj_align(p->closeButton, LV_ALIGN_TOP_RIGHT, 7, -10);
    lv_obj_add_event_cb(p->closeButton, event_otaProgressPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(p->closeButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
    lv_obj_move_foreground(p->closeButton);

    p->closeButtonLabel = lv_label_create(p->closeButton);
    lv_label_set_text(p->closeButtonLabel, closePopup_icon);
    lv_obj_set_style_text_font(p->closeButtonLabel, &FilMachineFontIcons_20, 0);
    lv_obj_align(p->closeButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* Status label */
    p->statusLabel = lv_label_create(p->popupContainer);
    lv_label_set_text(p->statusLabel, "Starting...");
    lv_obj_set_style_text_font(p->statusLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(WHITE), 0);
    lv_obj_set_width(p->statusLabel, 320);
    lv_obj_set_style_text_align(p->statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(p->statusLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(p->statusLabel, LV_ALIGN_TOP_MID, 0, 40);

    /* Progress bar */
    p->progressBar = lv_bar_create(p->popupContainer);
    lv_obj_set_size(p->progressBar, 300, 30);
    lv_obj_align(p->progressBar, LV_ALIGN_TOP_MID, 0, 85);
    lv_bar_set_range(p->progressBar, 0, 100);
    lv_bar_set_value(p->progressBar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(p->progressBar, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_bg_color(p->progressBar, lv_color_hex(GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_radius(p->progressBar, 6, LV_PART_MAIN);
    lv_obj_set_style_radius(p->progressBar, 6, LV_PART_INDICATOR);

    /* Percentage label (centered under bar) */
    p->percentLabel = lv_label_create(p->popupContainer);
    lv_label_set_text(p->percentLabel, "0%");
    lv_obj_set_style_text_font(p->percentLabel, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(p->percentLabel, lv_color_hex(WHITE), 0);
    lv_obj_set_style_text_align(p->percentLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(p->percentLabel, 320);
    lv_obj_align(p->percentLabel, LV_ALIGN_TOP_MID, 0, 125);

    /* Start timer to poll progress */
    ota_progress_timer = lv_timer_create(ota_progress_timer_cb, 200, NULL);

    LV_LOG_USER("[OTA] Progress popup created");
}
