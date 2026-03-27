/**
 * @file ota_update.c
 *
 * OTA firmware update module — supports SD card and Wi-Fi (web upload).
 *
 * Wi-Fi mode: the board starts a local HTTP server. The user connects
 * with a browser to http://<board-ip> and uploads the .bin file via
 * a drag-and-drop web page. The firmware is streamed directly to the
 * OTA partition — no intermediate storage needed.
 *
 * API consumed by the UI (page_tools.c):
 *   - ota_check_sd()            → checks if firmware file exists on SD
 *   - ota_start_sd()            → starts OTA from SD card
 *   - ota_wifi_server_start()   → connects Wi-Fi + starts HTTP server
 *   - ota_wifi_server_stop()    → stops HTTP server + disconnects Wi-Fi
 *   - ota_wifi_server_is_running() → true if server is up
 *   - ota_get_ip_address()      → returns IP string (e.g. "192.168.1.42")
 *   - ota_get_progress()        → returns 0-100 progress
 *   - ota_get_status_text()     → returns human-readable status string
 *   - ota_get_running_version() → returns current firmware version
 *   - ota_is_running()          → true if an update is in progress
 */

#include "FilMachine.h"

#ifdef SIMULATOR_BUILD
/* ═══════════════════════════════════════════════════════════════
 *  SIMULATOR STUBS
 * ═══════════════════════════════════════════════════════════════ */

#include <stdio.h>
#include <string.h>

static int      sim_ota_progress = 0;
static bool     sim_ota_running  = false;
static bool     sim_wifi_server  = false;
static char     sim_ota_status[64] = "";
static char     sim_ip_addr[20]  = "";
static lv_timer_t *sim_ota_timer = NULL;

static void sim_ota_timer_cb(lv_timer_t *timer);

const char *ota_get_running_version(void) {
    return softwareVersionValue_text;
}

int ota_get_progress(void) {
    return sim_ota_progress;
}

const char *ota_get_status_text(void) {
    return sim_ota_status;
}

bool ota_is_running(void) {
    return sim_ota_running;
}

const char *ota_get_ip_address(void) {
    return sim_ip_addr;
}

bool ota_wifi_server_is_running(void) {
    return sim_wifi_server;
}

/* ── SD card ─────────────────────────────────────────────── */

bool ota_check_sd(char *version_out, size_t version_out_len) {
    FILE *f = fopen("sd/FilMachine_fw.bin", "rb");
    if (f) {
        fclose(f);
        if (version_out && version_out_len > 0)
            snprintf(version_out, version_out_len, "v0.0.0.2 (simulated)");
        LV_LOG_USER("[OTA] SD firmware found: sd/FilMachine_fw.bin");
        return true;
    }
    LV_LOG_USER("[OTA] No SD firmware found");
    return false;
}

static void sim_ota_timer_cb(lv_timer_t *timer) {
    (void)timer;
    if (!sim_ota_running) return;

    sim_ota_progress += 5;
    if (sim_ota_progress >= 100) {
        sim_ota_progress = 100;
        sim_ota_running = false;
        snprintf(sim_ota_status, sizeof(sim_ota_status), "Update complete! Reboot required.");
        safeTimerDelete(&sim_ota_timer);
        LV_LOG_USER("[OTA] Simulated update complete");
    } else if (sim_ota_progress < 30) {
        snprintf(sim_ota_status, sizeof(sim_ota_status), "Erasing partition...");
    } else {
        snprintf(sim_ota_status, sizeof(sim_ota_status), "Writing firmware...");
    }
}

bool ota_start_sd(void) {
    if (sim_ota_running) return false;
    LV_LOG_USER("[OTA] Starting SD update (simulated)");
    sim_ota_progress = 0;
    sim_ota_running = true;
    snprintf(sim_ota_status, sizeof(sim_ota_status), "Reading SD card...");
    sim_ota_timer = lv_timer_create(sim_ota_timer_cb, 200, NULL);
    return true;
}

