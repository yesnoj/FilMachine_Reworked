/*
 * ST7701 LCD wrapper - ported from Arduino C++ to ESP-IDF C
 */
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_ldo_regulator.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_cache.h"
#include <string.h>

#include "esp_lcd_st7701.h"
#include "st7701_lcd.h"
#include "ppa_engine.h"
#include "esp_timer.h"

/* Physical panel resolution (internal to this driver — do NOT use
 * ST7701_PHYS_H_RES / ST7701_PHYS_V_RES which are the LVGL logical resolution from board.h) */
#define ST7701_PHYS_H_RES 480
#define ST7701_PHYS_V_RES 800

#define MIPI_DPI_PX_FORMAT (LCD_COLOR_PIXEL_FORMAT_RGB565)
#define LCD_BIT_PER_PIXEL (16)

#define EXAMPLE_MIPI_DSI_PHY_PWR_LDO_CHAN       3
#define EXAMPLE_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV 2500
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL           1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL          0
/* Backlight pin — matches LCD_BLK from board header (GPIO 23 on JC4880P433) */
#define EXAMPLE_PIN_NUM_BK_LIGHT                GPIO_NUM_23

static const char *TAG = "st7701_lcd";

static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;

static void enable_dsi_phy_power(void)
{
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = EXAMPLE_MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = EXAMPLE_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    ESP_LOGI(TAG, "MIPI DSI PHY Powered on");
}

static void init_lcd_backlight(void)
{
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
}

static void set_lcd_backlight(uint32_t level)
{
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, level);
}

esp_err_t st7701_lcd_init(void)
{
    enable_dsi_phy_power();
    init_lcd_backlight();

    // Create MIPI DSI bus
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus;
    esp_lcd_dsi_bus_config_t bus_config = ST7701_PANEL_BUS_DSI_2CH_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));

    ESP_LOGI(TAG, "Install MIPI DSI LCD control panel");
    esp_lcd_dbi_io_config_t dbi_config = ST7701_PANEL_IO_DBI_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &s_io_handle));

    // Create DPI panel config
    esp_lcd_dpi_panel_config_t dpi_config = ST7701_480_800_PANEL_60HZ_DPI_CONFIG(MIPI_DPI_PX_FORMAT);

    st7701_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
        .flags = {
            .use_mipi_interface = 1,
        }
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_5,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(s_io_handle, &panel_config, &s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));

    // Clear DPI framebuffer to black BEFORE turning on backlight
    // to avoid a white flash from uninitialized DPI buffer
    st7701_lcd_fill_screen(0x0000);

    // Turn on backlight
    set_lcd_backlight(EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    ESP_LOGI(TAG, "LCD initialized successfully (%dx%d, RGB565)", ST7701_PHYS_H_RES, ST7701_PHYS_V_RES);
    return ESP_OK;
}

esp_err_t st7701_lcd_draw_bitmap(uint16_t x_start, uint16_t y_start,
                                  uint16_t x_end, uint16_t y_end,
                                  const uint16_t *color_data)
{
    return esp_lcd_panel_draw_bitmap(s_panel_handle, x_start, y_start, x_end, y_end, color_data);
}

esp_err_t st7701_lcd_draw_rgb_bitmap(uint16_t x, uint16_t y,
                                      uint16_t w, uint16_t h,
                                      const uint16_t *color_data)
{
    return esp_lcd_panel_draw_bitmap(s_panel_handle, x, y, x + w, y + h, color_data);
}

