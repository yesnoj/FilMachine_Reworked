/*
 * FilMachine_support.c
 *
 *  Created on: 2 May 2025
 *      Author: PeterB
 */
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "driver/sdmmc_host.h"
#if defined(BOARD_JC4880P433)
    #include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif
#include "esp_err.h"
#include "esp_log.h"
#include "ff.h"
#include "sdmmc_cmd.h"
#if defined(BOARD_JC4880P433)
    #include "driver/i2c_master.h"
#else
    #include "driver/i2c.h"
#endif
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_vfs_fat.h"
#include "esp_heap_caps.h"

#include "FilMachine.h"
#include "ws_server.h"
#include "mcp23017.h"
#include "ds18b20.h"
#include "pca9685.h"

#define LESS_THAN_10 1
#define GREATER_THAN_9 2
#define GREATER_THAN_99 3

extern struct gui_components		gui;
extern struct sys_components		sys;
extern uint8_t						initErrors;

static const char			*TAG = "Accessory"; /* ESP Debug Message Tag */
static mcp23017_t           mcp;                /* MCP23017 I/O expander handle */
static ds18b20_bus_t        ds_bus;             /* DS18B20 shared OneWire bus (both sensors) */
static pca9685_t            pca_pwm;            /* PCA9685 I2C PWM controller (pump speed) */

static void process_list_set_time_label(processNode *process);

/* ═══════════════════════════════════════════════
 * Utility Helpers
 * ═══════════════════════════════════════════════ */

/** Round a value to the nearest step (e.g., roundToStep(77, 10) → 80) */
int32_t roundToStep(int32_t value, int32_t step) {
    int32_t remainder = value % step;
    if (remainder == 0) return value;
    return (remainder > step / 2) ? value + (step - remainder) : value - remainder;
}

/** Safely delete an LVGL timer and null-out the pointer */
void safeTimerDelete(lv_timer_t **timer) {
    if (timer != NULL && *timer != NULL) {
        lv_timer_delete(*timer);
        *timer = NULL;
    }
}

/* LEDC PWM for motor ENA pin */
#define MOTOR_LEDC_TIMER      LEDC_TIMER_0
#define MOTOR_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define MOTOR_LEDC_CHANNEL    LEDC_CHANNEL_0
#define MOTOR_LEDC_FREQ_HZ    5000
#define MOTOR_LEDC_RESOLUTION  LEDC_TIMER_8_BIT

static void motor_ledc_set_duty(uint8_t duty)
{
    ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CHANNEL, duty);
    ledc_update_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CHANNEL);
}

/**
 * Set pump speed via PCA9685 I2C PWM controller.
 * @param duty  0-255 (same 8-bit range as old LEDC approach)
 */
static void __attribute__((unused)) pump_set_duty(uint8_t duty)
{
    if (!pca_pwm.initialized) return;
    uint16_t duty12 = pca9685_duty_from_8bit(duty);
    pca9685_set_duty(&pca_pwm, PUMP_PCA9685_CHANNEL, duty12);
}

void rebootBoard(void) {
	esp_restart();
};

/* Put a system request in the queue returns true if successful false if queue is full */
uint8_t qSysAction( uint16_t msg ) {
  
  return xQueueSend( sys.sysActionQ, &msg, 0 );
}

/* Put a motor request in the queue returns true if successful false if queue is full */
uint8_t qMotorAction( uint16_t msg ) {
  
  return xQueueSend( sys.motorActionQ, &msg, 0 );
}


void event_cb(lv_event_t * e) {
	
  lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t * objCont = (lv_obj_t *)lv_obj_get_parent(obj);
  lv_obj_t * mboxCont = (lv_obj_t *)lv_obj_get_parent(objCont);

    //const char *label = lv_msgbox_get_active_btn(mbox);
    //LV_UNUSED(label);
    lv_msgbox_close(mboxCont);
    //lv_obj_delete(mboxCont);
    //LV_LOG_USER("Button %s clicked", lv_label_get(label));
}