/* ── Wi-Fi web server (simulated) ────────────────────────── */

bool ota_wifi_server_start(void) {
    if (sim_wifi_server) return true;
    LV_LOG_USER("[OTA] Starting Wi-Fi update server (simulated)");
    sim_wifi_server = true;
    snprintf(sim_ip_addr, sizeof(sim_ip_addr), "192.168.1.42");
    snprintf(sim_ota_status, sizeof(sim_ota_status), "Server running");
    LV_LOG_USER("[OTA] Web server at http://%s", sim_ip_addr);
    return true;
}

bool ota_wifi_server_stop(void) {
    if (!sim_wifi_server) return true;
    LV_LOG_USER("[OTA] Stopping Wi-Fi update server (simulated)");
    sim_wifi_server = false;
    sim_ip_addr[0] = '\0';
    sim_ota_status[0] = '\0';
    return true;
}

/* ── Wi-Fi scan stubs (simulator) ──────────────────────────── */
static bool sim_wifi_connected = false;
static char sim_connected_ssid[33] = "";

int wifi_scan_start(void) {
    LV_LOG_USER("[WiFi-Sim] Scan started");
    return 0;
}

int wifi_scan_get_results(wifiScanResult_t *results, int max_results) {
    /* Return fake scan results for testing */
    const struct { const char *ssid; int8_t rssi; bool open; } fake[] = {
        {"HomeNetwork_5G",    -42, false},
        {"HomeNetwork_2.4G",  -55, false},
        {"Neighbor_WiFi",     -68, false},
        {"CafePublic",        -75, true},
        {"IoT_Network",       -80, false},
    };
    int count = sizeof(fake) / sizeof(fake[0]);
    if(count > max_results) count = max_results;

    for(int i = 0; i < count; i++) {
        snprintf(results[i].ssid, sizeof(results[i].ssid), "%s", fake[i].ssid);
        results[i].rssi = fake[i].rssi;
        results[i].open = fake[i].open;
    }
    LV_LOG_USER("[WiFi-Sim] Scan returned %d results", count);
    return count;
}

bool wifi_connect(const char *ssid, const char *password) {
    LV_LOG_USER("[WiFi-Sim] Connecting to '%s' with password '%s'", ssid, password);
    sim_wifi_connected = true;
    snprintf(sim_connected_ssid, sizeof(sim_connected_ssid), "%s", ssid);
    return true;
}

void wifi_disconnect(void) {
    LV_LOG_USER("[WiFi-Sim] Disconnected");
    sim_wifi_connected = false;
    sim_connected_ssid[0] = '\0';
}

bool wifi_is_connected(void) {
    return sim_wifi_connected;
}

const char *wifi_get_connected_ssid(void) {
    return sim_wifi_connected ? sim_connected_ssid : NULL;
}

const char *wifi_get_ip_address(void) {
    return sim_wifi_connected ? "192.168.1.42" : NULL;
}

#else
/* ═══════════════════════════════════════════════════════════════
 *  REAL ESP32 IMPLEMENTATION
 * ═══════════════════════════════════════════════════════════════ */

#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "ff.h"

static const char *TAG = "OTA";

#define OTA_BUF_SIZE 4096

static int   ota_progress  = 0;
static bool  ota_running   = false;
static char  ota_status[64] = "";
static char  ota_ip_addr[20] = "";

static httpd_handle_t ota_httpd = NULL;
static bool  wifi_connected = false;

extern struct gui_components gui;

const char *ota_get_running_version(void) {
    const esp_app_desc_t *desc = esp_app_get_description();
    return desc->version;
}

int ota_get_progress(void) {
    return ota_progress;
}

const char *ota_get_status_text(void) {
    return ota_status;
}

bool ota_is_running(void) {
    return ota_running;
}

const char *ota_get_ip_address(void) {
    return ota_ip_addr;
}

