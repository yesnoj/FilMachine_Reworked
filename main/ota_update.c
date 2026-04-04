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

void wifi_boot_auto_connect(void) {
    LV_LOG_USER("[WiFi SIM] wifi_boot_auto_connect — no-op in simulator");
}

#else
/* ═══════════════════════════════════════════════════════════════
 *  REAL ESP32 IMPLEMENTATION
 * ═══════════════════════════════════════════════════════════════ */

#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#if SOC_WIFI_SUPPORTED || defined(CONFIG_ESP_WIFI_REMOTE_ENABLED)
#include "esp_wifi.h"
#include "esp_netif.h"
#endif
#include "esp_event.h"
#include "esp_log.h"
#include "ws_server.h"
#include "ff.h"

static const char *TAG = "OTA";

#define OTA_BUF_SIZE 4096

static int   ota_progress  = 0;
static bool  ota_running   = false;
static char  ota_status[64] = "";
static char  ota_ip_addr[28] = "";  /* IP:port e.g. "192.168.100.200:8080" */

static httpd_handle_t ota_httpd = NULL;
static bool  wifi_connected = false;

extern struct gui_components gui;

/**
 * safe_strcopy — null-terminated copy without -Werror=stringop-truncation.
 * Uses memcpy instead of strncpy to avoid GCC truncation warnings under -O2.
 */
static void safe_strcopy(char *dst, const char *src, size_t dst_size)
{
    if (dst_size == 0) return;
    size_t len = strlen(src);
    if (len >= dst_size) len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

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
    config.server_port = 8080;
    config.ctrl_port   = 32770;  /* different from default 32768 to avoid conflict with ws_server */
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

#if SOC_WIFI_SUPPORTED || defined(CONFIG_ESP_WIFI_REMOTE_ENABLED)

/* Forward-declare WiFi state + helpers defined near the bottom of this file */
static bool fw_wifi_initialized = false;
static void fw_wifi_ensure_init(void);

/* ── Public Wi-Fi OTA server API ──────────────────────────── */
static bool ota_wifi_ap_active = false;

bool ota_wifi_server_start(void) {
    if (ota_httpd) return true; /* already running */

    /* Decide mode: if SSID configured → STA, otherwise → SoftAP */
    const char *ssid = gui.page.settings.settingsParams.wifiSSID;

    if (ssid[0] != '\0') {
        /* ── STA mode: connect to user's network ── */
        snprintf(ota_status, sizeof(ota_status), "Connecting to Wi-Fi...");
        fw_wifi_ensure_init();

        wifi_config_t wifi_config = {0};
        safe_strcopy((char *)wifi_config.sta.ssid, ssid,
                     sizeof(wifi_config.sta.ssid));
        safe_strcopy((char *)wifi_config.sta.password,
                     gui.page.settings.settingsParams.wifiPassword,
                     sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;

        esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
            snprintf(ota_status, sizeof(ota_status), "Wi-Fi config error");
            return false;
        }
        err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(err));
            snprintf(ota_status, sizeof(ota_status), "Wi-Fi connect error");
            return false;
        }
        ESP_LOGI(TAG, "Wi-Fi STA connecting to '%s'...", ssid);
    } else {
        /* ── SoftAP mode: create "FilMachine_WiFi" hotspot ── */
        snprintf(ota_status, sizeof(ota_status), "Starting AP...");
        ESP_LOGI(TAG, "No SSID configured — starting SoftAP 'FilMachine_WiFi'");

        /* Init TCP/IP + event loop (tolerate already-initialized) */
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

        /* Create AP netif (with DHCP server) — only once */
        static esp_netif_t *ap_netif = NULL;
        if (!ap_netif) {
            ap_netif = esp_netif_create_default_wifi_ap();
            if (!ap_netif) {
                ESP_LOGE(TAG, "Failed to create AP netif");
                snprintf(ota_status, sizeof(ota_status), "AP netif error");
                return false;
            }
        }

        /* Init WiFi if not yet done (skip if fw_wifi_ensure_init already ran) */
        if (!fw_wifi_initialized) {
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            err = esp_wifi_init(&cfg);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
                snprintf(ota_status, sizeof(ota_status), "WiFi init error");
                return false;
            }
        } else {
            /* WiFi was in STA mode — stop it first */
            esp_wifi_disconnect();
            esp_wifi_stop();
        }

        err = esp_wifi_set_mode(WIFI_MODE_AP);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_set_mode(AP) failed: %s", esp_err_to_name(err));
            snprintf(ota_status, sizeof(ota_status), "AP mode error");
            return false;
        }

        wifi_config_t ap_config = {
            .ap = {
                .ssid = "FilMachine_WiFi",
                .ssid_len = strlen("FilMachine_WiFi"),
                .max_connection = 2,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .channel = 6,
            },
        };
        /* Use the OTA PIN as WPA2 password */
        safe_strcopy((char *)ap_config.ap.password,
                     gui.element.otaWifiPopup.otaPin,
                     sizeof(ap_config.ap.password));

        err = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "AP set_config failed: %s", esp_err_to_name(err));
            snprintf(ota_status, sizeof(ota_status), "AP config error");
            return false;
        }
        err = esp_wifi_start();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "AP start failed: %s", esp_err_to_name(err));
            snprintf(ota_status, sizeof(ota_status), "AP start error");
            return false;
        }

        ota_wifi_ap_active = true;

        /* AP IP is always 192.168.4.1 by default, OTA server on port 8080 */
        safe_strcopy(ota_ip_addr, "192.168.4.1:8080", sizeof(ota_ip_addr));
        snprintf(ota_status, sizeof(ota_status), "AP: FilMachine_WiFi");

        /* Start the OTA HTTP server immediately (AP has IP from the start) */
        ota_start_webserver();
        ESP_LOGI(TAG, "SoftAP ready — SSID: FilMachine_WiFi, PIN: %s, http://192.168.4.1:8080",
                 gui.element.otaWifiPopup.otaPin);
    }

    return true;
}

