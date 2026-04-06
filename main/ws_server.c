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
#if defined(DISPLAY_DRIVER_ST7701)
#include "st7701_lcd.h"
#endif

extern struct gui_components gui;

/* ── Forward declarations ────────────────────────────────────────── */
extern void checkup(processNode *pn);
static sCheckupData *find_active_checkup(void);
static processNode *find_active_checkup_process(void);
extern void calculateTotalTimeData(processNode *processNode);  /* Thread-safe: no LVGL */
extern void calculateTotalTime(processNode *processNode);      /* LVGL thread only */
extern void *allocateAndInitializeNode(NodeType_t type);
static void ws_queue_lvgl_action(void (*fn)(void *), void *arg);

/* ── JSON helper: extract string value for a key ─────────────────── */
static int ws_json_get_str(const char *msg, const char *key, char *out, int outsize) {
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\":", key);
    const char *p = strstr(msg, needle);
    if (!p) return 0;
    p += strlen(needle);
    while (*p == ' ') p++;
    if (*p != '"') return 0;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < outsize - 1) out[i++] = *p++;
    out[i] = '\0';
    return i;
}

/* ── JSON helper: extract int value for a key ────────────────────── */
static int ws_json_get_int(const char *msg, const char *key, int def) {
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\":", key);
    const char *p = strstr(msg, needle);
    if (!p) return def;
    p += strlen(needle);
    while (*p == ' ') p++;
    return atoi(p);
}

/* ── JSON helper: extract float value for a key ──────────────────── */
static float ws_json_get_float(const char *msg, const char *key, float def) {
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\":", key);
    const char *p = strstr(msg, needle);
    if (!p) return def;
    p += strlen(needle);
    while (*p == ' ') p++;
    return (float)atof(p);
}

/* ── JSON helper: extract bool value for a key ───────────────────── */
static bool ws_json_get_bool(const char *msg, const char *key, bool def) {
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\":", key);
    const char *p = strstr(msg, needle);
    if (!p) return def;
    p += strlen(needle);
    while (*p == ' ') p++;
    if (strncmp(p, "true", 4) == 0) return true;
    if (strncmp(p, "false", 5) == 0) return false;
    return def;
}

/* ── Find step node by index within a process ────────────────────── */
static stepNode *find_step_by_index(processNode *pn, int index) {
    if (!pn || !pn->process.processDetails) return NULL;
    stepNode *node = pn->process.processDetails->stepElementsList.start;
    int i = 0;
    while (node) {
        if (i == index) return node;
        node = node->next;
        i++;
    }
    return NULL;
}

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

/* ── Async callbacks for LVGL-thread UI updates ──────────────────── */
extern uint32_t loadSDCardProcesses(void);
extern void     updateProcessElement(processNode *process);

/* Update a single process element in-place (for edit_process, step changes) */
static void ws_async_update_process(void *user_data) {
    processNode *pn = (processNode *)user_data;
    if (!pn) return;
    if (gui.page.processes.processesListContainer == NULL) return;

    /* Update the process-detail total-time label (safe — we are in LVGL thread) */
    calculateTotalTime(pn);

    if (pn->process.processElement != NULL) {
        /* Element exists → update in-place */
        updateProcessElement(pn);
    }

    /* Live-update the process detail popup if it's open for this process */
    process_detail_live_update(pn);

    LV_LOG_USER("[WS] Process element updated in-place");
}

/* Create a new process element (for create_process) */
static void ws_async_create_process_element(void *user_data) {
    processNode *pn = (processNode *)user_data;
    if (!pn) return;
    if (gui.page.processes.processesListContainer == NULL) return;

    processElementCreate(pn, -1);
    refreshProcessesLabel();
    LV_LOG_USER("[WS] New process element created in UI");
}

/* Refresh settings UI + process cards after a setting change (e.g. tempUnit) */
static void ws_async_refresh_settings(void *user_data) {
    (void)user_data;
    refreshSettingsUI();
    /* Also refresh process cards so temperature labels update */
    if (gui.page.processes.processesListContainer != NULL) {
        uint32_t cnt = lv_obj_get_child_cnt(gui.page.processes.processesListContainer);
        for (uint32_t i = 0; i < cnt; i++) {
            lv_obj_send_event(lv_obj_get_child(gui.page.processes.processesListContainer, i),
                              LV_EVENT_REFRESH, NULL);
        }
    }
    LV_LOG_USER("[WS] Settings UI + process cards refreshed");
}

/* Rebuild the whole process list UI (safe single-shot after batch operations) */
static void ws_async_refresh_process_list(void *user_data) {
    (void)user_data;
    if (gui.page.processes.processesListContainer == NULL) return;
    lv_obj_clean(gui.page.processes.processesListContainer);
    loadSDCardProcesses();
    refreshProcessesLabel();
    LV_LOG_USER("[WS] Process list UI rebuilt");
}

/* Delete process element, free memory, and reposition remaining cards */
static void ws_async_delete_and_refresh(void *user_data) {
    processNode *pn = (processNode *)user_data;
    if (!pn) return;

    /* 1. Delete this process's LVGL element (safe — node memory still alive) */
    if (pn->process.processElement != NULL) {
        lv_obj_delete(pn->process.processElement);
        pn->process.processElement = NULL;
    }

    /* 2. Now safe to free the process memory */
    stepNode *s = pn->process.processDetails->stepElementsList.start;
    while (s) { stepNode *nxt = s->next; free(s->step.stepDetails); free(s); s = nxt; }
    if (pn->process.processDetails->checkup) free(pn->process.processDetails->checkup);
    free(pn->process.processDetails);
    free(pn);

    /* 3. Reposition remaining process cards (clean + rebuild) */
    if (gui.page.processes.processesListContainer != NULL) {
        lv_obj_clean(gui.page.processes.processesListContainer);
        loadSDCardProcesses();
    }
    refreshProcessesLabel();
    LV_LOG_USER("[WS] Process deleted and UI refreshed");
}

/* Free an orphaned stepNode in the LVGL thread after delete_step.
 * The node was already unlinked from the step list on the httpd thread
 * but its LVGL element (with a style reference) may still be alive.
 * We must delete the LVGL element first (which resets the style via
 * LV_EVENT_DELETE), then free the C memory. */
static void ws_async_free_orphan_step(void *user_data) {
    stepNode *sn = (stepNode *)user_data;
    if (!sn) return;
    if (sn->step.stepElement != NULL) {
        lv_obj_delete(sn->step.stepElement);
        sn->step.stepElement = NULL;
    }
    /* Style was reset by LV_EVENT_DELETE (or was never initialised).
     * Now safe to free the node memory. */
    if (sn->step.stepDetails) free(sn->step.stepDetails);
    free(sn);
    LV_LOG_USER("[WS] Orphan step element deleted and memory freed");
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
    pn->process.processDetails->processTotalTimeValue = NULL;
    checkup(pn);
    LV_LOG_USER("[WS] Process started via remote: %s",
                pn->process.processDetails->data.processNameString);
}