bool ota_wifi_server_is_running(void) {
    return (ota_httpd != NULL);
}

/* ── HTML page served at GET / ────────────────────────────── */
static const char ota_html_page[] =
    "<!DOCTYPE html>"
    "<html><head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>FilMachine Update</title>"
    "<style>"
    "body{font-family:sans-serif;max-width:500px;margin:40px auto;padding:20px;"
    "background:#1a1a2e;color:#e0e0e0;}"
    "h1{color:#00d4aa;text-align:center;}"
    ".info{background:#16213e;padding:15px;border-radius:8px;margin:20px 0;}"
    ".info p{margin:5px 0;}"
    ".dropzone{border:3px dashed #00d4aa;border-radius:12px;padding:40px;"
    "text-align:center;cursor:pointer;transition:all 0.3s;margin:20px 0;}"
    ".dropzone:hover,.dropzone.dragover{background:#16213e;border-color:#00ff88;}"
    ".dropzone p{font-size:18px;margin:10px 0;}"
    ".progress{display:none;margin:20px 0;}"
    ".progress-bar{width:100%%;height:30px;background:#16213e;border-radius:15px;overflow:hidden;}"
    ".progress-fill{height:100%%;background:linear-gradient(90deg,#00d4aa,#00ff88);"
    "width:0%%;transition:width 0.3s;border-radius:15px;}"
    ".status{text-align:center;font-size:16px;margin:10px 0;}"
    "input[type=file]{display:none;}"
    ".btn{display:block;width:100%%;padding:15px;background:#00d4aa;color:#1a1a2e;"
    "border:none;border-radius:8px;font-size:18px;cursor:pointer;margin:10px 0;}"
    ".btn:hover{background:#00ff88;}"
    ".btn:disabled{background:#555;cursor:not-allowed;}"
    ".warn{color:#ffa500;font-size:14px;text-align:center;}"
    "</style></head><body>"
    "<h1>FilMachine</h1>"
    "<div class='info'>"
    "<p><strong>Current version:</strong> <span id='ver'>%s</span></p>"
    "</div>"
    "<div class='dropzone' id='dropzone'>"
    "<p>Drag & drop firmware .bin file here</p>"
    "<p style='font-size:14px;color:#888;'>or click to select file</p>"
    "</div>"
    "<input type='file' id='fileInput' accept='.bin'>"
    "<div class='progress' id='progress'>"
    "<div class='progress-bar'><div class='progress-fill' id='progressFill'></div></div>"
    "<p class='status' id='statusText'>Uploading...</p>"
    "</div>"
    "<p class='warn'>Do not turn off the machine during update!</p>"
    "<script>"
    "const dz=document.getElementById('dropzone'),"
    "fi=document.getElementById('fileInput'),"
    "pg=document.getElementById('progress'),"
    "pf=document.getElementById('progressFill'),"
    "st=document.getElementById('statusText');"
    "dz.onclick=()=>fi.click();"
    "dz.ondragover=(e)=>{e.preventDefault();dz.classList.add('dragover');};"
    "dz.ondragleave=()=>dz.classList.remove('dragover');"
    "dz.ondrop=(e)=>{e.preventDefault();dz.classList.remove('dragover');"
    "if(e.dataTransfer.files.length)upload(e.dataTransfer.files[0]);};"
    "fi.onchange=()=>{if(fi.files.length)upload(fi.files[0]);};"
    "function upload(file){"
    "if(!file.name.endsWith('.bin')){alert('Please select a .bin file');return;}"
    "if(!confirm('Update firmware with '+file.name+'?'))return;"
    "dz.style.display='none';pg.style.display='block';"
    "const xhr=new XMLHttpRequest();"
    "xhr.open('POST','/update');"
    "xhr.upload.onprogress=(e)=>{if(e.lengthComputable){"
    "const pct=Math.round(e.loaded/e.total*100);"
    "pf.style.width=pct+'%%';st.textContent='Uploading... '+pct+'%%';}};"
    "xhr.onload=()=>{if(xhr.status===200){"
    "pf.style.width='100%%';st.textContent='Update complete! Rebooting...';"
    "setTimeout(()=>location.reload(),5000);"
    "}else{st.textContent='Error: '+xhr.responseText;}};"
    "xhr.onerror=()=>{st.textContent='Upload failed. Check connection.';};"
    "xhr.setRequestHeader('Content-Type','application/octet-stream');"
    "xhr.send(file);}"
    "</script></body></html>";

