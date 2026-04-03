/**
 * @file board.h
 * @brief Board selection and shared board-wide constants.
 *
 * The active board is chosen at compile time via -DBOARD_xxx.
 * The only supported hardware board is the JC4880P433 (ESP32-P4).
 * The simulator uses BOARD_SIMULATOR for SDL2-based desktop builds.
 */

#ifndef BOARD_H
#define BOARD_H

#if defined(BOARD_SIMULATOR)
    #include "board_simulator.h"
#elif defined(BOARD_JC4880P433)
    #include "board_jc4880p433.h"
#else
    /* Default to P4 board for ESP-IDF builds */
    #define BOARD_JC4880P433    1
    #include "board_jc4880p433.h"
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

#endif /* BOARD_H */
