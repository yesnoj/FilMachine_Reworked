/*
 * FilMachine.h
 *
 *  Created on: May 1, 2025
 *      Author: PeterB
 */

#ifndef MAIN_FILMACHINE_H_
#define MAIN_FILMACHINE_H_

#include "lang.h"

#include "freertos/FreeRTOS.h"
#include "lvgl.h"

/* ═══════════════════════════════════════════════
 * Board-specific hardware definitions
 * All pin assignments, resolution, display/touch driver selection,
 * and sensor availability come from the active board header.
 * Select board at compile time: -DBOARD_JC4880P433 or -DBOARD_SIMULATOR
 * ═══════════════════════════════════════════════ */
#include "board.h"
#include "ui_profile.h"

#ifdef BOARD_SIMULATOR
void sim_ui_debug_tag(lv_obj_t *obj, const char *name);
#else
#define sim_ui_debug_tag(obj, name) ((void)0)
#endif

#define FILM_USE_LOG				1

/* Motor speed limits (independent of board) */
#define MAX_MOTOR_SPD				200
#define MOTOR_MIN_ANALOG_VAL		90      /* Lowered from 150: widens usable slider range. Re-tune if motor stalls at low duty. */
#define MOTOR_MAX_ANALOG_VAL		255
#define MOTOR_KICK_DUTY				170                    /* breakaway kick duty (overcomes stiction on start); ~67%, above the ~145 breakaway */
#define MOTOR_KICK_MS				100                    /* breakaway kick duration in ms */

/* Boot valve self-test — opens then closes each valve in sequence at power-on. */
#define VALVE_SELFTEST_ON_BOOT		1                      /* set to 0 to disable */
#define SENSOR_SELFTEST_ON_BOOT		1                      /* live-log Hall/water-level/flow every 500ms (set 0 to disable) */
#define VALVE_SELFTEST_LAST_PIN		4                      /* valves only (0-4). Pins 5-6 unused, heater moved to Port B — skip 5-7 */
#define VALVE_SELFTEST_ON_MS		500                    /* how long each channel stays on */
#define VALVE_SELFTEST_OFF_MS		250                    /* pause between channels */

/* Pump speed (0-255, 8-bit duty cycle) */
#define PUMP_DEFAULT_SPEED          200     /* Fallback only (e.g. checkup/manual actions) */
#define PUMP_MIN_ANALOG_VAL         150     /* Duty at pumpSpeed=10%. Measured stall/start ~148 on the bench, so 10% now already moves. */
#define PUMP_MAX_ANALOG_VAL         250     /* Duty at pumpSpeed=100% (DBH-12V safe max) */

/* LCD timing (used by parallel-bus boards, ignored by QSPI/simulator) */
#ifndef LCD_PIXEL_CLOCK_HZ
#define LCD_PIXEL_CLOCK_HZ			(40000000)
#endif
#define LCD_BK_LIGHT_ON_LEVEL		1
#define LCD_BK_LIGHT_OFF_LEVEL		!LCD_BK_LIGHT_ON_LEVEL
#define LCD_CMD_BITS				8
#define LCD_PARAM_BITS				8

/* LVGL timing */
#define LVGL_TICK_PERIOD_MS			2

/* I2C instance (shared across touch, MCP23017, PCA9685) */
#define I2C_NUM						0
#if defined(BOARD_JC4880P433)
    #include "driver/i2c_master.h"
    extern i2c_master_bus_handle_t g_i2c_bus_handle;
#endif

/* System defines */
#define FILENAME_SAVE				"/FilMachine.json"
#define FILENAME_BACKUP				"/FilMachine_Backup.json"
#define MAX_STEP_ELEMENTS			30
#define MAX_PROC_ELEMENTS			50
#define CONTAINER_FILLING_TIME		10		//Need to be calibrated
#define WB_FILLING_TIME				30		//Need to be calibrated
#define ENABLE_BOOT_ERRORS			0		//Set to 1 to enable all errors (SD/I2C)
#define INIT_ERROR_SD				1
#define INIT_ERROR_WIRE				2
#define INIT_ERROR_I2C_MCP			3
#define INIT_ERROR_I2C_ADS			4
#define initSDError_text	tr(STR_initSDError_text)
#define initI2CError_text	tr(STR_initI2CError_text)
#define initWIREError_text	tr(STR_initWIREError_text)
/* ACCESSORY STRUCTS */
typedef enum{
    STEP_NODE,
    PROCESS_NODE
} NodeType_t;

typedef enum {
    BLACK_AND_WHITE_FILM,
    COLOR_FILM,
    BOTH_FILM,
    FILM_TYPE_NA
} filmType_t;

typedef enum {
    CELSIUS_TEMP,
    FAHRENHEIT_TEMP,
    TEMP_UNIT_NA
} tempUnit_t;

typedef enum {
    CHEMISTRY,
    RINSE,
    MULTI_RINSE,
    CHEMICAL_TYPE_NA
} chemicalType_t;

/* GUI Related defines */
#define TAB_PROCESSES				3
#define TAB_SETTINGS				4
#define TAB_TOOLS					5

/* UI Layout Constants — resolved from ui_profile at runtime.
   These macros keep backward compatibility with existing code while
   routing through the profile so values adapt to the active board. */
#define STEP_HEIGHT					(ui_get_profile()->step_element.card_h)
#define STEP_Y_START				(ui_get_profile()->step_element.list_start_y)
#define PROCESS_ELEMENT_HEIGHT		(ui_get_profile()->process_element.card_h)
#define PROCESS_Y_START				(ui_get_profile()->process_element.list_start_y)
#define POPUP_WIDTH					(ui_get_profile()->popups.message_w)
#define POPUP_HEIGHT				(ui_get_profile()->popups.message_h)

/* Splash screen strings */
#define splashTitle_text	tr(STR_splashTitle_text)
#define splashSubtitle_text	tr(STR_splashSubtitle_text)
#define splashVersion_text						softwareVersionValue_text

/* Splash popup strings */
#define splashPopupTitle_text	tr(STR_splashPopupTitle_text)
#define splashPopupUseDefault_text	tr(STR_splashPopupUseDefault_text)
#define splashPopupRandom_text	tr(STR_splashPopupRandom_text)
#define splashPopupRandomBtn_text				LV_SYMBOL_SHUFFLE " Random"
#define splashPopupPalette_text	tr(STR_splashPopupPalette_text)
#define splashPopupShapeStyle_text	tr(STR_splashPopupShapeStyle_text)
#define splashPopupComplexity_text	tr(STR_splashPopupComplexity_text)
/* Settings row strings */
#define settingsSplashScreen_text	tr(STR_settingsSplashScreen_text)
/* Wi-Fi / Remote control section strings */
#define settingsWifi_text	tr(STR_settingsWifi_text)
#define settingsReset_text	tr(STR_settingsReset_text)
#define settingsResetPopupTitle_text	tr(STR_settingsResetPopupTitle_text)
#define settingsResetPopupBody_text	tr(STR_settingsResetPopupBody_text)
#define settingsResetConfirmTitle_text	tr(STR_settingsResetConfirmTitle_text)
#define settingsResetConfirmBody_text	tr(STR_settingsResetConfirmBody_text)
#define buttonOk_text	tr(STR_buttonOk_text)
#define wifiPopupTitle_text	tr(STR_wifiPopupTitle_text)
#define wifiScan_text	tr(STR_wifiScan_text)
#define wifiConnect_text	tr(STR_wifiConnect_text)
#define wifiDisconnect_text	tr(STR_wifiDisconnect_text)
#define wifiConnected_text	tr(STR_wifiConnected_text)
#define wifiDisconnected_text	tr(STR_wifiDisconnected_text)
#define wifiConnecting_text	tr(STR_wifiConnecting_text)
#define wifiScanning_text	tr(STR_wifiScanning_text)
#define wifiAutoConnect_text	tr(STR_wifiAutoConnect_text)
#define wifiEnterPassword_text	tr(STR_wifiEnterPassword_text)
#define wifiNoNetworks_text	tr(STR_wifiNoNetworks_text)
#define wifiErrorTitle_text	tr(STR_wifiErrorTitle_text)
#define wifiErrAuthFailed_text	tr(STR_wifiErrAuthFailed_text)
#define wifiErrHandshakeTimeout_text	tr(STR_wifiErrHandshakeTimeout_text)
#define wifiErrMicFailure_text	tr(STR_wifiErrMicFailure_text)
#define wifiErrGroupKeyTimeout_text	tr(STR_wifiErrGroupKeyTimeout_text)
#define wifiErrApNotFound_text	tr(STR_wifiErrApNotFound_text)
#define wifiErrApNotFoundGeneric_text	tr(STR_wifiErrApNotFoundGeneric_text)
#define wifiErrAuthExpired_text	tr(STR_wifiErrAuthExpired_text)
#define wifiErrClass2Frame_text	tr(STR_wifiErrClass2Frame_text)
#define wifiErrClass3Frame_text	tr(STR_wifiErrClass3Frame_text)
#define wifiErrConnectionFail_text	tr(STR_wifiErrConnectionFail_text)
#define wifiErrBeaconTimeout_text	tr(STR_wifiErrBeaconTimeout_text)
#define wifiErrUnknownFmt_text	tr(STR_wifiErrUnknownFmt_text)
#define wifiForgetTitle_text	tr(STR_wifiForgetTitle_text)
#define wifiForgetBody_text	tr(STR_wifiForgetBody_text)
#define wifiForgetYes_text	tr(STR_wifiForgetYes_text)
#define wifiForgetNo_text	tr(STR_wifiForgetNo_text)
/* Checkup placeholder */
#define checkupEllipsis_text	tr(STR_checkupEllipsis_text)
/* Temperature roller range */
#define TEMP_ROLLER_MIN  15
#define TEMP_ROLLER_MAX  40

/* Discard unsaved changes popup */
#define discardChangesTitle_text	tr(STR_discardChangesTitle_text)
#define discardChangesBody_text	tr(STR_discardChangesBody_text)
#define discardChangesNo_text	tr(STR_discardChangesNo_text)
#define discardChangesYes_text	tr(STR_discardChangesYes_text)
/* Step source format */
#define stepSourceFmt_text	tr(STR_stepSourceFmt_text)
/* Icon Characters */
#define plusplus_icon2				"\xC2\xB1"
#define tabProcess_label			"Process list"
#define tabProcess_icon				"\xEF\x85\xA0"
#define tabSetting_label			"Settings"
#define tabSetting_icon				"\xEF\x93\xBE"
#define tabTools_label				"Tools"
#define tabTools_icon				"\xEF\x9F\x99"
#define funnel_icon					"\xEF\x82\xB0"
#define newProcess_icon				"\xEF\x83\xBE"
#define preferred_icon				"\xEF\x80\x84"
#define colorpalette_icon			"\xEF\x94\xBF"
#define temp_icon					"\xEF\x8B\x89"
#define blackwhite_icon				"\xEF\x81\x82"
#define questionMark_icon			"\xEF\x81\x99"
#define closePopup_icon				"\xEF\x80\x8D"
#define play_icon					"\xEF\x81\x8B"
#define save_icon					"\xEF\x83\x87"
#define processModify_text	tr(STR_processModify_text)
#define trash_icon					"\xEF\x8B\xAD"
#define chemical_icon				"\xEF\x83\x83"
#define rinse_icon                  "\xEF\x8B\x8C"
#define multiRinse_icon				"\xEF\x86\xB8"
#define edit_icon					"\xEF\x81\x84"
#define checkStep_icon				"\xEF\x80\x8C"
#define arrowStep_icon				"\xEF\x81\xA1"
#define dotStep_icon				"\xEF\x86\x92"
#define clock_icon					"\xEF\x80\x97"
#define chip_icon					"\xEF\x8B\x9B"
#define alert_icon					"\xEF\x81\xAA"
#define sdCard_icon					"\xEF\x9F\x82"

/* Reserved Icons — not loaded in font files yet, uncomment and add to font.c when needed
#define oldCamera_icon				"\xEF\x82\x83"
#define plusplus_icon				"\xEF\x84\x81"
#define film_icon					"\xEF\x80\x88"
#define image_icon					"\xEF\x80\xBE"
#define filter_icon					"\xEF\x87\x9E"
#define moveUp_icon					"\xEF\x81\xB7"
#define moveDown_icon				"\xEF\x81\xB8"
#define copy_icon					"\xEF\x83\x85"
*/
#define discardAfter_icon			"\xEF\x8B\xB5"

/* Some colours */
#define RED							0xff2600
#define RED_DARK					0x91060a
#define BLUE						0x1d53ab
#define BLUE_DARK					0x123773 
#define GREEN_LIGHT					0x24d64e    
#define GREEN						0x77bb41
#define GREEN_DARK					0x0e6b2c
#define CYAN						0x00fdff
#define ORANGE						0xcc871b
#define ORANGE_DARK					0x6b470e
#define ORANGE_LIGHT				0xfcba03
#define YELLOW						0xf5ec00
#define GREY						0xd6d6d6
#define WHITE						0xffffff
#define BLACK						0x000000
#define LIGHT_BLUE					0x1fd3e0
#define LIGHT_BLUE_DARK				0x16939c

