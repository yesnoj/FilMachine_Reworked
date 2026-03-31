/**
 * @file element_wifiPopup.c
 *
 * Wi-Fi configuration popup with network scanning.
 *
 * Flow:
 *   1. User opens popup from Settings → Wi-Fi button
 *   2. Popup shows current status + auto-connect toggle
 *   3. User presses "Scan" → list populates with found networks
 *   4. User taps a network → keyboard opens for password
 *   5. User presses "Connect" → connects to selected network
 *   6. Credentials saved to machineSettings for auto-reconnect
 */

#include <string.h>
#include "FilMachine.h"
#include "ws_server.h"

extern struct gui_components gui;

#define WF             (&ui_get_profile()->wifi_popup)
#define WF_SWITCH_W    (ui_get_profile()->settings.toggle_switch_w)
#define WF_SWITCH_H    (ui_get_profile()->settings.toggle_switch_h)

/* ── Forward declarations ── */
static void wifi_populate_scan_list(void);
static void wifi_update_status(void);
static void event_wifi_network_clicked(lv_event_t *e);

/* ── Connect button re-enable (called via lv_async_call from WiFi events) ── */
static void wifi_reenable_connect_btn_async(void *arg) {
    (void)arg;
    struct sWifiPopup *p = &gui.element.wifiPopup;
    if (p->connectButton != NULL) {
        lv_obj_remove_state(p->connectButton, LV_STATE_DISABLED);
        wifi_update_status();   /* also refreshes the label text */
    }
}

/* Public: called from ota_update.c WiFi event handler */
void wifi_popup_connection_result(void) {
    lv_async_call(wifi_reenable_connect_btn_async, NULL);
}

/* ── Timer for status updates ── */
static lv_timer_t *wifi_status_timer = NULL;

static void wifi_status_timer_cb(lv_timer_t *timer) {
    (void)timer;
    wifi_update_status();
}

/* ── Auto-scan on popup open (with retry) ── */
static uint8_t wifi_scan_retries = 0;

/* Show a "Scanning..." spinner inside the list container */
static void wifi_show_scan_loading(void) {
    struct sWifiPopup *p = &gui.element.wifiPopup;
    lv_obj_clean(p->listContainer);
    lv_obj_t *spinner = lv_spinner_create(p->listContainer);
    lv_obj_set_size(spinner, WF->spinner_size, WF->spinner_size);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);
    lv_spinner_set_anim_params(spinner, 1000, 270);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(ORANGE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, WF->spinner_arc_w, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, WF->spinner_arc_w, LV_PART_MAIN);
}

/* Phase 2: actually run the blocking scan (called after spinner has rendered) */
static void wifi_do_scan_cb(lv_timer_t *timer) {
    lv_timer_delete(timer);

    struct sWifiPopup *p = &gui.element.wifiPopup;

    wifi_scan_start();
    p->scanCount = wifi_scan_get_results(p->scanResults, MAX_WIFI_SCAN_RESULTS);
    LV_LOG_USER("[WiFi] Scan found %d networks (attempt %d)", p->scanCount, wifi_scan_retries + 1);

    if (p->scanCount == 0 && wifi_scan_retries < 2) {
        /* C6 might not be ready yet — retry after 2 seconds */
        wifi_scan_retries++;
        lv_timer_create(wifi_do_scan_cb, 2000, NULL);
        return;
    }

    wifi_scan_retries = 0;
    wifi_populate_scan_list();
    wifi_update_status();
}

/* Phase 1: show spinner, then schedule the actual scan so LVGL renders first */
static void wifi_start_scan(void) {
    struct sWifiPopup *p = &gui.element.wifiPopup;
    lv_label_set_text(p->statusLabel, wifiScanning_text);
    lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(ORANGE), 0);
    wifi_show_scan_loading();
    /* Short delay so the spinner renders before the blocking scan */
    lv_timer_create(wifi_do_scan_cb, 50, NULL);
}

/* Auto-scan on popup open (one-shot timer) */
static void wifi_auto_scan_cb(lv_timer_t *timer) {
    lv_timer_delete(timer);
    wifi_scan_retries = 0;
    wifi_start_scan();
}