bool ota_wifi_server_stop(void) {
    ota_stop_webserver();

    if (ota_wifi_ap_active) {
        esp_wifi_stop();
        ota_wifi_ap_active = false;
        /* If STA was initialized before, restore STA mode */
        if (fw_wifi_initialized) {
            esp_wifi_set_mode(WIFI_MODE_STA);
            esp_wifi_start();
        }
        ESP_LOGI(TAG, "SoftAP stopped");
    } else if (wifi_connected) {
        esp_wifi_disconnect();
        wifi_connected = false;
    }

    ota_ip_addr[0] = '\0';
    ota_status[0] = '\0';
    ESP_LOGI(TAG, "Wi-Fi OTA server stopped");
    return true;
}

#else /* !SOC_WIFI_SUPPORTED and no ESP_WIFI_REMOTE — no Wi-Fi at all */

bool ota_wifi_server_start(void) {
    ESP_LOGW(TAG, "Wi-Fi OTA not available on this SoC (no Wi-Fi)");
    return false;
}

bool ota_wifi_server_stop(void) { return true; }

#endif /* SOC_WIFI_SUPPORTED || CONFIG_ESP_WIFI_REMOTE_ENABLED */

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

        /* Yield briefly so the DPI display controller can refresh the
         * framebuffer from PSRAM without bus contention with flash writes.
         * Without this delay the screen flickers blue during OTA. */
        vTaskDelay(pdMS_TO_TICKS(2));
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

/*
 * SD OTA runs in a dedicated FreeRTOS task so the LVGL main loop
 * continues to service the UI (progress bar, status text) while
 * the firmware is being read from SD and written to flash.
 */
static void ota_sd_task(void *arg) {
    /* Heap-allocate the context (contains FIL which is ~600+ bytes with LFN) */
    sd_reader_ctx_t *ctx = heap_caps_malloc(sizeof(sd_reader_ctx_t), MALLOC_CAP_SPIRAM);
    if (!ctx) {
        snprintf(ota_status, sizeof(ota_status), "Error: out of memory");
        ESP_LOGE(TAG, "SD OTA: cannot allocate sd_reader_ctx_t");
        ota_running = false;
        vTaskDelete(NULL);
        return;
    }

    FRESULT res = f_open(&ctx->file, "/FilMachine_fw.bin", FA_READ);
    if (res != FR_OK) {
        snprintf(ota_status, sizeof(ota_status), "Error: cannot open firmware file");
        ESP_LOGE(TAG, "SD OTA: cannot open /FilMachine_fw.bin (err %d)", res);
        free(ctx);
        ota_running = false;
        vTaskDelete(NULL);
        return;
    }

    int total_size = (int)f_size(&ctx->file);
    snprintf(ota_status, sizeof(ota_status), "Reading SD card...");
    ESP_LOGI(TAG, "Starting SD OTA, size: %d bytes", total_size);

    ota_write_from_stream(sd_read_fn, ctx, total_size);
    f_close(&ctx->file);
    free(ctx);
    vTaskDelete(NULL);
}