/*********************
* Process tab strings
*********************/
#define buttonFilter_text	tr(STR_buttonFilter_text)
#define Processes_text	tr(STR_Processes_text)
#define keyboard_placeholder_text	tr(STR_keyboard_placeholder_text)
#define filterPopupTitle_text	tr(STR_filterPopupTitle_text)
#define filterPopupNamePlaceHolder_text	tr(STR_filterPopupNamePlaceHolder_text)
#define filterPopupName_text	tr(STR_filterPopupName_text)
#define filterPopupColor_text	tr(STR_filterPopupColor_text)
#define filterPopupBnW_text	tr(STR_filterPopupBnW_text)
#define filterPopupBoth_text	tr(STR_filterPopupBoth_text)
#define filterPopupPreferred_text	tr(STR_filterPopupPreferred_text)
#define filterPopupApplyButton_text	tr(STR_filterPopupApplyButton_text)
#define filterPopupResetButton_text	tr(STR_filterPopupResetButton_text)
/*********************
* Config tab strings
*********************/
#define Settings_text	tr(STR_Settings_text)
#define tempUnit_text	tr(STR_tempUnit_text)
#define tempSensorTuning_text	tr(STR_tempSensorTuning_text)
#define tuneButton_text	tr(STR_tuneButton_text)
#define tempAlertMBox_text	tr(STR_tempAlertMBox_text)
#define soundAlertMBox_text	tr(STR_soundAlertMBox_text)
#define autostartAlertMBox_text	tr(STR_autostartAlertMBox_text)
#define filmRotationSpeedAlertMBox_text	tr(STR_filmRotationSpeedAlertMBox_text)
#define rotationInverseIntervalAlertMBox_text	tr(STR_rotationInverseIntervalAlertMBox_text)
#define filmRotationRandomAlertMBox_text	tr(STR_filmRotationRandomAlertMBox_text)
#define drainFillTimeAlertMBox_text	tr(STR_drainFillTimeAlertMBox_text)
#define multiRinseTimeAlertMBox_text	tr(STR_multiRinseTimeAlertMBox_text)
#define waterInletAlertMBox_text	tr(STR_waterInletAlertMBox_text)
#define rotationSpeed_text	tr(STR_rotationSpeed_text)
#define rotationInversionInterval_text	tr(STR_rotationInversionInterval_text)
#define rotationRandom_text	tr(STR_rotationRandom_text)
#define celsius_text	tr(STR_celsius_text)
#define fahrenheit_text	tr(STR_fahrenheit_text)
#define waterInlet_text	tr(STR_waterInlet_text)
#define persistentAlarm_text	tr(STR_persistentAlarm_text)
#define autostart_text	tr(STR_autostart_text)
#define drainFillTime_text	tr(STR_drainFillTime_text)
#define multiRinseTime_text	tr(STR_multiRinseTime_text)
#define lineRinse_text	tr(STR_lineRinse_text)
#define lineRinseTime_text	tr(STR_lineRinseTime_text)
#define lineRinseAlertMBox_text	tr(STR_lineRinseAlertMBox_text)
#define tankSize_text	tr(STR_tankSize_text)
#define tankSizeAlertMBox_text	tr(STR_tankSizeAlertMBox_text)
#define tankSizeSmall_text	tr(STR_tankSizeSmall_text)
#define tankSizeMedium_text	tr(STR_tankSizeMedium_text)
#define tankSizeLarge_text	tr(STR_tankSizeLarge_text)
#define pumpSpeed_text	tr(STR_pumpSpeed_text)
#define pumpSpeedAlertMBox_text	tr(STR_pumpSpeedAlertMBox_text)
#define speedTestSwitch_text	tr(STR_speedTestSwitch_text)
#define speedSetPopupTitle_text	tr(STR_speedSetPopupTitle_text)
#define volume_text	tr(STR_volume_text)
#define volumeAlertMBox_text	tr(STR_volumeAlertMBox_text)
#define volumeSetPopupTitle_text	tr(STR_volumeSetPopupTitle_text)
#define invertPump_text	tr(STR_invertPump_text)
#define invertPumpAlertMBox_text	tr(STR_invertPumpAlertMBox_text)
#define brightness_text	tr(STR_brightness_text)
#define brightnessAlertMBox_text	tr(STR_brightnessAlertMBox_text)
#define chemistryVolume_text	tr(STR_chemistryVolume_text)
#define splashScreenAlertMBox_text	tr(STR_splashScreenAlertMBox_text)
#define chemistryVolumeAlertMBox_text	tr(STR_chemistryVolumeAlertMBox_text)
#define chemistryVolumeList	tr(STR_chemistryVolumeList)

/* Language setting (EN/IT) — added by gen_lang.py */
#define language_text	tr(STR_language_text)
#define languageAlertMBox_text	tr(STR_languageAlertMBox_text)
#define languageSetPopupTitle_text	tr(STR_languageSetPopupTitle_text)
#define languageRebootTitle_text	tr(STR_languageRebootTitle_text)
#define languageRebootBody_text	tr(STR_languageRebootBody_text)
#define languageList	tr(STR_languageList)

/* Drain-machine & self-check strings (translated) */
#define drainConfirmBody_text	tr(STR_drainConfirmBody_text)
#define timeLeftMinFmt_text	tr(STR_timeLeftMinFmt_text)
#define timeLeftSecFmt_text	tr(STR_timeLeftSecFmt_text)
#define selfCheckDescTempSensors_text	tr(STR_selfCheckDescTempSensors_text)
#define selfCheckDescWaterPump_text	tr(STR_selfCheckDescWaterPump_text)
#define selfCheckDescHeater_text	tr(STR_selfCheckDescHeater_text)
#define selfCheckDescValves_text	tr(STR_selfCheckDescValves_text)
#define selfCheckDescC1_text	tr(STR_selfCheckDescC1_text)
#define selfCheckDescC2_text	tr(STR_selfCheckDescC2_text)
#define selfCheckDescC3_text	tr(STR_selfCheckDescC3_text)
#define selfCheckDescMotor_text	tr(STR_selfCheckDescMotor_text)
#define selfCheckWaterChemFmt_text	tr(STR_selfCheckWaterChemFmt_text)
#define selfCheckPumpForward_text	tr(STR_selfCheckPumpForward_text)
#define selfCheckPumpReverse_text	tr(STR_selfCheckPumpReverse_text)
#define selfCheckPausing_text	tr(STR_selfCheckPausing_text)
#define selfCheckPumpOk_text	tr(STR_selfCheckPumpOk_text)
#define selfCheckWaterBathFmt_text	tr(STR_selfCheckWaterBathFmt_text)
#define selfCheckAllValvesOk_text	tr(STR_selfCheckAllValvesOk_text)
#define selfCheckFillingFromWB_text	tr(STR_selfCheckFillingFromWB_text)
#define selfCheckDrainBackWB_text	tr(STR_selfCheckDrainBackWB_text)
#define selfCheckDrainToWaste_text	tr(STR_selfCheckDrainToWaste_text)
#define selfCheckFlowOk_text	tr(STR_selfCheckFlowOk_text)
#define selfCheckForward_text	tr(STR_selfCheckForward_text)
#define selfCheckReverse_text	tr(STR_selfCheckReverse_text)
#define selfCheckMotorOk_text	tr(STR_selfCheckMotorOk_text)
#define otaServerRunning_text	tr(STR_otaServerRunning_text)
#define otaReceiving_text	tr(STR_otaReceiving_text)
#define otaWriting_text	tr(STR_otaWriting_text)
#define otaCompleteRebooting_text	tr(STR_otaCompleteRebooting_text)
#define otaCompleteRebootReq_text	tr(STR_otaCompleteRebootReq_text)
#define otaReadingSD_text	tr(STR_otaReadingSD_text)
#define screenOff_text	tr(STR_screenOff_text)
#define screenOffAlertMBox_text	tr(STR_screenOffAlertMBox_text)
#define screenOffNever_text	tr(STR_screenOffNever_text)
/* Checkup strings/vars */
#define checkupNexStepsTitle_text	tr(STR_checkupNexStepsTitle_text)
#define checkupProcessReady_text	tr(STR_checkupProcessReady_text)
#define checkupTheMachineWillDo_text	tr(STR_checkupTheMachineWillDo_text)
#define checkupFillWater_text	tr(STR_checkupFillWater_text)
#define checkupTankRotation_text	tr(STR_checkupTankRotation_text)
#define checkupReachTemp_text	tr(STR_checkupReachTemp_text)
#define checkupStop_text	tr(STR_checkupStop_text)
#define checkupStart_text	tr(STR_checkupStart_text)
#define checkupSkip_text	tr(STR_checkupSkip_text)
#define checkupStopNow_text	tr(STR_checkupStopNow_text)
#define checkupStopAfter_text	tr(STR_checkupStopAfter_text)
#define checkupProcessingTitle_text	tr(STR_checkupProcessingTitle_text)
#define checkupStepSource_text	tr(STR_checkupStepSource_text)
#define checkupTempControl_text	tr(STR_checkupTempControl_text)
#define checkupWaterTemp_text	tr(STR_checkupWaterTemp_text)
#define checkupNextStep_text	tr(STR_checkupNextStep_text)
#define checkupSelectBeforeStart_text	tr(STR_checkupSelectBeforeStart_text)
#define checkupTankSize_text	tr(STR_checkupTankSize_text)
#define checkupChemistryVolume_text	tr(STR_checkupChemistryVolume_text)
#define checkupMinimumChemistry_text	tr(STR_checkupMinimumChemistry_text)
#define checkupFillWaterMachine_text	tr(STR_checkupFillWaterMachine_text)
#define checkupTargetTemp_text	tr(STR_checkupTargetTemp_text)
#define checkupWater_text	tr(STR_checkupWater_text)
#define checkupChemistry_text	tr(STR_checkupChemistry_text)
#define checkupTankPosition_text	tr(STR_checkupTankPosition_text)
#define checkupFilmRotation_text	tr(STR_checkupFilmRotation_text)
#define checkupYes_text	tr(STR_checkupYes_text)
#define checkupNo_text	tr(STR_checkupNo_text)
#define checkupChecking_text	tr(STR_checkupChecking_text)
#define checkupTargetToleranceTemp_text	tr(STR_checkupTargetToleranceTemp_text)
#define checkupProcessComplete_text	tr(STR_checkupProcessComplete_text)
#define checkupProcessStopped_text	tr(STR_checkupProcessStopped_text)
#define checkupTankSizePlaceHolder_text	tr(STR_checkupTankSizePlaceHolder_text)
#define checkupChemistryLowVol_text	tr(STR_checkupChemistryLowVol_text)
#define checkupChemistryHighVol_text	tr(STR_checkupChemistryHighVol_text)
#define checkupFilling_text	tr(STR_checkupFilling_text)
#define checkupDraining_text	tr(STR_checkupDraining_text)
#define checkupProcessing_text	tr(STR_checkupProcessing_text)
#define checkupRinsingLine_text	tr(STR_checkupRinsingLine_text)
#define checkupDrainingComplete_text	tr(STR_checkupDrainingComplete_text)
#define checkupHeaterStatusFmt_text	tr(STR_checkupHeaterStatusFmt_text)
#define checkupHeaterOn_text	tr(STR_checkupHeaterOn_text)
#define checkupHeaterOff_text	tr(STR_checkupHeaterOff_text)
#define checkupTempReached_text	tr(STR_checkupTempReached_text)
#define checkupTempTimedOut_text	tr(STR_checkupTempTimedOut_text)
#define checkupContinue_text	tr(STR_checkupContinue_text)
#define checkupNoTempControl_text	tr(STR_checkupNoTempControl_text)
/* Clean Popup elements */
#define cleanPopupTitle_text	tr(STR_cleanPopupTitle_text)
#define cleanCleanProcess_text	tr(STR_cleanCleanProcess_text)
#define cleanPopupSubTitle_text	tr(STR_cleanPopupSubTitle_text)
#define cleanRoller_text	tr(STR_cleanRoller_text)
#define cleanDrainWater_text	tr(STR_cleanDrainWater_text)
#define cleanCancelButton_text	tr(STR_cleanCancelButton_text)
#define cleanCanceled_text	tr(STR_cleanCanceled_text)
#define cleanRunButton_text	tr(STR_cleanRunButton_text)
#define cleanStopButton_text	tr(STR_cleanStopButton_text)
#define cleanCloseButton_text	tr(STR_cleanCloseButton_text)
#define cleanCycleFmt_text	tr(STR_cleanCycleFmt_text)
#define cleanCurrentClean_text	tr(STR_cleanCurrentClean_text)
#define cleanCompleteClean_text	tr(STR_cleanCompleteClean_text)
#define cleanWaste_text	tr(STR_cleanWaste_text)
#define cleanDraining_text	tr(STR_cleanDraining_text)
#define cleanFilling_text	tr(STR_cleanFilling_text)
/* Drain popup texts */
#define drainStopped_text	tr(STR_drainStopped_text)
#define drainComplete_text	tr(STR_drainComplete_text)
#define drainWasteIndicator_text	tr(STR_drainWasteIndicator_text)
#define drainDrainingFmt_text	tr(STR_drainDrainingFmt_text)
#define drainDrainingC1_text	tr(STR_drainDrainingC1_text)
/* Machine fill (Maintenance) texts */
#define fillBath_text	tr(STR_fillBath_text)
#define fillChem_text	tr(STR_fillChem_text)
#define fillPopupTitle_text	tr(STR_fillPopupTitle_text)
#define fillChemPopupTitle_text	tr(STR_fillChemPopupTitle_text)
#define fillChemFilling_text	tr(STR_fillChemFilling_text)
#define fillStatusReady_text	tr(STR_fillStatusReady_text)
#define fillManualPour_text	tr(STR_fillManualPour_text)
#define fillManualFilling_text	tr(STR_fillManualFilling_text)
#define fillStart_text	tr(STR_fillStart_text)
#define fillCancel_text	tr(STR_fillCancel_text)
#define fillStatusRunning_text	tr(STR_fillStatusRunning_text)
#define fillStatusFull_text	tr(STR_fillStatusFull_text)
#define fillStatusStopped_text	tr(STR_fillStatusStopped_text)
#define fillStatusTimeout_text	tr(STR_fillStatusTimeout_text)
#define fillStatusNoFlow_text	tr(STR_fillStatusNoFlow_text)
#define fillStatusNoLevel_text	tr(STR_fillStatusNoLevel_text)
#define fillStatusDoneNoMax_text	tr(STR_fillStatusDoneNoMax_text)
#define fillFlow_fmt                            "Flow: %.1f L/min"
#define fillVolume_fmt                          "%lu ml"
#define fillInfo_fmt                            "%lu ml    %.1f L/min"
#define fillStop_text	tr(STR_fillStop_text)
#define fillClose_text	tr(STR_fillClose_text)
/* Self-check popup texts */
#define selfCheck_text	tr(STR_selfCheck_text)
#define selfCheckTasks_text	tr(STR_selfCheckTasks_text)
#define selfCheckTempSensors_text	tr(STR_selfCheckTempSensors_text)
#define selfCheckWaterPump_text	tr(STR_selfCheckWaterPump_text)
#define selfCheckHeater_text	tr(STR_selfCheckHeater_text)
#define selfCheckValves_text	tr(STR_selfCheckValves_text)
#define selfCheckContainer1_text	tr(STR_selfCheckContainer1_text)
#define selfCheckContainer2_text	tr(STR_selfCheckContainer2_text)
#define selfCheckContainer3_text	tr(STR_selfCheckContainer3_text)
#define selfCheckMotor_text	tr(STR_selfCheckMotor_text)
#define selfCheckRunning_text	tr(STR_selfCheckRunning_text)
#define selfCheckDone_text	tr(STR_selfCheckDone_text)
#define selfCheckComplete_text	tr(STR_selfCheckComplete_text)
#define selfCheckFinished_text	tr(STR_selfCheckFinished_text)
#define selfCheckSkip_text	tr(STR_selfCheckSkip_text)
#define selfCheckNext_text	tr(STR_selfCheckNext_text)
#define selfCheckRerun_text	tr(STR_selfCheckRerun_text)
#define selfCheckStopped_text	tr(STR_selfCheckStopped_text)
#define selfCheckSkipped_text	tr(STR_selfCheckSkipped_text)
#define selfCheckPumpRunning_text	tr(STR_selfCheckPumpRunning_text)
#define selfCheckTimeFmt_text	tr(STR_selfCheckTimeFmt_text)
#define selfCheckTempFmt_text	tr(STR_selfCheckTempFmt_text)
#define selfCheckValveFmt_text	tr(STR_selfCheckValveFmt_text)
/* Common button texts */
#define buttonClose_text	tr(STR_buttonClose_text)
#define buttonStop_text	tr(STR_buttonStop_text)
#define buttonStart_text	tr(STR_buttonStart_text)
#define buttonCancel_text	tr(STR_buttonCancel_text)
/* Tank size display values */
#define tankSizeValues                          {"500ml", "700ml", "1000ml"}