esp_err_t st7701_lcd_draw_to_fb(uint16_t x, uint16_t y,
                                uint16_t w, uint16_t h,
                                const uint16_t *data)
{
    void *fb = NULL;
    esp_err_t ret = esp_lcd_dpi_panel_get_frame_buffer(s_panel_handle, 1, &fb);
    if (ret != ESP_OK || !fb) return ret ? ret : ESP_ERR_INVALID_STATE;

    uint16_t *fb16 = (uint16_t *)fb;
    for (int row = 0; row < h; row++) {
        memcpy(&fb16[(y + row) * ST7701_PHYS_H_RES + x],
               &data[row * w], w * sizeof(uint16_t));
    }

    void *sync_start = &fb16[y * ST7701_PHYS_H_RES];
    size_t sync_size = (size_t)h * ST7701_PHYS_H_RES * sizeof(uint16_t);
    esp_cache_msync(sync_start, sync_size,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    return ESP_OK;
}

/*
 * Persistent rotation buffer — only needed for CPU fallback path.
 * Primary path writes directly to DPI framebuffer via PPA.
 */
static uint16_t *s_rotate_buf = NULL;
static size_t    s_rotate_buf_bytes = 0;

/* ── Performance counters (logged every 100 flushes) ── */
static int64_t  s_perf_total_us   = 0;
static int      s_perf_count      = 0;
static int      s_perf_ppa_direct = 0;   /* PPA direct-to-FB */
static int      s_perf_ppa_buf    = 0;   /* PPA via temp buffer */
static int      s_perf_cpu_fb     = 0;   /* CPU fallback */

/*
 * PPA rotation angle for landscape→portrait mapping.
 * The CPU reference code does: output[r][c] = input[c][lw-1-r]
 * which is a 90° CCW rotation.
 *
 * Try 90 first (CCW per ESP-IDF docs).  If the display looks wrong
 * (mirrored / rotated), change to 270.
 */
#define PPA_ROTATION_ANGLE  90

esp_err_t st7701_lcd_draw_to_fb_rotated(uint16_t lx, uint16_t ly,
                                         uint16_t lw, uint16_t lh,
                                         const uint16_t *data)
{
    int64_t t_start = esp_timer_get_time();

    void *fb = NULL;
    esp_err_t ret = esp_lcd_dpi_panel_get_frame_buffer(s_panel_handle, 1, &fb);
    if (ret != ESP_OK || !fb) return ret ? ret : ESP_ERR_INVALID_STATE;

    const size_t fb_size = ST7701_PHYS_H_RES * ST7701_PHYS_V_RES * sizeof(uint16_t);
    const int phys_x_start = ly;
    const int phys_y_start = (ST7701_PHYS_V_RES - 1) - (lx + lw - 1);

    /*
     * ═══ PRIMARY PATH: PPA direct-to-framebuffer ═══
     *
     * PPA rotates the dirty rect and writes directly into the correct
     * position within the DPI framebuffer.  No temp buffer, no memcpy.
     * This eliminates ~6.6ms of memcpy overhead per flush.
     */
    ret = ppa_rotate_to_region(
        data, lw, lh,                              /* input dirty rect          */
        PPA_ROTATION_ANGLE,                         /* rotation angle            */
        fb, fb_size,                                /* output: DPI framebuffer   */
        ST7701_PHYS_H_RES, ST7701_PHYS_V_RES,      /* FB stride: 480×800        */
        (uint32_t)phys_x_start,                     /* x offset in physical FB   */
        (uint32_t)phys_y_start                      /* y offset in physical FB   */
    );

    if (ret == ESP_OK) {
        s_perf_ppa_direct++;

        /* PPA wrote to the DPI framebuffer via DMA, bypassing CPU cache.
         * Flush/invalidate so the DPI controller (and future CPU accesses
         * like fill_screen) see the freshly written pixels.              */
        uint16_t *fb16 = (uint16_t *)fb;
        void *sync_start = &fb16[phys_y_start * ST7701_PHYS_H_RES];
        size_t sync_size = (size_t)lw * ST7701_PHYS_H_RES * sizeof(uint16_t);
        esp_cache_msync(sync_start, sync_size,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    } else {
        /*
         * ═══ FALLBACK: PPA to temp buffer + memcpy ═══
         * (alignment constraints may prevent direct-to-FB)
         */
        const size_t need_pixels = (size_t)lw * lh;
        const size_t alloc_bytes = (need_pixels * sizeof(uint16_t) + 63) & ~(size_t)63;

        if (!s_rotate_buf || alloc_bytes > s_rotate_buf_bytes) {
            if (s_rotate_buf) heap_caps_free(s_rotate_buf);
            s_rotate_buf_bytes = alloc_bytes;
            s_rotate_buf = heap_caps_aligned_alloc(64, alloc_bytes,
                                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
            if (!s_rotate_buf) { s_rotate_buf_bytes = 0; return ESP_ERR_NO_MEM; }
        }

        ret = ppa_rotate_scale_rgb565_to(
            data, lw, lh, PPA_ROTATION_ANGLE,
            1.0f, 1.0f,
            s_rotate_buf, alloc_bytes,
            NULL, NULL, false
        );

        if (ret != ESP_OK) {
            /* ═══ LAST RESORT: CPU rotation ═══ */
            for (int dx = 0; dx < lw; dx++) {
                const int phys_row = lw - 1 - dx;
                uint16_t *dst_row = &s_rotate_buf[phys_row * lh];
                for (int dy = 0; dy < lh; dy++) {
                    dst_row[dy] = data[dy * lw + dx];
                }
            }
            s_perf_cpu_fb++;
        } else {
            s_perf_ppa_buf++;
        }

        /* Copy rotated data to DPI framebuffer row-by-row */
        uint16_t *fb16 = (uint16_t *)fb;
        for (int r = 0; r < lw; r++) {
            memcpy(&fb16[(phys_y_start + r) * ST7701_PHYS_H_RES + ly],
                   &s_rotate_buf[r * lh],
                   lh * sizeof(uint16_t));
        }

        /* Flush cache lines for memcpy-written region */
        void *sync_start = &fb16[phys_y_start * ST7701_PHYS_H_RES];
        size_t sync_size = (size_t)lw * ST7701_PHYS_H_RES * sizeof(uint16_t);
        esp_cache_msync(sync_start, sync_size,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    }

    int64_t t_end = esp_timer_get_time();

    /* ── Performance logging (every 100 flush calls) ── */
    s_perf_total_us += (t_end - t_start);
    s_perf_count++;

    if (s_perf_count >= 100) {
        ESP_LOGI(TAG, "PERF avg/flush: %lld us  (direct:%d buf:%d cpu:%d)",
                 s_perf_total_us / s_perf_count,
                 s_perf_ppa_direct, s_perf_ppa_buf, s_perf_cpu_fb);
        s_perf_total_us = 0;
        s_perf_count = s_perf_ppa_direct = s_perf_ppa_buf = s_perf_cpu_fb = 0;
    }

    return ESP_OK;
}

esp_err_t st7701_lcd_fill_screen(uint16_t color)
{
    /* Write directly to the single DPI framebuffer — faster than draw_bitmap
     * and avoids the DMA2D pipeline overhead. */
    void *fb = NULL;
    esp_err_t ret = esp_lcd_dpi_panel_get_frame_buffer(s_panel_handle, 1, &fb);
    if (ret != ESP_OK || !fb) return ret ? ret : ESP_ERR_INVALID_STATE;

    const size_t total_pixels = ST7701_PHYS_H_RES * ST7701_PHYS_V_RES;
    const size_t fb_bytes = total_pixels * sizeof(uint16_t);

    if (color == 0x0000) {
        memset(fb, 0, fb_bytes);
    } else {
        uint16_t *p = (uint16_t *)fb;
        for (size_t i = 0; i < total_pixels; i++) p[i] = color;
    }

    esp_cache_msync(fb, fb_bytes,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    return ESP_OK;
}

void st7701_lcd_get_handles(bsp_lcd_handles_t *ret_handles)
{
    if (ret_handles) {
        ret_handles->io = s_io_handle;
        ret_handles->mipi_dsi_bus = NULL;
        ret_handles->panel = s_panel_handle;
        ret_handles->control = NULL;
    }
}

uint16_t st7701_lcd_width(void)
{
    return ST7701_PHYS_H_RES;
}

uint16_t st7701_lcd_height(void)
{
    return ST7701_PHYS_V_RES;
}
