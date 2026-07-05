/**
 * @file page_settings.c
 *
 */


//ESSENTIAL INCLUDE
#include "FilMachine.h"
#if defined(DISPLAY_DRIVER_ST7701)
#include "st7701_lcd.h"
#endif

extern struct gui_components gui;
extern struct sys_components sys;

static uint8_t current_value;
static uint8_t new_value;


uint8_t minVal_rotationSpeedPercent;
uint8_t maxVal_rotationSpeedPercent;
uint8_t analogVal_rotationSpeedPercent;

#define UI_SETTINGS                  (&ui_get_profile()->settings)
#define SETTINGS_LEFT_X              (UI_SETTINGS->row_left_x)
#define SETTINGS_GAP_Y               (UI_SETTINGS->row_gap_y)

#define SETTINGS_H_ROW               (UI_SETTINGS->row_h)
#define SETTINGS_H_SLIDER            (UI_SETTINGS->slider_h)

#define Y_TEMP_UNIT                  (-25)
#define Y_TEMP_TUNING                (Y_TEMP_UNIT + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_AUTOSTART                  (Y_TEMP_TUNING + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_WATER_INLET                (Y_AUTOSTART + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_FILM_ROT_SPEED             (Y_WATER_INLET + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_FILM_ROT_INTERVAL          (Y_FILM_ROT_SPEED + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_FILM_RANDOM                (Y_FILM_ROT_INTERVAL + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_DRAIN_FILL                 (Y_FILM_RANDOM + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_MULTI_RINSE                (Y_DRAIN_FILL + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_PUMP_SPEED                 (Y_MULTI_RINSE + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_INVERT_PUMP                (Y_PUMP_SPEED + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#if defined(DISPLAY_DRIVER_ST7701)
#define Y_BRIGHTNESS                 (Y_INVERT_PUMP + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_VOLUME                     (Y_BRIGHTNESS + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_PERSISTENT_ALARM           (Y_VOLUME + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#else
#define Y_VOLUME                     (Y_INVERT_PUMP + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_PERSISTENT_ALARM           (Y_VOLUME + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#endif
#define Y_TANK_SIZE                  (Y_PERSISTENT_ALARM + SETTINGS_H_ROW + SETTINGS_GAP_Y)
/* Chem/WB capacity rows removed — fill times are now self-calibrated from the
 * MIN/MAX sensors (Maintenance fills). Chem volume follows tank size directly. */
#define Y_CHEM_VOLUME                (Y_TANK_SIZE + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_SPLASH_SCREEN              (Y_CHEM_VOLUME + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_WIFI_ROW                   (Y_SPLASH_SCREEN + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_RESET_ROW                  (Y_WIFI_ROW + SETTINGS_H_ROW + SETTINGS_GAP_Y)

//ACCESSORY INCLUDES

static void settings_refresh_process_cards(void)
{
    /* Iterate the process linked list directly — safer than iterating
       container children, which may include non-process-card objects
       after CRUD operations or filtering. */
    processNode *node = gui.page.processes.processElementsList.start;
    while (node) {
        if (node->process.processElement) {
            lv_obj_send_event(node->process.processElement, LV_EVENT_REFRESH, NULL);
        }
        node = node->next;
    }
}

void event_settings_style_delete(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_DELETE) {
        //list of all styles to be reset, so clean the memory.
        lv_style_reset(&gui.page.settings.style_sectionTitleLine);

    }
}

void event_settingPopupMBox(lv_event_t * e){
    lv_obj_t * data = (lv_obj_t *)lv_event_get_user_data(e);

    if(data == gui.page.settings.tempSensorTuningLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,tempAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.filmRotationSpeedLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,filmRotationSpeedAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.filmRotationInverseIntervalLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,rotationInverseIntervalAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.filmRotationRandomLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,filmRotationRandomAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.persistentAlarmLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,soundAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.autostartLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,autostartAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.drainFillTimeLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,drainFillTimeAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.pumpSpeedLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,pumpSpeedAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.invertPumpLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,invertPumpAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.brightnessLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,brightnessAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.volumeLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,volumeAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.chemContainerMlLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,chemContainerMlAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.wbContainerMlLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,wbContainerMlAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.chemVolumeLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,chemistryVolumeAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.multiRinseTimeLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,multiRinseTimeAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.waterInletLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,waterInletAlertMBox_text,NULL,NULL,NULL);
    }
    if(data == gui.page.settings.splashLabel) {
        messagePopupCreate(messagePopupDetailTitle_text,splashScreenAlertMBox_text,NULL,NULL,NULL);
    }
}


/* Restore every setting to its factory default, refresh the UI and persist.
 * Called from the reset-confirmation popup (OK button). */
void settingsApplyFactoryDefaults(void)
{
    struct machineSettings *p = &gui.page.settings.settingsParams;

    /* Same values as initGlobals in accessories.c */
    p->tempUnit = 0;               /* Celsius */
    p->waterInlet = false;
    p->filmRotationSpeedSetpoint = 50;
    p->rotationIntervalSetpoint = 10;
    p->randomSetpoint = 0;
    p->isPersistentAlarm = false;
    p->isProcessAutostart = false;
    p->drainFillOverlapSetpoint = 100;
    p->multiRinseTime = 60;
    p->tankSize = 2;               /* Medium */
    p->pumpSpeed = 30;
    p->invertPump = false;
    p->brightness = 100;
    p->volume = 60;
    p->chemistryVolume = 2;         /* High */
    p->tempCalibOffset = 0;
    p->chemCalibOffset = 0;
    p->chemCalibFillSecs = 0;       /* uncalibrated (was chemContainerMl) */
    p->wbCalibFillSecs = 0;         /* uncalibrated (was wbContainerMl) */

    refreshSettingsUI();            /* update all UI widgets */
    qSysAction(SAVE_PROCESS_CONFIG);/* persist to SD */
    LV_LOG_USER("Settings restored to factory defaults");
}