lv_obj_t * create_radiobutton(lv_obj_t * mBoxParent, const char * txt, const int32_t x, const int32_t y, const int32_t size, const lv_font_t * font, const lv_color_t borderColor, const lv_color_t bgColor) {
	
    lv_obj_t * obj = (lv_obj_t *)lv_checkbox_create(mBoxParent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
    lv_checkbox_set_text(obj, txt);
    lv_obj_align(obj, LV_ALIGN_RIGHT_MID, x, y);


    int32_t font_h = lv_font_get_line_height(font);
    int32_t pad = (size - font_h) / 2;
    lv_obj_set_style_pad_left(obj, pad, LV_PART_INDICATOR);
    lv_obj_set_style_pad_right(obj, pad, LV_PART_INDICATOR);
    lv_obj_set_style_pad_top(obj, pad, LV_PART_INDICATOR);
    lv_obj_set_style_pad_bottom(obj, pad, LV_PART_INDICATOR);

  lv_obj_set_style_border_color(obj, borderColor, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(obj, borderColor, LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(obj, bgColor, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_update_layout(obj);
    return obj;
}

void createMessageBox(lv_obj_t * messageBox, char *title, char *text, char *button1Text, char *button2Text) {
    
    messageBox = lv_msgbox_create(NULL);
    lv_msgbox_add_title(messageBox, title);
    lv_obj_set_style_bg_color(messageBox, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
    //lv_msgbox_add_header_button(messageBox, NULL);
    lv_msgbox_add_text(messageBox, text); 
    

    lv_obj_t * btn;
    btn = lv_msgbox_add_footer_button(messageBox, button1Text);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    btn = lv_msgbox_add_footer_button(messageBox, button2Text);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    
}

void event_checkbox_handler(lv_event_t * e) {
	
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        LV_UNUSED(obj);
        const char * txt = lv_checkbox_get_text(obj);
        const char * state = lv_obj_get_state(obj) & LV_STATE_CHECKED ? "Checked" : "Unchecked";
        LV_UNUSED(txt);
        LV_UNUSED(state);
        LV_LOG_USER("%s: %s", txt, state);
    }
}

/*--------------------------------------------------------------
 *  Keyboard context — captured once on CLICKED, reused by
 *  DEFOCUSED / CANCEL / READY so we never re-derive from globals.
 *-------------------------------------------------------------*/
static sKeyboardOwnerContext kbCtx = {0};

static void kb_ctx_clear(void) {
    memset(&kbCtx, 0, sizeof(kbCtx));
}

void kb_ctx_set(const sKeyboardOwnerContext *ctx) {
    kbCtx = *ctx;
}

static const char *kb_owner_name(kbOwnerType owner) __attribute__((unused));
static const char *kb_owner_name(kbOwnerType owner) {
    switch(owner) {
        case KB_OWNER_FILTER: return "filter";
        case KB_OWNER_PROCESS: return "process";
        case KB_OWNER_STEP: return "step";
        case KB_OWNER_SETTINGS: return "settings";
        default: return "unknown";
    }
}

static void kb_commit_text(const char *kbText) {
    if(kbText == NULL || kbCtx.owner == KB_OWNER_NONE) {
        return;
    }

    if(kbCtx.textArea != NULL) {
        lv_textarea_set_text(kbCtx.textArea, kbText);
    }

    switch(kbCtx.owner) {
        case KB_OWNER_FILTER:
            LV_LOG_USER("Press ok from filterPopup");
            snprintf(gui.element.filterPopup.filterName,
                     sizeof(gui.element.filterPopup.filterName), "%s", kbText);
            break;

        case KB_OWNER_PROCESS:
            if(kbCtx.ownerData != NULL) {
                sProcessDetail *pd = (sProcessDetail *)kbCtx.ownerData;
                LV_LOG_USER("Press ok from processDetailNameTextArea");
                if (strcmp(pd->data.processNameString, kbText) != 0) {
                    snprintf(pd->data.processNameString, sizeof(pd->data.processNameString), "%s", kbText);
                    pd->data.somethingChanged = true;
                }
            }
            break;

        case KB_OWNER_STEP:
            if(kbCtx.ownerData != NULL) {
                sStepDetail *sd = (sStepDetail *)kbCtx.ownerData;
                LV_LOG_USER("Press ok from stepDetailNameTextArea");
                if (strcmp(sd->data.stepNameString, kbText) != 0) {
                    snprintf(sd->data.stepNameString, sizeof(sd->data.stepNameString), "%s", kbText);
                    sd->data.somethingChanged = true;
                }
            }
            break;

        case KB_OWNER_SETTINGS:
            /* Wi-Fi password entered from popup */
            LV_LOG_USER("Wi-Fi password entered from keyboard");
            snprintf(gui.element.wifiPopup.pendingPassword,
                     sizeof(gui.element.wifiPopup.pendingPassword), "%s", kbText);
            break;

        default:
            return;
    }

    if(kbCtx.saveButton != NULL) {
        lv_obj_send_event(kbCtx.saveButton, LV_EVENT_REFRESH, NULL);
    }
}

void event_keyboard(lv_event_t* e) {

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);

  if(code == LV_EVENT_VALUE_CHANGED) {
      uint32_t btn_id = lv_buttonmatrix_get_selected_button(gui.element.keyboardPopup.keyboard);
      if(btn_id != LV_BUTTONMATRIX_BUTTON_NONE) {
          const char *txt = lv_buttonmatrix_get_button_text(gui.element.keyboardPopup.keyboard, btn_id);
          if(txt != NULL && strcmp(txt, "123") == 0) {
              for(int i = 0; i < 3; i++) lv_textarea_delete_char(gui.element.keyboardPopup.keyboardTextArea);
              lv_keyboard_set_mode(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_2);
              LV_LOG_USER("Keyboard switched to numbers/symbols");
              return;
          }
          if(txt != NULL && strcmp(txt, "Abc") == 0) {
              for(int i = 0; i < 3; i++) lv_textarea_delete_char(gui.element.keyboardPopup.keyboardTextArea);
              lv_keyboard_set_mode(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_3);
              LV_LOG_USER("Keyboard switched to lowercase letters");
              return;
          }
          if(txt != NULL && strcmp(txt, LV_SYMBOL_UP) == 0) {
              /* Toggle uppercase ↔ lowercase */
              lv_keyboard_mode_t cur = lv_keyboard_get_mode(gui.element.keyboardPopup.keyboard);
              if(cur == LV_KEYBOARD_MODE_USER_3) {
                  lv_keyboard_set_mode(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_1);
                  LV_LOG_USER("Keyboard switched to UPPERCASE");
              } else {
                  lv_keyboard_set_mode(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_3);
                  LV_LOG_USER("Keyboard switched to lowercase");
              }
              return;
          }
      }
      return;
  }

  if(code == LV_EVENT_CLICKED) {
      sKeyboardOwnerContext *ctx = (sKeyboardOwnerContext *)lv_event_get_user_data(e);
      if(ctx != NULL && ctx->textArea == obj) {
          LV_LOG_USER("LV_EVENT_FOCUSED on keyboard-managed textarea");
          kbCtx = *ctx;
          /* Set max length for the keyboard textarea (0 = use default) */
          uint32_t maxLen = kbCtx.maxLength > 0 ? kbCtx.maxLength : MAX_PROC_NAME_LEN;
          lv_textarea_set_max_length(gui.element.keyboardPopup.keyboardTextArea, maxLen);
          showKeyboard(kbCtx.parentScreen, kbCtx.textArea);
      }
      return;
  }

  if(code == LV_EVENT_DEFOCUSED) {
      if(kbCtx.owner != KB_OWNER_NONE && obj == kbCtx.textArea) {
          LV_LOG_USER("LV_EVENT_DEFOCUSED on %s textarea", kb_owner_name(kbCtx.owner));
          hideKeyboard(kbCtx.parentScreen);
      }
      return;
  }

  if(code == LV_EVENT_CANCEL) {
      if(kbCtx.owner != KB_OWNER_NONE) {
          LV_LOG_USER("LV_EVENT_CANCEL on %s textarea", kb_owner_name(kbCtx.owner));
          hideKeyboard(kbCtx.parentScreen);
      }
      return;
  }

  if(code == LV_EVENT_READY) {
      LV_LOG_USER("LV_EVENT_READY PRESSED");
      kb_commit_text(lv_textarea_get_text(gui.element.keyboardPopup.keyboardTextArea));
      hideKeyboard(kbCtx.parentScreen);
  }
}



void create_keyboard() {
		
    gui.element.keyboardPopup.keyBoardParent = lv_obj_class_create_obj(&lv_msgbox_backdrop_class, lv_layer_top());
    lv_obj_class_init_obj(gui.element.keyboardPopup.keyBoardParent);
    lv_obj_remove_flag(gui.element.keyboardPopup.keyBoardParent, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(gui.element.keyboardPopup.keyBoardParent, LV_PCT(100), LV_PCT(100));

    gui.element.keyboardPopup.keyboard = lv_keyboard_create(gui.element.keyboardPopup.keyBoardParent);
    lv_obj_add_event_cb(gui.element.keyboardPopup.keyboard, event_keyboard, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(gui.element.keyboardPopup.keyboard, event_keyboard, LV_EVENT_DEFOCUSED, NULL);
    lv_obj_add_event_cb(gui.element.keyboardPopup.keyboard, event_keyboard, LV_EVENT_CANCEL, NULL);
    lv_obj_add_event_cb(gui.element.keyboardPopup.keyboard, event_keyboard, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(gui.element.keyboardPopup.keyboard, event_keyboard, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_flag(gui.element.keyboardPopup.keyboard, LV_OBJ_FLAG_EVENT_BUBBLE);


    /* ── USER_1 : letter keyboard ── */
    /* ── USER_1 : UPPERCASE letters (default) ── */
    static const char * kb_map[] = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", " ", "\n",
                                    "A", "S", "D", "F", "G", "H", "J", "K", "L",  " ", "\n",
                                    LV_SYMBOL_UP," ", "Z", "X", "C", "V", "B", "N", "M"," "," ","\n",
                                    LV_SYMBOL_CLOSE, LV_SYMBOL_BACKSPACE,  " ", "123", LV_SYMBOL_OK, NULL
                                   };

    static const lv_buttonmatrix_ctrl_t kb_ctrl[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_BUTTONMATRIX_CTRL_HIDDEN,
                                                     4, 4, 4, 4, 4, 4, 4, 4, 4, LV_BUTTONMATRIX_CTRL_HIDDEN,
                                                     2, LV_BUTTONMATRIX_CTRL_HIDDEN, 4, 4, 4, 4, 4, 4, 4, LV_BUTTONMATRIX_CTRL_HIDDEN, LV_BUTTONMATRIX_CTRL_HIDDEN,
                                                     2, 2, 4, 2, 2
                                                    };

    /* ── USER_3 : lowercase letters ── */
    static const char * kb_map_lower[] = {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", " ", "\n",
                                          "a", "s", "d", "f", "g", "h", "j", "k", "l",  " ", "\n",
                                          LV_SYMBOL_UP," ", "z", "x", "c", "v", "b", "n", "m"," "," ","\n",
                                          LV_SYMBOL_CLOSE, LV_SYMBOL_BACKSPACE,  " ", "123", LV_SYMBOL_OK, NULL
                                         };

    static const lv_buttonmatrix_ctrl_t kb_ctrl_lower[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_BUTTONMATRIX_CTRL_HIDDEN,
                                                           4, 4, 4, 4, 4, 4, 4, 4, 4, LV_BUTTONMATRIX_CTRL_HIDDEN,
                                                           2, LV_BUTTONMATRIX_CTRL_HIDDEN, 4, 4, 4, 4, 4, 4, 4, LV_BUTTONMATRIX_CTRL_HIDDEN, LV_BUTTONMATRIX_CTRL_HIDDEN,
                                                           2, 2, 4, 2, 2
                                                          };

    /* ── USER_2 : number / symbol keyboard ── */
    static const char * kb_map_num[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", " ", "\n",
                                        "-", "/", ":", ";", "(", ")", ".", ",", "@", " ", "\n",
                                        " ", " ", "#", "!", "?", "'", "+", "=", "%", " ", " ", "\n",
                                        LV_SYMBOL_CLOSE, LV_SYMBOL_BACKSPACE, " ", "Abc", LV_SYMBOL_OK, NULL
                                       };

    static const lv_buttonmatrix_ctrl_t kb_ctrl_num[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_BUTTONMATRIX_CTRL_HIDDEN,
                                                         4, 4, 4, 4, 4, 4, 4, 4, 4, LV_BUTTONMATRIX_CTRL_HIDDEN,
                                                         LV_BUTTONMATRIX_CTRL_HIDDEN, LV_BUTTONMATRIX_CTRL_HIDDEN, 4, 4, 4, 4, 4, 4, 4, LV_BUTTONMATRIX_CTRL_HIDDEN, LV_BUTTONMATRIX_CTRL_HIDDEN,
                                                         2, 2, 4, 2, 2
                                                        };

    lv_keyboard_set_map(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
    lv_keyboard_set_map(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_2, kb_map_num, kb_ctrl_num);
    lv_keyboard_set_map(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_3, kb_map_lower, kb_ctrl_lower);
    lv_keyboard_set_mode(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_3);
    
    const ui_keyboard_layout_t *kb = &ui_get_profile()->keyboard;
    gui.element.keyboardPopup.keyboardTextArea = lv_textarea_create(gui.element.keyboardPopup.keyBoardParent);
    lv_obj_align(gui.element.keyboardPopup.keyboardTextArea, LV_ALIGN_TOP_MID, 0, kb->textarea_y);
    lv_textarea_set_placeholder_text(gui.element.keyboardPopup.keyboardTextArea, keyboard_placeholder_text);
    lv_obj_set_size(gui.element.keyboardPopup.keyboardTextArea, lv_pct(90), kb->textarea_h);
    lv_obj_add_state(gui.element.keyboardPopup.keyboardTextArea, LV_STATE_FOCUSED);
    lv_textarea_set_max_length(gui.element.keyboardPopup.keyboardTextArea, MAX_PROC_NAME_LEN);
    lv_keyboard_set_textarea(gui.element.keyboardPopup.keyboard, gui.element.keyboardPopup.keyboardTextArea);

    lv_obj_set_style_text_font(gui.element.keyboardPopup.keyboard, kb->keyboard_font, 0);
    lv_obj_set_style_text_font(gui.element.keyboardPopup.keyboardTextArea, kb->textarea_font, 0);
    
    lv_obj_add_flag(gui.element.keyboardPopup.keyBoardParent, LV_OBJ_FLAG_HIDDEN);
}


lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt) {
	
    lv_obj_t * obj =  (lv_obj_t *)lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * label = NULL;

    if(icon) {
        img = lv_image_create(obj);
        lv_image_set_src(img, icon);
        //lv_obj_set_size(img, 46, 46);
    }

    if(txt) {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
        lv_obj_align(label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    }


    return obj;
}


lv_obj_t * create_slider(lv_obj_t * parent, const char * icon, const char * txt, int32_t min, int32_t max, int32_t val) {
	
    lv_obj_t * obj =  (lv_obj_t *) create_text(parent, icon, txt);

    lv_obj_t * slider =  (lv_obj_t *) lv_slider_create(obj);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);

    if(icon == NULL) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }

    return obj;
}

lv_obj_t * create_switch(lv_obj_t * parent, const char * icon, const char * txt, bool chk) {
	
    lv_obj_t * obj =  (lv_obj_t *) create_text(parent, icon, txt);

    lv_obj_t * sw = (lv_obj_t *)lv_switch_create(obj);
    lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : 0);

    return obj;
}


void createQuestionMark(lv_obj_t * parent, lv_obj_t * element, lv_event_cb_t e, const int32_t x, const int32_t y)
{
    lv_obj_t *questionMark = lv_label_create(parent);

    #if (LCD_H_RES == 480) && (LCD_V_RES == 320)
        const lv_font_t *qm_font = &FilMachineFontIcons_15;
    #else
        const lv_font_t *qm_font = &FilMachineFontIcons_30;
    #endif

    int32_t sz = (int32_t)((lv_font_get_line_height(qm_font) * 3) / 2);

    lv_obj_set_size(questionMark, sz, sz);
    lv_label_set_text(questionMark, questionMark_icon);
    lv_obj_add_event_cb(questionMark, e, LV_EVENT_CLICKED, element);
    lv_obj_set_style_text_font(questionMark, qm_font, 0);
    lv_obj_align_to(questionMark, element, LV_ALIGN_OUT_RIGHT_MID, x, y);
    lv_obj_add_flag(questionMark, LV_OBJ_FLAG_CLICKABLE);
}

void createPopupBackdrop(lv_obj_t **parent, lv_obj_t **container, int32_t width, int32_t height) {
    *parent = lv_obj_class_create_obj(&lv_msgbox_backdrop_class, lv_layer_top());
    lv_obj_class_init_obj(*parent);
    lv_obj_remove_flag(*parent, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(*parent, LV_PCT(100), LV_PCT(100));

    *container = lv_obj_create(*parent);
    lv_obj_align(*container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(*container, width, height);
    lv_obj_remove_flag(*container, LV_OBJ_FLAG_SCROLLABLE);
}

void initTitleLineStyle(lv_style_t *style, uint32_t color) {
    lv_style_init(style);
    lv_style_set_line_width(style, ui_get_profile()->title_line_width);
    lv_style_set_line_color(style, lv_color_hex(color));
    lv_style_set_line_rounded(style, true);
}

int32_t convertCelsiusToFahrenheit( int32_t tempC ) {
  return (int32_t)((float)tempC * 1.8 + 32 + 0.5); // Add 0.5 for rounding
}

uint32_t calc_buf_len( uint32_t maxVal, uint8_t extra_len ) {
    uint8_t caseVal = LESS_THAN_10;
    uint32_t len = 0, remainder = maxVal;
    
    if( maxVal > 999 ) return 0; // Limit to 999 although this is alot!
    if( maxVal > 99 ) caseVal = GREATER_THAN_99;
    else if( maxVal > 9 ) caseVal = GREATER_THAN_9;
    switch( caseVal ) {
        case GREATER_THAN_99:
          len += ( (4 + extra_len) * (remainder - 99)); // Add storage length for the 3 digit numbers + a '\n' charater
          remainder -= (remainder - 99); // Deduct what we have accounted for so far
          /* FALLTHRU */
        case GREATER_THAN_9: // Fall through case (no break; required!)
          len += ( (3 + extra_len) * (remainder - 9)); // Add the storage length for the 2 digit numbers + a '\n' character
          remainder -= (remainder - 9); // Deduct what we have accounted for so far
          /* FALLTHRU */
        case LESS_THAN_10:
        default:
          len +=( (2 + extra_len) * remainder); // Add strorage length for the 1 digit numbers + a '\n'
          remainder -= remainder; // Deduct what we have finally accounted for... Remainder should now be zero!
        break;
        }
    if( !remainder ) {
        return len; // All well return length
    } else {
        return 0; // Something went wrong, so we return 0 length for buffer to show error!
    }
}

char *createRollerValues(uint32_t minVal, uint32_t maxVal, const char* extra_str, bool isFahrenheit) {
    // Calculate the required buffer length
    uint32_t buf_len = 0;
    for (uint32_t i = minVal; i <= maxVal; i++) {
        int value = isFahrenheit ? convertCelsiusToFahrenheit(i) : i;
        buf_len += snprintf(NULL, 0, "%s%d\n", extra_str, value);
    }
    buf_len -= 1; // Remove the last newline to avoid empty space

    // Allocate memory for the buffer
    char *buf = (char *)malloc(buf_len + 1); // +1 for null terminator
    if (!buf) {
        return NULL; // Handle malloc failure
    }

    // Popola il buffer con i valori
    uint32_t buf_ptr = 0;
    for (uint32_t i = minVal; i <= maxVal; i++) {
        int value = isFahrenheit ? convertCelsiusToFahrenheit( i ) : i;
        if (i == maxVal) {
            buf_ptr += snprintf(&buf[buf_ptr], (buf_len - buf_ptr + 1), "%s%d", extra_str, value);
        } else {
            buf_ptr += snprintf(&buf[buf_ptr], (buf_len - buf_ptr + 1), "%s%d\n", extra_str, value);
        }
    }
    return buf;
}
void myLongEvent(lv_event_t * e, uint32_t howLongInMs) {
	
    lv_event_code_t code = lv_event_get_code(e);
    static uint32_t t;
    if(code == LV_EVENT_PRESSED) {
        t = lv_tick_get();
    } 
    else if(code == LV_EVENT_PRESSING) {
        if(lv_tick_elaps(t) > howLongInMs) {
            /*Do something*/
        }
    }
}

void* allocateAndInitializeNode(NodeType_t type) {
    void *node = NULL;

    // Initialize and allocate according to the node type
    switch (type) {
        case STEP_NODE:
            node = heap_caps_malloc(sizeof(stepNode), MALLOC_CAP_SPIRAM);
            if (node != NULL) {
                memset(node, 0, sizeof(stepNode));
                stepNode* step = (stepNode*)node;
                step->step.stepDetails = (sStepDetail *)heap_caps_malloc(sizeof(sStepDetail), MALLOC_CAP_SPIRAM);
                if (step->step.stepDetails == NULL) {
                    // Handle memory allocation failure
                    free(step);
                    return NULL;
                }
                memset(step->step.stepDetails, 0, sizeof(sStepDetail));
            } else {
                // Handle memory allocation failure
                return NULL;
            }
            break;

        case PROCESS_NODE:
            node = heap_caps_malloc(sizeof(processNode), MALLOC_CAP_SPIRAM);
            if (node != NULL) {
                memset(node, 0, sizeof(processNode));
                processNode* process = (processNode*)node;
                process->process.processDetails = (sProcessDetail *)heap_caps_malloc(sizeof(sProcessDetail), MALLOC_CAP_SPIRAM);
                if (process->process.processDetails == NULL) {
                    // Handle memory allocation failure
                    free(process);
                    return NULL;
                }
                memset(process->process.processDetails, 0, sizeof(sProcessDetail));

                process->process.processDetails->checkup = (sCheckup *)heap_caps_malloc(sizeof(sCheckup), MALLOC_CAP_SPIRAM);
                if (process->process.processDetails->checkup == NULL) {
                    // Handle memory allocation failure
                    free(process->process.processDetails);
                    free(process);
                    return NULL;
                }
                memset(process->process.processDetails->checkup, 0, sizeof(sCheckup));
                process->process.processDetails->checkup->data.tankSize = gui.page.settings.settingsParams.tankSize > 0 ? gui.page.settings.settingsParams.tankSize : 2;
            } else {
                // Handle memory allocation failure
                return NULL;
            }
            break;

        default:
            // Handle invalid node type
            return NULL;
    }

    return node;
}

void* isNodeInList(void* list, void* node, NodeType_t type) {
    if (list == NULL || node == NULL) {
        return NULL;
    }

    switch (type) {
        case STEP_NODE: {
            stepList* sList = (stepList*)list;
            stepNode* current = sList->start;

            while (current != NULL) {
                if (current == (stepNode*)node) {
                    return (void*)current;  // Node found
                }
                current = current->next;
            }
            break;
        }

        case PROCESS_NODE: {
            processList* pList = (processList*)list;
            processNode* current = pList->start;

            while (current != NULL) {
                if (current == (processNode*)node) {
                    return (void*)current;  // Node found
                }
                current = current->next;
            }
            break;
        }
    }

    // Node not found
    return NULL;
}



void initGlobals( void ) {
  // Initialise the main GUI & sys structures to zero
  memset(&gui, 0, sizeof(gui));
  memset(&sys, 0, sizeof(sys));
  
  //gui.page.processes.processElementsList.start = NULL;  // Not rquired memset above does this only need to set non-zero values ( NULL is also zero )
  //gui.page.processes.processElementsList.end   = NULL;
  //gui.page.processes.processElementsList.size  = 0;

  // We only need to initialise the non-zero values
  const ui_profile_t *ui = ui_get_profile();
  gui.element.cleanPopup.titleLinePoints[1].x = ui->popups.clean_title_line_w;
    gui.element.drainPopup.titleLinePoints[1].x = ui->popups.drain_title_line_w;
    gui.element.selfcheckPopup.titleLinePoints[1].x = ui->popups.selfcheck_title_line_w;
    gui.element.filterPopup.titleLinePoints[1].x = ui->popups.filter_title_line_w;
    gui.element.rollerPopup.titleLinePoints[1].x = ui->popups.roller_title_line_w;
    gui.element.messagePopup.titleLinePoints[1].x = ui->popups.message_title_line_w;

    gui.page.processes.titleLinePoints[1].x = ui->common.title_line_w;
    gui.page.settings.titleLinePoints[1].x = ui->common.title_line_w;
    gui.page.tools.titleLinePoints[1].x = ui->common.title_line_w;
  
  /* Sensible defaults for settings (overridden by config file if present) */
  gui.page.settings.settingsParams.tankSize = 2;           /* Medium */
  gui.page.settings.settingsParams.pumpSpeed = 30;
  gui.page.settings.settingsParams.chemContainerMl = 500;
  gui.page.settings.settingsParams.wbContainerMl = 2000;
  gui.page.settings.settingsParams.chemistryVolume = 2;    /* High */
  gui.page.settings.settingsParams.filmRotationSpeedSetpoint = 50;
  gui.page.settings.settingsParams.rotationIntervalSetpoint = 10;
  gui.page.settings.settingsParams.calibratedTemp = 20;
  gui.page.settings.settingsParams.multiRinseTime = 60;
  gui.page.settings.settingsParams.drainFillOverlapSetpoint = 100;

  gui.element.rollerPopup.tempCelsiusOptions = createRollerValues(TEMP_ROLLER_MIN,TEMP_ROLLER_MAX,"",false);
  gui.element.rollerPopup.tempFahrenheitOptions = createRollerValues(TEMP_ROLLER_MIN,TEMP_ROLLER_MAX,"",true);
  gui.element.rollerPopup.minutesOptions = createRollerValues(0,240,"",false);
  gui.element.rollerPopup.secondsOptions = createRollerValues(0,59,"",false); 
  /* Fixed tolerance values matching Flutter: ±0.3°, ±0.5°, ±1.0° */
  {
      const char *tol = "0.3\n0.5\n1.0";
      gui.element.rollerPopup.tempToleranceOptions = strdup(tol);
  }

  //gui.element.filterPopup.filterName = ""; // Not Required this will set this to some constant pointer which is not good...
  //gui.element.filterPopup.isColorFilter = FILM_TYPE_NA;   // This breaks filtering not needed
  //gui.element.filterPopup.isBnWFilter = FILM_TYPE_NA;
  //gui.element.filterPopup.isBnWFilter = 0;
  //gui.element.filterPopup.preferredOnly = 0;
  
  //gui.page.processes.isFiltered = 0; // Not Required memset takes care of this also

  gui.tempProcessNode = (processNode*) allocateAndInitializeNode(PROCESS_NODE);
  if (gui.tempProcessNode == NULL) {
      LV_LOG_USER("Failed to allocate tempProcessNode");
  }
  gui.tempStepNode = (stepNode*) allocateAndInitializeNode(STEP_NODE);
  if (gui.tempStepNode == NULL) {
      LV_LOG_USER("Failed to allocate tempStepNode");
  }

}

//without the commented part, the keyboard will be shown OVER the caller
void showKeyboard(lv_obj_t * whoCallMe, lv_obj_t * textArea){
    if(textArea != NULL){
    //lv_obj_add_flag(whoCallMe, LV_OBJ_FLAG_HIDDEN);
      if(strlen(lv_textarea_get_text(textArea)) > 0)
        lv_textarea_set_text(gui.element.keyboardPopup.keyboardTextArea, lv_textarea_get_text(textArea));
      else
        lv_textarea_set_text(gui.element.keyboardPopup.keyboardTextArea, "");

      lv_keyboard_set_mode(gui.element.keyboardPopup.keyboard, LV_KEYBOARD_MODE_USER_3);
      lv_obj_remove_flag(gui.element.keyboardPopup.keyBoardParent, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(gui.element.keyboardPopup.keyBoardParent);
    } else lv_textarea_set_text(gui.element.keyboardPopup.keyboardTextArea, "");
}

void hideKeyboard(lv_obj_t * whoCallMe){
    LV_UNUSED(whoCallMe);
    lv_obj_add_flag(gui.element.keyboardPopup.keyBoardParent, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_background(gui.element.keyboardPopup.keyBoardParent);
    lv_textarea_set_text(gui.element.keyboardPopup.keyboardTextArea, "");
    kb_ctx_clear();
}

#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
    LV_UNUSED(level);
    printf( "%s\r\n", buf );
}
#endif

uint8_t SD_init() {

    sdmmc_card_t *card;
    esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false,
        .max_files = 5,
    };
    LV_LOG_USER("Initialise SD Card");

#if defined(SD_BUS_SDMMC)
    /* ── P4: SDMMC 4-bit bus (much faster than SPI) ──
     * The JC4880P433 board requires on-chip LDO channel 4 (3.3V)
     * to power the SD card slot — without it the card won't respond.
     * (Same pattern used by RetroESP32-P4 reference project.)
     */

    /* 1. Power the SD card via on-chip LDO */
    sd_pwr_ctrl_ldo_config_t ldo_config = { .ldo_chan_id = 4 };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;
    esp_err_t ldo_ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ldo_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init SD LDO power (ch4): %s", esp_err_to_name(ldo_ret));
    }

    /* 2. Configure SDMMC host — slot 0, default speed for reliable init */
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    if (pwr_ctrl_handle) {
        host.pwr_ctrl_handle = pwr_ctrl_handle;
    }

    /* 3. Configure slot pins (4-bit bus) */
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = SD_MMC_CLK;
    slot_config.cmd = SD_MMC_CMD;
    slot_config.d0  = SD_MMC_D0;
    slot_config.d1  = SD_MMC_D1;
    slot_config.d2  = SD_MMC_D2;
    slot_config.d3  = SD_MMC_D3;
    slot_config.width = 4;
    slot_config.cd  = SDMMC_SLOT_NO_CD;
    slot_config.wp  = SDMMC_SLOT_NO_WP;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    /* 4. Mount with retry + power-cycle (some cards need a reset) */
    ESP_LOGI(TAG, "Mounting filesystem (SDMMC 4-bit)");
    esp_err_t mount_ret = ESP_FAIL;
    for (int attempt = 0; attempt < 3; attempt++) {
        mount_ret = esp_vfs_fat_sdmmc_mount("/sd", &host, &slot_config, &mount, &card);
        if (mount_ret == ESP_OK) break;
        ESP_LOGW(TAG, "SD mount attempt %d failed (%s), power-cycling...",
                 attempt + 1, esp_err_to_name(mount_ret));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    if (mount_ret != ESP_OK) {
        LV_LOG_USER("Card Mount Failed (SDMMC) after 3 attempts");
        return ESP_FAIL;
    }

#else
    /* ── S3: SPI SD card ── */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SDSPI_HOST_ID;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    if (spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialise SPI bus.");
        return ESP_FAIL;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem (SPI)");
    if (esp_vfs_fat_sdspi_mount("/sd", &host, &slot_config, &mount, &card) != ESP_OK) {
        LV_LOG_USER("Card Mount Failed");
        return ESP_FAIL;
    }
#endif

    LV_LOG_USER("SD init okay.");
    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}

#if defined(BOARD_JC4880P433)
/* Global I2C master bus handle — used by init_touch() in FilMachine.c */
i2c_master_bus_handle_t g_i2c_bus_handle = NULL;
#endif

void init_Pins_and_Buses( void ) {
	
#if defined(DISPLAY_DRIVER_ST7701)
    /* P4: backlight is managed by st7701_lcd via LEDC PWM — skip GPIO init here */
    ESP_LOGI(TAG, "LCD backlight: managed by st7701_lcd (LEDC PWM)");
#else
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_BLK
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(LCD_BLK, LCD_BK_LIGHT_OFF_LEVEL);
#endif

#if defined(DISPLAY_BUS_PARALLEL16)
    ESP_LOGI(TAG, "Set RD Pin High");		/* Required for parallel-16 bus displays */
    gpio_config_t rd_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_RD
    };
    ESP_ERROR_CHECK(gpio_config(&rd_gpio_config));
    gpio_set_level(LCD_RD, 1);
#endif

	if (SD_init()) {
	    initErrors = INIT_ERROR_SD;
	    LV_LOG_USER("ERROR:   SD initErrors %d", initErrors);
	} else{
//	    createFile(SD, FILENAME_SAVE);
	    LV_LOG_USER("SD INIT OVER initErrors %d", initErrors);
	}

    /* Initialize I2C */
    ESP_LOGI(TAG, "Initialize I2C bus %d (SDA=%d, SCL=%d)", I2C_NUM, I2C_SDA, I2C_SCL);
#if defined(BOARD_JC4880P433)
    /* ESP32-P4: use the new I2C master driver (old API deprecated on P4).
     * The GT911 managed component and MCP23017/PCA9685 still work via
     * the legacy-compatible i2c_master_get_bus_handle() bridge. */
    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = I2C_NUM,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &g_i2c_bus_handle));
#else
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM, i2c_conf.mode, 0, 0, 0));
#endif

    /* Initialize MCP23017 I/O expander */
#if defined(BOARD_JC4880P433)
    if (mcp23017_init(&mcp, g_i2c_bus_handle, MCP23017_DEFAULT_ADDR) != ESP_OK) {
#else
    if (mcp23017_init(&mcp, I2C_NUM, MCP23017_DEFAULT_ADDR) != ESP_OK) {
#endif
        LV_LOG_USER("MCP23017 init ERROR!");
        initErrors = INIT_ERROR_I2C_MCP;
    } else {
        LV_LOG_USER("MCP23017 init OK at 0x%02X", MCP23017_DEFAULT_ADDR);
        initializeRelayPins();
        initializeMotorPins();
    }

    /* Initialize DS18B20 temperature sensors (both on same OneWire bus) */
    if (ds18b20_init(&ds_bus, TEMPERATURE_BUS_PIN) != ESP_OK) {
        LV_LOG_USER("DS18B20 bus NOT found on GPIO%d", TEMPERATURE_BUS_PIN);
    } else {
        LV_LOG_USER("DS18B20 bus OK: %d sensor(s) on GPIO%d", ds_bus.sensor_count, TEMPERATURE_BUS_PIN);
        for (int i = 0; i < ds_bus.sensor_count; i++) {
            LV_LOG_USER("  sensor[%d] ROM=%02X%02X%02X%02X%02X%02X%02X%02X",
                i, ds_bus.sensors[i].rom[0], ds_bus.sensors[i].rom[1],
                ds_bus.sensors[i].rom[2], ds_bus.sensors[i].rom[3],
                ds_bus.sensors[i].rom[4], ds_bus.sensors[i].rom[5],
                ds_bus.sensors[i].rom[6], ds_bus.sensors[i].rom[7]);
        }
        if (ds_bus.sensor_count < 2) {
            LV_LOG_USER("WARNING: expected 2 sensors (bath+chemical), found %d", ds_bus.sensor_count);
        }
    }

    /* Initialize PCA9685 I2C PWM controller for pump speed control.
     * Daisy-chained on same I2C bus as MCP23017 via Adafruit 6318 pass-through. */
    {
#if defined(BOARD_JC4880P433)
        esp_err_t pca_ret = pca9685_init(&pca_pwm, g_i2c_bus_handle, PCA9685_ADDR, PCA9685_PWM_FREQ);
#else
        esp_err_t pca_ret = pca9685_init(&pca_pwm, I2C_NUM, PCA9685_ADDR, PCA9685_PWM_FREQ);
#endif
        if (pca_ret == ESP_OK) {
            LV_LOG_USER("PCA9685 at 0x%02X init OK — pump on ch%d, %d Hz PWM",
                        PCA9685_ADDR, PUMP_PCA9685_CHANNEL, PCA9685_PWM_FREQ);
        } else {
            LV_LOG_USER("PCA9685 at 0x%02X init FAIL (err %d) — pump PWM unavailable",
                        PCA9685_ADDR, pca_ret);
        }
    }

  if (initErrors) {

    LV_LOG_USER("SOMETHING WRONG initErrors %d", initErrors);
  } else{
    LV_LOG_USER("ALL SUCCESS initErrors %d", initErrors);
  }
}

/* ── Read ONLY the machineSettings blob (no processes, no stats).
 *    Used at boot to know splash preferences before the full UI is ready. ── */
void readSettingsOnly(const char *path) {
    FIL *fp = heap_caps_malloc(sizeof(FIL), MALLOC_CAP_SPIRAM);
    if (!fp) {
        LV_LOG_USER("readSettingsOnly: FAILED to alloc FIL struct");
        return;
    }
    unsigned int br;
    FRESULT res = f_open(fp, path, FA_READ | FA_OPEN_EXISTING);
    if (res == FR_OK) {
        if (f_read(fp, &gui.page.settings.settingsParams,
                   sizeof(gui.page.settings.settingsParams), &br) == FR_OK) {
            LV_LOG_USER("readSettingsOnly: loaded %u bytes (splashDefault=%d splashRandom=%d pal=%d style=%d cmx=%d seed=%"PRIu32")",
                        (unsigned)br,
                        gui.page.settings.settingsParams.splashDefault,
                        gui.page.settings.settingsParams.splashRandom,
                        gui.page.settings.settingsParams.splashPalette,
                        gui.page.settings.settingsParams.splashShapeStyle,
                        gui.page.settings.settingsParams.splashComplexity,
                        gui.page.settings.settingsParams.splashSeed);
        } else {
            LV_LOG_USER("readSettingsOnly: f_read FAILED");
        }
        f_close(fp);
    } else {
        LV_LOG_USER("readSettingsOnly: f_open('%s') FAILED (res=%d) — using defaults", path, res);
    }
    free(fp);
}

void readConfigFile(const char *path, bool enableLog) {

	FIL				*fp;
	FRESULT			res;
	unsigned int	bytes_read;

	/* Heap-allocate FIL to avoid stack overflow */
	fp = heap_caps_malloc( sizeof(FIL), MALLOC_CAP_SPIRAM );
	if (!fp) {
		LV_LOG_USER("Error: cannot allocate FIL struct for read");
		return;
	}

	/* ── Crash recovery: if .cfg is missing but .tmp exists, the previous
	 *    write completed successfully but the rename was interrupted.
	 *    Recover by renaming .tmp → .cfg before proceeding. ── */
	res = f_open(fp, path, FA_READ | FA_OPEN_EXISTING);
	if (res != FR_OK) {
	    char tmpPath[64];
	    snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);
	    FRESULT tmpRes = f_open(fp, tmpPath, FA_READ | FA_OPEN_EXISTING);
	    if (tmpRes == FR_OK) {
	        f_close(fp);
	        f_rename(tmpPath, path);
	        LV_LOG_USER("Recovered config from %s → %s", tmpPath, path);
	        res = f_open(fp, path, FA_READ | FA_OPEN_EXISTING);
	    }
	}

  	if( res == FR_OK ) {
	    // Load Machine Settings
	    /* Zero the struct first so new fields (Wi-Fi etc.) get safe defaults
	     * when reading an older, shorter config file */
	    memset(&gui.page.settings.settingsParams, 0, sizeof(gui.page.settings.settingsParams));
	    if( (res = f_read( fp, &gui.page.settings.settingsParams, sizeof(gui.page.settings.settingsParams), &bytes_read ) ) !=  FR_OK ) {
	      f_close( fp );
	      free( fp );
	      LV_LOG_USER("Configuration file error aborting load err: %d", res );
	      return;
	    }
	    if(bytes_read < sizeof(gui.page.settings.settingsParams)) {
	        LV_LOG_USER("Config file shorter than expected (%u < %u) — new fields use defaults",
	                     bytes_read, (unsigned)sizeof(gui.page.settings.settingsParams));
	    }
	    ESP_LOGW(TAG, "[WiFi-Load] bytes_read=%u, struct_size=%u, enabled=%d, ssid='%s', pwd_len=%d",
	                 bytes_read, (unsigned)sizeof(gui.page.settings.settingsParams),
	                 gui.page.settings.settingsParams.wifiEnabled,
	                 gui.page.settings.settingsParams.wifiSSID,
	                 (int)strlen(gui.page.settings.settingsParams.wifiPassword));

	    if(enableLog) {
	        LV_LOG_USER("--- MACHINE PARAMS ---");
	        LV_LOG_USER("tempUnit:%d",gui.page.settings.settingsParams.tempUnit);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.tempUnit));
	        LV_LOG_USER("waterInlet:%d",gui.page.settings.settingsParams.waterInlet);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.waterInlet));
	        LV_LOG_USER("calibratedTemp:%d",gui.page.settings.settingsParams.calibratedTemp);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.calibratedTemp));
	        LV_LOG_USER("filmRotationSpeedSetpoint:%d",gui.page.settings.settingsParams.filmRotationSpeedSetpoint);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.filmRotationSpeedSetpoint));
	        LV_LOG_USER("rotationIntervalSetpoint:%d",gui.page.settings.settingsParams.rotationIntervalSetpoint);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.rotationIntervalSetpoint));
	        LV_LOG_USER("randomSetpoint:%d",gui.page.settings.settingsParams.randomSetpoint);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.randomSetpoint));
	        LV_LOG_USER("isPersistentAlarm:%d",gui.page.settings.settingsParams.isPersistentAlarm);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.isPersistentAlarm));
	        LV_LOG_USER("isProcessAutostart:%d",gui.page.settings.settingsParams.isProcessAutostart);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.isProcessAutostart));
	        LV_LOG_USER("drainFillOverlapSetpoint:%d",gui.page.settings.settingsParams.drainFillOverlapSetpoint);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.drainFillOverlapSetpoint));
	        LV_LOG_USER("multiRinseTime:%d",gui.page.settings.settingsParams.multiRinseTime);
	//        LV_LOG_USER("size:%d",sizeof(gui.page.settings.settingsParams.multiRinseTime));
	        LV_LOG_USER("tankSize:%d",gui.page.settings.settingsParams.tankSize);
	        LV_LOG_USER("pumpSpeed:%d",gui.page.settings.settingsParams.pumpSpeed);
	        LV_LOG_USER("chemContainerMl:%d",gui.page.settings.settingsParams.chemContainerMl);
	        LV_LOG_USER("wbContainerMl:%d",gui.page.settings.settingsParams.wbContainerMl);
	        LV_LOG_USER("chemistryVolume:%d",gui.page.settings.settingsParams.chemistryVolume);
	    }   

	    // Load Processes
	    processList *processElementsList = &gui.page.processes.processElementsList;
	    processElementsList->start = NULL;
	    processElementsList->end = NULL;
	    processElementsList->size = 0;    
	    // Read process list size
	    f_read( fp, &processElementsList->size, sizeof(processElementsList->size), &bytes_read );
	    if(enableLog) LV_LOG_USER("Number of Processes:%"PRIi32"", processElementsList->size);
		
		if( processElementsList->size > MAX_PROC_ELEMENTS ) {
			LV_LOG_USER("Warning: File may be corrupt number of processes(%"PRIi32") capped to %d", processElementsList->size, MAX_PROC_ELEMENTS);
			processElementsList->size = MAX_PROC_ELEMENTS;		// Damage limitation in the event of problems 
		}
	    for(int32_t process = 0; process < processElementsList->size; process++){
	      if(enableLog) LV_LOG_USER("Process:%"PRIi32"", process);
	
	      processNode *nodeP = (processNode*) allocateAndInitializeNode(PROCESS_NODE);
	      if (nodeP == NULL) {
	          LV_LOG_USER("Failed to allocate memory for process node");
	          continue;
	      }
	      f_read( fp, &nodeP->process.processDetails->data.processNameString, sizeof(nodeP->process.processDetails->data.processNameString), &bytes_read );
	      f_read( fp, &nodeP->process.processDetails->data.temp, sizeof(nodeP->process.processDetails->data.temp), &bytes_read );
	      f_read( fp, &nodeP->process.processDetails->data.tempTolerance, sizeof(nodeP->process.processDetails->data.tempTolerance), &bytes_read );
	      f_read( fp, &nodeP->process.processDetails->data.isTempControlled, sizeof(nodeP->process.processDetails->data.isTempControlled), &bytes_read );
	      f_read( fp, &nodeP->process.processDetails->data.isPreferred, sizeof(nodeP->process.processDetails->data.isPreferred), &bytes_read );
	      f_read( fp, &nodeP->process.processDetails->data.filmType, sizeof(nodeP->process.processDetails->data.filmType), &bytes_read );
	      /* Clamp temp loaded from old files: if below roller minimum (15), default to 20°C */
	      if (nodeP->process.processDetails->data.temp < 15) {
	          nodeP->process.processDetails->data.temp = 20;
	      }
	      f_read( fp, &nodeP->process.processDetails->data.timeMins, sizeof(nodeP->process.processDetails->data.timeMins), &bytes_read );
	      f_read( fp, &nodeP->process.processDetails->data.timeSecs, sizeof(nodeP->process.processDetails->data.timeSecs), &bytes_read );
	
	      if (processElementsList->start == NULL) {
	        processElementsList->start = nodeP;
	        nodeP->prev = NULL;
	      } else {
	        processElementsList->end->next = nodeP;
	        nodeP->prev = processElementsList->end;
	      }
	      processElementsList->end = nodeP;
	      processElementsList->end->next = NULL;
	
	      if(enableLog){
	        LV_LOG_USER("--- PROCESS PARAMS ---");
	        LV_LOG_USER("processNameString:%s",nodeP->process.processDetails->data.processNameString);
	//        LV_LOG_USER("size:%d",sizeof(nodeP->process.processDetails->data.processNameString));
	        LV_LOG_USER("temp:%"PRIi32"",nodeP->process.processDetails->data.temp);
	//        LV_LOG_USER("size:%d",sizeof(nodeP->process.processDetails->data.temp));
	        LV_LOG_USER("tempTolerance:%f",nodeP->process.processDetails->data.tempTolerance);
	//        LV_LOG_USER("size:%d",sizeof(nodeP->process.processDetails->data.tempTolerance));
	        LV_LOG_USER("isTempControlled:%d",nodeP->process.processDetails->data.isTempControlled);
	//        LV_LOG_USER("size:%d",sizeof(nodeP->process.processDetails->data.isTempControlled));
	        LV_LOG_USER("isPreferred:%d",nodeP->process.processDetails->data.isPreferred);
	//        LV_LOG_USER("size:%d",sizeof(nodeP->process.processDetails->data.isPreferred));
	        LV_LOG_USER("filmType:%d",nodeP->process.processDetails->data.filmType);
	//        LV_LOG_USER("size:%d",sizeof(nodeP->process.processDetails->data.filmType));
	        LV_LOG_USER("timeMins:%"PRIu32"",nodeP->process.processDetails->data.timeMins);
	//        LV_LOG_USER("size:%d",sizeof(nodeP->process.processDetails->data.timeMins));
	        LV_LOG_USER("timeSecs:%d",nodeP->process.processDetails->data.timeSecs);
	//        LV_LOG_USER("size:%d",sizeof(nodeP->process.processDetails->data.timeSecs));
	      }

	      stepList *stepElementsList = &nodeP->process.processDetails->stepElementsList;
	      stepElementsList->start = NULL;
	      stepElementsList->end = NULL;
	      stepElementsList->size = 0;
	
	      // Write step list size
	      f_read( fp, &stepElementsList->size, sizeof(stepElementsList->size), &bytes_read );
	      if(enableLog) LV_LOG_USER("Process Steps:%d", stepElementsList->size);
		  if( stepElementsList->size > MAX_STEP_ELEMENTS ) {
		  	LV_LOG_USER("Warning: File may be corrupt number of steps(%"PRIu16") capped to %d", stepElementsList->size, MAX_STEP_ELEMENTS);
		  	stepElementsList->size = MAX_STEP_ELEMENTS;		// Damage limitation in the event of problems 
		  }

	      for(int32_t step = 0; step < stepElementsList->size; step++){                
	        if(enableLog) LV_LOG_USER("Step:%"PRIi32"", step);
	        stepNode *nodeS = (stepNode*) allocateAndInitializeNode(STEP_NODE);
	        if (nodeS == NULL) {
	          LV_LOG_USER("Failed to allocate memory for step node");
	          continue;
	        }
	
	        f_read( fp, &nodeS->step.stepDetails->data.stepNameString, sizeof(nodeS->step.stepDetails->data.stepNameString), &bytes_read );
	        f_read( fp, &nodeS->step.stepDetails->data.timeMins, sizeof(nodeS->step.stepDetails->data.timeMins), &bytes_read );
	        f_read( fp, &nodeS->step.stepDetails->data.timeSecs, sizeof(nodeS->step.stepDetails->data.timeSecs), &bytes_read );
	        f_read( fp, &nodeS->step.stepDetails->data.type, sizeof(nodeS->step.stepDetails->data.type), &bytes_read );
	        f_read( fp, &nodeS->step.stepDetails->data.source, sizeof(nodeS->step.stepDetails->data.source), &bytes_read );
	        f_read( fp, &nodeS->step.stepDetails->data.discardAfterProc, sizeof(nodeS->step.stepDetails->data.discardAfterProc), &bytes_read );
	        
	        if (stepElementsList->start == NULL) {
	          stepElementsList->start = nodeS;
	          nodeS->prev = NULL;
	        } else {
	          stepElementsList->end->next = nodeS;
	          nodeS->prev = stepElementsList->end;
	        }
	        stepElementsList->end = nodeS;
	        stepElementsList->end->next = NULL;
	
	        if(enableLog){
	          LV_LOG_USER("--- STEP PARAMS ---");
	          LV_LOG_USER("stepNameString:%s",nodeS->step.stepDetails->data.stepNameString);
	//          LV_LOG_USER("size:%d",sizeof(nodeS->step.stepDetails->data.stepNameString));
	          LV_LOG_USER("timeMins:%d",nodeS->step.stepDetails->data.timeMins);
	//          LV_LOG_USER("size:%d",sizeof(nodeS->step.stepDetails->data.timeMins));
	          LV_LOG_USER("timeSecs:%d",nodeS->step.stepDetails->data.timeSecs);
	//          LV_LOG_USER("size:%d",sizeof(nodeS->step.stepDetails->data.timeSecs));
	          LV_LOG_USER("type:%d",nodeS->step.stepDetails->data.type);
	//          LV_LOG_USER("size:%d",sizeof(nodeS->step.stepDetails->data.type));
	          LV_LOG_USER("source:%d",nodeS->step.stepDetails->data.source);
	//          LV_LOG_USER("size:%d",sizeof(nodeS->step.stepDetails->data.source));
	          LV_LOG_USER("discardAfterProc:%d",nodeS->step.stepDetails->data.discardAfterProc);
	//          LV_LOG_USER("size:%d",sizeof(nodeS->step.stepDetails->data.discardAfterProc));
	        }
	
	      }
	
	    }

	    /* ── Machine statistics (appended after processes) ── */
	    {
	        unsigned int br = 0;
	        f_read( fp, &gui.page.tools.machineStats.completed, sizeof(gui.page.tools.machineStats.completed), &br );
	        if(br > 0) {
	            f_read( fp, &gui.page.tools.machineStats.totalMins, sizeof(gui.page.tools.machineStats.totalMins), &br );
	            f_read( fp, &gui.page.tools.machineStats.totalSecs, sizeof(gui.page.tools.machineStats.totalSecs), &br );
	            f_read( fp, &gui.page.tools.machineStats.stopped,   sizeof(gui.page.tools.machineStats.stopped),   &br );
	            f_read( fp, &gui.page.tools.machineStats.clean,     sizeof(gui.page.tools.machineStats.clean),     &br );
	            LV_LOG_USER("Loaded stats: completed=%"PRIu32" totalMins=%"PRIu64" totalSecs=%"PRIu32" stopped=%"PRIu32" clean=%"PRIu32"",
	                gui.page.tools.machineStats.completed, gui.page.tools.machineStats.totalMins,
	                gui.page.tools.machineStats.totalSecs, gui.page.tools.machineStats.stopped, gui.page.tools.machineStats.clean);
	        } else {
	            LV_LOG_USER("No stats section in config (old format) — starting from zero");
	        }
	    }

	    f_close( fp );
	} else {
		LV_LOG_USER("Failed to open configuration file for reading using default err: %d", res);
	}
	free( fp );  /* Release heap-allocated FIL struct */
}

