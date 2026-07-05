/**
 * @file page_debug.c
 * @brief Peripheral diagnostics screen — factory/bring-up test page.
 *
 * Opened from the splash by holding TWO fingers (board) or the on-screen DBG
 * button (simulator). Shows the live state of every connected peripheral in
 * grouped panels and provides buttons to actuate each output (valves, pump,
 * motor, heaters, audio) so a custom board can be validated end-to-end.
 *
 * Everything is read/driven through the existing public helpers so this page
 * stays decoupled from the drivers.
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "FilMachine.h"
#include "sensors.h"
#include "page_splash.h"
#ifndef SIMULATOR_BUILD
#include "audio.h"
#include "esp_system.h"                    /* esp_reset_reason */
#include "driver/temperature_sensor.h"     /* ESP32-P4 internal chip temp */
#endif

extern struct gui_components gui;
extern uint8_t initErrors;   /* boot init error code (SD / I2C / ...) */

#ifndef SIMULATOR_BUILD
static temperature_sensor_handle_t s_tsens = NULL;   /* chip temperature sensor */

static const char *reset_reason_str(void) {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "power-on";
        case ESP_RST_SW:        return "software";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "int WDT";
        case ESP_RST_TASK_WDT:  return "task WDT";
        case ESP_RST_WDT:       return "WDT";
        case ESP_RST_BROWNOUT:  return "brownout";
        case ESP_RST_DEEPSLEEP: return "deep-sleep";
        case ESP_RST_USB:       return "USB";
        default:                return "other";
    }
}
#endif

#if HAS_RAIL_MONITOR
#include "esp_adc/adc_oneshot.h"
static adc_oneshot_unit_handle_t s_adc = NULL;

static void rails_adc_init(void) {
    if (s_adc) return;
    adc_oneshot_unit_init_cfg_t uc = { .unit_id = ADC_UNIT_1 };
    if (adc_oneshot_new_unit(&uc, &s_adc) != ESP_OK) { s_adc = NULL; return; }
    adc_oneshot_chan_cfg_t cc = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    adc_oneshot_config_channel(s_adc, RAIL_12V_ADC_CH, &cc);
    adc_oneshot_config_channel(s_adc, RAIL_5V_ADC_CH,  &cc);
    adc_oneshot_config_channel(s_adc, RAIL_3V3_ADC_CH, &cc);
}

/* Approximate rail voltage. 12dB atten ≈ 0..~2.5V full-scale on 12 bit.
 * Refine with adc_cali once the real board + dividers exist. Returns <0 on error. */
static float rail_read(int ch, float divider) {
    if (!s_adc) return -1.0f;
    int raw = 0;
    if (adc_oneshot_read(s_adc, ch, &raw) != ESP_OK) return -1.0f;
    return ((float)raw * (2.5f / 4095.0f)) * divider;
}
#endif

/* ── Layout: header + 3x3 grid of panels on 800x480 ── */
#define DBG_HDR_H     46
#define DBG_PW        250
#define DBG_PH        138
#define DBG_GAP       6
#define DBG_COL_X(c)  (DBG_GAP + (c) * (DBG_PW + DBG_GAP))
#define DBG_ROW_Y(r)  (DBG_HDR_H + (r) * (DBG_PH + DBG_GAP))

#define DBG_GREEN     0x2E7D32
#define DBG_RED       0xC62828
#define DBG_GREY      0x555555
#define DBG_AMBER     0xF9A825

/* Live-updated widgets + output state. */
static struct {
    lv_obj_t  *scr;
    lv_timer_t *timer;

    lv_obj_t *lbl_bath, *lbl_chem, *dot_bath, *dot_chem;
    lv_obj_t *dot_wbmin, *dot_wbmax;
    lv_obj_t *dot_hall, *lbl_flow;
    lv_obj_t *dot_cmin[3], *dot_cmax[3];
    lv_obj_t *dot_mcp, *dot_sd, *dot_wifi, *lbl_ip;
    lv_obj_t *lbl_chip, *lbl_heap, *lbl_reset, *lbl_up;
    lv_obj_t *lbl_v12, *lbl_v5, *lbl_v33;