/* Chemistry volume display values (translated) */
#define chemVolumeValues                        {checkupChemistryLowVol_text, checkupChemistryHighVol_text}

/* UI language display values (proper nouns — same in both languages) */
#define languageValues                          {"English", "Italiano"}

/* Popup elements */
#define stopProcessPopupTitle_text	tr(STR_stopProcessPopupTitle_text)
#define warningPopupTitle_text	tr(STR_warningPopupTitle_text)
#define setMinutesPopupTitle_text	tr(STR_setMinutesPopupTitle_text)
#define setSecondsPopupTitle_text	tr(STR_setSecondsPopupTitle_text)
#define tuneTempPopupTitle_text	tr(STR_tuneTempPopupTitle_text)
#define tuneTolerancePopupTitle_text	tr(STR_tuneTolerancePopupTitle_text)
#define tuneRollerButton_text	tr(STR_tuneRollerButton_text)
#define calibResetButton_text	tr(STR_calibResetButton_text)
#define messagePopupDetailTitle_text	tr(STR_messagePopupDetailTitle_text)
#define deleteButton_text	tr(STR_deleteButton_text)
#define deletePopupTitle_text	tr(STR_deletePopupTitle_text)
#define duplicatePopupTitle_text	tr(STR_duplicatePopupTitle_text)
#define duplicateProcessPopupBody_text	tr(STR_duplicateProcessPopupBody_text)
#define duplicateStepPopupTitle_text	tr(STR_duplicateStepPopupTitle_text)
#define duplicateStepPopupBody_text	tr(STR_duplicateStepPopupBody_text)
#define deleteAllProcessPopupTitle_text	tr(STR_deleteAllProcessPopupTitle_text)
#define deletePopupBody_text	tr(STR_deletePopupBody_text)
#define deleteAllProcessPopupBody_text	tr(STR_deleteAllProcessPopupBody_text)
#define warningPopupLowWaterTitle_text	tr(STR_warningPopupLowWaterTitle_text)
#define stopNowProcessPopupBody_text	tr(STR_stopNowProcessPopupBody_text)
#define stopAfterProcessPopupBody_text	tr(STR_stopAfterProcessPopupBody_text)
#define maxNumberEntryProcessPopupBody_text	tr(STR_maxNumberEntryProcessPopupBody_text)
#define maxNumberEntryStepsPopupBody_text	tr(STR_maxNumberEntryStepsPopupBody_text)
/* Tools tab strings/vars */
#define Maintenance_text	tr(STR_Maintenance_text)
#define Utilities_text	tr(STR_Utilities_text)
#define Statistics_text	tr(STR_Statistics_text)
#define Software_text	tr(STR_Software_text)
#define cleanMachine_text	tr(STR_cleanMachine_text)
#define drainMachine_text	tr(STR_drainMachine_text)
#define importConfigAndProcesses_text	tr(STR_importConfigAndProcesses_text)
#define importConfigAndProcessesMBox_text	tr(STR_importConfigAndProcessesMBox_text)
#define importConfigAndProcessesMBox2_text	tr(STR_importConfigAndProcessesMBox2_text)
#define exportConfigAndProcesses_text	tr(STR_exportConfigAndProcesses_text)
#define exportConfigAndProcessesMBox_text	tr(STR_exportConfigAndProcessesMBox_text)
#define statCompleteProcesses_text	tr(STR_statCompleteProcesses_text)
#define statTotalProcessTime_text	tr(STR_statTotalProcessTime_text)
#define statCompleteCleanProcess_text	tr(STR_statCompleteCleanProcess_text)
#define statStoppedProcess_text	tr(STR_statStoppedProcess_text)
#define softwareVersion_text	tr(STR_softwareVersion_text)
#define softwareVersionValue_text    				"v0.0.0.1"
#define softwareSerialNum_text	tr(STR_softwareSerialNum_text)
#define softwareSerialNumValue_text 				"1234567890"
#define softwareCredits_text	tr(STR_softwareCredits_text)
#define softwareCreditsValue_text 					"Credit to Frank P. \nand \nPete B."
#define calibrationPopupTitle_text	tr(STR_calibrationPopupTitle_text)
#define calibBath_text	tr(STR_calibBath_text)
#define calibChem_text	tr(STR_calibChem_text)
#define calibrationResetPopupTitle_text	tr(STR_calibrationResetPopupTitle_text)
#define calibrationResetPopupBody_text	tr(STR_calibrationResetPopupBody_text)
/* OTA update strings */
#define otaConnecting_text	tr(STR_otaConnecting_text)
#define otaStartingServer_text	tr(STR_otaStartingServer_text)
#define otaStarting_text	tr(STR_otaStarting_text)
#define otaZeroPercent_text	tr(STR_otaZeroPercent_text)
#define otaUpdate_text	tr(STR_otaUpdate_text)
#define otaUpdateFromSD_text	tr(STR_otaUpdateFromSD_text)
#define otaUpdateFromSDMBox_text	tr(STR_otaUpdateFromSDMBox_text)
#define otaWifiUpdate_text	tr(STR_otaWifiUpdate_text)
#define otaWifiUpdateMBox_text	tr(STR_otaWifiUpdateMBox_text)
#define otaUpdating_text	tr(STR_otaUpdating_text)
#define otaNoFirmware_text	tr(STR_otaNoFirmware_text)
#define otaConfirmUpdate_text	tr(STR_otaConfirmUpdate_text)
#define otaRebootNow_text	tr(STR_otaRebootNow_text)
#define otaWifiSSID_text	tr(STR_otaWifiSSID_text)
#define otaWifiSSIDAlert_text	tr(STR_otaWifiSSIDAlert_text)
#define otaWifiPassword_text	tr(STR_otaWifiPassword_text)
#define otaWifiPasswordAlert_text	tr(STR_otaWifiPasswordAlert_text)
/* Process detail strings/vars */
#define processDetailStep_text	tr(STR_processDetailStep_text)
#define processDetailInfo_text	tr(STR_processDetailInfo_text)
#define processDetailIsColor_text	tr(STR_processDetailIsColor_text)
#define processDetailIsBnW_text	tr(STR_processDetailIsBnW_text)
#define processDetailIsTempControl_text	tr(STR_processDetailIsTempControl_text)
#define processDetailTemp_text	tr(STR_processDetailTemp_text)
#define processDetailIsPreferred_text	tr(STR_processDetailIsPreferred_text)
#define processDetailTotalTime_text	tr(STR_processDetailTotalTime_text)
#define processDetailTempPlaceHolder_text	tr(STR_processDetailTempPlaceHolder_text)
#define processDetailTempTolerance_text	tr(STR_processDetailTempTolerance_text)
#define processDetailPlaceHolder_text	tr(STR_processDetailPlaceHolder_text)
/* Step detail strings/vars */
#define stepDetailTitle_text	tr(STR_stepDetailTitle_text)
#define stepDetailName_text	tr(STR_stepDetailName_text)
#define stepDetailDuration_text	tr(STR_stepDetailDuration_text)
#define stepDetailDurationMinPlaceHolder_text	tr(STR_stepDetailDurationMinPlaceHolder_text)
#define stepDetailDurationSecPlaceHolder_text	tr(STR_stepDetailDurationSecPlaceHolder_text)
#define stepDetailType_text	tr(STR_stepDetailType_text)
#define stepDetailSource_text	tr(STR_stepDetailSource_text)
#define stepDetailDiscardAfter_text	tr(STR_stepDetailDiscardAfter_text)
#define stepDetailPlaceHolder_text	tr(STR_stepDetailPlaceHolder_text)
#define stepDetailSave_text	tr(STR_stepDetailSave_text)
#define stepDetailCancel_text	tr(STR_stepDetailCancel_text)
#define stepDetailCurrentTemp_text	tr(STR_stepDetailCurrentTemp_text)
/* Button sizes — resolved from ui_profile at runtime */
#define BUTTON_PROCESS_HEIGHT						(ui_get_profile()->buttons.process_h)
#define BUTTON_PROCESS_WIDTH						(ui_get_profile()->buttons.process_w)
#define BUTTON_START_HEIGHT							(ui_get_profile()->buttons.start_h)
#define BUTTON_START_WIDTH							(ui_get_profile()->buttons.start_w)
#define BUTTON_MBOX_HEIGHT							(ui_get_profile()->buttons.msgbox_btn_h)
#define BUTTON_MBOX_WIDTH							(ui_get_profile()->buttons.msgbox_btn_w)
#define BUTTON_POPUP_CLOSE_HEIGHT					(ui_get_profile()->buttons.popup_close_h)
#define BUTTON_POPUP_CLOSE_WIDTH					(ui_get_profile()->buttons.popup_close_w)
#define BUTTON_TUNE_HEIGHT							(ui_get_profile()->buttons.tune_h)
#define BUTTON_TUNE_WIDTH							(ui_get_profile()->buttons.tune_w)
#define LOGO_HEIGHT									(ui_get_profile()->buttons.logo_h)
#define LOGO_WIDTH									(ui_get_profile()->buttons.logo_w)

#define checkupTankSizesList	tr(STR_checkupTankSizesList)
#define checkupStepStatuses 						{ dotStep_icon, arrowStep_icon, checkStep_icon }
#define stepTypeList	tr(STR_stepTypeList)
#define stepSourceList	tr(STR_stepSourceList)
#define processSourceList							{"C1", "C2", "C3", "WB"}
#define processTempControlList						{"Off", "Run", "Susp."}
#define tanksSizesAndTimes 							{ { {250,  8}, {350, 11}, {550, 16} }, { {500,  15}, {700,  19}, {1000, 25} } } // Ml and seconds

typedef enum {
    SOURCE_C1    = 0,
    SOURCE_C2    = 1,
    SOURCE_C3    = 2,
    SOURCE_WB    = 3,
    SOURCE_WASTE = 4
} chemicalSource_t;