void writeConfigFile( const char *path, bool enableLog ) {

	FIL				*fp;
	FRESULT			res;
	unsigned int	bytes_written;

	/* Heap-allocate FIL to avoid stack overflow in sysMan task */
	fp = heap_caps_malloc( sizeof(FIL), MALLOC_CAP_SPIRAM );
	if (!fp) {
		LV_LOG_USER("Error: cannot allocate FIL struct for write");
		return;
	}

    if (initErrors != INIT_ERROR_SD) {
        /* ── Crash-safe write: write to .tmp, then rename ──
         * If the system crashes mid-write, the original .cfg is preserved.
         * On next boot, readConfigFile recovers from .tmp if .cfg is missing. */
        char tmpPath[64];
        snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);

        /* Remove stale temp file if any */
        f_unlink(tmpPath);

        LV_LOG_USER("Writing configuration file: %s (via %s)", path, tmpPath);

        if( ( res = f_open( fp, tmpPath, FA_WRITE | FA_CREATE_NEW ) ) != FR_OK ) {
            LV_LOG_USER("Failed to open temp file for writing (res=%d)", res);
            free(fp);
            return;
        }

        // Write Machine Parameters
        LV_LOG_USER("Writing settingsParams: %zu bytes", sizeof(gui.page.settings.settingsParams));
        ESP_LOGW(TAG, "[WiFi-Save] enabled=%d, ssid='%s', pwd_len=%d",
                     gui.page.settings.settingsParams.wifiEnabled,
                     gui.page.settings.settingsParams.wifiSSID,
                     (int)strlen(gui.page.settings.settingsParams.wifiPassword));
        f_write( fp, &gui.page.settings.settingsParams, sizeof(gui.page.settings.settingsParams), &bytes_written );
        LV_LOG_USER("settingsParams written OK: %u bytes", bytes_written);

        if (enableLog) {
            LV_LOG_USER("--- MACHINE PARAMS ---");
            LV_LOG_USER("tempUnit:%d", gui.page.settings.settingsParams.tempUnit);
            LV_LOG_USER("waterInlet:%d", gui.page.settings.settingsParams.waterInlet);
            LV_LOG_USER("calibratedTemp:%d", gui.page.settings.settingsParams.calibratedTemp);
            LV_LOG_USER("filmRotationSpeedSetpoint:%d", gui.page.settings.settingsParams.filmRotationSpeedSetpoint);
            LV_LOG_USER("rotationIntervalSetpoint:%d", gui.page.settings.settingsParams.rotationIntervalSetpoint);
            LV_LOG_USER("randomSetpoint:%d", gui.page.settings.settingsParams.randomSetpoint);
            LV_LOG_USER("isPersistentAlarm:%d", gui.page.settings.settingsParams.isPersistentAlarm);
            LV_LOG_USER("isProcessAutostart:%d", gui.page.settings.settingsParams.isProcessAutostart);
            LV_LOG_USER("drainFillOverlapSetpoint:%d", gui.page.settings.settingsParams.drainFillOverlapSetpoint);
        }

        // Write Processes
        LV_LOG_USER("Writing processes: count=%"PRIu32"", gui.page.processes.processElementsList.size);
        processNode *currentProcessNode = gui.page.processes.processElementsList.start;
        // Write process list size
        f_write( fp, &gui.page.processes.processElementsList.size, sizeof(gui.page.processes.processElementsList.size), &bytes_written );

        while (currentProcessNode != NULL) {
            f_write( fp, currentProcessNode->process.processDetails->data.processNameString, sizeof(currentProcessNode->process.processDetails->data.processNameString), &bytes_written );
            f_write( fp, &currentProcessNode->process.processDetails->data.temp, sizeof(currentProcessNode->process.processDetails->data.temp), &bytes_written );
            f_write( fp, &currentProcessNode->process.processDetails->data.tempTolerance, sizeof(currentProcessNode->process.processDetails->data.tempTolerance), &bytes_written );
            f_write( fp, &currentProcessNode->process.processDetails->data.isTempControlled, sizeof(currentProcessNode->process.processDetails->data.isTempControlled), &bytes_written );
            f_write( fp, &currentProcessNode->process.processDetails->data.isPreferred, sizeof(currentProcessNode->process.processDetails->data.isPreferred), &bytes_written );
            f_write( fp, &currentProcessNode->process.processDetails->data.filmType, sizeof(currentProcessNode->process.processDetails->data.filmType), &bytes_written );
            f_write( fp, &currentProcessNode->process.processDetails->data.timeMins, sizeof(currentProcessNode->process.processDetails->data.timeMins), &bytes_written );
            f_write( fp, &currentProcessNode->process.processDetails->data.timeSecs, sizeof(currentProcessNode->process.processDetails->data.timeSecs), &bytes_written );

            if (enableLog) {
                LV_LOG_USER("--- PROCESS PARAMS ---");
                LV_LOG_USER("processNameString:%s", currentProcessNode->process.processDetails->data.processNameString);
                LV_LOG_USER("temp:%"PRIu32"", currentProcessNode->process.processDetails->data.temp);
                LV_LOG_USER("tempTolerance:%f", currentProcessNode->process.processDetails->data.tempTolerance);
                LV_LOG_USER("isTempControlled:%d", currentProcessNode->process.processDetails->data.isTempControlled);
                LV_LOG_USER("isPreferred:%d", currentProcessNode->process.processDetails->data.isPreferred);
                LV_LOG_USER("filmType:%d", currentProcessNode->process.processDetails->data.filmType);
                LV_LOG_USER("timeMins:%"PRIu32"", currentProcessNode->process.processDetails->data.timeMins);
                LV_LOG_USER("timeSecs:%"PRIu8"", currentProcessNode->process.processDetails->data.timeSecs);
            }

            stepNode *currentStepNode = currentProcessNode->process.processDetails->stepElementsList.start;
            // Write step list size
            f_write( fp, &currentProcessNode->process.processDetails->stepElementsList.size, sizeof(currentProcessNode->process.processDetails->stepElementsList.size), &bytes_written );
            while (currentStepNode != NULL) {
                f_write( fp, currentStepNode->step.stepDetails->data.stepNameString, sizeof(currentStepNode->step.stepDetails->data.stepNameString), &bytes_written );
                f_write( fp, &currentStepNode->step.stepDetails->data.timeMins, sizeof(currentStepNode->step.stepDetails->data.timeMins), &bytes_written );
                f_write( fp, &currentStepNode->step.stepDetails->data.timeSecs, sizeof(currentStepNode->step.stepDetails->data.timeSecs), &bytes_written );
                f_write( fp, &currentStepNode->step.stepDetails->data.type, sizeof(currentStepNode->step.stepDetails->data.type), &bytes_written );
                f_write( fp, &currentStepNode->step.stepDetails->data.source, sizeof(currentStepNode->step.stepDetails->data.source), &bytes_written );
                f_write( fp, &currentStepNode->step.stepDetails->data.discardAfterProc, sizeof(currentStepNode->step.stepDetails->data.discardAfterProc), &bytes_written );

                if (enableLog) {
                    LV_LOG_USER("--- STEP PARAMS ---");
                    LV_LOG_USER("stepNameString:%s", currentStepNode->step.stepDetails->data.stepNameString);
                    LV_LOG_USER("timeMins:%d", currentStepNode->step.stepDetails->data.timeMins);
                    LV_LOG_USER("timeSecs:%d", currentStepNode->step.stepDetails->data.timeSecs);
                    LV_LOG_USER("type:%d", currentStepNode->step.stepDetails->data.type);
                    LV_LOG_USER("source:%d", currentStepNode->step.stepDetails->data.source);
                    LV_LOG_USER("discardAfterProc:%d", currentStepNode->step.stepDetails->data.discardAfterProc);
                }
                currentStepNode = currentStepNode->next;
            }
            currentProcessNode = currentProcessNode->next;
        }
        /* ── Machine statistics (appended after processes) ── */
        f_write( fp, &gui.page.tools.machineStats.completed,  sizeof(gui.page.tools.machineStats.completed),  &bytes_written );
        f_write( fp, &gui.page.tools.machineStats.totalMins,   sizeof(gui.page.tools.machineStats.totalMins),   &bytes_written );
        f_write( fp, &gui.page.tools.machineStats.totalSecs,   sizeof(gui.page.tools.machineStats.totalSecs),   &bytes_written );
        f_write( fp, &gui.page.tools.machineStats.stopped,     sizeof(gui.page.tools.machineStats.stopped),     &bytes_written );
        f_write( fp, &gui.page.tools.machineStats.clean,       sizeof(gui.page.tools.machineStats.clean),       &bytes_written );

        f_close( fp );

        /* ── Atomic swap: delete old config, rename temp → config ──
         * The old file is only deleted after the new one is fully written
         * and closed. If we crash between unlink and rename, the .tmp
         * file is still complete and will be recovered on next boot. */
        res = f_unlink(path);
        if (res != FR_OK && res != FR_NO_FILE) {
            LV_LOG_USER("Warning: could not delete old config (res=%d)", res);
        }
        res = f_rename(tmpPath, path);
        if (res != FR_OK) {
            LV_LOG_USER("Error: f_rename(%s → %s) failed (res=%d)", tmpPath, path, res);
            /* The .tmp file is still valid — will be recovered on next boot */
        } else {
            LV_LOG_USER("Config saved successfully: %s", path);
        }

        /* Notify connected WS clients (Flutter) about the updated data */
        ws_broadcast_process_list();
        ws_broadcast_state();
    }
    free( fp );  /* Release heap-allocated FIL struct */
}