/* ── Async callbacks: advance checkup phases on LVGL thread ───── */
static void ws_async_checkup_advance(void *user_data) {
    processNode *pn = find_active_checkup_process();
    (void)user_data;
    if (!pn || !pn->process.processDetails || !pn->process.processDetails->checkup) return;
    sCheckup *ckup = pn->process.processDetails->checkup;

    /* Stop any active alarm before advancing (mirrors event_checkup button handler) */
    alarm_stop();

    switch (ckup->data.processStep) {
        case 0: /* Setup → Fill Water */
            ckup->data.isProcessing = 0;
            ckup->data.processStep = 1;
            ckup->data.stepFillWaterStatus = 1;
            ckup->data.stepReachTempStatus = 0;
            ckup->data.stepCheckFilmStatus = 0;
            LV_LOG_USER("[WS] checkup_advance: 0→1 (Fill Water)");
            break;
        case 1: /* Fill Water → Reach Temp */
            ckup->data.isProcessing = 0;
            ckup->data.processStep = 2;
            ckup->data.stepFillWaterStatus = 2;
            ckup->data.stepReachTempStatus = 1;
            ckup->data.stepCheckFilmStatus = 0;
            LV_LOG_USER("[WS] checkup_advance: 1→2 (Reach Temp)");
            break;
        case 2: /* Reach Temp → Check Film */
            ckup->data.isProcessing = 0;
            ckup->data.processStep = 3;
            ckup->data.stepFillWaterStatus = 2;
            ckup->data.stepReachTempStatus = 2;
            ckup->data.stepCheckFilmStatus = 1;
            /* Stop temp timer before leaving Reach Temp — otherwise it
               accesses widgets that checkup_renderCheckFilm destroys */
            safeTimerDelete(&ckup->tempTimer);
            sim_setHeater(false);
            ckup->data.heaterOn = false;
            LV_LOG_USER("[WS] checkup_advance: 2→3 (Check Film)");
            break;
        case 3: /* Check Film → Processing */
            ckup->data.isProcessing = 1;
            ckup->data.processStep = 4;
            ckup->data.stepFillWaterStatus = 2;
            ckup->data.stepReachTempStatus = 2;
            ckup->data.stepCheckFilmStatus = 2;
            LV_LOG_USER("[WS] checkup_advance: 3→4 (Processing)");
            break;
        default:
            LV_LOG_WARN("[WS] checkup_advance: already at step %u", ckup->data.processStep);
            return;
    }
    checkup(pn);
    ws_broadcast_state();
}

/* ── Async callback: stop_now — mirrors message_popup_handle_stop_now() ─ */
static void ws_async_stop_now(void *user_data) {
    (void)user_data;
    processNode *pn = find_active_checkup_process();
    if (!pn || !pn->process.processDetails || !pn->process.processDetails->checkup) return;
    sCheckup *ckup = pn->process.processDetails->checkup;

    ckup->data.stopNow  = true;
    ckup->data.stopAfter = false;
    ckup->data.isDeveloping = false;

    if (ckup->checkupStopAfterButton) lv_obj_add_state(ckup->checkupStopAfterButton, LV_STATE_DISABLED);
    if (ckup->checkupStopNowButton)   lv_obj_add_state(ckup->checkupStopNowButton, LV_STATE_DISABLED);

    if (ckup->checkupProcessCompleteLabel) {
        lv_label_set_text(ckup->checkupProcessCompleteLabel, "Process stopped");
        lv_obj_remove_flag(ckup->checkupProcessCompleteLabel, LV_OBJ_FLAG_HIDDEN);
    }
    if (ckup->checkupStepNameValue) lv_obj_add_flag(ckup->checkupStepNameValue, LV_OBJ_FLAG_HIDDEN);
    if (ckup->checkupStepTimeLeftValue) lv_obj_add_flag(ckup->checkupStepTimeLeftValue, LV_OBJ_FLAG_HIDDEN);
    if (ckup->checkupProcessTimeLeftValue) lv_obj_add_flag(ckup->checkupProcessTimeLeftValue, LV_OBJ_FLAG_HIDDEN);

    gui.page.tools.machineStats.stopped++;
    safeTimerDelete(&ckup->processTimer);
    if (ckup->pumpTimer) lv_timer_resume(ckup->pumpTimer);
    qSysAction(SAVE_MACHINE_STATS);

    LV_LOG_USER("[WS] stop_now executed on LVGL thread");
    ws_broadcast_state();
}

/* ── Async callback: stop_after — mirrors message_popup_handle_stop_after() ─ */
static void ws_async_stop_after(void *user_data) {
    (void)user_data;
    processNode *pn = find_active_checkup_process();
    if (!pn || !pn->process.processDetails || !pn->process.processDetails->checkup) return;
    sCheckup *ckup = pn->process.processDetails->checkup;

    ckup->data.stopAfter = true;

    if (ckup->checkupStopAfterButton) lv_obj_add_state(ckup->checkupStopAfterButton, LV_STATE_DISABLED);
    if (ckup->checkupStopNowButton)   lv_obj_remove_state(ckup->checkupStopNowButton, LV_STATE_DISABLED);

    gui.page.tools.machineStats.stopped++;
    qSysAction(SAVE_MACHINE_STATS);

    LV_LOG_USER("[WS] stop_after executed on LVGL thread");
    ws_broadcast_state();
}

/* ── Async callback: close_process — mirrors checkupCloseButton handler ── */
static void ws_async_close_process(void *user_data) {
    (void)user_data;
    processNode *pn = find_active_checkup_process();
    if (!pn || !pn->process.processDetails || !pn->process.processDetails->checkup) return;
    sCheckup *ckup = pn->process.processDetails->checkup;

    /* Stop persistent alarm */
    alarm_stop();

    /* Safety: delete any timers that might still be running */
    safeTimerDelete(&ckup->processTimer);
    safeTimerDelete(&ckup->pumpTimer);
    safeTimerDelete(&ckup->tempTimer);
    sim_setHeater(false);

    /* Reset static state for next process run */
    checkup_reset_state();

    /* Clean up the style */
    lv_style_reset(&ckup->textAreaStyleCheckup);

    /* Switch back to the menu screen, then delete checkup screen */
#if defined(DISPLAY_DRIVER_ST7701)
    st7701_lcd_set_dim_inhibit(false);  /* Re-enable auto-dimming */
    st7701_lcd_fill_screen(0x0000);
#endif
    lv_screen_load(gui.page.menu.screen_mainMenu);
    lv_obj_invalidate(gui.page.menu.screen_mainMenu);
    if (ckup->checkupParent != NULL) {
        lv_obj_delete(ckup->checkupParent);
        ckup->checkupParent = NULL;
        ckup->checkupContainer = NULL;
    }

    LV_LOG_USER("[WS] close_process executed on LVGL thread");
    ws_broadcast_state();
}

