/*
 * FilMachine.h
 *
 *  Created on: May 1, 2025
 *      Author: PeterB
 */

#ifndef MAIN_FILMACHINE_H_
#define MAIN_FILMACHINE_H_

#include "freertos/FreeRTOS.h"
#include "lvgl.h"

/* ═══════════════════════════════════════════════
 * Board-specific hardware definitions
 * All pin assignments, resolution, display/touch driver selection,
 * and sensor availability come from the active board header.
 * Select board at compile time: -DBOARD_MAKERFABS_S3, -DBOARD_JC4827W543, -DBOARD_JC4880P433, or -DBOARD_SIMULATOR
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
#define MOTOR_MIN_ANALOG_VAL		150
#define MOTOR_MAX_ANALOG_VAL		255

/* PCA9685 I2C PWM controller (same on all boards) */
#define PCA9685_ADDR				0x40
#define PCA9685_PWM_FREQ			1000

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

/* System defines */
#define FILENAME_SAVE				"/FilMachine.cfg"
#define FILENAME_BACKUP				"/FilMachine_Backup.cfg"
#define MAX_STEP_ELEMENTS			30
#define MAX_PROC_ELEMENTS			50
#define CONTAINER_FILLING_TIME		10		//Need to be calibrated
#define WB_FILLING_TIME				30		//Need to be calibrated
#define ENABLE_BOOT_ERRORS			0		//Set to 1 to enable all errors (SD/I2C)
#define INIT_ERROR_SD				1
#define INIT_ERROR_WIRE				2
#define INIT_ERROR_I2C_MCP			3
#define INIT_ERROR_I2C_ADS			4
#define initSDError_text			"INITIALIZATION ERROR!\nSD-CARD FAILURE!\nFIX SD-CARD!\nTHEN CLICK ON ICON TO REBOOT!"
#define initI2CError_text			"INITIALIZATION ERROR!\nI2C MODULE FAILURE\nFIX IT!\nTHEN CLICK ON ICON TO REBOOT!"
#define initWIREError_text			"INITIALIZATION ERROR!\nINIT WIRE FAILURE\nFIX IT!\nTHEN CLICK ON ICON TO REBOOT!"

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
#define STEP_HEIGHT					(ui_get_profile()->step_element.item_h)
#define STEP_Y_START				(ui_get_profile()->step_element.y_start)
#define PROCESS_ELEMENT_HEIGHT		(ui_get_profile()->process_element.item_h)
#define PROCESS_Y_START				(ui_get_profile()->process_element.y_start)
#define POPUP_WIDTH					(ui_get_profile()->popups.message_w)
#define POPUP_HEIGHT				(ui_get_profile()->popups.message_h)

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
#define trash_icon					"\xEF\x8B\xAD"
#define chemical_icon				"\xEF\x83\x83"
#define rinse_icon					"\xEF\x81\x83"
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
#define buttonFilter_text 				 			"Filters"
#define Processes_text 					 			"Processes"
#define keyboard_placeholder_text 		 			"Write film name..."
#define filterPopupTitle_text 			 			"Process list filter"
#define filterPopupNamePlaceHolder_text  			"Part of name to filter"
#define filterPopupName_text 			 			"Name"
#define filterPopupColor_text 			 			"Color"
#define filterPopupBnW_text 			 			"B&W"
#define filterPopupBoth_text 			 			"Both"
#define filterPopupPreferred_text 		 			"Preferred"
#define filterPopupApplyButton_text 	 			"Apply"
#define filterPopupResetButton_text 	 			"Reset"

