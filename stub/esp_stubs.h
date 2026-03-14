#ifndef ESP_STUBS_H
#define ESP_STUBS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

/* Logging */
#define ESP_LOGI(tag, fmt, ...) printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) printf("[D][%s] " fmt "\n", tag, ##__VA_ARGS__)

/* Error handling */
typedef int esp_err_t;
#define ESP_OK   0
#define ESP_FAIL -1
#define ESP_ERROR_CHECK(x) do { (void)(x); } while(0)

/* GPIO */
typedef int gpio_num_t;
#define GPIO_NUM_NC -1
#define GPIO_NUM_0 0
#define GPIO_NUM_1 1
#define GPIO_NUM_2 2
#define GPIO_NUM_3 3
#define GPIO_NUM_4 4
#define GPIO_NUM_5 5
#define GPIO_NUM_6 6
#define GPIO_NUM_7 7
#define GPIO_NUM_8 8
#define GPIO_NUM_9 9
#define GPIO_NUM_10 10
#define GPIO_NUM_11 11
#define GPIO_NUM_12 12
#define GPIO_NUM_13 13
#define GPIO_NUM_14 14
#define GPIO_NUM_15 15
#define GPIO_NUM_16 16
#define GPIO_NUM_17 17
#define GPIO_NUM_18 18
#define GPIO_NUM_19 19
#define GPIO_NUM_20 20
#define GPIO_NUM_21 21
#define GPIO_NUM_35 35
#define GPIO_NUM_36 36
#define GPIO_NUM_37 37
#define GPIO_NUM_38 38
#define GPIO_NUM_39 39
#define GPIO_NUM_40 40
#define GPIO_NUM_41 41
#define GPIO_NUM_42 42
#define GPIO_NUM_45 45
#define GPIO_NUM_47 47
#define GPIO_NUM_48 48
#define GPIO_PULLUP_ENABLE 1
#define GPIO_PULLUP_DISABLE 0

typedef enum { GPIO_MODE_OUTPUT = 0, GPIO_MODE_INPUT = 1 } gpio_mode_t;
typedef struct { gpio_mode_t mode; uint64_t pin_bit_mask; } gpio_config_t;
static inline esp_err_t gpio_config_stub(const gpio_config_t *c) { return ESP_OK; }
static inline esp_err_t gpio_set_level(int g, uint32_t l) { return ESP_OK; }
static inline int gpio_get_level(int g) { return 0; }
#define gpio_config(x) gpio_config_stub(x)

/* Timer */
typedef void* esp_timer_handle_t;
typedef struct { void (*callback)(void*); const char *name; } esp_timer_create_args_t;
static inline esp_err_t esp_timer_create(const esp_timer_create_args_t *a, esp_timer_handle_t *h) { return ESP_OK; }
static inline esp_err_t esp_timer_start_periodic(esp_timer_handle_t t, uint64_t p) { return ESP_OK; }

/* LCD Panel */
typedef void* esp_lcd_panel_handle_t;
typedef void* esp_lcd_panel_io_handle_t;
typedef void* esp_lcd_i80_bus_handle_t;
typedef void* esp_lcd_touch_handle_t;
typedef struct { int dummy; } esp_lcd_panel_io_event_data_t;
#define LCD_CLK_SRC_PLL240M 0
static inline esp_err_t esp_lcd_new_i80_bus(const void *c, void **h) { return ESP_OK; }
static inline esp_err_t esp_lcd_new_panel_io_i80(void *b, const void *c, void **h) { return ESP_OK; }
static inline esp_err_t esp_lcd_panel_draw_bitmap(void *h, int x1, int y1, int x2, int y2, const void *d) { return ESP_OK; }

/* I2C */
#define I2C_MODE_MASTER 0
typedef struct {
    int mode;
    int sda_io_num;
    int scl_io_num;
    int sda_pullup_en;
    int scl_pullup_en;
    struct { int clk_speed; } master;
} i2c_config_t;
static inline esp_err_t i2c_param_config(int p, const i2c_config_t *c) { return ESP_OK; }
static inline esp_err_t i2c_driver_install(int p, int m, int rx, int tx, int f) { return ESP_OK; }

/* SPI / SD Card */
#define SPI3_HOST 2
#define SDSPI_HOST_ID SPI3_HOST
#define SDSPI_DEFAULT_DMA 0
typedef struct { int slot; } sdmmc_host_t;
typedef struct { int dummy; } sdmmc_card_t;
typedef struct { int mosi_io_num; int miso_io_num; int sclk_io_num; int quadwp_io_num; int quadhd_io_num; int max_transfer_sz; } spi_bus_config_t;
typedef struct { int host_id; int gpio_cs; int gpio_cd; int gpio_wp; } sdspi_device_config_t;
#define SDSPI_HOST_DEFAULT() (sdmmc_host_t){ .slot = SDSPI_HOST_ID }
#define SDSPI_DEVICE_CONFIG_DEFAULT() (sdspi_device_config_t){ .host_id = 0, .gpio_cs = -1, .gpio_cd = -1, .gpio_wp = -1 }
static inline esp_err_t spi_bus_initialize(int h, const spi_bus_config_t *c, int d) { return ESP_OK; }
static inline void sdmmc_card_print_info(FILE *f, void *card) { printf("[SIM] SD card info (stub)\n"); }

/* VFS FAT */
typedef struct { bool format_if_mount_failed; int max_files; } esp_vfs_fat_sdmmc_mount_config_t;
static inline esp_err_t esp_vfs_fat_sdspi_mount(const char *p, const sdmmc_host_t *h,
    const sdspi_device_config_t *s, const esp_vfs_fat_sdmmc_mount_config_t *m, sdmmc_card_t **c) { return ESP_OK; }

/* Heap / PSRAM */
#define MALLOC_CAP_SPIRAM   (1<<0)
#define MALLOC_CAP_INTERNAL (1<<1)
#define heap_caps_malloc(size, caps) malloc(size)
static inline size_t heap_caps_get_free_size(uint32_t c) { return (c == MALLOC_CAP_SPIRAM) ? 4194304 : 262144; }

/* PWM */
static inline esp_err_t ledcAttach(int p, int f, int r) { return ESP_OK; }
static inline void ledcWrite(int p, int d) {}

/* System */
static inline void esp_restart(void) { printf("[SIM] esp_restart() — ignoring\n"); }

/* FatFS extra flag */
#ifndef FA_OPEN_EXISTING
#define FA_OPEN_EXISTING 0x00
#endif

/* Misc */
#define LV_UNUSED(x) ((void)(x))
#ifndef random
#define random() rand()
#endif
#ifndef OUTPUT
#define OUTPUT 1
#endif
#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW 0
#endif

/* itoa — non-standard, not available on macOS */
static inline char* itoa(int val, char* buf, int base) {
    sprintf(buf, "%d", val);
    return buf;
}

#endif /* ESP_STUBS_H */