/* ── GET / handler ────────────────────────────────────────── */
static esp_err_t ota_index_handler(httpd_req_t *req) {
    char *html = malloc(sizeof(ota_html_page) + 64);
    if (!html) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    snprintf(html, sizeof(ota_html_page) + 64, ota_html_page,
             ota_get_running_version());
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    free(html);
    return ESP_OK;
}

/* ── POST /update handler — receives firmware binary ──────── */
static esp_err_t ota_upload_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "OTA upload started, content length: %d", req->content_len);

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
        return ESP_FAIL;
    }

    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
        return ESP_FAIL;
    }

    ota_running = true;
    ota_progress = 0;
    snprintf(ota_status, sizeof(ota_status), "Receiving firmware...");

    char *buf = malloc(OTA_BUF_SIZE);
    if (!buf) {
        esp_ota_abort(ota_handle);
        ota_running = false;
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    int total = req->content_len;
    int received = 0;
    bool success = true;

    while (received < total) {
        int n = httpd_req_recv(req, buf, OTA_BUF_SIZE);
        if (n <= 0) {
            if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
            ESP_LOGE(TAG, "Receive error at %d/%d", received, total);
            snprintf(ota_status, sizeof(ota_status), "Error: receive failed");
            success = false;
            break;
        }

        err = esp_ota_write(ota_handle, buf, n);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(err));
            snprintf(ota_status, sizeof(ota_status), "Error: write failed");
            success = false;
            break;
        }

        received += n;
        if (total > 0) {
            ota_progress = (received * 100) / total;
            snprintf(ota_status, sizeof(ota_status), "Writing firmware...");
        }
    }

    free(buf);

    if (!success) {
        esp_ota_abort(ota_handle);
        ota_running = false;
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload failed");
        return ESP_FAIL;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA end failed (invalid image?): %s", esp_err_to_name(err));
        snprintf(ota_status, sizeof(ota_status), "Error: invalid firmware");
        ota_running = false;
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid firmware image");
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Set boot partition failed: %s", esp_err_to_name(err));
        snprintf(ota_status, sizeof(ota_status), "Error: set boot failed");
        ota_running = false;
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Set boot partition failed");
        return ESP_FAIL;
    }

    ota_progress = 100;
    snprintf(ota_status, sizeof(ota_status), "Update complete! Rebooting...");
    ESP_LOGI(TAG, "OTA complete. Received %d bytes. Rebooting in 2s...", received);

    httpd_resp_sendstr(req, "OK");

    /* Give the response time to be sent, then reboot */
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

    return ESP_OK; /* never reached */
}

/* ── HTTP server start/stop ───────────────────────────────── */
static bool ota_start_webserver(void) {
    if (ota_httpd) return true;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 4;

    if (httpd_start(&ota_httpd, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return false;
    }

    /* GET / — serve the upload page */
    const httpd_uri_t uri_index = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = ota_index_handler,
    };
    httpd_register_uri_handler(ota_httpd, &uri_index);

    /* POST /update — receive firmware */
    const httpd_uri_t uri_update = {
        .uri      = "/update",
        .method   = HTTP_POST,
        .handler  = ota_upload_handler,
    };
    httpd_register_uri_handler(ota_httpd, &uri_update);

    ESP_LOGI(TAG, "OTA web server started on port %d", config.server_port);
    return true;
}

