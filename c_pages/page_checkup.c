/**
 * @file page_checkup.c
 *
 */


//ESSENTIAL INCLUDES
#include "FilMachine.h"


extern struct gui_components gui;

static uint8_t isProcessingStatus0created = 0;
static uint8_t isProcessingStatus1created = 0;
static uint8_t isStepStatus0created = 0;
static uint8_t isStepStatus1created = 0;
static uint8_t isStepStatus2created = 0;
static uint8_t isStepStatus3created = 0;
static uint8_t isStepStatus4created = 0;

static uint32_t minutesProcessElapsed = 0;
static uint8_t secondsProcessElapsed = 1;
static uint8_t hoursProcessElapsed = 0;

static uint32_t minutesStepElapsed = 0;
static uint8_t secondsStepElapsed = 1;

static uint32_t minutesProcessLeft = 0;
static uint8_t secondsProcessLeft = 0;
static uint32_t minutesStepLeft = 0;
static uint8_t secondsStepLeft = 0;

static uint8_t stepPercentage = 0;
static uint8_t processPercentage = 0;

static uint8_t tankFillTime = 0;
static uint8_t tankTimeElapsed = 0;
static uint8_t tankPercentage = 0;

/* static bool isPumping = false; */  /* Currently unused - reserved for future pump state tracking */
static uint8_t pumpFrom = 0;
static uint8_t pumpDir = 0;

static void resetStuffBeforeNextProcess(){
    isProcessingStatus0created = 0;
    isProcessingStatus1created = 0;
    isStepStatus0created = 0;
    isStepStatus1created = 0;
    isStepStatus2created = 0;
    isStepStatus3created = 0;
    isStepStatus4created = 0;

    minutesProcessElapsed = 0;
    secondsProcessElapsed = 1;
    hoursProcessElapsed = 0;
    minutesStepElapsed = 0;
    secondsStepElapsed = 1;
    minutesProcessLeft = 0;
    secondsProcessLeft = 0;
    minutesStepLeft = 0;
    secondsStepLeft = 0;
    stepPercentage = 0;
    processPercentage = 0;

    tankFillTime = 0;
    tankTimeElapsed = 0;
    tankPercentage = 0;

    sim_resetTemperatures();
}

void event_checkup(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *widget = lv_event_get_current_target(e);
  processNode *pn = (processNode *)lv_event_get_user_data(e);

  if(pn == NULL || pn->process.processDetails == NULL
      || pn->process.processDetails->checkup == NULL) return;
  sCheckup *ckup = pn->process.processDetails->checkup;

  /* ── Radio buttons: target may differ from current_target (event bubbles) ── */
  if(code == LV_EVENT_CLICKED && widget == ckup->checkupSelectTankChemistryContainer){
      lv_obj_t *act_cb = (lv_obj_t *)lv_event_get_target(e);
      if(act_cb == ckup->lowVolumeChemRadioButton || act_cb == ckup->highVolumeChemRadioButton){
          lv_obj_t *old_cb = lv_obj_get_child(widget, ckup->data.activeVolume_index);
          lv_obj_remove_state(old_cb, LV_STATE_CHECKED);
          lv_obj_add_state(act_cb, LV_STATE_CHECKED);
          ckup->data.activeVolume_index = lv_obj_get_index(act_cb);
          LV_LOG_USER("Selected chemistry volume: %d", (int)ckup->data.activeVolume_index);
      }
  }

  /* ── Tank size textarea events ── */
  if(code == LV_EVENT_FOCUSED) {
      if(widget == ckup->checkupTankSizeTextArea){
          if(gui.element.rollerPopup.mBoxRollerParent != NULL) return;
          LV_LOG_USER("Set Tank Size");
          rollerPopupCreate(checkupTankSizesList,checkupTankSize_text,widget,ckup->data.tankSize - 1);
      }
  }
  if(code == LV_EVENT_VALUE_CHANGED) {
      if(widget == ckup->checkupTankSizeTextArea){
          LV_LOG_USER("Set New Tank Size %d", ckup->data.tankSize);
      }
  }

  /* ── Button clicks: use processStep to disambiguate reused widgets ── */
  if(code == LV_EVENT_CLICKED){
    if(widget == ckup->checkupStartButton){
      if(ckup->data.processStep == 0){
        LV_LOG_USER("User pressed checkupStartButton on Step 0");
        ckup->data.isProcessing = 0;
        ckup->data.processStep = 1;
        ckup->data.stepFillWaterStatus = 1;
        ckup->data.stepReachTempStatus = 0;
        ckup->data.stepCheckFilmStatus = 0;
        checkup(pn);
      }
      else if(ckup->data.processStep == 3){
        LV_LOG_USER("User pressed checkupStartButton on Step 3");
        ckup->data.isProcessing = 1;
        ckup->data.processStep = 4;
        ckup->data.stepFillWaterStatus = 2;
        ckup->data.stepReachTempStatus = 2;
        ckup->data.stepCheckFilmStatus = 2;
        checkup(pn);
      }
    }
    if(widget == ckup->checkupSkipButton){
      if(ckup->data.processStep == 1){
        LV_LOG_USER("User pressed checkupSkipButton on Step 1");
        ckup->data.isProcessing = 0;
        ckup->data.processStep = 2;
        ckup->data.stepFillWaterStatus = 2;
        ckup->data.stepReachTempStatus = 1;
        ckup->data.stepCheckFilmStatus = 0;
        checkup(pn);
      }
      else if(ckup->data.processStep == 2){
        LV_LOG_USER("User pressed checkupSkipButton on Step 2");
        safeTimerDelete(&ckup->tempTimer);
        sim_setHeater(false);
        ckup->data.heaterOn = false;

        ckup->data.isProcessing = 0;
        ckup->data.processStep = 3;
        ckup->data.stepFillWaterStatus = 2;
        ckup->data.stepReachTempStatus = 2;
        ckup->data.stepCheckFilmStatus = 1;
        checkup(pn);
      }
    }

    if(widget == ckup->checkupCloseButton){
        LV_LOG_USER("User pressed checkupCloseButton");

        /* FIX #19: Do NOT call lv_msgbox_close — checkupParent is a regular
         * screen created with lv_obj_create(NULL), NOT an lv_msgbox.
         * Calling lv_msgbox_close on it causes a BUS ERROR (crash). */

        /* Safety: delete any timers that might still be running */
        safeTimerDelete(&ckup->processTimer);
        safeTimerDelete(&ckup->pumpTimer);
        safeTimerDelete(&ckup->tempTimer);
        /* Safety: always turn off the heater when leaving checkup */
        sim_setHeater(false);

        /* Reset static state for next process run */
        resetStuffBeforeNextProcess();

        /* Clean up the style */
        lv_style_reset(&ckup->textAreaStyleCheckup);

        /* Switch back to the menu screen FIRST, then delete the checkup screen */
        lv_scr_load(gui.page.menu.screen_mainMenu);

        /* Delete the entire checkup screen (checkupParent owns checkupContainer) */
        if(ckup->checkupParent != NULL) {
            lv_obj_delete(ckup->checkupParent);
            ckup->checkupParent = NULL;
            ckup->checkupContainer = NULL;
        }
        return;
    }
    if(widget == ckup->checkupStopAfterButton){
        LV_LOG_USER("User pressed checkupStopAfterButton");
        messagePopupCreate(warningPopupTitle_text,stopAfterProcessPopupBody_text,checkupStop_text,stepDetailCancel_text, ckup->checkupStopAfterButton);
    }
    if(widget == ckup->checkupStopNowButton){
        LV_LOG_USER("User pressed checkupStopNowButton");
        messagePopupCreate(warningPopupTitle_text,stopNowProcessPopupBody_text,checkupStop_text,stepDetailCancel_text, ckup->checkupStopNowButton);
    }

  }

  if(ckup->data.tankSize > 0
    && ckup->data.activeVolume_index > 0 &&
      ckup->data.processStep < 1){
        lv_obj_clear_state(ckup->checkupStartButton, LV_STATE_DISABLED);
  }
}