    bool      valve[5];       /* C1 C2 C3 WB WASTE */
    lv_obj_t *vbtn[5];
    bool      heater[2];
    lv_obj_t *hbtn[2];
} D;

/* ── small helpers ── */
static void dot_set(lv_obj_t *d, uint32_t rgb) {
    if (d) lv_obj_set_style_bg_color(d, lv_color_hex(rgb), 0);
}
static uint32_t dot_ok(bool ok) { return ok ? DBG_GREEN : DBG_GREY; }

static lv_obj_t *make_dot(lv_obj_t *parent, int x, int y) {
    lv_obj_t *d = lv_obj_create(parent);
    lv_obj_remove_style_all(d);
    lv_obj_set_size(d, 14, 14);
    lv_obj_set_pos(d, x, y);
    lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(d, lv_color_hex(DBG_GREY), 0);
    return d;
}

static lv_obj_t *make_label(lv_obj_t *parent, int x, int y, const lv_font_t *font,
                            uint32_t color, const char *txt) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_pos(l, x, y);
    return l;
}

/* A titled panel box; children are positioned in its local coordinates. */
static lv_obj_t *make_panel_ex(lv_obj_t *parent, int x, int y, int w, int h, const char *title) {
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_size(p, w, h);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(0x3A3A3A), 0);
    lv_obj_set_style_border_width(p, 1, 0);
    lv_obj_set_style_radius(p, 6, 0);
    lv_obj_set_style_pad_all(p, 6, 0);
    make_label(p, 0, 0, &lv_font_montserrat_16, 0xFFB300, title);
    return p;
}

static lv_obj_t *make_panel(lv_obj_t *parent, int col, int row, const char *title) {
    return make_panel_ex(parent, DBG_COL_X(col), DBG_ROW_Y(row), DBG_PW, DBG_PH, title);
}

static lv_obj_t *make_ctrl(lv_obj_t *parent, int x, int y, int w, const char *txt,
                           uint32_t bg, lv_event_cb_t cb, void *ud) {
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, w, 34);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
    lv_obj_set_style_radius(b, 5, 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_center(l);
    return b;
}

/* ── output control handlers ── */
static void ev_valve(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (i < 0 || i > 4) return;
    D.valve[i] = !D.valve[i];
    setValveState((uint8_t)i, D.valve[i]);           /* C1..WASTE = relay 0..4 */
    lv_obj_set_style_bg_color(D.vbtn[i], lv_color_hex(D.valve[i] ? DBG_GREEN : DBG_GREY), 0);
}

static void ev_heater(lv_event_t *e) {
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (i < 0 || i > 1) return;
    D.heater[i] = !D.heater[i];
    debugMcpWrite(i == 0 ? HEATER1_PIN : HEATER2_PIN, D.heater[i]);
    lv_obj_set_style_bg_color(D.hbtn[i], lv_color_hex(D.heater[i] ? DBG_RED : DBG_GREY), 0);
}

static void ev_pump(lv_event_t *e) {
    int m = (int)(intptr_t)lv_event_get_user_data(e);
    if (m == 0)      pump_set_forward(pumpPercentToDuty(60));
    else if (m == 1) pump_set_reverse(pumpPercentToDuty(60));
    else             pump_set_stop();
}

static void ev_motor(lv_event_t *e) {
    int m = (int)(intptr_t)lv_event_get_user_data(e);
    uint8_t duty = mapPercentageToValue(50, 10, 100);
    if (m == 0)      motor_set_forward(duty);
    else if (m == 1) motor_set_reverse(duty);
    else             motor_set_stop();
}

static void ev_tone(lv_event_t *e) {
    (void)e;
#ifndef SIMULATOR_BUILD
    audio_play_tone(1000, 200);
#endif
}

/* All outputs off, then leave the diagnostics page and return to the splash. */
static void ev_close(lv_event_t *e) {
    (void)e;
    closeAllValves();
    debugMcpWrite(HEATER1_PIN, false);
    debugMcpWrite(HEATER2_PIN, false);
    pump_set_stop();
    motor_set_stop();

    if (D.timer) { lv_timer_delete(D.timer); D.timer = NULL; }
    lv_obj_t *old = D.scr;
    D.scr = NULL;
    lv_obj_t *sp = splash_screen_create();
    lv_screen_load(sp);
    if (old) lv_obj_delete(old);
}