bool ota_start_sd(void) {
    if (ota_running) return false;
    ota_running = true;
    ota_progress = 0;
    snprintf(ota_status, sizeof(ota_status), "Starting SD update...");

    /* Launch OTA in a background task (8 KB stack — FIL struct is ~600 bytes) */
    BaseType_t ret = xTaskCreatePinnedToCore(
        ota_sd_task, "ota_sd", 8192, NULL, 5, NULL, tskNO_AFFINITY);
    if (ret != pdPASS) {
        snprintf(ota_status, sizeof(ota_status), "Error: cannot create OTA task");
        ota_running = false;
        return false;
    }
    return true;
}

/* ═══════════════════════════════════════════════════════════════
 * Wi-Fi scan/connect API (real firmware)
 *
 * Works on both native-WiFi SoCs (ESP32-S3) and on ESP32-P4
 * via esp_wifi_remote + esp_hosted (C6 companion chip over SDIO).
 * The esp_wifi API is identical in both cases.
 * ═══════════════════════════════════════════════════════════════ */
#if SOC_WIFI_SUPPORTED || defined(CONFIG_ESP_WIFI_REMOTE_ENABLED)

static bool fw_wifi_connected = false;
static char fw_connected_ssid[33] = "";
static char fw_ip_addr[20] = "";

/* ── Auto-retry state for reason-8 disconnects after connect ── */
static bool fw_wifi_connect_pending = false;  /* true while waiting for connect result */
static uint8_t fw_wifi_retry_count = 0;
#define FW_WIFI_MAX_RETRIES 2

/* ── Async popup for WiFi errors (runs in LVGL thread) ── */
static char wifi_error_msg[128] = "";

static void wifi_error_popup_async(void *arg) {
    (void)arg;
    messagePopupCreate(wifiErrorTitle_text, wifi_error_msg, NULL, NULL, NULL);
}

/* ── WiFi + IP event handler for scan/connect ── */
static void fw_wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *disc =
                    (wifi_event_sta_disconnected_t *)event_data;
                ESP_LOGW(TAG, "[WiFi] Disconnected, reason: %d (ssid: %.*s)",
                         disc->reason, disc->ssid_len, disc->ssid);
                fw_wifi_connected = false;
                fw_ip_addr[0] = '\0';

                /* Show error popup with human-readable reason
                 * Reason codes from esp_wifi_types.h:
                 *   2=AUTH_EXPIRE, 8=ASSOC_LEAVE, 14=MIC_FAILURE,
                 *   15=4WAY_HANDSHAKE_TIMEOUT, 16=HANDSHAKE_TIMEOUT,
                 *   201=NO_AP_FOUND, 202=AUTH_FAIL, 203/204/205=NO_AP variants
                 */
                uint8_t r = disc->reason;

                /* Reason 8 = ASSOC_LEAVE — if we just tried to connect, auto-retry
                 * (ESP-Hosted C6 often drops the first connect after a scan) */
                if (r == 8) {
                    if (fw_wifi_connect_pending && fw_wifi_retry_count < FW_WIFI_MAX_RETRIES) {
                        fw_wifi_retry_count++;
                        ESP_LOGI(TAG, "[WiFi] Reason 8 during connect — auto-retry %d/%d",
                                 fw_wifi_retry_count, FW_WIFI_MAX_RETRIES);
                        esp_wifi_connect();  /* retry with same config */
                        break;
                    }
                    /* Normal disconnect (not during connect) — just re-enable UI */
                    fw_wifi_connect_pending = false;
                    wifi_popup_connection_failed();
                    break;
                }

                /* Any other reason clears the connect-pending state */
                fw_wifi_connect_pending = false;
                fw_wifi_retry_count = 0;

                const char *reason_str;
                if (r == 202)        reason_str = wifiErrAuthFailed_text;
                else if (r == 15)    reason_str = wifiErrHandshakeTimeout_text;
                else if (r == 14)    reason_str = wifiErrMicFailure_text;
                else if (r == 16)    reason_str = wifiErrGroupKeyTimeout_text;
                else if (r == 201)   reason_str = wifiErrApNotFound_text;
                else if (r == 203 || r == 204 || r == 205)
                                     reason_str = wifiErrApNotFoundGeneric_text;
                else if (r == 2)     reason_str = wifiErrAuthExpired_text;
                else if (r == 6)     reason_str = wifiErrClass2Frame_text;
                else if (r == 7)     reason_str = wifiErrClass3Frame_text;
                else if (r == 210)   reason_str = wifiErrConnectionFail_text;
                else if (r == 200)   reason_str = wifiErrBeaconTimeout_text;
                else {
                    snprintf(wifi_error_msg, sizeof(wifi_error_msg),
                             wifiErrUnknownFmt_text, r);
                    lv_async_call(wifi_error_popup_async, NULL);
                    wifi_popup_connection_failed();
                    break;
                }
                snprintf(wifi_error_msg, sizeof(wifi_error_msg), "%s", reason_str);
                lv_async_call(wifi_error_popup_async, NULL);
                wifi_popup_connection_failed();  /* clear pending + re-enable Connect button */
                break;
            }
            case WIFI_EVENT_STA_CONNECTED: {
                wifi_event_sta_connected_t *conn =
                    (wifi_event_sta_connected_t *)event_data;
                /* Connection succeeded — clear retry state */
                fw_wifi_connect_pending = false;
                fw_wifi_retry_count = 0;
                /* Save connected SSID (handles auto-connect from NVS too) */
                int len = conn->ssid_len < sizeof(fw_connected_ssid) - 1
                        ? conn->ssid_len : sizeof(fw_connected_ssid) - 1;
                memcpy(fw_connected_ssid, conn->ssid, len);
                fw_connected_ssid[len] = '\0';
                ESP_LOGI(TAG, "[WiFi] Connected to '%s' (ch:%d)", fw_connected_ssid, conn->channel);
                break;
            }
            case WIFI_EVENT_SCAN_DONE: {
                ESP_LOGI(TAG, "[WiFi] Scan complete");
                /* Collect results into popup struct and notify UI */
                struct sWifiPopup *wp = &gui.element.wifiPopup;
                wp->scanCount = wifi_scan_get_results(wp->scanResults, MAX_WIFI_SCAN_RESULTS);
                ESP_LOGI(TAG, "[WiFi] Scan found %d APs", wp->scanCount);
                wifi_popup_scan_done();  /* async-notify LVGL to populate list */
                break;
            }
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(fw_ip_addr, sizeof(fw_ip_addr), IPSTR, IP2STR(&event->ip_info.ip));
        /* Also update OTA module's IP copy */
        snprintf(ota_ip_addr, sizeof(ota_ip_addr), "%s:8080", fw_ip_addr);
        ESP_LOGI(TAG, "[WiFi] Got IP: %s", fw_ip_addr);
        fw_wifi_connected = true;
        wifi_connected = true;
        wifi_popup_connection_result();  /* save credentials + re-enable Connect button */

        /* Start WebSocket server so the Flutter app can connect */
        ws_server_start(WS_SERVER_PORT);

        /* If OTA web server was requested, start it now that we have an IP */
        if (ota_httpd == NULL) {
            ota_start_webserver();
            snprintf(ota_status, sizeof(ota_status), "Server running");
        }
    }
}