static void wifi_update_status(void) {
    struct sWifiPopup *p = &gui.element.wifiPopup;

    /* Always update the Settings tab Wi-Fi indicator */
    if(gui.page.menu.wifiStatusIcon != NULL) {
        if(wifi_is_connected())
            lv_obj_remove_flag(gui.page.menu.wifiStatusIcon, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(gui.page.menu.wifiStatusIcon, LV_OBJ_FLAG_HIDDEN);
    }

    /* Update popup labels only if popup exists */
    if(p->popupParent == NULL) return;

    if(wifi_is_connected()) {
        const char *ssid = wifi_get_connected_ssid();
        const char *ip = wifi_get_ip_address();
        char buf[80];
        snprintf(buf, sizeof(buf), "%s %s\n%s", wifiConnected_text, ssid ? ssid : "?", ip ? ip : "");
        lv_label_set_text(p->statusLabel, buf);
        lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(GREEN), 0);
        lv_label_set_text(p->connectButtonLabel, wifiDisconnect_text);
    } else {
        lv_label_set_text(p->statusLabel, wifiDisconnected_text);
        lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(GREY), 0);
        lv_label_set_text(p->connectButtonLabel, wifiConnect_text);
    }
}

/* ── Signal strength to bar icon ── */
static const char *rssi_to_icon(int8_t rssi) {
    if(rssi > -50) return LV_SYMBOL_WIFI;
    if(rssi > -70) return LV_SYMBOL_WIFI;
    return LV_SYMBOL_WIFI;
}

