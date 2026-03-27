/**
 * @file ws_server.c
 *
 * WebSocket server for remote control of FilMachine.
 *
 * Simulator build: lightweight stub that logs commands.
 * Firmware build:  real esp_http_server with WebSocket support.
 *
 * JSON is built with snprintf (no cJSON dependency).
 * Command parsing uses simple strstr matching on small messages.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FilMachine.h"
#include "ws_server.h"

extern struct gui_components gui;

/* ── Forward declarations ────────────────────────────────────────── */
extern void checkup(processNode *pn);
static sCheckupData *find_active_checkup(void);

/* ── Find process node by index ─────────────────────────────────── */
static processNode *find_process_by_index(int index) {
    processNode *node = gui.page.processes.processElementsList.start;
    int i = 0;
    while (node) {
        if (i == index) return node;
        node = node->next;
        i++;
    }
    return NULL;
}

/* ── Async callback: start process on LVGL thread ───────────────── */
static void ws_async_start_process(void *user_data) {
    processNode *pn = (processNode *)user_data;
    if (pn == NULL || pn->process.processDetails == NULL) return;
    if (pn->process.processDetails->checkup == NULL) return;

    /* Guard: don't start if a process is already running */
    if (find_active_checkup()) {
        LV_LOG_WARN("[WS] Cannot start: a process is already running");
        return;
    }

    /* Prepare and launch — same as process_detail_run but without
       assuming processDetail page is open */
    pn->process.processDetails->checkup->checkupParent = NULL;
    lv_style_reset(&pn->process.processDetails->textAreaStyle);
    checkup(pn);
    LV_LOG_USER("[WS] Process started via remote: %s",
                pn->process.processDetails->data.processNameString);
}

/* ── Find the currently running process's checkup data ──────────── */
static sCheckupData *find_active_checkup(void) {
    processNode *node = gui.page.processes.processElementsList.start;
    while (node) {
        if (node->process.processDetails &&
            node->process.processDetails->checkup &&
            node->process.processDetails->checkup->data.isProcessing) {
            return &node->process.processDetails->checkup->data;
        }
        node = node->next;
    }
    return NULL;
}

/* ── Build full state JSON ───────────────────────────────────────── */
static int build_state_json(char *buf, int bufsize) {
    struct machineSettings *s = &gui.page.settings.settingsParams;
    static sCheckupData ck_default;  /* zeroed — used when no process runs */
    sCheckupData *ck = find_active_checkup();
    if (!ck) { memset(&ck_default, 0, sizeof(ck_default)); ck = &ck_default; }
    machineStatistics *st = &gui.page.tools.machineStats;

    int n = snprintf(buf, bufsize,
        "{\"type\":\"state\",\"data\":{"
        /* Settings */
        "\"tempUnit\":%d,"
        "\"waterInlet\":%s,"
        "\"calibratedTemp\":%u,"
        "\"tempCalibOffset\":%d,"
        "\"filmRotationSpeed\":%u,"
        "\"rotationInterval\":%u,"
        "\"random\":%u,"
        "\"persistentAlarm\":%s,"
        "\"autostart\":%s,"
        "\"drainFillOverlap\":%u,"
        "\"multiRinseTime\":%u,"
        "\"tankSize\":%u,"
        "\"pumpSpeed\":%u,"
        "\"chemContainerMl\":%u,"
        "\"wbContainerMl\":%u,"
        "\"chemistryVolume\":%u,"
        /* Wi-Fi */
        "\"wifiEnabled\":%s,"
        "\"wifiSSID\":\"%s\","
        "\"wifiConnected\":%s,"
        /* Runtime */
        "\"isProcessing\":%s,"
        "\"processStep\":%u,"
        "\"stopNow\":%s,"
        "\"stopAfter\":%s,"
        "\"isFilling\":%s,"
        "\"isDeveloping\":%s,"
        "\"waterTemp\":%.1f,"
        "\"chemTemp\":%.1f,"
        "\"heaterOn\":%s,"
        "\"fillStatus\":%u,"
        "\"tempStatus\":%u,"
        "\"filmStatus\":%u,"
        /* Statistics */
        "\"statsCompleted\":%lu,"
        "\"statsTotalMins\":%llu,"
        "\"statsStopped\":%lu,"
        "\"statsClean\":%lu,"
        /* Alarm */
        "\"alarmActive\":%s"
        "}}",
        (int)s->tempUnit,
        s->waterInlet ? "true" : "false",
        (unsigned)s->calibratedTemp,
        (int)s->tempCalibOffset,
        (unsigned)s->filmRotationSpeedSetpoint,
        (unsigned)s->rotationIntervalSetpoint,
        (unsigned)s->randomSetpoint,
        s->isPersistentAlarm ? "true" : "false",
        s->isProcessAutostart ? "true" : "false",
        (unsigned)s->drainFillOverlapSetpoint,
        (unsigned)s->multiRinseTime,
        (unsigned)s->tankSize,
        (unsigned)s->pumpSpeed,
        (unsigned)s->chemContainerMl,
        (unsigned)s->wbContainerMl,
        (unsigned)s->chemistryVolume,
        s->wifiEnabled ? "true" : "false",
        s->wifiSSID,
        wifi_is_connected() ? "true" : "false",
        ck->isProcessing ? "true" : "false",
        (unsigned)ck->processStep,
        ck->stopNow ? "true" : "false",
        ck->stopAfter ? "true" : "false",
        ck->isFilling ? "true" : "false",
        ck->isDeveloping ? "true" : "false",
        ck->currentWaterTemp,
        ck->currentChemTemp,
        ck->heaterOn ? "true" : "false",
        (unsigned)ck->stepFillWaterStatus,
        (unsigned)ck->stepReachTempStatus,
        (unsigned)ck->stepCheckFilmStatus,
        (unsigned long)st->completed,
        (unsigned long long)st->totalMins,
        (unsigned long)st->stopped,
        (unsigned long)st->clean,
        alarm_is_active() ? "true" : "false"
    );
    return n;
}