void event_settings_handler(lv_event_t * e)
{
    uint32_t * active_id = (uint32_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    lv_obj_t * act_cb = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * old_cb = active_id ? (lv_obj_t *)lv_obj_get_child(cont, *active_id) : NULL;

    /*Do nothing if the container was clicked*/

    if(act_cb == cont && cont != gui.page.settings.waterInletSwitch && cont != gui.page.settings.tempSensorTuneButton && cont != gui.page.settings.filmRotationSpeedSlider && cont != gui.page.settings.filmRotationInversionIntervalSlider && cont != gui.page.settings.filmRandomSlider && cont != gui.page.settings.persistentAlarmSwitch && cont != gui.page.settings.invertPumpSwitch && cont != gui.page.settings.autostartSwitch && cont != gui.page.settings.drainFillTimeSlider && cont != gui.page.settings.multiRinseTimeSlider && cont != gui.page.settings.tankSizeTextArea && cont != gui.page.settings.pumpSpeedSlider && cont != gui.page.settings.brightnessSlider && cont != gui.page.settings.chemContainerMlTextArea && cont != gui.page.settings.wbContainerMlTextArea && cont != gui.page.settings.chemVolumeTextArea && cont != gui.page.settings.splashButton && cont != gui.page.settings.wifiButton && cont != gui.page.settings.resetButton)
      return;

    if(act_cb == gui.page.settings.tempUnitCelsiusRadioButton || act_cb == gui.page.settings.tempUnitFahrenheitRadioButton){
       if(code == LV_EVENT_CLICKED) {
            lv_obj_remove_state(old_cb, LV_STATE_CHECKED);
            lv_obj_add_state(act_cb, LV_STATE_CHECKED);
            *active_id = lv_obj_get_index(act_cb);
            LV_LOG_USER("Selected °C or °F: %d", (int)gui.page.settings.active_index);
            gui.page.settings.settingsParams.tempUnit = (int)gui.page.settings.active_index;
            qSysAction( SAVE_PROCESS_CONFIG );
            settings_refresh_process_cards();
       }
    }

    if(act_cb == gui.page.settings.waterInletSwitch){
      if(code == LV_EVENT_VALUE_CHANGED) {
          LV_LOG_USER("State Inlet: %s", lv_obj_has_state(act_cb, LV_STATE_CHECKED) ? "On" : "Off");
          gui.page.settings.settingsParams.waterInlet = lv_obj_has_state(act_cb, LV_STATE_CHECKED);
          qSysAction( SAVE_PROCESS_CONFIG );
        }
    }


    if(act_cb == gui.page.settings.tempSensorTuneButton){
      if(code == LV_EVENT_SHORT_CLICKED) {
          LV_LOG_USER("TUNE short click");
          calibPopupCreate();   /* dual bath+chemical calibration popup (Cancel/Reset/Set inside) */
        }
      /* Calibration reset moved into the popup (Reset button) — the old
       * long-press shortcut was hard to discover, so it was removed. */
    }

    if (act_cb == gui.page.settings.filmRotationSpeedSlider) {
        if (code == LV_EVENT_VALUE_CHANGED) {
            current_value = gui.page.settings.settingsParams.filmRotationSpeedSetpoint;
            new_value = lv_slider_get_value(act_cb);

            new_value = roundToStep(new_value, 10);

            minVal_rotationSpeedPercent = lv_slider_get_min_value(gui.page.settings.filmRotationSpeedSlider);
            maxVal_rotationSpeedPercent = lv_slider_get_max_value(gui.page.settings.filmRotationSpeedSlider);
            analogVal_rotationSpeedPercent = mapPercentageToValue(new_value, minVal_rotationSpeedPercent, maxVal_rotationSpeedPercent);
            sys.analogVal_rotationSpeedPercent = analogVal_rotationSpeedPercent;

            lv_slider_set_value(act_cb, new_value, LV_ANIM_OFF);
            lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e), "%d%%", new_value);
            gui.page.settings.settingsParams.filmRotationSpeedSetpoint = new_value;
            LV_LOG_USER("Film Speed Rotation : %d, with analog value %d", new_value, analogVal_rotationSpeedPercent);
        }
        if (code == LV_EVENT_RELEASED) {
            qSysAction(SAVE_PROCESS_CONFIG);
        }
    }


    if(act_cb == gui.page.settings.filmRotationInversionIntervalSlider){
        if(code == LV_EVENT_VALUE_CHANGED) {
            current_value = gui.page.settings.settingsParams.rotationIntervalSetpoint;
            new_value = lv_slider_get_value(act_cb);

            new_value = roundToStep(new_value, 10);

            lv_slider_set_value(act_cb, new_value, LV_ANIM_OFF);
            lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e), "%"PRIi32"sec", lv_slider_get_value(act_cb));
            gui.page.settings.settingsParams.rotationIntervalSetpoint = lv_slider_get_value(act_cb);
            LV_LOG_USER("Film Rotation Inversion Interval : %"PRIi32"",lv_slider_get_value(act_cb));
        }
        if(code == LV_EVENT_RELEASED) {
            qSysAction(SAVE_PROCESS_CONFIG);
        }
    }

    if(act_cb == gui.page.settings.filmRandomSlider){
        if(code == LV_EVENT_VALUE_CHANGED) {
            current_value = gui.page.settings.settingsParams.randomSetpoint;
            new_value = lv_slider_get_value(act_cb);

            new_value = roundToStep(new_value, 20);

            lv_slider_set_value(act_cb, new_value, LV_ANIM_OFF);
            lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e), "~%"PRIi32"%%", lv_slider_get_value(act_cb));
            gui.page.settings.settingsParams.randomSetpoint = lv_slider_get_value(act_cb);
            LV_LOG_USER("Film Randomness : %"PRIi32", for time: %"PRIu8"sec, is %"PRIu8"sec",lv_slider_get_value(act_cb),gui.page.settings.settingsParams.rotationIntervalSetpoint, getRandomRotationInterval());
        }
        if(code == LV_EVENT_RELEASED) {
            qSysAction(SAVE_PROCESS_CONFIG);
        }
     }

    if(act_cb == gui.page.settings.persistentAlarmSwitch){
      if(code == LV_EVENT_VALUE_CHANGED) {
          LV_LOG_USER("Persistent Alarm: %s", lv_obj_has_state(act_cb, LV_STATE_CHECKED) ? "On" : "Off");
          gui.page.settings.settingsParams.isPersistentAlarm = lv_obj_has_state(act_cb, LV_STATE_CHECKED);
          qSysAction( SAVE_PROCESS_CONFIG );
        }
    }

    if(act_cb == gui.page.settings.invertPumpSwitch){
      if(code == LV_EVENT_VALUE_CHANGED) {
          LV_LOG_USER("Invert Pump: %s", lv_obj_has_state(act_cb, LV_STATE_CHECKED) ? "On" : "Off");
          gui.page.settings.settingsParams.invertPump = lv_obj_has_state(act_cb, LV_STATE_CHECKED);
          qSysAction( SAVE_PROCESS_CONFIG );
        }
    }

    if(act_cb == gui.page.settings.autostartSwitch){
      if(code == LV_EVENT_VALUE_CHANGED) {
          LV_LOG_USER("Autostart : %s", lv_obj_has_state(act_cb, LV_STATE_CHECKED) ? "On" : "Off");
          gui.page.settings.settingsParams.isProcessAutostart = lv_obj_has_state(act_cb, LV_STATE_CHECKED);
          qSysAction( SAVE_PROCESS_CONFIG );
        }
    }


    if(act_cb == gui.page.settings.drainFillTimeSlider) {
        if(code == LV_EVENT_VALUE_CHANGED) {
            current_value = gui.page.settings.settingsParams.drainFillOverlapSetpoint;
            new_value = lv_slider_get_value(act_cb);

            new_value = roundToStep(new_value, 50);

            lv_slider_set_value(act_cb, new_value, LV_ANIM_OFF);
            lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e), "%"PRIi32"%%", lv_slider_get_value(act_cb));
            LV_LOG_USER("Drain/fill time overlap percent : %"PRIi32"",lv_slider_get_value(act_cb));
            gui.page.settings.settingsParams.drainFillOverlapSetpoint = lv_slider_get_value(act_cb);
            settings_refresh_process_cards();
        }
        if(code == LV_EVENT_RELEASED) {
            qSysAction(SAVE_PROCESS_CONFIG);
        }
    }


    if(act_cb == gui.page.settings.multiRinseTimeSlider) {
        if(code == LV_EVENT_VALUE_CHANGED) {
            current_value = gui.page.settings.settingsParams.multiRinseTime;
            new_value = lv_slider_get_value(act_cb);

            new_value = roundToStep(new_value, 30);

            // Ensure new_value is within valid bounds (assuming 60 to 180 as mentioned)
            if(new_value < 60) new_value = 60;
            if(new_value > 180) new_value = 180;

            lv_slider_set_value(act_cb, new_value, LV_ANIM_OFF);
            lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e), "%ds", new_value);
            LV_LOG_USER("Multi rinse cycle time (s): %d", new_value);
            gui.page.settings.settingsParams.multiRinseTime = new_value;
        }
        if(code == LV_EVENT_RELEASED) {
            qSysAction(SAVE_PROCESS_CONFIG);
        }
    }

    if(act_cb == gui.page.settings.tankSizeTextArea){
        if(code == LV_EVENT_FOCUSED) {
            if(gui.element.rollerPopup.mBoxRollerParent != NULL) return;
            LV_LOG_USER("Set Tank Size from Settings");
            rollerPopupCreate(checkupTankSizesList, checkupTankSize_text, act_cb, gui.page.settings.tankSize_active_index, ORANGE);
        }
    }

    if(act_cb == gui.page.settings.pumpSpeedSlider) {
        if(code == LV_EVENT_VALUE_CHANGED) {
            new_value = lv_slider_get_value(act_cb);
            new_value = roundToStep(new_value, 10);
            if(new_value < 10) new_value = 10;
            if(new_value > 100) new_value = 100;
            lv_slider_set_value(act_cb, new_value, LV_ANIM_OFF);
            lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e), "%d%%", new_value);
            LV_LOG_USER("Pump speed: %d%%", new_value);
            gui.page.settings.settingsParams.pumpSpeed = new_value;
        }
        if(code == LV_EVENT_RELEASED) {
            qSysAction(SAVE_PROCESS_CONFIG);
        }
    }

    if(act_cb == gui.page.settings.brightnessSlider) {
        if(code == LV_EVENT_VALUE_CHANGED) {
            new_value = lv_slider_get_value(act_cb);
            new_value = roundToStep(new_value, 10);
            if(new_value < 10) new_value = 10;
            if(new_value > 100) new_value = 100;
            lv_slider_set_value(act_cb, new_value, LV_ANIM_OFF);
            lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e), "%d%%", new_value);
            LV_LOG_USER("Brightness: %d%%", new_value);
            gui.page.settings.settingsParams.brightness = new_value;
#if defined(DISPLAY_DRIVER_ST7701)
            st7701_lcd_set_user_brightness(new_value);
