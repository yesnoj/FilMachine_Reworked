/**
 * @file sd_log.h
 * Boot/crash logger on the microSD card.
 *
 * Every boot creates /log/boot_NNNN.txt on the SD card (oldest files are
 * pruned, SD_LOG_MAX_FILES kept). The file starts with a header (firmware
 * version, reset reason, crash summary from the flash core dump if the
 * previous run panicked) and then captures, in the background:
 *   - every ESP_LOGx line (via esp_log_set_vprintf hook)
 *   - every LV_LOG line   (via the my_print LVGL callback → sd_log_line)
 *
 * Lines are buffered in RAM and flushed to the SD by a low-priority task
 * every SD_LOG_FLUSH_MS, so logging never blocks the UI. Firmware only —
 * the simulator does not compile this module (weak no-op stubs are provided
 * where shared code calls into it).
 */
#ifndef SD_LOG_H
#define SD_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#define SD_LOG_DIR        "/sd/log"
#define SD_LOG_MAX_FILES  20      /* boot files kept on card               */
#define SD_LOG_BUF_SIZE   8192    /* RAM ring buffer                       */
#define SD_LOG_FLUSH_MS   2000    /* background flush period               */

/* Call once at boot, AFTER the SD card is mounted. Creates the /log dir,
 * rotates old files, writes the boot header (incl. previous-crash summary)
 * and installs the log hooks. Safe to call when the SD failed: it no-ops. */
void sd_log_init(void);

/* Append one raw line (a '\n' is added). Used by the LVGL log callback;
 * can be called from any task. No-op before sd_log_init(). */
void sd_log_line(const char *line);

/* Force a flush of the RAM buffer to the SD (fsync included). */
void sd_log_flush(void);

/* Flush and close the file — call right before an intentional reboot. */
void sd_log_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_LOG_H */
