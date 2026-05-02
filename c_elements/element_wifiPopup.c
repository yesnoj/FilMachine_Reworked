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
#include "esp_log.h"
#include "FilMachine.h"
#include "ws_server.h"

static const char *TAG = "WiFiPopup";

extern struct gui_components gui;

#define WF             (&ui_get_profile()->wifi_popup)
#define WF_SWITCH_W    (ui_get_profile()->settings.toggle_switch_w)
#define WF_SWITCH_H    (ui_get_profile()->settings.toggle_switch_h)

/* ── Forward declarations ── */
static void wifi_populate_scan_list(void);
static void wifi_update_status(void);
static void event_wifi_network_clicked(lv_event_t *e);
static void event_wifi_network_long_pressed(lv_event_t *e);
static void wifi_start_scan(void);

/* ── Scan retry counter ── */
static uint8_t wifi_scan_retries = 0;

/* ── Timer for status updates ── */
static lv_timer_t *wifi_status_timer = NULL;

static void wifi_status_timer_cb(lv_timer_t *timer) {
    (void)timer;
    wifi_update_status();
}

/* ── Pending credentials (saved to settingsParams only on GOT_IP) ── */
static char wifi_pending_ssid[33] = {0};
static char wifi_pending_pass[65] = {0};

/* ── WiFi icon blink timer for "connecting" state ── */
static lv_timer_t *wifi_blink_timer = NULL;
static bool wifi_blink_visible = false;

