/**
 * @file page_settings.c
 *
 */


//ESSENTIAL INCLUDE
#include "FilMachine.h"

extern struct gui_components gui;

static uint8_t current_value;
static uint8_t new_value;


uint8_t minVal_rotationSpeedPercent;
uint8_t maxVal_rotationSpeedPercent;
uint8_t analogVal_rotationSpeedPercent;

#define UI_SETTINGS                  (&ui_get_profile()->settings)
#define SETTINGS_LEFT_X              (UI_SETTINGS->left_x)
#define SETTINGS_GAP_Y               (UI_SETTINGS->gap_y)

#define SETTINGS_H_ROW               (UI_SETTINGS->row_h)
#define SETTINGS_H_SLIDER            (UI_SETTINGS->slider_h)

#define Y_TEMP_UNIT                  (0)
#define Y_TEMP_TUNING                (Y_TEMP_UNIT + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_AUTOSTART                  (Y_TEMP_TUNING + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_WATER_INLET                (Y_AUTOSTART + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_FILM_ROT_SPEED             (Y_WATER_INLET + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_FILM_ROT_INTERVAL          (Y_FILM_ROT_SPEED + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_FILM_RANDOM                (Y_FILM_ROT_INTERVAL + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_DRAIN_FILL                 (Y_FILM_RANDOM + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_MULTI_RINSE                (Y_DRAIN_FILL + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_PUMP_SPEED                 (Y_MULTI_RINSE + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_PERSISTENT_ALARM           (Y_PUMP_SPEED + SETTINGS_H_SLIDER + SETTINGS_GAP_Y)
#define Y_TANK_SIZE                  (Y_PERSISTENT_ALARM + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_CHEM_CONTAINER_ML          (Y_TANK_SIZE + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_WB_CONTAINER_ML            (Y_CHEM_CONTAINER_ML + SETTINGS_H_ROW + SETTINGS_GAP_Y)
#define Y_CHEM_VOLUME                (Y_WB_CONTAINER_ML + SETTINGS_H_ROW + SETTINGS_GAP_Y)

//ACCESSORY INCLUDES


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
}