/* Legacy aliases — keep for backward compatibility */
#define C1    SOURCE_C1
#define C2    SOURCE_C2
#define C3    SOURCE_C3
#define WB    SOURCE_WB
#define WASTE SOURCE_WASTE		                            

/*********************
* ELEMENTS STRUCTS
*********************/
#define MAX_PROC_NAME_LEN		  20
typedef struct processNode processNode;  // Forward declaration

struct __attribute__ ((packed)) machineSettings {
	tempUnit_t				tempUnit; //0= C° 1= °F
	bool					waterInlet;
	uint8_t					filmRotationSpeedSetpoint;
	uint8_t					rotationIntervalSetpoint;
	uint8_t					randomSetpoint;
	bool					isPersistentAlarm;
	bool					isProcessAutostart;
	uint8_t					drainFillOverlapSetpoint;
	uint8_t					multiRinseTime;
	bool					lineRinseEnabled;   /* flush the shared pump line with water (WB→WASTE) after each chemistry step, to avoid cross-contamination between chemistries */
	uint8_t					lineRinseTime;      /* line-rinse flush duration in seconds */
	uint8_t					tankSize;       // 1=Small, 2=Medium, 3=Large
	uint8_t					pumpSpeed;          // 10-100% pump speed
	uint16_t				chemCalibFillSecs;  // Measured C1/C2/C3 fill time in s, 0=uncal (was chemContainerMl)
	uint16_t				wbCalibFillSecs;    // Measured WB fill time in s, 0=uncal (was wbContainerMl)
	uint8_t					chemistryVolume;    // 1=Low, 2=High
	int16_t					tempCalibOffset;    /* Calibration offset in tenths of degree (e.g., -15 = -1.5°C). int16 to avoid overflow (was int8, wrapped past ±12.7°C) */
	/* ── Splash screen settings ── */
	bool					splashRandom;       /* true = randomize palette/shape/complexity/seed each boot */
	uint8_t					splashPalette;      /* 0–9 palette index */
	uint8_t					splashShapeStyle;   /* 0–5 shape style index */
	uint8_t					splashComplexity;   /* 20–100 shape count (step 20) */
	uint32_t				splashSeed;         /* (reserved, auto-generated from tick) */
	bool					splashDefault;      /* true = use standard Deep Ocean splash */
	/* ── Wi-Fi / Remote control settings ── */
	bool					wifiEnabled;        /* true = connect to Wi-Fi on boot & run WebSocket server */
	char					wifiSSID[33];       /* SSID (max 32 chars + NUL) */
	char					wifiPassword[65];   /* Password (max 64 chars + NUL) */
	/* ── Display settings (added last for binary config compatibility) ── */
	uint8_t					brightness;         /* 10-100% LCD backlight brightness */
	uint8_t					volume;             /* 0-100% audio output volume (was legacy dimTimeout — same byte, config-compatible) */
	/* ── New fields go here (end of struct) to preserve binary config compatibility ── */
	bool					invertPump;         /* true = invert pump H-bridge direction */
	int16_t					chemCalibOffset;    /* Chemical-sensor temp offset, tenths °C (in config file). int16 to avoid overflow */
	uint8_t					language;           /* UI language: 0=English, 1=Italiano (applied at boot) */
	uint8_t					screenOffMins;      /* screen-off timeout in minutes (5/10/30), 0 = never */
};



typedef enum kbOwnerType { KB_OWNER_NONE, KB_OWNER_FILTER, KB_OWNER_PROCESS, KB_OWNER_STEP, KB_OWNER_SETTINGS } kbOwnerType;

typedef struct sKeyboardOwnerContext {
    kbOwnerType         owner;
    lv_obj_t           *textArea;
    lv_obj_t           *parentScreen;
    lv_obj_t           *saveButton;
    void               *ownerData;
    uint32_t            maxLength;      /* 0 = use default MAX_PROC_NAME_LEN */
} sKeyboardOwnerContext;

typedef enum rollerOwnerType {
    ROLLER_OWNER_NONE,
    ROLLER_OWNER_PROCESS_TEMP,
    ROLLER_OWNER_PROCESS_TOLERANCE,
    ROLLER_OWNER_STEP_MIN,
    ROLLER_OWNER_STEP_SEC
} rollerOwnerType;

typedef struct sRollerOwnerContext {
    rollerOwnerType     owner;
    lv_obj_t           *textArea;
    lv_obj_t           *saveButton;
    void               *ownerData;
} sRollerOwnerContext;
typedef struct machineStatistics {
  uint32_t 	          		completed;
  uint64_t 	          		totalMins;
  uint32_t 	          		totalSecs;   /* 0-59 — carries into totalMins */
  uint32_t 	          		stopped;
  uint32_t 	          		clean;
} machineStatistics;

/* Business data for a step — separated from UI for safe deep copy */
typedef struct sStepData {
    char                	stepNameString[MAX_PROC_NAME_LEN+1];
    bool                	somethingChanged;
    bool                	isEditMode;
    uint8_t             	timeMins;
    uint8_t             	timeSecs;
    chemicalType_t      	type;
    uint8_t             	source;
    uint8_t             	discardAfterProc;
} sStepData;

typedef struct sStepDetail {
    /* LVGL objects */
	lv_obj_t 			    *stepDetailParent;
	lv_obj_t			    *mBoxStepPopupTitleLine;
	lv_style_t			    style_mBoxStepPopupTitleLine;
	lv_point_precise_t		titleLinePoints[2];


	lv_obj_t 	        	*stepDetailNameContainer;
	lv_obj_t 	        	*stepDetailContainer;
	lv_obj_t 	        	*stepDurationContainer;
	lv_obj_t 	        	*stepTypeContainer;
	lv_obj_t 	        	*stepSourceContainer;
	lv_obj_t 	        	*stepDiscardAfterContainer;

	lv_obj_t 	        	*stepDetailLabel;
	lv_obj_t 	        	*stepDetailNamelLabel;
	lv_obj_t 	        	*stepDurationLabel;
	lv_obj_t 	        	*stepDurationMinLabel;
	lv_obj_t 	        	*stepSaveLabel;
	lv_obj_t 	        	*stepCancelLabel;
	lv_obj_t 	        	*stepTypeLabel;
	lv_obj_t 	        	*stepSourceLabel;
	lv_obj_t 	        	*stepTypeHelpIcon;
	lv_obj_t 	        	*stepSourceTempLabel;
	lv_obj_t 	        	*stepDiscardAfterLabel;
	lv_obj_t 	        	*stepSourceTempHelpIcon;
	lv_obj_t 	        	*stepSourceTempValue;

	lv_obj_t 	        	*stepDiscardAfterSwitch;

	lv_obj_t 	        	*stepSaveButton;
	lv_obj_t 	        	*stepCancelButton;

	lv_obj_t 	        	*stepSourceDropDownList;
	lv_obj_t 	        	*stepTypeDropDownList;
	lv_style_t        		*dropDownListStyle;

	lv_obj_t	        	*stepDetailSecTextArea;
	lv_obj_t	        	*stepDetailMinTextArea;
	lv_obj_t	        	*stepDetailNameTextArea;

	/* Back-reference to parent process (set during stepDetail creation).
	 * Allows event handlers to access the parent process without relying
	 * on the gui.tempProcessNode global. NOT deep-copied. */
	struct processNode      *parentProcess;

	/* Business data (deep-copyable as a single block) */
    sStepData               data;
    sKeyboardOwnerContext   nameKeyboardCtx;
    sRollerOwnerContext     minRollerCtx;
    sRollerOwnerContext     secRollerCtx;
} sStepDetail;


typedef struct singleStep { //GRAPHIC ELEMENT IN THE STEPS LIST
    /* LVGL objects */
    lv_obj_t           		*stepElement;
    lv_obj_t           		*stepElementSummary;
    lv_style_t         		stepStyle;
    lv_obj_t          	 	*stepName;
    lv_obj_t          		*stepTime;
    lv_obj_t           		*stepTimeIcon;
    lv_obj_t           		*stepTypeIcon;
    lv_obj_t           		*discardAfterIcon;
    lv_obj_t           		*sourceLabel;
    int32_t         		container_y;
    lv_obj_t          		*deleteButton;
    lv_obj_t          		*deleteButtonLabel;
    lv_obj_t          		*editButton;
    lv_obj_t          		*editButtonLabel;
    bool               		swipedLeft;
    bool               		swipedRight;
    bool               		gestureHandled;
    bool               		longPressHandled;
    /* Params objects */
    sStepDetail 	  		*stepDetails;
} singleStep;

typedef struct stepNode {
    struct stepNode   		*next;   /* Pointer to next element in list */
    struct stepNode   		*prev;   /* Pointer to previous element in list */
    singleStep         		step;   /* Step data */
} stepNode;

typedef struct stepList {
    stepNode          		*start;  /* Start of list */
    stepNode          		*end;    /* End of list */
    uint16_t           		size;   /* Number of list entries currently */
} stepList;


/* Business data for a checkup — separated from UI for safe deep copy */
typedef struct sCheckupData {
    bool                isProcessing;
    uint8_t             processStep;
    uint32_t            activeVolume_index;
    uint8_t             tankSize;
    bool                stopNow;
    bool                stopAfter;
    bool                isFilling;
    bool                isAlreadyPumping;
    bool                isDeveloping;
    uint8_t             stepFillWaterStatus;
    uint8_t             stepReachTempStatus;
    uint8_t             stepCheckFilmStatus;
    float               currentWaterTemp;
    float               currentChemTemp;
    bool                heaterOn;
    uint16_t            tempTimeoutCounter;
    bool                multiRinseChanging; /* true while a multi-rinse step is doing a mid-step drain→refill water change */
    bool                lineRinsing;        /* true while flushing the shared pump line with water after a chemistry step */
    uint16_t            lineRinseElapsed;   /* seconds elapsed in the current line-rinse flush */
} sCheckupData;

typedef struct sCheckup{
    /* LVGL objects */
	lv_obj_t			*checkupParent;
	lv_style_t			textAreaStyleCheckup;
	lv_obj_t			*checkupContainer;
	lv_obj_t			*checkupNextStepsContainer;
	lv_obj_t			*checkupProcessNameContainer;
	lv_obj_t			*checkupStepContainer;
	lv_obj_t			*checkupWaterFillContainer;
	lv_obj_t			*checkupReachTempContainer;
	lv_obj_t			*checkupTankAndFilmContainer;
	lv_obj_t			*checkupStepSourceContainer;
	lv_obj_t			*checkupTempControlContainer;
	lv_obj_t			*checkupWaterTempContainer;
	lv_obj_t			*checkupNextStepContainer;
	lv_obj_t			*checkupSelectTankChemistryContainer;
	lv_obj_t			*checkupFillWaterContainer;
	lv_obj_t			*checkupTargetTempsContainer;
	lv_obj_t			*checkupTargetTempContainer;
	lv_obj_t			*checkupTargetWaterTempContainer;
	lv_obj_t			*checkupTargetChemistryTempContainer;
	lv_obj_t			*checkupTankIsPresentContainer;
	lv_obj_t			*checkupFilmRotatingContainer;
	lv_obj_t			*checkupFilmInPositionContainer;
	lv_obj_t			*checkupProcessingContainer;


	lv_obj_t			*checkupTankSizeLabel;
	lv_obj_t			*checkupChemistryVolumeLabel;
	lv_obj_t			*checkupNextStepsLabel;
	lv_obj_t			*checkupWaterFillLabel;
	lv_obj_t			*checkupReachTempLabel;
	lv_obj_t			*checkupTankAndFilmLabel;
	lv_obj_t			*checkupMachineWillDoLabel;
	lv_obj_t			*checkupCloseButtonLabel;
	lv_obj_t			*checkupStepSourceLabel;
	lv_obj_t			*checkupTempControlLabel;
	lv_obj_t			*checkupWaterTempLabel;
	lv_obj_t			*checkupNextStepLabel;
	lv_obj_t			*checkupStopAfterButtonLabel;
	lv_obj_t			*checkupStopNowButtonLabel;
	lv_obj_t			*checkupStartButtonLabel;
	lv_obj_t			*checkupProcessReadyLabel;
	lv_obj_t			*checkupSelectBeforeStartLabel;
	lv_obj_t			*checkupFillWaterLabel;
	lv_obj_t			*checkupSkipButtonLabel;
	lv_obj_t			*checkupTargetTempLabel;
	lv_obj_t			*checkupTargetWaterTempLabel;
	lv_obj_t			*checkupTargetChemistryTempLabel;
	lv_obj_t			*checkupTankIsPresentLabel;
	lv_obj_t			*checkupFilmRotatingLabel;
	lv_obj_t			*checkupProcessCompleteLabel;

	lv_obj_t			*checkupTargetTempValue;
  	lv_obj_t		  	*checkupTargetToleranceTempValue;
	lv_obj_t			*checkupTargetWaterTempValue;
	lv_obj_t			*checkupTargetChemistryTempValue;
	lv_obj_t			*checkupHeaterStatusLabel;
	lv_obj_t			*checkupTempTimeoutLabel;
	lv_obj_t			*checkupStepSourceValue;
	lv_obj_t			*checkupTempControlValue;
	lv_obj_t			*checkupWaterTempValue;
	lv_obj_t			*checkupNextStepValue;
	lv_obj_t			*checkupProcessNameValue;
	lv_obj_t			*checkupTankIsPresentValue;
	lv_obj_t			*checkupFilmRotatingValue;
	lv_obj_t			*checkupStepTimeLeftValue;
	lv_obj_t			*checkupProcessTimeLeftValue;
	lv_obj_t			*checkupStepNameValue;
	lv_obj_t			*checkupStepKindValue;

	lv_obj_t			*checkupWaterFillStatusIcon;
	lv_obj_t			*checkupReachTempStatusIcon;
	lv_obj_t			*checkupTankAndFilmStatusIcon;

	lv_obj_t			*lowVolumeChemRadioButton;
	lv_obj_t			*highVolumeChemRadioButton;


	lv_obj_t			*checkupSkipButton;
	lv_obj_t			*checkupStartButton;
	lv_obj_t			*checkupStopAfterButton;
	lv_obj_t			*checkupStopNowButton;
	lv_obj_t			*checkupCloseButton;
  	lv_timer_t    		*processTimer;
  	lv_timer_t    		*pumpTimer;
	lv_timer_t    		*tempTimer;
	lv_timer_t    		*checkupFilmTimer;   /* Hall-based tank rotation check */

	/* Business data (deep-copyable as a single block) */
	sCheckupData        data;

	lv_obj_t			*stepArc;
	lv_obj_t			*processArc;
  	lv_obj_t			*pumpArc;

	lv_obj_t			*checkupTankSizeTextArea;
	lv_obj_t			*checkupVolumeTextArea;

	/* Runtime cursor: current step being processed (replaces gui.tempStepNode in checkup) */
	struct stepNode		*currentStep;

	/* Params objects */
} sCheckup;

