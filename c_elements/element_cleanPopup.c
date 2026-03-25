/**
 * @file element_cleanPopup.c
 *
 */


//ESSENTIAL INCLUDES
#include "FilMachine.h"


extern struct gui_components gui;

//ACCESSORY INCLUDES

static uint8_t processPercentage = 0;
static uint8_t cyclePercentage = 0;
static int16_t stepPercentage = 0;  /* int16 to allow negative check during bidirectional pump cycle */

static uint32_t minutesProcessElapsed = 0;
static uint8_t secondsProcessElapsed = 1;
static uint8_t hoursProcessElapsed = 0;

static uint32_t minutesCycleElapsed = 0;
static uint8_t secondsCycleElapsed = 1;

static uint32_t minutesStepElapsed = 0;
static uint8_t secondsStepElapsed = 1;

static uint32_t minutesProcessLeft = 0;
static uint8_t secondsProcessLeft = 0;

static uint32_t minutesCycleLeft = 0;
static uint8_t secondsCycleLeft = 0;

static uint32_t minutesStepLeft = 0;
static uint8_t secondsStepLeft = 0;

static uint8_t firstContainerIndex = 0;
static bool containerSelected = false;

static uint8_t containerIndex = 0;

static uint8_t cycleMins = 0;
static uint8_t cycleSecs = 0;

static uint8_t stepMins = 0;
static uint8_t stepSecs = 0;
static uint8_t currentCycle = 1;

static uint8_t wasteSecs = 0;

static uint8_t previousStepDirection = 0;

static bool isWasting = false; 

