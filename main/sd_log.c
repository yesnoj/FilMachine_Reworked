/**
 * @file sd_log.c
 * Boot/crash logger on the microSD card — see sd_log.h for the overview.
 *
 * Design notes:
 *  - Uses stdio on the VFS path /sd/... (the card is mounted with
 *    esp_vfs_fat_sdmmc_mount("/sd", ...)), so it works from any task.
 *  - The esp_log vprintf hook and sd_log_line() only append to a RAM ring
 *    buffer under a mutex; a dedicated low-priority task drains the buffer
 *    to the file and fsync()s. If the buffer fills up, lines are dropped
 *    and accounted for (never blocks callers).
 *  - ANSI color escapes from ESP_LOG are stripped before writing.
 *  - If the previous run crashed (panic / WDT), the header includes the
 *    core dump summary read back from the flash "coredump" partition, and
 *    the dump is then erased so it is reported only once. The full dump
 *    can be inspected with:  idf.py coredump-info
 */
#ifndef SIMULATOR_BUILD

#include "sd_log.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"

#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH
#include "esp_core_dump.h"
#endif

static const char *TAG = "sd_log";

static FILE             *s_file      = NULL;
static SemaphoreHandle_t s_mutex     = NULL;
static char              s_buf[SD_LOG_BUF_SIZE];
static size_t            s_len       = 0;
static uint32_t          s_dropped   = 0;
static vprintf_like_t    s_orig_vprintf = NULL;
static bool              s_ready     = false;

/* ── helpers ─────────────────────────────────────────────────────── */

static const char *reset_reason_str(esp_reset_reason_t r)
{
    switch (r) {
        case ESP_RST_POWERON:   return "POWERON (normal power-up)";
        case ESP_RST_SW:        return "SW (esp_restart — intentional reboot)";
        case ESP_RST_PANIC:     return "PANIC (firmware crash!)";
        case ESP_RST_INT_WDT:   return "INT_WDT (interrupt watchdog!)";
        case ESP_RST_TASK_WDT:  return "TASK_WDT (task watchdog!)";
        case ESP_RST_WDT:       return "WDT (other watchdog!)";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP wake";
        case ESP_RST_BROWNOUT:  return "BROWNOUT (supply voltage dip!)";
        case ESP_RST_SDIO:      return "SDIO";
        case ESP_RST_EXT:       return "EXT (external reset pin)";
        default:                return "UNKNOWN";
    }
}

/* Append to the RAM ring buffer, stripping ANSI escape sequences.
 * Must NOT log (would recurse). */
static void buf_append(const char *text, size_t n)
{
    if (!s_ready) return;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(20)) != pdTRUE) return;

    for (size_t i = 0; i < n; i++) {
        char c = text[i];
        if (c == '\033') {           /* strip ANSI escape: ESC ... 'm' */
            while (i < n && text[i] != 'm') i++;
            continue;                /* outer i++ skips the 'm' itself */
        }
        if (s_len < SD_LOG_BUF_SIZE) s_buf[s_len++] = c;
        else                         { s_dropped++; break; }
    }
    xSemaphoreGive(s_mutex);
}

/* Drain the RAM buffer to the file (called from the flush task / shutdown). */
static void drain_to_file(bool do_sync)
{
    if (!s_file) return;
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return;

    size_t   n       = s_len;
    uint32_t dropped = s_dropped;
    static char local[SD_LOG_BUF_SIZE];
    if (n) memcpy(local, s_buf, n);
    s_len = 0;
    s_dropped = 0;
    xSemaphoreGive(s_mutex);

    if (n) fwrite(local, 1, n, s_file);
    if (dropped)
        fprintf(s_file, "[sd_log] !! %lu bytes dropped (buffer full)\n",
                (unsigned long)dropped);
    if (n || dropped) {
        fflush(s_file);
        if (do_sync) fsync(fileno(s_file));
    }
}

static void flush_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(SD_LOG_FLUSH_MS));
        drain_to_file(true);
    }
}

/* esp_log hook: forward to the console AND capture into the buffer. */
static int sd_log_vprintf(const char *fmt, va_list args)
{
    char line[256];
    va_list copy;
    va_copy(copy, args);
    int n = vsnprintf(line, sizeof(line), fmt, copy);
    va_end(copy);
    if (n > 0) buf_append(line, (size_t)((n < (int)sizeof(line)) ? n : (int)sizeof(line) - 1));

    return s_orig_vprintf ? s_orig_vprintf(fmt, args) : n;
}

/* ── file rotation ───────────────────────────────────────────────── */