/* Business data for a process — separated from UI for safe deep copy */
typedef struct sProcessData {
    char                processNameString[MAX_PROC_NAME_LEN+1];
    uint32_t            temp;
    float               tempTolerance;
    bool                isTempControlled;
    bool                isPreferred;
    bool                somethingChanged;
    filmType_t          filmType;
    uint32_t            timeMins;
    uint8_t             timeSecs;
} sProcessData;

typedef struct sProcessDetail {
    /* LVGL objects */
	lv_obj_t			*processesContainer;
	lv_obj_t			*processDetailParent;
	lv_style_t		    textAreaStyle;

	lv_obj_t			*processDetailContainer;
	lv_obj_t			*processDetailNameContainer;
	lv_obj_t			*processStepsContainer;
	lv_obj_t			*processInfoContainer;
	lv_obj_t			*processTempControlContainer;
	lv_obj_t			*processTempContainer;
	lv_obj_t			*processToleranceContainer;
	lv_obj_t			*processColorOrBnWContainer;
	lv_obj_t			*processTotalTimeContainer;

	lv_obj_t			*processDetailNameTextArea;
	lv_obj_t			*processDetailStepsLabel;
	lv_obj_t			*processDetailInfoLabel;
	lv_obj_t			*processDetailCloseButtonLabel;
	lv_obj_t			*processTempControlLabel;
	lv_obj_t			*processTempLabel;
	lv_obj_t			*processTempControlSwitch;
	lv_obj_t			*processTempUnitLabel;
	lv_obj_t			*processToleranceLabel;
	lv_obj_t			*processColorLabel;
	lv_obj_t			*processBnWLabel;
	lv_obj_t			*processPreferredLabel;
	lv_obj_t			*processSaveLabel;
	lv_obj_t			*processRunLabel;
	lv_obj_t			*processNewStepLabel;
	lv_obj_t			*processTotalTimeLabel;
	lv_obj_t			*processTotalTimeValue;

	lv_obj_t			*processDetailCloseButton;
	lv_obj_t			*processRunButton;
	lv_obj_t			*processSaveButton;
	lv_obj_t			*processNewStepButton;
	lv_obj_t			*processModifyButton;   /* toggles read-only <-> edit; shares Save's slot */
	lv_obj_t			*processModifyLabel;

	lv_obj_t			*processTempTextArea;
	lv_obj_t			*processToleranceTextArea;

	bool				editMode;               /* true = fields editable; false = read-only (view) */

	/* Non-data params (require special deep copy handling) */
    stepList          	stepElementsList;  /* Process steps list */
	sCheckup		    *checkup;

    /* Business data (deep-copyable as a single block) */
    sProcessData        data;
    sKeyboardOwnerContext nameKeyboardCtx;
    sRollerOwnerContext tempRollerCtx;
    sRollerOwnerContext toleranceRollerCtx;

} sProcessDetail;


//GRAPHIC ELEMENT IN THE PROCESS LIST
typedef struct singleProcess { 
    /* LVGL objects */
    lv_obj_t          	*processElement;
    lv_style_t        	processStyle;
    lv_obj_t          	*preferredIcon;
    lv_obj_t          	*processElementSummary;
    lv_obj_t          	*processName;
    lv_obj_t          	*processTemp;
    lv_obj_t          	*processTempIcon;
    lv_obj_t          	*processTime;
    lv_obj_t          	*processTimeIcon;
    lv_obj_t          	*processTypeIcon;
    lv_obj_t          	*deleteButton;
    lv_obj_t          	*deleteButtonLabel;
    int32_t        	container_y;
    bool               	swipedLeft;
    bool               	swipedRight;
    bool               	isFiltered;
    bool               	gestureHandled;
    bool               	longPressHandled;
    sProcessDetail     	*processDetails;  /* Process details, that includes all steps */

} singleProcess;

struct processNode {
	
    struct processNode 	*next;   /* Pointer to next element in list */
    struct processNode 	*prev;   /* Pointer to previous element in list */
    singleProcess       process; /* Process data */
};

typedef struct processList { // Linked list of processes
    processNode       	*start;  /* Start of list */
    processNode       	*end;    /* End of list */
    int32_t           	size;   /* Number of list entries currently */
} processList;

struct sRollerPopup {
	/* LVGL objects */
	lv_style_t			style_mBoxRollerTitleLine;
	lv_style_t          style_roller;
	lv_obj_t			*mBoxRollerParent;
	lv_obj_t			*mBoxRollerTitleLine;
	lv_obj_t	        *roller;
	lv_obj_t 	        *mBoxRollerContainer;
	lv_obj_t 	        *mBoxRollerTitle;
	lv_obj_t 	        *mBoxRollerButton;
	lv_obj_t 	        *mBoxRollerButtonLabel;
	lv_obj_t 	        *mBoxRollerCancelButton;
	lv_obj_t 	        *mBoxRollerCancelButtonLabel;
	lv_obj_t 	        *mBoxRollerRollerContainer;
	lv_obj_t 	        *whoCallMe;
	lv_point_precise_t	titleLinePoints[2];
	/* Params objects */
	char                *minutesOptions;
	char                *secondsOptions; 
	char                *tempFahrenheitOptions;
	char                *tempCelsiusOptions;       
	char                *tempToleranceOptions;  
};

/* Dedicated popup to tune Pump/Motor speed with a roller + live-test switch. */
struct sSpeedPopup {
	lv_obj_t            *parent;
	lv_obj_t            *container;
	lv_obj_t            *title;
	lv_obj_t            *roller;
	lv_obj_t            *testSwitch;
	lv_obj_t            *setButton;
	lv_obj_t            *targetValueLabel;   /* settings-page "%d%%" label to update on Set */
	lv_style_t          style_titleLine;
	lv_style_t          style_roller;
	lv_point_precise_t  titleLinePoints[2];
	bool                isPump;              /* true = pump, false = agitation motor */
	uint8_t             percent;             /* currently selected speed % */
	char                options[128];        /* roller options string "10%\n15%\n...\n100%" */
};

/* Dual temperature calibration popup (bath + chemical) with live sensor readings. */
struct sCalibPopup {
	lv_obj_t            *parent;
	lv_obj_t            *container;
	lv_obj_t            *title;
	lv_obj_t            *rollerBath;
	lv_obj_t            *rollerChem;
	lv_obj_t            *bathReadLabel;
	lv_obj_t            *chemReadLabel;
	lv_obj_t            *setButton;
	lv_style_t          style_titleLine;
	lv_style_t          style_rollerBath;
	lv_style_t          style_rollerChem;
	lv_point_precise_t  titleLinePoints[2];
	lv_timer_t          *liveTimer;
};

struct sFillPopup {
	lv_obj_t            *parent;
	lv_obj_t            *container;
	lv_obj_t            *title;
	lv_obj_t            *statusLabel;
	lv_obj_t            *levelBar;           /* water-bath level bargraph */
	lv_obj_t            *flowLabel;          /* live inlet flow rate */
	lv_obj_t            *actionButton;       /* Stop while filling, Close when done */
	lv_obj_t            *actionButtonLabel;
	lv_style_t          style_barIndic;
	lv_style_t          style_titleLine;
	lv_point_precise_t  titleLinePoints[2];
	lv_timer_t          *liveTimer;
};

struct sCleanPopup {
	/* LVGL objects */
	lv_style_t			    style_cleanTitleLine;
	lv_obj_t			      *cleanPopupParent;
	lv_obj_t			      *cleanPopupTitleLine;
	lv_obj_t	      		*cleanContainer;
	lv_obj_t	      		*cleanTitle;
	lv_obj_t	      		*cleanSubTitleLabel;
	lv_obj_t	      		*cleanSettingsContainer;
	lv_obj_t	      		*cleanChemicalTanksContainer;
	lv_obj_t	      		*cleanSelectC1CheckBox;
	lv_obj_t	      		*cleanSelectC2CheckBox;
	lv_obj_t	      		*cleanSelectC3CheckBox;
	lv_obj_t	      		*cleanC1CheckBoxLabel;
	lv_obj_t	      		*cleanC2CheckBoxLabel;
	lv_obj_t	      		*cleanC3CheckBoxLabel;
	lv_obj_t	      		*cleanRepeatTimesLabel;
	lv_obj_t	      		*cleanSpinBoxContainer;	
	lv_obj_t	      		*cleanSpinBox;
	lv_obj_t	      		*cleanSpinBoxPlusButton;
	lv_obj_t	      		*cleanSpinBoxMinusButton;
	lv_obj_t	      		*cleanDrainWaterLabelContainer;
	lv_obj_t	      		*cleanDrainWaterLabel;
	lv_obj_t	      		*cleanDrainWaterSwitch;
	lv_obj_t	      		*cleanCancelButton;
	lv_obj_t	      		*cleanCancelButtonLabel;
	lv_obj_t	      		*cleanRunButton;
	lv_obj_t	      		*cleanRunButtonLabel;
	lv_obj_t	      		*cleanProcessContainer;
	lv_obj_t	      		*cleanProcessArc;
	lv_obj_t	      		*cleanCycleArc;
	lv_obj_t	      		*cleanPumpArc;
	lv_obj_t	      		*cleanRemainingTimeValue;
	lv_obj_t	      		*cleanNowStepLabelValue;
	lv_obj_t	      		*cleanNowCleaningLabel;
	lv_obj_t	      		*cleanNowCleaningValue;
	lv_obj_t	      		*cleanStopButton;
	lv_obj_t	      		*cleanStopButtonLabel;
	lv_timer_t              *pumpTimer;
	lv_timer_t              *wasteTimer;
	lv_point_precise_t	titleLinePoints[2];
  /* Params objects */
	uint8_t 				cleanCycles;
	bool					cleanDrainWater;
	bool                    containerToClean[3];
	bool 					stopNowPressed;
	bool 					isAlreadyPumping;
	uint32_t				totalMins;
	uint32_t				totalSecs;
	uint8_t					stepDirection;
	bool	 				isCleaning;
};


struct sFilterPopup {
	/* LVGL objects */
	lv_obj_t			      *mBoxFilterPopupParent;
	lv_obj_t			      *mBoxStepPopupTitleLine;
	lv_style_t			    style_mBoxTitleLine;
	lv_point_precise_t	titleLinePoints[2];

    
	lv_obj_t	      		*mBoxContainer;
	lv_obj_t	      		*mBoxTitle;
	lv_obj_t	      		*mBoxNameContainer;
	lv_obj_t	      		*mBoxNameLabel;
	lv_obj_t	      		*selectColorContainerRadioButton;
	lv_obj_t	      		*selectBnWContainerRadioButton;
	lv_obj_t	      		*mBoxColorLabel;
	lv_obj_t	      		*mBoxBnWLabel;
	lv_obj_t	      		*mBoxPreferredContainer;
	lv_obj_t	      		*mBoxPreferredLabel;
	lv_obj_t	      		*mBoxResetFilterLabel;
	lv_obj_t	      		*mBoxApplyFilterLabel;


	lv_obj_t	      		*mBoxNameTextArea;
	lv_obj_t	      		*mBoxSelectColorRadioButton;
	lv_obj_t	      		*mBoxSelectBnWRadioButton;
	lv_obj_t	      		*mBoxOnlyPreferredSwitch;
	lv_obj_t	      		*mBoxResetFilterButton;
	lv_obj_t	      		*mBoxApplyFilterButton;
	lv_obj_t	      		*mBoxCloseButton;
	lv_obj_t	      		*mBoxCloseButtonLabel;


	/* Params objects */
  char                filterName[MAX_PROC_NAME_LEN + 1];
  bool                isColorFilter;
  bool                isBnWFilter;
  bool                preferredOnly;
  sKeyboardOwnerContext nameKeyboardCtx;
};