#define FILE_COPY_BUF_SIZE	4096
bool copyAndRenameFile( const char* sourceFile, const char* destFile) {

	/*
	 * FIL structs are ~600+ bytes each (larger with LFN enabled).
	 * Allocating on heap prevents stack overflow in sysMan task.
	 */
	FIL				*sp = NULL, *dp = NULL;
	FRESULT			res;
	unsigned int	bytes_read = 1, bytes_written, current_written;
	char			*buf = NULL;
	bool			ret = false;

	// Before removing the old destination, verify that the source exists!
	if( f_stat( sourceFile, NULL ) != FR_OK ) {
		LV_LOG_USER("Error: source file does not exist!" );
		return ret;
	}

	sp = heap_caps_malloc( sizeof(FIL), MALLOC_CAP_SPIRAM );
	dp = heap_caps_malloc( sizeof(FIL), MALLOC_CAP_SPIRAM );
	buf = malloc( FILE_COPY_BUF_SIZE );
	if( !sp || !dp || !buf ) {
		LV_LOG_USER("Error during allocation (FIL/buf)!" );
		free(sp); free(dp); free(buf);
		return ret;
	}

	// Check if the destination file already exists and remove it if necessary
	if( f_stat( destFile, NULL ) == FR_OK ) {
		unlink( destFile );
	}
	// Open the source file in read mode
	if( (res = f_open(sp, sourceFile, FA_READ) ) != FR_OK ) {
	    LV_LOG_USER("Error opening source file! err: %d", res);
	    goto out1;
	}

	// Create or overwrite the destination file
	if( ( res = f_open( dp, destFile, FA_WRITE | FA_CREATE_ALWAYS ) ) != FR_OK ) {
		LV_LOG_USER("Error opening destination file! err: %d", res);
		f_close( sp );
	    return false;
	}

	// Read the source file content and write it to the destination file
	while( bytes_read >  0 ) {
	    if( (res = f_read( sp, buf, FILE_COPY_BUF_SIZE, &bytes_read ) ) != FR_OK ) {
			LV_LOG_USER( "Error reading during copy! err:%d", res );
			goto out2;
		}
		current_written = 0;
		
		while( current_written < bytes_read ) {
			if( (res = f_write( dp, buf, bytes_read - current_written, &bytes_written ) ) != FR_OK ) {
				LV_LOG_USER( "Error writing during copy! err:%d", res );
				goto out2;
			}
			current_written += bytes_written;
		}
	}

    // Close both files
out2:
	f_close( sp );
	f_close( dp );
	// Verify that the destination file was created correctly and is not empty
	if( f_stat( destFile, NULL ) == FR_OK ) {
		res = f_open( dp, destFile, FA_READ );
	    if( res == FR_OK && f_size( dp ) > 0 ) {
			f_close( dp );
	        LV_LOG_USER("File copied and renamed successfully!");
	        ret = true;
	    } else {
	        LV_LOG_USER("Error: destination file is empty or cannot be read.");
	    }
	} else {
	    LV_LOG_USER("Error: destination file was not created correctly.");
	}
	
out1:
	free( buf );	// Free the transfer buffer
	free( sp );		// Free heap-allocated FIL structs
	free( dp );
	return ret;
}