static void ota_stop_webserver(void) {
    if (ota_httpd) {
        httpd_stop(ota_httpd);
        ota_httpd = NULL;
        ESP_LOGI(TAG, "OTA web server stopped");
    }
}

/* ── Wi-Fi event handler ──────────────────────────────────── */
static void ota_wifi_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected, reconnecting...");
        wifi_connected = false;
        ota_ip_addr[0] = '\0';
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(ota_ip_addr, sizeof(ota_ip_addr), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", ota_ip_addr);
        wifi_connected = true;

        /* Start web server once we have an IP */
        ota_start_webserver();
        snprintf(ota_status, sizeof(ota_status), "Server running");
    }
}

/* ── Public Wi-Fi server API ──────────────────────────────── */
bool ota_wifi_server_start(void) {
    if (ota_httpd) return true; /* already running */

    snprintf(ota_status, sizeof(ota_status), "Connecting to Wi-Fi...");

    /* Initialize TCP/IP and Wi-Fi (safe to call multiple times) */
    static bool netif_initialized = false;
    if (!netif_initialized) {
        ESP_ERROR_CHECK(esp_netif_init());
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        netif_initialized = true;
    }

    /* Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &ota_wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &ota_wifi_event_handler, NULL, NULL));

    /* Configure with credentials from settings */
    wifi_config_t wifi_config = {0};
    /* Use persistent Wi-Fi credentials from machineSettings */
    strncpy((char *)wifi_config.sta.ssid,
            gui.page.settings.settingsParams.wifiSSID,
            sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password,
            gui.page.settings.settingsParams.wifiPassword,
            sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Wi-Fi connecting to '%s'...", gui.page.settings.settingsParams.wifiSSID);
    return true;
}

bool ota_wifi_server_stop(void) {
    ota_stop_webserver();

    if (wifi_connected) {
        esp_wifi_disconnect();
        esp_wifi_stop();
        wifi_connected = false;
    }

    ota_ip_addr[0] = '\0';
    ota_status[0] = '\0';
    ESP_LOGI(TAG, "Wi-Fi OTA server stopped");
    return true;
}

/* ── SD card OTA (unchanged) ──────────────────────────────── */

typedef struct {
    FIL file;
} sd_reader_ctx_t;

static int sd_read_fn(void *ctx, uint8_t *buf, int len) {
    sd_reader_ctx_t *sr = (sd_reader_ctx_t *)ctx;
    UINT br;
    FRESULT res = f_read(&sr->file, buf, len, &br);
    if (res != FR_OK) return -1;
    return (int)br;
}

static bool ota_write_from_stream(
    int (*read_fn)(void *ctx, uint8_t *buf, int len),
    void *ctx,
    int total_size
) {
    esp_ota_handle_t ota_handle;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        snprintf(ota_status, sizeof(ota_status), "Error: no OTA partition");
        return false;
    }

    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) {
        snprintf(ota_status, sizeof(ota_status), "Error: OTA begin failed");
        return false;
    }

    uint8_t *buf = malloc(OTA_BUF_SIZE);
    if (!buf) {
        esp_ota_abort(ota_handle);
        snprintf(ota_status, sizeof(ota_status), "Error: out of memory");
        return false;
    }

    int written = 0;
    ota_running = true;
    ota_progress = 0;

    while (ota_running) {
        int n = read_fn(ctx, buf, OTA_BUF_SIZE);
        if (n < 0) {
            snprintf(ota_status, sizeof(ota_status), "Error: read failed");
            free(buf);
            esp_ota_abort(ota_handle);
            ota_running = false;
            return false;
        }
        if (n == 0) break;

        err = esp_ota_write(ota_handle, buf, n);
        if (err != ESP_OK) {
            snprintf(ota_status, sizeof(ota_status), "Error: write failed");
            free(buf);
            esp_ota_abort(ota_handle);
            ota_running = false;
            return false;
        }

        written += n;
        if (total_size > 0) {
            ota_progress = (written * 100) / total_size;
            snprintf(ota_status, sizeof(ota_status), "Writing firmware...");
        } else {
            snprintf(ota_status, sizeof(ota_status), "Writing firmware...");
        }
    }

    free(buf);

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        snprintf(ota_status, sizeof(ota_status), "Error: validation failed");
        ota_running = false;
        return false;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        snprintf(ota_status, sizeof(ota_status), "Error: set boot failed");
        ota_running = false;
        return false;
    }

    ota_progress = 100;
    ota_running = false;
    snprintf(ota_status, sizeof(ota_status), "Update complete! Reboot required.");
    ESP_LOGI(TAG, "SD OTA complete. Written %d bytes.", written);
    return true;
}