struct sDrainPopup {
	/* Main containers */
	lv_obj_t			*drainPopupParent;
	lv_obj_t			*drainContainer;
	lv_obj_t			*drainTitle;
	lv_obj_t			*drainTitleLine;
	lv_style_t			 style_drainTitleLine;
	lv_point_precise_t	 titleLinePoints[2];

	/* Close button */
	lv_obj_t			*drainCloseButton;
	lv_obj_t			*drainCloseButtonLabel;

	/* Confirm phase */
	lv_obj_t			*drainConfirmContainer;
	lv_obj_t			*drainInfoLabel;
	lv_obj_t			*drainStartButton;
	lv_obj_t			*drainStartButtonLabel;
	lv_obj_t			*drainCancelButton;
	lv_obj_t			*drainCancelButtonLabel;

	/* Process phase */
	lv_obj_t			*drainProcessContainer;
	lv_obj_t			*tankBar[4];
	lv_obj_t			*tankLabel[4];
	lv_obj_t			*drainStatusLabel;
	lv_obj_t			*drainWasteLabel;
	lv_obj_t			*drainTimeLabel;
	lv_obj_t			*drainStopButton;
	lv_obj_t			*drainStopButtonLabel;

	/* Timer */
	lv_timer_t			*drainTimer;

	/* Data */
	bool				 isDraining;
	bool				 stopNowPressed;
	uint8_t				 currentTank;
	int32_t				 tankElapsed;
	int32_t				 totalElapsed;
};

struct sSelfcheckPopup {
	lv_obj_t			*selfcheckPopupParent;
	lv_obj_t			*selfcheckContainer;
	lv_obj_t			*selfcheckTitle;
	lv_obj_t			*selfcheckTitleLine;
	lv_style_t			 style_selfcheckTitleLine;
	lv_point_precise_t	 titleLinePoints[2];

	/* Left panel - task list */
	lv_obj_t			*leftPanel;
	lv_obj_t			*tasksLabel;
	lv_obj_t			*phaseIcon[8];
	lv_obj_t			*phaseNameLabel[8];

	/* Right panel - current phase */
	lv_obj_t			*rightPanel;
	lv_obj_t			*phaseTitle;
	lv_obj_t			*phaseDescription;
	lv_obj_t			*phaseStatus;
	lv_obj_t			*phaseTimer;

	/* Progress bar (for container phases) */
	lv_obj_t			*progressBar;

	/* Close button (X, green) */
	lv_obj_t			*closeButton;
	lv_obj_t			*closeButtonLabel;

	/* Buttons */
	lv_obj_t			*stopButton;
	lv_obj_t			*stopButtonLabel;
	lv_obj_t			*startButton;
	lv_obj_t			*startButtonLabel;
	lv_obj_t			*advanceButton;
	lv_obj_t			*advanceButtonLabel;

	/* State */
	lv_timer_t			*checkTimer;
	uint8_t				 currentPhase;
	uint8_t				 phaseElapsed;
	bool				 isRunning;
};

struct sOtaProgressPopup {
	lv_obj_t			*popupParent;
	lv_obj_t			*popupContainer;
	lv_obj_t			*titleLabel;
	lv_obj_t			*titleLine;
	lv_style_t			 style_titleLine;
	lv_point_precise_t	 titleLinePoints[2];
	lv_obj_t			*closeButton;
	lv_obj_t			*closeButtonLabel;
	lv_obj_t			*statusLabel;
	lv_obj_t			*progressBar;
	lv_obj_t			*percentLabel;
};

struct sOtaWifiPopup {
	lv_obj_t			*popupParent;
	lv_obj_t			*popupContainer;
	lv_obj_t			*titleLabel;
	lv_obj_t			*titleLine;
	lv_style_t			 style_titleLine;
	lv_point_precise_t	 titleLinePoints[2];
	lv_obj_t			*closeButton;
	lv_obj_t			*closeButtonLabel;
	lv_obj_t			*ipLabel;
	lv_obj_t			*pinLabel;
	lv_obj_t			*statusLabel;
	lv_obj_t			*progressBar;
	char				 otaPin[9]; /* 8 digits + null (WPA2 min password) */
};

/* ── Wi-Fi configuration scan results ── */
#define MAX_WIFI_SCAN_RESULTS 15

typedef struct {
	char ssid[33];
	int8_t rssi;
	bool open;  /* true = no password needed */
} wifiScanResult_t;

struct sWifiPopup {
	lv_obj_t            *popupParent;
	lv_obj_t            *popupContainer;
	lv_obj_t            *titleLabel;
	lv_obj_t            *titleLine;
	lv_style_t           style_titleLine;
	lv_point_precise_t   titleLinePoints[2];
	lv_obj_t            *closeButton;
	lv_obj_t            *closeButtonLabel;
	lv_obj_t            *statusLabel;
	lv_obj_t            *scanButton;
	lv_obj_t            *scanButtonLabel;
	lv_obj_t            *listContainer;      /* scrollable list of scan results */
	lv_obj_t            *connectButton;
	lv_obj_t            *connectButtonLabel;
	lv_obj_t            *autoConnectContainer;
	lv_obj_t            *autoConnectLabel;
	lv_obj_t            *autoConnectSwitch;
	/* Scan state */
	wifiScanResult_t     scanResults[MAX_WIFI_SCAN_RESULTS];
	int                  scanCount;
	int                  selectedIndex;       /* -1 = none */
	char                 pendingPassword[65];
	sKeyboardOwnerContext wifiPasswordKbCtx;
};

struct sMessagePopup {
	/* LVGL objects */
	lv_obj_t			      *mBoxPopupParent;
	lv_obj_t			      *mBoxPopupTitleLine;
	lv_style_t			    style_mBoxPopupTitleLine;
	lv_point_precise_t	titleLinePoints[2];

	lv_obj_t		      	*mBoxPopupContainer;
	lv_obj_t		      	*mBoxPopupTextContainer;


	lv_obj_t		      	*mBoxPopupTitle;
	lv_obj_t		      	*mBoxPopupText;
	lv_obj_t		      	*mBoxPopupButtonLabel;
	lv_obj_t		      	*mBoxPopupButton1Label;
	lv_obj_t		      	*mBoxPopupButton2Label;

	lv_obj_t		      	*mBoxPopupButtonClose;
	lv_obj_t		      	*mBoxPopupButton1;
	lv_obj_t		      	*mBoxPopupButton2;  

  void    	         	*whoCallMe;
	/* Params objects */
};



/*********************
* MENU STRUCT
*********************/
struct sMenu {
    /* LVGL objects */
	lv_obj_t 			*oldTabSelected;
	lv_obj_t 			*newTabSelected;
	lv_obj_t			*screen_mainMenu;
	lv_obj_t			*processesTab;
	lv_obj_t			*settingsTab;
	lv_obj_t			*toolsTab;
	lv_obj_t			*iconLabel;
	lv_obj_t			*label;
	lv_obj_t			*wifiStatusIcon;

	/* Params objects */
	uint8_t				oldSelection;
	uint8_t				newSelection;
};


//THIS IS THE PROCESSES "PAGE" IN THE "PROCESS LIST" TAB
struct sProcesses {
  /* LVGL objects */
  lv_obj_t			    *processesSection;
  lv_obj_t			    *sectionTitleLine;
  lv_style_t			style_sectionTitleLine;
  lv_point_precise_t	titleLinePoints[2];


  lv_obj_t	        	*processesLabel;
  lv_obj_t	        	*iconFilterLabel;
  lv_obj_t	        	*iconNewProcessLabel;
  lv_obj_t	        	*processesListContainer;

  lv_obj_t	        	*processFilterButton;
  lv_obj_t	        	*newProcessButton;
  bool                isFiltered;
  /* Params objects */
  processList           processElementsList;
};


struct sSettings {
    /* LVGL objects */
	lv_obj_t			      *settingsSection;
	lv_obj_t			      *sectionTitleLine;
	lv_style_t			    style_sectionTitleLine;
	lv_point_precise_t	titleLinePoints[2];
	int32_t 			    pad;

	lv_obj_t 	        	*settingsLabel;
	lv_obj_t 	        	*tempUnitLabel;
	lv_obj_t 	        	*waterInletLabel;
	lv_obj_t 	        	*tempSensorTuneButtonLabel;
	lv_obj_t 	        	*tempSensorTuningLabel;
	lv_obj_t 	        	*filmRotationSpeedLabel;
	lv_obj_t 	        	*filmRotationInverseIntervalLabel;
	lv_obj_t 	        	*filmRotationRandomLabel;
	lv_obj_t 	        	*persistentAlarmLabel;
	lv_obj_t 	        	*autostartLabel;
	lv_obj_t 	        	*drainFillTimeLabel;
  lv_obj_t 	        	*multiRinseTimeLabel;
  lv_obj_t 	        	*lineRinseLabel;
  lv_obj_t 	        	*lineRinseTimeLabel;

	lv_obj_t 	        	*drainFillTimeValueLabel;
  lv_obj_t 	        	*multiRinseTimeValueLabel;
  lv_obj_t 	        	*lineRinseTimeValueLabel;
	lv_obj_t 	        	*filmRotationInverseIntervalValueLabel;
	lv_obj_t 	        	*filmRotationRandomValueLabel;
	lv_obj_t 	        	*filmRotationSpeedValueLabel;

	lv_obj_t 	        	*settingsContainer;
	lv_obj_t 	        	*tempUnitContainer;
	lv_obj_t 	        	*waterInletContainer;
	lv_obj_t 	        	*tempTuningContainer;
	lv_obj_t 	        	*filmRotationSpeedContainer;
	lv_obj_t 	        	*filmRotationInverseIntervalContainer;
	lv_obj_t 	        	*randomContainer;
	lv_obj_t 	        	*persistentAlarmContainer;
	lv_obj_t 	        	*autostartContainer;
	lv_obj_t 	        	*drainFillTimeContainer;
  lv_obj_t 	        	*multiRinseTimeContainer;
  lv_obj_t 	        	*lineRinseContainer;
  lv_obj_t 	        	*lineRinseTimeContainer;

	lv_obj_t 	        	*autostartSwitch;
	lv_obj_t 	        	*persistentAlarmSwitch;
	lv_obj_t 	        	*waterInletSwitch;
	lv_obj_t 	        	*lineRinseSwitch;

	lv_obj_t 	        	*filmRotationSpeedSlider;
	lv_obj_t 	        	*motorSpeedTuneButton;
	lv_obj_t 	        	*motorSpeedTuneButtonLabel;
	lv_obj_t 	        	*filmRotationInversionIntervalSlider;
	lv_obj_t 	        	*filmRandomSlider;
	lv_obj_t 	        	*drainFillTimeSlider;
  lv_obj_t 	        	*multiRinseTimeSlider;
  lv_obj_t 	        	*lineRinseTimeSlider;

	lv_obj_t	        	*tempSensorTuneButton;
	lv_obj_t 	        	*tempCalibOffsetValueLabel;  /* Display current calibration offset */
	lv_obj_t 	        	*tempUnitCelsiusRadioButton;
	lv_obj_t 	        	*tempUnitFahrenheitRadioButton;

	uint32_t 	        	active_index;

	lv_obj_t                *tankSizeContainer;
	lv_obj_t                *tankSizeLabel;
	lv_obj_t                *tankSizeTextArea;
	uint32_t                tankSize_active_index;

	lv_obj_t                *pumpSpeedContainer;
	lv_obj_t                *pumpSpeedLabel;
	lv_obj_t                *pumpSpeedSlider;
	lv_obj_t                *pumpSpeedTuneButton;
	lv_obj_t                *pumpSpeedTuneButtonLabel;
	lv_obj_t                *pumpSpeedValueLabel;

	lv_obj_t                *invertPumpContainer;
	lv_obj_t                *invertPumpLabel;
	lv_obj_t                *invertPumpSwitch;

	lv_obj_t                *volumeContainer;
	lv_obj_t                *volumeLabel;
	lv_obj_t                *volumeTuneButton;
	lv_obj_t                *volumeTuneButtonLabel;
	lv_obj_t                *volumeValueLabel;

	lv_obj_t                *brightnessContainer;
	lv_obj_t                *brightnessLabel;
	lv_obj_t                *brightnessSlider;
	lv_obj_t                *brightnessValueLabel;

	lv_obj_t                *chemVolumeContainer;
	lv_obj_t                *chemVolumeLabel;
	lv_obj_t                *chemVolumeTextArea;

	/* UI language row */
	lv_obj_t                *languageContainer;
	lv_obj_t                *languageLabel;
	lv_obj_t                *languageTextArea;

	/* Screen-off timeout row */
	lv_obj_t                *screenOffContainer;
	lv_obj_t                *screenOffLabel;
	lv_obj_t                *screenOffTextArea;

	/* Splash screen settings row */
	lv_obj_t                *splashContainer;
	lv_obj_t                *splashLabel;
	lv_obj_t                *splashButton;
	lv_obj_t                *splashButtonLabel;

	/* Wi-Fi settings row */
	lv_obj_t                *wifiContainer;
	lv_obj_t                *wifiLabel;
	lv_obj_t                *wifiButton;
	lv_obj_t                *wifiButtonLabel;

	/* Reset to Defaults row */
	lv_obj_t                *resetContainer;
	lv_obj_t                *resetLabel;
	lv_obj_t                *resetButton;
	lv_obj_t                *resetButtonLabel;