/* ── Build process list JSON ─────────────────────────────────────── */
static int build_process_list_json(char *buf, int bufsize) {
    int n = snprintf(buf, bufsize, "{\"type\":\"process_list\",\"data\":[");
    if (n < 0 || n >= bufsize) return 0;

    processNode *node = gui.page.processes.processElementsList.start;
    int idx = 0;
    while (node != NULL && n < bufsize - 200) {
        sProcessData *pd = &node->process.processDetails->data;

        /* Build step array inline */
        char steps_buf[1024] = "[";
        int sn = 1;
        stepNode *snode = node->process.processDetails->stepElementsList.start;
        int si = 0;
        while (snode != NULL && sn < (int)sizeof(steps_buf) - 150) {
            sStepData *sd = &snode->step.stepDetails->data;
            if (si > 0) steps_buf[sn++] = ',';
            sn += snprintf(steps_buf + sn, sizeof(steps_buf) - sn,
                "{\"name\":\"%s\",\"mins\":%u,\"secs\":%u,\"type\":%d,\"source\":%u,\"discard\":%s}",
                sd->stepNameString,
                (unsigned)sd->timeMins,
                (unsigned)sd->timeSecs,
                (int)sd->type,
                (unsigned)sd->source,
                sd->discardAfterProc ? "true" : "false"
            );
            snode = snode->next;
            si++;
        }
        snprintf(steps_buf + sn, sizeof(steps_buf) - sn, "]");

        if (idx > 0) buf[n++] = ',';
        n += snprintf(buf + n, bufsize - n,
            "{\"index\":%d,\"name\":\"%s\",\"temp\":%u,\"tolerance\":%.1f,"
            "\"tempControlled\":%s,\"preferred\":%s,\"filmType\":%d,"
            "\"mins\":%u,\"secs\":%u,\"steps\":%s}",
            idx,
            pd->processNameString,
            (unsigned)pd->temp,
            pd->tempTolerance,
            pd->isTempControlled ? "true" : "false",
            pd->isPreferred ? "true" : "false",
            (int)pd->filmType,
            (unsigned)pd->timeMins,
            (unsigned)pd->timeSecs,
            steps_buf
        );

        node = node->next;
        idx++;
    }

    n += snprintf(buf + n, bufsize - n, "]}");
    return n;
}