/*********************
* Config tab strings
*********************/
#define Settings_text 								"Machine settings"
#define tempUnit_text 								"Temp. unit:"
#define tempSensorTuning_text 						"Calib. temp. sensor"
#define tuneButton_text 							"TUNE"
#define tempAlertMBox_text							"Ensure stable machine temperature, measure ambient air, input the value, and press 'Tune'. For resetting, long-press 'Tune'."
#define soundAlertMBox_text 						"Continue to sound the alert when a process is about to end or when the heating phase reaches the desired temperature."
#define autostartAlertMBox_text 					"When the desired temperature is reached, automatically initiate the temperature-controlled process."
#define filmRotationSpeedAlertMBox_text 			"Stated as rpm, with a range of 20 to 90 revolutions per minute."
#define rotationInverseIntervalAlertMBox_text 		"The duration, which may be adjusted between 5 and 60 seconds,during which the film will spin in one direction before moving to the other."
#define filmRotationRandomAlertMBox_text 			"Introduce random variation to inversion times for consistent development, e.g.,10% randomization on a 10-second inversion yields times between 8 and 10 seconds."
#define drainFillTimeAlertMBox_text 				"Adjust the overlap of tank filling and draining times with processing times to affect the total processing time, as these actions cannot be rushed."
#define multiRinseTimeAlertMBox_text 				"Sets the duration of a single multi-rinse wash cycle (not the entire step). Adjustable from 30 seconds to 3 minutes. Longer cycles are better for larger tanks, shorter for smaller ones. 2 minutes is a good default."
#define waterInletAlertMBox_text 					"Tells the machine whether it's connected to a pressurized water source. If yes, the water bath will be automatically refilled. If no, you must manually fill the water bath."
#define rotationSpeed_text 							"Rotation speed"
#define rotationInversionInterval_text 				"Rotation inv. interval"
#define rotationRandom_text 						"Randomness"
#define celsius_text 								"°C"
#define fahrenheit_text 							"°F"
#define waterInlet_text 							"Water inlet link"
#define persistentAlarm_text 						"Persistent alarms"
#define autostart_text 								"Process autostart"
#define drainFillTime_text 							"Drain/fill time overlap"
#define multiRinseTime_text 						"Multi rinse cycle time"
#define tankSize_text                       "Tank size"
#define tankSizeAlertMBox_text              "Select the default tank size.\nSmall, Medium, or Large."
#define tankSizeSmall_text                  "S"
#define tankSizeMedium_text                 "M"
#define tankSizeLarge_text                  "L"
#define pumpSpeed_text                      "Pump speed"
#define pumpSpeedAlertMBox_text             "Set the pump speed percentage.\nHigher values = faster fill/drain."
#define chemContainerMl_text                "Container size"
#define chemContainerMlAlertMBox_text       "Set the chemistry container\nsize in milliliters."
#define wbContainerMl_text                  "Water bath size"
#define wbContainerMlAlertMBox_text         "Set the water bath size\nin milliliters."
#define chemistryVolume_text                "Chemistry size"
#define chemistryVolumeAlertMBox_text       "Low: uses half the chemistry.\nHigh: fills the tank completely."
#define chemistryVolumeList                 "Low\nHigh"
#define chemContainerMlList                 "250\n500\n750\n1000\n1250\n1500"
#define wbContainerMlList                   "1000\n1500\n2000\n2500\n3000\n3500\n4000\n5000"