  /* Params objects */
  struct machineSettings   settingsParams;
};


struct sTools {
    /* LVGL objects */
	lv_obj_t			      *toolsSection;
	lv_obj_t			      *sectionTitleLine;
	lv_style_t			    style_sectionTitleLine;
	lv_point_precise_t	titleLinePoints[2];

	lv_obj_t 	        	*toolsCleaningContainer;
	lv_obj_t 	        	*toolsDrainingContainer;
	lv_obj_t 	        	*toolsSelfcheckContainer;
	lv_obj_t 	        	*toolsFillContainer;
	lv_obj_t 	        	*toolsFillChemContainer;
	lv_obj_t 	        	*toolsImportContainer;
	lv_obj_t 	        	*toolsExportContainer;


	lv_obj_t 	        	*toolsCleaningLabel;
	lv_obj_t 	        	*toolsDrainingLabel;
	lv_obj_t 	        	*toolsSelfcheckLabel;
	lv_obj_t 	        	*toolsFillLabel;
	lv_obj_t 	        	*toolsImportLabel;
	lv_obj_t 	        	*toolsExportLabel;

	lv_obj_t 	        	*toolsCleaningButton;
  lv_obj_t 	        	*toolsCleaningButtonLabel;
	lv_obj_t 	        	*toolsDrainingButton;
  lv_obj_t 	        	*toolsDrainingButtonLabel;
	lv_obj_t 	        	*toolsSelfcheckButton;
  lv_obj_t 	        	*toolsSelfcheckButtonLabel;
	lv_obj_t 	        	*toolsFillButton;
  lv_obj_t 	        	*toolsFillButtonLabel;
	lv_obj_t 	        	*toolsFillChemLabel;
	lv_obj_t 	        	*toolsFillChemButton;
  lv_obj_t 	        	*toolsFillChemButtonLabel;
	lv_obj_t 	        	*toolsImportButton;
  lv_obj_t 	          *toolsImportButtonLabel;
	lv_obj_t 	        	*toolsExportButton;
  lv_obj_t 	        	*toolsExportButtonLabel;

	lv_obj_t 	        	*toolsMaintenanceLabel;
	lv_obj_t 	        	*toolsUtilitiesLabel;
	lv_obj_t 	        	*toolsStatisticsLabel;
	lv_obj_t 	        	*toolsSoftwareLabel;

	lv_obj_t 	        	*toolsStatCompleteProcessesContainer;
	lv_obj_t 	        	*toolsStatTotalTimeContainer;
	lv_obj_t 	        	*toolsStatCompleteCycleContainer;
	lv_obj_t 	        	*toolsStatStoppedProcessesContainer;

	lv_obj_t 	        	*toolStatCompletedProcessesLabel;
	lv_obj_t 	        	*toolStatCompletedProcessesValue;
	lv_obj_t 	        	*toolStatTotalTimeLabel;
	lv_obj_t 	        	*toolStatTotalTimeValue;
	lv_obj_t 	        	*toolStatCompleteCycleLabel;
	lv_obj_t 	        	*toolStatCompleteCycleValue;
	lv_obj_t 	        	*toolStatStoppedProcessesLabel;
	lv_obj_t 	        	*toolStatStoppedProcessesValue;


	lv_obj_t 	        	*toolsSoftwareVersionContainer;
	lv_obj_t 	        	*toolsSoftwareSerialContainer;

	lv_obj_t 	        	*toolSoftWareVersionLabel;
	lv_obj_t 	        	*toolSoftwareVersionValue;
	lv_obj_t 	        	*toolSoftwareSerialLabel;
	lv_obj_t 	        	*toolSoftwareSerialValue;

	lv_obj_t 	        	*toolCreditButton;
	lv_obj_t 	        	*toolCreditButtonLabel;

	/* OTA Update UI */
	lv_obj_t			*toolsUpdateContainer;
	lv_obj_t			*toolsUpdateSDLabel;
	lv_obj_t			*toolsUpdateSDButton;
	lv_obj_t			*toolsUpdateSDButtonLabel;
	lv_obj_t			*toolsUpdateWifiContainer;
	lv_obj_t			*toolsUpdateWifiLabel;
	lv_obj_t			*toolsUpdateWifiButton;
	lv_obj_t			*toolsUpdateWifiButtonLabel;

	/* Params objects */
  struct machineStatistics machineStats;
};

struct sKeyboardPopup {
	/* LVGL objects */
  lv_obj_t          *keyBoardParent;
	lv_obj_t 			    *keyboard;
	lv_obj_t			    *keyboardTextArea;
};

struct sSplashPopup {
	lv_obj_t			*splashPopupParent;
	lv_obj_t			*splashContainer;
	lv_obj_t			*previewContainer;      /* background preview of splash shapes */
	lv_obj_t			*overlayRect;           /* semi-transparent dark overlay for readability */
	lv_obj_t			*splashTitle;
	lv_obj_t			*splashTitleLine;
	lv_style_t			 style_titleLine;
	lv_point_precise_t	 titleLinePoints[2];
	lv_obj_t			*defaultSwitch;
	lv_obj_t			*defaultLabel;
	lv_obj_t			*randomSwitch;
	lv_obj_t			*randomLabel;
	lv_obj_t			*optionsContainer;
	lv_obj_t			*paletteLabel;
	lv_obj_t			*paletteTextArea;
	lv_obj_t			*shapeLabel;
	lv_obj_t			*shapeTextArea;
	lv_obj_t			*complexityLabel;
	lv_obj_t			*complexitySlider;
	lv_obj_t			*randomButton;          /* bottom button — regenerate random splash */
	lv_obj_t			*randomButtonLabel;
	lv_obj_t			*xCloseButton;          /* X button top-right */
	lv_obj_t			*xCloseButtonLabel;
};


/*********************
* ELEMENTS STRUCT
*********************/
struct sElements {
	struct sFilterPopup			filterPopup;
  struct sCleanPopup      cleanPopup;
  struct sDrainPopup      drainPopup;
  struct sSelfcheckPopup  selfcheckPopup;
  struct sOtaWifiPopup    otaWifiPopup;
  struct sOtaProgressPopup otaProgressPopup;
	struct sMessagePopup 		messagePopup;
	struct sRollerPopup			rollerPopup;
	struct sSpeedPopup			speedPopup;
	struct sCalibPopup			calibPopup;
	struct sFillPopup			fillPopup;
  struct sKeyboardPopup   keyboardPopup;
  struct sSplashPopup     splashPopup;
  struct sWifiPopup       wifiPopup;
};


/*********************
* PAGES STRUCT
*********************/
struct sPages {
	struct sMenu				  menu;
	struct sProcesses			processes;
	struct sSettings			settings;
	struct sTools				  tools;
};


/*********************
* ALL ELEMENTS/PAGES GUI COMPONENT STRUCT
*********************/
struct gui_components {
	struct sElements	element;
	struct sPages		page;
	processNode			*tempProcessNode;
	stepNode			*tempStepNode;
};

struct sys_components {

	QueueHandle_t			sysActionQ;
	QueueHandle_t			motorActionQ;
	uint8_t					minVal_rotationSpeedPercent;
	uint8_t					maxVal_rotationSpeedPercent;
	uint8_t					analogVal_rotationSpeedPercent;
};

typedef struct _LVGLObjectScale {
	
    lv_obj_t *obj;
    int32_t original_width;
    int32_t original_height;
    float current_scale;
} LVGLObjectScale;

/*********************
* GLOBAL DEFINES
*********************/

/*********************
* System manager defines
*********************/
#define SAVE_PROCESS_CONFIG         0x0001
#define SAVE_MACHINE_STATS          0x0002
#define RELOAD_CFG                  0x0003
#define EXPORT_CFG					0x0004
#define TANK_ROTATION               0x0005
#define SELFTEST_VALVES             0x0006   /* deferred boot valve self-test (runs in sysMan, off the display path) */

/* Our Fonts */
LV_FONT_DECLARE(FilMachineFontIcons_15);
LV_FONT_DECLARE(FilMachineFontIcons_20);
LV_FONT_DECLARE(FilMachineFontIcons_30);
LV_FONT_DECLARE(FilMachineFontIcons_40);
LV_FONT_DECLARE(FilMachineFontIcons_50);
LV_FONT_DECLARE(FilMachineFontIcons_60);
LV_FONT_DECLARE(FilMachineFontIcons_100);
LV_FONT_DECLARE(lv_font_montserrat_64);

/* Custom splash title fonts */
LV_FONT_DECLARE(font_air_americana_48);
LV_FONT_DECLARE(font_decaying_felt_pen_48);
LV_FONT_DECLARE(font_ds_digital_48);
LV_FONT_DECLARE(font_evanescent_48);
LV_FONT_DECLARE(font_nerdropol_lattice_48);
LV_FONT_DECLARE(font_retrolight_48);
LV_FONT_DECLARE(font_tropical_leaves_48);
LV_FONT_DECLARE(font_wishful_melisande_48);

/* HELPER UTILITIES Function Prototypes */
// @file accessories.c
int32_t roundToStep(int32_t value, int32_t step);
void safeTimerDelete(lv_timer_t **timer);

/* ELEMENTS Function Prototypes */
// @file element_filterPopup.c
void event_filterMBox(lv_event_t *e);
void filterPopupCreate(void);
// @file element_messagePopup.c
void messagePopupCreate(const char *popupTitleText, const char *popupText, const char *textButton1, const char *textButton2, void *whoCallMe);
void event_messagePopup(lv_event_t *e);
void *getProcessDiscardSentinel(void);
processNode *getProcessDetailBackup(void);
void clearProcessDetailBackup(void);
// @file element_cleanPopup.c
void event_cleanPopup(lv_event_t *e);
void cleanPopup(void);
// @file element_drainPopup.c
void event_drainPopup(lv_event_t *e);
void drainPopupCreate(void);
// @file element_selfcheckPopup.c
void event_selfcheckPopup(lv_event_t *e);
void selfcheckPopupCreate(void);
// @file element_splashPopup.c
void splashPopupCreate(void);
void splashPopupRefreshPreview(void);
// @file page_splash.c
void splash_preview_generate(lv_obj_t *parent, uint32_t seed,
                              uint8_t palette_idx, uint8_t shape_style,
                              uint8_t complexity);
void splash_standard_preview_generate(lv_obj_t *parent);
// @file element_process.c
void event_processElement(lv_event_t *e);
void processElementCreate(processNode *newProcess, int32_t tempSize);
bool deleteProcessElement( processNode	*processToDelete );
processNode *getProcElementEntryByObject( lv_obj_t *obj );
processNode *addProcessElement(processNode	*processToAdd);
// @file element_rollerPopup.c
void event_Roller(lv_event_t *e);
void rollerPopupCreate(const char * tempOptions,const char * popupTitle, void *whoCallMe, uint32_t currentVal, uint32_t accentColor);
// @file element_step.c
void event_stepElement(lv_event_t *e);
void stepElementCreate(stepNode * newStep,processNode * processReference, int8_t tempSize);
bool deleteStepElement( stepNode	*stepToDelete, processNode * processReference , bool isDeleteProcess);
stepNode *addStepElement(stepNode * stepToAdd, processNode * processReference);
void insertStepElementAfter(processNode *data, stepNode *afterNode, stepNode *node);
void reorderStepElements(processNode *data);

/* PAGES Function Prototypes */
void initGlobals( void );
// @file page_checkup.c
void event_checkup(lv_event_t *e);
void checkup(processNode *referenceProcess);
/* Runtime getters for WebSocket server */
processNode *checkup_find_active_process(void);
void     checkup_reset_state(void);
uint8_t  checkup_get_step_percentage(void);
uint8_t  checkup_get_process_percentage(void);
uint8_t  checkup_get_tank_percentage(void);
uint32_t checkup_get_step_elapsed_mins(void);
uint8_t  checkup_get_step_elapsed_secs(void);
uint32_t checkup_get_step_left_mins(void);
uint8_t  checkup_get_step_left_secs(void);
uint32_t checkup_get_process_left_mins(void);
uint8_t  checkup_get_process_left_secs(void);
uint32_t checkup_get_process_elapsed_mins(void);
uint8_t  checkup_get_process_elapsed_secs(void);
// @file page_menu.c
void event_tab_switch(lv_event_t *e);
void menu(void);
// @file page_processDetail.c
void event_processDetail(lv_event_t *e);
void event_processDetail_style_delete(lv_event_t *e);
void processDetail(lv_obj_t *referenceProcess);
void process_detail_live_update(processNode *pn);
void step_detail_live_update(stepNode *sn);
// @file page_processes.c
void event_processes_style_delete(lv_event_t *e);
void event_tabProcesses(lv_event_t *e);
void processes(void);
void refreshProcessesLabel(void);
// @file page_settings.c
void event_settings_style_delete(lv_event_t *e);
void event_settingPopupMBox(lv_event_t *e);
void event_settings_handler(lv_event_t *e);
void initSettings(void);
void settings(void);
void refreshSettingsUI(void);
void settingsApplyFactoryDefaults(void);   /* restore all settings + refresh UI + save */
// @file page_stepDetail.c
void event_stepDetail(lv_event_t *e);
void stepDetail(processNode *referenceNode, stepNode *currentNode);
// @file page_tools.c
void event_toolsElement(lv_event_t *e);
void initTools(void);
void tools(void);
void tools_pause_timer(void);
void tools_resume_timer(void);
void tools_delete_timer(void);
// @file element_speedPopup.c
void speedPopupCreate(bool isPump, uint8_t currentPercent);
void volumePopupCreate(uint8_t currentPercent);
// @file element_calibPopup.c
void calibPopupCreate(void);
// @file accessories.c — per-sensor temperature calibration offsets
void    loadChemCalibOffset(void);
void    setChemCalibOffset(int16_t offsetTenths);
int16_t getChemCalibOffset(void);
/* Temperature: a background poller is the SOLE reader of the (slow, blocking)
 * DS18B20 bus; the UI reads getCachedTemperature() which never blocks. */