bool ota_check_sd(char *version_out, size_t version_out_len) {
    FIL f;
    FRESULT res = f_open(&f, "/FilMachine_fw.bin", FA_READ);
    if (res != FR_OK) {
        ESP_LOGI(TAG, "No firmware file on SD card");
        return false;
    }

    UINT br;
    uint8_t hdr[288];
    f_lseek(&f, 0);
    res = f_read(&f, hdr, sizeof(hdr), &br);
    f_close(&f);

    if (res == FR_OK && br >= 288) {
        const char *fw_version = (const char *)&hdr[48];
        if (version_out && version_out_len > 0)
            snprintf(version_out, version_out_len, "%s", fw_version);
        ESP_LOGI(TAG, "SD firmware version: %s", fw_version);
    } else {
        if (version_out && version_out_len > 0)
            snprintf(version_out, version_out_len, "unknown");
    }
    return true;
}

bool ota_start_sd(void) {
    if (ota_running) return false;

    sd_reader_ctx_t ctx;
    FRESULT res = f_open(&ctx.file, "/FilMachine_fw.bin", FA_READ);
    if (res != FR_OK) {
        snprintf(ota_status, sizeof(ota_status), "Error: cannot open firmware file");
        return false;
    }

    int total_size = (int)f_size(&ctx.file);
    snprintf(ota_status, sizeof(ota_status), "Reading SD card...");
    ESP_LOGI(TAG, "Starting SD OTA, size: %d bytes", total_size);

    bool ok = ota_write_from_stream(sd_read_fn, &ctx, total_size);
    f_close(&ctx.file);
    return ok;
}

/* ── Wi-Fi scan API (real firmware) ──────────────────────────── */
static bool fw_wifi_connected = false;
static char fw_connected_ssid[33] = "";

int wifi_scan_start(void) {
    /* TODO: Implement with esp_wifi_scan_start() */
    ESP_LOGI(TAG, "[WiFi] Scan not yet implemented on real firmware");
    return 0;
}

int wifi_scan_get_results(wifiScanResult_t *results, int max_results) {
    /* TODO: Implement with esp_wifi_scan_get_ap_records() */
    (void)results; (void)max_results;
    return 0;
}

bool wifi_connect(const char *ssid, const char *password) {
    /* TODO: Implement with esp_wifi_connect() */
    ESP_LOGI(TAG, "[WiFi] Connect to '%s'", ssid);
    fw_wifi_connected = true;
    snprintf(fw_connected_ssid, sizeof(fw_connected_ssid), "%s", ssid);
    return true;
}

void wifi_disconnect(void) {
    /* TODO: Implement with esp_wifi_disconnect() */
    fw_wifi_connected = false;
    fw_connected_ssid[0] = '\0';
}

bool wifi_is_connected(void) {
    return fw_wifi_connected;
}

const char *wifi_get_connected_ssid(void) {
    return fw_wifi_connected ? fw_connected_ssid : NULL;
}

const char *wifi_get_ip_address(void) {
    return fw_wifi_connected ? "0.0.0.0" : NULL;
}

#endif /* SIMULATOR_BUILD */
