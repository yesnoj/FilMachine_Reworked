/**
 * @file board.h
 * @brief Board selection and shared board-wide constants.
 *
 * The active board is chosen at compile time via -DBOARD_xxx.
 * Each board header defines hardware constants such as pins, display/touch
 * driver type and resolution.
 */

#ifndef BOARD_H
#define BOARD_H

#if defined(BOARD_JC4827W543)
    #include "board_jc4827w543.h"
#elif defined(BOARD_MAKERFABS_S3)
    #include "board_makerfabs_s3.h"
#elif defined(BOARD_SIMULATOR)
    #include "board_simulator.h"
#else
    #warning "No BOARD_xxx defined — defaulting to BOARD_MAKERFABS_S3"
    #include "board_makerfabs_s3.h"
#endif

#ifndef LCD_H_RES
    #error "Board header must define LCD_H_RES"
#endif
#ifndef LCD_V_RES
    #error "Board header must define LCD_V_RES"
#endif
#ifndef BOARD_NAME
    #error "Board header must define BOARD_NAME"
#endif

/* Shared derived constants */
#define BYTES_PER_PIXEL         2   /* RGB565 = 16 bit */
#define LVGL_BUF_SIZE           (LCD_H_RES * (LCD_V_RES / 10) * BYTES_PER_PIXEL)

/* Optional generic scaling helper retained for non-profiled values. */
#define LAYOUT_REF_V_RES        320
#define SCALE_Y(v)              ((int32_t)(v) * LCD_V_RES / LAYOUT_REF_V_RES)

#endif /* BOARD_H */