/* Checkup strings/vars */
#define checkupNexStepsTitle_text 					"Next steps:"
#define checkupProcessReady_text 					"Process starting..."
#define checkupTheMachineWillDo_text 				"The machine will:"
#define checkupFillWater_text 						"Fill the water bath"
#define checkupTankRotation_text 					"Check tank presence and film rotation"
#define checkupReachTemp_text 						"Reach the chemistry target temperature"
#define checkupStop_text 							"Stop"
#define checkupStart_text 							"Start"
#define checkupSkip_text 							"Skip"
#define checkupStopNow_text 						"Stop now!"
#define checkupStopAfter_text 						"Stop after!"
#define checkupProcessingTitle_text 				"Processing:"
#define checkupStepSource_text 						"Step source:"
#define checkupTempControl_text 					"Temp. control:"
#define checkupWaterTemp_text 						"Water temp:"
#define checkupNextStep_text						"Next step:"
#define checkupSelectBeforeStart_text 				"Select the tank size and chemistry amount and click the 'Start' button"
#define checkupTankSize_text 						"Selected tank size"
#define checkupChemistryVolume_text 				"Selected volume"
#define checkupMinimumChemistry_text 				"Minimum required chemistry : 500ml"
#define checkupFillWaterMachine_text 				"The machine is not connected to a water source.\n\nFill the water bath manually up to the top water level sensor"
#define checkupTargetTemp_text 						"Target temperature"
#define checkupWater_text 							"Water"
#define checkupChemistry_text 						"Chemistry"
#define checkupTankPosition_text 					"Tank is in position:"
#define checkupFilmRotation_text 					"Film is rotating:"
#define checkupYes_text 							"Yes"
#define checkupNo_text 								"No"
#define checkupChecking_text 						"Checking..."
#define checkupTargetToleranceTemp_text 			"tolerance"
#define checkupProcessComplete_text 				"Process\nCOMPLETE!"
#define checkupProcessStopped_text 					"Process\nSTOPPED!"
#define checkupTankSizePlaceHolder_text				"Size"
#define checkupChemistryLowVol_text					"Low"
#define checkupChemistryHighVol_text				"High"
#define checkupFilling_text							"Filling"
#define checkupDraining_text						"Draining"
#define checkupProcessing_text						"Processing"
#define checkupDrainingComplete_text				"Complete"
#define checkupHeaterStatusFmt_text					"Heater: %s"
#define checkupHeaterOn_text						"ON"
#define checkupHeaterOff_text						"OFF"
#define checkupTempReached_text						"Temp ok!"
#define checkupTempTimedOut_text						"Timeout!"
#define checkupContinue_text						"Continue"
#define checkupNoTempControl_text					"No temp control"

/* Clean Popup elements */
#define cleanPopupTitle_text						"Cleaning process setup"
#define cleanCleanProcess_text						"Cleaning machine"
#define cleanPopupSubTitle_text						"Select containers to clean"
#define cleanRoller_text							"Clean cycles"
#define cleanDrainWater_text						"Drain water when finish"
#define cleanCancelButton_text						"Cancel"
#define cleanCanceled_text							"Canceled"
#define cleanRunButton_text							"Run"
#define cleanStopButton_text						"Stop"
#define cleanCloseButton_text						"Close"
#define cleanCurrentClean_text						"Cleaning"
#define cleanCompleteClean_text						"COMPLETE"
#define cleanWaste_text								"Waste"
#define cleanDraining_text							"Draining"
#define cleanFilling_text							"Filling"

/* Drain popup texts */
#define drainStopped_text                       "Drain stopped"
#define drainComplete_text                      "Drain complete!"
#define drainWasteIndicator_text                ">> WASTE <<"

/* Self-check popup texts */
#define selfCheck_text                          "Self-check"
#define selfCheckTasks_text                     "Tasks:"
#define selfCheckTempSensors_text               "Temp. sensors"
#define selfCheckWaterPump_text                 "Water pump"
#define selfCheckHeater_text                    "Heater"
#define selfCheckValves_text                    "Valves"
#define selfCheckContainer1_text                "Container C1"
#define selfCheckContainer2_text                "Container C2"
#define selfCheckContainer3_text                "Container C3"
#define selfCheckRunning_text                   "Running..."
#define selfCheckDone_text                      "Done"
#define selfCheckComplete_text                  "Self-check complete!"
#define selfCheckFinished_text                  "Self-check finished"
#define selfCheckSkip_text                      "Skip"
#define selfCheckNext_text                      "Next"

/* Common button texts */
#define buttonClose_text                        "Close"
#define buttonStop_text                         "Stop"
#define buttonStart_text                        "Start"
#define buttonCancel_text                       "Cancel"