/* ── Find any active checkup (ANY phase 0-4, not just isProcessing) ── */
static sCheckupData *find_active_checkup(void) {
    processNode *node = gui.page.processes.processElementsList.start;
    while (node) {
        if (node->process.processDetails &&
            node->process.processDetails->checkup &&
            node->process.processDetails->checkup->checkupParent != NULL) {
            return &node->process.processDetails->checkup->data;
        }
        node = node->next;
    }
    return NULL;
}

/* ── Find the processNode with an active checkup ── */
static processNode *find_active_checkup_process(void) {
    processNode *node = gui.page.processes.processElementsList.start;
    while (node) {
        if (node->process.processDetails &&
            node->process.processDetails->checkup &&
            node->process.processDetails->checkup->checkupParent != NULL) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

/* ── JSON-safe string copy: escape control chars, backslash and quotes ── */
static int json_escape(char *dst, int dstsize, const char *src) {
    int w = 0;
    for (int i = 0; src[i] != '\0' && w < dstsize - 6; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c == '"')       { dst[w++] = '\\'; dst[w++] = '"'; }
        else if (c == '\\') { dst[w++] = '\\'; dst[w++] = '\\'; }
        else if (c == '\n') { dst[w++] = '\\'; dst[w++] = 'n'; }
        else if (c == '\r') { dst[w++] = '\\'; dst[w++] = 'r'; }
        else if (c == '\t') { dst[w++] = '\\'; dst[w++] = 't'; }
        else if (c < 0x20)  { w += snprintf(dst + w, dstsize - w, "\\u%04x", c); }
        else                { dst[w++] = c; }
    }
    dst[w] = '\0';
    return w;
}

/* ── Build full state JSON ───────────────────────────────────────── */
static int build_state_json(char *buf, int bufsize) {
    struct machineSettings *s = &gui.page.settings.settingsParams;
    static sCheckupData ck_default;  /* zeroed — used when no process runs */
    sCheckupData *ck = find_active_checkup();
    if (!ck) { memset(&ck_default, 0, sizeof(ck_default)); ck = &ck_default; }
    machineStatistics *st = &gui.page.tools.machineStats;

    /* Active process runtime data — use broader finder that works in ALL phases */
    processNode *active_pn = find_active_checkup_process();
    const char *proc_name = "";
    const char *step_name = "";
    const char *next_step_name = "";
    uint8_t  cur_step_idx = 0;
    uint8_t  total_steps  = 0;
    uint8_t  step_mins = 0, step_secs = 0;
    uint8_t  target_temp = 0;

    if (active_pn && active_pn->process.processDetails) {
        proc_name = active_pn->process.processDetails->data.processNameString;
        target_temp = (uint8_t)active_pn->process.processDetails->data.temp;

        /* Count steps and find current step index */
        sCheckup *ckup = active_pn->process.processDetails->checkup;
        stepNode *sn = active_pn->process.processDetails->stepElementsList.start;
        uint8_t idx = 0;
        while (sn) {
            if (ckup && sn == ckup->currentStep) {
                cur_step_idx = idx;
                step_name = sn->step.stepDetails->data.stepNameString;
                step_mins = sn->step.stepDetails->data.timeMins;
                step_secs = sn->step.stepDetails->data.timeSecs;
                if (sn->next) {
                    next_step_name = sn->next->step.stepDetails->data.stepNameString;
                }
            }
            idx++;
            sn = sn->next;
        }
        total_steps = idx;
    }

    /* Escape all user-facing strings for JSON safety */
    char esc_proc[MAX_PROC_NAME_LEN * 6 + 1];
    char esc_step[MAX_PROC_NAME_LEN * 6 + 1];
    char esc_next[MAX_PROC_NAME_LEN * 6 + 1];
    char esc_ssid[sizeof(s->wifiSSID) * 6 + 1];
    json_escape(esc_proc, sizeof(esc_proc), proc_name);
    json_escape(esc_step, sizeof(esc_step), step_name);
    json_escape(esc_next, sizeof(esc_next), next_step_name);
    json_escape(esc_ssid, sizeof(esc_ssid), s->wifiSSID);

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
        "\"checkupActive\":%s,"
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
        /* Process runtime detail */
        "\"processName\":\"%s\","
        "\"currentStepName\":\"%s\","
        "\"nextStepName\":\"%s\","
        "\"currentStepIndex\":%u,"
        "\"totalSteps\":%u,"
        "\"stepDurationMins\":%u,"
        "\"stepDurationSecs\":%u,"
        "\"targetTemp\":%u,"
        "\"stepPct\":%u,"
        "\"processPct\":%u,"
        "\"tankPct\":%u,"
        "\"stepElapsedMins\":%lu,"
        "\"stepElapsedSecs\":%u,"
        "\"stepLeftMins\":%lu,"
        "\"stepLeftSecs\":%u,"
        "\"processLeftMins\":%lu,"
        "\"processLeftSecs\":%u,"
        "\"processElapsedMins\":%lu,"
        "\"processElapsedSecs\":%u,"
        /* Statistics */
        "\"statsCompleted\":%lu,"
        "\"statsTotalMins\":%llu,"
        "\"statsTotalSecs\":%lu,"
        "\"statsStopped\":%lu,"
        "\"statsClean\":%lu,"
        /* Alarm */
        "\"alarmActive\":%s,"
        /* Device info */
        "\"fwVersion\":\"%s\","
        "\"serialNumber\":\"%s\""
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
        esc_ssid,
        wifi_is_connected() ? "true" : "false",
        (find_active_checkup_process() != NULL) ? "true" : "false",
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
        /* Process runtime detail — escaped for JSON safety */
        esc_proc,
        esc_step,
        esc_next,
        (unsigned)cur_step_idx,
        (unsigned)total_steps,
        (unsigned)step_mins,
        (unsigned)step_secs,
        (unsigned)target_temp,
        (unsigned)checkup_get_step_percentage(),
        (unsigned)checkup_get_process_percentage(),
        (unsigned)checkup_get_tank_percentage(),
        (unsigned long)checkup_get_step_elapsed_mins(),
        (unsigned)checkup_get_step_elapsed_secs(),
        (unsigned long)checkup_get_step_left_mins(),
        (unsigned)checkup_get_step_left_secs(),
        (unsigned long)checkup_get_process_left_mins(),
        (unsigned)checkup_get_process_left_secs(),
        (unsigned long)checkup_get_process_elapsed_mins(),
        (unsigned)checkup_get_process_elapsed_secs(),
        /* Statistics */
        (unsigned long)st->completed,
        (unsigned long long)st->totalMins,
        (unsigned long)st->totalSecs,
        (unsigned long)st->stopped,
        (unsigned long)st->clean,
        alarm_is_active() ? "true" : "false",
        softwareVersionValue_text,
        softwareSerialNumValue_text
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
        while (snode != NULL && sn < (int)sizeof(steps_buf) - 200) {
            sStepData *sd = &snode->step.stepDetails->data;
            char esc_step_name[MAX_PROC_NAME_LEN * 6 + 1];
            json_escape(esc_step_name, sizeof(esc_step_name), sd->stepNameString);
            if (si > 0) steps_buf[sn++] = ',';
            sn += snprintf(steps_buf + sn, sizeof(steps_buf) - sn,
                "{\"name\":\"%s\",\"mins\":%u,\"secs\":%u,\"type\":%d,\"source\":%u,\"discard\":%s}",
                esc_step_name,
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

        char esc_proc_name[MAX_PROC_NAME_LEN * 6 + 1];
        json_escape(esc_proc_name, sizeof(esc_proc_name), pd->processNameString);

        if (idx > 0) buf[n++] = ',';
        n += snprintf(buf + n, bufsize - n,
            "{\"index\":%d,\"name\":\"%s\",\"temp\":%u,\"tolerance\":%.1f,"
            "\"tempControlled\":%s,\"preferred\":%s,\"filmType\":%d,"
            "\"mins\":%u,\"secs\":%u,\"steps\":%s}",
            idx,
            esc_proc_name,
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
        /* Rebuild LVGL process cards — this is called by the app after
         * a batch of add/delete/reorder step operations to do a single
         * safe UI refresh instead of one per operation. */
        ws_queue_lvgl_action(ws_async_refresh_process_list, NULL);
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
        ws_queue_lvgl_action(ws_async_refresh_settings, NULL);
        return;
    }

    /* ── reset_defaults: restore factory settings ── */
    if (strstr(msg, "\"reset_defaults\"")) {
        struct machineSettings *s = &gui.page.settings.settingsParams;
        s->tempUnit = 0;
        s->waterInlet = false;
        s->calibratedTemp = 20;
        s->tempCalibOffset = 0;
        s->filmRotationSpeedSetpoint = 50;
        s->rotationIntervalSetpoint = 10;
        s->randomSetpoint = 0;
        s->isPersistentAlarm = false;
        s->isProcessAutostart = false;
        s->drainFillOverlapSetpoint = 100;
        s->multiRinseTime = 60;
        s->tankSize = 2;
        s->pumpSpeed = 30;
        s->chemContainerMl = 500;
        s->wbContainerMl = 2000;
        s->chemistryVolume = 2;
        qSysAction(SAVE_PROCESS_CONFIG);
        ws_broadcast_state();
        ws_queue_lvgl_action(ws_async_refresh_settings, NULL);
        LV_LOG_USER("[WS] reset_defaults applied");
        return;
    }

    /* ── checkup_advance: advance to next checkup phase ── */
    if (strstr(msg, "\"checkup_advance\"")) {
        ws_queue_lvgl_action(ws_async_checkup_advance, NULL);
        LV_LOG_USER("[WS] checkup_advance queued");
        return;
    }

    /* ── close_process ── */
    if (strstr(msg, "\"close_process\"")) {
        ws_queue_lvgl_action(ws_async_close_process, NULL);
        LV_LOG_USER("[WS] close_process queued");
        return;
    }

    /* ── stop_now ── */
    if (strstr(msg, "\"stop_now\"")) {
        ws_queue_lvgl_action(ws_async_stop_now, NULL);
        LV_LOG_USER("[WS] stop_now queued");
        return;
    }

    /* ── stop_after ── */
    if (strstr(msg, "\"stop_after\"")) {
        ws_queue_lvgl_action(ws_async_stop_after, NULL);
        LV_LOG_USER("[WS] stop_after queued");
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
                ws_queue_lvgl_action(ws_async_start_process, pn);
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

    /* ── create_process ── */
    if (strstr(msg, "\"create_process\"")) {
        processNode *pn = (processNode *)allocateAndInitializeNode(PROCESS_NODE);
        if (!pn) { LV_LOG_ERROR("[WS] create_process: alloc failed"); return; }
        sProcessData *pd = &pn->process.processDetails->data;
        ws_json_get_str(msg, "name", pd->processNameString, sizeof(pd->processNameString));
        if (pd->processNameString[0] == '\0') snprintf(pd->processNameString, sizeof(pd->processNameString), "New Process");
        pd->temp             = (uint32_t)ws_json_get_int(msg, "temp", 20);
        pd->tempTolerance    = ws_json_get_float(msg, "tolerance", 0.5f);
        pd->isTempControlled = ws_json_get_bool(msg, "tempControlled", true);
        pd->isPreferred      = ws_json_get_bool(msg, "preferred", false);
        pd->filmType         = (filmType_t)ws_json_get_int(msg, "filmType", 0);
        /* Add to list (without creating UI elements — headless) */
        processList *list = &gui.page.processes.processElementsList;
        pn->prev = NULL; pn->next = NULL;
        if (list->start == NULL) { list->start = pn; }
        else { list->end->next = pn; pn->prev = list->end; }
        list->end = pn;
        list->size++;
        calculateTotalTimeData(pn);
        qSysAction(SAVE_PROCESS_CONFIG);
        ws_broadcast_process_list();
        ws_queue_lvgl_action(ws_async_create_process_element, pn);
        LV_LOG_USER("[WS] create_process: '%s'", pd->processNameString);
        return;
    }

    /* ── edit_process ── */
    if (strstr(msg, "\"edit_process\"")) {
        int index = ws_json_get_int(msg, "index", -1);
        processNode *pn = find_process_by_index(index);
        if (!pn) { LV_LOG_WARN("[WS] edit_process: index %d not found", index); return; }
        sProcessData *pd = &pn->process.processDetails->data;
        char name[MAX_PROC_NAME_LEN + 1];
        if (ws_json_get_str(msg, "name", name, sizeof(name)) > 0)
            snprintf(pd->processNameString, sizeof(pd->processNameString), "%s", name);
        if (strstr(msg, "\"temp\":"))          pd->temp = (uint32_t)ws_json_get_int(msg, "temp", (int)pd->temp);
        if (strstr(msg, "\"tolerance\":"))     pd->tempTolerance = ws_json_get_float(msg, "tolerance", pd->tempTolerance);
        if (strstr(msg, "\"tempControlled\":"))pd->isTempControlled = ws_json_get_bool(msg, "tempControlled", pd->isTempControlled);
        if (strstr(msg, "\"preferred\":"))     pd->isPreferred = ws_json_get_bool(msg, "preferred", pd->isPreferred);
        if (strstr(msg, "\"filmType\":"))      pd->filmType = (filmType_t)ws_json_get_int(msg, "filmType", (int)pd->filmType);
        calculateTotalTimeData(pn);
        qSysAction(SAVE_PROCESS_CONFIG);
        ws_broadcast_process_list();
        ws_queue_lvgl_action(ws_async_update_process, pn);
        LV_LOG_USER("[WS] edit_process index=%d", index);
        return;
    }

    /* ── delete_process ── */
    if (strstr(msg, "\"delete_process\"")) {
        int index = ws_json_get_int(msg, "index", -1);
        processNode *pn = find_process_by_index(index);
        if (!pn) { LV_LOG_WARN("[WS] delete_process: index %d not found", index); return; }
        /* Remove from linked list manually (no UI to clean) */
        processList *list = &gui.page.processes.processElementsList;
        if (pn->prev) pn->prev->next = pn->next; else list->start = pn->next;
        if (pn->next) pn->next->prev = pn->prev; else list->end = pn->prev;
        list->size--;
        /* Don't free here — defer to LVGL thread so UI element can be
           safely deleted before the node memory is released */
        qSysAction(SAVE_PROCESS_CONFIG);
        ws_broadcast_process_list();
        ws_queue_lvgl_action(ws_async_delete_and_refresh, pn);
        LV_LOG_USER("[WS] delete_process index=%d (deferred free)", index);
        return;
    }

    /* ── add_step ── */
    if (strstr(msg, "\"add_step\"")) {
        int pi = ws_json_get_int(msg, "processIndex", -1);
        processNode *pn = find_process_by_index(pi);
        if (!pn) { LV_LOG_WARN("[WS] add_step: process %d not found", pi); return; }
        stepNode *sn = (stepNode *)allocateAndInitializeNode(STEP_NODE);
        if (!sn) { LV_LOG_ERROR("[WS] add_step: alloc failed"); return; }
        sStepData *sd = &sn->step.stepDetails->data;
        ws_json_get_str(msg, "name", sd->stepNameString, sizeof(sd->stepNameString));
        if (sd->stepNameString[0] == '\0') snprintf(sd->stepNameString, sizeof(sd->stepNameString), "New Step");
        sd->timeMins       = (uint8_t)ws_json_get_int(msg, "mins", 0);
        sd->timeSecs       = (uint8_t)ws_json_get_int(msg, "secs", 0);
        sd->type           = (chemicalType_t)ws_json_get_int(msg, "type", 0);
        sd->source         = (uint8_t)ws_json_get_int(msg, "source", 0);
        sd->discardAfterProc = (uint8_t)(ws_json_get_bool(msg, "discard", false) ? 1 : 0);
        /* Add to step list */
        stepList *sl = &pn->process.processDetails->stepElementsList;
        sn->prev = NULL; sn->next = NULL;
        if (sl->start == NULL) { sl->start = sn; }
        else { sl->end->next = sn; sn->prev = sl->end; }
        sl->end = sn;
        sl->size++;
        calculateTotalTimeData(pn);
        qSysAction(SAVE_PROCESS_CONFIG);
        ws_broadcast_process_list();
        ws_queue_lvgl_action(ws_async_update_process, pn);
        LV_LOG_USER("[WS] add_step to process %d: '%s'", pi, sd->stepNameString);
        return;
    }

    /* ── edit_step ── */
    if (strstr(msg, "\"edit_step\"")) {
        int pi = ws_json_get_int(msg, "processIndex", -1);
        int si = ws_json_get_int(msg, "stepIndex", -1);
        processNode *pn = find_process_by_index(pi);
        if (!pn) { LV_LOG_WARN("[WS] edit_step: process %d not found", pi); return; }
        stepNode *sn = find_step_by_index(pn, si);
        if (!sn) { LV_LOG_WARN("[WS] edit_step: step %d not found", si); return; }
        sStepData *sd = &sn->step.stepDetails->data;
        char name[MAX_PROC_NAME_LEN + 1];
        if (ws_json_get_str(msg, "name", name, sizeof(name)) > 0)
            snprintf(sd->stepNameString, sizeof(sd->stepNameString), "%s", name);
        if (strstr(msg, "\"mins\":"))    sd->timeMins = (uint8_t)ws_json_get_int(msg, "mins", sd->timeMins);
        if (strstr(msg, "\"secs\":"))    sd->timeSecs = (uint8_t)ws_json_get_int(msg, "secs", sd->timeSecs);
        if (strstr(msg, "\"type\":"))    sd->type = (chemicalType_t)ws_json_get_int(msg, "type", (int)sd->type);
        if (strstr(msg, "\"source\":"))  sd->source = (uint8_t)ws_json_get_int(msg, "source", sd->source);
        if (strstr(msg, "\"discard\":")) sd->discardAfterProc = (uint8_t)(ws_json_get_bool(msg, "discard", sd->discardAfterProc) ? 1 : 0);
        LV_LOG_USER("[WS] edit_step: name='%s' mins=%d secs=%d type=%d src=%d discard=%d somethingChanged=%d",
            sd->stepNameString, sd->timeMins, sd->timeSecs, sd->type, sd->source,
            sd->discardAfterProc, sd->somethingChanged);
        calculateTotalTimeData(pn);
        qSysAction(SAVE_PROCESS_CONFIG);
        ws_broadcast_process_list();
        ws_queue_lvgl_action(ws_async_update_process, pn);
        LV_LOG_USER("[WS] edit_step process=%d step=%d — queued LVGL update", pi, si);
        return;
    }

    /* ── delete_step ── */
    if (strstr(msg, "\"delete_step\"")) {
        int pi = ws_json_get_int(msg, "processIndex", -1);
        int si = ws_json_get_int(msg, "stepIndex", -1);
        processNode *pn = find_process_by_index(pi);
        if (!pn) { LV_LOG_WARN("[WS] delete_step: process %d not found", pi); return; }
        stepNode *sn = find_step_by_index(pn, si);
        if (!sn) { LV_LOG_WARN("[WS] delete_step: step %d not found", si); return; }
        stepList *sl = &pn->process.processDetails->stepElementsList;
        if (sn->prev) sn->prev->next = sn->next; else sl->start = sn->next;
        if (sn->next) sn->next->prev = sn->prev; else sl->end = sn->prev;
        sl->size--;
        calculateTotalTimeData(pn);
        qSysAction(SAVE_PROCESS_CONFIG);
        ws_broadcast_process_list();
        ws_queue_lvgl_action(ws_async_free_orphan_step, sn);
        ws_queue_lvgl_action(ws_async_update_process, pn);
        LV_LOG_USER("[WS] delete_step process=%d step=%d", pi, si);
        return;
    }

    /* ── reorder_step ── */
    if (strstr(msg, "\"reorder_step\"")) {
        int pi = ws_json_get_int(msg, "processIndex", -1);
        int from = ws_json_get_int(msg, "from", -1);
        int to = ws_json_get_int(msg, "to", -1);
        processNode *pn = find_process_by_index(pi);
        if (!pn || from == to) return;
        stepNode *sn = find_step_by_index(pn, from);
        if (!sn) return;
        stepList *sl = &pn->process.processDetails->stepElementsList;
        /* Remove from current position */
        if (sn->prev) sn->prev->next = sn->next; else sl->start = sn->next;
        if (sn->next) sn->next->prev = sn->prev; else sl->end = sn->prev;
        /* Insert at new position */
        if (to == 0) {
            sn->prev = NULL; sn->next = sl->start;
            if (sl->start) sl->start->prev = sn;
            sl->start = sn;
            if (!sl->end) sl->end = sn;
        } else {
            stepNode *target = find_step_by_index(pn, (from < to) ? to : to - 1);
            if (!target) target = sl->end;
            sn->next = target->next;
            sn->prev = target;
            if (target->next) target->next->prev = sn; else sl->end = sn;
            target->next = sn;
        }
        qSysAction(SAVE_PROCESS_CONFIG);
        ws_broadcast_process_list();
        ws_queue_lvgl_action(ws_async_update_process, pn);
        LV_LOG_USER("[WS] reorder_step process=%d from=%d to=%d", pi, from, to);
        return;
    }

    LV_LOG_WARN("[WS] Unknown command: %.80s", msg);
}


/* ═══════════════════════════════════════════════════════════════════
 *  SIMULATOR BUILD — real WebSocket server using POSIX sockets
 * ═══════════════════════════════════════════════════════════════════ */
#ifdef SIMULATOR_BUILD

/* Simulator: lv_async_call is safe (single-threaded SDL event loop) */
static void ws_queue_lvgl_action(void (*fn)(void *), void *arg) {
    lv_async_call(fn, arg);
}

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

/* macOS does not have MSG_NOSIGNAL; use SO_NOSIGPIPE on the socket instead */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* ── Minimal SHA-1 for WebSocket handshake ───────────────────────── */
typedef struct {
    uint32_t state[5];
    uint64_t count;
    uint8_t  buffer[64];
} SHA1_CTX;

static void sha1_transform(uint32_t state[5], const uint8_t buf[64]) {
    uint32_t a, b, c, d, e, w[80];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)buf[i*4]<<24)|((uint32_t)buf[i*4+1]<<16)|
               ((uint32_t)buf[i*4+2]<<8)|(uint32_t)buf[i*4+3];
    for (int i = 16; i < 80; i++) {
        uint32_t t = w[i-3]^w[i-8]^w[i-14]^w[i-16];
        w[i] = (t<<1)|(t>>31);
    }
    a=state[0]; b=state[1]; c=state[2]; d=state[3]; e=state[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if      (i < 20) { f=(b&c)|((~b)&d);       k=0x5A827999; }
        else if (i < 40) { f=b^c^d;                 k=0x6ED9EBA1; }
        else if (i < 60) { f=(b&c)|(b&d)|(c&d);     k=0x8F1BBCDC; }
        else              { f=b^c^d;                 k=0xCA62C1D6; }
        uint32_t t = ((a<<5)|(a>>27)) + f + e + k + w[i];
        e=d; d=c; c=(b<<30)|(b>>2); b=a; a=t;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d; state[4]+=e;
}

static void sha1_init(SHA1_CTX *ctx) {
    ctx->state[0]=0x67452301; ctx->state[1]=0xEFCDAB89;
    ctx->state[2]=0x98BADCFE; ctx->state[3]=0x10325476;
    ctx->state[4]=0xC3D2E1F0; ctx->count=0;
}

static void sha1_update(SHA1_CTX *ctx, const uint8_t *data, size_t len) {
    size_t i = 0, idx = (size_t)(ctx->count & 63);
    ctx->count += len;
    for (; i < len; i++) {
        ctx->buffer[idx++] = data[i];
        if (idx == 64) { sha1_transform(ctx->state, ctx->buffer); idx = 0; }
    }
}

static void sha1_final(SHA1_CTX *ctx, uint8_t digest[20]) {
    uint8_t pad[64]; memset(pad, 0, 64);
    size_t idx = (size_t)(ctx->count & 63);
    ctx->buffer[idx++] = 0x80;
    if (idx > 56) {
        while (idx < 64) ctx->buffer[idx++] = 0;
        sha1_transform(ctx->state, ctx->buffer);
        memset(ctx->buffer, 0, 56);
    } else {
        while (idx < 56) ctx->buffer[idx++] = 0;
    }
    uint64_t bits = ctx->count * 8;
    for (int i = 0; i < 8; i++) ctx->buffer[56+i] = (uint8_t)(bits >> (56-i*8));
    sha1_transform(ctx->state, ctx->buffer);
    for (int i = 0; i < 5; i++) {
        digest[i*4]   = (uint8_t)(ctx->state[i]>>24);
        digest[i*4+1] = (uint8_t)(ctx->state[i]>>16);
        digest[i*4+2] = (uint8_t)(ctx->state[i]>>8);
        digest[i*4+3] = (uint8_t)(ctx->state[i]);
    }
}

/* ── Base64 encoding ─────────────────────────────────────────────── */
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_encode(const uint8_t *in, int inlen, char *out, int outsize) {
    int o = 0;
    for (int i = 0; i < inlen && o + 4 < outsize; i += 3) {
        uint32_t v = (uint32_t)in[i] << 16;
        if (i+1 < inlen) v |= (uint32_t)in[i+1] << 8;
        if (i+2 < inlen) v |= (uint32_t)in[i+2];
        out[o++] = b64_table[(v>>18)&63];
        out[o++] = b64_table[(v>>12)&63];
        out[o++] = (i+1 < inlen) ? b64_table[(v>>6)&63] : '=';
        out[o++] = (i+2 < inlen) ? b64_table[v&63] : '=';
    }
    out[o] = '\0';
    return o;
}

/* ── WebSocket constants & client management ─────────────────────── */
#define SIM_WS_MAX_CLIENTS 4

static int sim_server_fd = -1;
static bool sim_ws_running = false;
static pthread_t sim_accept_thread;
static lv_timer_t *sim_broadcast_timer = NULL;

static int sim_client_fds[SIM_WS_MAX_CLIENTS];
static int sim_client_count = 0;
static pthread_mutex_t sim_clients_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t sim_client_threads[SIM_WS_MAX_CLIENTS];

static void sim_add_client(int fd) {
    pthread_mutex_lock(&sim_clients_mutex);
    if (sim_client_count < SIM_WS_MAX_CLIENTS) {
        sim_client_fds[sim_client_count++] = fd;
        LV_LOG_USER("[WS-SIM] Client connected (fd=%d, total=%d)", fd, sim_client_count);
    } else {
        LV_LOG_WARN("[WS-SIM] Max clients reached, rejecting fd=%d", fd);
        close(fd);
    }
    pthread_mutex_unlock(&sim_clients_mutex);
}

static void sim_remove_client(int fd) {
    pthread_mutex_lock(&sim_clients_mutex);
    for (int i = 0; i < sim_client_count; i++) {
        if (sim_client_fds[i] == fd) {
            sim_client_fds[i] = sim_client_fds[--sim_client_count];
            LV_LOG_USER("[WS-SIM] Client disconnected (fd=%d, total=%d)", fd, sim_client_count);
            break;
        }
    }
    pthread_mutex_unlock(&sim_clients_mutex);
    close(fd);
}

/* ── Send a WebSocket text frame (unmasked, from server) ─────────── */
static int ws_send_frame(int fd, const char *data, int len) {
    uint8_t header[10];
    int hlen = 0;
    header[0] = 0x81;  /* FIN + TEXT opcode */
    if (len < 126) {
        header[1] = (uint8_t)len;
        hlen = 2;
    } else if (len < 65536) {
        header[1] = 126;
        header[2] = (uint8_t)(len >> 8);
        header[3] = (uint8_t)(len & 0xFF);
        hlen = 4;
    } else {
        header[1] = 127;
        memset(header+2, 0, 4);
        header[6] = (uint8_t)((len >> 24) & 0xFF);
        header[7] = (uint8_t)((len >> 16) & 0xFF);
        header[8] = (uint8_t)((len >> 8) & 0xFF);
        header[9] = (uint8_t)(len & 0xFF);
        hlen = 10;
    }
    if (send(fd, header, hlen, MSG_NOSIGNAL) != hlen) return -1;
    if (send(fd, data, len, MSG_NOSIGNAL) != len) return -1;
    return 0;
}

/* ── Send to all connected clients ───────────────────────────────── */
static void sim_send_all(const char *data, int len) {
    pthread_mutex_lock(&sim_clients_mutex);
    for (int i = sim_client_count - 1; i >= 0; i--) {
        if (ws_send_frame(sim_client_fds[i], data, len) < 0) {
            int fd = sim_client_fds[i];
            sim_client_fds[i] = sim_client_fds[--sim_client_count];
            close(fd);
            LV_LOG_WARN("[WS-SIM] Send failed fd=%d, removed", fd);
        }
    }
    pthread_mutex_unlock(&sim_clients_mutex);
}

/* ── WebSocket handshake ─────────────────────────────────────────── */
static const char *WS_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static bool ws_do_handshake(int fd) {
    char buf[2048];
    int total = 0;
    /* Read HTTP request (wait up to 3 seconds) */
    struct timeval tv = { .tv_sec = 3, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (total < (int)sizeof(buf) - 1) {
        int n = (int)recv(fd, buf + total, sizeof(buf) - 1 - total, 0);
        if (n <= 0) return false;
        total += n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) break;
    }

    /* Verify it's a WebSocket upgrade request */
    if (!strstr(buf, "Upgrade: websocket") && !strstr(buf, "Upgrade: Websocket")
        && !strstr(buf, "upgrade: websocket")) return false;

    /* Extract Sec-WebSocket-Key */
    const char *key_hdr = strstr(buf, "Sec-WebSocket-Key:");
    if (!key_hdr) key_hdr = strstr(buf, "sec-websocket-key:");
    if (!key_hdr) return false;
    key_hdr += 18;
    while (*key_hdr == ' ') key_hdr++;
    const char *key_end = strstr(key_hdr, "\r\n");
    if (!key_end) return false;
    int keylen = (int)(key_end - key_hdr);

    /* Compute accept: SHA1(key + magic) → base64 */
    char concat[128];
    snprintf(concat, sizeof(concat), "%.*s%s", keylen, key_hdr, WS_MAGIC);
    SHA1_CTX sha;
    uint8_t digest[20];
    sha1_init(&sha);
    sha1_update(&sha, (const uint8_t *)concat, strlen(concat));
    sha1_final(&sha, digest);
    char accept[64];
    base64_encode(digest, 20, accept, sizeof(accept));

    /* Send HTTP 101 response */
    char resp[512];
    int rn = snprintf(resp, sizeof(resp),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n", accept);
    send(fd, resp, rn, 0);

    /* Reset timeout for normal operation */
    tv.tv_sec = 0; tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return true;
}

/* ── Read a WebSocket frame (client sends masked) ────────────────── */
static int ws_read_frame(int fd, char *out, int outsize) {
    uint8_t h[2];
    if (recv(fd, h, 2, MSG_WAITALL) != 2) return -1;

    int opcode = h[0] & 0x0F;
    bool masked = (h[1] & 0x80) != 0;
    uint64_t payload_len = h[1] & 0x7F;

    if (payload_len == 126) {
        uint8_t ext[2];
        if (recv(fd, ext, 2, MSG_WAITALL) != 2) return -1;
        payload_len = ((uint64_t)ext[0]<<8) | ext[1];
    } else if (payload_len == 127) {
        uint8_t ext[8];
        if (recv(fd, ext, 8, MSG_WAITALL) != 8) return -1;
        payload_len = 0;
        for (int i = 0; i < 8; i++) payload_len = (payload_len<<8)|ext[i];
    }

    uint8_t mask[4] = {0};
    if (masked) {
        if (recv(fd, mask, 4, MSG_WAITALL) != 4) return -1;
    }

    if (payload_len > (uint64_t)(outsize - 1)) return -1;

    int plen = (int)payload_len;
    if (plen > 0 && recv(fd, out, plen, MSG_WAITALL) != plen) return -1;

    /* Unmask */
    if (masked) {
        for (int i = 0; i < plen; i++) out[i] ^= mask[i & 3];
    }
    out[plen] = '\0';

    /* Handle opcodes */
    if (opcode == 0x8) return -1;   /* close */
    if (opcode == 0x9) {            /* ping → pong */
        uint8_t pong[2] = { 0x8A, 0x00 };
        send(fd, pong, 2, 0);
        return 0;
    }
    if (opcode == 0x1 || opcode == 0x2) return plen;  /* text or binary */
    return 0;  /* continuation or pong — ignore */
}

/* ── Per-client thread ───────────────────────────────────────────── */
static void *sim_client_thread(void *arg) {
    int fd = *(int *)arg;
    free(arg);

    /* Perform WebSocket handshake */
    if (!ws_do_handshake(fd)) {
        LV_LOG_WARN("[WS-SIM] Handshake failed for fd=%d", fd);
        close(fd);
        return NULL;
    }

    sim_add_client(fd);

    /* Send initial state + process list */
    {
        char buf[8192];
        int n = build_state_json(buf, sizeof(buf));
        ws_send_frame(fd, buf, n);
    }
    {
        char buf[8192];
        int n = build_process_list_json(buf, sizeof(buf));
        ws_send_frame(fd, buf, n);
    }

    /* Read loop */
    char msg[2048];
    while (sim_ws_running) {
        int n = ws_read_frame(fd, msg, sizeof(msg));
        if (n < 0) break;     /* error or close */
        if (n == 0) continue;  /* ping/pong or empty */
        ws_handle_command(msg, n);
    }

    sim_remove_client(fd);
    return NULL;
}

/* ── Accept thread ───────────────────────────────────────────────── */
static void *sim_accept_thread_fn(void *arg) {
    (void)arg;
    while (sim_ws_running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(sim_server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            if (sim_ws_running) usleep(10000);
            continue;
        }

        /* Enable SO_NOSIGPIPE on macOS */
        #ifdef SO_NOSIGPIPE
        int one = 1;
        setsockopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
        #endif

        int *fd_ptr = malloc(sizeof(int));
        *fd_ptr = client_fd;
        pthread_t t;
        if (pthread_create(&t, NULL, sim_client_thread, fd_ptr) != 0) {
            close(client_fd);
            free(fd_ptr);
        } else {
            pthread_detach(t);
        }
    }
    return NULL;
}

/* ── Periodic broadcast timer ────────────────────────────────────── */
static void sim_broadcast_timer_cb(lv_timer_t *t) {
    (void)t;
    if (sim_client_count > 0) {
        ws_broadcast_state();
    }
}

/* ── Public API ──────────────────────────────────────────────────── */

bool ws_server_start(uint16_t port) {
    if (sim_ws_running) return true;

    /* Create server socket */
    sim_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sim_server_fd < 0) {
        LV_LOG_ERROR("[WS-SIM] socket() failed: %s", strerror(errno));
        return false;
    }

    int opt = 1;
    setsockopt(sim_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #ifdef SO_REUSEPORT
    setsockopt(sim_server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    #endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sim_server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LV_LOG_ERROR("[WS-SIM] bind() failed on port %u: %s", port, strerror(errno));
        close(sim_server_fd);
        sim_server_fd = -1;
        return false;
    }

    if (listen(sim_server_fd, 4) < 0) {
        LV_LOG_ERROR("[WS-SIM] listen() failed: %s", strerror(errno));
        close(sim_server_fd);
        sim_server_fd = -1;
        return false;
    }

    sim_ws_running = true;
    sim_client_count = 0;

    /* Start accept thread */
    pthread_create(&sim_accept_thread, NULL, sim_accept_thread_fn, NULL);

    /* Periodic broadcast timer (500ms) */
    if (sim_broadcast_timer == NULL) {
        sim_broadcast_timer = lv_timer_create(sim_broadcast_timer_cb, 500, NULL);
    }

    LV_LOG_USER("[WS-SIM] WebSocket server started on port %u at /ws", port);
    return true;
}

void ws_server_stop(void) {
    if (!sim_ws_running) return;
    sim_ws_running = false;

    /* Close server socket to unblock accept() */
    if (sim_server_fd >= 0) {
        close(sim_server_fd);
        sim_server_fd = -1;
    }

    pthread_join(sim_accept_thread, NULL);

    /* Close all client connections */
    pthread_mutex_lock(&sim_clients_mutex);
    for (int i = 0; i < sim_client_count; i++) {
        close(sim_client_fds[i]);
    }
    sim_client_count = 0;
    pthread_mutex_unlock(&sim_clients_mutex);

    safeTimerDelete(&sim_broadcast_timer);
    LV_LOG_USER("[WS-SIM] Server stopped");
}

bool ws_server_is_running(void) {
    return sim_ws_running;
}

int ws_server_client_count(void) {
    return sim_client_count;
}

void ws_broadcast_state(void) {
    char buf[8192];
    int n = build_state_json(buf, sizeof(buf));
    sim_send_all(buf, n);
}

void ws_broadcast_process_list(void) {
    char buf[8192];
    int n = build_process_list_json(buf, sizeof(buf));
    sim_send_all(buf, n);
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
    sim_send_all(buf, n);
}


/* ═══════════════════════════════════════════════════════════════════
 *  FIRMWARE BUILD — real ESP-IDF httpd WebSocket server
 * ═══════════════════════════════════════════════════════════════════ */
#else /* !SIMULATOR_BUILD */

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED || defined(CONFIG_ESP_WIFI_REMOTE_ENABLED)

#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "mdns.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

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

/* ── Thread-safe action queue for LVGL calls from httpd task ── */
/* lv_async_call is NOT thread-safe in LVGL 9 when called from a non-LVGL task.
 * Instead, we queue actions here and drain them from the broadcast timer
 * which runs safely in the LVGL task context. */

typedef struct {
    void (*fn)(void *);
    void *arg;
} ws_pending_action_t;

#define WS_ACTION_QUEUE_SIZE 16
static QueueHandle_t ws_action_queue = NULL;

/* Queue an action to be executed on the LVGL thread (safe from any task) */
static void ws_queue_lvgl_action(void (*fn)(void *), void *arg) {
    if (ws_action_queue == NULL) return;
    ws_pending_action_t action = { .fn = fn, .arg = arg };
    if (xQueueSend(ws_action_queue, &action, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "LVGL action queue full — dropped action");
    }
}

/* ── Periodic broadcast timer (runs in LVGL task context) ── */

static lv_timer_t *fw_broadcast_timer = NULL;

static void fw_broadcast_timer_cb(lv_timer_t *t) {
    (void)t;

    /* Drain pending LVGL actions (queued from httpd task) */
    ws_pending_action_t action;
    while (ws_action_queue &&
           xQueueReceive(ws_action_queue, &action, 0) == pdTRUE) {
        if (action.fn) action.fn(action.arg);
    }

    if (ws_fd_count > 0) {
        ws_broadcast_state();
    }
}

/* ── Public API ── */

bool ws_server_start(uint16_t port) {
    if (ws_httpd) return true;

    /* Create LVGL action queue (once) */
    if (ws_action_queue == NULL) {
        ws_action_queue = xQueueCreate(WS_ACTION_QUEUE_SIZE, sizeof(ws_pending_action_t));
    }

    /* Ensure TCP/IP stack is up (safe to call multiple times — returns
     * ESP_ERR_INVALID_STATE if already initialised, which we ignore). */
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err));
        return false;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return false;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.stack_size = 16384;
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
    char *buf = malloc(8192);
    if (!buf) { ESP_LOGE(TAG, "broadcast_state: malloc failed"); return; }
    int n = build_state_json(buf, 8192);
    ws_send_all(buf, n);
    free(buf);
}

void ws_broadcast_process_list(void) {
    char *buf = malloc(8192);
    if (!buf) { ESP_LOGE(TAG, "broadcast_process_list: malloc failed"); return; }
    int n = build_process_list_json(buf, 8192);
    ws_send_all(buf, n);
    free(buf);
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

#else /* !SOC_WIFI_SUPPORTED and no ESP_WIFI_REMOTE — no Wi-Fi at all, stubs */

#include "esp_log.h"
static const char *TAG = "ws_server";

bool ws_server_start(uint16_t port) {
    ESP_LOGW(TAG, "WebSocket server not available (no Wi-Fi on this SoC)");
    return false;
}
void ws_server_stop(void) {}
bool ws_server_is_running(void) { return false; }
void ws_broadcast_state(void) {}
void ws_broadcast_process_list(void) {}
void ws_broadcast_event(const char *event_name, const char *json_data) {
    (void)event_name; (void)json_data;
}

#endif /* SOC_WIFI_SUPPORTED || CONFIG_ESP_WIFI_REMOTE_ENABLED */

#endif /* SIMULATOR_BUILD */