void processTimer(lv_timer_t * timer) {
	/* Extract context from timer user_data */
	processNode *pn = (processNode *)lv_timer_get_user_data(timer);
	if(pn == NULL || pn->process.processDetails == NULL || pn->process.processDetails->checkup == NULL) return;
	sCheckup *ckup = pn->process.processDetails->checkup;

	char *tmp_processSourceList[] = processSourceList;

    LV_LOG_USER("processTimer running");
    ckup->data.isDeveloping = true;

    if(secondsProcessElapsed >= 60) {
        secondsProcessElapsed = 0;
        minutesProcessElapsed++;
        if(minutesProcessElapsed >= 60) {
            minutesProcessElapsed = 0;
            hoursProcessElapsed++;
            if(hoursProcessElapsed >= 12)
                hoursProcessElapsed = 0;
        }
    }

    if(secondsStepElapsed >= 60) {
        secondsStepElapsed = 0;
        minutesStepElapsed++;
        if(minutesStepElapsed >= 60) {
            minutesStepElapsed = 0;
        }
    }


    processPercentage = calculatePercentage(minutesProcessElapsed, secondsProcessElapsed, pn->process.processDetails->data.timeMins, pn->process.processDetails->data.timeSecs);
    LV_LOG_USER("Elapsed Time %"PRIu8"h:%"PRIu32"m:%"PRIu8"s, processPercentage %"PRIu8" stepPercentage %"PRIu8"", hoursProcessElapsed, minutesProcessElapsed, secondsProcessElapsed, processPercentage, stepPercentage);

    // Convert the remaining process time to minutes and seconds
    uint8_t totalProcessSecs = pn->process.processDetails->data.timeMins * 60 + pn->process.processDetails->data.timeSecs;
    uint8_t elapsedProcessSecs = minutesProcessElapsed * 60 + secondsProcessElapsed;
    uint8_t remainingProcessSecs = totalProcessSecs - elapsedProcessSecs;

    uint8_t remainingProcessMins = remainingProcessSecs / 60;
    uint8_t remainingProcessSecsOnly = remainingProcessSecs % 60;

    // Convert the remaining step time to minutes and seconds
    uint8_t totalStepSecs = ckup->currentStep->step.stepDetails->data.timeMins * 60 + ckup->currentStep->step.stepDetails->data.timeSecs;
    uint8_t elapsedStepSecs = minutesStepElapsed * 60 + secondsStepElapsed;
    uint8_t remainingStepSecs = totalStepSecs - elapsedStepSecs;

    uint8_t remainingStepMins = remainingStepSecs / 60;
    uint8_t remainingStepSecsOnly = remainingStepSecs % 60;

    if(ckup->currentStep != NULL) {
        if(ckup->data.stopAfter == true && remainingStepMins == 0 && remainingStepSecsOnly == 0){
            lv_obj_add_state(ckup->checkupStopAfterButton, LV_STATE_DISABLED);
            lv_obj_add_state(ckup->checkupStopNowButton, LV_STATE_DISABLED);

            lv_label_set_text(ckup->checkupProcessCompleteLabel, checkupProcessStopped_text);
            lv_obj_remove_flag(ckup->checkupProcessCompleteLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ckup->checkupStepNameValue, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ckup->checkupStepTimeLeftValue, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ckup->checkupProcessTimeLeftValue, LV_OBJ_FLAG_HIDDEN);

            lv_arc_set_value(ckup->stepArc, 100);

            lv_timer_resume(ckup->pumpTimer);
        }
        else{
          stepPercentage = calculatePercentage(minutesStepElapsed, secondsStepElapsed, ckup->currentStep->step.stepDetails->data.timeMins, ckup->currentStep->step.stepDetails->data.timeSecs);
          lv_arc_set_value(ckup->stepArc, stepPercentage);
          if(ckup->currentStep->next != NULL)
              lv_label_set_text(ckup->checkupStepSourceValue, tmp_processSourceList[ckup->currentStep->next->step.stepDetails->data.source]);
          else
              lv_label_set_text(ckup->checkupStepSourceValue, tmp_processSourceList[ckup->currentStep->step.stepDetails->data.source]);

          if(pn->process.processDetails->stepElementsList.size == 1)
            lv_label_set_text(ckup->checkupNextStepValue,
              pn->process.processDetails->stepElementsList.start->step.stepDetails->data.stepNameString);
          else
            if(ckup->currentStep->next != NULL)
              lv_label_set_text(ckup->checkupNextStepValue, ckup->currentStep->next->step.stepDetails->data.stepNameString);

          lv_label_set_text(ckup->checkupStepNameValue, ckup->currentStep->step.stepDetails->data.stepNameString);


          lv_label_set_text_fmt(ckup->checkupStepTimeLeftValue, "%dm%ds", remainingStepMins,remainingStepSecsOnly);

          if(stepPercentage == 100) {
              stepPercentage = 0;
              minutesStepElapsed = 0;
              secondsStepElapsed = 0;
              ckup->currentStep = ckup->currentStep->next;
              lv_label_set_text(ckup->checkupStepNameValue, "...");
              lv_arc_set_value(ckup->stepArc, stepPercentage);
              lv_timer_pause(ckup->processTimer);
              lv_timer_resume(ckup->pumpTimer);
          }
        }
    }
    else{
        safeTimerDelete(&ckup->processTimer);
    }

    lv_label_set_text_fmt(ckup->checkupProcessTimeLeftValue, "%dm%ds", remainingProcessMins,
      remainingProcessSecsOnly);

    lv_arc_set_value(ckup->processArc, processPercentage);

    if(processPercentage <= 100) {
        secondsProcessElapsed++;
        secondsStepElapsed++;
        if(processPercentage == 100) {
          ckup->data.isDeveloping = false;

          lv_obj_add_state(ckup->checkupStopAfterButton, LV_STATE_DISABLED);
          lv_obj_add_state(ckup->checkupStopNowButton, LV_STATE_DISABLED);

          lv_label_set_text(ckup->checkupProcessCompleteLabel, checkupProcessComplete_text);
          lv_obj_remove_flag(ckup->checkupProcessCompleteLabel, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ckup->checkupStepNameValue, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ckup->checkupStepTimeLeftValue, LV_OBJ_FLAG_HIDDEN);
          lv_obj_add_flag(ckup->checkupProcessTimeLeftValue, LV_OBJ_FLAG_HIDDEN);

          gui.page.tools.machineStats.completed++;
          gui.page.tools.machineStats.totalMins += pn->process.processDetails->data.timeMins;
          qSysAction( SAVE_MACHINE_STATS );

          safeTimerDelete(&ckup->processTimer);
          lv_timer_resume(ckup->pumpTimer);
        }
    }
}

/* ═══════════════════════════════════════════════
 * Temperature monitoring timer callback (1 second interval)
 * Reads simulated sensors, controls heater, updates UI
 * ═══════════════════════════════════════════════ */
#define TEMP_TIMEOUT_SECONDS 600  /* 10 minutes safety timeout */

void tempTimerCallback(lv_timer_t * timer) {
    /* Extract context from timer user_data */
    processNode *pn = (processNode *)lv_timer_get_user_data(timer);
    if(pn == NULL || pn->process.processDetails == NULL || pn->process.processDetails->checkup == NULL) return;

    sCheckup *ckup = pn->process.processDetails->checkup;
    sProcessDetail *pd = pn->process.processDetails;

    /* Read current temperatures */
    ckup->data.currentWaterTemp = sim_getTemperature(TEMPERATURE_SENSOR_BATH);
    ckup->data.currentChemTemp  = sim_getTemperature(TEMPERATURE_SENSOR_CHEMICAL);

    /* Update UI with live values (use snprintf — LVGL builtin sprintf lacks %f) */
    {
        char buf[16];
        if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP) {
            snprintf(buf, sizeof(buf), "%.1f°C", ckup->data.currentWaterTemp);
            lv_label_set_text(ckup->checkupTargetWaterTempValue, buf);
            snprintf(buf, sizeof(buf), "%.1f°C", ckup->data.currentChemTemp);
            lv_label_set_text(ckup->checkupTargetChemistryTempValue, buf);
        } else {
            snprintf(buf, sizeof(buf), "%.1f°F", ckup->data.currentWaterTemp * 1.8f + 32.0f);
            lv_label_set_text(ckup->checkupTargetWaterTempValue, buf);
            snprintf(buf, sizeof(buf), "%.1f°F", ckup->data.currentChemTemp * 1.8f + 32.0f);
            lv_label_set_text(ckup->checkupTargetChemistryTempValue, buf);
        }
    }

    /* Heater control logic: ON when below (target - tolerance), OFF when at target */
    float targetTemp = (float)pd->data.temp;
    float tolerance  = pd->data.tempTolerance;

    if(ckup->data.currentWaterTemp < targetTemp - tolerance) {
        if(!ckup->data.heaterOn) {
            sim_setHeater(true);
            ckup->data.heaterOn = true;
        }
    } else if(ckup->data.currentWaterTemp >= targetTemp) {
        if(ckup->data.heaterOn) {
            sim_setHeater(false);
            ckup->data.heaterOn = false;
        }
    }

    /* Update heater status label */
    if(ckup->checkupHeaterStatusLabel != NULL)
        lv_label_set_text_fmt(ckup->checkupHeaterStatusLabel, checkupHeaterStatusFmt_text, ckup->data.heaterOn ? checkupHeaterOn_text : checkupHeaterOff_text);

    /* Safety timeout counter (logged to terminal only) */
    ckup->data.tempTimeoutCounter++;

    /* Check if temperature reached (water temp within target ± tolerance) */
    bool tempReached = (ckup->data.currentWaterTemp >= targetTemp - tolerance) &&
                       (ckup->data.currentWaterTemp <= targetTemp + tolerance);

    if(tempReached) {
        LV_LOG_USER("Temperature reached! Water=%.1f Target=%"PRIu32" Tol=%.1f",
            ckup->data.currentWaterTemp, pd->data.temp, pd->data.tempTolerance);

        sim_setHeater(false);
        ckup->data.heaterOn = false;

        safeTimerDelete(&ckup->tempTimer);

        if(ckup->checkupHeaterStatusLabel != NULL)
            lv_label_set_text(ckup->checkupHeaterStatusLabel, checkupTempReached_text);

        /* If autostart is enabled, advance to step 3 automatically */
        if(gui.page.settings.settingsParams.isProcessAutostart) {
            LV_LOG_USER("Autostart: advancing to step 3");
            ckup->data.processStep = 3;
            ckup->data.stepReachTempStatus = 2;
            ckup->data.stepCheckFilmStatus = 1;
            checkup(pn);
        } else {
            /* Show CONTINUE button for manual advance — widen to fit text */
            if(ckup->checkupSkipButton != NULL)
                lv_obj_set_width(ckup->checkupSkipButton, 150);
            if(ckup->checkupSkipButtonLabel != NULL)
                lv_label_set_text(ckup->checkupSkipButtonLabel, checkupContinue_text);
        }
        return;
    }

    /* Safety timeout reached — stop heating, let user skip manually */
    if(ckup->data.tempTimeoutCounter >= TEMP_TIMEOUT_SECONDS) {
        LV_LOG_USER("Temperature timeout reached! Stopping heater.");

        sim_setHeater(false);
        ckup->data.heaterOn = false;

        safeTimerDelete(&ckup->tempTimer);

        if(ckup->checkupHeaterStatusLabel != NULL)
            lv_label_set_text(ckup->checkupHeaterStatusLabel, checkupTempTimedOut_text);
        if(ckup->checkupSkipButtonLabel != NULL)
            lv_label_set_text(ckup->checkupSkipButtonLabel, checkupSkip_text);
    }
}

void handleFirstStep(processNode *pn) {
    sCheckup* checkup = pn->process.processDetails->checkup;
    pumpFrom = getValueForChemicalSource(checkup->currentStep->step.stepDetails->data.source);
    pumpDir = PUMP_IN_RLY;

    LV_LOG_USER("First Step");
    if (tankPercentage < 100) {
        LV_LOG_USER("First step FILLING");

        if (checkup->data.isAlreadyPumping == false) {
            runMotorTask();
            sendValueToRelay(pumpFrom, pumpDir, true);
            checkup->data.isAlreadyPumping = true;
        }

        lv_arc_set_value(checkup->pumpArc, tankPercentage);
        lv_obj_remove_flag(checkup->checkupStepKindValue, LV_OBJ_FLAG_HIDDEN);
        tankTimeElapsed++;
    } else {
        LV_LOG_USER("First step FILLING COMPLETE");

        sendValueToRelay(pumpFrom, pumpDir, false);
        checkup->data.isAlreadyPumping = false;

        lv_arc_set_value(checkup->pumpArc, tankPercentage);
        lv_label_set_text(checkup->checkupStepKindValue, checkupProcessing_text);

        tankPercentage = 0;
        tankTimeElapsed = 0;
        checkup->data.isFilling = false;

        lv_timer_pause(checkup->pumpTimer);

        checkup->processTimer = lv_timer_create(processTimer, 1000, pn);
    }
}

