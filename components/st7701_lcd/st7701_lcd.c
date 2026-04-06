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
/* Backlight pin — matches LCD_BLK from board header (GPIO 23 on JC4880P433) */
#define EXAMPLE_PIN_NUM_BK_LIGHT                GPIO_NUM_23

/* LEDC PWM configuration for backlight dimming */
#define BL_LEDC_TIMER       LEDC_TIMER_1       /* Timer 0 used by motor in accessories.c */
#define BL_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL     LEDC_CHANNEL_1     /* Channel 0 used by motor in accessories.c */
#define BL_LEDC_DUTY_RES    LEDC_TIMER_10_BIT  /* 0–1023, enough for smooth dimming */
#define BL_LEDC_FREQ_HZ     25000              /* 25 kHz — above audible range, no coil whine */
#define BL_DUTY_MAX         ((1 << 10) - 1)    /* 1023 */

static const char *TAG = "st7701_lcd";

static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static uint8_t s_brightness_pct = 0;  /* current brightness 0-100 */

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
    /* Configure LEDC timer */
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = BL_LEDC_MODE,
        .duty_resolution = BL_LEDC_DUTY_RES,
        .timer_num       = BL_LEDC_TIMER,
        .freq_hz         = BL_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    /* Configure LEDC channel — starts at duty 0 (backlight off) */
    ledc_channel_config_t ledc_channel = {
        .speed_mode = BL_LEDC_MODE,
        .channel    = BL_LEDC_CHANNEL,
        .timer_sel  = BL_LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = EXAMPLE_PIN_NUM_BK_LIGHT,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    /* Enable hardware fade for smooth transitions */
    ESP_ERROR_CHECK(ledc_fade_func_install(0));

    ESP_LOGI(TAG, "Backlight PWM initialized (LEDC timer%d ch%d, %d Hz, 10-bit)",
             BL_LEDC_TIMER, BL_LEDC_CHANNEL, BL_LEDC_FREQ_HZ);
}

esp_err_t st7701_lcd_set_brightness(uint8_t percent)
{
    if (percent > 100) percent = 100;
    s_brightness_pct = percent;
    uint32_t duty = (uint32_t)BL_DUTY_MAX * percent / 100;
    esp_err_t ret = ledc_set_fade_with_time(BL_LEDC_MODE, BL_LEDC_CHANNEL,
                                             duty, 300 /* ms */);
    if (ret == ESP_OK) {
        ret = ledc_fade_start(BL_LEDC_MODE, BL_LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
    }
    ESP_LOGI(TAG, "Backlight brightness: %d%% (duty %lu/%d)", percent, (unsigned long)duty, BL_DUTY_MAX);
    return ret;
}

uint8_t st7701_lcd_get_brightness(void)
{
    return s_brightness_pct;
}

/* ── Auto-dimming ──
 * 3-stage dimming: ACTIVE → DIM1 (50%) → DIM2 (20%) → OFF
 * Default timeouts: 60s → 300s (5min) → 600s (10min)
 */
static int64_t   s_last_activity_us = 0;     /* timestamp of last touch/interaction */
static uint8_t   s_user_brightness  = 100;   /* brightness the user chose (slider value) */
static uint16_t  s_dim1_timeout_s   = 60;    /* seconds until first dim (0=disabled) */
static uint16_t  s_dim2_timeout_s   = 300;   /* seconds until deep dim */
static uint16_t  s_off_timeout_s    = 600;   /* seconds until screen off */
static bool      s_dim_inhibit      = false; /* true = dimming suspended (e.g. during checkup) */

typedef enum { DIM_STATE_ACTIVE, DIM_STATE_DIM1, DIM_STATE_DIM2, DIM_STATE_OFF } dim_state_t;
static dim_state_t s_dim_state = DIM_STATE_ACTIVE;
static volatile bool s_wake_pending = false;

void st7701_lcd_activity_reset(void)
{
    s_last_activity_us = esp_timer_get_time();

    /* Mark wake-up needed — actual LEDC call happens in dimming_tick
     * on the LVGL thread (safe context). Touch callback may run in
     * ISR or timer context where LEDC fade functions are NOT safe. */
    if (s_dim_state != DIM_STATE_ACTIVE) {
        s_dim_state = DIM_STATE_ACTIVE;
        s_wake_pending = true;
    }
}

void st7701_lcd_set_dim_timeout(uint16_t dim1_seconds, uint16_t dim2_seconds, uint16_t off_seconds)
{
    s_dim1_timeout_s = dim1_seconds;
    s_dim2_timeout_s = dim2_seconds;
    s_off_timeout_s  = off_seconds;
    ESP_LOGI(TAG, "Dim timeouts: %ds→50%%, %ds→20%%, %ds→off",
             dim1_seconds, dim2_seconds, off_seconds);
}

void st7701_lcd_set_user_brightness(uint8_t percent)
{
    if (percent > 100) percent = 100;
    if (percent < 10) percent = 10;
    s_user_brightness = percent;
    /* If currently active, apply immediately */
    if (s_dim_state == DIM_STATE_ACTIVE) {
        st7701_lcd_set_brightness(percent);
    }
}

void st7701_lcd_set_dim_inhibit(bool inhibit)
{
    s_dim_inhibit = inhibit;
    ESP_LOGI(TAG, "Dimming %s", inhibit ? "INHIBITED (process running)" : "ENABLED");
    if (inhibit && s_dim_state != DIM_STATE_ACTIVE) {
        /* Restore brightness immediately when entering inhibit mode */
        st7701_lcd_activity_reset();
    }
}

void st7701_lcd_dimming_tick(void)
{
    /* Handle deferred wake-up (touch callback sets s_wake_pending) */
    if (s_wake_pending) {
        s_wake_pending = false;
        uint32_t duty = (uint32_t)BL_DUTY_MAX * s_user_brightness / 100;
        ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, duty);
        ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL);
        s_brightness_pct = s_user_brightness;
        return;
    }

    if (s_dim1_timeout_s == 0) return;  /* auto-dim disabled */
    if (s_dim_inhibit) return;          /* inhibited during checkup */
    if (s_last_activity_us == 0) return; /* not yet initialized */

    int64_t elapsed_us = esp_timer_get_time() - s_last_activity_us;
    int64_t dim1_us = (int64_t)s_dim1_timeout_s * 1000000;
    int64_t dim2_us = (int64_t)s_dim2_timeout_s * 1000000;
    int64_t off_us  = (int64_t)s_off_timeout_s  * 1000000;

    if (elapsed_us >= off_us && s_dim_state != DIM_STATE_OFF) {
        /* 10 min → screen off */
        s_dim_state = DIM_STATE_OFF;
        st7701_lcd_set_brightness(0);
    } else if (elapsed_us >= dim2_us && s_dim_state < DIM_STATE_DIM2) {
        /* 5 min → 20% of user brightness */
        s_dim_state = DIM_STATE_DIM2;
        uint8_t dim_val = s_user_brightness / 5;
        if (dim_val < 5) dim_val = 5;
        st7701_lcd_set_brightness(dim_val);
    } else if (elapsed_us >= dim1_us && s_dim_state == DIM_STATE_ACTIVE) {
        /* 1 min → 50% of user brightness */
        s_dim_state = DIM_STATE_DIM1;
        uint8_t dim_val = s_user_brightness / 2;
        if (dim_val < 5) dim_val = 5;
        st7701_lcd_set_brightness(dim_val);
    }
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

    // Turn on backlight at full brightness via PWM
    st7701_lcd_set_brightness(100);

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
