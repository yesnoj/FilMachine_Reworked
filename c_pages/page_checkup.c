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


  /* ── Button clicks: use processStep to disambiguate reused widgets ── */
  if(code == LV_EVENT_CLICKED){
    if(widget == ckup->checkupStartButton){
      alarm_stop();
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
      alarm_stop();
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

        /* Stop persistent alarm */
        alarm_stop();

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
        alarm_stop();
        messagePopupCreate(warningPopupTitle_text,stopAfterProcessPopupBody_text,checkupStop_text,stepDetailCancel_text, ckup->checkupStopAfterButton);
    }
    if(widget == ckup->checkupStopNowButton){
        LV_LOG_USER("User pressed checkupStopNowButton");
        alarm_stop();
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


    /* Get overlap settings */
    uint8_t overlapPct = gui.page.settings.settingsParams.drainFillOverlapSetpoint;
    uint32_t overlapPerPhase = (uint32_t)tankFillTime * overlapPct / 100;
    uint32_t totalOverlapPerStep = 2 * overlapPerPhase; /* fill + drain */

    /* Adjusted total process time */
    uint32_t origProcSecs = (uint32_t)pn->process.processDetails->data.timeMins * 60
                           + pn->process.processDetails->data.timeSecs;
    uint32_t numSteps = pn->process.processDetails->stepElementsList.size;
    uint32_t totalAdjustment = numSteps * totalOverlapPerStep;
    uint32_t adjProcSecs = (origProcSecs > totalAdjustment) ? (origProcSecs - totalAdjustment) : 1;
    uint8_t adjProcMins = adjProcSecs / 60;
    uint8_t adjProcRemSecs = adjProcSecs % 60;

    processPercentage = calculatePercentage(minutesProcessElapsed, secondsProcessElapsed, adjProcMins, adjProcRemSecs);
    LV_LOG_USER("Elapsed Time %"PRIu8"h:%"PRIu32"m:%"PRIu8"s, processPercentage %"PRIu8" stepPercentage %"PRIu8"", hoursProcessElapsed, minutesProcessElapsed, secondsProcessElapsed, processPercentage, stepPercentage);

    // Convert the remaining process time to minutes and seconds (using adjusted values)
    uint8_t totalProcessSecs = adjProcSecs;
    uint8_t elapsedProcessSecs = minutesProcessElapsed * 60 + secondsProcessElapsed;
    uint8_t remainingProcessSecs = (totalProcessSecs > elapsedProcessSecs) ? (totalProcessSecs - elapsedProcessSecs) : 0;

    uint8_t remainingProcessMins = remainingProcessSecs / 60;
    uint8_t remainingProcessSecsOnly = remainingProcessSecs % 60;

    /* Compute adjusted step time for remaining time display */
    uint32_t origStepSecs = 0;
    uint32_t adjStepSecs = 0;
    if(ckup->currentStep != NULL) {
        origStepSecs = (uint32_t)ckup->currentStep->step.stepDetails->data.timeMins * 60
                      + ckup->currentStep->step.stepDetails->data.timeSecs;
        adjStepSecs = (origStepSecs > totalOverlapPerStep) ? (origStepSecs - totalOverlapPerStep) : 0;
    }

    // Convert the remaining step time to minutes and seconds (using adjusted values)
    uint8_t totalStepSecs = adjStepSecs;
    uint8_t elapsedStepSecs = minutesStepElapsed * 60 + secondsStepElapsed;
    uint8_t remainingStepSecs = (totalStepSecs > elapsedStepSecs) ? (totalStepSecs - elapsedStepSecs) : 0;

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
          /* Compute adjusted step time for percentage calculation */
          origStepSecs = (uint32_t)ckup->currentStep->step.stepDetails->data.timeMins * 60
                        + ckup->currentStep->step.stepDetails->data.timeSecs;
          adjStepSecs = (origStepSecs > totalOverlapPerStep) ? (origStepSecs - totalOverlapPerStep) : 0;
          uint8_t adjStepMins = adjStepSecs / 60;
          uint8_t adjStepRemSecs = adjStepSecs % 60;

          stepPercentage = calculatePercentage(minutesStepElapsed, secondsStepElapsed, adjStepMins, adjStepRemSecs);
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

          /* Start persistent alarm for process completion */
          alarm_start_persistent();

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

        /* Start persistent alarm for temperature reached (if temp control enabled) */
        alarm_start_persistent();

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
                lv_obj_set_width(ckup->checkupSkipButton, ui_get_profile()->checkup.continue_button_w);
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
    const ui_checkup_layout_t *ui = &ui_get_profile()->checkup;
    /* processStep 0: Tank size and chemistry volume selection */
    sCheckup *ckup = proc->process.processDetails->checkup;
    if(isStepStatus0created == 0) {
        ckup->checkupSelectTankChemistryContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupSelectTankChemistryContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupSelectTankChemistryContainer, LV_ALIGN_TOP_LEFT, ui->stage_panel_x, ui->stage_panel_y);
        lv_obj_set_size(ckup->checkupSelectTankChemistryContainer, ui_get_profile()->checkup.stage_panel_w, ui_get_profile()->checkup.stage_panel_h);
        lv_obj_set_style_border_opa(ckup->checkupSelectTankChemistryContainer, LV_OPA_TRANSP, 0);
        lv_obj_add_event_cb(ckup->checkupSelectTankChemistryContainer, event_checkup, LV_EVENT_CLICKED, proc);

        ckup->checkupProcessReadyLabel = lv_label_create(ckup->checkupSelectTankChemistryContainer);
        lv_label_set_text(ckup->checkupProcessReadyLabel, checkupProcessReady_text);
        lv_obj_set_width(ckup->checkupProcessReadyLabel, ui->stage_title_w);
        lv_obj_set_style_text_font(ckup->checkupProcessReadyLabel, ui->stage_title_font, 0);
        lv_obj_align(ckup->checkupProcessReadyLabel, LV_ALIGN_TOP_LEFT, ui->stage_title_x, ui->stage_title_y);

        ckup->checkupTankSizeLabel = lv_label_create(ckup->checkupSelectTankChemistryContainer);
        lv_label_set_text(ckup->checkupTankSizeLabel, checkupTankSize_text);
        lv_obj_set_width(ckup->checkupTankSizeLabel, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(ckup->checkupTankSizeLabel, ui->stage_label_font, 0);
        lv_obj_align(ckup->checkupTankSizeLabel, LV_ALIGN_TOP_MID, 0, ui->tank_size_label_y);

        ckup->checkupTankSizeTextArea = lv_textarea_create(ckup->checkupSelectTankChemistryContainer);
        lv_textarea_set_one_line(ckup->checkupTankSizeTextArea, true);
        lv_textarea_set_placeholder_text(ckup->checkupTankSizeTextArea, checkupTankSizePlaceHolder_text);
        lv_obj_align(ckup->checkupTankSizeTextArea, LV_ALIGN_TOP_MID, 0, ui->stage_textarea_y1);
        lv_obj_set_width(ckup->checkupTankSizeTextArea, ui->stage_textarea_w);

        lv_obj_set_style_bg_color(ckup->checkupTankSizeTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_set_style_text_align(ckup->checkupTankSizeTextArea, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_add_style(ckup->checkupTankSizeTextArea, &ckup->textAreaStyleCheckup, LV_PART_MAIN);
        lv_obj_set_style_border_color(ckup->checkupTankSizeTextArea, lv_color_hex(WHITE), 0);
        lv_obj_remove_flag(ckup->checkupTankSizeTextArea, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(ckup->checkupTankSizeTextArea, lv_palette_darken(LV_PALETTE_GREY, 1), 0);

        /* Always read tank size from settings (persistent) and sync to checkup data */
        {
            const char *sizes[] = tankSizeValues;
            uint8_t idx = gui.page.settings.settingsParams.tankSize;
            if(idx < 1 || idx > 3) idx = 2;
            ckup->data.tankSize = idx;
            lv_textarea_set_text(ckup->checkupTankSizeTextArea, sizes[idx - 1]);
        }

        ckup->checkupChemistryVolumeLabel = lv_label_create(ckup->checkupSelectTankChemistryContainer);
        lv_label_set_text(ckup->checkupChemistryVolumeLabel, checkupChemistryVolume_text);
        lv_obj_set_width(ckup->checkupChemistryVolumeLabel, LV_SIZE_CONTENT);
        lv_obj_set_style_text_font(ckup->checkupChemistryVolumeLabel, ui->stage_label_font, 0);
        lv_obj_align(ckup->checkupChemistryVolumeLabel, LV_ALIGN_TOP_MID, 0, ui->chem_volume_label_y);

        /* Read-only volume textarea */
        ckup->checkupVolumeTextArea = lv_textarea_create(ckup->checkupSelectTankChemistryContainer);
        lv_textarea_set_one_line(ckup->checkupVolumeTextArea, true);
        lv_obj_align(ckup->checkupVolumeTextArea, LV_ALIGN_TOP_MID, 0, ui->stage_textarea_y2);
        lv_obj_set_width(ckup->checkupVolumeTextArea, ui->stage_textarea_w);
        lv_obj_set_style_bg_color(ckup->checkupVolumeTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_set_style_text_align(ckup->checkupVolumeTextArea, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_add_style(ckup->checkupVolumeTextArea, &ckup->textAreaStyleCheckup, LV_PART_MAIN);
        lv_obj_set_style_border_color(ckup->checkupVolumeTextArea, lv_color_hex(WHITE), 0);
        lv_obj_remove_flag(ckup->checkupVolumeTextArea, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(ckup->checkupVolumeTextArea, lv_palette_darken(LV_PALETTE_GREY, 1), 0);

        /* Always read volume from settings and sync to checkup data */
        {
            const char *vols[] = chemVolumeValues;
            uint8_t v = gui.page.settings.settingsParams.chemistryVolume;
            if(v < 1 || v > 2) v = 2;
            ckup->data.activeVolume_index = v;
            lv_textarea_set_text(ckup->checkupVolumeTextArea, vols[v - 1]);
        }

        ckup->checkupStartButton = lv_button_create(ckup->checkupSelectTankChemistryContainer);
        lv_obj_set_size(ckup->checkupStartButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
        lv_obj_align(ckup->checkupStartButton, LV_ALIGN_BOTTOM_MID, 0, ui->stage_button_y);
        lv_obj_add_event_cb(ckup->checkupStartButton, event_checkup, LV_EVENT_CLICKED, proc);
        lv_obj_set_style_bg_color(ckup->checkupStartButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->checkupStartButton);

        ckup->checkupStartButtonLabel = lv_label_create(ckup->checkupStartButton);
        lv_label_set_text(ckup->checkupStartButtonLabel, checkupStart_text);
        lv_obj_set_style_text_font(ckup->checkupStartButtonLabel, ui->stage_title_font, 0);
        lv_obj_align(ckup->checkupStartButtonLabel, LV_ALIGN_CENTER, 0, 0);
        /* Enable Start immediately if tank/volume are already set from settings */
        if (ckup->data.tankSize > 0 && ckup->data.activeVolume_index > 0 && ckup->data.processStep < 1) {
            lv_obj_clear_state(ckup->checkupStartButton, LV_STATE_DISABLED);
            LV_LOG_USER("checkup: Start button ENABLED (tank=%d, vol=%lu)", ckup->data.tankSize, (unsigned long)ckup->data.activeVolume_index);
        } else {
            lv_obj_add_state(ckup->checkupStartButton, LV_STATE_DISABLED);
        }

        isStepStatus0created = 1;
    }
}

static void checkup_renderFillWater(processNode *proc) {
    const ui_checkup_layout_t *ui = &ui_get_profile()->checkup;
    /* processStep 1: Fill water */
    sCheckup *ckup = proc->process.processDetails->checkup;
    if(isStepStatus1created == 0) {
        lv_obj_clean(ckup->checkupStepContainer);
        ckup->checkupFillWaterContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupFillWaterContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupFillWaterContainer, LV_ALIGN_TOP_LEFT, ui->stage_panel_x, ui->stage_panel_y);
        lv_obj_set_size(ckup->checkupFillWaterContainer, ui_get_profile()->checkup.stage_panel_w, ui_get_profile()->checkup.stage_panel_h);
        lv_obj_set_style_border_opa(ckup->checkupFillWaterContainer, LV_OPA_TRANSP, 0);

        ckup->checkupFillWaterLabel = lv_label_create(ckup->checkupFillWaterContainer);
        lv_label_set_text(ckup->checkupFillWaterLabel, checkupFillWaterMachine_text);
        lv_obj_set_style_text_font(ckup->checkupFillWaterLabel, ui->stage_label_font, 0);
        lv_obj_align(ckup->checkupFillWaterLabel, LV_ALIGN_CENTER, 0, ui->fill_label_y);
        lv_obj_set_style_text_align(ckup->checkupFillWaterLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(ckup->checkupFillWaterLabel, LV_LABEL_LONG_WRAP);
        lv_obj_set_size(ckup->checkupFillWaterLabel, ui->fill_label_w, LV_SIZE_CONTENT);

        ckup->checkupSkipButton = lv_button_create(ckup->checkupFillWaterContainer);
        lv_obj_set_size(ckup->checkupSkipButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
        lv_obj_align(ckup->checkupSkipButton, LV_ALIGN_BOTTOM_MID, 0, ui->stage_button_y);
        lv_obj_add_event_cb(ckup->checkupSkipButton, event_checkup, LV_EVENT_CLICKED, proc);
        lv_obj_set_style_bg_color(ckup->checkupSkipButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->checkupSkipButton);

        ckup->checkupSkipButtonLabel = lv_label_create(ckup->checkupSkipButton);
        lv_label_set_text(ckup->checkupSkipButtonLabel, checkupSkip_text);
        lv_obj_set_style_text_font(ckup->checkupSkipButtonLabel, ui->stage_title_font, 0);
        lv_obj_align(ckup->checkupSkipButtonLabel, LV_ALIGN_CENTER, 0, 0);

        isStepStatus1created = 1;
    }
}

static void checkup_renderReachTemp(processNode *proc) {
    const ui_checkup_layout_t *ui = &ui_get_profile()->checkup;
    /* processStep 2: Reach temperature */
    sCheckup *ckup = proc->process.processDetails->checkup;
    if(isStepStatus2created == 0) {
        lv_obj_clean(ckup->checkupStepContainer);
        ckup->checkupTargetTempsContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupTargetTempsContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTargetTempsContainer, LV_ALIGN_TOP_LEFT, ui->stage_panel_x, ui->stage_panel_y);
        lv_obj_set_size(ckup->checkupTargetTempsContainer, ui_get_profile()->checkup.stage_panel_w, ui_get_profile()->checkup.stage_panel_h);
        lv_obj_set_style_border_opa(ckup->checkupTargetTempsContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTargetTempContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupTargetTempContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTargetTempContainer, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_size(ckup->checkupTargetTempContainer, ui->target_title_w, ui->target_title_h);
        lv_obj_set_style_border_opa(ckup->checkupTargetTempContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTargetTempLabel = lv_label_create(ckup->checkupTargetTempContainer);
        lv_label_set_text(ckup->checkupTargetTempLabel, checkupTargetTemp_text);
        lv_obj_set_style_text_font(ckup->checkupTargetTempLabel, ui->target_title_font, 0);
        lv_obj_align(ckup->checkupTargetTempLabel, LV_ALIGN_CENTER, 0, ui->target_title_y);

        ckup->checkupTargetTempValue = lv_label_create(ckup->checkupTargetTempContainer);

        if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
            lv_label_set_text_fmt(ckup->checkupTargetTempValue, "%"PRIi32"°C", proc->process.processDetails->data.temp);
        } else{
            lv_label_set_text_fmt(ckup->checkupTargetTempValue, "%"PRIi32"°F", convertCelsiusToFahrenheit(proc->process.processDetails->data.temp));
        }

        lv_obj_set_style_text_font(ckup->checkupTargetTempValue, ui->target_value_font, 0);
        lv_obj_align(ckup->checkupTargetTempValue, LV_ALIGN_CENTER, 0, ui->target_title_value_y);

        ckup->checkupTargetToleranceTempValue = lv_label_create(ckup->checkupTargetTempContainer);
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%s ~%.1f", checkupTargetToleranceTemp_text, proc->process.processDetails->data.tempTolerance);
            lv_label_set_text(ckup->checkupTargetToleranceTempValue, buf);
        }
        lv_obj_set_style_text_font(ckup->checkupTargetToleranceTempValue, ui->target_title_font, 0);
        lv_obj_align(ckup->checkupTargetToleranceTempValue, LV_ALIGN_CENTER, 0, ui->target_tolerance_y);

        ckup->checkupTargetWaterTempContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupTargetWaterTempContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTargetWaterTempContainer, LV_ALIGN_BOTTOM_LEFT, ui->target_temp_left_x, ui->target_temp_y);
        lv_obj_set_size(ckup->checkupTargetWaterTempContainer, ui->target_temp_w, ui->target_temp_h);
        lv_obj_set_style_border_opa(ckup->checkupTargetWaterTempContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTargetWaterTempLabel = lv_label_create(ckup->checkupTargetWaterTempContainer);
        lv_label_set_text(ckup->checkupTargetWaterTempLabel, checkupWater_text);
        lv_obj_set_style_text_font(ckup->checkupTargetWaterTempLabel, ui->stage_label_font, 0);
        lv_obj_align(ckup->checkupTargetWaterTempLabel, LV_ALIGN_CENTER, 0, ui->target_temp_label_y);

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

        lv_obj_set_style_text_font(ckup->checkupTargetWaterTempValue, ui->stage_value_font, 0);
        lv_obj_align(ckup->checkupTargetWaterTempValue, LV_ALIGN_CENTER, 0, ui->target_temp_value_y);

        ckup->checkupTargetChemistryTempContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupTargetChemistryTempContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTargetChemistryTempContainer, LV_ALIGN_BOTTOM_RIGHT, ui->target_temp_right_x, ui->target_temp_y);
        lv_obj_set_size(ckup->checkupTargetChemistryTempContainer, ui->target_temp_w, ui->target_temp_h);
        lv_obj_set_style_border_opa(ckup->checkupTargetChemistryTempContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTargetChemistryTempLabel = lv_label_create(ckup->checkupTargetChemistryTempContainer);
        lv_label_set_text(ckup->checkupTargetChemistryTempLabel, checkupChemistry_text);
        lv_obj_set_style_text_font(ckup->checkupTargetChemistryTempLabel, ui->stage_label_font, 0);
        lv_obj_align(ckup->checkupTargetChemistryTempLabel, LV_ALIGN_CENTER, 0, ui->target_temp_label_y);

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

        lv_obj_set_style_text_font(ckup->checkupTargetChemistryTempValue, ui->stage_value_font, 0);
        lv_obj_align(ckup->checkupTargetChemistryTempValue, LV_ALIGN_CENTER, 0, ui->target_temp_value_y);

        ckup->checkupSkipButton = lv_button_create(ckup->checkupTargetTempsContainer);
        lv_obj_set_size(ckup->checkupSkipButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
        lv_obj_align(ckup->checkupSkipButton, LV_ALIGN_BOTTOM_MID, 0, ui->stage_button_y);
        lv_obj_add_event_cb(ckup->checkupSkipButton, event_checkup, LV_EVENT_CLICKED, proc);
        lv_obj_set_style_bg_color(ckup->checkupSkipButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->checkupSkipButton);

        ckup->checkupSkipButtonLabel = lv_label_create(ckup->checkupSkipButton);
        lv_label_set_text(ckup->checkupSkipButtonLabel, checkupSkip_text);
        lv_obj_set_style_text_font(ckup->checkupSkipButtonLabel, ui->stage_title_font, 0);
        lv_obj_align(ckup->checkupSkipButtonLabel, LV_ALIGN_CENTER, 0, 0);

        ckup->checkupHeaterStatusLabel = lv_label_create(ckup->checkupTargetTempsContainer);
        lv_label_set_text_fmt(ckup->checkupHeaterStatusLabel, checkupHeaterStatusFmt_text, checkupHeaterOff_text);
        lv_obj_set_style_text_font(ckup->checkupHeaterStatusLabel, ui->small_font, 0);
        lv_obj_align(ckup->checkupHeaterStatusLabel, LV_ALIGN_BOTTOM_MID, 0, ui->heater_status_y);

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
    const ui_checkup_layout_t *ui = &ui_get_profile()->checkup;
    /* processStep 3: Check film/tank */
    sCheckup *ckup = proc->process.processDetails->checkup;
    if(isStepStatus3created == 0) {
        lv_obj_clean(ckup->checkupStepContainer);
        ckup->checkupFilmRotatingContainer = lv_obj_create(ckup->checkupStepContainer);
        lv_obj_remove_flag(ckup->checkupFilmRotatingContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupFilmRotatingContainer, LV_ALIGN_TOP_LEFT, ui->stage_panel_x, ui->stage_panel_y);
        lv_obj_set_size(ckup->checkupFilmRotatingContainer, ui_get_profile()->checkup.stage_panel_w, ui_get_profile()->checkup.stage_panel_h);
        lv_obj_set_style_border_opa(ckup->checkupFilmRotatingContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTankIsPresentContainer = lv_obj_create(ckup->checkupFilmRotatingContainer);
        lv_obj_remove_flag(ckup->checkupTankIsPresentContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupTankIsPresentContainer, LV_ALIGN_CENTER, 0, ui->film_box1_y);
        lv_obj_set_size(ckup->checkupTankIsPresentContainer, ui->film_box_w, ui->film_box_h);
        lv_obj_set_style_border_opa(ckup->checkupTankIsPresentContainer, LV_OPA_TRANSP, 0);

        ckup->checkupTankIsPresentLabel = lv_label_create(ckup->checkupTankIsPresentContainer);
        lv_label_set_text(ckup->checkupTankIsPresentLabel, checkupTankPosition_text);
        lv_obj_set_style_text_font(ckup->checkupTankIsPresentLabel, ui->stage_value_font, 0);
        lv_obj_align(ckup->checkupTankIsPresentLabel, LV_ALIGN_CENTER, 0, ui->film_label_y);

        ckup->checkupTankIsPresentValue = lv_label_create(ckup->checkupTankIsPresentContainer);
        lv_label_set_text(ckup->checkupTankIsPresentValue, checkupYes_text);
        lv_obj_set_style_text_font(ckup->checkupTankIsPresentValue, ui->process_name_font, 0);
        lv_obj_align(ckup->checkupTankIsPresentValue, LV_ALIGN_CENTER, 0, ui->film_value_y);

        ckup->checkupFilmInPositionContainer = lv_obj_create(ckup->checkupFilmRotatingContainer);
        lv_obj_remove_flag(ckup->checkupFilmInPositionContainer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(ckup->checkupFilmInPositionContainer, LV_ALIGN_CENTER, 0, ui->film_box2_y);
        lv_obj_set_size(ckup->checkupFilmInPositionContainer, ui->film_box_w, ui->film_box_h);
        lv_obj_set_style_border_opa(ckup->checkupFilmInPositionContainer, LV_OPA_TRANSP, 0);

        ckup->checkupFilmRotatingLabel = lv_label_create(ckup->checkupFilmInPositionContainer);
        lv_label_set_text(ckup->checkupFilmRotatingLabel, checkupFilmRotation_text);
        lv_obj_set_style_text_font(ckup->checkupFilmRotatingLabel, ui->stage_value_font, 0);
        lv_obj_align(ckup->checkupFilmRotatingLabel, LV_ALIGN_CENTER, 0, ui->film_label_y);

        ckup->checkupFilmRotatingValue = lv_label_create(ckup->checkupFilmInPositionContainer);
        lv_label_set_text(ckup->checkupFilmRotatingValue, checkupChecking_text);
        lv_obj_set_style_text_font(ckup->checkupFilmRotatingValue, ui->process_name_font, 0);
        lv_obj_align(ckup->checkupFilmRotatingValue, LV_ALIGN_CENTER, 0, ui->film_value_y);

        ckup->checkupStartButton = lv_button_create(ckup->checkupFilmRotatingContainer);
        lv_obj_set_size(ckup->checkupStartButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
        lv_obj_align(ckup->checkupStartButton, LV_ALIGN_BOTTOM_MID, 0, ui->stage_button_y);
        lv_obj_add_event_cb(ckup->checkupStartButton, event_checkup, LV_EVENT_CLICKED, proc);
        lv_obj_set_style_bg_color(ckup->checkupStartButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->checkupStartButton);

        ckup->checkupStartButtonLabel = lv_label_create(ckup->checkupStartButton);
        lv_label_set_text(ckup->checkupStartButtonLabel, checkupStart_text);
        lv_obj_set_style_text_font(ckup->checkupStartButtonLabel, ui->stage_title_font, 0);
        lv_obj_align(ckup->checkupStartButtonLabel, LV_ALIGN_CENTER, 0, 0);

        isStepStatus3created = 1;
    }
}

static void checkup_renderProcessing(processNode *proc) {
    const ui_checkup_layout_t *ui = &ui_get_profile()->checkup;
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
        lv_obj_align(ckup->checkupProcessingContainer, LV_ALIGN_TOP_LEFT, ui->processing_container_x, ui->processing_container_y);
        lv_obj_set_size(ckup->checkupProcessingContainer, ui->processing_container_w, ui->processing_container_h);
        lv_obj_set_style_border_opa(ckup->checkupProcessingContainer, LV_OPA_TRANSP, 0);

        ckup->processArc = lv_arc_create(ckup->checkupProcessingContainer);
        lv_obj_set_size(ckup->processArc, ui->processing_arc_size, ui->processing_arc_size);
        lv_arc_set_rotation(ckup->processArc, 140);
        lv_arc_set_bg_angles(ckup->processArc, 0, 260);
        lv_arc_set_value(ckup->processArc, 0);
        lv_arc_set_range(ckup->processArc, 0, 100);
        lv_obj_align(ckup->processArc, LV_ALIGN_CENTER, 0, ui->processing_arc_center_y);
        lv_obj_remove_style(ckup->processArc, NULL, LV_PART_KNOB);
        lv_obj_remove_flag(ckup->processArc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_color(ckup->processArc,lv_color_hex(GREEN) , LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(ckup->processArc, lv_color_hex(GREEN_DARK), LV_PART_MAIN);

        ckup->checkupProcessTimeLeftValue = lv_label_create(ckup->checkupProcessingContainer);
        lv_label_set_text_fmt(ckup->checkupProcessTimeLeftValue, "%"PRIu32"m%"PRIu8"s",
          proc->process.processDetails->data.timeMins, proc->process.processDetails->data.timeSecs);
        lv_obj_set_style_text_font(ckup->checkupProcessTimeLeftValue, ui->target_value_font, 0);
        lv_obj_align(ckup->checkupProcessTimeLeftValue, LV_ALIGN_CENTER, 0, ui->processing_time_y);

        ckup->checkupStepNameValue = lv_label_create(ckup->checkupProcessingContainer);
        lv_label_set_text(ckup->checkupStepNameValue, "...");
        lv_obj_set_style_text_align(ckup->checkupStepNameValue, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(ckup->checkupStepNameValue, ui->stage_title_font, 0);
        lv_obj_align(ckup->checkupStepNameValue, LV_ALIGN_CENTER, 0, ui->processing_step_name_y);
        lv_obj_set_width(ckup->checkupStepNameValue, ui->processing_step_name_w);
        lv_label_set_long_mode(ckup->checkupStepNameValue, LV_LABEL_LONG_SCROLL_CIRCULAR);

        ckup->checkupStepTimeLeftValue = lv_label_create(ckup->checkupProcessingContainer);
        lv_label_set_text_fmt(ckup->checkupStepTimeLeftValue, "%dm%ds",
            proc->process.processDetails->stepElementsList.start->step.stepDetails->data.timeMins,
            proc->process.processDetails->stepElementsList.start->step.stepDetails->data.timeSecs);
        lv_obj_set_style_text_font(ckup->checkupStepTimeLeftValue, ui->stage_title_font, 0);
        lv_obj_align(ckup->checkupStepTimeLeftValue, LV_ALIGN_CENTER, 0, ui->processing_step_time_y);

        ckup->stepArc = lv_arc_create(ckup->checkupProcessingContainer);
        lv_obj_set_size(ckup->stepArc, ui->processing_step_arc_size, ui->processing_step_arc_size);
        lv_arc_set_rotation(ckup->stepArc, 230);
        lv_arc_set_bg_angles(ckup->stepArc, 0, 80);
        lv_arc_set_value(ckup->stepArc, 0);
        lv_arc_set_range(ckup->stepArc, 0, 100);
        lv_obj_align(ckup->stepArc, LV_ALIGN_CENTER, 0, ui->processing_step_arc_center_y);
        lv_obj_remove_style(ckup->stepArc, NULL, LV_PART_KNOB);
        lv_obj_remove_flag(ckup->stepArc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_color(ckup->stepArc,lv_color_hex(ORANGE_LIGHT) , LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(ckup->stepArc, lv_color_hex(ORANGE_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->stepArc);

        ckup->checkupStepKindValue = lv_label_create(ckup->checkupProcessingContainer);
        lv_label_set_text(ckup->checkupStepKindValue, checkupFilling_text);
        lv_obj_set_style_text_font(ckup->checkupStepKindValue, ui->stage_value_font, 0);
        lv_obj_align(ckup->checkupStepKindValue, LV_ALIGN_CENTER, 0, ui->processing_kind_y);
        lv_obj_add_flag(ckup->checkupStepKindValue, LV_OBJ_FLAG_HIDDEN);

        ckup->pumpArc = lv_arc_create(ckup->checkupProcessingContainer);
        lv_obj_set_size(ckup->pumpArc, ui->processing_pump_arc_size, ui->processing_pump_arc_size);
        lv_arc_set_rotation(ckup->pumpArc, 48);
        lv_arc_set_bg_angles(ckup->pumpArc, 0, 84);
        lv_arc_set_range(ckup->pumpArc, 0, 100);
        lv_obj_align(ckup->pumpArc, LV_ALIGN_CENTER, 0, ui->processing_arc_center_y);
        lv_obj_remove_style(ckup->pumpArc, NULL, LV_PART_KNOB);
        lv_obj_remove_flag(ckup->pumpArc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_arc_color(ckup->pumpArc,lv_color_hex(LIGHT_BLUE) , LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(ckup->pumpArc, lv_color_hex(BLUE_DARK), LV_PART_MAIN);
        lv_obj_move_foreground(ckup->pumpArc);
        lv_arc_set_mode(ckup->pumpArc, LV_ARC_MODE_REVERSE);

        ckup->checkupProcessCompleteLabel = lv_label_create(ckup->checkupProcessingContainer);
        lv_obj_set_style_text_align(ckup->checkupProcessCompleteLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(ckup->checkupProcessCompleteLabel, ui->stage_title_font, 0);
        lv_obj_align(ckup->checkupProcessCompleteLabel, LV_ALIGN_CENTER, 0, ui->processing_complete_y);
        lv_obj_add_flag(ckup->checkupProcessCompleteLabel, LV_OBJ_FLAG_HIDDEN);

        isStepStatus4created = 1;
    }
}

void initCheckup(processNode *pn)
{
      sCheckup *ckup = pn->process.processDetails->checkup;
      const ui_checkup_layout_t *ui = &ui_get_profile()->checkup;
      LV_LOG_USER("Final checks, current on ckup->data.processStep :%d",ckup->data.processStep);

      //in this way processDetail is deleted, to free memory and the "checkout" is a standard screen on top
      lv_obj_del(pn->process.processDetails->processDetailParent);
      ckup->checkupParent = lv_obj_create(NULL);
      lv_scr_load(ckup->checkupParent);

      ckup->currentStep = pn->process.processDetails->stepElementsList.start;
      
      //in this way create a new layer on top of others, so "checkout" will be on top of processDetail
      //ckup->checkupParent = lv_obj_class_create_obj(&lv_msgbox_backdrop_class, lv_layer_top());
      
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
            lv_obj_set_size(ckup->checkupCloseButton, ui->close_w, ui->close_h);
            lv_obj_align(ckup->checkupCloseButton, LV_ALIGN_TOP_RIGHT, ui->close_x , ui->close_y);
            lv_obj_add_event_cb(ckup->checkupCloseButton, event_checkup, LV_EVENT_CLICKED, pn);
            lv_obj_set_style_bg_color(ckup->checkupCloseButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
            if(ckup->data.processStep > 0){
              lv_obj_add_state(ckup->checkupCloseButton, LV_STATE_DISABLED);            
            }

                  ckup->checkupCloseButtonLabel = lv_label_create(ckup->checkupCloseButton);         
                  lv_label_set_text(ckup->checkupCloseButtonLabel, closePopup_icon); 
                  lv_obj_set_style_text_font(ckup->checkupCloseButtonLabel, ui->close_icon_font, 0);              
                  lv_obj_align(ckup->checkupCloseButtonLabel, LV_ALIGN_CENTER, 0, 0);


            ckup->checkupProcessNameContainer = lv_obj_create(ckup->checkupContainer);
            lv_obj_remove_flag(ckup->checkupProcessNameContainer, LV_OBJ_FLAG_SCROLLABLE); 
            lv_obj_align(ckup->checkupProcessNameContainer, LV_ALIGN_TOP_LEFT, ui->process_name_x, ui->process_name_y);
            lv_obj_set_size(ckup->checkupProcessNameContainer, ui->process_name_w, ui->process_name_h);
            lv_obj_set_style_border_opa(ckup->checkupProcessNameContainer, LV_OPA_TRANSP, 0);

                  ckup->checkupProcessNameValue = lv_label_create(ckup->checkupProcessNameContainer);
                  lv_label_set_text(ckup->checkupProcessNameValue, pn->process.processDetails->data.processNameString);
                  lv_obj_set_width(ckup->checkupProcessNameValue, ui->process_name_label_w);
                  lv_obj_set_style_text_font(ckup->checkupProcessNameValue, ui->process_name_font, 0);              
                  lv_obj_align(ckup->checkupProcessNameValue, LV_ALIGN_TOP_LEFT, ui->process_name_label_x, ui->process_name_label_y);
                  lv_label_set_long_mode(ckup->checkupProcessNameValue, LV_LABEL_LONG_SCROLL_CIRCULAR);

            //RIGHT GREEN CONTAINER
            ckup->checkupStepContainer = lv_obj_create(ckup->checkupContainer);
            lv_obj_remove_flag(ckup->checkupStepContainer, LV_OBJ_FLAG_SCROLLABLE); 
            lv_obj_align(ckup->checkupStepContainer, LV_ALIGN_TOP_LEFT, ui->step_panel_x, ui->step_panel_y);
            lv_obj_set_size(ckup->checkupStepContainer, ui->step_panel_w, ui->step_panel_h);
            lv_obj_set_style_border_color(ckup->checkupStepContainer, lv_palette_main(LV_PALETTE_GREEN), 0);

            //LEFT WHITE CONTAINER
            ckup->checkupNextStepsContainer = lv_obj_create(ckup->checkupContainer);
            lv_obj_remove_flag(ckup->checkupNextStepsContainer, LV_OBJ_FLAG_SCROLLABLE); 
            lv_obj_align(ckup->checkupNextStepsContainer, LV_ALIGN_TOP_LEFT, ui->left_panel_x, ui->left_panel_y);
            lv_obj_set_size(ckup->checkupNextStepsContainer, ui->left_panel_w, ui->left_panel_h);
            lv_obj_set_style_border_color(ckup->checkupNextStepsContainer, lv_color_hex(WHITE), 0);
}

void checkup(processNode *processToCheckup) {

	const ui_checkup_layout_t *ui = &ui_get_profile()->checkup;
	char *tmp_checkupStepStatuses[] = checkupStepStatuses;
	char *tmp_processSourceList[] = processSourceList;
	char *tmp_processTempControlList[] = processTempControlList;

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
    /* Initialize from settings so Start is active immediately */
    ckup->data.tankSize            = gui.page.settings.settingsParams.tankSize;
    ckup->data.activeVolume_index  = gui.page.settings.settingsParams.chemistryVolume;
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
                  lv_obj_set_style_text_font(ckup->checkupNextStepsLabel, ui->left_title_font, 0);              
                  lv_obj_align(ckup->checkupNextStepsLabel, LV_ALIGN_TOP_LEFT, ui->left_title_x, ui->left_title_y);

                  ckup->checkupMachineWillDoLabel = lv_label_create(ckup->checkupNextStepsContainer);
                  lv_label_set_text(ckup->checkupMachineWillDoLabel, checkupTheMachineWillDo_text); 
                  lv_obj_set_width(ckup->checkupMachineWillDoLabel, LV_SIZE_CONTENT);
                  lv_obj_set_style_text_font(ckup->checkupMachineWillDoLabel, ui->left_body_font, 0);              
                  lv_obj_align(ckup->checkupMachineWillDoLabel, LV_ALIGN_TOP_LEFT, ui->left_title_x, ui->left_body_intro_y);

                  ckup->checkupWaterFillContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupWaterFillContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupWaterFillContainer, LV_ALIGN_TOP_LEFT, ui->left_row_x, ui->left_row1_y);
                  lv_obj_set_size(ckup->checkupWaterFillContainer, ui->left_row_w, ui->left_row_h);
                  lv_obj_set_style_border_opa(ckup->checkupWaterFillContainer, LV_OPA_TRANSP, 0);

                  
                          ckup->checkupWaterFillStatusIcon = lv_label_create(ckup->checkupWaterFillContainer);         
                          lv_label_set_text(ckup->checkupWaterFillStatusIcon, tmp_checkupStepStatuses[ckup->data.stepFillWaterStatus]); 
                          lv_obj_set_style_text_font(ckup->checkupWaterFillStatusIcon, ui->status_icon_font, 0);              
                          lv_obj_align(ckup->checkupWaterFillStatusIcon, LV_ALIGN_LEFT_MID, ui->left_status_icon_x, 0);

                          ckup->checkupWaterFillLabel = lv_label_create(ckup->checkupWaterFillContainer);         
                          lv_label_set_text(ckup->checkupWaterFillLabel, checkupFillWater_text); 
                          lv_obj_set_width(ckup->checkupWaterFillLabel, ui->left_status_label_w);
                          lv_obj_set_style_text_font(ckup->checkupWaterFillLabel, ui->left_body_font, 0);              
                          lv_obj_align(ckup->checkupWaterFillLabel, LV_ALIGN_LEFT_MID, ui->left_status_label_x, 0);
                          lv_label_set_long_mode(ckup->checkupWaterFillLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);


                  ckup->checkupReachTempContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupReachTempContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupReachTempContainer, LV_ALIGN_TOP_LEFT, ui->left_row_x, ui->left_row2_y);
                  lv_obj_set_size(ckup->checkupReachTempContainer, ui->left_row_w, ui->left_row_h);
                  lv_obj_set_style_border_opa(ckup->checkupReachTempContainer, LV_OPA_TRANSP, 0);

                          ckup->checkupReachTempStatusIcon = lv_label_create(ckup->checkupReachTempContainer);
                          lv_obj_set_style_text_font(ckup->checkupReachTempStatusIcon, ui->status_icon_font, 0);              
                          lv_obj_align(ckup->checkupReachTempStatusIcon, LV_ALIGN_LEFT_MID, ui->left_status_icon_x, 0);


                          ckup->checkupReachTempLabel = lv_label_create(ckup->checkupReachTempContainer);         
                          lv_label_set_text(ckup->checkupReachTempLabel, checkupReachTemp_text); 
                          lv_obj_set_width(ckup->checkupReachTempLabel, ui->left_status_label_w);
                          lv_obj_set_style_text_font(ckup->checkupReachTempLabel, ui->left_body_font, 0);              
                          lv_obj_align(ckup->checkupReachTempLabel, LV_ALIGN_LEFT_MID, ui->left_status_label_x, 0);
                          lv_label_set_long_mode(ckup->checkupReachTempLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);



                  ckup->checkupTankAndFilmContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupTankAndFilmContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupTankAndFilmContainer, LV_ALIGN_TOP_LEFT, ui->left_row_x, ui->left_row3_y);
                  lv_obj_set_size(ckup->checkupTankAndFilmContainer, ui->left_row_w, ui->left_row_h);
                  lv_obj_set_style_border_opa(ckup->checkupTankAndFilmContainer, LV_OPA_TRANSP, 0);

                          ckup->checkupTankAndFilmStatusIcon = lv_label_create(ckup->checkupTankAndFilmContainer);
                          lv_obj_set_style_text_font(ckup->checkupTankAndFilmStatusIcon, ui->status_icon_font, 0);              
                          lv_obj_align(ckup->checkupTankAndFilmStatusIcon, LV_ALIGN_LEFT_MID, ui->left_status_icon_x, 0);


                          ckup->checkupTankAndFilmLabel = lv_label_create(ckup->checkupTankAndFilmContainer);         
                          lv_label_set_text(ckup->checkupTankAndFilmLabel, checkupTankRotation_text); 
                          lv_obj_set_width(ckup->checkupTankAndFilmLabel, ui->left_status_label_w);
                          lv_obj_set_style_text_font(ckup->checkupTankAndFilmLabel, ui->left_body_font, 0);              
                          lv_obj_align(ckup->checkupTankAndFilmLabel, LV_ALIGN_LEFT_MID, ui->left_status_label_x, 0);
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
                  lv_obj_set_style_text_font(ckup->checkupNextStepsLabel, ui->left_title_font, 0);              
                  lv_obj_align(ckup->checkupNextStepsLabel, LV_ALIGN_TOP_LEFT, ui->left_title_x, ui->left_title_y);


                  ckup->checkupStepSourceContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupStepSourceContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupStepSourceContainer, LV_ALIGN_TOP_LEFT, ui->left_row_x, ui->processing_row1_y);
                  lv_obj_set_size(ckup->checkupStepSourceContainer, ui->processing_row_w, ui->left_row_h);
                  lv_obj_set_style_border_opa(ckup->checkupStepSourceContainer, LV_OPA_TRANSP, 0);


                          ckup->checkupStepSourceLabel = lv_label_create(ckup->checkupStepSourceContainer);         
                          lv_label_set_text(ckup->checkupStepSourceLabel, checkupStepSource_text); 
                          lv_obj_set_style_text_font(ckup->checkupStepSourceLabel, ui->left_body_font, 0);              
                          lv_obj_align(ckup->checkupStepSourceLabel, LV_ALIGN_LEFT_MID, ui->left_status_icon_x, 0);

                          ckup->checkupStepSourceValue = lv_label_create(ckup->checkupStepSourceContainer);         
                          lv_label_set_text(ckup->checkupStepSourceValue, tmp_processSourceList[processToCheckup->process.processDetails->stepElementsList.start->step.stepDetails->data.source]); //THIS NEED TO BE ALIGNED WITH THE ACTUAL STEP OF THE PROCESS!
                          lv_obj_set_width(ckup->checkupStepSourceValue, LV_SIZE_CONTENT);
                          lv_obj_set_style_text_font(ckup->checkupStepSourceValue, ui->left_value_font, 0);              
                          lv_obj_align(ckup->checkupStepSourceValue, LV_ALIGN_RIGHT_MID, ui->processing_value_x, 0);


                  ckup->checkupTempControlContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupTempControlContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupTempControlContainer, LV_ALIGN_TOP_LEFT, ui->left_row_x, ui->processing_row2_y);
                  lv_obj_set_size(ckup->checkupTempControlContainer, ui->processing_row_w, ui->left_row_h);
                  lv_obj_set_style_border_opa(ckup->checkupTempControlContainer, LV_OPA_TRANSP, 0);


                          ckup->checkupTempControlLabel = lv_label_create(ckup->checkupTempControlContainer);         
                          lv_label_set_text(ckup->checkupTempControlLabel, checkupTempControl_text); 
                          lv_obj_set_style_text_font(ckup->checkupTempControlLabel, ui->left_body_font, 0);              
                          lv_obj_align(ckup->checkupTempControlLabel, LV_ALIGN_LEFT_MID, ui->left_status_icon_x, 0);

                          ckup->checkupTempControlValue = lv_label_create(ckup->checkupTempControlContainer);         
                          lv_label_set_text(ckup->checkupTempControlValue, tmp_processTempControlList[processToCheckup->process.processDetails->data.isTempControlled]);
                          lv_obj_set_width(ckup->checkupTempControlValue, LV_SIZE_CONTENT);
                          lv_obj_set_style_text_font(ckup->checkupTempControlValue, ui->left_value_font, 0);              
                          lv_obj_align(ckup->checkupTempControlValue, LV_ALIGN_RIGHT_MID, ui->processing_value_x, 0);



                  ckup->checkupWaterTempContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupWaterTempContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupWaterTempContainer, LV_ALIGN_TOP_LEFT, ui->left_row_x, ui->processing_row3_y);
                  lv_obj_set_size(ckup->checkupWaterTempContainer, ui->processing_row_w, ui->left_row_h);
                  lv_obj_set_style_border_opa(ckup->checkupWaterTempContainer, LV_OPA_TRANSP, 0);


                          ckup->checkupWaterTempLabel = lv_label_create(ckup->checkupWaterTempContainer);         
                          lv_label_set_text(ckup->checkupWaterTempLabel, checkupWaterTemp_text); 
                          lv_obj_set_style_text_font(ckup->checkupWaterTempLabel, ui->left_body_font, 0);              
                          lv_obj_align(ckup->checkupWaterTempLabel, LV_ALIGN_LEFT_MID, ui->left_status_icon_x, 0);

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
                          lv_obj_set_style_text_font(ckup->checkupWaterTempValue, ui->left_value_font, 0);              
                          lv_obj_align(ckup->checkupWaterTempValue, LV_ALIGN_RIGHT_MID, ui->processing_value_x, 0);



                  ckup->checkupNextStepContainer = lv_obj_create(ckup->checkupNextStepsContainer);
                  lv_obj_remove_flag(ckup->checkupNextStepContainer, LV_OBJ_FLAG_SCROLLABLE); 
                  lv_obj_align(ckup->checkupNextStepContainer, LV_ALIGN_TOP_LEFT, ui->left_row_x, ui->processing_row4_y);
                  lv_obj_set_size(ckup->checkupNextStepContainer, ui->processing_row_w, ui->left_row_h);
                  lv_obj_set_style_border_opa(ckup->checkupNextStepContainer, LV_OPA_TRANSP, 0);


                          ckup->checkupNextStepLabel = lv_label_create(ckup->checkupNextStepContainer);         
                          lv_label_set_text(ckup->checkupNextStepLabel, checkupNextStep_text); 
                          lv_obj_set_style_text_font(ckup->checkupNextStepLabel, ui->left_body_font, 0);              
                          lv_obj_align(ckup->checkupNextStepLabel, LV_ALIGN_LEFT_MID, ui->left_status_icon_x, 0);

                          ckup->checkupNextStepValue = lv_label_create(ckup->checkupNextStepContainer);
                          //if(ckup->currentStep->next != NULL)
                          //    lv_label_set_text(ckup->checkupNextStepValue, ckup->currentStep->next->step.stepDetails->data.stepNameString);
                          //else
                              lv_label_set_text(ckup->checkupNextStepValue, ckup->currentStep->step.stepDetails->data.stepNameString);

                          lv_obj_set_width(ckup->checkupNextStepValue, ui->processing_next_step_w);
                          lv_obj_set_style_text_font(ckup->checkupNextStepValue, ui->left_value_font, 0);
                          lv_obj_align(ckup->checkupNextStepValue, LV_ALIGN_RIGHT_MID, ui->processing_value_x, 0);
                          lv_label_set_long_mode(ckup->checkupNextStepValue, LV_LABEL_LONG_SCROLL_CIRCULAR);


                  ckup->checkupStopNowButton = lv_button_create(ckup->checkupNextStepsContainer);
                  lv_obj_set_size(ckup->checkupStopNowButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
                  lv_obj_align(ckup->checkupStopNowButton, LV_ALIGN_BOTTOM_LEFT, ui->processing_stop_left_x, ui->processing_stop_y);
                  lv_obj_add_event_cb(ckup->checkupStopNowButton, event_checkup, LV_EVENT_CLICKED, processToCheckup);
                  lv_obj_set_style_bg_color(ckup->checkupStopNowButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
                  lv_obj_move_foreground(ckup->checkupStopNowButton);


                          ckup->checkupStopNowButtonLabel = lv_label_create(ckup->checkupStopNowButton);         
                          lv_label_set_text(ckup->checkupStopNowButtonLabel, checkupStopNow_text); 
                          lv_obj_set_style_text_font(ckup->checkupStopNowButtonLabel, ui->button_font, 0);              
                          lv_obj_align(ckup->checkupStopNowButtonLabel, LV_ALIGN_CENTER, 0, 0);

                  
                  ckup->checkupStopAfterButton = lv_button_create(ckup->checkupNextStepsContainer);
                  lv_obj_set_size(ckup->checkupStopAfterButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
                  lv_obj_align(ckup->checkupStopAfterButton, LV_ALIGN_BOTTOM_RIGHT, ui->processing_stop_right_x, ui->processing_stop_y);
                  lv_obj_add_event_cb(ckup->checkupStopAfterButton, event_checkup, LV_EVENT_CLICKED, processToCheckup);
                  lv_obj_set_style_bg_color(ckup->checkupStopAfterButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
                  lv_obj_move_foreground(ckup->checkupStopAfterButton);
                  if(processToCheckup->process.processDetails->stepElementsList.size == 1){
                    lv_obj_add_state(ckup->checkupStopAfterButton, LV_STATE_DISABLED);
                  }

                          ckup->checkupStopAfterButtonLabel = lv_label_create(ckup->checkupStopAfterButton);         
                          lv_label_set_text(ckup->checkupStopAfterButtonLabel, checkupStopAfter_text); 
                          lv_obj_set_style_text_font(ckup->checkupStopAfterButtonLabel, ui->button_font, 0);              
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