/* Tank size display values */
#define tankSizeValues                          {"500ml", "700ml", "1000ml"}

/* Chemistry volume display values */
#define chemVolumeValues                        {"Low", "High"}

/* Popup elements */
#define stopProcessPopupTitle_text 					"Stop process"
#define warningPopupTitle_text 						"Warning!"
#define setMinutesPopupTitle_text 					"Set minutes"
#define setSecondsPopupTitle_text 					"Set seconds"
#define tuneTempPopupTitle_text 					"Set temperature"
#define tuneTolerancePopupTitle_text 				"Set tolerance"
#define tuneRollerButton_text 						"SET"
#define messagePopupDetailTitle_text 				"Detail information"
#define deleteButton_text 							"Delete"
#define deletePopupTitle_text 						"Delete element"
#define duplicatePopupTitle_text 					"Duplicate process"
#define duplicateProcessPopupBody_text 				"Do you want do duplicate the selected process?"
#define duplicateStepPopupTitle_text				"Duplicate step"
#define duplicateStepPopupBody_text					"Do you want to duplicate the selected step?"
#define deleteAllProcessPopupTitle_text 			"Delete all process"
#define deletePopupBody_text 						"Are you sure to delete the selected element?"
#define deleteAllProcessPopupBody_text 				"Are you sure to delete all the process created?"
#define warningPopupLowWaterTitle_text 		   		"The water level is too low!Temperature control has been suspended\nRefill the water bath immediately to correct resume the temperature control process"
#define stopNowProcessPopupBody_text			   	"Stopping a process will ruin the film inside the tank and leave the chemistry inside!\nDo you want to stop the process now?"
#define stopAfterProcessPopupBody_text			   	"Do you want to stop the process after this step is completed?"
#define maxNumberEntryProcessPopupBody_text			"Max number of PROCESS already reached!"
#define maxNumberEntryStepsPopupBody_text			"Max number of STEP already reached!"

/* Tools tab strings/vars */
#define Maintenance_text 							"Maintenance"
#define Utilities_text 								"Utilities"
#define Statistics_text 							"Statistics"
#define Software_text 								"Software"
#define cleanMachine_text 							"Clean machine"
#define drainMachine_text 							"Drain machine"
#define importConfigAndProcesses_text 				"Import"
#define importConfigAndProcessesMBox_text 			"Import configuration and processes from micro SD Card"
#define importConfigAndProcessesMBox2_text			"This will reboot your FilMachine!Are you sure?!"
#define exportConfigAndProcesses_text 				"Export"
#define exportConfigAndProcessesMBox_text 			"Export configuration and processes to micro SD Card"
#define statCompleteProcesses_text 					"Completed processes"
#define statTotalProcessTime_text 					"Total process time"
#define statCompleteCleanProcess_text 				"Completed clean cycles"
#define statStoppedProcess_text 					"Stopped processes"
#define softwareVersion_text 						"Software version"
#define softwareVersionValue_text    				"v0.0.0.1"
#define softwareSerialNum_text 						"Serial number"
#define softwareSerialNumValue_text 				"1234567890"
#define softwareCredits_text 						"Credits"
#define softwareCreditsValue_text 					"Credit to Frank P. \nand \nPete B."
#define calibrationPopupTitle_text					"Calibration"
#define calibrationResetPopupTitle_text				"Calibration Reset"
#define calibrationResetPopupBody_text				"Temperature calibration has been reset to default values."