/* ── Lazy init: call once before any WiFi operation ── */
static void fw_wifi_ensure_init(void) {
    if (fw_wifi_initialized) return;

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err));
        return;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return;
    }
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &fw_wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &fw_wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    /* Clear any credentials the C6 has cached in its NVS
     * so it doesn't auto-connect without our permission */
    wifi_config_t empty_config = {0};
    esp_wifi_set_config(WIFI_IF_STA, &empty_config);

    ESP_ERROR_CHECK(esp_wifi_start());

    fw_wifi_initialized = true;

    /* If auto-connect is enabled and credentials are saved, connect now */
    if (gui.page.settings.settingsParams.wifiEnabled &&
        gui.page.settings.settingsParams.wifiSSID[0] != '\0') {
        ESP_LOGI(TAG, "[WiFi] Auto-connecting to '%s'...",
                 gui.page.settings.settingsParams.wifiSSID);
        wifi_icon_set_connecting();  /* blink white icon while connecting */
        wifi_config_t wifi_config = {0};
        safe_strcopy((char *)wifi_config.sta.ssid,
                     gui.page.settings.settingsParams.wifiSSID,
                     sizeof(wifi_config.sta.ssid));
        safe_strcopy((char *)wifi_config.sta.password,
                     gui.page.settings.settingsParams.wifiPassword,
                     sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_connect();
    } else {
        ESP_LOGI(TAG, "[WiFi] Auto-connect skipped: enabled=%d, ssid='%s'",
                 gui.page.settings.settingsParams.wifiEnabled,
                 gui.page.settings.settingsParams.wifiSSID);
    }

    ESP_LOGI(TAG, "[WiFi] Stack initialized (STA mode)");
}

int wifi_scan_start(void) {
    fw_wifi_ensure_init();

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };

    esp_err_t err = esp_wifi_scan_start(&scan_config, false); /* non-blocking */
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[WiFi] Scan failed: %s", esp_err_to_name(err));
        return -1;  /* error */
    }

    ESP_LOGI(TAG, "[WiFi] Async scan started");
    return 0;  /* scan launched, results arrive via SCAN_DONE event */
}