/* Data-only version: sums step durations into processData.timeMins/timeSecs.
 * Safe to call from ANY thread (no LVGL calls). */
void calculateTotalTimeData(processNode *processNode){
    uint32_t mins = 0;
    uint8_t  secs = 0;

     stepList *stepElementsList;
     stepElementsList = &(processNode->process.processDetails->stepElementsList);

            stepNode *stepNode;
            stepNode = stepElementsList->start;

            while(stepNode != NULL){
                mins += stepNode->step.stepDetails->data.timeMins;
                secs += stepNode->step.stepDetails->data.timeSecs;

                if (secs >= 60) {
                    mins += secs / 60;
                    secs = secs % 60;
                }
                stepNode = stepNode->next;
            }
    processNode->process.processDetails->data.timeMins = mins;
    processNode->process.processDetails->data.timeSecs = secs;
    LV_LOG_USER("Process %p has a total time of %"PRIu32"min:%"PRIu8"sec", processNode, mins, secs);
}

/* Full version: recalculates data AND updates the LVGL label.
 * Must be called from the LVGL thread only! */
void calculateTotalTime(processNode *processNode){
    calculateTotalTimeData(processNode);

    if(processNode->process.processDetails->processTotalTimeValue != NULL)
        lv_label_set_text_fmt(processNode->process.processDetails->processTotalTimeValue, "%"PRIu32"m%"PRIu8"s", processNode->process.processDetails->data.timeMins,
          processNode->process.processDetails->data.timeSecs);
}