/* OTA update strings */
#define otaUpdate_text								"Update"
#define otaUpdateFromSD_text						"Update from SD"
#define otaUpdateFromSDMBox_text					"Update firmware from\nSD card file."
#define otaWifiUpdate_text							"Wi-Fi update"
#define otaWifiUpdateMBox_text						"Start a local web server.\nUpload firmware via browser."
#define otaUpdating_text							"Updating..."
#define otaNoFirmware_text							"No firmware found on SD"
#define otaConfirmUpdate_text						"Update firmware to %s?\nDo not turn off the machine!"
#define otaRebootNow_text							"Reboot now to apply update?"
#define otaWifiSSID_text							"Wi-Fi SSID"
#define otaWifiSSIDAlert_text						"Enter the Wi-Fi network\nname for OTA updates."
#define otaWifiPassword_text						"Wi-Fi password"
#define otaWifiPasswordAlert_text					"Enter the Wi-Fi network\npassword."

/* Process detail strings/vars */
#define processDetailStep_text				 		"Steps"
#define processDetailInfo_text				 		"Details"
#define processDetailIsColor_text			 		"For color negative"
#define processDetailIsBnW_text				 		"For b&w negative"
#define processDetailIsTempControl_text		 		"Temp. control"
#define processDetailTemp_text				 		"Temperature:"
#define processDetailIsPreferred_text		 		"Preferred:"
#define processDetailTotalTime_text			 		"Total time:"
#define processDetailTempPlaceHolder_text	 		"Tap"
#define processDetailTempTolerance_text		 		"Tolerance:"
#define processDetailPlaceHolder_text		   		"Enter name"

/* Step detail strings/vars */
#define stepDetailTitle_text						"New step"
#define stepDetailName_text							"Name:"
#define stepDetailDuration_text				   		"Duration:             :"
#define stepDetailDurationMinPlaceHolder_text 		"min"
#define stepDetailDurationSecPlaceHolder_text 		"sec"
#define stepDetailType_text							"Type:"
#define stepDetailSource_text						"Source:"
#define stepDetailDiscardAfter_text			   		"Discard after:"
#define stepDetailPlaceHolder_text			   		"Enter name for new step"
#define stepDetailSave_text							"Save"
#define stepDetailCancel_text						"Cancel"
#define stepDetailCurrentTemp_text			   		"Now:"

/* Button sizes — resolved from ui_profile at runtime */
#define BUTTON_PROCESS_HEIGHT						(ui_get_profile()->buttons.process_h)
#define BUTTON_PROCESS_WIDTH						(ui_get_profile()->buttons.process_w)
#define BUTTON_START_HEIGHT							(ui_get_profile()->buttons.start_h)
#define BUTTON_START_WIDTH							(ui_get_profile()->buttons.start_w)
#define BUTTON_MBOX_HEIGHT							(ui_get_profile()->buttons.mbox_h)
#define BUTTON_MBOX_WIDTH							(ui_get_profile()->buttons.mbox_w)
#define BUTTON_POPUP_CLOSE_HEIGHT					(ui_get_profile()->buttons.popup_close_h)
#define BUTTON_POPUP_CLOSE_WIDTH					(ui_get_profile()->buttons.popup_close_w)
#define BUTTON_TUNE_HEIGHT							(ui_get_profile()->buttons.tune_h)
#define BUTTON_TUNE_WIDTH							(ui_get_profile()->buttons.tune_w)
#define LOGO_HEIGHT									(ui_get_profile()->buttons.logo_h)
#define LOGO_WIDTH									(ui_get_profile()->buttons.logo_w)

#define checkupTankSizesList						"500ml\n700ml\n1000ml"
#define checkupStepStatuses 						{ dotStep_icon, arrowStep_icon, checkStep_icon }
#define stepTypeList								"Chemistry\nRinse\nMultiRinse"
#define stepSourceList								"C1\nC2\nC3\nWB"
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
	uint8_t					calibratedTemp;
	uint8_t					filmRotationSpeedSetpoint;
	uint8_t					rotationIntervalSetpoint;
	uint8_t					randomSetpoint;
	bool					isPersistentAlarm;
	bool					isProcessAutostart;
	uint8_t					drainFillOverlapSetpoint;
	uint8_t					multiRinseTime;
	uint8_t					tankSize;       // 1=Small, 2=Medium, 3=Large
	uint8_t					pumpSpeed;          // 10-100% pump speed
	uint16_t				chemContainerMl;    // Chemistry container capacity in ml (250-2000)
	uint16_t				wbContainerMl;      // Water bath capacity in ml (1000-5000)
	uint8_t					chemistryVolume;    // 1=Low, 2=High
	int8_t					tempCalibOffset;    /* Calibration offset in tenths of degree (e.g., -15 = -1.5°C) */
};