/* ── Command dispatch (shared logic) ─────────────────────────────── */
static void ws_handle_command(const char *msg, int len) {
    (void)len;

    /* ── get_state ── */
    if (strstr(msg, "\"get_state\"")) {
        ws_broadcast_state();
        return;
    }

    /* ── get_processes ── */
    if (strstr(msg, "\"get_processes\"")) {
        ws_broadcast_process_list();
        return;
    }

    /* ── set_setting ── */
    if (strstr(msg, "\"set_setting\"")) {
        struct machineSettings *s = &gui.page.settings.settingsParams;

        /* Parse key and value with simple extraction */
        const char *kp = strstr(msg, "\"key\":");
        const char *vp = strstr(msg, "\"value\":");
        if (!kp || !vp) return;

        /* Extract key string */
        const char *ks = strchr(kp + 6, '"');
        if (!ks) return;
        ks++;
        const char *ke = strchr(ks, '"');
        if (!ke) return;
        int klen = (int)(ke - ks);

        /* Extract value (number or bool) */
        const char *vs = vp + 8;
        while (*vs == ' ') vs++;

        /* Match known settings keys */
        #define KEY_IS(k) (klen == (int)strlen(k) && strncmp(ks, k, klen) == 0)

        if (KEY_IS("tempUnit"))             s->tempUnit = (tempUnit_t)atoi(vs);
        else if (KEY_IS("waterInlet"))      s->waterInlet = (strstr(vs, "true") != NULL);
        else if (KEY_IS("calibratedTemp"))  s->calibratedTemp = (uint8_t)atoi(vs);
        else if (KEY_IS("tempCalibOffset")) s->tempCalibOffset = (int8_t)atoi(vs);
        else if (KEY_IS("filmRotationSpeed"))  s->filmRotationSpeedSetpoint = (uint8_t)atoi(vs);
        else if (KEY_IS("rotationInterval"))   s->rotationIntervalSetpoint = (uint8_t)atoi(vs);
        else if (KEY_IS("random"))             s->randomSetpoint = (uint8_t)atoi(vs);
        else if (KEY_IS("persistentAlarm"))    s->isPersistentAlarm = (strstr(vs, "true") != NULL);
        else if (KEY_IS("autostart"))          s->isProcessAutostart = (strstr(vs, "true") != NULL);
        else if (KEY_IS("drainFillOverlap"))   s->drainFillOverlapSetpoint = (uint8_t)atoi(vs);
        else if (KEY_IS("multiRinseTime"))     s->multiRinseTime = (uint8_t)atoi(vs);
        else if (KEY_IS("tankSize"))           s->tankSize = (uint8_t)atoi(vs);
        else if (KEY_IS("pumpSpeed"))          s->pumpSpeed = (uint8_t)atoi(vs);
        else if (KEY_IS("chemContainerMl"))    s->chemContainerMl = (uint16_t)atoi(vs);
        else if (KEY_IS("wbContainerMl"))      s->wbContainerMl = (uint16_t)atoi(vs);
        else if (KEY_IS("chemistryVolume"))    s->chemistryVolume = (uint8_t)atoi(vs);
        else if (KEY_IS("wifiEnabled"))        s->wifiEnabled = (strstr(vs, "true") != NULL);
        else {
            LV_LOG_WARN("[WS] Unknown setting key: %.*s", klen, ks);
            return;
        }

        #undef KEY_IS

        qSysAction(SAVE_PROCESS_CONFIG);
        ws_broadcast_state();
        return;
    }

    /* ── stop_now ── */
    if (strstr(msg, "\"stop_now\"")) {
        sCheckupData *ck = find_active_checkup();
        if (ck) { ck->stopNow = true; }
        LV_LOG_USER("[WS] stop_now");
        ws_broadcast_state();
        return;
    }

    /* ── stop_after ── */
    if (strstr(msg, "\"stop_after\"")) {
        sCheckupData *ck = find_active_checkup();
        if (ck) { ck->stopAfter = true; }
        LV_LOG_USER("[WS] stop_after");
        ws_broadcast_state();
        return;
    }

    /* ── start_process ── */
    if (strstr(msg, "\"start_process\"")) {
        /* Expected: {"cmd":"start_process","index":0} */
        const char *idx_str = strstr(msg, "\"index\":");
        if (idx_str) {
            int index = atoi(idx_str + 8);
            processNode *pn = find_process_by_index(index);
            if (pn) {
                lv_async_call(ws_async_start_process, pn);
                LV_LOG_USER("[WS] start_process index=%d queued", index);
            } else {
                LV_LOG_WARN("[WS] start_process: index %d not found", index);
            }
        }
        return;
    }

    /* ── wifi_scan ── */
    if (strstr(msg, "\"wifi_scan\"")) {
        struct sWifiPopup *p = &gui.element.wifiPopup;
        wifi_scan_start();
        p->scanCount = wifi_scan_get_results(p->scanResults, MAX_WIFI_SCAN_RESULTS);

        /* Build scan results JSON */
        char scan_buf[512];
        int sn = snprintf(scan_buf, sizeof(scan_buf), "[");
        for (int i = 0; i < p->scanCount && sn < (int)sizeof(scan_buf) - 80; i++) {
            if (i > 0) scan_buf[sn++] = ',';
            sn += snprintf(scan_buf + sn, sizeof(scan_buf) - sn,
                "{\"ssid\":\"%s\",\"rssi\":%d,\"open\":%s}",
                p->scanResults[i].ssid,
                (int)p->scanResults[i].rssi,
                p->scanResults[i].open ? "true" : "false"
            );
        }
        snprintf(scan_buf + sn, sizeof(scan_buf) - sn, "]");
        ws_broadcast_event("wifi_scan_results", scan_buf);
        return;
    }

    LV_LOG_WARN("[WS] Unknown command: %.80s", msg);
}