static void wifi_blink_timer_cb(lv_timer_t *timer) {
    (void)timer;
    if (gui.page.menu.wifiStatusIcon == NULL) return;
    wifi_blink_visible = !wifi_blink_visible;
    if (wifi_blink_visible)
        lv_obj_remove_flag(gui.page.menu.wifiStatusIcon, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(gui.page.menu.wifiStatusIcon, LV_OBJ_FLAG_HIDDEN);
}

static void wifi_icon_stop_blink(void) {
    if (wifi_blink_timer != NULL) {
        lv_timer_delete(wifi_blink_timer);
        wifi_blink_timer = NULL;
    }
}

/* ── Update Settings tab Wi-Fi icon (safe from any context via lv_async_call) ── */
static void wifi_update_icon_async(void *arg) {
    (void)arg;
    if (gui.page.menu.wifiStatusIcon == NULL) return;

    wifi_icon_stop_blink();

    if (wifi_is_connected()) {
        lv_obj_set_style_text_color(gui.page.menu.wifiStatusIcon, lv_color_hex(GREEN), 0);
        lv_obj_remove_flag(gui.page.menu.wifiStatusIcon, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(gui.page.menu.wifiStatusIcon, LV_OBJ_FLAG_HIDDEN);
    }
}

/* Start blinking white icon (called via lv_async_call when connecting) */
static void wifi_icon_start_blink_async(void *arg) {
    (void)arg;
    if (gui.page.menu.wifiStatusIcon == NULL) return;

    wifi_icon_stop_blink();
    lv_obj_set_style_text_color(gui.page.menu.wifiStatusIcon, lv_color_white(), 0);
    lv_obj_remove_flag(gui.page.menu.wifiStatusIcon, LV_OBJ_FLAG_HIDDEN);
    wifi_blink_visible = true;
    wifi_blink_timer = lv_timer_create(wifi_blink_timer_cb, 500, NULL);
}

/* Public: call when a WiFi connection attempt begins */
void wifi_icon_set_connecting(void) {
    lv_async_call(wifi_icon_start_blink_async, NULL);
}

/* ── Connect succeeded (GOT_IP) — save credentials and update UI ── */
static void wifi_connect_success_async(void *arg) {
    (void)arg;
    struct sWifiPopup *p = &gui.element.wifiPopup;

    /* Persist pending credentials now that connection succeeded */
    if (wifi_pending_ssid[0] != '\0') {
        snprintf(gui.page.settings.settingsParams.wifiSSID,
                 sizeof(gui.page.settings.settingsParams.wifiSSID), "%s", wifi_pending_ssid);
        snprintf(gui.page.settings.settingsParams.wifiPassword,
                 sizeof(gui.page.settings.settingsParams.wifiPassword), "%s", wifi_pending_pass);
        ESP_LOGW(TAG, "[WiFi] Credentials saved: ssid='%s', pwd_len=%d, wifiEnabled=%d",
                 gui.page.settings.settingsParams.wifiSSID,
                 (int)strlen(gui.page.settings.settingsParams.wifiPassword),
                 gui.page.settings.settingsParams.wifiEnabled);
        qSysAction(SAVE_PROCESS_CONFIG);
        /* Also persist to NVS for SD-card-free boot */
        wifi_nvs_save(gui.page.settings.settingsParams.wifiSSID,
                      gui.page.settings.settingsParams.wifiPassword);
        wifi_pending_ssid[0] = '\0';
        wifi_pending_pass[0] = '\0';
    }

    if (p->connectButton != NULL)
        lv_obj_remove_state(p->connectButton, LV_STATE_DISABLED);
    wifi_update_icon_async(NULL);
    if (p->popupParent != NULL) wifi_update_status();
}

/* ── Connect failed (disconnect) — clear pending and update UI ── */
static void wifi_connect_fail_async(void *arg) {
    (void)arg;
    struct sWifiPopup *p = &gui.element.wifiPopup;

    /* Discard pending credentials — don't save wrong password */
    wifi_pending_ssid[0] = '\0';
    wifi_pending_pass[0] = '\0';

    if (p->connectButton != NULL)
        lv_obj_remove_state(p->connectButton, LV_STATE_DISABLED);
    wifi_update_icon_async(NULL);
    if (p->popupParent != NULL) wifi_update_status();
}

/* Public: called from ota_update.c on GOT_IP */
void wifi_popup_connection_result(void) {
    lv_async_call(wifi_connect_success_async, NULL);
}

/* Public: called from ota_update.c on disconnect/failure */
void wifi_popup_connection_failed(void) {
    lv_async_call(wifi_connect_fail_async, NULL);
}

/* ── Delayed retry timer (gives ESP-Hosted C6 time to finish init) ── */
static void wifi_retry_scan_timer_cb(lv_timer_t *timer) {
    lv_timer_delete(timer);
    ESP_LOGW(TAG, "[WiFi] Retry scan %d/3 (after delay)", wifi_scan_retries);
    wifi_start_scan();
}

/* ── Scan-done callback (called via lv_async_call from SCAN_DONE event) ── */
static void wifi_scan_done_async(void *arg) {
    (void)arg;
    struct sWifiPopup *p = &gui.element.wifiPopup;

    /* If popup isn't open, ignore */
    if (p->popupParent == NULL) return;

    ESP_LOGW(TAG, "[WiFi] Scan done — %d networks", p->scanCount);

    if (p->scanCount == 0 && wifi_scan_retries < 3) {
        /* C6 might not be ready yet — retry after a delay */
        wifi_scan_retries++;
        /* Wait 2s before retrying to give ESP-Hosted transport time to settle */
        lv_timer_create(wifi_retry_scan_timer_cb, 2000, NULL);
        return;
    }

    wifi_scan_retries = 0;
    wifi_populate_scan_list();
    wifi_update_status();
}

/* Public: called from ota_update.c SCAN_DONE event handler */
void wifi_popup_scan_done(void) {
    lv_async_call(wifi_scan_done_async, NULL);
}

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

/* Start async scan: show spinner, launch non-blocking scan.
 * Results arrive via SCAN_DONE → wifi_popup_scan_done → wifi_scan_done_async */
static void wifi_start_scan(void) {
    struct sWifiPopup *p = &gui.element.wifiPopup;
    lv_label_set_text(p->statusLabel, wifiScanning_text);
    lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(ORANGE), 0);
    wifi_show_scan_loading();
    int ret = wifi_scan_start();  /* non-blocking — results come via event */
    if (ret < 0 && wifi_scan_retries < 3) {
        /* Scan failed (WiFi not ready) — schedule a delayed retry */
        wifi_scan_retries++;
        ESP_LOGW(TAG, "[WiFi] Scan start failed, scheduling retry %d/3 in 2s", wifi_scan_retries);
        lv_timer_create(wifi_retry_scan_timer_cb, 2000, NULL);
    }
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
        /* Check if this network has saved credentials */
        bool is_saved = (!p->scanResults[i].open &&
                         strcmp(p->scanResults[i].ssid, gui.page.settings.settingsParams.wifiSSID) == 0 &&
                         gui.page.settings.settingsParams.wifiPassword[0] != '\0');

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
        if (is_saved) {
            /* Show a star before the SSID to indicate saved credentials */
            char saved_ssid[40];
            snprintf(saved_ssid, sizeof(saved_ssid), LV_SYMBOL_OK " %s", p->scanResults[i].ssid);
            lv_label_set_text(ssid_lbl, saved_ssid);
        } else {
            lv_label_set_text(ssid_lbl, p->scanResults[i].ssid);
        }
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

        /* Store index in user_data and add click/long-press handlers */
        lv_obj_set_user_data(btn, (void *)(intptr_t)i);
        lv_obj_add_event_cb(btn, event_wifi_network_clicked, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(btn, event_wifi_network_long_pressed, LV_EVENT_LONG_PRESSED, NULL);
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
    ESP_LOGW(TAG, "[WiFi] Selected network: %s (rssi: %d)", p->scanResults[idx].ssid, p->scanResults[idx].rssi);

    /* Check if this network has saved credentials */
    bool is_saved = (!p->scanResults[idx].open &&
                     strcmp(p->scanResults[idx].ssid, gui.page.settings.settingsParams.wifiSSID) == 0 &&
                     gui.page.settings.settingsParams.wifiPassword[0] != '\0');

    if (is_saved) {
        /* Known network — pre-load saved password, no keyboard needed */
        snprintf(p->pendingPassword, sizeof(p->pendingPassword), "%s",
                 gui.page.settings.settingsParams.wifiPassword);
        ESP_LOGW(TAG, "[WiFi] Saved network — password loaded, ready to connect");
        return;
    }

    /* If network requires password and not saved, open keyboard */
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
        lv_textarea_set_text(gui.element.keyboardPopup.keyboardTextArea, "");

        lv_keyboard_set_mode(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_3);
        lv_obj_remove_flag(gui.element.keyboardPopup.keyBoardParent, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(gui.element.keyboardPopup.keyBoardParent);
    }
}

/* ── Network item long-pressed — offer to forget saved credentials ── */
static void event_wifi_network_long_pressed(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    struct sWifiPopup *p = &gui.element.wifiPopup;
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);

    if (idx < 0 || idx >= p->scanCount) return;

    /* Only show "Forget" for saved networks */
    bool is_saved = (!p->scanResults[idx].open &&
                     strcmp(p->scanResults[idx].ssid, gui.page.settings.settingsParams.wifiSSID) == 0 &&
                     gui.page.settings.settingsParams.wifiPassword[0] != '\0');
    if (!is_saved) return;

    ESP_LOGW(TAG, "[WiFi] Long press on saved network: %s — showing forget popup", p->scanResults[idx].ssid);

    /* Use listContainer as whoCallMe identifier for MSGPOP_OWNER_WIFI_FORGET */
    messagePopupCreate(wifiForgetTitle_text, wifiForgetBody_text,
                       wifiForgetYes_text, wifiForgetNo_text,
                       p->listContainer);
}

/* ── Public: re-populate scan list (called after forgetting a network) ── */
void wifi_popup_refresh_list(void) {
    struct sWifiPopup *p = &gui.element.wifiPopup;
    if (p->popupParent == NULL) return;
    wifi_populate_scan_list();
    wifi_update_status();
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
        ESP_LOGW(TAG, "[WiFi] Popup closed");
        return;
    }

    /* Scan / Refresh button */
    if(obj == p->scanButton && code == LV_EVENT_CLICKED) {
        ESP_LOGW(TAG, "[WiFi] Manual scan started");
        wifi_scan_retries = 0;
        wifi_start_scan();
        return;
    }

    /* Connect/Disconnect button */
    if(obj == p->connectButton && code == LV_EVENT_CLICKED) {
        if(wifi_is_connected()) {
            /* Disconnect */
            ESP_LOGW(TAG, "[WiFi] Disconnecting");
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

                ESP_LOGW(TAG, "[WiFi] Connecting to: %s", ssid);
                lv_label_set_text(p->statusLabel, wifiConnecting_text);
                lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(ORANGE), 0);

                /* Disable button while connection is in progress */
                lv_obj_add_state(p->connectButton, LV_STATE_DISABLED);
                wifi_icon_set_connecting();  /* blink white icon */

                if(wifi_connect(ssid, pass)) {
                    /* Store credentials temporarily — only persist on GOT_IP */
                    snprintf(wifi_pending_ssid, sizeof(wifi_pending_ssid), "%s", ssid);
                    snprintf(wifi_pending_pass, sizeof(wifi_pending_pass), "%s", pass);
                    ESP_LOGW(TAG, "[WiFi] Pending credentials: ssid='%s', pwd_len=%d",
                             wifi_pending_ssid, (int)strlen(wifi_pending_pass));
                    /* Start WebSocket server */
                    ws_server_start(WS_SERVER_PORT);
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
        ESP_LOGW(TAG, "[WiFi] Auto-connect switch toggled: %s (wifiEnabled=%d)",
                 gui.page.settings.settingsParams.wifiEnabled ? "ON" : "OFF",
                 gui.page.settings.settingsParams.wifiEnabled);
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
        /* Show spinner immediately */
        lv_label_set_text(p->statusLabel, wifiScanning_text);
        lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(ORANGE), 0);
        wifi_show_scan_loading();
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

    /* Show spinner immediately so user sees loading feedback */
    lv_label_set_text(p->statusLabel, wifiScanning_text);
    lv_obj_set_style_text_color(p->statusLabel, lv_color_hex(ORANGE), 0);
    wifi_show_scan_loading();

    /* Start status timer */
    wifi_status_timer = lv_timer_create(wifi_status_timer_cb, 1000, NULL);

    /* Auto-scan after short delay so UI renders first */
    lv_timer_create(wifi_auto_scan_cb, 300, NULL);

    ESP_LOGW(TAG, "[WiFi] Popup created");
}