typedef enum kbOwnerType { KB_OWNER_NONE, KB_OWNER_FILTER, KB_OWNER_PROCESS, KB_OWNER_STEP } kbOwnerType;

typedef struct sKeyboardOwnerContext {
    kbOwnerType         owner;
    lv_obj_t           *textArea;
    lv_obj_t           *parentScreen;
    lv_obj_t           *saveButton;
    void               *ownerData;
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
    lv_coord_t         		container_y;
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

	lv_obj_t			*processTempTextArea;
	lv_obj_t			*processToleranceTextArea;

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
    lv_coord_t        	container_y;
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
	lv_obj_t			*phaseIcon[7];
	lv_obj_t			*phaseNameLabel[7];

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
	char				 otaPin[6]; /* 5 digits + null */
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
* HOME PAGE STRUCT
*********************/
struct sHome {
    /* LVGL objects */
	lv_obj_t *screen_home;
  lv_obj_t *startButton;
  lv_obj_t *splashImage;
  lv_obj_t *errorButtonLabel;
  lv_obj_t *errorLabel;
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
	lv_coord_t 			    pad;

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

	lv_obj_t 	        	*drainFillTimeValueLabel;
  lv_obj_t 	        	*multiRinseTimeValueLabel;
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

	lv_obj_t 	        	*autostartSwitch;
	lv_obj_t 	        	*persistentAlarmSwitch;
	lv_obj_t 	        	*waterInletSwitch;

	lv_obj_t 	        	*filmRotationSpeedSlider;
	lv_obj_t 	        	*filmRotationInversionIntervalSlider;
	lv_obj_t 	        	*filmRandomSlider;
	lv_obj_t 	        	*drainFillTimeSlider;
  lv_obj_t 	        	*multiRinseTimeSlider;

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
	lv_obj_t                *pumpSpeedValueLabel;

	lv_obj_t                *chemContainerMlContainer;
	lv_obj_t                *chemContainerMlLabel;
	lv_obj_t                *chemContainerMlTextArea;

	lv_obj_t                *wbContainerMlContainer;
	lv_obj_t                *wbContainerMlLabel;
	lv_obj_t                *wbContainerMlTextArea;

	lv_obj_t                *chemVolumeContainer;
	lv_obj_t                *chemVolumeLabel;
	lv_obj_t                *chemVolumeTextArea;

	/* OTA / Wi-Fi settings */
	/* OTA / Wi-Fi settings */
	lv_obj_t                *otaSectionLabel;
	lv_obj_t                *otaWifiSSIDContainer;
	lv_obj_t                *otaWifiSSIDLabel;
	lv_obj_t                *otaWifiSSIDTextArea;
	lv_obj_t                *otaWifiPasswordContainer;
	lv_obj_t                *otaWifiPasswordLabel;
	lv_obj_t                *otaWifiPasswordTextArea;
	char                     otaWifiSSID[33];
	char                     otaWifiPassword[65];

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
	lv_obj_t 	        	*toolsImportContainer;
	lv_obj_t 	        	*toolsExportContainer;


	lv_obj_t 	        	*toolsCleaningLabel;
	lv_obj_t 	        	*toolsDrainingLabel;
	lv_obj_t 	        	*toolsSelfcheckLabel;
	lv_obj_t 	        	*toolsImportLabel;
	lv_obj_t 	        	*toolsExportLabel;

	lv_obj_t 	        	*toolsCleaningButton;
  lv_obj_t 	        	*toolsCleaningButtonLabel;
	lv_obj_t 	        	*toolsDrainingButton;
  lv_obj_t 	        	*toolsDrainingButtonLabel;
	lv_obj_t 	        	*toolsSelfcheckButton;
  lv_obj_t 	        	*toolsSelfcheckButtonLabel;
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
  struct sKeyboardPopup   keyboardPopup;
};


/*********************
* PAGES STRUCT
*********************/
struct sPages {
	struct sHome				  home;
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
    lv_coord_t original_width;
    lv_coord_t original_height;
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

/* Our Fonts */
LV_FONT_DECLARE(FilMachineFontIcons_15);
LV_FONT_DECLARE(FilMachineFontIcons_20);
LV_FONT_DECLARE(FilMachineFontIcons_30);
LV_FONT_DECLARE(FilMachineFontIcons_40);
LV_FONT_DECLARE(FilMachineFontIcons_100);

/* Our Images */
LV_IMG_DECLARE(splash_img);

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
// @file element_cleanPopup.c
void event_cleanPopup(lv_event_t *e);
void cleanPopup(void);
// @file element_drainPopup.c
void event_drainPopup(lv_event_t *e);
void drainPopupCreate(void);
// @file element_selfcheckPopup.c
void event_selfcheckPopup(lv_event_t *e);
void selfcheckPopupCreate(void);
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
// @file page_home.c
void event_btn_start(lv_event_t *e);
void homePage(void);
// @file page_menu.c
void event_tab_switch(lv_event_t *e);
void menu(void);
// @file page_processDetail.c
void event_processDetail(lv_event_t *e);
void event_processDetail_style_delete(lv_event_t *e);
void processDetail(lv_obj_t *referenceProcess);
// @file page_processes.c
void event_processes_style_delete(lv_event_t *e);
void event_tabProcesses(lv_event_t *e);
void processes(void);
// @file page_settings.c
void event_settings_style_delete(lv_event_t *e);
void event_settingPopupMBox(lv_event_t *e);
void event_settings_handler(lv_event_t *e);
void initSettings(void);
void settings(void);
void refreshSettingsUI(void);
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
// @file FilMachine.c
void stopMotorTask(void);
void runMotorTask(void);
// @file accessories.c
void rebootBoard(void);
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
void calculateTotalTime(processNode *processNode);
uint8_t calculatePercentage(uint32_t minutes, uint8_t seconds, uint32_t total_minutes, uint8_t total_seconds);
int32_t convertCelsiusToFahrenheit(int32_t tempC);
void updateProcessElement(processNode *process);
void updateStepElement(processNode *referenceProcess, stepNode *step);
uint32_t loadSDCardProcesses();
char *generateRandomCharArray(uint8_t length);
void initializeRelayPins();
void sendValueToRelay(uint8_t pumpFrom, uint8_t pumpDir, bool activePump);
void initializeMotorPins();
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
uint8_t getRandomRotationInterval();
uint8_t mapPercentageToValue(uint8_t percentage, uint8_t minPercent, uint8_t maxPercent);
void pwmLedTest();
void readConfigFile(const char *path, bool enableLog);
void writeConfigFile( const char *path, bool enableLog );
bool copyAndRenameFile( const char* sourceFile, const char* destFile );
uint16_t calculateFillTime(uint16_t capacityMl, uint8_t pumpSpeedPercent);
uint16_t getContainerFillTime(void);
uint16_t getWbFillTime(void);

/* Buzzer / alarm API */
void buzzer_beep(void);            /* play a short beep (hardware or simulator) */
void alarm_start_persistent(void); /* start repeating beep every 10 seconds */
void alarm_stop(void);             /* stop repeating beep */
bool alarm_is_active(void);        /* check if persistent alarm is running */

#endif /* MAIN_FILMACHINE_H_ */