/* ═══════════════════════════════════════════════════════════════════
 *  SIMULATOR BUILD — stubs
 * ═══════════════════════════════════════════════════════════════════ */
#ifdef SIMULATOR_BUILD

static bool sim_ws_running = false;
static lv_timer_t *sim_broadcast_timer = NULL;

static void sim_broadcast_timer_cb(lv_timer_t *t) {
    (void)t;
    /* In the simulator we just log that a broadcast would happen */
    /* A real simulator could use a local socket here */
}

bool ws_server_start(uint16_t port) {
    if (sim_ws_running) return true;
    sim_ws_running = true;
    LV_LOG_USER("[WS-SIM] Server started on port %u", port);

    /* Periodic broadcast timer (500ms) */
    if (sim_broadcast_timer == NULL) {
        sim_broadcast_timer = lv_timer_create(sim_broadcast_timer_cb, 500, NULL);
    }
    return true;
}

void ws_server_stop(void) {
    if (!sim_ws_running) return;
    sim_ws_running = false;
    safeTimerDelete(&sim_broadcast_timer);
    LV_LOG_USER("[WS-SIM] Server stopped");
}

bool ws_server_is_running(void) {
    return sim_ws_running;
}

int ws_server_client_count(void) {
    return 0; /* simulator has no real clients */
}

void ws_broadcast_state(void) {
    char buf[2048];
    int n = build_state_json(buf, sizeof(buf));
    (void)n;
    LV_LOG_USER("[WS-SIM] Broadcast state (%d bytes)", n);
}

void ws_broadcast_process_list(void) {
    char buf[4096];
    int n = build_process_list_json(buf, sizeof(buf));
    (void)n;
    LV_LOG_USER("[WS-SIM] Broadcast process_list (%d bytes)", n);
}

void ws_broadcast_event(const char *event_name, const char *json_data) {
    LV_LOG_USER("[WS-SIM] Event: %s", event_name);
    (void)json_data;
}


/* ═══════════════════════════════════════════════════════════════════
 *  FIRMWARE BUILD — real ESP-IDF httpd WebSocket server
 * ═══════════════════════════════════════════════════════════════════ */
#else /* !SIMULATOR_BUILD */

#include "esp_http_server.h"
#include "esp_log.h"
#include "mdns.h"

static const char *TAG = "ws_server";

static httpd_handle_t ws_httpd = NULL;
static bool mdns_started = false;

/* ── mDNS: announce as filmachine.local ─────────────────────────── */
static void mdns_service_start(uint16_t port) {
    if (mdns_started) return;
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mDNS init failed: %s", esp_err_to_name(err));
        return;
    }
    mdns_hostname_set("filmachine");
    mdns_instance_name_set("FilMachine Controller");
    mdns_service_add("FilMachine-WS", "_filmachine", "_tcp", port, NULL, 0);
    mdns_started = true;
    ESP_LOGI(TAG, "mDNS started: filmachine.local (port %u)", port);
}

static void mdns_service_stop(void) {
    if (!mdns_started) return;
    mdns_service_remove_all();
    mdns_free();
    mdns_started = false;
    ESP_LOGI(TAG, "mDNS stopped");
}

/* Track connected WebSocket file descriptors (max 4 clients) */
#define WS_MAX_CLIENTS 4
static int ws_fds[WS_MAX_CLIENTS];
static int ws_fd_count = 0;

/* ── Client tracking ── */

static void ws_add_client(int fd) {
    if (ws_fd_count >= WS_MAX_CLIENTS) {
        ESP_LOGW(TAG, "Max clients reached, rejecting fd=%d", fd);
        return;
    }
    ws_fds[ws_fd_count++] = fd;
    ESP_LOGI(TAG, "Client connected (fd=%d, total=%d)", fd, ws_fd_count);
}

static void ws_remove_client(int fd) {
    for (int i = 0; i < ws_fd_count; i++) {
        if (ws_fds[i] == fd) {
            ws_fds[i] = ws_fds[--ws_fd_count];
            ESP_LOGI(TAG, "Client disconnected (fd=%d, total=%d)", fd, ws_fd_count);
            return;
        }
    }
}

/* ── Send to all connected clients ── */