/* ── Populate scan results list ── */
static void wifi_populate_scan_list(void) {
    struct sWifiPopup *p = &gui.element.wifiPopup;

    /* Clear existing children */
    lv_obj_clean(p->listContainer);
    p->selectedIndex = -1;

    if(p->scanCount == 0) {
        lv_obj_t *lbl = lv_label_create(p->listContainer);
        lv_label_set_text(lbl, wifiNoNetworks_text);
        lv_obj_set_style_text_color(lbl, lv_color_hex(GREY), 0);
        lv_obj_set_style_text_font(lbl, WF->list_font, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    for(int i = 0; i < p->scanCount; i++) {
        lv_obj_t *btn = lv_button_create(p->listContainer);
        lv_obj_set_size(btn, lv_pct(100), WF->list_item_h);
        lv_obj_set_style_bg_color(btn, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(ORANGE_DARK), LV_STATE_CHECKED);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_style_radius(btn, WF->list_item_radius, 0);
        lv_obj_set_style_pad_left(btn, WF->list_item_pad_x, 0);
        lv_obj_set_style_pad_right(btn, WF->list_item_pad_x, 0);

        /* SSID label */
        lv_obj_t *ssid_lbl = lv_label_create(btn);
        lv_label_set_text(ssid_lbl, p->scanResults[i].ssid);
        lv_obj_set_style_text_font(ssid_lbl, WF->list_font, 0);
        lv_obj_align(ssid_lbl, LV_ALIGN_LEFT_MID, 0, 0);

        /* Signal icon */
        lv_obj_t *sig_lbl = lv_label_create(btn);
        lv_label_set_text(sig_lbl, rssi_to_icon(p->scanResults[i].rssi));
        lv_obj_set_style_text_font(sig_lbl, WF->list_font, 0);
        lv_obj_align(sig_lbl, LV_ALIGN_RIGHT_MID, 0, 0);

        /* Open network indicator */
        if(p->scanResults[i].open) {
            lv_obj_set_style_text_color(sig_lbl, lv_color_hex(GREEN), 0);
        }

        /* Store index in user_data and add click handler */
        lv_obj_set_user_data(btn, (void *)(intptr_t)i);
        lv_obj_add_event_cb(btn, event_wifi_network_clicked, LV_EVENT_CLICKED, NULL);
    }
}

/* ── Network item clicked ── */
static void event_wifi_network_clicked(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    struct sWifiPopup *p = &gui.element.wifiPopup;
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);

    /* Uncheck all other buttons */
    uint32_t cnt = lv_obj_get_child_cnt(p->listContainer);
    for(uint32_t i = 0; i < cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(p->listContainer, i);
        if(child != btn) lv_obj_remove_state(child, LV_STATE_CHECKED);
    }
    lv_obj_add_state(btn, LV_STATE_CHECKED);

    p->selectedIndex = idx;
    LV_LOG_USER("[WiFi] Selected network: %s (rssi: %d)", p->scanResults[idx].ssid, p->scanResults[idx].rssi);

    /* If network requires password, open keyboard */
    if(!p->scanResults[idx].open) {
        p->pendingPassword[0] = '\0';

        /* Set up keyboard context and activate it */
        p->wifiPasswordKbCtx = (sKeyboardOwnerContext){
            .owner       = KB_OWNER_SETTINGS,
            .textArea    = NULL,  /* No visible textarea — commit directly */
            .parentScreen = p->popupParent,
            .saveButton  = NULL,
            .ownerData   = p->pendingPassword,
            .maxLength   = 64
        };
        kb_ctx_set(&p->wifiPasswordKbCtx);

        /* Use the existing keyboard popup — set max length and show it */
        lv_textarea_set_max_length(gui.element.keyboardPopup.keyboardTextArea, 64);

        /* Pre-fill with saved password if same SSID */
        if(strcmp(p->scanResults[idx].ssid, gui.page.settings.settingsParams.wifiSSID) == 0 &&
           gui.page.settings.settingsParams.wifiPassword[0] != '\0') {
            lv_textarea_set_text(gui.element.keyboardPopup.keyboardTextArea,
                                 gui.page.settings.settingsParams.wifiPassword);
        } else {
            lv_textarea_set_text(gui.element.keyboardPopup.keyboardTextArea, "");
        }

        lv_keyboard_set_mode(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_3);
        lv_obj_remove_flag(gui.element.keyboardPopup.keyBoardParent, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(gui.element.keyboardPopup.keyBoardParent);
    }
}

/* ── Main event handler ── */
void event_wifiPopup(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    struct sWifiPopup *p = &gui.element.wifiPopup;

    if(code != LV_EVENT_CLICKED && code != LV_EVENT_VALUE_CHANGED) return;

    /* Close button — keep timer running to update Settings tab icon */
    if(obj == p->closeButton && code == LV_EVENT_CLICKED) {
        lv_obj_add_flag(p->popupParent, LV_OBJ_FLAG_HIDDEN);
        LV_LOG_USER("[WiFi] Popup closed");
        return;
    }

    /* Scan / Refresh button */
    if(obj == p->scanButton && code == LV_EVENT_CLICKED) {
        LV_LOG_USER("[WiFi] Manual scan started");
        wifi_scan_retries = 0;
        wifi_start_scan();
        return;
    }

    /* Connect/Disconnect button */
    if(obj == p->connectButton && code == LV_EVENT_CLICKED) {
        if(wifi_is_connected()) {
            /* Disconnect */
            LV_LOG_USER("[WiFi] Disconnecting");
            ws_server_stop();
            wifi_disconnect();
            wifi_update_status();
        } else {
            /* Connect to selected network */
            if(p->selectedIndex >= 0 && p->selectedIndex < p->scanCount) {
                const char *ssid = p->scanResults[p->selectedIndex].ssid;
                const char *pass = "";

                if(!p->scanResults[p->selectedIndex].open) {
                    /* Get password from pendingPassword (set by keyboard commit) or saved */
                    if(p->pendingPassword[0] != '\0') {
                        pass = p->pendingPassword;
                    } else if(strcmp(ssid, gui.page.settings.settingsParams.wifiSSID) == 0) {
                        pass = gui.page.settings.settingsParams.wifiPassword;
                    }
                }

                LV_LOG_USER("[WiFi] Connecting to: %s", ssid);
                lv_label_set_text(p->statusLabel, wifiConnecting_text);
                lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(ORANGE), 0);

                /* Disable button while connection is in progress */
                lv_obj_add_state(p->connectButton, LV_STATE_DISABLED);

                if(wifi_connect(ssid, pass)) {
                    /* Save credentials */
                    snprintf(gui.page.settings.settingsParams.wifiSSID,
                             sizeof(gui.page.settings.settingsParams.wifiSSID), "%s", ssid);
                    snprintf(gui.page.settings.settingsParams.wifiPassword,
                             sizeof(gui.page.settings.settingsParams.wifiPassword), "%s", pass);
                    /* Start WebSocket server */
                    ws_server_start(WS_SERVER_PORT);
                    qSysAction(SAVE_PROCESS_CONFIG);
                } else {
                    /* Immediate failure — re-enable button right away */
                    lv_obj_remove_state(p->connectButton, LV_STATE_DISABLED);
                }
                wifi_update_status();
            }
        }
        return;
    }

    /* Auto-connect switch */
    if(obj == p->autoConnectSwitch && code == LV_EVENT_VALUE_CHANGED) {
        gui.page.settings.settingsParams.wifiEnabled = lv_obj_has_state(obj, LV_STATE_CHECKED);
        LV_LOG_USER("[WiFi] Auto-connect: %s", gui.page.settings.settingsParams.wifiEnabled ? "ON" : "OFF");
        qSysAction(SAVE_PROCESS_CONFIG);
        return;
    }
}

/* ── Create / show the Wi-Fi popup ── */
void wifiPopupCreate(void) {
    struct sWifiPopup *p = &gui.element.wifiPopup;

    if(p->popupParent != NULL) {
        /* Already created — just show */
        wifi_update_status();
        if(gui.page.settings.settingsParams.wifiEnabled)
            lv_obj_add_state(p->autoConnectSwitch, LV_STATE_CHECKED);
        else
            lv_obj_remove_state(p->autoConnectSwitch, LV_STATE_CHECKED);
        lv_obj_remove_flag(p->popupParent, LV_OBJ_FLAG_HIDDEN);
        if(wifi_status_timer == NULL)
            wifi_status_timer = lv_timer_create(wifi_status_timer_cb, 1000, NULL);
        /* Auto-scan after short delay so UI renders first */
        lv_timer_create(wifi_auto_scan_cb, 300, NULL);
        return;
    }

    /* ── First-time creation ── */
    /* Use OTA popup size for now */
    createPopupBackdrop(&p->popupParent, &p->popupContainer,
                        ui_get_profile()->popups.ota_wifi_w,
                        ui_get_profile()->popups.ota_wifi_h);

    const ui_ota_popup_layout_t *ui = &ui_get_profile()->ota_popup;

    /* Title */
    p->titleLabel = lv_label_create(p->popupContainer);
    lv_label_set_text(p->titleLabel, wifiPopupTitle_text);
    lv_obj_set_style_text_font(p->titleLabel, ui->title_font, 0);
    lv_obj_align(p->titleLabel, LV_ALIGN_TOP_MID, ui->title_x, ui->title_y);

    /* Underline */
    initTitleLineStyle(&p->style_titleLine, ORANGE);
    p->titleLinePoints[1].x = WF->title_line_w;
    p->titleLine = lv_line_create(p->popupContainer);
    lv_line_set_points(p->titleLine, p->titleLinePoints, 2);
    lv_obj_add_style(p->titleLine, &p->style_titleLine, 0);
    lv_obj_align(p->titleLine, LV_ALIGN_TOP_MID, 0, ui->title_line_y);

    /* X close button */
    p->closeButton = lv_button_create(p->popupContainer);
    lv_obj_set_size(p->closeButton, ui->close_w, ui->close_h);
    lv_obj_align(p->closeButton, LV_ALIGN_TOP_RIGHT, ui->close_x, ui->close_y);
    lv_obj_add_event_cb(p->closeButton, event_wifiPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(p->closeButton, lv_color_hex(ORANGE), LV_PART_MAIN);
    lv_obj_move_foreground(p->closeButton);

    p->closeButtonLabel = lv_label_create(p->closeButton);
    lv_label_set_text(p->closeButtonLabel, closePopup_icon);
    lv_obj_set_style_text_font(p->closeButtonLabel, ui->close_icon_font, 0);
    lv_obj_align(p->closeButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* Status label */
    p->statusLabel = lv_label_create(p->popupContainer);
    lv_label_set_text(p->statusLabel, wifiDisconnected_text);
    lv_obj_set_style_text_font(p->statusLabel, WF->status_font, 0);
    lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(GREY), 0);
    lv_obj_set_width(p->statusLabel, lv_pct(WF->status_w_pct));
    lv_obj_set_style_text_align(p->statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(p->statusLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(p->statusLabel, LV_ALIGN_TOP_MID, 0, WF->status_y);

    /* Scan button — icon only */
    p->scanButton = lv_button_create(p->popupContainer);
    lv_obj_set_size(p->scanButton, WF->scan_btn_w, WF->scan_btn_h);
    lv_obj_align(p->scanButton, LV_ALIGN_TOP_RIGHT, ui->close_x, WF->scan_btn_y);
    lv_obj_add_event_cb(p->scanButton, event_wifiPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(p->scanButton, lv_color_hex(ORANGE), LV_PART_MAIN);

    p->scanButtonLabel = lv_label_create(p->scanButton);
    lv_label_set_text(p->scanButtonLabel, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(p->scanButtonLabel, WF->scan_btn_font, 0);
    lv_obj_align(p->scanButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* Scrollable list container for scan results */
    p->listContainer = lv_obj_create(p->popupContainer);
    lv_obj_set_size(p->listContainer, lv_pct(WF->list_w_pct), WF->list_h);
    lv_obj_align(p->listContainer, LV_ALIGN_TOP_MID, 0, WF->list_y);
    lv_obj_set_scroll_dir(p->listContainer, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(p->listContainer, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_flex_flow(p->listContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p->listContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(p->listContainer, WF->list_pad_row, 0);
    lv_obj_set_style_pad_all(p->listContainer, WF->list_pad_all, 0);
    lv_obj_set_style_bg_color(p->listContainer, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_color(p->listContainer, lv_color_hex(ORANGE_DARK), 0);
    lv_obj_set_style_border_width(p->listContainer, WF->list_border_w, 0);
    lv_obj_set_style_radius(p->listContainer, WF->list_radius, 0);

    /* Bottom row: auto-connect toggle + connect button */
    /* Auto-connect switch */
    p->autoConnectContainer = lv_obj_create(p->popupContainer);
    lv_obj_set_size(p->autoConnectContainer, lv_pct(WF->autoconnect_container_w_pct), WF->autoconnect_container_h);
    lv_obj_align(p->autoConnectContainer, LV_ALIGN_BOTTOM_LEFT, WF->autoconnect_container_x, WF->autoconnect_container_y);
    lv_obj_remove_flag(p->autoConnectContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_opa(p->autoConnectContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(p->autoConnectContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(p->autoConnectContainer, 0, 0);

    p->autoConnectLabel = lv_label_create(p->autoConnectContainer);
    lv_label_set_text(p->autoConnectLabel, wifiAutoConnect_text);
    lv_obj_set_style_text_font(p->autoConnectLabel, WF->autoconnect_font, 0);
    lv_obj_align(p->autoConnectLabel, LV_ALIGN_LEFT_MID, 0, 0);

    p->autoConnectSwitch = lv_switch_create(p->autoConnectContainer);
    lv_obj_set_size(p->autoConnectSwitch, WF_SWITCH_W, WF_SWITCH_H);
    lv_obj_align_to(p->autoConnectSwitch, p->autoConnectLabel, LV_ALIGN_OUT_RIGHT_MID, WF->autoconnect_switch_gap, 0);
    lv_obj_add_event_cb(p->autoConnectSwitch, event_wifiPopup, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_bg_color(p->autoConnectSwitch, lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(p->autoConnectSwitch, lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(p->autoConnectSwitch, lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
    if(gui.page.settings.settingsParams.wifiEnabled)
        lv_obj_add_state(p->autoConnectSwitch, LV_STATE_CHECKED);

    /* Connect button — sized like Run/Cancel in checkup */
    p->connectButton = lv_button_create(p->popupContainer);
    lv_obj_set_size(p->connectButton, WF->connect_btn_w, WF->connect_btn_h);
    lv_obj_align(p->connectButton, LV_ALIGN_BOTTOM_RIGHT, WF->connect_btn_x, WF->connect_btn_y);
    lv_obj_add_event_cb(p->connectButton, event_wifiPopup, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(p->connectButton, lv_color_hex(ORANGE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(p->connectButton, LV_OPA_50, LV_STATE_DISABLED);
    lv_obj_set_style_text_opa(p->connectButton, LV_OPA_50, LV_STATE_DISABLED);

    p->connectButtonLabel = lv_label_create(p->connectButton);
    lv_label_set_text(p->connectButtonLabel, wifiConnect_text);
    lv_obj_set_style_text_font(p->connectButtonLabel, WF->connect_btn_font, 0);
    lv_obj_align(p->connectButtonLabel, LV_ALIGN_CENTER, 0, 0);

    /* Initial state */
    p->scanCount = 0;
    p->selectedIndex = -1;
    wifi_update_status();

    /* Start status timer */
    wifi_status_timer = lv_timer_create(wifi_status_timer_cb, 1000, NULL);

    /* Auto-scan after short delay so UI renders first */
    lv_timer_create(wifi_auto_scan_cb, 300, NULL);

    LV_LOG_USER("[WiFi] Popup created");
}