#endif
        }
        if(code == LV_EVENT_RELEASED) {
            qSysAction(SAVE_PROCESS_CONFIG);
        }
    }

    /* Chem/WB capacity rollers removed — self-calibrated via Maintenance fills. */

    if(act_cb == gui.page.settings.chemVolumeTextArea) {
        if(code == LV_EVENT_FOCUSED) {
            if(gui.element.rollerPopup.mBoxRollerParent != NULL) return;
            LV_LOG_USER("Set Chemistry Volume");
            uint32_t idx = gui.page.settings.settingsParams.chemistryVolume >= 2 ? 1 : 0;
            rollerPopupCreate(chemistryVolumeList, chemistryVolume_text, act_cb, idx, ORANGE);
        }
    }

    /* ── Wi-Fi button → open popup ── */
    if(act_cb == gui.page.settings.wifiButton) {
        if(code == LV_EVENT_CLICKED) {
            LV_LOG_USER("PRESSED wifiButton");
            wifiPopupCreate();
        }
    }

    /* ── Splash Screen button → open popup ── */
    if(act_cb == gui.page.settings.splashButton) {
        if(code == LV_EVENT_CLICKED) {
            LV_LOG_USER("PRESSED splashButton");
            if(gui.element.splashPopup.splashPopupParent == NULL) {
                splashPopupCreate();
            } else {
                lv_obj_remove_flag(gui.element.splashPopup.splashPopupParent, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    /* ── Reset to Defaults button — confirm first, only reset on OK ── */
    if(act_cb == gui.page.settings.resetButton) {
        if(code == LV_EVENT_CLICKED) {
            LV_LOG_USER("PRESSED resetButton — asking confirmation");
            /* Cancel (left) closes; OK (right) triggers the actual reset via
             * the settings-reset owner in element_messagePopup.c. */
            messagePopupCreate(settingsResetConfirmTitle_text,
                               settingsResetConfirmBody_text,
                               buttonCancel_text, buttonOk_text,
                               gui.page.settings.resetButton);
        }
    }

}


static void initSettings_tempUnit(lv_obj_t *parent)
{
  gui.page.settings.tempUnitContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.tempUnitContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_TEMP_UNIT);  /* 1. Temp unit */
  lv_obj_set_size(gui.page.settings.tempUnitContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.tempUnitContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.tempUnitContainer, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(gui.page.settings.tempUnitContainer, event_settings_handler, LV_EVENT_CLICKED, &gui.page.settings.active_index);

        gui.page.settings.tempUnitCelsiusRadioButton = create_radiobutton(gui.page.settings.tempUnitContainer, celsius_text, -75, 0, UI_SETTINGS->radio_size, UI_SETTINGS->radio_font, lv_color_hex(ORANGE_DARK), lv_color_hex(ORANGE));
        gui.page.settings.tempUnitFahrenheitRadioButton = create_radiobutton(gui.page.settings.tempUnitContainer, fahrenheit_text, 5, 0, UI_SETTINGS->radio_size, UI_SETTINGS->radio_font, lv_color_hex(ORANGE_DARK), lv_color_hex(ORANGE));

        //Make the checkbox checked according the saved param
        gui.page.settings.active_index = gui.page.settings.settingsParams.tempUnit;
        lv_obj_add_state(lv_obj_get_child(gui.page.settings.tempUnitContainer, gui.page.settings.settingsParams.tempUnit), LV_STATE_CHECKED);

        gui.page.settings.tempUnitLabel = lv_label_create(gui.page.settings.tempUnitContainer);
        lv_label_set_text(gui.page.settings.tempUnitLabel, tempUnit_text);
        lv_obj_set_style_text_font(gui.page.settings.tempUnitLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.tempUnitLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);
}


static void initSettings_switches(lv_obj_t *parent)
{
  gui.page.settings.waterInletContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.waterInletContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_WATER_INLET);
  lv_obj_set_size(gui.page.settings.waterInletContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.waterInletContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.waterInletContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.waterInletLabel = lv_label_create(gui.page.settings.waterInletContainer);
        lv_label_set_text(gui.page.settings.waterInletLabel, waterInlet_text);
        lv_obj_set_style_text_font(gui.page.settings.waterInletLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.waterInletLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        createQuestionMark(gui.page.settings.waterInletContainer, gui.page.settings.waterInletLabel, event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.waterInletSwitch = lv_switch_create(gui.page.settings.waterInletContainer);
        lv_obj_set_size(gui.page.settings.waterInletSwitch, UI_SETTINGS->toggle_switch_w, UI_SETTINGS->toggle_switch_h);
        lv_obj_add_event_cb(gui.page.settings.waterInletSwitch, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.waterInletSwitch);
        lv_obj_align(gui.page.settings.waterInletSwitch, LV_ALIGN_RIGHT_MID, UI_SETTINGS->switch_x, UI_SETTINGS->switch_y);
        lv_obj_set_style_bg_color(gui.page.settings.waterInletSwitch,  lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.waterInletSwitch,  lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.waterInletSwitch,  lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_state(gui.page.settings.waterInletSwitch, gui.page.settings.settingsParams.waterInlet);


  gui.page.settings.tempTuningContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.tempTuningContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_TEMP_TUNING);  /* 2. Tune */
  lv_obj_set_size(gui.page.settings.tempTuningContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.tempTuningContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.tempTuningContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.tempSensorTuningLabel = lv_label_create(gui.page.settings.tempTuningContainer);
        lv_label_set_text(gui.page.settings.tempSensorTuningLabel, tempSensorTuning_text);
        lv_obj_set_style_text_font(gui.page.settings.tempSensorTuningLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.tempSensorTuningLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        createQuestionMark(gui.page.settings.tempTuningContainer,gui.page.settings.tempSensorTuningLabel,event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.tempSensorTuneButton = lv_button_create(gui.page.settings.tempTuningContainer);
        lv_obj_set_size(gui.page.settings.tempSensorTuneButton, BUTTON_TUNE_WIDTH, BUTTON_TUNE_HEIGHT);
        lv_obj_align(gui.page.settings.tempSensorTuneButton, LV_ALIGN_RIGHT_MID, UI_SETTINGS->tune_button_x, UI_SETTINGS->tune_button_y);
        lv_obj_add_event_cb(gui.page.settings.tempSensorTuneButton, event_settings_handler, LV_EVENT_CLICKED, gui.page.settings.tempSensorTuneButton);
        lv_obj_add_event_cb(gui.page.settings.tempSensorTuneButton, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.tempSensorTuneButton);
        lv_obj_add_event_cb(gui.page.settings.tempSensorTuneButton, event_settings_handler, LV_EVENT_SHORT_CLICKED, gui.page.settings.tempSensorTuneButton);
        lv_obj_add_event_cb(gui.page.settings.tempSensorTuneButton, event_settings_handler, LV_EVENT_LONG_PRESSED, gui.page.settings.tempSensorTuneButton);
        lv_obj_add_event_cb(gui.page.settings.tempSensorTuneButton, event_settings_handler, LV_EVENT_RELEASED, gui.page.settings.tempSensorTuneButton);
        lv_obj_set_style_bg_color(gui.page.settings.tempSensorTuneButton, lv_color_hex(ORANGE), LV_PART_MAIN);

        gui.page.settings.tempSensorTuneButtonLabel = lv_label_create(gui.page.settings.tempSensorTuneButton);
        lv_label_set_text(gui.page.settings.tempSensorTuneButtonLabel, tuneButton_text);
        lv_obj_set_style_text_font(gui.page.settings.tempSensorTuneButtonLabel, UI_SETTINGS->button_font, 0);
        lv_obj_align(gui.page.settings.tempSensorTuneButtonLabel, LV_ALIGN_CENTER, 0, 0);


  gui.page.settings.persistentAlarmContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.persistentAlarmContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_PERSISTENT_ALARM);
  lv_obj_set_size(gui.page.settings.persistentAlarmContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.persistentAlarmContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.persistentAlarmContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.persistentAlarmLabel = lv_label_create(gui.page.settings.persistentAlarmContainer);
        lv_label_set_text(gui.page.settings.persistentAlarmLabel, persistentAlarm_text);
        lv_obj_set_style_text_font(gui.page.settings.persistentAlarmLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.persistentAlarmLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        createQuestionMark(gui.page.settings.persistentAlarmContainer,gui.page.settings.persistentAlarmLabel,event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.persistentAlarmSwitch = lv_switch_create(gui.page.settings.persistentAlarmContainer);
        lv_obj_set_size(gui.page.settings.persistentAlarmSwitch, UI_SETTINGS->toggle_switch_w, UI_SETTINGS->toggle_switch_h);
        lv_obj_add_event_cb(gui.page.settings.persistentAlarmSwitch, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.persistentAlarmSwitch);
        lv_obj_align(gui.page.settings.persistentAlarmSwitch, LV_ALIGN_RIGHT_MID, UI_SETTINGS->switch_x, UI_SETTINGS->switch_y);
        lv_obj_set_style_bg_color(gui.page.settings.persistentAlarmSwitch,  lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.persistentAlarmSwitch,  lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.persistentAlarmSwitch,  lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_state(gui.page.settings.persistentAlarmSwitch, gui.page.settings.settingsParams.isPersistentAlarm);


  gui.page.settings.autostartContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.autostartContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_AUTOSTART);  /* 3. Autostart */
  lv_obj_set_size(gui.page.settings.autostartContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.autostartContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.autostartContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.autostartLabel = lv_label_create(gui.page.settings.autostartContainer);
        lv_label_set_text(gui.page.settings.autostartLabel, autostart_text);
        lv_obj_set_style_text_font(gui.page.settings.autostartLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.autostartLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        createQuestionMark(gui.page.settings.autostartContainer,gui.page.settings.autostartLabel,event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.autostartSwitch = lv_switch_create(gui.page.settings.autostartContainer);
        lv_obj_set_size(gui.page.settings.autostartSwitch, UI_SETTINGS->toggle_switch_w, UI_SETTINGS->toggle_switch_h);
        lv_obj_add_event_cb(gui.page.settings.autostartSwitch, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.autostartSwitch);
        lv_obj_align(gui.page.settings.autostartSwitch, LV_ALIGN_RIGHT_MID, UI_SETTINGS->switch_x, UI_SETTINGS->switch_y);
        lv_obj_set_style_bg_color(gui.page.settings.autostartSwitch,  lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.autostartSwitch,  lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.autostartSwitch,  lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_state(gui.page.settings.autostartSwitch, gui.page.settings.settingsParams.isProcessAutostart);
}


/* Dedicated handler for the Pump/Motor speed TUNE buttons — opens the speed popup. */
static void event_speedTuneButton(lv_event_t *e)
{
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    if (btn == gui.page.settings.pumpSpeedTuneButton)
        speedPopupCreate(true, gui.page.settings.settingsParams.pumpSpeed);
    else if (btn == gui.page.settings.motorSpeedTuneButton)
        speedPopupCreate(false, gui.page.settings.settingsParams.filmRotationSpeedSetpoint);
    else if (btn == gui.page.settings.volumeTuneButton)
        volumePopupCreate(gui.page.settings.settingsParams.volume);
}

static void initSettings_sliders(lv_obj_t *parent)
{
  gui.page.settings.filmRotationSpeedContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.filmRotationSpeedContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_FILM_ROT_SPEED);
  lv_obj_set_size(gui.page.settings.filmRotationSpeedContainer, UI_SETTINGS->row_w, SETTINGS_H_SLIDER);
  lv_obj_remove_flag(gui.page.settings.filmRotationSpeedContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.filmRotationSpeedContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.filmRotationSpeedLabel = lv_label_create(gui.page.settings.filmRotationSpeedContainer);
        lv_label_set_text(gui.page.settings.filmRotationSpeedLabel, rotationSpeed_text);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationSpeedLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.filmRotationSpeedLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);

        createQuestionMark(gui.page.settings.filmRotationSpeedContainer,gui.page.settings.filmRotationSpeedLabel,event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.motorSpeedTuneButton = lv_button_create(gui.page.settings.filmRotationSpeedContainer);
        lv_obj_set_size(gui.page.settings.motorSpeedTuneButton, BUTTON_TUNE_WIDTH, BUTTON_TUNE_HEIGHT);
        lv_obj_align(gui.page.settings.motorSpeedTuneButton, LV_ALIGN_RIGHT_MID, UI_SETTINGS->tune_button_x, UI_SETTINGS->tune_button_y);
        lv_obj_set_style_bg_color(gui.page.settings.motorSpeedTuneButton, lv_color_hex(ORANGE), LV_PART_MAIN);
        lv_obj_add_event_cb(gui.page.settings.motorSpeedTuneButton, event_speedTuneButton, LV_EVENT_CLICKED, NULL);

        gui.page.settings.motorSpeedTuneButtonLabel = lv_label_create(gui.page.settings.motorSpeedTuneButton);
        lv_label_set_text(gui.page.settings.motorSpeedTuneButtonLabel, tuneButton_text);
        lv_obj_set_style_text_font(gui.page.settings.motorSpeedTuneButtonLabel, UI_SETTINGS->button_font, 0);
        lv_obj_align(gui.page.settings.motorSpeedTuneButtonLabel, LV_ALIGN_CENTER, 0, 0);

        gui.page.settings.filmRotationSpeedValueLabel = lv_label_create(gui.page.settings.filmRotationSpeedContainer);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationSpeedValueLabel, UI_SETTINGS->value_font, 0);
        lv_label_set_text_fmt(gui.page.settings.filmRotationSpeedValueLabel, "%d%%", gui.page.settings.settingsParams.filmRotationSpeedSetpoint);
        lv_obj_align_to(gui.page.settings.filmRotationSpeedValueLabel, gui.page.settings.motorSpeedTuneButton, LV_ALIGN_OUT_LEFT_MID, -12, 0);



  gui.page.settings.filmRotationInverseIntervalContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.filmRotationInverseIntervalContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_FILM_ROT_INTERVAL);
  lv_obj_set_size(gui.page.settings.filmRotationInverseIntervalContainer, UI_SETTINGS->row_w, SETTINGS_H_SLIDER);
  lv_obj_remove_flag(gui.page.settings.filmRotationInverseIntervalContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.filmRotationInverseIntervalContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.filmRotationInverseIntervalLabel = lv_label_create(gui.page.settings.filmRotationInverseIntervalContainer);
        lv_label_set_text(gui.page.settings.filmRotationInverseIntervalLabel, rotationInversionInterval_text);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationInverseIntervalLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.filmRotationInverseIntervalLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);

        createQuestionMark(gui.page.settings.filmRotationInverseIntervalContainer,gui.page.settings.filmRotationInverseIntervalLabel,event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.filmRotationInversionIntervalSlider = lv_slider_create(gui.page.settings.filmRotationInverseIntervalContainer);
        lv_obj_set_width(gui.page.settings.filmRotationInversionIntervalSlider, UI_SETTINGS->slider_w);
        lv_obj_align(gui.page.settings.filmRotationInversionIntervalSlider, LV_ALIGN_TOP_LEFT, UI_SETTINGS->slider_x_offset, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.filmRotationInversionIntervalSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.filmRotationInversionIntervalSlider,lv_color_hex(ORANGE) , LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.filmRotationInversionIntervalSlider,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.filmRotationInversionIntervalSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_range(gui.page.settings.filmRotationInversionIntervalSlider, 10, 60);
        lv_slider_set_value(gui.page.settings.filmRotationInversionIntervalSlider, gui.page.settings.settingsParams.rotationIntervalSetpoint, LV_ANIM_OFF);

        gui.page.settings.filmRotationInverseIntervalValueLabel = lv_label_create(gui.page.settings.filmRotationInverseIntervalContainer);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationInverseIntervalValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.filmRotationInverseIntervalValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);
        lv_obj_add_event_cb(gui.page.settings.filmRotationInversionIntervalSlider, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.filmRotationInverseIntervalValueLabel);
        lv_obj_add_event_cb(gui.page.settings.filmRotationInversionIntervalSlider, event_settings_handler, LV_EVENT_RELEASED, gui.page.settings.filmRotationInverseIntervalValueLabel);
        lv_label_set_text_fmt(gui.page.settings.filmRotationInverseIntervalValueLabel, "%"PRIu8"sec", gui.page.settings.settingsParams.rotationIntervalSetpoint);



  gui.page.settings.randomContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.randomContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_FILM_RANDOM);
  lv_obj_set_size(gui.page.settings.randomContainer, UI_SETTINGS->row_w, SETTINGS_H_SLIDER);
  lv_obj_remove_flag(gui.page.settings.randomContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.randomContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.filmRotationRandomLabel = lv_label_create(gui.page.settings.randomContainer);
        lv_label_set_text(gui.page.settings.filmRotationRandomLabel, rotationRandom_text);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationRandomLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.filmRotationRandomLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);

        createQuestionMark(gui.page.settings.randomContainer,gui.page.settings.filmRotationRandomLabel,event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.filmRandomSlider = lv_slider_create(gui.page.settings.randomContainer);
        lv_obj_set_width(gui.page.settings.filmRandomSlider, UI_SETTINGS->slider_w);
        lv_obj_align(gui.page.settings.filmRandomSlider, LV_ALIGN_TOP_LEFT, UI_SETTINGS->slider_x_offset, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.filmRandomSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.filmRandomSlider,lv_color_hex(ORANGE) , LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.filmRandomSlider,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.filmRandomSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_value(gui.page.settings.filmRandomSlider, gui.page.settings.settingsParams.randomSetpoint, LV_ANIM_OFF);

        gui.page.settings.filmRotationRandomValueLabel = lv_label_create(gui.page.settings.randomContainer);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationRandomValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.filmRotationRandomValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);
        lv_obj_add_event_cb(gui.page.settings.filmRandomSlider, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.filmRotationRandomValueLabel);
        lv_obj_add_event_cb(gui.page.settings.filmRandomSlider, event_settings_handler, LV_EVENT_RELEASED, gui.page.settings.filmRotationRandomValueLabel);
        lv_label_set_text_fmt(gui.page.settings.filmRotationRandomValueLabel, "~%"PRIu8"%%", gui.page.settings.settingsParams.randomSetpoint);


  gui.page.settings.drainFillTimeContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.drainFillTimeContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_DRAIN_FILL);
  lv_obj_set_size(gui.page.settings.drainFillTimeContainer, UI_SETTINGS->row_w, SETTINGS_H_SLIDER);
  lv_obj_remove_flag(gui.page.settings.drainFillTimeContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.drainFillTimeContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.drainFillTimeLabel = lv_label_create(gui.page.settings.drainFillTimeContainer);
        lv_label_set_text(gui.page.settings.drainFillTimeLabel, drainFillTime_text);
        lv_obj_set_style_text_font(gui.page.settings.drainFillTimeLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.drainFillTimeLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);

        createQuestionMark(gui.page.settings.drainFillTimeContainer,gui.page.settings.drainFillTimeLabel,event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.drainFillTimeSlider = lv_slider_create(gui.page.settings.drainFillTimeContainer);
        lv_obj_set_width(gui.page.settings.drainFillTimeSlider, UI_SETTINGS->slider_w);
        lv_obj_align(gui.page.settings.drainFillTimeSlider, LV_ALIGN_TOP_LEFT, UI_SETTINGS->slider_x_offset, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.drainFillTimeSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.drainFillTimeSlider,lv_color_hex(ORANGE) , LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.drainFillTimeSlider,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.drainFillTimeSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_value(gui.page.settings.drainFillTimeSlider, gui.page.settings.settingsParams.drainFillOverlapSetpoint, LV_ANIM_OFF);


        gui.page.settings.drainFillTimeValueLabel = lv_label_create(gui.page.settings.drainFillTimeContainer);
        lv_obj_set_style_text_font(gui.page.settings.drainFillTimeValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.drainFillTimeValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);
        lv_obj_add_event_cb(gui.page.settings.drainFillTimeSlider, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.drainFillTimeValueLabel);
        lv_obj_add_event_cb(gui.page.settings.drainFillTimeSlider, event_settings_handler, LV_EVENT_RELEASED, gui.page.settings.drainFillTimeValueLabel);
        lv_label_set_text_fmt(gui.page.settings.drainFillTimeValueLabel, "%d%%", gui.page.settings.settingsParams.drainFillOverlapSetpoint);


gui.page.settings.multiRinseTimeContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.multiRinseTimeContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_MULTI_RINSE);
  lv_obj_set_size(gui.page.settings.multiRinseTimeContainer, UI_SETTINGS->row_w, SETTINGS_H_SLIDER);
  lv_obj_remove_flag(gui.page.settings.multiRinseTimeContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.multiRinseTimeContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.multiRinseTimeLabel = lv_label_create(gui.page.settings.multiRinseTimeContainer);
        lv_label_set_text(gui.page.settings.multiRinseTimeLabel, multiRinseTime_text);
        lv_obj_set_style_text_font(gui.page.settings.multiRinseTimeLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.multiRinseTimeLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);

        createQuestionMark(gui.page.settings.multiRinseTimeContainer, gui.page.settings.multiRinseTimeLabel, event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.multiRinseTimeSlider = lv_slider_create(gui.page.settings.multiRinseTimeContainer);
        lv_obj_set_width(gui.page.settings.multiRinseTimeSlider, UI_SETTINGS->slider_w);
        lv_obj_align(gui.page.settings.multiRinseTimeSlider, LV_ALIGN_TOP_LEFT, UI_SETTINGS->slider_x_offset, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.multiRinseTimeSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.multiRinseTimeSlider,lv_color_hex(ORANGE) , LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.multiRinseTimeSlider,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.multiRinseTimeSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_range(gui.page.settings.multiRinseTimeSlider, 60, 180);
        lv_slider_set_value(gui.page.settings.multiRinseTimeSlider, gui.page.settings.settingsParams.multiRinseTime, LV_ANIM_OFF);

        gui.page.settings.multiRinseTimeValueLabel = lv_label_create(gui.page.settings.multiRinseTimeContainer);
        lv_obj_set_style_text_font(gui.page.settings.multiRinseTimeValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.multiRinseTimeValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);
        lv_obj_add_event_cb(gui.page.settings.multiRinseTimeSlider, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.multiRinseTimeValueLabel);
        lv_obj_add_event_cb(gui.page.settings.multiRinseTimeSlider, event_settings_handler, LV_EVENT_RELEASED, gui.page.settings.multiRinseTimeValueLabel);
        lv_label_set_text_fmt(gui.page.settings.multiRinseTimeValueLabel, "%ds", gui.page.settings.settingsParams.multiRinseTime);

gui.page.settings.tankSizeContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.tankSizeContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_TANK_SIZE);
  lv_obj_set_size(gui.page.settings.tankSizeContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.tankSizeContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.tankSizeContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.tankSizeLabel = lv_label_create(gui.page.settings.tankSizeContainer);
        lv_label_set_text(gui.page.settings.tankSizeLabel, tankSize_text);
        lv_obj_set_style_text_font(gui.page.settings.tankSizeLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.tankSizeLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        gui.page.settings.tankSizeTextArea = lv_textarea_create(gui.page.settings.tankSizeContainer);
        lv_obj_set_size(gui.page.settings.tankSizeTextArea, UI_SETTINGS->textarea_w, UI_SETTINGS->textarea_h);
        lv_obj_align(gui.page.settings.tankSizeTextArea, LV_ALIGN_RIGHT_MID, UI_SETTINGS->textarea_x, UI_SETTINGS->textarea_y);
        lv_textarea_set_one_line(gui.page.settings.tankSizeTextArea, true);
        lv_obj_set_scrollbar_mode(gui.page.settings.tankSizeTextArea, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_bg_color(gui.page.settings.tankSizeTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_set_style_text_align(gui.page.settings.tankSizeTextArea, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(gui.page.settings.tankSizeTextArea, UI_SETTINGS->label_font, 0);
        lv_obj_set_style_border_color(gui.page.settings.tankSizeTextArea, lv_color_hex(ORANGE), 0);
        lv_obj_add_event_cb(gui.page.settings.tankSizeTextArea, event_settings_handler, LV_EVENT_FOCUSED, &gui.page.settings.tankSize_active_index);

        /* Show saved tank size value */
        {
            const char *sizes[] = tankSizeValues;
            uint8_t tsIdx = gui.page.settings.settingsParams.tankSize;
            if(tsIdx < 1 || tsIdx > 3) tsIdx = 2;
            gui.page.settings.tankSize_active_index = tsIdx - 1;
            lv_textarea_set_text(gui.page.settings.tankSizeTextArea, sizes[tsIdx - 1]);
        }

/* ── Pump speed slider ── */
gui.page.settings.pumpSpeedContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.pumpSpeedContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_PUMP_SPEED);
  lv_obj_set_size(gui.page.settings.pumpSpeedContainer, UI_SETTINGS->row_w, SETTINGS_H_SLIDER);
  lv_obj_remove_flag(gui.page.settings.pumpSpeedContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.pumpSpeedContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.pumpSpeedLabel = lv_label_create(gui.page.settings.pumpSpeedContainer);
        lv_label_set_text(gui.page.settings.pumpSpeedLabel, pumpSpeed_text);
        lv_obj_set_style_text_font(gui.page.settings.pumpSpeedLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.pumpSpeedLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);

        createQuestionMark(gui.page.settings.pumpSpeedContainer, gui.page.settings.pumpSpeedLabel, event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.pumpSpeedTuneButton = lv_button_create(gui.page.settings.pumpSpeedContainer);
        lv_obj_set_size(gui.page.settings.pumpSpeedTuneButton, BUTTON_TUNE_WIDTH, BUTTON_TUNE_HEIGHT);
        lv_obj_align(gui.page.settings.pumpSpeedTuneButton, LV_ALIGN_RIGHT_MID, UI_SETTINGS->tune_button_x, UI_SETTINGS->tune_button_y);
        lv_obj_set_style_bg_color(gui.page.settings.pumpSpeedTuneButton, lv_color_hex(ORANGE), LV_PART_MAIN);
        lv_obj_add_event_cb(gui.page.settings.pumpSpeedTuneButton, event_speedTuneButton, LV_EVENT_CLICKED, NULL);

        gui.page.settings.pumpSpeedTuneButtonLabel = lv_label_create(gui.page.settings.pumpSpeedTuneButton);
        lv_label_set_text(gui.page.settings.pumpSpeedTuneButtonLabel, tuneButton_text);
        lv_obj_set_style_text_font(gui.page.settings.pumpSpeedTuneButtonLabel, UI_SETTINGS->button_font, 0);
        lv_obj_align(gui.page.settings.pumpSpeedTuneButtonLabel, LV_ALIGN_CENTER, 0, 0);

        gui.page.settings.pumpSpeedValueLabel = lv_label_create(gui.page.settings.pumpSpeedContainer);
        lv_obj_set_style_text_font(gui.page.settings.pumpSpeedValueLabel, UI_SETTINGS->value_font, 0);
        lv_label_set_text_fmt(gui.page.settings.pumpSpeedValueLabel, "%d%%", gui.page.settings.settingsParams.pumpSpeed);
        lv_obj_align_to(gui.page.settings.pumpSpeedValueLabel, gui.page.settings.pumpSpeedTuneButton, LV_ALIGN_OUT_LEFT_MID, -12, 0);

/* ── Invert Pump switch ── */
  gui.page.settings.invertPumpContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.invertPumpContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_INVERT_PUMP);
  lv_obj_set_size(gui.page.settings.invertPumpContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.invertPumpContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.invertPumpContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.invertPumpLabel = lv_label_create(gui.page.settings.invertPumpContainer);
        lv_label_set_text(gui.page.settings.invertPumpLabel, invertPump_text);
        lv_obj_set_style_text_font(gui.page.settings.invertPumpLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.invertPumpLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        createQuestionMark(gui.page.settings.invertPumpContainer, gui.page.settings.invertPumpLabel, event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.invertPumpSwitch = lv_switch_create(gui.page.settings.invertPumpContainer);
        lv_obj_set_size(gui.page.settings.invertPumpSwitch, UI_SETTINGS->toggle_switch_w, UI_SETTINGS->toggle_switch_h);
        lv_obj_add_event_cb(gui.page.settings.invertPumpSwitch, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.invertPumpSwitch);
        lv_obj_align(gui.page.settings.invertPumpSwitch, LV_ALIGN_RIGHT_MID, UI_SETTINGS->switch_x, UI_SETTINGS->switch_y);
        lv_obj_set_style_bg_color(gui.page.settings.invertPumpSwitch, lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.invertPumpSwitch, lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.invertPumpSwitch, lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
        if (gui.page.settings.settingsParams.invertPump)
            lv_obj_add_state(gui.page.settings.invertPumpSwitch, LV_STATE_CHECKED);

/* ── Brightness slider ── */
#if defined(DISPLAY_DRIVER_ST7701)
gui.page.settings.brightnessContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.brightnessContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_BRIGHTNESS);
  lv_obj_set_size(gui.page.settings.brightnessContainer, UI_SETTINGS->row_w, SETTINGS_H_SLIDER);
  lv_obj_remove_flag(gui.page.settings.brightnessContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.brightnessContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.brightnessLabel = lv_label_create(gui.page.settings.brightnessContainer);
        lv_label_set_text(gui.page.settings.brightnessLabel, brightness_text);
        lv_obj_set_style_text_font(gui.page.settings.brightnessLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.brightnessLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);

        createQuestionMark(gui.page.settings.brightnessContainer, gui.page.settings.brightnessLabel, event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.brightnessSlider = lv_slider_create(gui.page.settings.brightnessContainer);
        lv_obj_set_width(gui.page.settings.brightnessSlider, UI_SETTINGS->slider_w);
        lv_obj_align(gui.page.settings.brightnessSlider, LV_ALIGN_TOP_LEFT, UI_SETTINGS->slider_x_offset, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.brightnessSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.brightnessSlider, lv_color_hex(ORANGE), LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.brightnessSlider, lv_color_hex(ORANGE_LIGHT), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.brightnessSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_range(gui.page.settings.brightnessSlider, 10, 100);
        lv_slider_set_value(gui.page.settings.brightnessSlider, gui.page.settings.settingsParams.brightness, LV_ANIM_OFF);

        gui.page.settings.brightnessValueLabel = lv_label_create(gui.page.settings.brightnessContainer);
        lv_obj_set_style_text_font(gui.page.settings.brightnessValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.brightnessValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->row_value_x, UI_SETTINGS->row_value_y);
        lv_obj_add_event_cb(gui.page.settings.brightnessSlider, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.brightnessValueLabel);
        lv_obj_add_event_cb(gui.page.settings.brightnessSlider, event_settings_handler, LV_EVENT_RELEASED, gui.page.settings.brightnessValueLabel);
        lv_label_set_text_fmt(gui.page.settings.brightnessValueLabel, "%d%%", gui.page.settings.settingsParams.brightness);
#endif

/* ── Volume (TUNE popup, plays a tone as you scroll) ── */
gui.page.settings.volumeContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.volumeContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_VOLUME);
  lv_obj_set_size(gui.page.settings.volumeContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.volumeContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.volumeContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.volumeLabel = lv_label_create(gui.page.settings.volumeContainer);
        lv_label_set_text(gui.page.settings.volumeLabel, volume_text);
        lv_obj_set_style_text_font(gui.page.settings.volumeLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.volumeLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        createQuestionMark(gui.page.settings.volumeContainer, gui.page.settings.volumeLabel, event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.volumeTuneButton = lv_button_create(gui.page.settings.volumeContainer);
        lv_obj_set_size(gui.page.settings.volumeTuneButton, BUTTON_TUNE_WIDTH, BUTTON_TUNE_HEIGHT);
        lv_obj_align(gui.page.settings.volumeTuneButton, LV_ALIGN_RIGHT_MID, UI_SETTINGS->tune_button_x, UI_SETTINGS->tune_button_y);
        lv_obj_set_style_bg_color(gui.page.settings.volumeTuneButton, lv_color_hex(ORANGE), LV_PART_MAIN);
        lv_obj_add_event_cb(gui.page.settings.volumeTuneButton, event_speedTuneButton, LV_EVENT_CLICKED, NULL);

        gui.page.settings.volumeTuneButtonLabel = lv_label_create(gui.page.settings.volumeTuneButton);
        lv_label_set_text(gui.page.settings.volumeTuneButtonLabel, tuneButton_text);
        lv_obj_set_style_text_font(gui.page.settings.volumeTuneButtonLabel, UI_SETTINGS->button_font, 0);
        lv_obj_align(gui.page.settings.volumeTuneButtonLabel, LV_ALIGN_CENTER, 0, 0);

        gui.page.settings.volumeValueLabel = lv_label_create(gui.page.settings.volumeContainer);
        lv_obj_set_style_text_font(gui.page.settings.volumeValueLabel, UI_SETTINGS->value_font, 0);
        lv_label_set_text_fmt(gui.page.settings.volumeValueLabel, "%d%%", gui.page.settings.settingsParams.volume);
        lv_obj_align_to(gui.page.settings.volumeValueLabel, gui.page.settings.volumeTuneButton, LV_ALIGN_OUT_LEFT_MID, -12, 0);

/* Chem/WB capacity rows removed — fill times are self-calibrated via the
 * Maintenance fills (WB fill bath / chem container fill) using the floats. */

/* ── Chemistry volume ── */
gui.page.settings.chemVolumeContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.chemVolumeContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_CHEM_VOLUME);
  lv_obj_set_size(gui.page.settings.chemVolumeContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.chemVolumeContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.chemVolumeContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.chemVolumeLabel = lv_label_create(gui.page.settings.chemVolumeContainer);
        lv_label_set_text(gui.page.settings.chemVolumeLabel, chemistryVolume_text);
        lv_obj_set_style_text_font(gui.page.settings.chemVolumeLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.chemVolumeLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);
        
        createQuestionMark(gui.page.settings.chemVolumeContainer,gui.page.settings.chemVolumeLabel,event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.chemVolumeTextArea = lv_textarea_create(gui.page.settings.chemVolumeContainer);
        lv_obj_set_size(gui.page.settings.chemVolumeTextArea, UI_SETTINGS->textarea_w, UI_SETTINGS->textarea_h);
        lv_obj_align(gui.page.settings.chemVolumeTextArea, LV_ALIGN_RIGHT_MID, UI_SETTINGS->textarea_x, UI_SETTINGS->textarea_y);
        lv_textarea_set_one_line(gui.page.settings.chemVolumeTextArea, true);
        lv_obj_set_scrollbar_mode(gui.page.settings.chemVolumeTextArea, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_bg_color(gui.page.settings.chemVolumeTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_set_style_text_align(gui.page.settings.chemVolumeTextArea, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(gui.page.settings.chemVolumeTextArea, UI_SETTINGS->label_font, 0);
        lv_obj_set_style_border_color(gui.page.settings.chemVolumeTextArea, lv_color_hex(ORANGE), 0);
        lv_obj_add_event_cb(gui.page.settings.chemVolumeTextArea, event_settings_handler, LV_EVENT_FOCUSED, NULL);
        {
            const char *vols[] = chemVolumeValues;
            uint8_t v = gui.page.settings.settingsParams.chemistryVolume;
            if(v < 1 || v > 2) v = 2;
            lv_textarea_set_text(gui.page.settings.chemVolumeTextArea, vols[v - 1]);
        }

  /* ── Splash Screen ── */
  gui.page.settings.splashContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.splashContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_SPLASH_SCREEN);
  lv_obj_set_size(gui.page.settings.splashContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.splashContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.splashContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.splashLabel = lv_label_create(gui.page.settings.splashContainer);
        lv_label_set_text(gui.page.settings.splashLabel, settingsSplashScreen_text);
        lv_obj_set_style_text_font(gui.page.settings.splashLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.splashLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        createQuestionMark(gui.page.settings.splashContainer, gui.page.settings.splashLabel, event_settingPopupMBox, UI_SETTINGS->help_icon_x, UI_SETTINGS->help_icon_y);

        gui.page.settings.splashButton = lv_button_create(gui.page.settings.splashContainer);
        lv_obj_set_size(gui.page.settings.splashButton, BUTTON_TUNE_WIDTH, BUTTON_TUNE_HEIGHT);
        lv_obj_align(gui.page.settings.splashButton, LV_ALIGN_RIGHT_MID, UI_SETTINGS->tune_button_x, UI_SETTINGS->tune_button_y);
        lv_obj_add_event_cb(gui.page.settings.splashButton, event_settings_handler, LV_EVENT_CLICKED, gui.page.settings.splashButton);
        lv_obj_set_style_bg_color(gui.page.settings.splashButton, lv_color_hex(ORANGE), LV_PART_MAIN);

        gui.page.settings.splashButtonLabel = lv_label_create(gui.page.settings.splashButton);
        lv_label_set_text(gui.page.settings.splashButtonLabel, LV_SYMBOL_SETTINGS);
        lv_obj_set_style_text_font(gui.page.settings.splashButtonLabel, UI_SETTINGS->button_font, 0);
        lv_obj_align(gui.page.settings.splashButtonLabel, LV_ALIGN_CENTER, 0, 0);

  /* ── Wi-Fi ── */
  gui.page.settings.wifiContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.wifiContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_WIFI_ROW);
  lv_obj_set_size(gui.page.settings.wifiContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.wifiContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.wifiContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.wifiLabel = lv_label_create(gui.page.settings.wifiContainer);
        lv_label_set_text(gui.page.settings.wifiLabel, settingsWifi_text);
        lv_obj_set_style_text_font(gui.page.settings.wifiLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.wifiLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        gui.page.settings.wifiButton = lv_button_create(gui.page.settings.wifiContainer);
        lv_obj_set_size(gui.page.settings.wifiButton, BUTTON_TUNE_WIDTH, BUTTON_TUNE_HEIGHT);
        lv_obj_align(gui.page.settings.wifiButton, LV_ALIGN_RIGHT_MID, UI_SETTINGS->tune_button_x, UI_SETTINGS->tune_button_y);
        lv_obj_add_event_cb(gui.page.settings.wifiButton, event_settings_handler, LV_EVENT_CLICKED, gui.page.settings.wifiButton);
        lv_obj_set_style_bg_color(gui.page.settings.wifiButton, lv_color_hex(ORANGE), LV_PART_MAIN);

        gui.page.settings.wifiButtonLabel = lv_label_create(gui.page.settings.wifiButton);
        lv_label_set_text(gui.page.settings.wifiButtonLabel, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_font(gui.page.settings.wifiButtonLabel, UI_SETTINGS->button_font, 0);
        lv_obj_align(gui.page.settings.wifiButtonLabel, LV_ALIGN_CENTER, 0, 0);

  /* ── Reset to Defaults ── */
  gui.page.settings.resetContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.resetContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_RESET_ROW);
  lv_obj_set_size(gui.page.settings.resetContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.resetContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.resetContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.resetLabel = lv_label_create(gui.page.settings.resetContainer);
        lv_label_set_text(gui.page.settings.resetLabel, settingsReset_text);
        lv_obj_set_style_text_font(gui.page.settings.resetLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.resetLabel, LV_ALIGN_LEFT_MID, UI_SETTINGS->row_label_x, UI_SETTINGS->row_label_y);

        gui.page.settings.resetButton = lv_button_create(gui.page.settings.resetContainer);
        lv_obj_set_size(gui.page.settings.resetButton, BUTTON_TUNE_WIDTH, BUTTON_TUNE_HEIGHT);
        lv_obj_align(gui.page.settings.resetButton, LV_ALIGN_RIGHT_MID, UI_SETTINGS->tune_button_x, UI_SETTINGS->tune_button_y);
        lv_obj_add_event_cb(gui.page.settings.resetButton, event_settings_handler, LV_EVENT_CLICKED, gui.page.settings.resetButton);
        lv_obj_set_style_bg_color(gui.page.settings.resetButton, lv_color_hex(ORANGE), LV_PART_MAIN);

        gui.page.settings.resetButtonLabel = lv_label_create(gui.page.settings.resetButton);
        lv_label_set_text(gui.page.settings.resetButtonLabel, LV_SYMBOL_REFRESH);
        lv_obj_set_style_text_font(gui.page.settings.resetButtonLabel, UI_SETTINGS->button_font, 0);
        lv_obj_align(gui.page.settings.resetButtonLabel, LV_ALIGN_CENTER, 0, 0);
}


void initSettings(void){
/*********************
 *    PAGE HEADER
 *********************/
  LV_LOG_USER("Settings Creation");
  gui.page.settings.settingsSection = lv_obj_create(lv_screen_active());
  lv_obj_set_pos(gui.page.settings.settingsSection, ui_get_profile()->common.content_x, ui_get_profile()->common.content_y);
  lv_obj_set_size(gui.page.settings.settingsSection, ui_get_profile()->common.content_w, ui_get_profile()->common.content_h);
  lv_obj_remove_flag(gui.page.settings.settingsSection, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_color(gui.page.settings.settingsSection, lv_color_hex(ORANGE_LIGHT), 0);


    int32_t pad = UI_SETTINGS->section_padding;
    lv_obj_set_style_pad_left(gui.page.settings.settingsSection, pad, LV_PART_INDICATOR);
    lv_obj_set_style_pad_right(gui.page.settings.settingsSection, pad, LV_PART_INDICATOR);
    lv_obj_set_style_pad_top(gui.page.settings.settingsSection, pad, LV_PART_INDICATOR);
    lv_obj_set_style_pad_bottom(gui.page.settings.settingsSection, pad, LV_PART_INDICATOR);



  gui.page.settings.settingsLabel = lv_label_create(gui.page.settings.settingsSection);
  lv_label_set_text(gui.page.settings.settingsLabel, Settings_text);
  lv_obj_set_style_text_font(gui.page.settings.settingsLabel, UI_SETTINGS->section_title_font, 0);
  lv_obj_align(gui.page.settings.settingsLabel, LV_ALIGN_TOP_LEFT, UI_SETTINGS->section_label_x, UI_SETTINGS->section_label_y);

  /*Create style*/
  lv_style_init(&gui.page.settings.style_sectionTitleLine);
  lv_style_set_line_width(&gui.page.settings.style_sectionTitleLine, ui_get_profile()->title_line_width);
  lv_style_set_line_color(&gui.page.settings.style_sectionTitleLine, lv_palette_main(LV_PALETTE_ORANGE));
  lv_style_set_line_rounded(&gui.page.settings.style_sectionTitleLine, true);

  /*Create a line and apply the new style*/
  gui.page.settings.sectionTitleLine = lv_line_create(gui.page.settings.settingsSection);
  gui.page.settings.titleLinePoints[1].x = ui_get_profile()->common.title_line_w;
  lv_line_set_points(gui.page.settings.sectionTitleLine, gui.page.settings.titleLinePoints, 2);
  lv_obj_add_style(gui.page.settings.sectionTitleLine, &gui.page.settings.style_sectionTitleLine, 0);
  lv_obj_align(gui.page.settings.sectionTitleLine, LV_ALIGN_TOP_MID, UI_SETTINGS->section_line_x, UI_SETTINGS->section_line_y);

  lv_obj_update_layout(gui.page.settings.settingsSection);

  /*********************
 *    PAGE ELEMENTS
 *********************/

  gui.page.settings.settingsContainer = lv_obj_create(gui.page.settings.settingsSection);
  lv_obj_set_pos(gui.page.settings.settingsContainer, UI_SETTINGS->scroll_x, UI_SETTINGS->scroll_y);
  lv_obj_set_size(gui.page.settings.settingsContainer, UI_SETTINGS->scroll_w, UI_SETTINGS->scroll_h);
  lv_obj_set_style_border_opa(gui.page.settings.settingsContainer, LV_OPA_TRANSP, 0);
  lv_obj_set_scroll_dir(gui.page.settings.settingsContainer, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(gui.page.settings.settingsContainer, LV_SCROLLBAR_MODE_AUTO);

  /* Initialize UI sub-sections */
  initSettings_tempUnit(gui.page.settings.settingsContainer);
  initSettings_switches(gui.page.settings.settingsContainer);
  initSettings_sliders(gui.page.settings.settingsContainer);

  /* Ensure title label and line stay on top of the scroll container (Z-order) */
  lv_obj_move_foreground(gui.page.settings.settingsLabel);
  lv_obj_move_foreground(gui.page.settings.sectionTitleLine);

}

void settings(void)
{
  if(gui.page.settings.settingsSection == NULL){
    initSettings();
    lv_obj_add_flag(gui.page.settings.settingsSection, LV_OBJ_FLAG_HIDDEN);
  }


  lv_style_set_line_color(&gui.page.settings.style_sectionTitleLine, lv_palette_main(LV_PALETTE_ORANGE));
}


/*──────────────────────────────────────────────────────────────
 * refreshSettingsUI()
 *
 * Synchronise every settings widget with the values currently
 * stored in gui.page.settings.settingsParams.
 * Call this once after readConfigFile() so that the UI reflects
 * the saved configuration.
 *──────────────────────────────────────────────────────────────*/
void refreshSettingsUI(void)
{
    /* Settings page not yet created — nothing to refresh.
     * The widgets will pick up the correct values from settingsParams
     * when initSettings() is eventually called. */
    if (gui.page.settings.settingsSection == NULL)
        return;

    struct machineSettings *p = &gui.page.settings.settingsParams;

    /* ── Temperature unit radio buttons ── */
    /* Uncheck both first, then check the correct one */
    lv_obj_remove_state(gui.page.settings.tempUnitCelsiusRadioButton, LV_STATE_CHECKED);
    lv_obj_remove_state(gui.page.settings.tempUnitFahrenheitRadioButton, LV_STATE_CHECKED);
    gui.page.settings.active_index = p->tempUnit;
    lv_obj_add_state(lv_obj_get_child(gui.page.settings.tempUnitContainer, p->tempUnit),
                     LV_STATE_CHECKED);

    /* ── Switches ── */
    if (p->waterInlet)
        lv_obj_add_state(gui.page.settings.waterInletSwitch, LV_STATE_CHECKED);
    else
        lv_obj_remove_state(gui.page.settings.waterInletSwitch, LV_STATE_CHECKED);

    if (p->isPersistentAlarm)
        lv_obj_add_state(gui.page.settings.persistentAlarmSwitch, LV_STATE_CHECKED);
    else
        lv_obj_remove_state(gui.page.settings.persistentAlarmSwitch, LV_STATE_CHECKED);

    if (p->invertPump)
        lv_obj_add_state(gui.page.settings.invertPumpSwitch, LV_STATE_CHECKED);
    else
        lv_obj_remove_state(gui.page.settings.invertPumpSwitch, LV_STATE_CHECKED);

    if (p->isProcessAutostart)
        lv_obj_add_state(gui.page.settings.autostartSwitch, LV_STATE_CHECKED);
    else
        lv_obj_remove_state(gui.page.settings.autostartSwitch, LV_STATE_CHECKED);

    /* ── Sliders + value labels ── */
    /* Pump/Motor speed use a TUNE popup now — just refresh the value label. */
    lv_label_set_text_fmt(gui.page.settings.filmRotationSpeedValueLabel,
                          "%d%%", p->filmRotationSpeedSetpoint);

    /* Compute analog motor speed from loaded setpoint (slider range 10-100) */
    sys.analogVal_rotationSpeedPercent = mapPercentageToValue(
        p->filmRotationSpeedSetpoint, 10, 100);
    LV_LOG_USER("Motor analog speed synced: %d%% -> %d",
                p->filmRotationSpeedSetpoint, sys.analogVal_rotationSpeedPercent);

    lv_slider_set_value(gui.page.settings.filmRotationInversionIntervalSlider,
                        p->rotationIntervalSetpoint, LV_ANIM_OFF);
    lv_label_set_text_fmt(gui.page.settings.filmRotationInverseIntervalValueLabel,
                          "%dsec", p->rotationIntervalSetpoint);

    lv_slider_set_value(gui.page.settings.filmRandomSlider,
                        p->randomSetpoint, LV_ANIM_OFF);
    lv_label_set_text_fmt(gui.page.settings.filmRotationRandomValueLabel,
                          "~%d%%", p->randomSetpoint);

    lv_slider_set_value(gui.page.settings.drainFillTimeSlider,
                        p->drainFillOverlapSetpoint, LV_ANIM_OFF);
    lv_label_set_text_fmt(gui.page.settings.drainFillTimeValueLabel,
                          "%d%%", p->drainFillOverlapSetpoint);

    lv_slider_set_value(gui.page.settings.multiRinseTimeSlider,
                        p->multiRinseTime, LV_ANIM_OFF);
    lv_label_set_text_fmt(gui.page.settings.multiRinseTimeValueLabel,
                          "%ds", p->multiRinseTime);

    /* ── Volume (TUNE popup) ── */
    lv_label_set_text_fmt(gui.page.settings.volumeValueLabel, "%d%%", p->volume);

    /* ── Tank size textarea ── */
    {
        uint8_t tsIdx = p->tankSize;
        if(tsIdx < 1 || tsIdx > 3) tsIdx = 2;
        gui.page.settings.tankSize_active_index = tsIdx - 1;
        const char *sizes[] = tankSizeValues;
        lv_textarea_set_text(gui.page.settings.tankSizeTextArea, sizes[tsIdx - 1]);
    }

    /* ── Pump speed (TUNE popup now — just refresh the value label) ── */
    lv_label_set_text_fmt(gui.page.settings.pumpSpeedValueLabel,
                          "%d%%", p->pumpSpeed);

    /* ── Brightness slider ── */
#if defined(DISPLAY_DRIVER_ST7701)
    /* Safe defaults for old configs (fields zeroed by memset in readConfigFile) */
    if (p->volume == 0)     p->volume = 60;       /* default audio volume */
    if (p->brightness < 10) p->brightness = 100;  /* default full brightness */
    lv_slider_set_value(gui.page.settings.brightnessSlider,
                        p->brightness, LV_ANIM_OFF);
    lv_label_set_text_fmt(gui.page.settings.brightnessValueLabel,
                          "%d%%", p->brightness);
    st7701_lcd_set_user_brightness(p->brightness);
    st7701_lcd_set_dim_timeout(60, 300, 600);  /* 1min→50%, 5min→20%, 10min→off */
#endif

    /* Chem/WB capacity rows removed (self-calibrated fill times). */

    /* ── Chemistry volume ── */
    {
        const char *vols[] = chemVolumeValues;
        uint8_t v = p->chemistryVolume;
        if(v < 1 || v > 2) v = 2;
        lv_textarea_set_text(gui.page.settings.chemVolumeTextArea, vols[v - 1]);
    }

    LV_LOG_USER("Settings UI refreshed from config");
}