/* ── live refresh (~3x/s) ── */
static void dbg_refresh(lv_timer_t *t) {
    (void)t;
    if (D.scr == NULL) return;

    float bath = getCachedTemperature(TEMPERATURE_SENSOR_BATH);
    float chem = getCachedTemperature(TEMPERATURE_SENSOR_CHEMICAL);
    char b[24];
    if (bath > -100.0f) snprintf(b, sizeof b, "%.1f C", (double)bath); else strcpy(b, "--");
    lv_label_set_text(D.lbl_bath, b);
    if (chem > -100.0f) snprintf(b, sizeof b, "%.1f C", (double)chem); else strcpy(b, "--");
    lv_label_set_text(D.lbl_chem, b);
    dot_set(D.dot_bath, dot_ok(bath > -100.0f));
    dot_set(D.dot_chem, dot_ok(chem > -100.0f));

    dot_set(D.dot_wbmin, sensors_water_level_detected()     ? DBG_GREEN : DBG_GREY);
    dot_set(D.dot_wbmax, sensors_water_level_max_detected() ? DBG_GREEN : DBG_GREY);
    dot_set(D.dot_hall,  sensors_hall_magnet_detected()     ? DBG_GREEN : DBG_GREY);

    snprintf(b, sizeof b, "%.2f L/min  %lu p",
             (double)sensors_flow_get_litres_per_min(),
             (unsigned long)sensors_flow_get_total_pulses());
    lv_label_set_text(D.lbl_flow, b);

    for (int i = 0; i < 3; i++) {
        dot_set(D.dot_cmin[i], chemLevelMinDetected((uint8_t)i) ? DBG_GREEN : DBG_GREY);
        dot_set(D.dot_cmax[i], chemLevelMaxDetected((uint8_t)i) ? DBG_GREEN : DBG_GREY);
    }

    dot_set(D.dot_mcp, debugMcpPresent() ? DBG_GREEN : DBG_RED);
    dot_set(D.dot_sd,  (initErrors != INIT_ERROR_SD) ? DBG_GREEN : DBG_RED);
#ifndef SIMULATOR_BUILD
    bool wifi = wifi_is_connected();
    dot_set(D.dot_wifi, wifi ? DBG_GREEN : DBG_GREY);
    lv_label_set_text(D.lbl_ip, wifi ? wifi_get_ip_address() : "not connected");
#else
    dot_set(D.dot_wifi, DBG_GREY);
    lv_label_set_text(D.lbl_ip, "sim");
#endif

#ifndef SIMULATOR_BUILD
    float chipC = 0;
    if (s_tsens && temperature_sensor_get_celsius(s_tsens, &chipC) == ESP_OK)
        snprintf(b, sizeof b, "chip %.1f C", (double)chipC);
    else
        strcpy(b, "chip --");
    lv_label_set_text(D.lbl_chip, b);
#else
    lv_label_set_text(D.lbl_chip, "chip sim");
#endif

    snprintf(b, sizeof b, "PSRAM %uk", (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    lv_label_set_text(D.lbl_heap, b);
    snprintf(b, sizeof b, "up %lus", (unsigned long)(lv_tick_get() / 1000));
    lv_label_set_text(D.lbl_up, b);

#if HAS_RAIL_MONITOR
    float rv;
    rv = rail_read(RAIL_12V_ADC_CH, RAIL_12V_DIVIDER);
    if (rv >= 0) { snprintf(b, sizeof b, "%.2f V", (double)rv); lv_label_set_text(D.lbl_v12, b); }
    rv = rail_read(RAIL_5V_ADC_CH, RAIL_5V_DIVIDER);
    if (rv >= 0) { snprintf(b, sizeof b, "%.2f V", (double)rv); lv_label_set_text(D.lbl_v5, b); }
    rv = rail_read(RAIL_3V3_ADC_CH, RAIL_3V3_DIVIDER);
    if (rv >= 0) { snprintf(b, sizeof b, "%.2f V", (double)rv); lv_label_set_text(D.lbl_v33, b); }
#endif
}

/* ── build the screen ── */
void debugScreenCreate(void) {
    memset(&D, 0, sizeof(D));

    D.scr = lv_obj_create(NULL);
    lv_obj_set_scroll_dir(D.scr, LV_DIR_VER);   /* extra Rails panel below the grid */
    lv_obj_set_style_bg_color(D.scr, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(D.scr, LV_OPA_COVER, 0);

    /* Header */
    make_label(D.scr, DBG_GAP, 10, &lv_font_montserrat_20, 0xFFFFFF, "Peripheral Diagnostics");
    make_label(D.scr, 360, 16, &lv_font_montserrat_14, 0xAAAAAA, ota_get_running_version());
    make_ctrl(D.scr, 800 - 96 - DBG_GAP, 8, 96, "Close", DBG_RED, ev_close, NULL);

    /* ── Row 0 ── */
    lv_obj_t *p;

    p = make_panel(D.scr, 0, 0, "Temperature");
    make_label(p, 0, 34, &lv_font_montserrat_14, 0xDDDDDD, "Bath");
    D.dot_bath = make_dot(p, 96, 36);
    D.lbl_bath = make_label(p, 120, 34, &lv_font_montserrat_14, 0xFFFFFF, "--");
    make_label(p, 0, 66, &lv_font_montserrat_14, 0xDDDDDD, "Chem");
    D.dot_chem = make_dot(p, 96, 68);
    D.lbl_chem = make_label(p, 120, 66, &lv_font_montserrat_14, 0xFFFFFF, "--");

    p = make_panel(D.scr, 1, 0, "Water level (WB)");
    make_label(p, 0, 40, &lv_font_montserrat_14, 0xDDDDDD, "MIN");
    D.dot_wbmin = make_dot(p, 70, 42);
    make_label(p, 0, 74, &lv_font_montserrat_14, 0xDDDDDD, "MAX");
    D.dot_wbmax = make_dot(p, 70, 76);

    p = make_panel(D.scr, 2, 0, "Motion");
    make_label(p, 0, 40, &lv_font_montserrat_14, 0xDDDDDD, "Hall magnet");
    D.dot_hall = make_dot(p, 140, 42);
    make_label(p, 0, 74, &lv_font_montserrat_14, 0xDDDDDD, "Flow");
    D.lbl_flow = make_label(p, 44, 74, &lv_font_montserrat_14, 0xFFFFFF, "--");

    /* ── Row 1 ── */
    p = make_panel(D.scr, 0, 1, "Chem levels");
    for (int i = 0; i < 3; i++) {
        char c[8]; snprintf(c, sizeof c, "C%d", i + 1);
        make_label(p, 0, 32 + i * 30, &lv_font_montserrat_14, 0xDDDDDD, c);
        make_label(p, 40, 32 + i * 30, &lv_font_montserrat_12, 0x999999, "min");
        D.dot_cmin[i] = make_dot(p, 84, 34 + i * 30);
        make_label(p, 120, 32 + i * 30, &lv_font_montserrat_12, 0x999999, "max");
        D.dot_cmax[i] = make_dot(p, 164, 34 + i * 30);
    }

    p = make_panel(D.scr, 1, 1, "Bus / Network");
    make_label(p, 0, 32, &lv_font_montserrat_14, 0xDDDDDD, "MCP23017");
    D.dot_mcp = make_dot(p, 130, 34);
    make_label(p, 0, 58, &lv_font_montserrat_14, 0xDDDDDD, "SD card");
    D.dot_sd = make_dot(p, 130, 60);
    make_label(p, 0, 84, &lv_font_montserrat_14, 0xDDDDDD, "WiFi");
    D.dot_wifi = make_dot(p, 130, 86);
    D.lbl_ip = make_label(p, 0, 108, &lv_font_montserrat_12, 0x999999, "--");

    p = make_panel(D.scr, 2, 1, "System");
    D.lbl_chip  = make_label(p, 0, 30, &lv_font_montserrat_14, 0xFFFFFF, "chip --");
    D.lbl_heap  = make_label(p, 0, 56, &lv_font_montserrat_14, 0xFFFFFF, "PSRAM --");
    D.lbl_reset = make_label(p, 0, 82, &lv_font_montserrat_14, 0xFFFFFF, "rst --");
    D.lbl_up    = make_label(p, 0, 108, &lv_font_montserrat_14, 0xFFFFFF, "up --");
#ifndef SIMULATOR_BUILD
    if (!s_tsens) {
        /* -10..80 maps to a single valid ESP32-P4 measurement range; a wider
         * span (e.g. -10..100) crosses ranges and fails to install. */
        temperature_sensor_config_t tcfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
        if (temperature_sensor_install(&tcfg, &s_tsens) == ESP_OK)
            temperature_sensor_enable(s_tsens);
    }
    lv_label_set_text(D.lbl_reset, reset_reason_str());
#else
    lv_label_set_text(D.lbl_reset, "rst sim");
#endif

    /* ── Row 2: interactive output controls ── */
    p = make_panel(D.scr, 0, 2, "Valves");
    {
        const char *nm[5] = { "C1", "C2", "C3", "WB", "Wst" };
        for (int i = 0; i < 5; i++) {
            int col = i % 3, r = i / 3;
            D.vbtn[i] = make_ctrl(p, col * 78, 30 + r * 42, 74, nm[i], DBG_GREY,
                                  ev_valve, (void *)(intptr_t)i);
        }
    }

    p = make_panel(D.scr, 1, 2, "Pump / Motor");
    make_label(p, 0, 26, &lv_font_montserrat_12, 0x999999, "Pump");
    make_ctrl(p, 44, 22, 52, "FW",  DBG_GREEN, ev_pump, (void *)(intptr_t)0);
    make_ctrl(p, 100, 22, 52, "RV", DBG_AMBER, ev_pump, (void *)(intptr_t)1);
    make_ctrl(p, 156, 22, 60, "Stop", DBG_RED, ev_pump, (void *)(intptr_t)2);
    make_label(p, 0, 74, &lv_font_montserrat_12, 0x999999, "Motor");
    make_ctrl(p, 44, 70, 52, "FW",  DBG_GREEN, ev_motor, (void *)(intptr_t)0);
    make_ctrl(p, 100, 70, 52, "RV", DBG_AMBER, ev_motor, (void *)(intptr_t)1);
    make_ctrl(p, 156, 70, 60, "Stop", DBG_RED, ev_motor, (void *)(intptr_t)2);

    p = make_panel(D.scr, 2, 2, "Heaters / Audio");
    D.hbtn[0] = make_ctrl(p, 0, 30, 74, "H1", DBG_GREY, ev_heater, (void *)(intptr_t)0);
    D.hbtn[1] = make_ctrl(p, 82, 30, 74, "H2", DBG_GREY, ev_heater, (void *)(intptr_t)1);
    make_ctrl(p, 0, 74, 156, "Test tone", 0x1565C0, ev_tone, NULL);

    /* ── Row 3: Power rails (future custom board — scroll down to see) ── */
    p = make_panel_ex(D.scr, DBG_COL_X(0), DBG_ROW_Y(3), 3 * DBG_PW + 2 * DBG_GAP, 120, "Power rails");
    make_label(p, 0,   34, &lv_font_montserrat_14, 0xDDDDDD, "12V");
    D.lbl_v12 = make_label(p, 70,  34, &lv_font_montserrat_14, 0xFFFFFF, "n/a");
    make_label(p, 270, 34, &lv_font_montserrat_14, 0xDDDDDD, "5V");
    D.lbl_v5  = make_label(p, 330, 34, &lv_font_montserrat_14, 0xFFFFFF, "n/a");
    make_label(p, 520, 34, &lv_font_montserrat_14, 0xDDDDDD, "3.3V");
    D.lbl_v33 = make_label(p, 590, 34, &lv_font_montserrat_14, 0xFFFFFF, "n/a");
#if HAS_RAIL_MONITOR
    rails_adc_init();
#else
    make_label(p, 0, 70, &lv_font_montserrat_12, 0x888888,
               "Add resistor dividers on the custom board, then set HAS_RAIL_MONITOR + channels/ratios.");
#endif

    lv_screen_load(D.scr);
    dbg_refresh(NULL);
    D.timer = lv_timer_create(dbg_refresh, 350, NULL);
}