static void resetStuffBeforeNextProcess(){
    alarm_stop();
    minutesProcessElapsed = 0;
    secondsProcessElapsed = 1;
    hoursProcessElapsed = 0;

    minutesCycleElapsed = 0;
    secondsCycleElapsed = 1;

    minutesStepElapsed = 0;
    secondsStepElapsed = 1;



    minutesProcessLeft = 0;
    secondsProcessLeft = 0;

    minutesCycleLeft = 0;
    secondsCycleLeft = 0;

    minutesStepLeft = 0;
    secondsStepLeft = 0;

    stepPercentage = 0;
    processPercentage = 0;
    cyclePercentage = 0;

    containerIndex = 0;
    currentCycle = 1;
    
    previousStepDirection = 0;

    wasteSecs = 0;

    isWasting = false; 

    gui.element.cleanPopup.stepDirection = 1;
    gui.element.cleanPopup.stopNowPressed = false;
    gui.element.cleanPopup.isAlreadyPumping = false;
    gui.element.cleanPopup.isCleaning = false;
    
    lv_obj_clear_state(gui.element.cleanPopup.cleanStopButton, LV_STATE_DISABLED);
    lv_label_set_text(gui.element.cleanPopup.cleanNowStepLabelValue,cleanFilling_text);             

    lv_obj_clear_flag(gui.element.cleanPopup.cleanRemainingTimeValue, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_clear_flag(gui.element.cleanPopup.cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningLabel, cleanCurrentClean_text);
    
    lv_label_set_text(gui.element.cleanPopup.cleanStopButtonLabel, cleanStopButton_text);
    
    lv_obj_clear_flag(gui.element.cleanPopup.cleanNowStepLabelValue, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(gui.element.cleanPopup.cleanNowStepLabelValue,cleanFilling_text);             

    lv_arc_set_value(gui.element.cleanPopup.cleanProcessArc, 0);
    lv_arc_set_value(gui.element.cleanPopup.cleanCycleArc, 0);
    lv_arc_set_value(gui.element.cleanPopup.cleanPumpArc, 0);

    cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
}

void cleanWasteTimer(lv_timer_t * timer) {
    /* Increment seconds for step */
    secondsStepElapsed++;
    if (secondsStepElapsed >= 60) {
        secondsStepElapsed = 0;
        minutesStepElapsed++;
        if (minutesStepElapsed >= 60) {
            minutesStepElapsed = 0;
        }
    }

    /* Calculate remaining minutes and seconds */
    uint16_t wbFillTime = getWbFillTime();
    stepMins = wbFillTime / 60;
    stepSecs = wbFillTime % 60;

    uint32_t elapsedStepSecs = minutesStepElapsed * 60 + secondsStepElapsed;
    uint32_t remainingStepSecs = wbFillTime - elapsedStepSecs;
    uint8_t remainingStepMins = remainingStepSecs / 60;
    uint8_t remainingStepSecsOnly = remainingStepSecs % 60;

    /* Calculate step percentage */
    stepPercentage = calculatePercentage(minutesStepElapsed, secondsStepElapsed, stepMins, stepSecs);

    /* Update labels and arcs */
    lv_label_set_text_fmt(gui.element.cleanPopup.cleanRemainingTimeValue, "%dm%ds", remainingStepMins, remainingStepSecsOnly);
    lv_label_set_text(gui.element.cleanPopup.cleanNowStepLabelValue, cleanDraining_text);
    lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningLabel, cleanWaste_text);
    lv_obj_add_flag(gui.element.cleanPopup.cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_state(gui.element.cleanPopup.cleanStopButton, LV_STATE_DISABLED);
    lv_obj_clear_flag(gui.element.cleanPopup.cleanNowStepLabelValue, LV_OBJ_FLAG_HIDDEN);
    lv_arc_set_value(gui.element.cleanPopup.cleanPumpArc, stepPercentage);

    /* Run cleanRelayManager only once at the start */
    if (!isWasting) {
        cleanRelayManager(getValueForChemicalSource(WB), getValueForChemicalSource(WASTE), PUMP_IN_RLY, true);
        isWasting = true;
        LV_LOG_USER("Initial execution of cleanRelayManager done");
    }

    /* Check if time has expired */
    if (elapsedStepSecs >= wbFillTime) {
        lv_arc_set_value(gui.element.cleanPopup.cleanPumpArc, stepPercentage);

        lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanStopButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_label_set_text(gui.element.cleanPopup.cleanStopButtonLabel, cleanCloseButton_text);
        lv_obj_clear_state(gui.element.cleanPopup.cleanStopButton, LV_STATE_DISABLED);
        lv_obj_add_flag(gui.element.cleanPopup.cleanNowStepLabelValue, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(gui.element.cleanPopup.cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningValue, cleanCompleteClean_text);

        /* Start persistent alarm for clean completion */
        alarm_start_persistent();

        cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
        LV_LOG_USER("Execution of cleanRelayManager after WB_FILLING_TIME done");

        // Cancella il timer
        lv_timer_del(gui.element.cleanPopup.wasteTimer);
        gui.element.cleanPopup.wasteTimer = NULL;
        LV_LOG_USER("cleanWasteTimer stopped");
    }
}




void cleanPumpTimer(lv_timer_t * timer) {

    char *tmp_processSourceList[] = processSourceList;
    previousStepDirection = 0; /* Remember previous step direction */

    LV_LOG_USER("cleanPumpTimer running");
    gui.element.cleanPopup.isCleaning = true;
    /* Variables for calculating remaining process time */
    uint8_t totalProcessSecs = gui.element.cleanPopup.totalMins * 60 + gui.element.cleanPopup.totalSecs;
    uint8_t elapsedProcessSecs = minutesProcessElapsed * 60 + secondsProcessElapsed;
    uint8_t remainingProcessSecs = totalProcessSecs - elapsedProcessSecs;
    uint8_t remainingProcessMins = remainingProcessSecs / 60;
    uint8_t remainingProcessSecsOnly = remainingProcessSecs % 60;
    
    // Verifica se è stato premuto il bottone STOP
    if (gui.element.cleanPopup.stopNowPressed) {
        secondsStepElapsed--; 
        // Decrementa l'arco più interno (cleanPumpArc) fino a 0
        if (stepPercentage > 0) {
            stepPercentage = calculatePercentage(minutesStepElapsed, secondsStepElapsed, stepMins, stepSecs);
            lv_arc_set_value(gui.element.cleanPopup.cleanPumpArc, 100 - stepPercentage);
            lv_arc_set_value(gui.element.cleanPopup.cleanPumpArc, stepPercentage);
            LV_LOG_USER("Decrementing Step Percentage: %d", stepPercentage);
            
            /* Update label for step */
            lv_label_set_text_fmt(gui.element.cleanPopup.cleanNowStepLabelValue, cleanDraining_text);
        } else {
            /* When stepPercentage is 0, stop timer and update final state */
            lv_obj_clear_state(gui.element.cleanPopup.cleanStopButton, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanStopButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
            lv_label_set_text(gui.element.cleanPopup.cleanStopButtonLabel, cleanCloseButton_text);

            /* Stop timer and save state */
            lv_timer_del(gui.element.cleanPopup.pumpTimer);
            gui.element.cleanPopup.pumpTimer = NULL;
            cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
            gui.page.tools.machineStats.clean++;
            qSysAction(SAVE_MACHINE_STATS);

            /* Start persistent alarm after stop-now clean draining is complete */
            alarm_start_persistent();
            return;
        }
    } else {
        /* If STOP was not pressed, continue with normal increment */
        /* Increment seconds for step, cycle, and process */
        secondsStepElapsed++;
        secondsCycleElapsed++;
        secondsProcessElapsed++;

        /* Update minutes and seconds */
        if (secondsStepElapsed >= 60) {
            secondsStepElapsed = 0;
            minutesStepElapsed++;
            if (minutesStepElapsed >= 60) {
                minutesStepElapsed = 0;
            }
        }

        if (secondsCycleElapsed >= 60) {
            secondsCycleElapsed = 0;
            minutesCycleElapsed++;
            if (minutesCycleElapsed >= 60) {
                minutesCycleElapsed = 0;
            }
        }

        if (secondsProcessElapsed >= 60) {
            secondsProcessElapsed = 0;
            minutesProcessElapsed++;
            if (minutesProcessElapsed >= 60) {
                minutesProcessElapsed = 0;
                hoursProcessElapsed++;
                if (hoursProcessElapsed >= 12) {
                    hoursProcessElapsed = 0;
                }
            }
        }

        /* Calculate percentages */
        uint16_t containerFillTime = getContainerFillTime();
        stepMins = containerFillTime / 60;
        stepSecs = containerFillTime % 60;
        stepPercentage = calculatePercentage(minutesStepElapsed, secondsStepElapsed, stepMins, stepSecs);

        cycleMins = ((containerFillTime * 2) * gui.element.cleanPopup.cleanCycles) / 60;
        cycleSecs = ((containerFillTime * 2) * gui.element.cleanPopup.cleanCycles) % 60;
        cyclePercentage = calculatePercentage(minutesCycleElapsed, secondsCycleElapsed, cycleMins, cycleSecs);

        processPercentage = calculatePercentage(minutesProcessElapsed, secondsProcessElapsed, gui.element.cleanPopup.totalMins, gui.element.cleanPopup.totalSecs);

        /* Update arcs */
        lv_arc_set_value(gui.element.cleanPopup.cleanPumpArc, (gui.element.cleanPopup.stepDirection == 1) ? stepPercentage : 100 - stepPercentage);
        lv_arc_set_value(gui.element.cleanPopup.cleanCycleArc, cyclePercentage);
        lv_arc_set_value(gui.element.cleanPopup.cleanProcessArc, processPercentage);

        /* Debug log */
        LV_LOG_USER("Step Percentage: %d", stepPercentage);
        LV_LOG_USER("Cycle Percentage: %d", cyclePercentage);
        LV_LOG_USER("Process Percentage: %d", processPercentage);

        lv_label_set_text_fmt(gui.element.cleanPopup.cleanNowCleaningValue, "%s cycle:%d", tmp_processSourceList[containerIndex], currentCycle);

        /* Progress check */
        if (processPercentage < 100) {
            /* Remaining process time */
            lv_label_set_text_fmt(gui.element.cleanPopup.cleanRemainingTimeValue, "%dm%ds", remainingProcessMins, remainingProcessSecsOnly);

            if (stepPercentage >= 100) {
                if (gui.element.cleanPopup.stepDirection == 1) {
                    /* Move to draining phase */
                    gui.element.cleanPopup.stepDirection = -1;
                } else {
                    /* Complete cycle and move to next */
                    gui.element.cleanPopup.stepDirection = 1; /* Reset for filling */

                    if (++currentCycle > gui.element.cleanPopup.cleanCycles) {
                        currentCycle = 1;
                        if (++containerIndex >= (sizeof(tmp_processSourceList) / sizeof(char*)) ) {
                            containerIndex = 0;
                            /* Process completed */
                            lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningValue, cleanCompleteClean_text);
                            lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanStopButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
                            lv_label_set_text(gui.element.cleanPopup.cleanStopButtonLabel, cleanCloseButton_text);

                            /* Start persistent alarm for clean completion */
                            alarm_start_persistent();

                            /* Stop timer and save state */
                            lv_timer_del(gui.element.cleanPopup.pumpTimer);
                            gui.element.cleanPopup.pumpTimer = NULL;

                            gui.page.tools.machineStats.clean++;
                            qSysAction(SAVE_MACHINE_STATS);
                            return;
                        }
                    }
                }
                /* Restart timer for next step */
                minutesStepElapsed = 0;
                secondsStepElapsed = 0;
                
                if(stepPercentage == 100)
                    gui.element.cleanPopup.isAlreadyPumping = false;
            } else {
                /* Update step */
                stepPercentage += gui.element.cleanPopup.stepDirection;
                if (stepPercentage < 0) {
                    stepPercentage = 0;
                    gui.element.cleanPopup.stepDirection = 1; /* Move to filling phase */
                } else if (stepPercentage > 100) {
                    stepPercentage = 100;
                    gui.element.cleanPopup.stepDirection = -1; /* Move to draining phase */
                }
            }

            /* Update cycle timer */
            if (cyclePercentage == 100) {
                cyclePercentage = 0;
                minutesCycleElapsed = 0;
                secondsCycleElapsed = 0;
            }

            /* Check if step direction changed */
            if (gui.element.cleanPopup.stepDirection != previousStepDirection) {
                /* Execute function only when direction changes */
                LV_LOG_USER("sendValueToRelay containerIndex %d",containerIndex);
                if(gui.element.cleanPopup.isAlreadyPumping == false){
                  gui.element.cleanPopup.isAlreadyPumping = true;
                  if( gui.element.cleanPopup.stepDirection == 1 ) {
                      cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
                      cleanRelayManager(getValueForChemicalSource(WB), getValueForChemicalSource(containerIndex), PUMP_IN_RLY, true);
                  } else {
                      cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
                      cleanRelayManager(getValueForChemicalSource(containerIndex), getValueForChemicalSource(WB), PUMP_OUT_RLY, true);
                  }
                }
                previousStepDirection = gui.element.cleanPopup.stepDirection; /* Update previous direction */
            }

            lv_label_set_text(gui.element.cleanPopup.cleanNowStepLabelValue, (gui.element.cleanPopup.stepDirection == 1) ? cleanFilling_text : cleanDraining_text);
        } else {
            /* Process completed */
            lv_label_set_text_fmt(gui.element.cleanPopup.cleanRemainingTimeValue, "%dm%ds", 0, 0);
            lv_obj_add_flag(gui.element.cleanPopup.cleanNowStepLabelValue, LV_OBJ_FLAG_HIDDEN);

            cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
            /* Stop timer and save state */
            lv_timer_del(gui.element.cleanPopup.pumpTimer);
            gui.element.cleanPopup.pumpTimer = NULL;

            gui.page.tools.machineStats.clean++;
            qSysAction(SAVE_MACHINE_STATS);

            if(gui.element.cleanPopup.cleanDrainWater){
              secondsStepElapsed = 0;
              minutesStepElapsed = 0;
              gui.element.cleanPopup.wasteTimer = lv_timer_create(cleanWasteTimer, 1000,  NULL);
            }
            else{
                lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningValue, cleanCompleteClean_text);
                lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanStopButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
                lv_label_set_text(gui.element.cleanPopup.cleanStopButtonLabel, cleanCloseButton_text);

                /* Start persistent alarm for clean completion */
                alarm_start_persistent();
            }
        }
    }
}




void event_cleanPopup(lv_event_t * e) {

	char *tmp_processSourceList[] = processSourceList;

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
  if(code == LV_EVENT_SHORT_CLICKED){
    if(obj == gui.element.cleanPopup.cleanSpinBoxPlusButton){
      lv_spinbox_increment(gui.element.cleanPopup.cleanSpinBox);
      gui.element.cleanPopup.cleanCycles = lv_spinbox_get_value(gui.element.cleanPopup.cleanSpinBox);
      LV_LOG_USER("Cycles programmed :%d",gui.element.cleanPopup.cleanCycles);
    }
    if(obj == gui.element.cleanPopup.cleanSpinBoxMinusButton){
      lv_spinbox_decrement(gui.element.cleanPopup.cleanSpinBox);
      gui.element.cleanPopup.cleanCycles = lv_spinbox_get_value(gui.element.cleanPopup.cleanSpinBox);
      LV_LOG_USER("Cycles programmed :%d",gui.element.cleanPopup.cleanCycles);
    }
  }
  if(code == LV_EVENT_RELEASED){
    if(obj == gui.element.cleanPopup.cleanRunButton){
      if (containerSelected){
            resetStuffBeforeNextProcess();

            lv_obj_add_flag(gui.element.cleanPopup.cleanSettingsContainer, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(gui.element.cleanPopup.cleanRunButton, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(gui.element.cleanPopup.cleanCancelButton, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(gui.element.cleanPopup.cleanProcessContainer, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(gui.element.cleanPopup.cleanTitle, cleanCleanProcess_text);
            lv_obj_remove_flag(gui.element.cleanPopup.cleanRemainingTimeValue, LV_OBJ_FLAG_HIDDEN); 

            lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanStopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
            lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningLabel, cleanCurrentClean_text);

            getMinutesAndSeconds(getContainerFillTime(), gui.element.cleanPopup.containerToClean);
            lv_label_set_text_fmt(gui.element.cleanPopup.cleanRemainingTimeValue, "%"PRIu32"m%"PRIu32"s",gui.element.cleanPopup.totalMins, gui.element.cleanPopup.totalSecs); 
            LV_LOG_USER("Process totalMin: %"PRIu32" totalSecs: %"PRIu32"",gui.element.cleanPopup.totalMins,gui.element.cleanPopup.totalSecs);
            
            lv_obj_remove_flag(gui.element.cleanPopup.cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_fmt(gui.element.cleanPopup.cleanNowCleaningValue, "%s cycle:%d",tmp_processSourceList[firstContainerIndex],currentCycle);

            gui.element.cleanPopup.pumpTimer = lv_timer_create(cleanPumpTimer, 1000,  NULL);
            LV_LOG_USER("Started pumpTimer");

      }

    }
    if(obj == gui.element.cleanPopup.cleanCancelButton){
      alarm_stop();
      lv_obj_add_flag(gui.element.cleanPopup.cleanPopupParent, LV_OBJ_FLAG_HIDDEN);
    }

	if(obj == gui.element.cleanPopup.cleanStopButton) {
		if(gui.element.cleanPopup.stopNowPressed == false) {
	    	if(processPercentage != 100) {
	       		LV_LOG_USER("Stopped processTimer");
		      //lv_timer_delete(gui.element.cleanPopup.pumpTimer);
		      gui.element.cleanPopup.stopNowPressed = true;
		      alarm_stop();
		      lv_label_set_text(gui.element.cleanPopup.cleanTitle, cleanCanceled_text);
		      lv_obj_add_state(gui.element.cleanPopup.cleanStopButton, LV_STATE_DISABLED);
		      lv_obj_add_flag(gui.element.cleanPopup.cleanRemainingTimeValue, LV_OBJ_FLAG_HIDDEN);
		      lv_obj_add_flag(gui.element.cleanPopup.cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
		      lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningLabel, cleanCanceled_text);
		      return;
			} else {
				alarm_stop();
				lv_obj_add_flag(gui.element.cleanPopup.cleanProcessContainer, LV_OBJ_FLAG_HIDDEN);
				lv_obj_remove_flag(gui.element.cleanPopup.cleanSettingsContainer, LV_OBJ_FLAG_HIDDEN);
				lv_obj_remove_flag(gui.element.cleanPopup.cleanRunButton, LV_OBJ_FLAG_HIDDEN);
				lv_obj_remove_flag(gui.element.cleanPopup.cleanCancelButton, LV_OBJ_FLAG_HIDDEN);
				lv_label_set_text(gui.element.cleanPopup.cleanTitle, cleanPopupTitle_text);
			}
		} else {
			alarm_stop();
			lv_obj_add_flag(gui.element.cleanPopup.cleanRemainingTimeValue, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(gui.element.cleanPopup.cleanProcessContainer, LV_OBJ_FLAG_HIDDEN);
			lv_obj_remove_flag(gui.element.cleanPopup.cleanSettingsContainer, LV_OBJ_FLAG_HIDDEN);
			lv_obj_remove_flag(gui.element.cleanPopup.cleanRunButton, LV_OBJ_FLAG_HIDDEN);
			lv_obj_remove_flag(gui.element.cleanPopup.cleanCancelButton, LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text(gui.element.cleanPopup.cleanTitle, cleanPopupTitle_text);
			lv_obj_clear_state(gui.element.cleanPopup.cleanStopButton, LV_STATE_DISABLED);
			lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanStopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
			lv_label_set_text(gui.element.cleanPopup.cleanStopButtonLabel, cleanStopButton_text);
		}
	}    
}
  
if(obj == gui.element.cleanPopup.cleanDrainWaterSwitch){
	if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("State cleanDrainWaterSwitch: %s", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
		gui.element.cleanPopup.cleanDrainWater = lv_obj_has_state(obj, LV_STATE_CHECKED);  
	}
}


if(obj == gui.element.cleanPopup.cleanSelectC1CheckBox || obj == gui.element.cleanPopup.cleanSelectC2CheckBox || obj == gui.element.cleanPopup.cleanSelectC3CheckBox){
    if(code == LV_EVENT_VALUE_CHANGED) {
        if(obj == gui.element.cleanPopup.cleanSelectC1CheckBox){
          LV_LOG_USER("State C1: %d", lv_obj_has_state(obj, LV_STATE_CHECKED));
          gui.element.cleanPopup.containerToClean[0] = lv_obj_has_state(obj, LV_STATE_CHECKED);
          }
        if(obj == gui.element.cleanPopup.cleanSelectC2CheckBox){
          LV_LOG_USER("State C2: %d", lv_obj_has_state(obj, LV_STATE_CHECKED));
          gui.element.cleanPopup.containerToClean[1] = lv_obj_has_state(obj, LV_STATE_CHECKED);
          }
        if(obj == gui.element.cleanPopup.cleanSelectC3CheckBox){
          LV_LOG_USER("State C3: %d", lv_obj_has_state(obj, LV_STATE_CHECKED));
          gui.element.cleanPopup.containerToClean[2] = lv_obj_has_state(obj, LV_STATE_CHECKED);
          }
        for (uint8_t i = 0; i < ( sizeof(tmp_processSourceList) / sizeof(char*) ); ++i) {
          if (gui.element.cleanPopup.containerToClean[i]) {
              lv_obj_clear_state(gui.element.cleanPopup.cleanRunButton, LV_STATE_DISABLED);
              containerSelected = true;
              firstContainerIndex = i;
              LV_LOG_USER("firstContainerIndex :%d",firstContainerIndex);
              break;
          }else {
            lv_obj_add_state(gui.element.cleanPopup.cleanRunButton, LV_STATE_DISABLED);
            containerSelected = false;
          }
        }

      }
    } 
}


void cleanPopup (void){

	char *tmp_processSourceList[] = processSourceList;
  const ui_clean_popup_layout_t *ui = &ui_get_profile()->clean_popup;
  /*********************
   *    PAGE ELEMENTS
   *********************/
  if (gui.element.cleanPopup.cleanPopupParent == NULL)
  {  
      gui.element.cleanPopup.totalMins = 0;
      gui.element.cleanPopup.totalSecs = 0;
      gui.element.cleanPopup.cleanCycles = 1;
      gui.element.cleanPopup.stopNowPressed = false;
      gui.element.cleanPopup.isAlreadyPumping = false;
      gui.element.cleanPopup.cleanDrainWater = false;
      gui.element.cleanPopup.isCleaning = false;

      createPopupBackdrop(&gui.element.cleanPopup.cleanPopupParent, &gui.element.cleanPopup.cleanContainer, ui_get_profile()->popups.clean_w, ui_get_profile()->popups.clean_h); 

              gui.element.cleanPopup.cleanTitle = lv_label_create(gui.element.cleanPopup.cleanContainer);         
              lv_label_set_text(gui.element.cleanPopup.cleanTitle, cleanPopupTitle_text); 
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanTitle, ui->title_font, 0);              
              lv_obj_align(gui.element.cleanPopup.cleanTitle, LV_ALIGN_TOP_MID, 0, ui->title_y);

              /*Create style*/
              initTitleLineStyle(&gui.element.cleanPopup.style_cleanTitleLine, WHITE);

              /*Create a line and apply the new style*/
              gui.element.cleanPopup.cleanPopupTitleLine = lv_line_create(gui.element.cleanPopup.cleanContainer);
              lv_line_set_points(gui.element.cleanPopup.cleanPopupTitleLine, gui.element.cleanPopup.titleLinePoints, 2);
              lv_obj_add_style(gui.element.cleanPopup.cleanPopupTitleLine, &gui.element.cleanPopup.style_cleanTitleLine, 0);
              lv_obj_align(gui.element.cleanPopup.cleanPopupTitleLine, LV_ALIGN_TOP_MID, 0, ui->title_line_y);

              gui.element.cleanPopup.cleanSettingsContainer = lv_obj_create(gui.element.cleanPopup.cleanPopupParent);
              lv_obj_align(gui.element.cleanPopup.cleanSettingsContainer, LV_ALIGN_TOP_MID, 0, ui->settings_y);
              lv_obj_set_size(gui.element.cleanPopup.cleanSettingsContainer, ui_get_profile()->popups.clean_settings_w, ui_get_profile()->popups.clean_settings_h); 
              lv_obj_remove_flag(gui.element.cleanPopup.cleanSettingsContainer, LV_OBJ_FLAG_SCROLLABLE); 
              lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSettingsContainer , LV_OPA_TRANSP, 0);
            
                          gui.element.cleanPopup.cleanSubTitleLabel = lv_label_create(gui.element.cleanPopup.cleanSettingsContainer);         
                          lv_label_set_text(gui.element.cleanPopup.cleanSubTitleLabel, cleanPopupSubTitle_text); 
                          lv_obj_set_style_text_font(gui.element.cleanPopup.cleanSubTitleLabel, ui->subtitle_font, 0);              
                          lv_obj_align(gui.element.cleanPopup.cleanSubTitleLabel, LV_ALIGN_TOP_MID, 0, ui->subtitle_y);
                        
                          
                          gui.element.cleanPopup.cleanChemicalTanksContainer = lv_obj_create(gui.element.cleanPopup.cleanSettingsContainer);
                          lv_obj_remove_flag(gui.element.cleanPopup.cleanChemicalTanksContainer , LV_OBJ_FLAG_SCROLLABLE); 
                          lv_obj_align(gui.element.cleanPopup.cleanChemicalTanksContainer , LV_ALIGN_LEFT_MID, ui->chem_container_x, ui->chem_container_y);
                          lv_obj_set_size(gui.element.cleanPopup.cleanChemicalTanksContainer , ui->chem_container_w, ui->chem_container_h); 
                          lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanChemicalTanksContainer , LV_OPA_TRANSP, 0);


                                  //Container checkboxes
                                  gui.element.cleanPopup.cleanSelectC1CheckBox = lv_obj_create(gui.element.cleanPopup.cleanChemicalTanksContainer);
                                  lv_obj_remove_flag(gui.element.cleanPopup.cleanSelectC1CheckBox, LV_OBJ_FLAG_SCROLLABLE); 
                                  lv_obj_align(gui.element.cleanPopup.cleanSelectC1CheckBox, LV_ALIGN_LEFT_MID, ui->chem_c1_checkbox_x, 0);
                                  lv_obj_set_size(gui.element.cleanPopup.cleanSelectC1CheckBox, ui->checkbox_w, ui->checkbox_h); 
                                  lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSelectC1CheckBox, LV_OPA_TRANSP, 0);

                                        gui.element.cleanPopup.cleanC1CheckBoxLabel = lv_label_create(gui.element.cleanPopup.cleanSelectC1CheckBox);         
                                        lv_label_set_text(gui.element.cleanPopup.cleanC1CheckBoxLabel, tmp_processSourceList[0]); 
                                        lv_obj_set_style_text_font(gui.element.cleanPopup.cleanC1CheckBoxLabel, ui->label_font, 0);              
                                        lv_obj_align(gui.element.cleanPopup.cleanC1CheckBoxLabel, LV_ALIGN_LEFT_MID, ui->checkbox_label_x, 0);

                                        gui.element.cleanPopup.cleanSelectC1CheckBox = create_radiobutton(gui.element.cleanPopup.cleanSelectC1CheckBox, "", 0, 0, ui->checkbox_radio_size, ui->subtitle_font, lv_color_hex(WHITE), lv_palette_main(LV_PALETTE_BLUE));
                                        lv_obj_add_event_cb(gui.element.cleanPopup.cleanSelectC1CheckBox, event_cleanPopup, LV_EVENT_VALUE_CHANGED, gui.element.cleanPopup.cleanSelectC1CheckBox);


                                  gui.element.cleanPopup.cleanSelectC2CheckBox = lv_obj_create(gui.element.cleanPopup.cleanChemicalTanksContainer);
                                  lv_obj_remove_flag(gui.element.cleanPopup.cleanSelectC2CheckBox, LV_OBJ_FLAG_SCROLLABLE); 
                                  lv_obj_align(gui.element.cleanPopup.cleanSelectC2CheckBox, LV_ALIGN_LEFT_MID, ui->chem_c2_checkbox_x, 0);
                                  lv_obj_set_size(gui.element.cleanPopup.cleanSelectC2CheckBox, ui->checkbox_w, ui->checkbox_h); 
                                  lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSelectC2CheckBox, LV_OPA_TRANSP, 0);

                                        gui.element.cleanPopup.cleanC2CheckBoxLabel = lv_label_create(gui.element.cleanPopup.cleanSelectC2CheckBox);         
                                        lv_label_set_text(gui.element.cleanPopup.cleanC2CheckBoxLabel, tmp_processSourceList[1]); 
                                        lv_obj_set_style_text_font(gui.element.cleanPopup.cleanC2CheckBoxLabel, ui->label_font, 0);              
                                        lv_obj_align(gui.element.cleanPopup.cleanC2CheckBoxLabel, LV_ALIGN_LEFT_MID, ui->checkbox_label_x, 0);

                                        gui.element.cleanPopup.cleanSelectC2CheckBox = create_radiobutton(gui.element.cleanPopup.cleanSelectC2CheckBox, "", 0, 0, ui->checkbox_radio_size, ui->subtitle_font, lv_color_hex(WHITE), lv_palette_main(LV_PALETTE_BLUE));
                                        lv_obj_add_event_cb(gui.element.cleanPopup.cleanSelectC2CheckBox, event_cleanPopup, LV_EVENT_VALUE_CHANGED, gui.element.cleanPopup.cleanSelectC2CheckBox);


                                  gui.element.cleanPopup.cleanSelectC3CheckBox = lv_obj_create(gui.element.cleanPopup.cleanChemicalTanksContainer);
                                  lv_obj_remove_flag(gui.element.cleanPopup.cleanSelectC3CheckBox, LV_OBJ_FLAG_SCROLLABLE); 
                                  lv_obj_align(gui.element.cleanPopup.cleanSelectC3CheckBox, LV_ALIGN_LEFT_MID, ui->chem_c3_checkbox_x, 0);
                                  lv_obj_set_size(gui.element.cleanPopup.cleanSelectC3CheckBox, ui->checkbox_w, ui->checkbox_h); 
                                  lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSelectC3CheckBox, LV_OPA_TRANSP, 0);

                                        gui.element.cleanPopup.cleanC3CheckBoxLabel = lv_label_create(gui.element.cleanPopup.cleanSelectC3CheckBox);         
                                        lv_label_set_text(gui.element.cleanPopup.cleanC3CheckBoxLabel, tmp_processSourceList[2]); 
                                        lv_obj_set_style_text_font(gui.element.cleanPopup.cleanC3CheckBoxLabel, ui->label_font, 0);              
                                        lv_obj_align(gui.element.cleanPopup.cleanC3CheckBoxLabel, LV_ALIGN_LEFT_MID, ui->checkbox_label_x, 0);

                                        gui.element.cleanPopup.cleanSelectC3CheckBox = create_radiobutton(gui.element.cleanPopup.cleanSelectC3CheckBox, "", 0, 0, ui->checkbox_radio_size, ui->subtitle_font, lv_color_hex(WHITE), lv_palette_main(LV_PALETTE_BLUE));
                                        lv_obj_add_event_cb(gui.element.cleanPopup.cleanSelectC3CheckBox, event_cleanPopup, LV_EVENT_VALUE_CHANGED, gui.element.cleanPopup.cleanSelectC3CheckBox);  


                          
                          gui.element.cleanPopup.cleanSpinBoxContainer = lv_obj_create(gui.element.cleanPopup.cleanSettingsContainer);
                          lv_obj_remove_flag(gui.element.cleanPopup.cleanSpinBoxContainer, LV_OBJ_FLAG_SCROLLABLE); 
                          lv_obj_align(gui.element.cleanPopup.cleanSpinBoxContainer, LV_ALIGN_CENTER, 0, ui->spinbox_container_y);
                          lv_obj_set_size(gui.element.cleanPopup.cleanSpinBoxContainer, ui->spinbox_container_w, ui->spinbox_container_h); 
                          lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSpinBoxContainer, LV_OPA_TRANSP, 0);


                                gui.element.cleanPopup.cleanSpinBox = lv_spinbox_create(gui.element.cleanPopup.cleanSpinBoxContainer);
                                lv_spinbox_set_range(gui.element.cleanPopup.cleanSpinBox, 1, 5);
                                lv_spinbox_set_digit_format(gui.element.cleanPopup.cleanSpinBox, 1, 0);
                                lv_obj_set_width(gui.element.cleanPopup.cleanSpinBox, ui->spinbox_w);
                                lv_obj_align(gui.element.cleanPopup.cleanSpinBox, LV_ALIGN_LEFT_MID, ui->spinbox_x, 0);
                                lv_obj_set_style_bg_opa(gui.element.cleanPopup.cleanSpinBox, 0, LV_PART_CURSOR);


                                      gui.element.cleanPopup.cleanRepeatTimesLabel = lv_label_create(gui.element.cleanPopup.cleanSpinBoxContainer);         
                                      lv_label_set_text(gui.element.cleanPopup.cleanRepeatTimesLabel, cleanRoller_text); 
                                      lv_obj_set_style_text_font(gui.element.cleanPopup.cleanRepeatTimesLabel, ui->label_font, 0);              
                                      lv_obj_align(gui.element.cleanPopup.cleanRepeatTimesLabel, LV_ALIGN_LEFT_MID, ui->repeat_label_x, 0);

                                      gui.element.cleanPopup.cleanSpinBoxPlusButton = lv_button_create(gui.element.cleanPopup.cleanSpinBoxContainer);
                                      lv_obj_set_size(gui.element.cleanPopup.cleanSpinBoxPlusButton, lv_obj_get_height(gui.element.cleanPopup.cleanSpinBox), lv_obj_get_height(gui.element.cleanPopup.cleanSpinBox));
                                      lv_obj_align_to(gui.element.cleanPopup.cleanSpinBoxPlusButton, gui.element.cleanPopup.cleanSpinBox, LV_ALIGN_OUT_RIGHT_MID, ui_get_profile()->clean_spinbox_btn_offset, 0);
                                      lv_obj_set_style_bg_image_src(gui.element.cleanPopup.cleanSpinBoxPlusButton, LV_SYMBOL_PLUS, 0);
                                      lv_obj_add_event_cb(gui.element.cleanPopup.cleanSpinBoxPlusButton, event_cleanPopup, LV_EVENT_ALL,  NULL);

                                      gui.element.cleanPopup.cleanSpinBoxMinusButton = lv_button_create(gui.element.cleanPopup.cleanSpinBoxContainer);
                                      lv_obj_set_size(gui.element.cleanPopup.cleanSpinBoxMinusButton, lv_obj_get_height(gui.element.cleanPopup.cleanSpinBox), lv_obj_get_height(gui.element.cleanPopup.cleanSpinBox));
                                      lv_obj_align_to(gui.element.cleanPopup.cleanSpinBoxMinusButton, gui.element.cleanPopup.cleanSpinBox, LV_ALIGN_OUT_LEFT_MID, -ui_get_profile()->clean_spinbox_btn_offset, 0);
                                      lv_obj_set_style_bg_image_src(gui.element.cleanPopup.cleanSpinBoxMinusButton, LV_SYMBOL_MINUS, 0);
                                      lv_obj_add_event_cb(gui.element.cleanPopup.cleanSpinBoxMinusButton, event_cleanPopup, LV_EVENT_ALL, NULL);




                      gui.element.cleanPopup.cleanDrainWaterLabelContainer = lv_obj_create(gui.element.cleanPopup.cleanSettingsContainer);
                      lv_obj_remove_flag(gui.element.cleanPopup.cleanDrainWaterLabelContainer , LV_OBJ_FLAG_SCROLLABLE); 
                      lv_obj_align(gui.element.cleanPopup.cleanDrainWaterLabelContainer, LV_ALIGN_CENTER, 0, ui->drain_container_y);
                      lv_obj_set_size(gui.element.cleanPopup.cleanDrainWaterLabelContainer, ui->drain_container_w, ui_get_profile()->clean_drain_container_h);
                      lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanDrainWaterLabelContainer , LV_OPA_TRANSP, 0);


                          gui.element.cleanPopup.cleanDrainWaterLabel = lv_label_create(gui.element.cleanPopup.cleanDrainWaterLabelContainer);         
                          lv_label_set_text(gui.element.cleanPopup.cleanDrainWaterLabel, cleanDrainWater_text); 
                          lv_obj_set_style_text_font(gui.element.cleanPopup.cleanDrainWaterLabel, ui->label_font, 0);              
                          lv_obj_align(gui.element.cleanPopup.cleanDrainWaterLabel, LV_ALIGN_LEFT_MID, ui->checkbox_label_x, 0);

                          gui.element.cleanPopup.cleanDrainWaterSwitch = lv_switch_create(gui.element.cleanPopup.cleanDrainWaterLabelContainer);
                          lv_obj_set_size(gui.element.cleanPopup.cleanDrainWaterSwitch, ui_get_profile()->settings.toggle_switch_w, ui_get_profile()->settings.toggle_switch_h);
                          lv_obj_add_event_cb(gui.element.cleanPopup.cleanDrainWaterSwitch , event_cleanPopup, LV_EVENT_VALUE_CHANGED, gui.element.cleanPopup.cleanDrainWaterSwitch);
                          lv_obj_align(gui.element.cleanPopup.cleanDrainWaterSwitch , LV_ALIGN_LEFT_MID, ui->drain_switch_x + ui->drain_switch_extra_x, 0);
                          lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanDrainWaterSwitch, lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
                          lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanDrainWaterSwitch,  lv_palette_main(LV_PALETTE_BLUE), LV_PART_KNOB | LV_STATE_DEFAULT);
                          lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanDrainWaterSwitch, lv_color_hex(BLUE_DARK) , LV_PART_INDICATOR | LV_STATE_CHECKED);



              
                      gui.element.cleanPopup.cleanRunButton = lv_button_create(gui.element.cleanPopup.cleanContainer);
                      lv_obj_set_size(gui.element.cleanPopup.cleanRunButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
                      lv_obj_align(gui.element.cleanPopup.cleanRunButton, LV_ALIGN_BOTTOM_LEFT, ui->run_button_x , ui->run_button_y);
                      lv_obj_add_event_cb(gui.element.cleanPopup.cleanRunButton, event_cleanPopup, LV_EVENT_RELEASED, NULL);
                      lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanRunButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
                      lv_obj_add_state(gui.element.cleanPopup.cleanRunButton, LV_STATE_DISABLED);


                          gui.element.cleanPopup.cleanCancelButtonLabel = lv_label_create(gui.element.cleanPopup.cleanRunButton);
                          lv_label_set_text(gui.element.cleanPopup.cleanCancelButtonLabel, cleanRunButton_text);
                          lv_obj_set_style_text_font(gui.element.cleanPopup.cleanCancelButtonLabel, ui->button_font, 0);
                          lv_obj_align(gui.element.cleanPopup.cleanCancelButtonLabel, LV_ALIGN_CENTER, 0, 0);


                      gui.element.cleanPopup.cleanCancelButton = lv_button_create(gui.element.cleanPopup.cleanContainer);
                      lv_obj_set_size(gui.element.cleanPopup.cleanCancelButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
                      lv_obj_align(gui.element.cleanPopup.cleanCancelButton, LV_ALIGN_BOTTOM_RIGHT, ui->cancel_button_x , ui->cancel_button_y);
                      lv_obj_add_event_cb(gui.element.cleanPopup.cleanCancelButton, event_cleanPopup, LV_EVENT_RELEASED, NULL);
                      lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanCancelButton, lv_color_hex(RED_DARK), LV_PART_MAIN);

                          gui.element.cleanPopup.cleanCancelButtonLabel = lv_label_create(gui.element.cleanPopup.cleanCancelButton);
                          lv_label_set_text(gui.element.cleanPopup.cleanCancelButtonLabel, cleanCancelButton_text);
                          lv_obj_set_style_text_font(gui.element.cleanPopup.cleanCancelButtonLabel, ui->button_font, 0);
                          lv_obj_align(gui.element.cleanPopup.cleanCancelButtonLabel, LV_ALIGN_CENTER, 0, 0);
        
        

        gui.element.cleanPopup.cleanProcessContainer = lv_obj_create(gui.element.cleanPopup.cleanPopupParent);
        lv_obj_align(gui.element.cleanPopup.cleanProcessContainer, LV_ALIGN_TOP_MID, 0, ui->settings_y);
        lv_obj_set_size(gui.element.cleanPopup.cleanProcessContainer, ui_get_profile()->popups.clean_process_w, ui_get_profile()->popups.clean_process_h);  
        lv_obj_remove_flag(gui.element.cleanPopup.cleanProcessContainer, LV_OBJ_FLAG_SCROLLABLE); 
        lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanProcessContainer , LV_OPA_TRANSP, 0);
        lv_obj_add_flag(gui.element.cleanPopup.cleanProcessContainer, LV_OBJ_FLAG_HIDDEN);



              gui.element.cleanPopup.cleanProcessArc = lv_arc_create(gui.element.cleanPopup.cleanProcessContainer);
              lv_obj_set_size(gui.element.cleanPopup.cleanProcessArc, ui->process_arc_size, ui->process_arc_size);
              lv_arc_set_rotation(gui.element.cleanPopup.cleanProcessArc, 140);
              lv_arc_set_bg_angles(gui.element.cleanPopup.cleanProcessArc, 0, 260);
              lv_arc_set_value(gui.element.cleanPopup.cleanProcessArc, 0);
              lv_arc_set_range(gui.element.cleanPopup.cleanProcessArc, 0, 100);
              lv_obj_align(gui.element.cleanPopup.cleanProcessArc, LV_ALIGN_CENTER, 0, ui->process_arc_y);
              lv_obj_remove_style(gui.element.cleanPopup.cleanProcessArc, NULL, LV_PART_KNOB);
              lv_obj_remove_flag(gui.element.cleanPopup.cleanProcessArc, LV_OBJ_FLAG_CLICKABLE);
              lv_obj_set_style_arc_color(gui.element.cleanPopup.cleanProcessArc,lv_color_hex(LIGHT_BLUE) , LV_PART_INDICATOR);
              lv_obj_set_style_arc_color(gui.element.cleanPopup.cleanProcessArc, lv_color_hex(BLUE_DARK), LV_PART_MAIN);
              if (ui->progress_arc_width > 0) {
                  lv_obj_set_style_arc_width(gui.element.cleanPopup.cleanProcessArc, ui->progress_arc_width, LV_PART_MAIN);
                  lv_obj_set_style_arc_width(gui.element.cleanPopup.cleanProcessArc, ui->progress_arc_width, LV_PART_INDICATOR);
              }


              gui.element.cleanPopup.cleanCycleArc = lv_arc_create(gui.element.cleanPopup.cleanProcessContainer);
              lv_obj_set_size(gui.element.cleanPopup.cleanCycleArc, ui->cycle_arc_size, ui->cycle_arc_size);
              lv_arc_set_rotation(gui.element.cleanPopup.cleanCycleArc, 140);
              lv_arc_set_bg_angles(gui.element.cleanPopup.cleanCycleArc, 0, 260);
              lv_arc_set_value(gui.element.cleanPopup.cleanCycleArc, 0);
              lv_arc_set_range(gui.element.cleanPopup.cleanCycleArc, 0, 100);
              lv_obj_align(gui.element.cleanPopup.cleanCycleArc, LV_ALIGN_CENTER, 0, ui->process_arc_y);
              lv_obj_remove_style(gui.element.cleanPopup.cleanCycleArc, NULL, LV_PART_KNOB);
              lv_obj_remove_flag(gui.element.cleanPopup.cleanCycleArc, LV_OBJ_FLAG_CLICKABLE);
              lv_obj_set_style_arc_color(gui.element.cleanPopup.cleanCycleArc,lv_color_hex(GREEN_LIGHT) , LV_PART_INDICATOR);
              lv_obj_set_style_arc_color(gui.element.cleanPopup.cleanCycleArc, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
              if (ui->progress_arc_width > 0) {
                  lv_obj_set_style_arc_width(gui.element.cleanPopup.cleanCycleArc, ui->progress_arc_width, LV_PART_MAIN);
                  lv_obj_set_style_arc_width(gui.element.cleanPopup.cleanCycleArc, ui->progress_arc_width, LV_PART_INDICATOR);
              }


              gui.element.cleanPopup.cleanPumpArc = lv_arc_create(gui.element.cleanPopup.cleanProcessContainer);
              lv_obj_set_size(gui.element.cleanPopup.cleanPumpArc, ui->pump_arc_size, ui->pump_arc_size);
              lv_arc_set_rotation(gui.element.cleanPopup.cleanPumpArc, 140);
              lv_arc_set_bg_angles(gui.element.cleanPopup.cleanPumpArc, 0, 260);
              lv_arc_set_value(gui.element.cleanPopup.cleanPumpArc, 0);
              lv_arc_set_range(gui.element.cleanPopup.cleanPumpArc, 0, 100);
              lv_obj_align(gui.element.cleanPopup.cleanPumpArc, LV_ALIGN_CENTER, 0, ui->process_arc_y);
              lv_obj_remove_style(gui.element.cleanPopup.cleanPumpArc, NULL, LV_PART_KNOB);
              lv_obj_remove_flag(gui.element.cleanPopup.cleanPumpArc, LV_OBJ_FLAG_CLICKABLE);
              lv_obj_set_style_arc_color(gui.element.cleanPopup.cleanPumpArc,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
              lv_obj_set_style_arc_color(gui.element.cleanPopup.cleanPumpArc, lv_color_hex(ORANGE_DARK), LV_PART_MAIN);
              if (ui->progress_arc_width > 0) {
                  lv_obj_set_style_arc_width(gui.element.cleanPopup.cleanPumpArc, ui->progress_arc_width, LV_PART_MAIN);
                  lv_obj_set_style_arc_width(gui.element.cleanPopup.cleanPumpArc, ui->progress_arc_width, LV_PART_INDICATOR);
              }

              gui.element.cleanPopup.cleanRemainingTimeValue = lv_label_create(gui.element.cleanPopup.cleanProcessContainer);         
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanRemainingTimeValue, ui->time_font, 0);              
              lv_obj_align(gui.element.cleanPopup.cleanRemainingTimeValue, LV_ALIGN_CENTER, 0, ui->remaining_time_y);

              gui.element.cleanPopup.cleanNowCleaningLabel = lv_label_create(gui.element.cleanPopup.cleanProcessContainer);         
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanNowCleaningLabel, ui->value_font, 0);              
              lv_obj_align(gui.element.cleanPopup.cleanNowCleaningLabel, LV_ALIGN_CENTER, 0, ui->now_cleaning_label_y);
              lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningLabel, cleanCurrentClean_text); 

              gui.element.cleanPopup.cleanNowCleaningValue = lv_label_create(gui.element.cleanPopup.cleanProcessContainer);          
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanNowCleaningValue, ui->value_font, 0);              
              lv_obj_align(gui.element.cleanPopup.cleanNowCleaningValue, LV_ALIGN_CENTER, 0, ui->now_cleaning_value_y);


              gui.element.cleanPopup.cleanNowStepLabelValue = lv_label_create(gui.element.cleanPopup.cleanProcessContainer);          
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanNowStepLabelValue, ui->step_font, 0); 
              //lv_label_set_text(gui.element.cleanPopup.cleanNowStepLabelValue,cleanFilling_text);             
              lv_obj_align(gui.element.cleanPopup.cleanNowStepLabelValue, LV_ALIGN_CENTER, 0, ui->now_step_y);


              gui.element.cleanPopup.cleanStopButton = lv_button_create(gui.element.cleanPopup.cleanProcessContainer);
              lv_obj_set_size(gui.element.cleanPopup.cleanStopButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
              lv_obj_align(gui.element.cleanPopup.cleanStopButton, LV_ALIGN_BOTTOM_MID, 0 , ui->stop_button_y);
              lv_obj_add_event_cb(gui.element.cleanPopup.cleanStopButton, event_cleanPopup, LV_EVENT_RELEASED, NULL);
              lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanStopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);

              gui.element.cleanPopup.cleanStopButtonLabel = lv_label_create(gui.element.cleanPopup.cleanStopButton);
              lv_label_set_text(gui.element.cleanPopup.cleanStopButtonLabel, cleanStopButton_text);
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanStopButtonLabel, ui->button_font, 0);
              lv_obj_align(gui.element.cleanPopup.cleanStopButtonLabel, LV_ALIGN_CENTER, 0, 0);

  }
}

