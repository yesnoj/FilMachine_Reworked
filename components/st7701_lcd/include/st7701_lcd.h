/*
 * ST7701 LCD component - public header
 * Ported from Arduino wrapper to ESP-IDF C API
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "esp_lcd_mipi_dsi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_lcd_dsi_bus_handle_t    mipi_dsi_bus;
    esp_lcd_panel_io_handle_t   io;
    esp_lcd_panel_handle_t      panel;
    esp_lcd_panel_handle_t      control;
} bsp_lcd_handles_t;

/**
 * @brief Initialize the LCD (MIPI DSI, backlight, LDO, ST7701 panel)
 */
esp_err_t st7701_lcd_init(void);

/**
 * @brief Draw a bitmap to the LCD
 */
esp_err_t st7701_lcd_draw_bitmap(uint16_t x_start, uint16_t y_start,
                                  uint16_t x_end, uint16_t y_end,
                                  const uint16_t *color_data);

/**
 * @brief Draw a 16-bit RGB bitmap at (x,y) with dimensions w x h
 */
esp_err_t st7701_lcd_draw_rgb_bitmap(uint16_t x, uint16_t y,
                                      uint16_t w, uint16_t h,
                                      const uint16_t *color_data);

/**
 * @brief Fill the screen with a solid color
 */
esp_err_t st7701_lcd_fill_screen(uint16_t color);

/**
 * @brief Get the LCD panel handles
 */
void st7701_lcd_get_handles(bsp_lcd_handles_t *ret_handles);

/**
 * @brief Get LCD width
 */
uint16_t st7701_lcd_width(void);

/**
 * @brief Get LCD height
 */
uint16_t st7701_lcd_height(void);

/**
 * @brief Write a bitmap directly to the DPI framebuffer via CPU memcpy.
 *        Bypasses the async DMA2D pipeline, so it never contends with
 *        a previous draw_bitmap call.
 */
esp_err_t st7701_lcd_draw_to_fb(uint16_t x, uint16_t y,
                                uint16_t w, uint16_t h,
                                const uint16_t *data);

/**
 * @brief Write a rotated bitmap to the DPI framebuffer.
 *        Accepts pixel data in LOGICAL landscape coordinates (800×480)
 *        and rotates each pixel into the PHYSICAL portrait framebuffer (480×800).
 *        Rotation mapping: phys_x = logical_y, phys_y = (799 - logical_x).
 *        Only the dirty rectangle is processed — ideal for partial rendering.
 *
 * @param lx  Logical X origin (0..799)
 * @param ly  Logical Y origin (0..479)
 * @param lw  Logical width of the dirty rectangle
 * @param lh  Logical height of the dirty rectangle
 * @param data  Pixel data in row-major logical order (lw * lh pixels, RGB565)
 */
esp_err_t st7701_lcd_draw_to_fb_rotated(uint16_t lx, uint16_t ly,
                                         uint16_t lw, uint16_t lh,
                                         const uint16_t *data);

#ifdef __cplusplus
}
#endif