void handleIntermediateOrLastStep(processNode *pn, bool isLastStep) {
    sCheckup* checkup = pn->process.processDetails->checkup;
    if (isLastStep) {
        pumpFrom = getValueForChemicalSource(WASTE);//last step go into the waste
        pumpDir = PUMP_OUT_RLY;
        LV_LOG_USER("Last step");
        if (tankPercentage < 100) {
            LV_LOG_USER("Last step DRAINING");

            if (checkup->data.isAlreadyPumping == false) {
                sendValueToRelay(pumpFrom, pumpDir, true);
                checkup->data.isAlreadyPumping = true;
            }

            lv_arc_set_value(checkup->pumpArc, 100 - tankPercentage);
            lv_label_set_text(checkup->checkupStepKindValue, checkupDraining_text);
            tankTimeElapsed++;
        } else {
            LV_LOG_USER("Last step DRAINING COMPLETE");

            sendValueToRelay(pumpFrom, pumpDir, false);
            stopMotor(MOTOR_IN1_PIN, MOTOR_IN2_PIN);
            stopMotorTask();
            checkup->data.isAlreadyPumping = false;

            lv_arc_set_value(checkup->pumpArc, 100 - tankPercentage);
            lv_label_set_text(checkup->checkupStepKindValue, checkupDrainingComplete_text);
            lv_obj_clear_state(checkup->checkupCloseButton, LV_STATE_DISABLED);
            safeTimerDelete(&checkup->pumpTimer);
        }
    } else {
        LV_LOG_USER("Intermediate step");
        if (checkup->data.isFilling) {
            pumpFrom = getValueForChemicalSource(checkup->currentStep->step.stepDetails->data.source);
            pumpDir = PUMP_IN_RLY;
            if (tankPercentage < 100) {
                LV_LOG_USER("Middle step FILLING");

                if (checkup->data.isAlreadyPumping == false) {

                    sendValueToRelay(pumpFrom, pumpDir, true);
                    checkup->data.isAlreadyPumping = true;
                }

                lv_arc_set_value(checkup->pumpArc, tankPercentage);
                lv_label_set_text(checkup->checkupStepKindValue, checkupFilling_text);
                tankTimeElapsed++;
            } else {
                LV_LOG_USER("Middle step FILLING COMPLETE");


                sendValueToRelay(pumpFrom, pumpDir, false);
                checkup->data.isAlreadyPumping = false;

                lv_arc_set_value(checkup->pumpArc, tankPercentage);
                lv_label_set_text(checkup->checkupStepKindValue, checkupProcessing_text);

                tankPercentage = 0;
                tankTimeElapsed = 0;
                checkup->data.isFilling = false;

                lv_timer_pause(checkup->pumpTimer);
                lv_timer_resume(checkup->processTimer);
            }
        } else {
            if(checkup->currentStep->prev != NULL && checkup->currentStep->prev->step.stepDetails->data.discardAfterProc == false)
                pumpFrom = getValueForChemicalSource(checkup->currentStep->prev->step.stepDetails->data.source);
            else
                pumpFrom = getValueForChemicalSource(WASTE);
            pumpDir = PUMP_OUT_RLY;
            if (tankPercentage < 100) {
                LV_LOG_USER("Middle step DRAINING");

                if (checkup->data.isAlreadyPumping == false) {
                    sendValueToRelay(pumpFrom, pumpDir, true);
                    checkup->data.isAlreadyPumping = true;
                }

                lv_arc_set_value(checkup->pumpArc, 100 - tankPercentage);
                lv_label_set_text(checkup->checkupStepKindValue, checkupDraining_text);
                tankTimeElapsed++;
            } else {
                LV_LOG_USER("Middle step DRAINING COMPLETE");

                sendValueToRelay(pumpFrom, pumpDir, false);
                checkup->data.isAlreadyPumping = false;

                lv_arc_set_value(checkup->pumpArc, 100 - tankPercentage);
                lv_label_set_text(checkup->checkupStepKindValue, checkupDrainingComplete_text);

                tankPercentage = 0;
                tankTimeElapsed = 0;
                checkup->data.isFilling = true;
            }
        }
    }
}

void handleStopNow(processNode *pn) {
    sCheckup* checkup = pn->process.processDetails->checkup;
    pumpFrom = getValueForChemicalSource(checkup->currentStep->step.stepDetails->data.source);
    pumpDir = PUMP_OUT_RLY;

    if (!checkup->data.isFilling && tankPercentage == 0) {
        tankPercentage = 100;
        tankTimeElapsed = tankFillTime;
        checkup->data.isFilling = true;

        sendValueToRelay(0, 0, false);
        sendValueToRelay(pumpFrom, pumpDir, true);
        checkup->data.isAlreadyPumping = false;
    }
    if (tankPercentage > 0) {
        LV_LOG_USER("STOP NOW DRAINING");

        
        if(checkup->data.isAlreadyPumping == true){
            sendValueToRelay(0, 0, false);
            sendValueToRelay(pumpFrom, pumpDir, true);
            checkup->data.isAlreadyPumping = false;
        }
      

        lv_arc_set_value(checkup->pumpArc, tankPercentage);
        lv_label_set_text(checkup->checkupStepKindValue, checkupDraining_text);
        tankTimeElapsed--;
    } else {
        LV_LOG_USER("STOP NOW DRAINING COMPLETE");

        sendValueToRelay(pumpFrom, pumpDir, false);
        stopMotor(MOTOR_IN1_PIN, MOTOR_IN2_PIN);
        stopMotorTask();
        checkup->data.isAlreadyPumping = false;

        lv_arc_set_value(checkup->pumpArc, tankPercentage);
        lv_label_set_text(checkup->checkupStepKindValue, checkupDrainingComplete_text);
        lv_obj_clear_state(checkup->checkupCloseButton, LV_STATE_DISABLED);
        safeTimerDelete(&checkup->pumpTimer);

    }
}

void handleStopAfter(processNode *pn) {
    sCheckup* checkup = pn->process.processDetails->checkup;
    if (checkup->data.isFilling) {
        pumpFrom = getValueForChemicalSource(checkup->currentStep->step.stepDetails->data.source);
        pumpDir = PUMP_IN_RLY;
        if (tankPercentage < 100) {
            LV_LOG_USER("STOP AFTER step FILLING");

        if (checkup->data.isAlreadyPumping == false) {
            sendValueToRelay(pumpFrom, pumpDir, true);
            checkup->data.isAlreadyPumping = true;
        }


            lv_arc_set_value(checkup->pumpArc, tankPercentage);
            lv_label_set_text(checkup->checkupStepKindValue, checkupFilling_text);
            tankTimeElapsed++;
        } else {
            LV_LOG_USER("STOP AFTER step FILLING COMPLETE");


            sendValueToRelay(pumpFrom, pumpDir, false);
            checkup->data.isAlreadyPumping = false;

            lv_arc_set_value(checkup->pumpArc, tankPercentage);
            lv_label_set_text(checkup->checkupStepKindValue, checkupProcessing_text);

            tankPercentage = 0;
            tankTimeElapsed = 0;
            checkup->data.isFilling = false;

            lv_timer_pause(checkup->pumpTimer);
            if(checkup->processTimer != NULL)
                lv_timer_resume(checkup->processTimer);
            else
                checkup->processTimer = lv_timer_create(processTimer, 1000, pn);
        }
    } else {
        if(checkup->currentStep->prev != NULL){
            pumpFrom = getValueForChemicalSource(checkup->currentStep->prev->step.stepDetails->data.source);
        }else{
            pumpFrom = getValueForChemicalSource(checkup->currentStep->step.stepDetails->data.source);
        }
        
        pumpDir = PUMP_OUT_RLY;
        if (tankPercentage < 100) {
            LV_LOG_USER("STOP AFTER step DRAINING");


            if (checkup->data.isAlreadyPumping == false) {
                sendValueToRelay(pumpFrom, pumpDir, true);
                checkup->data.isAlreadyPumping = true;
            }

            lv_arc_set_value(checkup->pumpArc, 100 - tankPercentage);
            lv_label_set_text(checkup->checkupStepKindValue, checkupDraining_text);
            tankTimeElapsed++;
        } else {
            LV_LOG_USER("STOP AFTER step DRAINING COMPLETE");

            sendValueToRelay(pumpFrom, pumpDir, false);
            stopMotor(MOTOR_IN1_PIN, MOTOR_IN2_PIN);
            stopMotorTask();
            checkup->data.isAlreadyPumping = false;

            lv_arc_set_value(checkup->pumpArc, 100 - tankPercentage);
            lv_label_set_text(checkup->checkupStepKindValue, checkupDrainingComplete_text);
            lv_obj_clear_state(checkup->checkupCloseButton, LV_STATE_DISABLED);
            safeTimerDelete(&checkup->pumpTimer);
        }
    }
}

void handleStopNowAfterStopAfter(processNode *pn) {
    sCheckup* checkup = pn->process.processDetails->checkup;
    pumpFrom = getValueForChemicalSource(checkup->currentStep->step.stepDetails->data.source);
    pumpDir = PUMP_OUT_RLY;

    if (!checkup->data.isFilling && tankPercentage == 0) {
        tankPercentage = 100;
        tankTimeElapsed = tankFillTime;
        checkup->data.isFilling = true;

        sendValueToRelay(0, 0, false);
        sendValueToRelay(pumpFrom, pumpDir, true);
        checkup->data.isAlreadyPumping = false;
    }
    if (tankPercentage > 0) {
        LV_LOG_USER("STOP NOW after STOP AFTER step NOW DRAINING");

        if(checkup->data.isAlreadyPumping == true){
            sendValueToRelay(0, 0, false);
            sendValueToRelay(pumpFrom, pumpDir, true);
            checkup->data.isAlreadyPumping = false;
        }

        lv_arc_set_value(checkup->pumpArc, tankPercentage);
        lv_label_set_text(checkup->checkupStepKindValue, checkupDraining_text);
        tankTimeElapsed--;
    } else {
        LV_LOG_USER("STOP NOW after STOP AFTER DRAINING COMPLETE");

        sendValueToRelay(pumpFrom, pumpDir, false);
        checkup->data.isAlreadyPumping = false;

        lv_arc_set_value(checkup->pumpArc, tankPercentage);
        lv_label_set_text(checkup->checkupStepKindValue, checkupDrainingComplete_text);
        lv_obj_clear_state(checkup->checkupCloseButton, LV_STATE_DISABLED);
        safeTimerDelete(&checkup->pumpTimer);
    }
}