void event_settings_handler(lv_event_t * e)
{
    uint32_t * active_id = (uint32_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * cont = (lv_obj_t *)lv_event_get_current_target(e);
    lv_obj_t * act_cb = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * old_cb = active_id ? (lv_obj_t *)lv_obj_get_child(cont, *active_id) : NULL;

    /*Do nothing if the container was clicked*/

    if(act_cb == cont && cont != gui.page.settings.waterInletSwitch && cont != gui.page.settings.tempSensorTuneButton && cont != gui.page.settings.filmRotationSpeedSlider && cont != gui.page.settings.filmRotationInversionIntervalSlider && cont != gui.page.settings.filmRandomSlider && cont != gui.page.settings.persistentAlarmSwitch && cont != gui.page.settings.autostartSwitch && cont != gui.page.settings.drainFillTimeSlider && cont != gui.page.settings.multiRinseTimeSlider && cont != gui.page.settings.tankSizeTextArea && cont != gui.page.settings.pumpSpeedSlider && cont != gui.page.settings.chemContainerMlTextArea && cont != gui.page.settings.wbContainerMlTextArea && cont != gui.page.settings.chemVolumeTextArea)
      return;

    if(act_cb == gui.page.settings.tempUnitCelsiusRadioButton || act_cb == gui.page.settings.tempUnitFahrenheitRadioButton){
       if(code == LV_EVENT_CLICKED) {
            lv_obj_remove_state(old_cb, LV_STATE_CHECKED);
            lv_obj_add_state(act_cb, LV_STATE_CHECKED);
            *active_id = lv_obj_get_index(act_cb);
            LV_LOG_USER("Selected °C or °F: %d", (int)gui.page.settings.active_index);
            gui.page.settings.settingsParams.tempUnit = (int)gui.page.settings.active_index;
            qSysAction( SAVE_PROCESS_CONFIG );
            uint8_t i = 0;
            for(i = 0; i < lv_obj_get_child_cnt(gui.page.processes.processesListContainer); i++) {
              lv_obj_send_event(lv_obj_get_child(gui.page.processes.processesListContainer, i), LV_EVENT_REFRESH, NULL);
            }
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
          rollerPopupCreate(gui.element.rollerPopup.tempCelsiusOptions,tuneTempPopupTitle_text,gui.page.settings.tempSensorTuneButton, gui.page.settings.settingsParams.calibratedTemp);
        }
      if(code == LV_EVENT_LONG_PRESSED) {
          LV_LOG_USER("TUNE Long click - Resetting calibration");
          gui.page.settings.settingsParams.tempCalibOffset = 0;
          gui.page.settings.settingsParams.calibratedTemp = 20; /* default */
          qSysAction(SAVE_PROCESS_CONFIG);
          messagePopupCreate("Calibration Reset", "Temperature calibration has been reset to default values.", NULL, NULL, NULL);
        }
    }

    if (act_cb == gui.page.settings.filmRotationSpeedSlider) {
        if (code == LV_EVENT_VALUE_CHANGED) {
            current_value = gui.page.settings.settingsParams.filmRotationSpeedSetpoint;
            new_value = lv_slider_get_value(act_cb);

            new_value = roundToStep(new_value, 10);

            minVal_rotationSpeedPercent = lv_slider_get_min_value(gui.page.settings.filmRotationSpeedSlider);
            maxVal_rotationSpeedPercent = lv_slider_get_max_value(gui.page.settings.filmRotationSpeedSlider);
            analogVal_rotationSpeedPercent = mapPercentageToValue(new_value, minVal_rotationSpeedPercent, maxVal_rotationSpeedPercent);

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
            rollerPopupCreate(checkupTankSizesList, checkupTankSize_text, act_cb, gui.page.settings.tankSize_active_index);
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

    if(act_cb == gui.page.settings.chemContainerMlTextArea) {
        if(code == LV_EVENT_FOCUSED) {
            if(gui.element.rollerPopup.mBoxRollerParent != NULL) return;
            LV_LOG_USER("Set Chemistry Container Capacity");
            uint16_t val = gui.page.settings.settingsParams.chemContainerMl;
            uint32_t idx = 0;
            uint16_t vals[] = {250, 500, 750, 1000, 1250, 1500};
            for(int i = 0; i < 6; i++) { if(vals[i] == val) { idx = i; break; } }
            rollerPopupCreate(chemContainerMlList, chemContainerMl_text, act_cb, idx);
        }
    }

    if(act_cb == gui.page.settings.wbContainerMlTextArea) {
        if(code == LV_EVENT_FOCUSED) {
            if(gui.element.rollerPopup.mBoxRollerParent != NULL) return;
            LV_LOG_USER("Set Water Bath Capacity");
            uint16_t val = gui.page.settings.settingsParams.wbContainerMl;
            uint32_t idx = 0;
            uint16_t vals[] = {1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000};
            for(int i = 0; i < 8; i++) { if(vals[i] == val) { idx = i; break; } }
            rollerPopupCreate(wbContainerMlList, wbContainerMl_text, act_cb, idx);
        }
    }

    if(act_cb == gui.page.settings.chemVolumeTextArea) {
        if(code == LV_EVENT_FOCUSED) {
            if(gui.element.rollerPopup.mBoxRollerParent != NULL) return;
            LV_LOG_USER("Set Chemistry Volume");
            uint32_t idx = gui.page.settings.settingsParams.chemistryVolume >= 2 ? 1 : 0;
            rollerPopupCreate(chemistryVolumeList, chemistryVolume_text, act_cb, idx);
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

        gui.page.settings.tempUnitCelsiusRadioButton = create_radiobutton(gui.page.settings.tempUnitContainer, celsius_text, -55, 0, 27, UI_SETTINGS->radio_font, lv_color_hex(ORANGE_DARK), lv_color_hex(ORANGE));
        gui.page.settings.tempUnitFahrenheitRadioButton = create_radiobutton(gui.page.settings.tempUnitContainer, fahrenheit_text, 5, 0, 27, UI_SETTINGS->radio_font, lv_color_hex(ORANGE_DARK), lv_color_hex(ORANGE));

        //Make the checkbox checked according the saved param
        gui.page.settings.active_index = gui.page.settings.settingsParams.tempUnit;
        lv_obj_add_state(lv_obj_get_child(gui.page.settings.tempUnitContainer, gui.page.settings.settingsParams.tempUnit), LV_STATE_CHECKED);

        gui.page.settings.tempUnitLabel = lv_label_create(gui.page.settings.tempUnitContainer);
        lv_label_set_text(gui.page.settings.tempUnitLabel, tempUnit_text);
        lv_obj_set_style_text_font(gui.page.settings.tempUnitLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.tempUnitLabel, LV_ALIGN_LEFT_MID, -5, 0);
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
        lv_obj_align(gui.page.settings.waterInletLabel, LV_ALIGN_LEFT_MID, -5, 0);

        createQuestionMark(gui.page.settings.waterInletContainer, gui.page.settings.waterInletLabel, event_settingPopupMBox, 2, -3);

        gui.page.settings.waterInletSwitch = lv_switch_create(gui.page.settings.waterInletContainer);
        lv_obj_add_event_cb(gui.page.settings.waterInletSwitch, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.waterInletSwitch);
        lv_obj_align(gui.page.settings.waterInletSwitch, LV_ALIGN_RIGHT_MID, 0, 0);
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
        lv_obj_align(gui.page.settings.tempSensorTuningLabel, LV_ALIGN_LEFT_MID, -5, 0);

        createQuestionMark(gui.page.settings.tempTuningContainer,gui.page.settings.tempSensorTuningLabel,event_settingPopupMBox, 2, -3);

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
        lv_obj_align(gui.page.settings.persistentAlarmLabel, LV_ALIGN_LEFT_MID, -5, 0);

        createQuestionMark(gui.page.settings.persistentAlarmContainer,gui.page.settings.persistentAlarmLabel,event_settingPopupMBox, 2, -3);

        gui.page.settings.persistentAlarmSwitch = lv_switch_create(gui.page.settings.persistentAlarmContainer);
        lv_obj_add_event_cb(gui.page.settings.persistentAlarmSwitch, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.persistentAlarmSwitch);
        lv_obj_align(gui.page.settings.persistentAlarmSwitch, LV_ALIGN_RIGHT_MID, 0, 0);
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
        lv_obj_align(gui.page.settings.autostartLabel, LV_ALIGN_LEFT_MID, -5, 0);

        createQuestionMark(gui.page.settings.autostartContainer,gui.page.settings.autostartLabel,event_settingPopupMBox, 2, -3);

        gui.page.settings.autostartSwitch = lv_switch_create(gui.page.settings.autostartContainer);
        lv_obj_add_event_cb(gui.page.settings.autostartSwitch, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.autostartSwitch);
        lv_obj_align(gui.page.settings.autostartSwitch, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_style_bg_color(gui.page.settings.autostartSwitch,  lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.autostartSwitch,  lv_color_hex(ORANGE), LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(gui.page.settings.autostartSwitch,  lv_color_hex(ORANGE_DARK), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_state(gui.page.settings.autostartSwitch, gui.page.settings.settingsParams.isProcessAutostart);
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
        lv_obj_align(gui.page.settings.filmRotationSpeedLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->value_x, UI_SETTINGS->value_y);

        createQuestionMark(gui.page.settings.filmRotationSpeedContainer,gui.page.settings.filmRotationSpeedLabel,event_settingPopupMBox, 2, -3);

        gui.page.settings.filmRotationSpeedSlider = lv_slider_create(gui.page.settings.filmRotationSpeedContainer);
        lv_obj_align(gui.page.settings.filmRotationSpeedSlider, LV_ALIGN_TOP_LEFT, 0, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.filmRotationSpeedSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.filmRotationSpeedSlider,lv_color_hex(ORANGE) , LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.filmRotationSpeedSlider,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.filmRotationSpeedSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_range(gui.page.settings.filmRotationSpeedSlider, 10, 100);
        lv_slider_set_value(gui.page.settings.filmRotationSpeedSlider, gui.page.settings.settingsParams.filmRotationSpeedSetpoint, LV_ANIM_OFF);


        gui.page.settings.filmRotationSpeedValueLabel = lv_label_create(gui.page.settings.filmRotationSpeedContainer);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationSpeedValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.filmRotationSpeedValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->value_x, UI_SETTINGS->value_y);
        lv_obj_add_event_cb(gui.page.settings.filmRotationSpeedSlider, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.filmRotationSpeedValueLabel);
        lv_obj_add_event_cb(gui.page.settings.filmRotationSpeedSlider, event_settings_handler, LV_EVENT_RELEASED, gui.page.settings.filmRotationSpeedValueLabel);
        lv_label_set_text_fmt(gui.page.settings.filmRotationSpeedValueLabel, "%d%%", gui.page.settings.settingsParams.filmRotationSpeedSetpoint);



  gui.page.settings.filmRotationInverseIntervalContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.filmRotationInverseIntervalContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_FILM_ROT_INTERVAL);
  lv_obj_set_size(gui.page.settings.filmRotationInverseIntervalContainer, UI_SETTINGS->row_w, SETTINGS_H_SLIDER);
  lv_obj_remove_flag(gui.page.settings.filmRotationInverseIntervalContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.filmRotationInverseIntervalContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.filmRotationInverseIntervalLabel = lv_label_create(gui.page.settings.filmRotationInverseIntervalContainer);
        lv_label_set_text(gui.page.settings.filmRotationInverseIntervalLabel, rotationInversionInterval_text);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationInverseIntervalLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.filmRotationInverseIntervalLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->value_x, UI_SETTINGS->value_y);

        createQuestionMark(gui.page.settings.filmRotationInverseIntervalContainer,gui.page.settings.filmRotationInverseIntervalLabel,event_settingPopupMBox, 2, -3);

        gui.page.settings.filmRotationInversionIntervalSlider = lv_slider_create(gui.page.settings.filmRotationInverseIntervalContainer);
        lv_obj_align(gui.page.settings.filmRotationInversionIntervalSlider, LV_ALIGN_TOP_LEFT, 0, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.filmRotationInversionIntervalSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.filmRotationInversionIntervalSlider,lv_color_hex(ORANGE) , LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.filmRotationInversionIntervalSlider,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.filmRotationInversionIntervalSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_range(gui.page.settings.filmRotationInversionIntervalSlider, 10, 60);
        lv_slider_set_value(gui.page.settings.filmRotationInversionIntervalSlider, gui.page.settings.settingsParams.rotationIntervalSetpoint, LV_ANIM_OFF);

        gui.page.settings.filmRotationInverseIntervalValueLabel = lv_label_create(gui.page.settings.filmRotationInverseIntervalContainer);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationInverseIntervalValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.filmRotationInverseIntervalValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->value_x, UI_SETTINGS->value_y);
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
        lv_obj_align(gui.page.settings.filmRotationRandomLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->value_x, UI_SETTINGS->value_y);

        createQuestionMark(gui.page.settings.randomContainer,gui.page.settings.filmRotationRandomLabel,event_settingPopupMBox, 2, -3);

        gui.page.settings.filmRandomSlider = lv_slider_create(gui.page.settings.randomContainer);
        lv_obj_align(gui.page.settings.filmRandomSlider, LV_ALIGN_TOP_LEFT, 0, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.filmRandomSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.filmRandomSlider,lv_color_hex(ORANGE) , LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.filmRandomSlider,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.filmRandomSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_value(gui.page.settings.filmRandomSlider, gui.page.settings.settingsParams.randomSetpoint, LV_ANIM_OFF);

        gui.page.settings.filmRotationRandomValueLabel = lv_label_create(gui.page.settings.randomContainer);
        lv_obj_set_style_text_font(gui.page.settings.filmRotationRandomValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.filmRotationRandomValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->value_x, UI_SETTINGS->value_y);
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
        lv_obj_align(gui.page.settings.drainFillTimeLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->value_x, UI_SETTINGS->value_y);

        createQuestionMark(gui.page.settings.drainFillTimeContainer,gui.page.settings.drainFillTimeLabel,event_settingPopupMBox, 2, -3);

        gui.page.settings.drainFillTimeSlider = lv_slider_create(gui.page.settings.drainFillTimeContainer);
        lv_obj_align(gui.page.settings.drainFillTimeSlider, LV_ALIGN_TOP_LEFT, 0, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.drainFillTimeSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.drainFillTimeSlider,lv_color_hex(ORANGE) , LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.drainFillTimeSlider,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.drainFillTimeSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_value(gui.page.settings.drainFillTimeSlider, gui.page.settings.settingsParams.drainFillOverlapSetpoint, LV_ANIM_OFF);


        gui.page.settings.drainFillTimeValueLabel = lv_label_create(gui.page.settings.drainFillTimeContainer);
        lv_obj_set_style_text_font(gui.page.settings.drainFillTimeValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.drainFillTimeValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->value_x, UI_SETTINGS->value_y);
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
        lv_obj_align(gui.page.settings.multiRinseTimeLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->value_x, UI_SETTINGS->value_y);

        createQuestionMark(gui.page.settings.multiRinseTimeContainer, gui.page.settings.multiRinseTimeLabel, event_settingPopupMBox, 2, -3);

        gui.page.settings.multiRinseTimeSlider = lv_slider_create(gui.page.settings.multiRinseTimeContainer);
        lv_obj_align(gui.page.settings.multiRinseTimeSlider, LV_ALIGN_TOP_LEFT, 0, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.multiRinseTimeSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.multiRinseTimeSlider,lv_color_hex(ORANGE) , LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.multiRinseTimeSlider,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.multiRinseTimeSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_range(gui.page.settings.multiRinseTimeSlider, 60, 180);
        lv_slider_set_value(gui.page.settings.multiRinseTimeSlider, gui.page.settings.settingsParams.multiRinseTime, LV_ANIM_OFF);

        gui.page.settings.multiRinseTimeValueLabel = lv_label_create(gui.page.settings.multiRinseTimeContainer);
        lv_obj_set_style_text_font(gui.page.settings.multiRinseTimeValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.multiRinseTimeValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->value_x, UI_SETTINGS->value_y);
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
        lv_obj_align(gui.page.settings.tankSizeLabel, LV_ALIGN_LEFT_MID, -5, 0);

        gui.page.settings.tankSizeTextArea = lv_textarea_create(gui.page.settings.tankSizeContainer);
        lv_obj_set_size(gui.page.settings.tankSizeTextArea, UI_SETTINGS->textarea_w, UI_SETTINGS->textarea_h);
        lv_obj_align(gui.page.settings.tankSizeTextArea, LV_ALIGN_RIGHT_MID, UI_SETTINGS->text_area_x, UI_SETTINGS->text_area_y);
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
        lv_obj_align(gui.page.settings.pumpSpeedLabel, LV_ALIGN_TOP_LEFT, -UI_SETTINGS->value_x, UI_SETTINGS->value_y);

        createQuestionMark(gui.page.settings.pumpSpeedContainer, gui.page.settings.pumpSpeedLabel, event_settingPopupMBox, 2, -3);

        gui.page.settings.pumpSpeedSlider = lv_slider_create(gui.page.settings.pumpSpeedContainer);
        lv_obj_align(gui.page.settings.pumpSpeedSlider, LV_ALIGN_TOP_LEFT, 0, UI_SETTINGS->slider_y);
        lv_obj_set_style_anim_duration(gui.page.settings.pumpSpeedSlider, 2000, 0);
        lv_obj_set_style_bg_color(gui.page.settings.pumpSpeedSlider, lv_color_hex(ORANGE), LV_PART_KNOB);
        lv_obj_set_style_bg_color(gui.page.settings.pumpSpeedSlider, lv_color_hex(ORANGE_LIGHT), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(gui.page.settings.pumpSpeedSlider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
        lv_slider_set_range(gui.page.settings.pumpSpeedSlider, 10, 100);
        lv_slider_set_value(gui.page.settings.pumpSpeedSlider, gui.page.settings.settingsParams.pumpSpeed, LV_ANIM_OFF);

        gui.page.settings.pumpSpeedValueLabel = lv_label_create(gui.page.settings.pumpSpeedContainer);
        lv_obj_set_style_text_font(gui.page.settings.pumpSpeedValueLabel, UI_SETTINGS->value_font, 0);
        lv_obj_align(gui.page.settings.pumpSpeedValueLabel, LV_ALIGN_TOP_RIGHT, UI_SETTINGS->value_x, UI_SETTINGS->value_y);
        lv_obj_add_event_cb(gui.page.settings.pumpSpeedSlider, event_settings_handler, LV_EVENT_VALUE_CHANGED, gui.page.settings.pumpSpeedValueLabel);
        lv_obj_add_event_cb(gui.page.settings.pumpSpeedSlider, event_settings_handler, LV_EVENT_RELEASED, gui.page.settings.pumpSpeedValueLabel);
        lv_label_set_text_fmt(gui.page.settings.pumpSpeedValueLabel, "%d%%", gui.page.settings.settingsParams.pumpSpeed);

/* ── Chemistry container capacity ── */
gui.page.settings.chemContainerMlContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.chemContainerMlContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_CHEM_CONTAINER_ML);
  lv_obj_set_size(gui.page.settings.chemContainerMlContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.chemContainerMlContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.chemContainerMlContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.chemContainerMlLabel = lv_label_create(gui.page.settings.chemContainerMlContainer);
        lv_label_set_text(gui.page.settings.chemContainerMlLabel, chemContainerMl_text);
        lv_obj_set_style_text_font(gui.page.settings.chemContainerMlLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.chemContainerMlLabel, LV_ALIGN_LEFT_MID, -5, 0);

        gui.page.settings.chemContainerMlTextArea = lv_textarea_create(gui.page.settings.chemContainerMlContainer);
        lv_obj_set_size(gui.page.settings.chemContainerMlTextArea, UI_SETTINGS->textarea_w, UI_SETTINGS->textarea_h);
        lv_obj_align(gui.page.settings.chemContainerMlTextArea, LV_ALIGN_RIGHT_MID, UI_SETTINGS->text_area_x, UI_SETTINGS->text_area_y);
        lv_textarea_set_one_line(gui.page.settings.chemContainerMlTextArea, true);
        lv_obj_set_scrollbar_mode(gui.page.settings.chemContainerMlTextArea, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_bg_color(gui.page.settings.chemContainerMlTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_set_style_text_align(gui.page.settings.chemContainerMlTextArea, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(gui.page.settings.chemContainerMlTextArea, UI_SETTINGS->label_font, 0);
        lv_obj_set_style_border_color(gui.page.settings.chemContainerMlTextArea, lv_color_hex(ORANGE), 0);
        lv_obj_add_event_cb(gui.page.settings.chemContainerMlTextArea, event_settings_handler, LV_EVENT_FOCUSED, NULL);
        { char buf[16]; snprintf(buf, sizeof(buf), "%dml", gui.page.settings.settingsParams.chemContainerMl); lv_textarea_set_text(gui.page.settings.chemContainerMlTextArea, buf); }

/* ── Water bath capacity ── */
gui.page.settings.wbContainerMlContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.wbContainerMlContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_WB_CONTAINER_ML);
  lv_obj_set_size(gui.page.settings.wbContainerMlContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.wbContainerMlContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.wbContainerMlContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.wbContainerMlLabel = lv_label_create(gui.page.settings.wbContainerMlContainer);
        lv_label_set_text(gui.page.settings.wbContainerMlLabel, wbContainerMl_text);
        lv_obj_set_style_text_font(gui.page.settings.wbContainerMlLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.wbContainerMlLabel, LV_ALIGN_LEFT_MID, -5, 0);

        gui.page.settings.wbContainerMlTextArea = lv_textarea_create(gui.page.settings.wbContainerMlContainer);
        lv_obj_set_size(gui.page.settings.wbContainerMlTextArea, UI_SETTINGS->textarea_w, UI_SETTINGS->textarea_h);
        lv_obj_align(gui.page.settings.wbContainerMlTextArea, LV_ALIGN_RIGHT_MID, UI_SETTINGS->text_area_x, UI_SETTINGS->text_area_y);
        lv_textarea_set_one_line(gui.page.settings.wbContainerMlTextArea, true);
        lv_obj_set_scrollbar_mode(gui.page.settings.wbContainerMlTextArea, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_bg_color(gui.page.settings.wbContainerMlTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_set_style_text_align(gui.page.settings.wbContainerMlTextArea, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(gui.page.settings.wbContainerMlTextArea, UI_SETTINGS->label_font, 0);
        lv_obj_set_style_border_color(gui.page.settings.wbContainerMlTextArea, lv_color_hex(ORANGE), 0);
        lv_obj_add_event_cb(gui.page.settings.wbContainerMlTextArea, event_settings_handler, LV_EVENT_FOCUSED, NULL);
        { char buf[16]; snprintf(buf, sizeof(buf), "%dml", gui.page.settings.settingsParams.wbContainerMl); lv_textarea_set_text(gui.page.settings.wbContainerMlTextArea, buf); }

/* ── Chemistry volume ── */
gui.page.settings.chemVolumeContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.settings.chemVolumeContainer, LV_ALIGN_TOP_LEFT, SETTINGS_LEFT_X, Y_CHEM_VOLUME);
  lv_obj_set_size(gui.page.settings.chemVolumeContainer, UI_SETTINGS->row_w, SETTINGS_H_ROW);
  lv_obj_remove_flag(gui.page.settings.chemVolumeContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_opa(gui.page.settings.chemVolumeContainer, LV_OPA_TRANSP, 0);

        gui.page.settings.chemVolumeLabel = lv_label_create(gui.page.settings.chemVolumeContainer);
        lv_label_set_text(gui.page.settings.chemVolumeLabel, chemistryVolume_text);
        lv_obj_set_style_text_font(gui.page.settings.chemVolumeLabel, UI_SETTINGS->label_font, 0);
        lv_obj_align(gui.page.settings.chemVolumeLabel, LV_ALIGN_LEFT_MID, -5, 0);
        
        createQuestionMark(gui.page.settings.chemVolumeContainer,gui.page.settings.chemVolumeLabel,event_settingPopupMBox, 2, -3);

        gui.page.settings.chemVolumeTextArea = lv_textarea_create(gui.page.settings.chemVolumeContainer);
        lv_obj_set_size(gui.page.settings.chemVolumeTextArea, UI_SETTINGS->textarea_w, UI_SETTINGS->textarea_h);
        lv_obj_align(gui.page.settings.chemVolumeTextArea, LV_ALIGN_RIGHT_MID, UI_SETTINGS->text_area_x, UI_SETTINGS->text_area_y);
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


    lv_coord_t pad = 2;
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
  lv_style_set_line_width(&gui.page.settings.style_sectionTitleLine, 2);
  lv_style_set_line_color(&gui.page.settings.style_sectionTitleLine, lv_palette_main(LV_PALETTE_ORANGE));
  lv_style_set_line_rounded(&gui.page.settings.style_sectionTitleLine, true);

  /*Create a line and apply the new style*/
  gui.page.settings.sectionTitleLine = lv_line_create(gui.page.settings.settingsSection);
  lv_line_set_points(gui.page.settings.sectionTitleLine, gui.page.settings.titleLinePoints, 2);
  lv_obj_add_style(gui.page.settings.sectionTitleLine, &gui.page.settings.style_sectionTitleLine, 0);
  lv_obj_align(gui.page.settings.sectionTitleLine, LV_ALIGN_TOP_MID, 0, UI_SETTINGS->section_line_y);

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

    if (p->isProcessAutostart)
        lv_obj_add_state(gui.page.settings.autostartSwitch, LV_STATE_CHECKED);
    else
        lv_obj_remove_state(gui.page.settings.autostartSwitch, LV_STATE_CHECKED);

    /* ── Sliders + value labels ── */
    lv_slider_set_value(gui.page.settings.filmRotationSpeedSlider,
                        p->filmRotationSpeedSetpoint, LV_ANIM_OFF);
    lv_label_set_text_fmt(gui.page.settings.filmRotationSpeedValueLabel,
                          "%d%%", p->filmRotationSpeedSetpoint);

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

    /* ── Tank size textarea ── */
    {
        uint8_t tsIdx = p->tankSize;
        if(tsIdx < 1 || tsIdx > 3) tsIdx = 2;
        gui.page.settings.tankSize_active_index = tsIdx - 1;
        const char *sizes[] = tankSizeValues;
        lv_textarea_set_text(gui.page.settings.tankSizeTextArea, sizes[tsIdx - 1]);
    }

    /* ── Pump speed slider ── */
    lv_slider_set_value(gui.page.settings.pumpSpeedSlider,
                        p->pumpSpeed, LV_ANIM_OFF);
    lv_label_set_text_fmt(gui.page.settings.pumpSpeedValueLabel,
                          "%d%%", p->pumpSpeed);

    /* ── Chemistry container capacity ── */
    { char buf[16]; snprintf(buf, sizeof(buf), "%dml", p->chemContainerMl); lv_textarea_set_text(gui.page.settings.chemContainerMlTextArea, buf); }

    /* ── Water bath capacity ── */
    { char buf[16]; snprintf(buf, sizeof(buf), "%dml", p->wbContainerMl); lv_textarea_set_text(gui.page.settings.wbContainerMlTextArea, buf); }

    /* ── Chemistry volume ── */
    {
        const char *vols[] = chemVolumeValues;
        uint8_t v = p->chemistryVolume;
        if(v < 1 || v > 2) v = 2;
        lv_textarea_set_text(gui.page.settings.chemVolumeTextArea, vols[v - 1]);
    }

    LV_LOG_USER("Settings UI refreshed from config");
}