uint8_t calculatePercentage(uint32_t minutes, uint8_t seconds, uint32_t total_minutes, uint8_t total_seconds) {
    // Calculate total time in seconds
    uint32_t total_time_seconds = total_minutes * 60 + total_seconds;

    // Calculate elapsed time in seconds
    uint32_t elapsed_time_seconds = minutes * 60 + seconds;

    // Calculate the percentage of elapsed time relative to total time
    uint8_t percentage = (uint8_t)((elapsed_time_seconds * 100) / total_time_seconds);

    // Ensure that the percentage is between 0 and 100
 	if (percentage > 100) {		// It's unsigned so don't need to check less than 0
        percentage = 100;
    }

    return percentage;
}

void updateProcessElement(processNode *process){
  processNode* existingProcess = (processNode*)isNodeInList((void*)&(gui.page.processes.processElementsList), process, PROCESS_NODE);
  
  if(existingProcess != NULL) {
      LV_LOG_USER("Updating process element in list");
      //Update time
      process_list_set_time_label(existingProcess);
      
      //Update temp
      lv_obj_send_event(existingProcess->process.processElement, LV_EVENT_REFRESH, NULL);


      if(process->process.processDetails->data.isTempControlled == false) {
          lv_obj_add_flag(process->process.processTempIcon, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(process->process.processTemp, LV_OBJ_FLAG_HIDDEN);
          lv_obj_align(process->process.processTimeIcon, LV_ALIGN_LEFT_MID, ui_get_profile()->process_element.time_icon_no_temp_x, ui_get_profile()->process_element.time_icon_y);
          lv_obj_align(process->process.processTime, LV_ALIGN_LEFT_MID, ui_get_profile()->process_element.time_value_no_temp_x, ui_get_profile()->process_element.time_value_y);
          lv_obj_set_width(process->process.processTime, ui_get_profile()->process_element.card_content_w - ui_get_profile()->process_element.time_value_no_temp_x - 18);
       } else {
            lv_obj_align(process->process.processTime, LV_ALIGN_LEFT_MID, ui_get_profile()->process_element.time_value_x, ui_get_profile()->process_element.time_value_y);
            lv_obj_align(process->process.processTimeIcon, LV_ALIGN_LEFT_MID, ui_get_profile()->process_element.time_icon_x, ui_get_profile()->process_element.time_icon_y);
            lv_obj_set_width(process->process.processTime, ui_get_profile()->process_element.card_content_w - ui_get_profile()->process_element.time_value_x - 18);
            lv_obj_remove_flag(process->process.processTempIcon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(process->process.processTemp, LV_OBJ_FLAG_HIDDEN);
       }
 
      //Update preferred
      if(process->process.processDetails->data.isPreferred == 1){
            lv_obj_set_style_text_color(existingProcess->process.preferredIcon, lv_color_hex(RED), LV_PART_MAIN);
      }
      else{
            lv_obj_set_style_text_color(existingProcess->process.preferredIcon, lv_color_hex(WHITE), LV_PART_MAIN);
      }
      
      //Update name
      lv_label_set_text(existingProcess->process.processName, process->process.processDetails->data.processNameString);

      //Update film type
      lv_label_set_text(existingProcess->process.processTypeIcon, process->process.processDetails->data.filmType == BLACK_AND_WHITE_FILM ? blackwhite_icon : colorpalette_icon);


  } 
}

void updateStepElement(processNode *referenceProcess, stepNode *step){
  stepNode* existingStep = (stepNode*)isNodeInList((void*)&(referenceProcess->process.processDetails->stepElementsList), step, STEP_NODE);

      if(existingStep != NULL) {
         LV_LOG_USER("Updating element element in list");
         
         //Update name
         lv_label_set_text(existingStep->step.stepName, step->step.stepDetails->data.stepNameString);

        //Update source
        char *temp[] = processSourceList;
         lv_label_set_text_fmt(existingStep->step.sourceLabel, stepSourceFmt_text, temp[step->step.stepDetails->data.source]); 

        //Update discard after icon
         if(step->step.stepDetails->data.discardAfterProc){
             lv_obj_set_style_text_color(existingStep->step.discardAfterIcon, lv_color_hex(WHITE), LV_PART_MAIN);
           } else {
             lv_obj_set_style_text_color(existingStep->step.discardAfterIcon, lv_color_hex(GREY), LV_PART_MAIN);
           }

          //Update type icon
          if(step->step.stepDetails->data.type == CHEMISTRY)
              lv_label_set_text(existingStep->step.stepTypeIcon, chemical_icon);
          if(step->step.stepDetails->data.type == RINSE)
              lv_label_set_text(existingStep->step.stepTypeIcon, rinse_icon);           
          if(step->step.stepDetails->data.type == MULTI_RINSE)
              lv_label_set_text(existingStep->step.stepTypeIcon, multiRinse_icon); 

          //Update time
          lv_label_set_text_fmt(existingStep->step.stepTime, "%"PRIu8"m%"PRIu8"s", step->step.stepDetails->data.timeMins, step->step.stepDetails->data.timeSecs); 
      }
}

uint32_t loadSDCardProcesses() {

    int32_t tempSize = 1;

    if (gui.page.processes.processElementsList.size > 0) {
        processNode *process = gui.page.processes.processElementsList.start;

        while (process != NULL) {
            processElementCreate(process, tempSize);
            process = process->next;
            tempSize++;
        }
        return gui.page.processes.processElementsList.size;
    } else {
        return 0;
    }
}

char* generateRandomCharArray(uint8_t length) {

	char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	char *randomArray; // Dynamic allocation of the array plus one for the null terminator

	randomArray = heap_caps_malloc( length + 1, MALLOC_CAP_SPIRAM );
	if( randomArray == NULL ) return NULL;		// If allocation fails return NULL pointer
	for (uint8_t i = 0; i < length; ++i) {
		randomArray[i] = charset[random() % sizeof(charset)];
	}
	randomArray[length] = '\0'; // Add the null terminator
	return randomArray;
}

void initializeRelayPins(){
    for (uint8_t i = 0; i < RELAY_NUMBER; i++) {
        mcp23017_pinMode(&mcp, i, MCP23017_OUTPUT);
        mcp23017_digitalWrite(&mcp, i, 0);
        LV_LOG_USER("Relay pin %d init: %d", i, mcp23017_digitalRead(&mcp, i));
    }
}


void cleanRelayManager(uint8_t pumpFrom, uint8_t pumpTo, uint8_t pumpDir, bool activePump){
    if (activePump) {
        mcp23017_digitalWrite(&mcp, pumpFrom, 1);
        LV_LOG_USER("From %d on : %d", pumpFrom, mcp23017_digitalRead(&mcp, pumpFrom));
        mcp23017_digitalWrite(&mcp, pumpTo, 1);
        LV_LOG_USER("To %d on : %d", pumpTo, mcp23017_digitalRead(&mcp, pumpTo));
        mcp23017_digitalWrite(&mcp, pumpDir, 1);
        LV_LOG_USER("Direction %d on : %d", pumpDir, mcp23017_digitalRead(&mcp, pumpDir));
    } else {
        if (pumpFrom != INVALID_RELAY && pumpTo != INVALID_RELAY && pumpDir != INVALID_RELAY) {
            mcp23017_digitalWrite(&mcp, pumpFrom, 0);
            LV_LOG_USER("From %d off : %d", pumpFrom, mcp23017_digitalRead(&mcp, pumpFrom));
            mcp23017_digitalWrite(&mcp, pumpTo, 0);
            LV_LOG_USER("To %d off : %d", pumpTo, mcp23017_digitalRead(&mcp, pumpTo));
            mcp23017_digitalWrite(&mcp, pumpDir, 0);
            LV_LOG_USER("Direction %d off : %d", pumpDir, mcp23017_digitalRead(&mcp, pumpDir));
        } else {
            for (uint8_t i = 0; i < RELAY_NUMBER; i++) {
                mcp23017_digitalWrite(&mcp, i, 0);
                LV_LOG_USER("Relay %d off : %d", i, mcp23017_digitalRead(&mcp, i));
            }
        }
    }
}


void sendValueToRelay(uint8_t pumpFrom, uint8_t pumpDir, bool activePump) {
    if (activePump) {
        mcp23017_digitalWrite(&mcp, pumpFrom, 1);
        LV_LOG_USER("From %d on : %d", pumpFrom, mcp23017_digitalRead(&mcp, pumpFrom));
        mcp23017_digitalWrite(&mcp, pumpDir, 1);
        LV_LOG_USER("Direction %d on : %d", pumpDir, mcp23017_digitalRead(&mcp, pumpDir));
        gui.tempProcessNode->process.processDetails->checkup->data.isAlreadyPumping = true;
    } else {
        if (pumpFrom != INVALID_RELAY && pumpDir != INVALID_RELAY) {
            mcp23017_digitalWrite(&mcp, pumpFrom, 0);
            LV_LOG_USER("From %d off : %d", pumpFrom, mcp23017_digitalRead(&mcp, pumpFrom));
            mcp23017_digitalWrite(&mcp, pumpDir, 0);
            LV_LOG_USER("Direction %d off : %d", pumpDir, mcp23017_digitalRead(&mcp, pumpDir));
        } else {
            for (uint8_t i = 0; i < RELAY_NUMBER; i++) {
                mcp23017_digitalWrite(&mcp, i, 0);
                LV_LOG_USER("Relay %d off : %d", i, mcp23017_digitalRead(&mcp, i));
            }
        }
    }
}


/* ── Motor direction helpers ──
 * P4 board: IN1/IN2 are ESP32 GPIOs on the Expand IO header (direct control).
 * Other boards: IN1/IN2 are MCP23017 pins (I2C expander). */
static inline void motor_dir_set(uint8_t pin, uint8_t value)
{
#if defined(BOARD_JC4880P433)
    gpio_set_level(pin, value);
#else
    mcp23017_digitalWrite(&mcp, pin, value);
#endif
}

void initializeMotorPins(){
#if defined(BOARD_JC4880P433)
    /* IN1 and IN2 on ESP32 GPIOs (Expand IO header) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_IN1_PIN) | (1ULL << MOTOR_IN2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(MOTOR_IN1_PIN, 0);
    gpio_set_level(MOTOR_IN2_PIN, 0);
    LV_LOG_USER("Motor IN1 (GPIO %d) + IN2 (GPIO %d) init OK", MOTOR_IN1_PIN, MOTOR_IN2_PIN);
#else
    /* IN1 and IN2 on MCP23017 */
    mcp23017_pinMode(&mcp, MOTOR_IN1_PIN, MCP23017_OUTPUT);
    mcp23017_digitalWrite(&mcp, MOTOR_IN1_PIN, 0);
    LV_LOG_USER("Motor pin init %d: %d", MOTOR_IN1_PIN, mcp23017_digitalRead(&mcp, MOTOR_IN1_PIN));

    mcp23017_pinMode(&mcp, MOTOR_IN2_PIN, MCP23017_OUTPUT);
    mcp23017_digitalWrite(&mcp, MOTOR_IN2_PIN, 0);
    LV_LOG_USER("Motor pin init %d: %d", MOTOR_IN2_PIN, mcp23017_digitalRead(&mcp, MOTOR_IN2_PIN));
#endif

    /* ENA on ESP32 GPIO via LEDC PWM (8-bit, 5kHz) */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = MOTOR_LEDC_MODE,
        .timer_num        = MOTOR_LEDC_TIMER,
        .duty_resolution  = MOTOR_LEDC_RESOLUTION,
        .freq_hz          = MOTOR_LEDC_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t ch_conf = {
        .speed_mode = MOTOR_LEDC_MODE,
        .channel    = MOTOR_LEDC_CHANNEL,
        .timer_sel  = MOTOR_LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = MOTOR_ENA_PIN,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

    LV_LOG_USER("Motor ENA pin %d LEDC init OK", MOTOR_ENA_PIN);
}

void stopMotor(uint8_t pin1, uint8_t pin2){
  for (uint8_t dutyCycle = sys.analogVal_rotationSpeedPercent; dutyCycle >= MOTOR_MIN_ANALOG_VAL; dutyCycle--) {
    motor_ledc_set_duty(dutyCycle);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  motor_dir_set(pin1, 0);
  motor_dir_set(pin2, 0);
  motor_ledc_set_duty(0);
  LV_LOG_USER("Run stopMotor");
}

void runMotorFW(uint8_t pin1, uint8_t pin2){
  motor_dir_set(pin1, 1);
  motor_dir_set(pin2, 0);

  for (uint8_t dutyCycle = MOTOR_MIN_ANALOG_VAL; dutyCycle <= sys.analogVal_rotationSpeedPercent; dutyCycle++) {
    motor_ledc_set_duty(dutyCycle);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  LV_LOG_USER("Run runMotorFW at speed %d", sys.analogVal_rotationSpeedPercent);
}

void runMotorRV(uint8_t pin1, uint8_t pin2){
  motor_dir_set(pin1, 0);
  motor_dir_set(pin2, 1);

  for (uint8_t dutyCycle = MOTOR_MIN_ANALOG_VAL; dutyCycle <= sys.analogVal_rotationSpeedPercent; dutyCycle++) {
    motor_ledc_set_duty(dutyCycle);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  LV_LOG_USER("Run runMotorRV at speed %d", sys.analogVal_rotationSpeedPercent);
}

void setMotorSpeed(uint8_t pin, uint8_t spd){
  motor_ledc_set_duty(spd);
  LV_LOG_USER("Set motor speed: %d", spd);
}

void setMotorSpeedUp(uint8_t pin, uint8_t spd){
  for (uint8_t i = 0; i <= spd; i++) {
    motor_ledc_set_duty(i);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  LV_LOG_USER("Increase speed to: %d", spd);
}

void setMotorSpeedDown(uint8_t pin, uint8_t spd){
  for (int16_t i = spd; i >= 0; --i) {
    motor_ledc_set_duty((uint8_t)i);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  LV_LOG_USER("Decrease speed to: %d", spd);
}




//++++++++++++++++ READ TEMPERATURE SENSOR METHODS ++++++++++++++++



void initializeTemperatureSensor()
{
}


void testPin(uint8_t pin){
    mcp23017_digitalWrite(&mcp, pin, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    mcp23017_digitalWrite(&mcp, pin, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
}

/* ═══════════════════════════════════════════════════
 * Temperature control wrappers
 * On real hardware (ESP32), these will call the actual sensor/relay.
 * In the simulator, src/main.c provides non-weak overrides.
 * ═══════════════════════════════════════════════════ */
#ifndef SIMULATOR_BUILD
float sim_getTemperature(uint8_t sensorIndex) {
    float temp = -255.0f;

    if (ds_bus.initialized && sensorIndex < ds_bus.sensor_count) {
        if (ds18b20_read_temp(&ds_bus, sensorIndex, &temp) != ESP_OK) {
            LV_LOG_USER("DS18B20 read error sensor[%d]", sensorIndex);
            temp = -255.0f;
        }
    }

    /* Apply calibration offset to temperature reading */
    extern struct gui_components gui;
    float offset = gui.page.settings.settingsParams.tempCalibOffset / 10.0f;
    temp += offset;

    return temp;
}

void sim_setHeater(bool on) {
    mcp23017_digitalWrite(&mcp, HEATER_RLY, on ? 1 : 0);
    LV_LOG_USER("Heater %s (pin %d = %d)", on ? "ON" : "OFF", HEATER_RLY, mcp23017_digitalRead(&mcp, HEATER_RLY));
}

void sim_resetTemperatures(void) {
    /* No-op on real hardware */
}
#endif

void toLowerCase(char *str) {
    while (*str) {
        *str = tolower((int)*str);
        str++;
    }
}

uint8_t caseInsensitiveStrstr(const char *haystack, const char *needle) {

    char *haystackLower = heap_caps_malloc(strlen(haystack) + 1, MALLOC_CAP_INTERNAL);
    char *needleLower = heap_caps_malloc(strlen(needle) + 1, MALLOC_CAP_INTERNAL);

    if (haystackLower == NULL || needleLower == NULL) {
        if (haystackLower) free(haystackLower);
        if (needleLower) free(needleLower);
        return 0;
    }

    strcpy(haystackLower, haystack);
    strcpy(needleLower, needle);

    toLowerCase(haystackLower);
    toLowerCase(needleLower);

    uint8_t result = strstr(haystackLower, needleLower) != NULL;

    free(haystackLower);
    free(needleLower);

    return result;
}


void filterAndDisplayProcesses() {
    processNode *currentNode = gui.page.processes.processElementsList.start;
    int32_t displayedCount = 1;

    if (gui.page.processes.isFiltered == 1)
        removeFiltersAndDisplayAllProcesses();

    // Debugging info
    LV_LOG_USER("Filter %s, %d, %d, %d", 
                gui.element.filterPopup.filterName,
                gui.element.filterPopup.isColorFilter, 
                gui.element.filterPopup.isBnWFilter, 
                gui.element.filterPopup.preferredOnly);

    while (currentNode != NULL) {
        bool isFiltered = true;

        // Check name filter
        if (strlen(gui.element.filterPopup.filterName) > 0) {
            if (!caseInsensitiveStrstr(currentNode->process.processDetails->data.processNameString, gui.element.filterPopup.filterName)) {
                isFiltered = false;
            }
        }

        // Check film type filter (Color and/or BnW)
        if (gui.element.filterPopup.isColorFilter && gui.element.filterPopup.isBnWFilter) {
            /* Both selected — show all film types (no filtering needed) */
        } else if (gui.element.filterPopup.isColorFilter) {
            if (currentNode->process.processDetails->data.filmType != COLOR_FILM) {
                isFiltered = false;
            }
        } else if (gui.element.filterPopup.isBnWFilter) {
            if (currentNode->process.processDetails->data.filmType != BLACK_AND_WHITE_FILM) {
                isFiltered = false;
            }
        }

        // Check preferred status filter
        if (gui.element.filterPopup.preferredOnly) {
            if (!currentNode->process.processDetails->data.isPreferred) {
                isFiltered = false;
            }
        }

        currentNode->process.isFiltered = isFiltered;
        currentNode = currentNode->next;
    }

    lv_obj_clean(gui.page.processes.processesListContainer);   
    lv_obj_update_layout(gui.page.processes.processesListContainer);

    currentNode = gui.page.processes.processElementsList.start;
    while (currentNode != NULL) { 
        if (currentNode->process.isFiltered == true) {
            processElementCreate(currentNode, displayedCount);
            displayedCount++;
        }
        currentNode = currentNode->next;
    }

    gui.page.processes.isFiltered = 1;
}


void removeFiltersAndDisplayAllProcesses() {
    processNode *currentNode = gui.page.processes.processElementsList.start;
    if (currentNode == NULL) {
        LV_LOG_USER("processElementsList.start is NULL");
        return;
    }

    int32_t displayedCount = 1;  // Initialize count

    // Pulizia del contenitore della lista dei processi per prepararlo alla visualizzazione di tutti i processi
    lv_obj_clean(gui.page.processes.processesListContainer);
    lv_obj_update_layout(gui.page.processes.processesListContainer);
    // Itera attraverso tutti i nodi dei processi e li visualizza
    while (currentNode != NULL) {
        LV_LOG_USER("Process %"PRIi32" created",displayedCount);
        processElementCreate(currentNode, displayedCount);
        currentNode = currentNode->next;
        displayedCount++;
    }

    // Aggiorna il layout del contenitore per riallineare gli elementi visibili
    lv_obj_update_layout(gui.page.processes.processesListContainer);
}

/* ── Sub-object destroy functions (Point 4 refactor) ────────────── */

void step_detail_destroy(sStepDetail *sd) {
    if (sd == NULL) return;
    /* Reset runtime style allocated during stepDetail() creation */
    lv_style_reset(&sd->style_mBoxStepPopupTitleLine);
    free(sd);
}

void checkup_destroy(sCheckup *ckup) {
    if (ckup == NULL) return;
    /* Stop all timers owned by this checkup */
    safeTimerDelete(&ckup->processTimer);
    safeTimerDelete(&ckup->pumpTimer);
    safeTimerDelete(&ckup->tempTimer);
    /* Reset runtime style */
    lv_style_reset(&ckup->textAreaStyleCheckup);
    free(ckup);
}

void process_detail_destroy(sProcessDetail *pd) {
    if (pd == NULL) return;
    /* Null out LVGL pointers to prevent stale access from async callbacks */
    pd->processTotalTimeValue = NULL;
    pd->processDetailParent   = NULL;
    /* Reset runtime style allocated during processDetail() creation */
    lv_style_reset(&pd->textAreaStyle);
    /* Free all steps in the step list */
    stepNode *s = pd->stepElementsList.start;
    while (s != NULL) {
        stepNode *next = s->next;
        step_node_destroy(s);
        s = next;
    }
    pd->stepElementsList.start = NULL;
    pd->stepElementsList.end = NULL;
    pd->stepElementsList.size = 0;
    /* Free checkup */
    checkup_destroy(pd->checkup);
    pd->checkup = NULL;
    /* Free self */
    free(pd);
}

/* ── Node-level destroy functions (use sub-object destroyers) ──── */

void step_node_destroy(stepNode *node) {
    if (node == NULL) return;
    step_detail_destroy(node->step.stepDetails);
    node->step.stepDetails = NULL;
    free(node);
}

void process_node_destroy(processNode *node) {
    if (node == NULL) return;
    process_detail_destroy(node->process.processDetails);
    node->process.processDetails = NULL;
    free(node);
}

void emptyList(void *list, NodeType_t type) {
    if (list == NULL) {
        return;
    }

    if (type == PROCESS_NODE) {
        processList *plist = (processList *)list;

        /* Pre-cleanup: invalidate globals that may reference nodes about to be freed */
        processNode *scan = plist->start;
        while (scan != NULL) {
            if (scan == gui.tempProcessNode) {
                gui.tempProcessNode = NULL;
            }
            /* Stop any running timers before freeing the node */
            if (scan->process.processDetails != NULL &&
                scan->process.processDetails->checkup != NULL) {
                safeTimerDelete(&scan->process.processDetails->checkup->processTimer);
                safeTimerDelete(&scan->process.processDetails->checkup->pumpTimer);
                safeTimerDelete(&scan->process.processDetails->checkup->tempTimer);
            }
            scan = scan->next;
        }

        processNode *currentNode = plist->start;
        while (currentNode != NULL) {
            processNode *nextNode = currentNode->next;
            process_node_destroy(currentNode);
            currentNode = nextNode;
        }

        plist->start = NULL;
        plist->end = NULL;
        plist->size = 0;

    } else if (type == STEP_NODE) {
        stepList *slist = (stepList *)list;

        /* Pre-cleanup: invalidate globals that may reference step nodes about to be freed */
        stepNode *scan = slist->start;
        while (scan != NULL) {
            if (scan == gui.tempStepNode) {
                gui.tempStepNode = NULL;
            }
            scan = scan->next;
        }

        stepNode *currentNode = slist->start;
        while (currentNode != NULL) {
            stepNode *nextNode = currentNode->next;
            step_node_destroy(currentNode);
            currentNode = nextNode;
        }

        slist->start = NULL;
        slist->end = NULL;
        slist->size = 0;
    }
}

void readMachineStats(machineStatistics *machineStats) {
    /* Stats are now read as part of readConfigFile — nothing extra needed */
    (void)machineStats;
}

void writeMachineStats(machineStatistics *machineStats) {
    /* Rewrite the full config file which now includes stats at the end */
    (void)machineStats;
    writeConfigFile(FILENAME_SAVE, false);
}


uint32_t findRollerStringIndex(const char *target, const char *array) {
    const char *start = array;
    uint32_t index = 0;

    while (*start != '\0') {
        // Find the end of the current string
        const char *end = strchr(start, '\n');
        if (end == NULL) {
            end = start + strlen(start);  // If we don't find newline, take until the end of the string
        }

        // Length of the current string
        uint32_t length = end - start;

        // Compare the substring
        if (strncmp(start, target, length) == 0 && target[length] == '\0') {
            return index;
        }

        // Move to the next line
        start = end;
        if (*start == '\n') {
            start++;  // Skip the newline if we find it
        }
        index++;
    }

    return -1; // String not found
}

char* getRollerStringIndex(uint32_t index, const char *list) {
    const char *start = list;
    const char *end;
    uint32_t currentIndex = 0;

    while ((end = strchr(start, '\n')) != NULL) {
        if (currentIndex == index) {
            uint32_t length = end - start;
            char *result = (char*) malloc((length + 1) * sizeof(char));
            if (result == NULL) {
                return NULL; // Allocation failed
            }
            snprintf(result, length + 1, "%.*s", (int)length, start);
            return result;
        }
        start = end + 1;
        currentIndex++;
    }

    // Check for the last item in the list (without a trailing newline)
    if (currentIndex == index) {
        uint32_t length = strlen(start);
        char *result = (char*) malloc((length + 1) * sizeof(char));
        if (result == NULL) {
            return NULL; // Allocation failed
        }
        strcpy(result, start);
        return result;
    }

    // If index is out of bounds
    return NULL;
}

char* generateRandomSuffix(const char* baseName) {
    static char newProcessName[MAX_PROC_NAME_LEN + 1]; // Include space for the null terminator

    // Check if baseName already has a numeric suffix
    size_t len = strlen(baseName);
    uint8_t suffix = 1; // Initial suffix

    // Calculate the maximum possible length for the base name (without suffix)
    size_t maxBaseLen = MAX_PROC_NAME_LEN - 4; // 4 characters for "_000"

    if (len > maxBaseLen) {
        // If baseName is longer than the maximum allowed for the base name, shorten it
        memcpy(newProcessName, baseName, maxBaseLen);
        newProcessName[maxBaseLen] = '\0';
        len = maxBaseLen; // Update the effective length of the new base name
    } else {
        // Otherwise copy it directly
        memcpy(newProcessName, baseName, len);
        newProcessName[len] = '\0';
    }

    // Check if baseName already has a valid numeric suffix
    if (len > 4 && baseName[len - 4] == '_' && isdigit((int)baseName[len - 3]) && isdigit((int)baseName[len - 2]) && isdigit((int)baseName[len - 1])) {
        // If baseName already has a numeric suffix, extract the current suffix
        suffix = atoi(&baseName[len - 3]); // Get the numeric suffix
        newProcessName[len - 4] = '\0'; // Remove the numeric suffix to create the new base name
    }

    bool uniqueNameFound = false;

    while (!uniqueNameFound) {
        // Create the new suffix
        char suffixStr[5]; // 4 characters for "_000" + 1 for the null terminator
        snprintf(suffixStr, sizeof(suffixStr), "_%03d", suffix); // Format the suffix to three digits with leading zeros if necessary

        // Concatenate the suffix to the new name
        strncat(newProcessName, suffixStr, sizeof(newProcessName) - strlen(newProcessName) - 1);

        // Check if the new name with suffix is unique in the list
        uniqueNameFound = true;
        processNode *current = gui.page.processes.processElementsList.start;
        while (current != NULL) {
            if (strcmp(current->process.processDetails->data.processNameString, newProcessName) == 0) {
                uniqueNameFound = false;
                suffix++; // Increment the suffix
                // Remove the added suffix to restart the loop and try with a new suffix
                size_t baseLen = strlen(newProcessName) - 4; // Length of the base name without the suffix
                newProcessName[baseLen] = '\0'; // Terminate the string after the base name
                break;
            }
            current = current->next;
        }
    }

    return newProcessName;
}

sStepDetail *deepCopyStepDetail(sStepDetail *original) {
    if (original == NULL) return NULL;
    sStepDetail *copy = (sStepDetail*)malloc(sizeof(sStepDetail));
    if (copy == NULL) return NULL;

    /* Zero entire struct (all UI pointers become NULL) then copy business data */
    memset(copy, 0, sizeof(sStepDetail));
    copy->data = original->data;
    /* Context structs (nameKeyboardCtx, minRollerCtx, secRollerCtx) are already
       zeroed by the memset above — they're in sStepDetail, not sStepData */

    return copy;
}

/**
 * Clone a singleStep in place: zero dst, then deep copy business data from src.
 * Returns true on success, false on failure (dst is left zeroed on failure).
 */
bool single_step_clone(const singleStep *src, singleStep *dst) {
    if (src == NULL || dst == NULL) return false;

    /* Zero everything (NULLs all LVGL pointers, resets runtime flags) */
    memset(dst, 0, sizeof(singleStep));

    /* Deep copy the business data sub-struct */
    dst->stepDetails = deepCopyStepDetail((sStepDetail *)src->stepDetails);
    if (dst->stepDetails == NULL && src->stepDetails != NULL) {
        return false;
    }
    return true;
}

stepNode *deepCopyStepNode(stepNode *original) {
    if (original == NULL) return NULL;
    stepNode *copy = (stepNode*)malloc(sizeof(stepNode));
    if (copy == NULL) return NULL;

    /* Zero everything (NULLs next/prev pointers and all LVGL fields) */
    memset(copy, 0, sizeof(stepNode));

    /* Clone step content directly into copy — no intermediate allocation */
    if (!single_step_clone(&original->step, &copy->step)) {
        free(copy);
        return NULL;
    }
    /* next/prev are set by deepCopyStepList, not here */
    return copy;
}

stepList deepCopyStepList(stepList original) {
    stepList copy;
    copy.start = NULL;
    copy.end = NULL;
    copy.size = 0;

    stepNode *current = original.start;
    stepNode *lastCopied = NULL;

    while (current != NULL) {
        stepNode *newNode = deepCopyStepNode(current);
        if (newNode == NULL) {
            /* Rollback: free all already-copied nodes */
            stepNode *cleanup = copy.start;
            while (cleanup != NULL) {
                stepNode *tmp = cleanup->next;
                step_node_destroy(cleanup);
                cleanup = tmp;
            }
            copy.start = NULL;
            copy.end = NULL;
            copy.size = 0;
            return copy;
        }

        newNode->prev = lastCopied;
        newNode->next = NULL;

        if (lastCopied != NULL) {
            lastCopied->next = newNode;
        } else {
            copy.start = newNode;  /* First node */
        }

        lastCopied = newNode;
        copy.size++;
        current = current->next;
    }

    copy.end = lastCopied;
    return copy;
}

sCheckup *deepCopyCheckup(sCheckup *original) {
    if (original == NULL) return NULL;
    sCheckup *copy = (sCheckup*)malloc(sizeof(sCheckup));
    if (copy == NULL) return NULL;

    /* Zero entire struct (all UI pointers + timers become NULL) then copy business data */
    memset(copy, 0, sizeof(sCheckup));
    copy->data = original->data;

    return copy;
}

sProcessDetail *deepCopyProcessDetail(sProcessDetail *original) {
    if (original == NULL) return NULL;
    sProcessDetail *copy =  (sProcessDetail*)malloc(sizeof(sProcessDetail));
    if (copy == NULL) return NULL;

    /* Zero entire struct (all UI pointers become NULL) then copy business data */
    memset(copy, 0, sizeof(sProcessDetail));
    copy->data = original->data;
    /* Context structs (nameKeyboardCtx, tempRollerCtx, toleranceRollerCtx) are already
       zeroed by the memset above — they're in sProcessDetail, not sProcessData */

    /* Deep copy sub-structures that need special handling */
    copy->stepElementsList = deepCopyStepList(original->stepElementsList);
    if (copy->stepElementsList.size != original->stepElementsList.size) {
        /* Step list copy failed (partial or total) — clean up */
        free(copy);
        return NULL;
    }

    copy->checkup = deepCopyCheckup(original->checkup);
    if (copy->checkup == NULL && original->checkup != NULL) {
        /* Checkup copy failed — free step list and copy */
        stepNode *cleanup = copy->stepElementsList.start;
        while (cleanup != NULL) {
            stepNode *tmp = cleanup->next;
            step_node_destroy(cleanup);
            cleanup = tmp;
        }
        free(copy);
        return NULL;
    }

    return copy;
}

/**
 * Clone a singleProcess in place: zero dst, then deep copy business data from src.
 * Returns true on success, false on failure (dst is left zeroed on failure).
 */
bool single_process_clone(const singleProcess *src, singleProcess *dst) {
    if (src == NULL || dst == NULL) return false;

    /* Zero everything (NULLs all LVGL pointers, resets runtime flags) */
    memset(dst, 0, sizeof(singleProcess));

    /* Deep copy the business data sub-struct */
    dst->processDetails = deepCopyProcessDetail((sProcessDetail *)src->processDetails);
    if (dst->processDetails == NULL && src->processDetails != NULL) {
        return false;
    }
    return true;
}

struct processNode *deepCopyProcessNode(struct processNode *original) {
    if (original == NULL) return NULL;
    struct processNode *copy = (processNode*)malloc(sizeof(struct processNode));
    if (copy == NULL) return NULL;

    /* Zero everything (NULLs next/prev pointers and all LVGL fields) */
    memset(copy, 0, sizeof(struct processNode));

    /* Clone process content directly into copy — no intermediate allocation */
    if (!single_process_clone(&original->process, &copy->process)) {
        free(copy);
        return NULL;
    }
    /* next/prev are set by caller, not here */
    return copy;
}

uint8_t getValueForChemicalSource(uint8_t source) {
    switch (source) {
        case C1:
            return C1_RLY;
        case C2:
            return C2_RLY;
        case C3:
            return C3_RLY;
        case WASTE:
            return WASTE_RLY;
        case WB:
            return WB_RLY; // if is water, the drain is always on waste
        default:
            return 255;
    }
}

void getMinutesAndSeconds(uint8_t containerFillingTime, const bool containerToClean[3]) {
    // Multiply containerFillingTime by 2
    uint32_t totalTime = (containerFillingTime * 2) * gui.element.cleanPopup.cleanCycles;

    // Count the number of selected containers
    uint8_t selectedContainersCount = 0;
    for (uint8_t i = 0; i < 3; ++i) {
        if (containerToClean[i]) {
            selectedContainersCount++;
        }
    }

    // Multiply total time by the number of selected containers
    totalTime *= selectedContainersCount;

    // Calculate minutes and seconds
    gui.element.cleanPopup.totalMins = totalTime / 60;
    gui.element.cleanPopup.totalSecs = totalTime % 60;
}


uint8_t getRandomRotationInterval() {
	
    uint8_t baseTime = gui.page.settings.settingsParams.rotationIntervalSetpoint;
    uint8_t randomPercentage = gui.page.settings.settingsParams.randomSetpoint;

    // Calculate percentage
    uint8_t variation = (baseTime * randomPercentage) / 100;
    
    // Calculate the intervals
    uint8_t minValue = baseTime - variation; // Minimum value that can be returned
//    uint8_t maxValue = baseTime;             // Maximum value that can be returned

    // Calculate the offset between [-variation, variation]
    int8_t randomOffset = (rand() % (2 * variation + 1)) - variation; // [ -variation, variation ]

    uint8_t result = minValue + randomOffset;

    // Limit values between 5 and 60 seconds, the min and max value for rotationIntervalSetpoint.
    if (result > 60) {
        result = 60;
    }
    if (result < 5) {
        result = 5;
    }

    LV_LOG_USER("baseTime: %d, randomPercentage: %d, variation: %d, randomOffset: %d, result: %d",
                baseTime, randomPercentage, variation, randomOffset, result);

    return result;
}

void pwmLedTest(){
   LV_LOG_USER("pwmLedTest");
   motor_dir_set(MOTOR_IN1_PIN, 0);
   motor_dir_set(MOTOR_IN2_PIN, 1);

   for (uint16_t dutyCycle = MOTOR_MIN_ANALOG_VAL; dutyCycle <= MOTOR_MAX_ANALOG_VAL; dutyCycle++) {
    motor_ledc_set_duty(dutyCycle);
    LV_LOG_USER("dutyCycle %d", dutyCycle);
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  for (uint8_t dutyCycle = MOTOR_MAX_ANALOG_VAL; dutyCycle >= MOTOR_MIN_ANALOG_VAL; dutyCycle--) {
    motor_ledc_set_duty(dutyCycle);
    LV_LOG_USER("dutyCycle %d", dutyCycle);
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  motor_dir_set(MOTOR_IN1_PIN, 0);
  motor_dir_set(MOTOR_IN2_PIN, 0);
  motor_ledc_set_duty(0);
}

uint8_t mapPercentageToValue(uint8_t percentage, uint8_t minPercent, uint8_t maxPercent) {
    uint8_t value = ((percentage - minPercent) * (MOTOR_MAX_ANALOG_VAL - MOTOR_MIN_ANALOG_VAL)) / (maxPercent - minPercent) + MOTOR_MIN_ANALOG_VAL;
    return value;
}

uint16_t calculateFillTime(uint16_t capacityMl, uint8_t pumpSpeedPercent) {
    if (pumpSpeedPercent == 0) pumpSpeedPercent = 30;
    /* Pump max = 15L/min = 250 ml/s */
    /* flow = 250 * pumpSpeed / 100 */
    /* time = capacity / flow = capacity * 100 / (250 * pumpSpeed) */
    uint32_t timeMs = (uint32_t)capacityMl * 100 * 1000 / (250 * pumpSpeedPercent);
    uint16_t timeSec = (timeMs + 500) / 1000; /* round to nearest second */
    if (timeSec < 1) timeSec = 1;
    return timeSec;
}

uint16_t getContainerFillTime(void) {
    uint16_t ml = gui.page.settings.settingsParams.chemContainerMl;
    uint8_t spd = gui.page.settings.settingsParams.pumpSpeed;
    if (ml == 0) ml = 500;
    if (spd == 0) spd = 30;
    return calculateFillTime(ml, spd);
}

uint16_t getWbFillTime(void) {
    uint16_t ml = gui.page.settings.settingsParams.wbContainerMl;
    uint8_t spd = gui.page.settings.settingsParams.pumpSpeed;
    if (ml == 0) ml = 2000;
    if (spd == 0) spd = 30;
    return calculateFillTime(ml, spd);
}

static uint16_t process_list_fill_time_seconds(void) {
    uint8_t volume_index = gui.page.settings.settingsParams.chemistryVolume;
    uint8_t tank_size = gui.page.settings.settingsParams.tankSize;
    uint16_t table[2][3][2] = tanksSizesAndTimes;

    if(volume_index < 1 || volume_index > 2) volume_index = 2;
    if(tank_size < 1 || tank_size > 3) tank_size = 2;

    return table[volume_index - 1][tank_size - 1][1];
}

static uint32_t process_list_overlap_credit_seconds(uint16_t fill_time_seconds) {
    uint32_t overlap_pct = gui.page.settings.settingsParams.drainFillOverlapSetpoint;
    if (overlap_pct > 100U) overlap_pct = 100U;
    return (((uint32_t)fill_time_seconds * 2U) * overlap_pct) / 100U;
}

static uint32_t process_list_adjusted_processing_seconds(const stepNode *step, uint16_t fill_time_seconds) {
    if(step == NULL || step->step.stepDetails == NULL) return 0U;

    uint32_t recipe_seconds = ((uint32_t)step->step.stepDetails->data.timeMins * 60U) +
                              (uint32_t)step->step.stepDetails->data.timeSecs;
    uint32_t overlap_credit = process_list_overlap_credit_seconds(fill_time_seconds);
    return (recipe_seconds > overlap_credit) ? (recipe_seconds - overlap_credit) : 0U;
}

static uint32_t process_list_estimated_runtime_seconds(const processNode *process) {
    if(process == NULL || process->process.processDetails == NULL) return 0U;

    uint16_t fill_time_seconds = process_list_fill_time_seconds();
    uint32_t total_seconds = 0U;

    for(stepNode *step = process->process.processDetails->stepElementsList.start; step != NULL; step = step->next) {
        total_seconds += ((uint32_t)fill_time_seconds * 2U);
        total_seconds += process_list_adjusted_processing_seconds(step, fill_time_seconds);
    }

    return total_seconds;
}

static void process_list_set_time_label(processNode *process) {
    if(process == NULL || process->process.processTime == NULL || process->process.processDetails == NULL) return;

    uint32_t estimated_seconds = process_list_estimated_runtime_seconds(process);
    int steps = process->process.processDetails->stepElementsList.size;
    lv_label_set_text_fmt(process->process.processTime,
        "%" PRIu32 "m%" PRIu8 "s / ~%" PRIu32 "m%" PRIu32 "s - %d step%s",
        process->process.processDetails->data.timeMins,
        process->process.processDetails->data.timeSecs,
        estimated_seconds / 60U,
        estimated_seconds % 60U,
        steps, steps == 1 ? "" : "s");
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Alarm  (moved from alarm.c)
 *  The simulator keeps its own implementation in src/main.c.
 * ═══════════════════════════════════════════════════════════════════════════ */
#ifndef SIMULATOR_BUILD

static lv_timer_t *s_alarm_timer = NULL;

void buzzer_beep(void)
{
    /*
     * Hardware buzzer implementation is not wired here yet.
     * Keep this as a safe no-op so alarm logic links and runs.
     */
}

static void alarm_timer_cb(lv_timer_t *t)
{
    (void)t;
    buzzer_beep();
}

void alarm_start_persistent(void)
{
    buzzer_beep();

    if (gui.page.settings.settingsParams.isPersistentAlarm)
    {
        if (s_alarm_timer == NULL)
        {
            s_alarm_timer = lv_timer_create(alarm_timer_cb, 10000, NULL);
        }
        else
        {
            lv_timer_resume(s_alarm_timer);
        }
    }
}

void alarm_stop(void)
{
    if (s_alarm_timer != NULL)
    {
        lv_timer_pause(s_alarm_timer);
    }
}

bool alarm_is_active(void)
{
    return (s_alarm_timer != NULL) && (lv_timer_get_paused(s_alarm_timer) == false);
}

#endif /* SIMULATOR_BUILD */