void pumpTimer(lv_timer_t *timer) {
    /* Extract context from timer user_data */
    processNode *pn = (processNode *)lv_timer_get_user_data(timer);
    if(pn == NULL || pn->process.processDetails == NULL || pn->process.processDetails->checkup == NULL) return;

    tankPercentage = calculatePercentage(0, tankTimeElapsed, 0, tankFillTime);

    sCheckup *checkup = pn->process.processDetails->checkup;

    if (!checkup->data.stopAfter) {
        if (!checkup->data.stopNow) {
            if (checkup->currentStep) {
                if (!checkup->currentStep->prev) {
                    handleFirstStep(pn);
                } else {
                    handleIntermediateOrLastStep(pn, false);
                }
            } else {
                handleIntermediateOrLastStep(pn, true);
            }
        } else {
            handleStopNow(pn);
        }
    } else {
        if (!checkup->data.stopNow) {
            handleStopAfter(pn);
        } else {
            handleStopNowAfterStopAfter(pn);
        }
    }
}






/* ═══════════════════════════════════════════════════════════════════════
 * Checkup sub-screen rendering functions
 * These helper functions handle the UI construction for each processStep
 * ═══════════════════════════════════════════════════════════════════════ */

static void checkup_renderPreFlight(processNode *proc) {
    /* processStep 0: Tank size and chemistry volume selection */
    sCheckup *ckup = proc->process.processDetails->checkup;
    if(isStepStatus0created == 0) {
        ckup->checkupSelectTankChemistryContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupSelectTankChemistryContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupSelectTankChemistryContainer, LV_ALIGN_TOP_LEFT, -18, -18);
        lv_obj_set_size(ckup->checkupSelectTankChemistryContainer, 240, 265);
        lv_obj_set_style_border_opa(ckup->checkupSelectTankChemistryContainer, LV_OPA_TRANSP, 0);
        lv_obj_add_event_cb(ckup->checkupSelectTankChemistryContainer, event_checkup, LV_EVENT_CLICKED, proc);

        ckup->checkupProcessReadyLabel = lv_label_create(ckup->checkupSelectTankChemistryContainer);
        lv_label_set_text(ckup->checkupProcessReadyLabel, checkupProcessReady_text);
        lv_obj_set_width(ckup->checkupProcessReadyLabel, 230);
        lv_obj_set_style_text_font(ckup->checkupProcessReadyLabel, &lv_font_montserrat_22, 0);
        lv_obj_align(ckup->checkupProcessReadyLabel, LV_ALIGN_TOP_LEFT, -10, -8);

        ckup->lowVolumeChemRadioButton = create_radiobutton(ckup->checkupSelectTankChemistryContainer, checkupChemistryLowVol_text, -105, 45, 27, &lv_font_montserrat_18, lv_color_hex(GREEN_DARK), lv_palette_main(LV_PALETTE_GREEN));
        ckup->highVolumeChemRadioButton = create_radiobutton(ckup->checkupSelectTankChemistryContainer, checkupChemistryHighVol_text, -10, 45, 27, &lv_font_montserrat_18, lv_color_hex(GREEN_DARK), lv_palette_main(LV_PALETTE_GREEN));

        ckup->checkupTankSizeLabel = lv_label_create(ckup->checkupSelectTankChemistryContainer);
        lv_label_set_text(ckup->checkupTankSizeLabel, checkupTankSize_text);
        lv_obj_set_width(ckup->checkupTankSizeLabel, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(ckup->checkupTankSizeLabel, &lv_font_montserrat_18, 0);
        lv_obj_align(ckup->checkupTankSizeLabel, LV_ALIGN_TOP_MID, 0, 20);

        ckup->checkupTankSizeTextArea = lv_textarea_create(ckup->checkupSelectTankChemistryContainer);
        lv_textarea_set_one_line(ckup->checkupTankSizeTextArea, true);
        lv_textarea_set_placeholder_text(ckup->checkupTankSizeTextArea, checkupTankSizePlaceHolder_text);
        lv_obj_align(ckup->checkupTankSizeTextArea, LV_ALIGN_TOP_MID, 0, 45);
        lv_obj_set_width(ckup->checkupTankSizeTextArea, 100);

        lv_obj_add_event_cb(ckup->checkupTankSizeTextArea, event_checkup, LV_EVENT_FOCUSED, proc);
        lv_obj_add_event_cb(ckup->checkupTankSizeTextArea, event_checkup, LV_EVENT_VALUE_CHANGED, proc);
        lv_obj_add_state(ckup->checkupTankSizeTextArea, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_color(ckup->checkupTankSizeTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_set_style_text_align(ckup->checkupTankSizeTextArea, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_add_style(ckup->checkupTankSizeTextArea, &ckup->textAreaStyleCheckup, LV_PART_MAIN);
        lv_obj_set_style_border_color(ckup->checkupTankSizeTextArea, lv_color_hex(WHITE), 0);

        /* Show the default tank size value in the text area (tankSize is 1-based) */
        {
            const char *sizes[] = {"Small", "Medium", "Large"};
            uint8_t idx = ckup->data.tankSize;
            if(idx >= 1 && idx <= 3) {
                lv_textarea_set_text(ckup->checkupTankSizeTextArea, sizes[idx - 1]);
            }
        }

        ckup->checkupChemistryVolumeLabel = lv_label_create(ckup->checkupSelectTankChemistryContainer);
        lv_label_set_text(ckup->checkupChemistryVolumeLabel, checkupChemistryVolume_text);
        lv_obj_set_width(ckup->checkupChemistryVolumeLabel, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(ckup->checkupChemistryVolumeLabel, &lv_font_montserrat_18, 0);
        lv_obj_align(ckup->checkupChemistryVolumeLabel, LV_ALIGN_TOP_MID, 0, 110);

        ckup->checkupStartButton = lv_button_create(ckup->checkupSelectTankChemistryContainer);
        lv_obj_set_size(ckup->checkupStartButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
        lv_obj_align(ckup->checkupStartButton, LV_ALIGN_BOTTOM_MID, 0, 10);
        lv_obj_add_event_cb(ckup->checkupStartButton, event_checkup, LV_EVENT_CLICKED, proc);
        lv_obj_set_style_bg_color(ckup->checkupStartButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->checkupStartButton);

        ckup->checkupStartButtonLabel = lv_label_create(ckup->checkupStartButton);
        lv_label_set_text(ckup->checkupStartButtonLabel, checkupStart_text);
        lv_obj_set_style_text_font(ckup->checkupStartButtonLabel, &lv_font_montserrat_22, 0);
        lv_obj_align(ckup->checkupStartButtonLabel, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_state(ckup->checkupStartButton, LV_STATE_DISABLED);

        isStepStatus0created = 1;
    }
}

static void checkup_renderFillWater(processNode *proc) {
    /* processStep 1: Fill water */
    sCheckup *ckup = proc->process.processDetails->checkup;
    if(isStepStatus1created == 0) {
        lv_obj_clean(ckup->checkupStepContainer);
        ckup->checkupFillWaterContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupFillWaterContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupFillWaterContainer, LV_ALIGN_TOP_LEFT, -18, -18);
        lv_obj_set_size(ckup->checkupFillWaterContainer, 240, 265);
        lv_obj_set_style_border_opa(ckup->checkupFillWaterContainer, LV_OPA_TRANSP, 0);

        ckup->checkupFillWaterLabel = lv_label_create(ckup->checkupFillWaterContainer);
        lv_label_set_text(ckup->checkupFillWaterLabel, checkupFillWaterMachine_text);
        lv_obj_set_style_text_font(ckup->checkupFillWaterLabel, &lv_font_montserrat_18, 0);
        lv_obj_align(ckup->checkupFillWaterLabel, LV_ALIGN_CENTER, 0, -20);
        lv_obj_set_style_text_align(ckup->checkupFillWaterLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(ckup->checkupFillWaterLabel, LV_LABEL_LONG_WRAP);
        lv_obj_set_size(ckup->checkupFillWaterLabel, 235, LV_SIZE_CONTENT);

        ckup->checkupSkipButton = lv_button_create(ckup->checkupFillWaterContainer);
        lv_obj_set_size(ckup->checkupSkipButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
        lv_obj_align(ckup->checkupSkipButton, LV_ALIGN_BOTTOM_MID, 0, 10);
        lv_obj_add_event_cb(ckup->checkupSkipButton, event_checkup, LV_EVENT_CLICKED, proc);
        lv_obj_set_style_bg_color(ckup->checkupSkipButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->checkupSkipButton);

        ckup->checkupSkipButtonLabel = lv_label_create(ckup->checkupSkipButton);
        lv_label_set_text(ckup->checkupSkipButtonLabel, checkupSkip_text);
        lv_obj_set_style_text_font(ckup->checkupSkipButtonLabel, &lv_font_montserrat_22, 0);
        lv_obj_align(ckup->checkupSkipButtonLabel, LV_ALIGN_CENTER, 0, 0);

        isStepStatus1created = 1;
    }
}

static void checkup_renderReachTemp(processNode *proc) {
    /* processStep 2: Reach temperature */
    sCheckup *ckup = proc->process.processDetails->checkup;
    if(isStepStatus2created == 0) {
        lv_obj_clean(ckup->checkupStepContainer);
        ckup->checkupTargetTempsContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupTargetTempsContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTargetTempsContainer, LV_ALIGN_TOP_LEFT, -18, -18);
        lv_obj_set_size(ckup->checkupTargetTempsContainer, 240, 265);
        lv_obj_set_style_border_opa(ckup->checkupTargetTempsContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTargetTempContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupTargetTempContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTargetTempContainer, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_size(ckup->checkupTargetTempContainer, 200, 90);
        lv_obj_set_style_border_opa(ckup->checkupTargetTempContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTargetTempLabel = lv_label_create(ckup->checkupTargetTempContainer);
        lv_label_set_text(ckup->checkupTargetTempLabel, checkupTargetTemp_text);
        lv_obj_set_style_text_font(ckup->checkupTargetTempLabel, &lv_font_montserrat_20, 0);
        lv_obj_align(ckup->checkupTargetTempLabel, LV_ALIGN_CENTER, 0, -30);

        ckup->checkupTargetTempValue = lv_label_create(ckup->checkupTargetTempContainer);

        if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
            lv_label_set_text_fmt(ckup->checkupTargetTempValue, "%"PRIi32"°C", proc->process.processDetails->data.temp);
        } else{
            lv_label_set_text_fmt(ckup->checkupTargetTempValue, "%"PRIi32"°F", convertCelsiusToFahrenheit(proc->process.processDetails->data.temp));
        }

        lv_obj_set_style_text_font(ckup->checkupTargetTempValue, &lv_font_montserrat_28, 0);
        lv_obj_align(ckup->checkupTargetTempValue, LV_ALIGN_CENTER, 0, 5);

        ckup->checkupTargetToleranceTempValue = lv_label_create(ckup->checkupTargetTempContainer);
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%s ~%.1f", checkupTargetToleranceTemp_text, proc->process.processDetails->data.tempTolerance);
            lv_label_set_text(ckup->checkupTargetToleranceTempValue, buf);
        }
        lv_obj_set_style_text_font(ckup->checkupTargetToleranceTempValue, &lv_font_montserrat_20, 0);
        lv_obj_align(ckup->checkupTargetToleranceTempValue, LV_ALIGN_CENTER, 0, 30);

        ckup->checkupTargetWaterTempContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupTargetWaterTempContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTargetWaterTempContainer, LV_ALIGN_BOTTOM_LEFT, 0, -50);
        lv_obj_set_size(ckup->checkupTargetWaterTempContainer, 100, 80);
        lv_obj_set_style_border_opa(ckup->checkupTargetWaterTempContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTargetWaterTempLabel = lv_label_create(ckup->checkupTargetWaterTempContainer);
        lv_label_set_text(ckup->checkupTargetWaterTempLabel, checkupWater_text);
        lv_obj_set_style_text_font(ckup->checkupTargetWaterTempLabel, &lv_font_montserrat_18, 0);
        lv_obj_align(ckup->checkupTargetWaterTempLabel, LV_ALIGN_CENTER, 0, -15);

        ckup->checkupTargetWaterTempValue = lv_label_create(ckup->checkupTargetWaterTempContainer);

        {
            char buf[16];
            float initWaterTemp = sim_getTemperature(TEMPERATURE_SENSOR_BATH);
            if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
                snprintf(buf, sizeof(buf), "%.1f°C", initWaterTemp);
            } else{
                snprintf(buf, sizeof(buf), "%.1f°F", initWaterTemp * 1.8f + 32.0f);
            }
            lv_label_set_text(ckup->checkupTargetWaterTempValue, buf);
        }

        lv_obj_set_style_text_font(ckup->checkupTargetWaterTempValue, &lv_font_montserrat_20, 0);
        lv_obj_align(ckup->checkupTargetWaterTempValue, LV_ALIGN_CENTER, 0, 20);

        ckup->checkupTargetChemistryTempContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupTargetChemistryTempContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTargetChemistryTempContainer, LV_ALIGN_BOTTOM_RIGHT, 0, -50);
        lv_obj_set_size(ckup->checkupTargetChemistryTempContainer, 100, 80);
        lv_obj_set_style_border_opa(ckup->checkupTargetChemistryTempContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTargetChemistryTempLabel = lv_label_create(ckup->checkupTargetChemistryTempContainer);
        lv_label_set_text(ckup->checkupTargetChemistryTempLabel, checkupChemistry_text);
        lv_obj_set_style_text_font(ckup->checkupTargetChemistryTempLabel, &lv_font_montserrat_18, 0);
        lv_obj_align(ckup->checkupTargetChemistryTempLabel, LV_ALIGN_CENTER, 0, -15);

        ckup->checkupTargetChemistryTempValue = lv_label_create(ckup->checkupTargetChemistryTempContainer);

        {
            char buf[16];
            float initChemTemp = sim_getTemperature(TEMPERATURE_SENSOR_CHEMICAL);
            if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
                snprintf(buf, sizeof(buf), "%.1f°C", initChemTemp);
            } else{
                snprintf(buf, sizeof(buf), "%.1f°F", initChemTemp * 1.8f + 32.0f);
            }
            lv_label_set_text(ckup->checkupTargetChemistryTempValue, buf);
        }

        lv_obj_set_style_text_font(ckup->checkupTargetChemistryTempValue, &lv_font_montserrat_20, 0);
        lv_obj_align(ckup->checkupTargetChemistryTempValue, LV_ALIGN_CENTER, 0, 20);

        ckup->checkupSkipButton = lv_button_create(ckup->checkupTargetTempsContainer);
        lv_obj_set_size(ckup->checkupSkipButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
        lv_obj_align(ckup->checkupSkipButton, LV_ALIGN_BOTTOM_MID, 0, 10);
        lv_obj_add_event_cb(ckup->checkupSkipButton, event_checkup, LV_EVENT_CLICKED, proc);
        lv_obj_set_style_bg_color(ckup->checkupSkipButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->checkupSkipButton);

        ckup->checkupSkipButtonLabel = lv_label_create(ckup->checkupSkipButton);
        lv_label_set_text(ckup->checkupSkipButtonLabel, checkupSkip_text);
        lv_obj_set_style_text_font(ckup->checkupSkipButtonLabel, &lv_font_montserrat_22, 0);
        lv_obj_align(ckup->checkupSkipButtonLabel, LV_ALIGN_CENTER, 0, 0);

        ckup->checkupHeaterStatusLabel = lv_label_create(ckup->checkupTargetTempsContainer);
        lv_label_set_text_fmt(ckup->checkupHeaterStatusLabel, checkupHeaterStatusFmt_text, checkupHeaterOff_text);
        lv_obj_set_style_text_font(ckup->checkupHeaterStatusLabel, &lv_font_montserrat_16, 0);
        lv_obj_align(ckup->checkupHeaterStatusLabel, LV_ALIGN_BOTTOM_MID, 0, -35);

        ckup->checkupTempTimeoutLabel = NULL;

        /* Start temperature monitoring timer if temp control is enabled */
        if(proc->process.processDetails->data.isTempControlled) {
            LV_LOG_USER("Temperature control enabled — starting temp timer (target=%"PRIu32"°C, tolerance=%.1f)",
                proc->process.processDetails->data.temp,
                proc->process.processDetails->data.tempTolerance);
            ckup->data.tempTimeoutCounter = 0;
            ckup->data.heaterOn = false;
            ckup->data.currentWaterTemp = sim_getTemperature(TEMPERATURE_SENSOR_BATH);
            ckup->data.currentChemTemp = sim_getTemperature(TEMPERATURE_SENSOR_CHEMICAL);
            ckup->tempTimer = lv_timer_create(tempTimerCallback, 1000, proc);
        } else {
            ckup->tempTimer = NULL;
            lv_label_set_text(ckup->checkupHeaterStatusLabel, checkupNoTempControl_text);
        }

        isStepStatus2created = 1;
    }
}

static void checkup_renderCheckFilm(processNode *proc) {
    /* processStep 3: Check film/tank */
    sCheckup *ckup = proc->process.processDetails->checkup;
    if(isStepStatus3created == 0) {
        lv_obj_clean(ckup->checkupStepContainer);
        ckup->checkupFilmRotatingContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupFilmRotatingContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupFilmRotatingContainer, LV_ALIGN_TOP_LEFT, -18, -18);
        lv_obj_set_size(ckup->checkupFilmRotatingContainer, 240, 265);
        lv_obj_set_style_border_opa(ckup->checkupFilmRotatingContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTankIsPresentContainer = lv_obj_create(ckup->checkupFilmRotatingContainer);
        lv_obj_remove_flag(ckup->checkupTankIsPresentContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTankIsPresentContainer, LV_ALIGN_CENTER, 0, -55);
        lv_obj_set_size(ckup->checkupTankIsPresentContainer, 200, 80);
        lv_obj_set_style_border_opa(ckup->checkupTankIsPresentContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTankIsPresentLabel = lv_label_create(ckup->checkupTankIsPresentContainer);
        lv_label_set_text(ckup->checkupTankIsPresentLabel, checkupTankPosition_text);
        lv_obj_set_style_text_font(ckup->checkupTankIsPresentLabel, &lv_font_montserrat_20, 0);
        lv_obj_align(ckup->checkupTankIsPresentLabel, LV_ALIGN_CENTER, 0, -15);

        ckup->checkupTankIsPresentValue = lv_label_create(ckup->checkupTankIsPresentContainer);
        lv_label_set_text(ckup->checkupTankIsPresentValue, checkupYes_text);
        lv_obj_set_style_text_font(ckup->checkupTankIsPresentValue, &lv_font_montserrat_24, 0);
        lv_obj_align(ckup->checkupTankIsPresentValue, LV_ALIGN_CENTER, 0, 20);

        ckup->checkupFilmInPositionContainer = lv_obj_create(ckup->checkupFilmRotatingContainer);
        lv_obj_remove_flag(ckup->checkupFilmInPositionContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupFilmInPositionContainer, LV_ALIGN_CENTER, 0, 40);
        lv_obj_set_size(ckup->checkupFilmInPositionContainer, 200, 80);
        lv_obj_set_style_border_opa(ckup->checkupFilmInPositionContainer, LV_OPA_TRANSP, 0);

        ckup->checkupFilmRotatingLabel = lv_label_create(ckup->checkupFilmInPositionContainer);
        lv_label_set_text(ckup->checkupFilmRotatingLabel, checkupFilmRotation_text);
        lv_obj_set_style_text_font(ckup->checkupFilmRotatingLabel, &lv_font_montserrat_20, 0);
        lv_obj_align(ckup->checkupFilmRotatingLabel, LV_ALIGN_CENTER, 0, -15);

        ckup->checkupFilmRotatingValue = lv_label_create(ckup->checkupFilmInPositionContainer);
        lv_label_set_text(ckup->checkupFilmRotatingValue, checkupChecking_text);
        lv_obj_set_style_text_font(ckup->checkupFilmRotatingValue, &lv_font_montserrat_24, 0);
        lv_obj_align(ckup->checkupFilmRotatingValue, LV_ALIGN_CENTER, 0, 20);

        ckup->checkupStartButton = lv_button_create(ckup->checkupFilmRotatingContainer);
        lv_obj_set_size(ckup->checkupStartButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
        lv_obj_align(ckup->checkupStartButton, LV_ALIGN_BOTTOM_MID, 0, 10);
        lv_obj_add_event_cb(ckup->checkupStartButton, event_checkup, LV_EVENT_CLICKED, proc);
        lv_obj_set_style_bg_color(ckup->checkupStartButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->checkupStartButton);

        ckup->checkupStartButtonLabel = lv_label_create(ckup->checkupStartButton);
        lv_label_set_text(ckup->checkupStartButtonLabel, checkupStart_text);
        lv_obj_set_style_text_font(ckup->checkupStartButtonLabel, &lv_font_montserrat_22, 0);
        lv_obj_align(ckup->checkupStartButtonLabel, LV_ALIGN_CENTER, 0, 0);

        isStepStatus3created = 1;
    }
}

static void checkup_renderProcessing(processNode *proc) {
    /* processStep 4: Main processing */
    sCheckup *ckup = proc->process.processDetails->checkup;
    if(isStepStatus4created == 0) {
        uint16_t tmp_tanksSizesAndTimes[2][3][2] = tanksSizesAndTimes;
        tankFillTime = tmp_tanksSizesAndTimes[ckup->data.activeVolume_index - 1][ckup->data.tankSize -1][1];
        LV_LOG_USER("tankFillTime Volume %"PRIu32", size %"PRIu8", time %"PRIu8"",ckup->data.activeVolume_index -1,ckup->data.tankSize - 1,tankFillTime);

        ckup->data.isFilling = true;
        ckup->pumpTimer = lv_timer_create(pumpTimer, 1000, proc);
        lv_obj_add_state(ckup->checkupCloseButton, LV_STATE_DISABLED);

        lv_obj_clean(ckup->checkupStepContainer);
        ckup->checkupProcessingContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupProcessingContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupProcessingContainer, LV_ALIGN_TOP_LEFT, -18, -18);
        lv_obj_set_size(ckup->checkupProcessingContainer, 240, 265);
        lv_obj_set_style_border_opa(ckup->checkupProcessingContainer, LV_OPA_TRANSP, 0);

        ckup->processArc = lv_arc_create(ckup->checkupProcessingContainer);
        lv_obj_set_size(ckup->processArc, 220, 220);
        lv_arc_set_rotation(ckup->processArc, 140);
        lv_arc_set_bg_angles(ckup->processArc, 0, 260);
        lv_arc_set_value(ckup->processArc, 0);
        lv_arc_set_range(ckup->processArc, 0, 100);
        lv_obj_align(ckup->processArc, LV_ALIGN_CENTER, 0, 2);
        lv_obj_remove_style(ckup->processArc, NULL, LV_PART_KNOB);
        lv_obj_remove_flag(ckup->processArc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_color(ckup->processArc,lv_color_hex(GREEN) , LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(ckup->processArc, lv_color_hex(GREEN_DARK), LV_PART_MAIN);

        ckup->checkupProcessTimeLeftValue = lv_label_create(ckup->checkupProcessingContainer);
        lv_label_set_text_fmt(ckup->checkupProcessTimeLeftValue, "%"PRIu32"m%"PRIu8"s",
          proc->process.processDetails->data.timeMins, proc->process.processDetails->data.timeSecs);
        lv_obj_set_style_text_font(ckup->checkupProcessTimeLeftValue, &lv_font_montserrat_28, 0);
        lv_obj_align(ckup->checkupProcessTimeLeftValue, LV_ALIGN_CENTER, 0, -60);

        ckup->checkupStepNameValue = lv_label_create(ckup->checkupProcessingContainer);
        lv_label_set_text(ckup->checkupStepNameValue, "...");
        lv_obj_set_style_text_align(ckup->checkupStepNameValue, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(ckup->checkupStepNameValue, &lv_font_montserrat_22, 0);
        lv_obj_align(ckup->checkupStepNameValue, LV_ALIGN_CENTER, 0, -10);
        lv_obj_set_width(ckup->checkupStepNameValue, 170);
        lv_label_set_long_mode(ckup->checkupStepNameValue, LV_LABEL_LONG_SCROLL_CIRCULAR);

        ckup->checkupStepTimeLeftValue = lv_label_create(ckup->checkupProcessingContainer);
        lv_label_set_text_fmt(ckup->checkupStepTimeLeftValue, "%dm%ds",
            proc->process.processDetails->stepElementsList.start->step.stepDetails->data.timeMins,
            proc->process.processDetails->stepElementsList.start->step.stepDetails->data.timeSecs);
        lv_obj_set_style_text_font(ckup->checkupStepTimeLeftValue, &lv_font_montserrat_22, 0);
        lv_obj_align(ckup->checkupStepTimeLeftValue, LV_ALIGN_CENTER, 0, 14);

        ckup->stepArc = lv_arc_create(ckup->checkupProcessingContainer);
        lv_obj_set_size(ckup->stepArc, 220, 220);
        lv_arc_set_rotation(ckup->stepArc, 230);
        lv_arc_set_bg_angles(ckup->stepArc, 0, 80);
        lv_arc_set_value(ckup->stepArc, 0);
        lv_arc_set_range(ckup->stepArc, 0, 100);
        lv_obj_align(ckup->stepArc, LV_ALIGN_CENTER, 0, 139);
        lv_obj_remove_style(ckup->stepArc, NULL, LV_PART_KNOB);
        lv_obj_remove_flag(ckup->stepArc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_color(ckup->stepArc,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(ckup->stepArc, lv_color_hex(ORANGE_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->stepArc);

        ckup->checkupStepKindValue = lv_label_create(ckup->checkupProcessingContainer);
        lv_label_set_text(ckup->checkupStepKindValue, checkupFilling_text);
        lv_obj_set_style_text_font(ckup->checkupStepKindValue, &lv_font_montserrat_20, 0);
        lv_obj_align(ckup->checkupStepKindValue, LV_ALIGN_CENTER, 0, 69);
        lv_obj_add_flag(ckup->checkupStepKindValue, LV_OBJ_FLAG_HIDDEN);

        ckup->pumpArc = lv_arc_create(ckup->checkupProcessingContainer);
        lv_obj_set_size(ckup->pumpArc, 220, 220);
        lv_arc_set_rotation(ckup->pumpArc, 48);
        lv_arc_set_bg_angles(ckup->pumpArc, 0, 84);
        lv_arc_set_range(ckup->pumpArc, 0, 100);
        lv_obj_align(ckup->pumpArc, LV_ALIGN_CENTER, 0, 2);
        lv_obj_remove_style(ckup->pumpArc, NULL, LV_PART_KNOB);
        lv_obj_remove_flag(ckup->pumpArc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_color(ckup->pumpArc,lv_color_hex(LIGHT_BLUE) , LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(ckup->pumpArc, lv_color_hex(BLUE_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->pumpArc);
        lv_arc_set_mode(ckup->pumpArc, LV_ARC_MODE_REVERSE);

        ckup->checkupProcessCompleteLabel = lv_label_create(ckup->checkupProcessingContainer);
        lv_obj_set_style_text_align(ckup->checkupProcessCompleteLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(ckup->checkupProcessCompleteLabel, &lv_font_montserrat_22, 0);
        lv_obj_align(ckup->checkupProcessCompleteLabel, LV_ALIGN_CENTER, 0, -30);
        lv_obj_add_flag(ckup->checkupProcessCompleteLabel, LV_OBJ_FLAG_HIDDEN);

        isStepStatus4created = 1;
    }
}

void initCheckup(processNode *pn)
{
      sCheckup *ckup = pn->process.processDetails->checkup;
      LV_LOG_USER("Final checks, current on ckup->data.processStep :%d",ckup->data.processStep);

      //in this way processDetail is deleted, to free memory and the "checkout" is a standard screen on top
      lv_obj_del(pn->process.processDetails->processDetailParent);
      ckup->checkupParent = lv_obj_create(NULL);
      lv_scr_load(ckup->checkupParent);

      ckup->currentStep = pn->process.processDetails->stepElementsList.start;
      
      //in this way create a new layer on top of others, so "checkout" will be on top of processDetail
      //ckup->checkupParent = lv_obj_class_create_obj(&lv_msgbox_backdrop_class, lv_layer_top());
      //lv_obj_class_init_obj(ckup->checkupParent);
      //lv_obj_remove_flag(ckup->checkupParent, LV_OBJ_FLAG_IGNORE_LAYOUT);
      //lv_obj_set_size(ckup->checkupParent, LV_PCT(100), LV_PCT(100));
      
      
      if(ckup->checkupParent == NULL){
        LV_LOG_USER("Object not created");
      }
      else {
        LV_LOG_USER("Object created");
      }
      
	    if(ckup->textAreaStyleCheckup.values_and_props == NULL ) {		/* Only initialise the style once! */
        lv_style_init(&ckup->textAreaStyleCheckup);
      }
   

      ckup->checkupContainer = lv_obj_create(ckup->checkupParent);
      lv_obj_align(ckup->checkupContainer, LV_ALIGN_CENTER, 0, 0);
      lv_obj_set_size(ckup->checkupContainer, LV_PCT(100), LV_PCT(100)); 
      lv_obj_remove_flag(ckup->checkupContainer, LV_OBJ_FLAG_SCROLLABLE); 

            ckup->checkupCloseButton = lv_button_create(ckup->checkupContainer);
            lv_obj_set_size(ckup->checkupCloseButton, BUTTON_POPUP_CLOSE_WIDTH * 1.2, BUTTON_POPUP_CLOSE_HEIGHT * 1.2);
            lv_obj_align(ckup->checkupCloseButton, LV_ALIGN_TOP_RIGHT, 7 , -10);
            lv_obj_add_event_cb(ckup->checkupCloseButton, event_checkup, LV_EVENT_CLICKED, pn);
            lv_obj_set_style_bg_color(ckup->checkupCloseButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
            if(ckup->data.processStep > 0){
              lv_obj_add_state(ckup->checkupCloseButton, LV_STATE_DISABLED);            
            }

                  ckup->checkupCloseButtonLabel = lv_label_create(ckup->checkupCloseButton);         
                  lv_label_set_text(ckup->checkupCloseButtonLabel, closePopup_icon); 
                  lv_obj_set_style_text_font(ckup->checkupCloseButtonLabel, &FilMachineFontIcons_30, 0);              
                  lv_obj_align(ckup->checkupCloseButtonLabel, LV_ALIGN_CENTER, 0, 0);


            ckup->checkupProcessNameContainer = lv_obj_create(ckup->checkupContainer);
            lv_obj_remove_flag(ckup->checkupProcessNameContainer, LV_OBJ_FLAG_SCROLLABLE); 
            lv_obj_align(ckup->checkupProcessNameContainer, LV_ALIGN_TOP_LEFT, -10, -15);
            lv_obj_set_size(ckup->checkupProcessNameContainer, 420, 50);
            lv_obj_set_style_border_opa(ckup->checkupProcessNameContainer, LV_OPA_TRANSP, 0);

                  ckup->checkupProcessNameValue = lv_label_create(ckup->checkupProcessNameContainer);
                  lv_label_set_text(ckup->checkupProcessNameValue, pn->process.processDetails->data.processNameString);
                  lv_obj_set_width(ckup->checkupProcessNameValue, 420);
                  lv_obj_set_style_text_font(ckup->checkupProcessNameValue, &lv_font_montserrat_30, 0);              
                  lv_obj_align(ckup->checkupProcessNameValue, LV_ALIGN_TOP_LEFT, -10, -8);
                  lv_label_set_long_mode(ckup->checkupProcessNameValue, LV_LABEL_LONG_SCROLL_CIRCULAR);

            //RIGHT GREEN CONTAINER
            ckup->checkupStepContainer = lv_obj_create(ckup->checkupContainer);
            lv_obj_remove_flag(ckup->checkupStepContainer, LV_OBJ_FLAG_SCROLLABLE); 
            lv_obj_align(ckup->checkupStepContainer, LV_ALIGN_TOP_LEFT, 217, 35);
            lv_obj_set_size(ckup->checkupStepContainer, 240, 265);
            lv_obj_set_style_border_color(ckup->checkupStepContainer, lv_palette_main(LV_PALETTE_GREEN), 0);

            //LEFT WHITE CONTAINER
            ckup->checkupNextStepsContainer = lv_obj_create(ckup->checkupContainer);
            lv_obj_remove_flag(ckup->checkupNextStepsContainer, LV_OBJ_FLAG_SCROLLABLE); 
            lv_obj_align(ckup->checkupNextStepsContainer, LV_ALIGN_TOP_LEFT, -10, 35);
            lv_obj_set_size(ckup->checkupNextStepsContainer, 225, 265);
            lv_obj_set_style_border_color(ckup->checkupNextStepsContainer, lv_color_hex(WHITE), 0);
}

void checkup(processNode *processToCheckup) {

	char *tmp_checkupStepStatuses[] = checkupStepStatuses;
	char *tmp_processSourceList[] = processSourceList;
	char *tmp_processTempControlList[] = processTempControlList;
	uint16_t tmp_tanksSizesAndTimes[2][3][2] = tanksSizesAndTimes;

	/* Backward compatibility: keep gui.tempProcessNode in sync for other modules */
	gui.tempProcessNode = processToCheckup;

	sCheckup *ckup = processToCheckup->process.processDetails->checkup;

  if(ckup->checkupParent == NULL){
    LV_LOG_USER("initCheckup");

//    processToCheckup->process.processDetails->checkup = malloc(sizeof(sCheckup));
    ckup->data.isProcessing = false;
    ckup->data.isFilling = false;
    ckup->data.processStep = 0;

    ckup->data.stepFillWaterStatus = 0;
    ckup->data.stepCheckFilmStatus = 0;
    ckup->data.stepReachTempStatus = 0;
    ckup->data.activeVolume_index  = 0;
    ckup->data.stopAfter  = false;
    ckup->data.stopNow    = false;
    ckup->data.isAlreadyPumping    = false;

    LV_LOG_USER("isProcessing %d", ckup->data.isProcessing);
    if(ckup->data.isProcessing == 0)
        LV_LOG_USER("isProcessing False");

    initCheckup(processToCheckup);
  }
    LV_LOG_USER("initCheckup Done!");
        resetStuffBeforeNextProcess();
        //LEFT SIDE OF SCREEN
        if(ckup->data.isProcessing == 0){
            if(isProcessingStatus0created == 0){
                  ckup->checkupNextStepsLabel = lv_label_create(ckup->checkupNextStepsContainer);         
                  lv_label_set_text(ckup->checkupNextStepsLabel, checkupNexStepsTitle_text); 
                  lv_obj_set_width(ckup->checkupNextStepsLabel, LV_SIZE_CONTENT);
                  lv_obj_set_style_text_font(ckup->checkupNextStepsLabel, &lv_font_montserrat_22, 0);              
                  lv_obj_align(ckup->checkupNextStepsLabel, LV_ALIGN_TOP_LEFT, -10, -8);

                  ckup->checkupMachineWillDoLabel = lv_label_create(ckup->checkupNextStepsContainer);
                  lv_label_set_text(ckup->checkupMachineWillDoLabel, checkupTheMachineWillDo_text); 
                  lv_obj_set_width(ckup->checkupMachineWillDoLabel, LV_SIZE_CONTENT);
                  lv_obj_set_style_text_font(ckup->checkupMachineWillDoLabel, &lv_font_montserrat_18, 0);              
                  lv_obj_align(ckup->checkupMachineWillDoLabel, LV_ALIGN_TOP_LEFT, -10, 17);

                  ckup->checkupWaterFillContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupWaterFillContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupWaterFillContainer, LV_ALIGN_TOP_LEFT, -10, 45);
                  lv_obj_set_size(ckup->checkupWaterFillContainer, 195, 45);
                  lv_obj_set_style_border_opa(ckup->checkupWaterFillContainer, LV_OPA_TRANSP, 0);

                  
                          ckup->checkupWaterFillStatusIcon = lv_label_create(ckup->checkupWaterFillContainer);         
                          lv_label_set_text(ckup->checkupWaterFillStatusIcon, tmp_checkupStepStatuses[ckup->data.stepFillWaterStatus]); 
                          lv_obj_set_style_text_font(ckup->checkupWaterFillStatusIcon, &FilMachineFontIcons_15, 0);              
                          lv_obj_align(ckup->checkupWaterFillStatusIcon, LV_ALIGN_LEFT_MID, -15, 0);

                          ckup->checkupWaterFillLabel = lv_label_create(ckup->checkupWaterFillContainer);         
                          lv_label_set_text(ckup->checkupWaterFillLabel, checkupFillWater_text); 
                          lv_obj_set_width(ckup->checkupWaterFillLabel, 168);
                          lv_obj_set_style_text_font(ckup->checkupWaterFillLabel, &lv_font_montserrat_18, 0);              
                          lv_obj_align(ckup->checkupWaterFillLabel, LV_ALIGN_LEFT_MID, 2, 0);
                          lv_label_set_long_mode(ckup->checkupWaterFillLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);


                  ckup->checkupReachTempContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupReachTempContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupReachTempContainer, LV_ALIGN_TOP_LEFT, -10, 90);
                  lv_obj_set_size(ckup->checkupReachTempContainer, 195, 45);
                  lv_obj_set_style_border_opa(ckup->checkupReachTempContainer, LV_OPA_TRANSP, 0);

                          ckup->checkupReachTempStatusIcon = lv_label_create(ckup->checkupReachTempContainer);
                          lv_obj_set_style_text_font(ckup->checkupReachTempStatusIcon, &FilMachineFontIcons_15, 0);              
                          lv_obj_align(ckup->checkupReachTempStatusIcon, LV_ALIGN_LEFT_MID, -15, 0);


                          ckup->checkupReachTempLabel = lv_label_create(ckup->checkupReachTempContainer);         
                          lv_label_set_text(ckup->checkupReachTempLabel, checkupReachTemp_text); 
                          lv_obj_set_width(ckup->checkupReachTempLabel, 168);
                          lv_obj_set_style_text_font(ckup->checkupReachTempLabel, &lv_font_montserrat_18, 0);              
                          lv_obj_align(ckup->checkupReachTempLabel, LV_ALIGN_LEFT_MID, 2, 0);
                          lv_label_set_long_mode(ckup->checkupReachTempLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);



                  ckup->checkupTankAndFilmContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupTankAndFilmContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupTankAndFilmContainer, LV_ALIGN_TOP_LEFT, -10, 135);
                  lv_obj_set_size(ckup->checkupTankAndFilmContainer, 195, 45);
                  lv_obj_set_style_border_opa(ckup->checkupTankAndFilmContainer, LV_OPA_TRANSP, 0);

                          ckup->checkupTankAndFilmStatusIcon = lv_label_create(ckup->checkupTankAndFilmContainer);
                          lv_obj_set_style_text_font(ckup->checkupTankAndFilmStatusIcon, &FilMachineFontIcons_15, 0);              
                          lv_obj_align(ckup->checkupTankAndFilmStatusIcon, LV_ALIGN_LEFT_MID, -15, 0);


                          ckup->checkupTankAndFilmLabel = lv_label_create(ckup->checkupTankAndFilmContainer);         
                          lv_label_set_text(ckup->checkupTankAndFilmLabel, checkupTankRotation_text); 
                          lv_obj_set_width(ckup->checkupTankAndFilmLabel, 168);
                          lv_obj_set_style_text_font(ckup->checkupTankAndFilmLabel, &lv_font_montserrat_18, 0);              
                          lv_obj_align(ckup->checkupTankAndFilmLabel, LV_ALIGN_LEFT_MID, 2, 0);
                          lv_label_set_long_mode(ckup->checkupTankAndFilmLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
            
                  isProcessingStatus0created = 1;
            }

            lv_label_set_text(ckup->checkupWaterFillStatusIcon, tmp_checkupStepStatuses[ckup->data.stepFillWaterStatus]); 
            lv_label_set_text(ckup->checkupTankAndFilmStatusIcon, tmp_checkupStepStatuses[ckup->data.stepCheckFilmStatus]); 
            lv_label_set_text(ckup->checkupReachTempStatusIcon, tmp_checkupStepStatuses[ckup->data.stepReachTempStatus]); 

        }
        if(ckup->data.isProcessing == 1 && isProcessingStatus1created == 0){
                  lv_obj_clean(ckup->checkupNextStepsContainer);
                  ckup->checkupNextStepsLabel = lv_label_create(ckup->checkupNextStepsContainer);         
                  lv_label_set_text(ckup->checkupNextStepsLabel, checkupProcessingTitle_text); 
                  lv_obj_set_width(ckup->checkupNextStepsLabel, LV_SIZE_CONTENT);
                  lv_obj_set_style_text_font(ckup->checkupNextStepsLabel, &lv_font_montserrat_22, 0);              
                  lv_obj_align(ckup->checkupNextStepsLabel, LV_ALIGN_TOP_LEFT, -10, -8);


                  ckup->checkupStepSourceContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupStepSourceContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupStepSourceContainer, LV_ALIGN_TOP_LEFT, -10, 17);
                  lv_obj_set_size(ckup->checkupStepSourceContainer, 215, 45);
                  lv_obj_set_style_border_opa(ckup->checkupStepSourceContainer, LV_OPA_TRANSP, 0);


                          ckup->checkupStepSourceLabel = lv_label_create(ckup->checkupStepSourceContainer);         
                          lv_label_set_text(ckup->checkupStepSourceLabel, checkupStepSource_text); 
                          lv_obj_set_style_text_font(ckup->checkupStepSourceLabel, &lv_font_montserrat_18, 0);              
                          lv_obj_align(ckup->checkupStepSourceLabel, LV_ALIGN_LEFT_MID, -15, 0);

                          ckup->checkupStepSourceValue = lv_label_create(ckup->checkupStepSourceContainer);         
                          lv_label_set_text(ckup->checkupStepSourceValue, tmp_processSourceList[processToCheckup->process.processDetails->stepElementsList.start->step.stepDetails->data.source]); //THIS NEED TO BE ALIGNED WITH THE ACTUAL STEP OF THE PROCESS!
                          lv_obj_set_width(ckup->checkupStepSourceValue, LV_SIZE_CONTENT);
                          lv_obj_set_style_text_font(ckup->checkupStepSourceValue, &lv_font_montserrat_20, 0);              
                          lv_obj_align(ckup->checkupStepSourceValue, LV_ALIGN_RIGHT_MID, 10, 0);


                  ckup->checkupTempControlContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupTempControlContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupTempControlContainer, LV_ALIGN_TOP_LEFT, -10, 62);
                  lv_obj_set_size(ckup->checkupTempControlContainer, 215, 45);
                  lv_obj_set_style_border_opa(ckup->checkupTempControlContainer, LV_OPA_TRANSP, 0);


                          ckup->checkupTempControlLabel = lv_label_create(ckup->checkupTempControlContainer);         
                          lv_label_set_text(ckup->checkupTempControlLabel, checkupTempControl_text); 
                          lv_obj_set_style_text_font(ckup->checkupTempControlLabel, &lv_font_montserrat_18, 0);              
                          lv_obj_align(ckup->checkupTempControlLabel, LV_ALIGN_LEFT_MID, -15, 0);

                          ckup->checkupTempControlValue = lv_label_create(ckup->checkupTempControlContainer);         
                          lv_label_set_text(ckup->checkupTempControlValue, tmp_processTempControlList[processToCheckup->process.processDetails->data.isTempControlled]);
                          lv_obj_set_width(ckup->checkupTempControlValue, LV_SIZE_CONTENT);
                          lv_obj_set_style_text_font(ckup->checkupTempControlValue, &lv_font_montserrat_20, 0);              
                          lv_obj_align(ckup->checkupTempControlValue, LV_ALIGN_RIGHT_MID, 10, 0);



                  ckup->checkupWaterTempContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupWaterTempContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupWaterTempContainer, LV_ALIGN_TOP_LEFT, -10, 107);
                  lv_obj_set_size(ckup->checkupWaterTempContainer, 215, 45);
                  lv_obj_set_style_border_opa(ckup->checkupWaterTempContainer, LV_OPA_TRANSP, 0);


                          ckup->checkupWaterTempLabel = lv_label_create(ckup->checkupWaterTempContainer);         
                          lv_label_set_text(ckup->checkupWaterTempLabel, checkupWaterTemp_text); 
                          lv_obj_set_style_text_font(ckup->checkupWaterTempLabel, &lv_font_montserrat_18, 0);              
                          lv_obj_align(ckup->checkupWaterTempLabel, LV_ALIGN_LEFT_MID, -15, 0);

                          ckup->checkupWaterTempValue = lv_label_create(ckup->checkupWaterTempContainer);         

                          {
                              char buf[16];
                              float waterTemp = sim_getTemperature(TEMPERATURE_SENSOR_BATH);
                              if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
                                  snprintf(buf, sizeof(buf), "%.1f°C", waterTemp);
                              } else{
                                  snprintf(buf, sizeof(buf), "%.1f°F", waterTemp * 1.8f + 32.0f);
                              }
                              lv_label_set_text(ckup->checkupWaterTempValue, buf);
                          }

                          lv_obj_set_width(ckup->checkupWaterTempValue, LV_SIZE_CONTENT);
                          lv_obj_set_style_text_font(ckup->checkupWaterTempValue, &lv_font_montserrat_20, 0);              
                          lv_obj_align(ckup->checkupWaterTempValue, LV_ALIGN_RIGHT_MID, 10, 0);



                  ckup->checkupNextStepContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupNextStepContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupNextStepContainer, LV_ALIGN_TOP_LEFT, -10, 152);
                  lv_obj_set_size(ckup->checkupNextStepContainer, 215, 45);
                  lv_obj_set_style_border_opa(ckup->checkupNextStepContainer, LV_OPA_TRANSP, 0);


                          ckup->checkupNextStepLabel = lv_label_create(ckup->checkupNextStepContainer);         
                          lv_label_set_text(ckup->checkupNextStepLabel, checkupNextStep_text); 
                          lv_obj_set_style_text_font(ckup->checkupNextStepLabel, &lv_font_montserrat_18, 0);              
                          lv_obj_align(ckup->checkupNextStepLabel, LV_ALIGN_LEFT_MID, -15, 0);

                          ckup->checkupNextStepValue = lv_label_create(ckup->checkupNextStepContainer);
                          //if(ckup->currentStep->next != NULL)
                          //    lv_label_set_text(ckup->checkupNextStepValue, ckup->currentStep->next->step.stepDetails->data.stepNameString);
                          //else
                              lv_label_set_text(ckup->checkupNextStepValue, ckup->currentStep->step.stepDetails->data.stepNameString);

                          lv_obj_set_width(ckup->checkupNextStepValue, 105);
                          lv_obj_set_style_text_font(ckup->checkupNextStepValue, &lv_font_montserrat_20, 0);
                          lv_obj_align(ckup->checkupNextStepValue, LV_ALIGN_RIGHT_MID, 10, 0);
                          lv_label_set_long_mode(ckup->checkupNextStepValue, LV_LABEL_LONG_SCROLL_CIRCULAR);


                  ckup->checkupStopNowButton = lv_button_create(ckup->checkupNextStepsContainer);
                  lv_obj_set_size(ckup->checkupStopNowButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
                  lv_obj_align(ckup->checkupStopNowButton, LV_ALIGN_BOTTOM_LEFT, -10, 10);
                  lv_obj_add_event_cb(ckup->checkupStopNowButton, event_checkup, LV_EVENT_CLICKED, processToCheckup);
                  lv_obj_set_style_bg_color(ckup->checkupStopNowButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
                  lv_obj_move_foreground(ckup->checkupStopNowButton);


                          ckup->checkupStopNowButtonLabel = lv_label_create(ckup->checkupStopNowButton);         
                          lv_label_set_text(ckup->checkupStopNowButtonLabel, checkupStopNow_text); 
                          lv_obj_set_style_text_font(ckup->checkupStopNowButtonLabel, &lv_font_montserrat_16, 0);              
                          lv_obj_align(ckup->checkupStopNowButtonLabel, LV_ALIGN_CENTER, 0, 0);

                  
                  ckup->checkupStopAfterButton = lv_button_create(ckup->checkupNextStepsContainer);
                  lv_obj_set_size(ckup->checkupStopAfterButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
                  lv_obj_align(ckup->checkupStopAfterButton, LV_ALIGN_BOTTOM_RIGHT, 10, 10);
                  lv_obj_add_event_cb(ckup->checkupStopAfterButton, event_checkup, LV_EVENT_CLICKED, processToCheckup);
                  lv_obj_set_style_bg_color(ckup->checkupStopAfterButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
                  lv_obj_move_foreground(ckup->checkupStopAfterButton);
                  if(processToCheckup->process.processDetails->stepElementsList.size == 1){
                    lv_obj_add_state(ckup->checkupStopAfterButton, LV_STATE_DISABLED);
                  }

                          ckup->checkupStopAfterButtonLabel = lv_label_create(ckup->checkupStopAfterButton);         
                          lv_label_set_text(ckup->checkupStopAfterButtonLabel, checkupStopAfter_text); 
                          lv_obj_set_style_text_font(ckup->checkupStopAfterButtonLabel, &lv_font_montserrat_16, 0);              
                          lv_obj_align(ckup->checkupStopAfterButtonLabel, LV_ALIGN_CENTER, 0, 0);
        
            isProcessingStatus1created = 1;
        }

            /* RIGHT SIDE OF SCREEN: Dispatch to appropriate sub-screen based on processStep */
            if(ckup->data.processStep == 0) {
                checkup_renderPreFlight(processToCheckup);
            }

            if(ckup->data.processStep == 1) {
                checkup_renderFillWater(processToCheckup);
            }

            if(ckup->data.processStep == 2) {
                checkup_renderReachTemp(processToCheckup);
            }

            if(ckup->data.processStep == 3) {
                checkup_renderCheckFilm(processToCheckup);
            }


            if(ckup->data.processStep == 4) {
                checkup_renderProcessing(processToCheckup);
            }
}