int wifi_scan_get_results(wifiScanResult_t *results, int max_results) {
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count == 0) return 0;

    uint16_t fetch = (ap_count > (uint16_t)max_results) ? (uint16_t)max_results : ap_count;
    wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t) * fetch);
    if (!ap_records) return 0;

    esp_err_t err = esp_wifi_scan_get_ap_records(&fetch, ap_records);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[WiFi] Get scan results failed: %s", esp_err_to_name(err));
        free(ap_records);
        return 0;
    }

    int count = (int)fetch;
    for (int i = 0; i < count; i++) {
        snprintf(results[i].ssid, sizeof(results[i].ssid), "%s", (char *)ap_records[i].ssid);
        results[i].rssi = ap_records[i].rssi;
        results[i].open = (ap_records[i].authmode == WIFI_AUTH_OPEN);
    }

    free(ap_records);
    ESP_LOGI(TAG, "[WiFi] Returning %d scan results", count);
    return count;
}

bool wifi_connect(const char *ssid, const char *password) {
    fw_wifi_ensure_init();

    /* Disconnect from any previous network first */
    esp_wifi_disconnect();

    wifi_config_t wifi_config = {0};
    safe_strcopy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (password && password[0]) {
        safe_strcopy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }
    /* Enable PMF (Protected Management Frames) — required by many modern routers */
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_LOGI(TAG, "[WiFi] Connecting to '%s' (pwd len=%d)...", ssid,
             password ? (int)strlen(password) : 0);

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[WiFi] set_config failed: %s", esp_err_to_name(err));
        return false;
    }
    /* Mark connect-pending so reason-8 disconnect triggers auto-retry */
    fw_wifi_connect_pending = true;
    fw_wifi_retry_count = 0;

    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[WiFi] Connect failed: %s", esp_err_to_name(err));
        fw_wifi_connect_pending = false;
        return false;
    }

    snprintf(fw_connected_ssid, sizeof(fw_connected_ssid), "%s", ssid);
    return true;
}

void wifi_disconnect(void) {
    if (fw_wifi_connected) {
        esp_wifi_disconnect();
    }
    fw_wifi_connected = false;
    fw_connected_ssid[0] = '\0';
    fw_ip_addr[0] = '\0';
    ESP_LOGI(TAG, "[WiFi] Disconnected");
}

bool wifi_is_connected(void) {
    return fw_wifi_connected;
}

const char *wifi_get_connected_ssid(void) {
    return fw_wifi_connected ? fw_connected_ssid : NULL;
}

const char *wifi_get_ip_address(void) {
    return fw_wifi_connected ? fw_ip_addr : NULL;
}

void wifi_boot_auto_connect(void) {
    if (gui.page.settings.settingsParams.wifiEnabled &&
        gui.page.settings.settingsParams.wifiSSID[0] != '\0') {
        ESP_LOGI(TAG, "[WiFi] Boot auto-connect: enabled=%d, ssid='%s'",
                 gui.page.settings.settingsParams.wifiEnabled,
                 gui.page.settings.settingsParams.wifiSSID);
        fw_wifi_ensure_init();   /* will auto-connect inside */
    } else {
        ESP_LOGI(TAG, "[WiFi] Boot auto-connect skipped: enabled=%d, ssid='%s'",
                 gui.page.settings.settingsParams.wifiEnabled,
                 gui.page.settings.settingsParams.wifiSSID);
    }
}

#else /* No WiFi at all — provide stub implementations */

int wifi_scan_start(void) { return 0; }
int wifi_scan_get_results(wifiScanResult_t *results, int max_results) {
    (void)results; (void)max_results; return 0;
}
bool wifi_connect(const char *ssid, const char *password) {
    (void)ssid; (void)password; return false;
}
void wifi_disconnect(void) {}
bool wifi_is_connected(void) { return false; }
const char *wifi_get_connected_ssid(void) { return NULL; }
const char *wifi_get_ip_address(void) { return NULL; }
void wifi_boot_auto_connect(void) {}
void wifi_popup_connection_result(void) {}
void wifi_popup_connection_failed(void) {}
void wifi_popup_scan_done(void) {}
void wifi_icon_set_connecting(void) {}
void wifi_popup_refresh_list(void) {}

#endif /* SOC_WIFI_SUPPORTED || CONFIG_ESP_WIFI_REMOTE_ENABLED — wifi scan/connect */

#endif /* SIMULATOR_BUILD */
