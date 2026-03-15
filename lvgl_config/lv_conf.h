/**
 * lv_conf.h — LVGL configuration for FilMachine PC Simulator
 * Matches the ESP32-S3 project sdkconfig as closely as possible
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16          /* Matches CONFIG_LV_COLOR_DEPTH_16 */

/*====================
   MEMORY SETTINGS
 *====================*/
#define LV_USE_BUILTIN_MALLOC 1
#define LV_USE_BUILTIN_STRING 1
#define LV_USE_BUILTIN_SPRINTF 1
#define LV_MEM_SIZE (256 * 1024)   /* 256KB — PC has plenty of RAM */

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DEF_REFR_PERIOD 33      /* Matches CONFIG_LV_DEF_REFR_PERIOD */
#define LV_DPI_DEF 130             /* Matches CONFIG_LV_DPI_DEF */
#define LV_TICK_CUSTOM 0           /* We call lv_tick_inc() manually */

/*====================
   OPERATING SYSTEM
 *====================*/
#define LV_USE_OS LV_OS_NONE       /* Matches CONFIG_LV_OS_NONE */

/*====================
   DISPLAY
 *====================*/
#define LV_USE_DRAW_SW 1

/*====================
   RENDERING
 *====================*/
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_DRAW_BUF_ALIGN 4
#define LV_DRAW_SW_COMPLEX 1

/*====================
   LOGGING
 *====================*/
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_USER
#define LV_LOG_PRINTF 1

/*====================
   FONTS
 *====================*/
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*====================
   THEMES
 *====================*/
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1     /* 0: Light mode; 1: Dark mode */

/*====================
   WIDGETS
 *====================*/
#define LV_USE_ANIMIMG     1
#define LV_USE_ARC         1
#define LV_USE_BAR         1
#define LV_USE_BUTTON      1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR    1
#define LV_USE_CANVAS      1
#define LV_USE_CHART       1
#define LV_USE_CHECKBOX    1
#define LV_USE_DROPDOWN    1
#define LV_USE_IMAGE       1
#define LV_USE_IMAGEBUTTON 1
#define LV_USE_KEYBOARD    1
#define LV_USE_LABEL       1
#define LV_USE_LED         1
#define LV_USE_LINE        1
#define LV_USE_LIST        1
#define LV_USE_MENU        1
#define LV_USE_MSGBOX      1
#define LV_USE_OBJID       0
#define LV_USE_ROLLER      1
#define LV_USE_SCALE       1
#define LV_USE_SLIDER      1
#define LV_USE_SPAN        1
#define LV_USE_SPINBOX     1
#define LV_USE_SPINNER     1
#define LV_USE_SWITCH      1
#define LV_USE_TABLE       1
#define LV_USE_TABVIEW     1
#define LV_USE_TEXTAREA    1
#define LV_USE_TILEVIEW    1
#define LV_USE_WIN         1

/*====================
   SDL DRIVER
 *====================*/
#define LV_USE_SDL 1
#if LV_USE_SDL
    #define LV_SDL_WINDOW_TITLE  "FilMachine Simulator"
    #define LV_SDL_INCLUDE_PATH  <SDL2/SDL.h>
    #define LV_SDL_RENDER_MODE   LV_DISPLAY_RENDER_MODE_DIRECT
    #define LV_SDL_BUF_COUNT     1
    #define LV_SDL_FULLSCREEN    0
    #define LV_SDL_DIRECT_EXIT   1
    #define LV_SDL_MOUSEWHEEL_MODE LV_SDL_MOUSEWHEEL_MODE_ENCODER
#endif

#endif /* LV_CONF_H */