static void ws_send_all(const char *data, int len) {
    if (!ws_httpd || ws_fd_count == 0) return;

    httpd_ws_frame_t pkt = {
        .type    = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)data,
        .len     = len,
        .final   = true,
    };

    for (int i = ws_fd_count - 1; i >= 0; i--) {
        esp_err_t ret = httpd_ws_send_frame_async(ws_httpd, ws_fds[i], &pkt);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Send failed fd=%d, removing", ws_fds[i]);
            ws_remove_client(ws_fds[i]);
        }
    }
}

/* ── WebSocket handler ── */

static esp_err_t ws_handler(httpd_req_t *req) {
    /* New connection — track the file descriptor */
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ws_add_client(fd);
        /* Send initial state */
        ws_broadcast_state();
        ws_broadcast_process_list();
        return ESP_OK;
    }

    /* Receive frame */
    httpd_ws_frame_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.type = HTTPD_WS_TYPE_TEXT;

    /* First call with len=0 to get the frame length */
    esp_err_t ret = httpd_ws_recv_frame(req, &pkt, 0);
    if (ret != ESP_OK) return ret;

    if (pkt.len == 0) return ESP_OK;
    if (pkt.len > 2048) {
        ESP_LOGW(TAG, "Frame too large: %d", (int)pkt.len);
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t *buf = malloc(pkt.len + 1);
    if (!buf) return ESP_ERR_NO_MEM;

    pkt.payload = buf;
    ret = httpd_ws_recv_frame(req, &pkt, pkt.len);
    if (ret == ESP_OK) {
        buf[pkt.len] = '\0';

        if (pkt.type == HTTPD_WS_TYPE_TEXT) {
            ws_handle_command((const char *)buf, pkt.len);
        } else if (pkt.type == HTTPD_WS_TYPE_CLOSE) {
            int fd = httpd_req_to_sockfd(req);
            ws_remove_client(fd);
        }
    }

    free(buf);
    return ret;
}

/* ── Periodic broadcast timer (FreeRTOS) ── */

static lv_timer_t *fw_broadcast_timer = NULL;

static void fw_broadcast_timer_cb(lv_timer_t *t) {
    (void)t;
    if (ws_fd_count > 0) {
        ws_broadcast_state();
    }
}

/* ── Public API ── */

bool ws_server_start(uint16_t port) {
    if (ws_httpd) return true;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.stack_size = 8192;
    config.max_uri_handlers = 2;
    config.max_open_sockets = WS_MAX_CLIENTS + 1;

    if (httpd_start(&ws_httpd, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WS server on port %u", port);
        return false;
    }

    /* Register WebSocket URI */
    const httpd_uri_t ws_uri = {
        .uri          = "/ws",
        .method       = HTTP_GET,
        .handler      = ws_handler,
        .is_websocket = true,
    };
    httpd_register_uri_handler(ws_httpd, &ws_uri);

    ws_fd_count = 0;

    /* Broadcast state every 500ms */
    if (fw_broadcast_timer == NULL) {
        fw_broadcast_timer = lv_timer_create(fw_broadcast_timer_cb, 500, NULL);
    }

    /* Start mDNS so the app can find us as filmachine.local */
    mdns_service_start(port);

    ESP_LOGI(TAG, "WebSocket server started on port %u at /ws", port);
    return true;
}

void ws_server_stop(void) {
    if (!ws_httpd) return;

    mdns_service_stop();
    safeTimerDelete(&fw_broadcast_timer);
    httpd_stop(ws_httpd);
    ws_httpd = NULL;
    ws_fd_count = 0;

    ESP_LOGI(TAG, "WebSocket server stopped");
}

bool ws_server_is_running(void) {
    return ws_httpd != NULL;
}

int ws_server_client_count(void) {
    return ws_fd_count;
}

void ws_broadcast_state(void) {
    char buf[2048];
    int n = build_state_json(buf, sizeof(buf));
    ws_send_all(buf, n);
}

void ws_broadcast_process_list(void) {
    char buf[4096];
    int n = build_process_list_json(buf, sizeof(buf));
    ws_send_all(buf, n);
}

void ws_broadcast_event(const char *event_name, const char *json_data) {
    char buf[1024];
    int n;
    if (json_data && json_data[0] != '\0') {
        n = snprintf(buf, sizeof(buf),
            "{\"type\":\"event\",\"event\":\"%s\",\"data\":%s}",
            event_name, json_data);
    } else {
        n = snprintf(buf, sizeof(buf),
            "{\"type\":\"event\",\"event\":\"%s\"}",
            event_name);
    }
    ws_send_all(buf, n);
}

#endif /* SIMULATOR_BUILD */