/* Scan SD_LOG_DIR for boot_NNNN.txt: returns next index, deletes the oldest
 * files so that at most SD_LOG_MAX_FILES-1 remain before the new one. */
static int rotate_and_next_index(void)
{
    int max_idx = -1, count = 0, min_idx = 0x7FFFFFFF;

    DIR *d = opendir(SD_LOG_DIR);
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            int idx;
            if (sscanf(e->d_name, "boot_%d.txt", &idx) == 1) {
                if (idx > max_idx) max_idx = idx;
                if (idx < min_idx) min_idx = idx;
                count++;
            }
        }
        closedir(d);
    }

    /* prune oldest until we are below the cap */
    while (count >= SD_LOG_MAX_FILES && min_idx <= max_idx) {
        char path[64];
        snprintf(path, sizeof(path), SD_LOG_DIR "/boot_%04d.txt", min_idx);
        if (unlink(path) == 0) count--;
        min_idx++;
    }

    return max_idx + 1;
}

/* ── boot header ─────────────────────────────────────────────────── */

static void write_header(int boot_idx, esp_reset_reason_t reason)
{
    const esp_app_desc_t *app = esp_app_get_description();

    fprintf(s_file,
            "════════════════════════════════════════════\n"
            " FilMachine boot log #%04d\n"
            " Firmware : %s (built %s %s)\n"
            " Reset    : %s\n"
            " Free heap: %lu bytes (min ever: n/a at boot)\n"
            "════════════════════════════════════════════\n",
            boot_idx,
            app ? app->version : "?",
            app ? app->date : "?", app ? app->time : "?",
            reset_reason_str(reason),
            (unsigned long)esp_get_free_heap_size());

#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH
    /* If the previous run crashed, pull the summary out of the flash
     * core dump and append it — this is the "black box" readout. */
    if (reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT ||
        reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT) {
        if (esp_core_dump_image_check() == ESP_OK) {
            esp_core_dump_summary_t *sum =
                heap_caps_malloc(sizeof(*sum), MALLOC_CAP_DEFAULT);
            if (sum && esp_core_dump_get_summary(sum) == ESP_OK) {
                fprintf(s_file,
                        "‼ PREVIOUS RUN CRASHED ‼\n"
                        "  Task    : %s\n"
                        "  Exc PC  : 0x%08lx\n"
                        "  ELF SHA : %.16s\n"
                        "  (full dump in flash — read it with: idf.py coredump-info)\n"
                        "────────────────────────────────────────────\n",
                        sum->exc_task,
                        (unsigned long)sum->exc_pc,
                        (const char *)sum->app_elf_sha256);
                /* Reported once — clear it so the next boot is clean. */
                esp_core_dump_image_erase();
            }
            if (sum) free(sum);
        } else {
            fprintf(s_file, "‼ Crash reset but no core dump image found.\n");
        }
    }
#endif

    fflush(s_file);
    fsync(fileno(s_file));
}

/* ── public API ──────────────────────────────────────────────────── */

void sd_log_init(void)
{
    if (s_ready) return;

    mkdir(SD_LOG_DIR, 0775);   /* no-op if it already exists */

    int idx = rotate_and_next_index();
    char path[64];
    snprintf(path, sizeof(path), SD_LOG_DIR "/boot_%04d.txt", idx);
    s_file = fopen(path, "w");
    if (!s_file) {
        ESP_LOGW(TAG, "cannot create %s — SD logging disabled", path);
        return;
    }

    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) { fclose(s_file); s_file = NULL; return; }

    write_header(idx, esp_reset_reason());

    /* Hook ESP_LOGx and start the background flusher. */
    s_orig_vprintf = esp_log_set_vprintf(sd_log_vprintf);
    s_ready = true;
    xTaskCreate(flush_task, "sd_log", 3072, NULL, tskIDLE_PRIORITY + 1, NULL);

    ESP_LOGI(TAG, "SD boot log: %s", path);
}

void sd_log_line(const char *line)
{
    if (!s_ready || !line) return;
    buf_append(line, strlen(line));
    buf_append("\n", 1);
}

void sd_log_flush(void)
{
    drain_to_file(true);
}

void sd_log_shutdown(void)
{
    if (!s_ready) return;
    s_ready = false;                     /* stop accepting new lines   */
    if (s_orig_vprintf) esp_log_set_vprintf(s_orig_vprintf);
    drain_to_file(true);
    if (s_file) { fclose(s_file); s_file = NULL; }
}

#endif /* !SIMULATOR_BUILD */