float   getCachedTemperature(uint8_t sensorIndex);
void    temperaturePollerStart(void);

/* ── Machine (water-bath) fill: open WB valve, meter the volume in ──
 * The water inlet is already pressurized, so no pump is used — we only open the
 * WB valve. The flow meter integrates the dispensed volume and the fill stops
 * once FILL_TARGET_ML has gone in (the MAX level switch is a safety backstop).
 * If no flow is seen shortly after opening the valve, the fill aborts.
 *
 * FILL_TARGET_ML: 5000 for the 5-litre bath (500 for a small bench test). */
#ifndef FILL_TARGET_ML
#define FILL_TARGET_ML   5000
#endif
/* Bench testing: set to 1 to ignore the MAX float switch (rely on metered
 * volume only) while the MAX sensor wiring is being sorted out. */
#ifndef FILL_IGNORE_MAX
#define FILL_IGNORE_MAX  0
#endif
/* Clean/drain bargraphs: when 1, the per-container min/max float sensors finish
 * each fill/drain step as soon as the container is really full/empty (instead of
 * waiting out the capacity-based time estimate). Keep 0 until the 6 chem sensors
 * are installed and verified — without sensors the readings would be ambiguous. */
#ifndef CLEAN_USE_LEVEL_SENSORS
#define CLEAN_USE_LEVEL_SENSORS  0
#endif
#define FILL_IDLE        0
#define FILL_RUNNING     1
#define FILL_FULL        2   /* target reached (or MAX tripped), MAX confirmed  */
#define FILL_STOPPED     3
#define FILL_TIMEOUT     4
#define FILL_NOFLOW      5   /* flow meter saw no water after opening the valve */
#define FILL_NOLEVEL     6   /* flow OK but low float never wetted (not filling)*/
#define FILL_DONE_NOMAX  7   /* target volume in, but MAX switch not confirmed  */
#define FILL_TARGET_WB    0   /* water bath (inlet/manual, level or flow) */
#define FILL_TARGET_CHEM  1   /* a chemistry container filled from the WB by pump */
void     machineFillStart(uint8_t target);
void     machineFillSetTarget(uint8_t target);   /* set target before Start (popup helpers) */
void     machineFillStop(void);
int      machineFillState(void);
float    machineFillFlowLpm(void);   /* live inlet flow rate, L/min           */
uint32_t machineFillVolumeMl(void);  /* metered volume dispensed so far, ml    */
int      machineFillProgress(void);  /* 0..100 = volume / FILL_TARGET_ML       */
int      machineFillLevelPct(void);  /* 0/50/100 from the MIN/MAX floats        */
bool     machineFillBathFull(void);  /* true = MAX float already wet right now  */
bool     machineFillManual(void);    /* true = no inlet, fill by hand (floats)  */
void     fillPopupCreate(uint8_t target);   /* @file element_fillPopup.c (FILL_TARGET_*) */
// @file accessories.c
uint8_t pumpPercentToDuty(uint8_t pct);
// @file ota_update.c
const char *ota_get_running_version(void);
int         ota_get_progress(void);
const char *ota_get_status_text(void);
bool        ota_is_running(void);
bool        ota_check_sd(char *version_out, size_t version_out_len);
bool        ota_start_sd(void);
bool        ota_wifi_server_start(void);
bool        ota_wifi_server_stop(void);
bool        ota_wifi_server_is_running(void);
const char *ota_get_ip_address(void);
// @file element_otaWifiPopup.c
void otaWifiPopupCreate(void);
void event_otaWifiPopup(lv_event_t *e);
void otaProgressPopupCreate(const char *title);
void event_otaProgressPopup(lv_event_t *e);
// @file element_wifiPopup.c
void wifiPopupCreate(void);
void event_wifiPopup(lv_event_t *e);
// Wi-Fi scan API
int  wifi_scan_start(void);
int  wifi_scan_get_results(wifiScanResult_t *results, int max_results);
bool wifi_connect(const char *ssid, const char *password);
void wifi_disconnect(void);
bool wifi_is_connected(void);
const char *wifi_get_connected_ssid(void);
const char *wifi_get_ip_address(void);
void wifi_boot_auto_connect(void);  /* Call once after readConfigFile to auto-connect if enabled */
/* NVS-based Wi-Fi credential persistence (works without SD card) */
void wifi_nvs_save(const char *ssid, const char *password);
bool wifi_nvs_load(char *ssid_buf, size_t ssid_sz, char *pwd_buf, size_t pwd_sz);
void wifi_nvs_clear(void);
void wifi_popup_connection_result(void); /* Save credentials + update UI on GOT_IP */
void wifi_popup_connection_failed(void); /* Clear pending credentials + update UI on failure */
void wifi_popup_scan_done(void);        /* Notify popup that async scan results are ready */
void wifi_icon_set_connecting(void);    /* Start blinking white WiFi icon (connecting state) */
void wifi_popup_refresh_list(void);     /* Re-populate scan list (e.g. after forgetting a network) */
// @file FilMachine.c
void stopMotorTask(void);
void runMotorTask(void);
// @file accessories.c
void rebootBoard(void);
void applyScreenOffTimeout(uint8_t mins);          /* dim chain from screenOffMins */
const char *screenOffLabelFor(uint8_t mins);       /* '5 min'/'10 min'/'30 min'/Never */
#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char *buf);   /* LVGL log → console + SD log */
#endif
uint8_t qSysAction( uint16_t msg );
uint8_t qMotorAction( uint16_t msg );
lv_obj_t *create_radiobutton(lv_obj_t * mBoxParent, const char * txt, const int32_t x, const int32_t y, const int32_t size, const lv_font_t * font, const lv_color_t borderColor, const lv_color_t bgColor);
lv_obj_t *create_text(lv_obj_t * parent, const char * icon, const char * txt);
lv_obj_t *create_slider(lv_obj_t * parent, const char * icon, const char * txt, int32_t min, int32_t max,int32_t val);
lv_obj_t *create_switch(lv_obj_t * parent, const char * icon, const char * txt, bool chk);
void *isNodeInList(void* list, void* node, NodeType_t type);
void *allocateAndInitializeNode(NodeType_t type);
void event_cb(lv_event_t * e);
void event_checkbox_handler(lv_event_t * e);
void event_keyboard(lv_event_t* e);
void kb_ctx_set(const sKeyboardOwnerContext *ctx);
void createQuestionMark(lv_obj_t * parent,lv_obj_t * element,lv_event_cb_t e, const int32_t x, const int32_t y);
void createPopupBackdrop(lv_obj_t **parent, lv_obj_t **container, int32_t width, int32_t height);
void initTitleLineStyle(lv_style_t *style, uint32_t color);
void createMessageBox(lv_obj_t *messageBox, char *title, char *text, char *button1Text, char *button2Text);
void create_keyboard();
void showKeyboard(lv_obj_t * whoCallMe, lv_obj_t * textArea);
void hideKeyboard(lv_obj_t * whoCallMe);
char *createRollerValues( uint32_t minVal, uint32_t maxVal, const char* extra_str, bool isFahrenheit );
uint8_t SD_init(void);
void init_Pins_and_Buses(void);
void initMCP23017Pins();
void calculateTotalTimeData(processNode *processNode);  /* Thread-safe: data only, no LVGL */
void calculateTotalTime(processNode *processNode);      /* LVGL thread only: data + label update */
uint8_t calculatePercentage(uint32_t minutes, uint8_t seconds, uint32_t total_minutes, uint8_t total_seconds);
int32_t convertCelsiusToFahrenheit(int32_t tempC);
void updateProcessElement(processNode *process);
void updateStepElement(processNode *referenceProcess, stepNode *step);
uint32_t loadSDCardProcesses();
char *generateRandomCharArray(uint8_t length);
void hbridge_safe_init(void);
void initializeRelayPins();
void sendValueToRelay(uint8_t pumpFrom, uint8_t pumpDir, bool activePump);
void initializeMotorPins();
void initializeChemLevelPins(void);              /* Port B: 6 chem sensors + 2 heaters */
bool chemLevelMinDetected(uint8_t container);    /* container 0..2 (C1..C3), true=water */
bool chemLevelMaxDetected(uint8_t container);
void stopMotor(uint8_t pin1, uint8_t pin2);
void runMotorFW(uint8_t pin1, uint8_t pin2);
void runMotorRV(uint8_t pin1, uint8_t pin2);
void setMotorSpeed(uint8_t pin,uint8_t spd);
void setMotorSpeedUp(uint8_t pin, uint8_t spd);
void setMotorSpeedDown(uint8_t pin, uint8_t spd);
void enableMotor(uint8_t pin);
void testPin(uint8_t pin);
//float getTemperature(DeviceAddress sensor);
void initializeTemperatureSensor();
void printTemperature(float temp);
/* Temperature control — simulator stubs / hardware wrappers */
float sim_getTemperature(uint8_t sensorIndex);
void  sim_setHeater(bool on);
void  sim_resetTemperatures(void);
//char* printAddressSensor(DeviceAddress deviceAddress);
void writeMachineStats(machineStatistics *machineStats);
void readMachineStats(machineStatistics *machineStats);
uint32_t findRollerStringIndex(const char *input, const char *list);
char *getRollerStringIndex(uint32_t index, const char *list);
char *generateRandomSuffix(const char* baseName);
sStepDetail *deepCopyStepDetail(sStepDetail *original);
bool single_step_clone(const singleStep *src, singleStep *dst);
stepNode *deepCopyStepNode(stepNode *original);
stepList deepCopyStepList(stepList original);
sCheckup *deepCopyCheckup(sCheckup *original);
sProcessDetail *deepCopyProcessDetail(sProcessDetail *original);
bool single_process_clone(const singleProcess *src, singleProcess *dst);
struct processNode *deepCopyProcessNode(struct processNode *original);
void toLowerCase(char *str);
uint8_t caseInsensitiveStrstr(const char *haystack, const char *needle);
void filterAndDisplayProcesses( void );
void removeFiltersAndDisplayAllProcesses( void );
void step_detail_destroy(sStepDetail *sd);
void checkup_destroy(sCheckup *ckup);
void process_detail_destroy(sProcessDetail *pd);
void step_node_destroy(stepNode *node);
void process_node_destroy(processNode *node);
void emptyList(void *list, NodeType_t type);
char *ftoa(char *a, float f, uint8_t precisione);
uint8_t getValueForChemicalSource(uint8_t source);
void getMinutesAndSeconds(uint8_t containerFillingTime, const bool containerToClean[3]);
void cleanRelayManager(uint8_t pumpFrom, uint8_t pumpTo,uint8_t pumpDir,bool activePump);
void setValveState(uint8_t relayPin, bool open);
void closeAllValves(void);

/* ── Peripheral diagnostics page (2-finger gesture on the splash) ── */
void    debugScreenCreate(void);            /* build + show the diagnostics screen */
void    debugMcpWrite(uint8_t pin, bool on);/* direct MCP output write (heaters etc.) */
bool    debugMcpPresent(void);              /* MCP23017 detected on the I2C bus */
uint8_t ui_active_touch_points(void);       /* simultaneous touch points (board only) */
void valveSelfTest(void);
void sensorsSelfTestInit(void);
void motor_set_forward(uint8_t duty);
void motor_set_reverse(uint8_t duty);
void motor_set_stop(void);
void motor_start_kicked(bool forward, uint8_t target_duty);
void pump_set_forward(uint8_t duty);
void pump_set_reverse(uint8_t duty);
void pump_set_stop(void);
uint8_t getRandomRotationInterval();
uint8_t mapPercentageToValue(uint8_t percentage, uint8_t minPercent, uint8_t maxPercent);
void pwmLedTest();
void readConfigFile(const char *path, bool enableLog);
void readSettingsOnly(const char *path);  /* Read just machineSettings — for splash boot */
void writeConfigFile( const char *path, bool enableLog );
bool copyAndRenameFile( const char* sourceFile, const char* destFile );
uint16_t calculateFillTime(uint16_t capacityMl, uint8_t pumpSpeedPercent);
uint16_t getContainerFillTime(void);
uint16_t getWbFillTime(void);
/* Self-calibration: store the real measured fill time (seconds) once a fill
 * completes on the MIN/MAX sensors, so bargraphs use the true time next run. */
void     recordFillCalibration(bool isWb, uint16_t secs);

/* Buzzer / alarm API */
void buzzer_beep(void);            /* play a short beep (hardware or simulator) */
void alarm_start_persistent(void); /* start repeating beep every 10 seconds */
void alarm_stop(void);             /* stop repeating beep */
bool alarm_is_active(void);        /* check if persistent alarm is running */

#endif /* MAIN_FILMACHINE_H_ */
