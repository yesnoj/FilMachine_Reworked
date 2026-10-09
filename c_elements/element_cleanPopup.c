/**
 * @file element_cleanPopup.c
 *
 */


//ESSENTIAL INCLUDES
#include "FilMachine.h"
#include "esp_log.h"
#if defined(DISPLAY_DRIVER_ST7701)
#include "st7701_lcd.h"
#endif

static const char *TAG = "CleanProc";

extern struct gui_components gui;

//ACCESSORY INCLUDES

/* ── Clean cycle ─────────────────────────────────────────────
 * For every selected container (C1, C2, C3 in this order), `cleanCycles`
 * times: fill it with water from the bath (pump WB → Cx) for the container
 * fill time, then pump it back (Cx → WB) for the same time. At the end,
 * optionally drain the water bath to waste ("Drain water" switch).
 * Stop: the container being filled/drained is pumped back to the bath for the
 * time it still holds water, then the run ends (STOPPED).
 * The sequence decides the end, the clock only drives the arcs and the
 * remaining time (fill time × 2 × cycles × containers). */

static uint8_t processPercentage = 0;
static uint8_t cyclePercentage = 0;
static int16_t stepPercentage = 0;

static uint32_t minutesProcessElapsed = 0;
static uint8_t  secondsProcessElapsed = 0;
static uint8_t  hoursProcessElapsed = 0;

static uint32_t minutesCycleElapsed = 0;
static uint8_t  secondsCycleElapsed = 0;

static uint32_t minutesStepElapsed = 0;
static uint8_t  secondsStepElapsed = 0;

static uint8_t firstContainerIndex = 0;
static bool containerSelected = false;

static uint8_t containerIndex = 0;
static uint8_t currentCycle = 1;

static int8_t previousStepDirection = 0;

static bool isWasting = false;

/* The three arcs as shown on the display (process, cycle, pump step), kept
 * here so the app draws exactly the same thing (cleanTool*Arc). */
static uint8_t s_arcProcess, s_arcCycle, s_arcPump;
static void clean_set_arc(lv_obj_t *arc, uint8_t *mirror, int value)
{
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    *mirror = (uint8_t)value;
    lv_arc_set_value(arc, value);
}

#define CLEAN_CONTAINERS 3   /* C1..C3 (processSourceList also has WB, never cleaned here) */

static uint32_t clean_total_secs(void) {
    return gui.element.cleanPopup.totalMins * 60 + gui.element.cleanPopup.totalSecs;
}
static uint32_t clean_process_elapsed_secs(void) {
    return (uint32_t)hoursProcessElapsed * 3600 + minutesProcessElapsed * 60 + secondsProcessElapsed;
}

/* Next selected container at or after `from`, or CLEAN_CONTAINERS when none. */
static uint8_t clean_next_container(uint8_t from) {
    for (uint8_t i = from; i < CLEAN_CONTAINERS; i++)
        if (gui.element.cleanPopup.containerToClean[i]) return i;
    return CLEAN_CONTAINERS;
}

/* Run button enabled only with at least one container selected. */
static void clean_update_selection(void) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    uint8_t first = clean_next_container(0);
    containerSelected = first < CLEAN_CONTAINERS;
    if (containerSelected) firstContainerIndex = first;
    if (cp->cleanRunButton != NULL) {
        if (containerSelected) lv_obj_clear_state(cp->cleanRunButton, LV_STATE_DISABLED);
        else                   lv_obj_add_state(cp->cleanRunButton, LV_STATE_DISABLED);
    }
}

static void resetStuffBeforeNextProcess(){
    alarm_stop();
    minutesProcessElapsed = 0;
    secondsProcessElapsed = 0;
    hoursProcessElapsed = 0;

    minutesCycleElapsed = 0;
    secondsCycleElapsed = 0;

    minutesStepElapsed = 0;
    secondsStepElapsed = 0;

    stepPercentage = 0;
    processPercentage = 0;
    cyclePercentage = 0;

    containerIndex = 0;
    currentCycle = 1;

    previousStepDirection = 0;

    isWasting = false;

    gui.element.cleanPopup.stepDirection = 1;
    gui.element.cleanPopup.stopNowPressed = false;
    gui.element.cleanPopup.isAlreadyPumping = false;
    gui.element.cleanPopup.isCleaning = false;
    gui.element.cleanPopup.result = 0;

    lv_obj_clear_state(gui.element.cleanPopup.cleanStopButton, LV_STATE_DISABLED);

    lv_obj_clear_flag(gui.element.cleanPopup.cleanRemainingTimeValue, LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(gui.element.cleanPopup.cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningLabel, cleanCurrentClean_text);

    lv_label_set_text(gui.element.cleanPopup.cleanStopButtonLabel, cleanStopButton_text);

    lv_obj_clear_flag(gui.element.cleanPopup.cleanNowStepLabelValue, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(gui.element.cleanPopup.cleanNowStepLabelValue,cleanFilling_text);

    clean_set_arc(gui.element.cleanPopup.cleanProcessArc, &s_arcProcess, 0);
    clean_set_arc(gui.element.cleanPopup.cleanCycleArc, &s_arcCycle, 0);
    clean_set_arc(gui.element.cleanPopup.cleanPumpArc, &s_arcPump, 0);

    cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
}

/* End of a run: Close button, alarm, result for the app.
 * `text` (may be NULL) replaces the "now cleaning" value. */
static void clean_show_end(const char *text, uint8_t result) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    if (text != NULL) {
        lv_label_set_text(cp->cleanNowCleaningValue, text);
        lv_obj_clear_flag(cp->cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(cp->cleanNowStepLabelValue, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(cp->cleanStopButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
    lv_label_set_text(cp->cleanStopButtonLabel, cleanCloseButton_text);
    lv_obj_clear_state(cp->cleanStopButton, LV_STATE_DISABLED);
    cp->result = result;
    cp->isCleaning = false;
#if defined(DISPLAY_DRIVER_ST7701)
    st7701_lcd_set_dim_inhibit(false); /* Re-enable auto-dimming */
#endif
    alarm_start_persistent();
}

void cleanWasteTimer(lv_timer_t * timer) {
    LV_UNUSED(timer);
    struct sCleanPopup *cp = &gui.element.cleanPopup;

    /* Increment seconds for step */
    secondsStepElapsed++;
    if (secondsStepElapsed >= 60) {
        secondsStepElapsed = 0;
        minutesStepElapsed++;
    }

    uint16_t wbFillTime = getWbFillTime();
    if (wbFillTime == 0) wbFillTime = 1;
    uint32_t elapsedStepSecs = minutesStepElapsed * 60 + secondsStepElapsed;
    uint32_t remainingStepSecs = elapsedStepSecs < wbFillTime ? wbFillTime - elapsedStepSecs : 0;

    stepPercentage = (int16_t)(elapsedStepSecs >= wbFillTime ? 100 : (elapsedStepSecs * 100) / wbFillTime);

    /* Update labels and arcs */
    lv_label_set_text_fmt(cp->cleanRemainingTimeValue, "%dm%ds", (int)(remainingStepSecs / 60), (int)(remainingStepSecs % 60));
    lv_label_set_text(cp->cleanNowStepLabelValue, cleanDraining_text);
    lv_label_set_text(cp->cleanNowCleaningLabel, cleanWaste_text);
    lv_obj_add_flag(cp->cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_state(cp->cleanStopButton, LV_STATE_DISABLED);
    lv_obj_clear_flag(cp->cleanNowStepLabelValue, LV_OBJ_FLAG_HIDDEN);
    clean_set_arc(cp->cleanPumpArc, &s_arcPump, stepPercentage);

    /* Run cleanRelayManager only once at the start */
    if (!isWasting) {
        cleanRelayManager(getValueForChemicalSource(WB), getValueForChemicalSource(WASTE), PUMP_IN_RLY, true);
        isWasting = true;
    }

    if (elapsedStepSecs >= wbFillTime) {
        cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
        safeTimerDelete(&cp->wasteTimer);
        isWasting = false;
        ESP_LOGI(TAG, "=== CLEANING PROCESS FINISHED (bath drained) ===");
        clean_show_end(cleanCompleteClean_text, CLEAN_TOOL_DONE);
    }
}

/* Every selected container done: count it, then drain the bath or finish. */
static void clean_sequence_done(void) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
    safeTimerDelete(&cp->pumpTimer);

    gui.page.tools.machineStats.clean++;
    qSysAction(SAVE_MACHINE_STATS);

    processPercentage = 100;
    clean_set_arc(cp->cleanProcessArc, &s_arcProcess, 100);
    clean_set_arc(cp->cleanCycleArc, &s_arcCycle, 100);
    lv_label_set_text_fmt(cp->cleanRemainingTimeValue, "%dm%ds", 0, 0);

    if (cp->cleanDrainWater) {
        secondsStepElapsed = 0;
        minutesStepElapsed = 0;
        isWasting = false;
        cp->wasteTimer = lv_timer_create(cleanWasteTimer, 1000, NULL);
    } else {
        ESP_LOGI(TAG, "=== CLEANING PROCESS FINISHED ===");
        clean_show_end(cleanCompleteClean_text, CLEAN_TOOL_DONE);
    }
}

void cleanPumpTimer(lv_timer_t * timer) {
    LV_UNUSED(timer);
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    char *tmp_processSourceList[] = processSourceList;
    uint16_t fillTime = getContainerFillTime();
    if (fillTime == 0) fillTime = 1;

    /* ── Stop pressed: pump the current container back, then end ── */
    if (cp->stopNowPressed) {
        uint32_t left = minutesStepElapsed * 60 + secondsStepElapsed;   /* seconds of water still in it */
        if (left > 0) {
            left--;
            minutesStepElapsed = left / 60;
            secondsStepElapsed = left % 60;
            stepPercentage = (int16_t)((left * 100) / fillTime);
            if (stepPercentage > 100) stepPercentage = 100;
            clean_set_arc(cp->cleanPumpArc, &s_arcPump, stepPercentage);
            lv_label_set_text(cp->cleanNowStepLabelValue, cleanDraining_text);
            return;
        }
        cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
        safeTimerDelete(&cp->pumpTimer);
        gui.page.tools.machineStats.clean++;
        qSysAction(SAVE_MACHINE_STATS);
        ESP_LOGW(TAG, "=== CLEANING STOPPED, container drained back ===");
        clean_show_end(NULL, CLEAN_TOOL_STOPPED);
        return;
    }

    cp->isCleaning = true;

    /* ── Advance the clocks ── */
    if (++secondsStepElapsed >= 60)    { secondsStepElapsed = 0;    minutesStepElapsed++; }
    if (++secondsCycleElapsed >= 60)   { secondsCycleElapsed = 0;   minutesCycleElapsed++; }
    if (++secondsProcessElapsed >= 60) {
        secondsProcessElapsed = 0;
        if (++minutesProcessElapsed >= 60) { minutesProcessElapsed = 0; hoursProcessElapsed++; }
    }

    uint32_t stepEl = minutesStepElapsed * 60 + secondsStepElapsed;
    stepPercentage = (int16_t)(stepEl >= fillTime ? 100 : (stepEl * 100) / fillTime);

#if CLEAN_USE_LEVEL_SENSORS
    /* #13: finish the fill/drain step as soon as the container's float sensor
     * confirms the target level, instead of waiting out the time estimate.
     * Filling (dir 1): MAX wet = full. Draining (dir -1): MIN dry = empty.
     * containerIndex 0/1/2 = C1/C2/C3. Disabled by default (see the flag). */
    if (cp->stepDirection == 1) {
        if (chemLevelMaxDetected(containerIndex)) {
            /* Full confirmed: remember the real fill time for the bargraph. */
            recordFillCalibration(false, (uint16_t)stepEl);
            stepPercentage = 100;
        }
    } else {
        if (!chemLevelMinDetected(containerIndex)) stepPercentage = 100;
    }
#endif

    uint32_t cycleTotal = (uint32_t)fillTime * 2 * cp->cleanCycles;      /* one container, all its cycles */
    uint32_t cycleEl = minutesCycleElapsed * 60 + secondsCycleElapsed;
    cyclePercentage = (uint8_t)(cycleTotal == 0 || cycleEl >= cycleTotal ? 100 : (cycleEl * 100) / cycleTotal);

    uint32_t total = clean_total_secs();
    uint32_t procEl = clean_process_elapsed_secs();
    processPercentage = (uint8_t)(total == 0 ? 0 : (procEl >= total ? 99 : (procEl * 100) / total));
    if (processPercentage > 99) processPercentage = 99;        /* 100 only when the sequence ends */
    uint32_t remaining = procEl < total ? total - procEl : 0;

    /* Arcs and labels */
    clean_set_arc(cp->cleanPumpArc, &s_arcPump, (cp->stepDirection == 1) ? stepPercentage : 100 - stepPercentage);
    clean_set_arc(cp->cleanCycleArc, &s_arcCycle, cyclePercentage);
    clean_set_arc(cp->cleanProcessArc, &s_arcProcess, processPercentage);
    lv_label_set_text_fmt(cp->cleanRemainingTimeValue, "%dm%ds", (int)(remaining / 60), (int)(remaining % 60));
    lv_label_set_text_fmt(cp->cleanNowCleaningValue, cleanCycleFmt_text, tmp_processSourceList[containerIndex], currentCycle);

    /* ── Step finished: fill → drain → next cycle → next container ── */
    if (stepPercentage >= 100) {
        minutesStepElapsed = 0;
        secondsStepElapsed = 0;
        if (cp->stepDirection == 1) {
            cp->stepDirection = -1;                     /* full: pump it back to the bath */
        } else {
            cp->stepDirection = 1;
            if (++currentCycle > cp->cleanCycles) {
                currentCycle = 1;
                minutesCycleElapsed = 0;
                secondsCycleElapsed = 0;
                uint8_t next = clean_next_container(containerIndex + 1);
                if (next >= CLEAN_CONTAINERS) {          /* keep the last one cleaned for the app */
                    clean_sequence_done();
                    return;
                }
                containerIndex = next;
            }
        }
    }

    /* Valves and pump follow the direction (switched once per step) */
    if (cp->stepDirection != previousStepDirection) {
        LV_LOG_USER("Clean C%d: %s", containerIndex + 1, cp->stepDirection == 1 ? "fill" : "drain");
        cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
        if (cp->stepDirection == 1)
            cleanRelayManager(getValueForChemicalSource(WB), getValueForChemicalSource(containerIndex), PUMP_IN_RLY, true);
        else
            cleanRelayManager(getValueForChemicalSource(containerIndex), getValueForChemicalSource(WB), PUMP_OUT_RLY, true);
        previousStepDirection = cp->stepDirection;
    }

    lv_label_set_text(cp->cleanNowStepLabelValue, (cp->stepDirection == 1) ? cleanFilling_text : cleanDraining_text);
}

/* Run: start the clean with the choices on the settings page. */
static bool clean_run(void) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    char *tmp_processSourceList[] = processSourceList;

    clean_update_selection();
    if (!containerSelected || cp->pumpTimer != NULL || cp->wasteTimer != NULL) return false;

    ESP_LOGI(TAG, "=== CLEANING PROCESS STARTED ===");
#if defined(DISPLAY_DRIVER_ST7701)
    st7701_lcd_set_dim_inhibit(true);   /* Keep screen on during cleaning */
#endif
    resetStuffBeforeNextProcess();
    containerIndex = firstContainerIndex;
    cp->isCleaning = true;

    lv_obj_add_flag(cp->cleanSettingsContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cp->cleanRunButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cp->cleanCancelButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(cp->cleanProcessContainer, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(cp->cleanTitle, cleanCleanProcess_text);
    lv_obj_remove_flag(cp->cleanRemainingTimeValue, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_bg_color(cp->cleanStopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
    lv_label_set_text(cp->cleanNowCleaningLabel, cleanCurrentClean_text);

    getMinutesAndSeconds(getContainerFillTime(), cp->containerToClean);
    lv_label_set_text_fmt(cp->cleanRemainingTimeValue, "%"PRIu32"m%"PRIu32"s", cp->totalMins, cp->totalSecs);
    LV_LOG_USER("Process totalMin: %"PRIu32" totalSecs: %"PRIu32"", cp->totalMins, cp->totalSecs);

    lv_obj_remove_flag(cp->cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text_fmt(cp->cleanNowCleaningValue, cleanCycleFmt_text, tmp_processSourceList[firstContainerIndex], currentCycle);

    cp->pumpTimer = lv_timer_create(cleanPumpTimer, 1000, NULL);
    return true;
}

/* Stop: drain back what is in the current container, then end (STOPPED). */
static void clean_stop_request(void) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    if (cp->pumpTimer == NULL || cp->stopNowPressed) return;
    ESP_LOGW(TAG, "Cleaning STOPPED by user");
    cp->stopNowPressed = true;
    alarm_stop();

    uint16_t fillTime = getContainerFillTime();
    uint32_t el = minutesStepElapsed * 60 + secondsStepElapsed;
    if (cp->stepDirection == 1) {
        /* Was filling: pump back for the time it has been filling. */
        cleanRelayManager(INVALID_RELAY, INVALID_RELAY, INVALID_RELAY, false);
        cleanRelayManager(getValueForChemicalSource(containerIndex), getValueForChemicalSource(WB), PUMP_OUT_RLY, true);
        cp->stepDirection = -1;
        previousStepDirection = -1;
    } else {
        /* Was draining: only what is still in it. */
        uint32_t left = el < fillTime ? fillTime - el : 0;
        minutesStepElapsed = left / 60;
        secondsStepElapsed = left % 60;
    }

    lv_label_set_text(cp->cleanTitle, cleanCanceled_text);
    lv_obj_add_state(cp->cleanStopButton, LV_STATE_DISABLED);
    lv_obj_add_flag(cp->cleanRemainingTimeValue, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cp->cleanNowCleaningValue, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(cp->cleanNowCleaningLabel, cleanCanceled_text);
}

/* Close after the end: back to the settings page, alarm off. */
static void clean_back_to_settings(void) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    alarm_stop();
#if defined(DISPLAY_DRIVER_ST7701)
    st7701_lcd_set_dim_inhibit(false);
#endif
    cp->result = 0;
    cp->stopNowPressed = false;
    cp->isCleaning = false;
    lv_obj_add_flag(cp->cleanRemainingTimeValue, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cp->cleanProcessContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(cp->cleanSettingsContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(cp->cleanRunButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(cp->cleanCancelButton, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(cp->cleanTitle, cleanPopupTitle_text);
    lv_obj_clear_state(cp->cleanStopButton, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(cp->cleanStopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
    lv_label_set_text(cp->cleanStopButtonLabel, cleanStopButton_text);
}


void event_cleanPopup(lv_event_t * e) {

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
  struct sCleanPopup *cp = &gui.element.cleanPopup;

  if(code == LV_EVENT_SHORT_CLICKED){
    if(obj == cp->cleanSpinBoxPlusButton){
      lv_spinbox_increment(cp->cleanSpinBox);
      cp->cleanCycles = lv_spinbox_get_value(cp->cleanSpinBox);
      LV_LOG_USER("Cycles programmed :%d",cp->cleanCycles);
    }
    if(obj == cp->cleanSpinBoxMinusButton){
      lv_spinbox_decrement(cp->cleanSpinBox);
      cp->cleanCycles = lv_spinbox_get_value(cp->cleanSpinBox);
      LV_LOG_USER("Cycles programmed :%d",cp->cleanCycles);
    }
  }
  if(code == LV_EVENT_RELEASED){
    if(obj == cp->cleanRunButton){
      clean_run();
    }
    if(obj == cp->cleanCancelButton){
      alarm_stop();
      lv_obj_add_flag(cp->cleanPopupParent, LV_OBJ_FLAG_HIDDEN);
    }
    if(obj == cp->cleanStopButton) {
      if (cp->pumpTimer != NULL) clean_stop_request();                 /* Stop */
      else if (cp->wasteTimer == NULL) clean_back_to_settings();       /* Close after the end */
    }
  }

  if(obj == cp->cleanDrainWaterSwitch){
    if(code == LV_EVENT_VALUE_CHANGED) {
      LV_LOG_USER("State cleanDrainWaterSwitch: %s", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
      cp->cleanDrainWater = lv_obj_has_state(obj, LV_STATE_CHECKED);
    }
  }

  if(obj == cp->cleanSelectC1CheckBox || obj == cp->cleanSelectC2CheckBox || obj == cp->cleanSelectC3CheckBox){
    if(code == LV_EVENT_VALUE_CHANGED) {
      if(obj == cp->cleanSelectC1CheckBox) cp->containerToClean[0] = lv_obj_has_state(obj, LV_STATE_CHECKED);
      if(obj == cp->cleanSelectC2CheckBox) cp->containerToClean[1] = lv_obj_has_state(obj, LV_STATE_CHECKED);
      if(obj == cp->cleanSelectC3CheckBox) cp->containerToClean[2] = lv_obj_has_state(obj, LV_STATE_CHECKED);
      clean_update_selection();
      LV_LOG_USER("Containers C1=%d C2=%d C3=%d", cp->containerToClean[0], cp->containerToClean[1], cp->containerToClean[2]);
    }
  }
}

/* ══════════════════════════════════════════════════════════
 *  STATE FOR THE APP  +  REMOTE CONTROL (WebSocket)
 * ══════════════════════════════════════════════════════════ */
bool cleanToolBusy(void) {
    return gui.element.cleanPopup.pumpTimer != NULL || gui.element.cleanPopup.wasteTimer != NULL;
}

int cleanToolState(void) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    if (cp->pumpTimer != NULL)  return cp->stopNowPressed ? CLEAN_TOOL_STOPPING : CLEAN_TOOL_RUNNING;
    if (cp->wasteTimer != NULL) return CLEAN_TOOL_WASTE;
    return cp->result;
}

int  cleanToolContainer(void) { return containerIndex < CLEAN_CONTAINERS ? containerIndex : CLEAN_CONTAINERS - 1; }
int  cleanToolCycle(void)     { return currentCycle; }
int  cleanToolCycles(void)    { return gui.element.cleanPopup.cleanCycles ? gui.element.cleanPopup.cleanCycles : 1; }
/* Filling only while a container is being filled: not while it is pumped
 * back after Stop, not while the bath goes to waste, not after the end. */
bool cleanToolFilling(void)   {
    const struct sCleanPopup *cp = &gui.element.cleanPopup;
    return cp->pumpTimer != NULL && cp->stepDirection == 1 && !cp->stopNowPressed;
}
int  cleanToolProcessArc(void) { return s_arcProcess; }
int  cleanToolCycleArc(void)   { return s_arcCycle; }
int  cleanToolPumpArc(void)    { return s_arcPump; }
bool cleanToolDrainWb(void)   { return gui.element.cleanPopup.cleanDrainWater; }

int cleanToolPercent(void) {
    if (gui.element.cleanPopup.result == CLEAN_TOOL_DONE || gui.element.cleanPopup.wasteTimer != NULL) return 100;
    return processPercentage;
}

uint32_t cleanToolRemainingSecs(void) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    if (cp->wasteTimer != NULL) {
        uint32_t el = minutesStepElapsed * 60 + secondsStepElapsed, wb = getWbFillTime();
        return el < wb ? wb - el : 0;
    }
    if (cp->pumpTimer == NULL) return 0;
    if (cp->stopNowPressed) return minutesStepElapsed * 60 + secondsStepElapsed;
    uint32_t total = clean_total_secs(), el = clean_process_elapsed_secs();
    return el < total ? total - el : 0;
}

uint8_t cleanToolMask(void) {
    const bool *c = gui.element.cleanPopup.containerToClean;
    return (uint8_t)((c[0] ? 1 : 0) | (c[1] ? 2 : 0) | (c[2] ? 4 : 0));
}

bool cleanPopupRemoteStart(uint8_t mask, uint8_t cycles, bool drainWb) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    if (cleanToolBusy()) return false;
    if (cp->cleanPopupParent == NULL) cleanPopup();
    if (cp->cleanPopupParent == NULL) return false;
    if (cp->result != 0) clean_back_to_settings();

    /* Same choices on the popup, so the display shows what the app asked */
    lv_obj_t *boxes[CLEAN_CONTAINERS] = { cp->cleanSelectC1CheckBox, cp->cleanSelectC2CheckBox, cp->cleanSelectC3CheckBox };
    for (uint8_t i = 0; i < CLEAN_CONTAINERS; i++) {
        cp->containerToClean[i] = (mask >> i) & 1;
        if (cp->containerToClean[i]) lv_obj_add_state(boxes[i], LV_STATE_CHECKED);
        else                         lv_obj_remove_state(boxes[i], LV_STATE_CHECKED);
    }
    if (cycles < CLEAN_CYCLES_MIN) cycles = CLEAN_CYCLES_MIN;
    if (cycles > CLEAN_CYCLES_MAX) cycles = CLEAN_CYCLES_MAX;
    cp->cleanCycles = cycles;
    lv_spinbox_set_value(cp->cleanSpinBox, cycles);
    cp->cleanDrainWater = drainWb;
    if (drainWb) lv_obj_add_state(cp->cleanDrainWaterSwitch, LV_STATE_CHECKED);
    else         lv_obj_remove_state(cp->cleanDrainWaterSwitch, LV_STATE_CHECKED);

    lv_obj_remove_flag(cp->cleanPopupParent, LV_OBJ_FLAG_HIDDEN);
    return clean_run();
}

void cleanPopupRemoteStop(void) {
    clean_stop_request();
}

void cleanPopupRemoteClose(void) {
    struct sCleanPopup *cp = &gui.element.cleanPopup;
    if (cp->cleanPopupParent == NULL || cleanToolBusy()) return;
    clean_back_to_settings();
    lv_obj_add_flag(cp->cleanPopupParent, LV_OBJ_FLAG_HIDDEN);
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
              lv_obj_align(gui.element.cleanPopup.cleanTitle, LV_ALIGN_TOP_MID, ui->title_x, ui->title_y);

              /*Create style*/
              initTitleLineStyle(&gui.element.cleanPopup.style_cleanTitleLine, WHITE);

              /*Create a line and apply the new style*/
              gui.element.cleanPopup.cleanPopupTitleLine = lv_line_create(gui.element.cleanPopup.cleanContainer);
              lv_line_set_points(gui.element.cleanPopup.cleanPopupTitleLine, gui.element.cleanPopup.titleLinePoints, 2);
              lv_obj_add_style(gui.element.cleanPopup.cleanPopupTitleLine, &gui.element.cleanPopup.style_cleanTitleLine, 0);
              lv_obj_align(gui.element.cleanPopup.cleanPopupTitleLine, LV_ALIGN_TOP_MID, ui->title_line_x, ui->title_line_y);

              gui.element.cleanPopup.cleanSettingsContainer = lv_obj_create(gui.element.cleanPopup.cleanPopupParent);
              lv_obj_align(gui.element.cleanPopup.cleanSettingsContainer, LV_ALIGN_TOP_MID, ui->settings_x, ui->settings_y);
              lv_obj_set_size(gui.element.cleanPopup.cleanSettingsContainer, ui_get_profile()->popups.clean_settings_w, ui_get_profile()->popups.clean_settings_h); 
              lv_obj_remove_flag(gui.element.cleanPopup.cleanSettingsContainer, LV_OBJ_FLAG_SCROLLABLE); 
              lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSettingsContainer , LV_OPA_TRANSP, 0);
            
                          gui.element.cleanPopup.cleanSubTitleLabel = lv_label_create(gui.element.cleanPopup.cleanSettingsContainer);         
                          lv_label_set_text(gui.element.cleanPopup.cleanSubTitleLabel, cleanPopupSubTitle_text); 
                          lv_obj_set_style_text_font(gui.element.cleanPopup.cleanSubTitleLabel, ui->subtitle_font, 0);              
                          lv_obj_align(gui.element.cleanPopup.cleanSubTitleLabel, LV_ALIGN_TOP_MID, ui->subtitle_x, ui->subtitle_y);
                        
                          
                          gui.element.cleanPopup.cleanChemicalTanksContainer = lv_obj_create(gui.element.cleanPopup.cleanSettingsContainer);
                          lv_obj_remove_flag(gui.element.cleanPopup.cleanChemicalTanksContainer , LV_OBJ_FLAG_SCROLLABLE); 
                          lv_obj_align(gui.element.cleanPopup.cleanChemicalTanksContainer , LV_ALIGN_LEFT_MID, ui->chem_container_x, ui->chem_container_y);
                          lv_obj_set_size(gui.element.cleanPopup.cleanChemicalTanksContainer , ui->chem_container_w, ui->chem_container_h); 
                          lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanChemicalTanksContainer , LV_OPA_TRANSP, 0);


                                  //Container checkboxes
                                  gui.element.cleanPopup.cleanSelectC1CheckBox = lv_obj_create(gui.element.cleanPopup.cleanChemicalTanksContainer);
                                  lv_obj_remove_flag(gui.element.cleanPopup.cleanSelectC1CheckBox, LV_OBJ_FLAG_SCROLLABLE); 
                                  lv_obj_align(gui.element.cleanPopup.cleanSelectC1CheckBox, LV_ALIGN_LEFT_MID, ui->chem_c1_checkbox_x, ui->checkbox_y);
                                  lv_obj_set_size(gui.element.cleanPopup.cleanSelectC1CheckBox, ui->checkbox_w, ui->checkbox_h); 
                                  lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSelectC1CheckBox, LV_OPA_TRANSP, 0);

                                        gui.element.cleanPopup.cleanC1CheckBoxLabel = lv_label_create(gui.element.cleanPopup.cleanSelectC1CheckBox);         
                                        lv_label_set_text(gui.element.cleanPopup.cleanC1CheckBoxLabel, tmp_processSourceList[0]); 
                                        lv_obj_set_style_text_font(gui.element.cleanPopup.cleanC1CheckBoxLabel, ui->label_font, 0);              
                                        lv_obj_align(gui.element.cleanPopup.cleanC1CheckBoxLabel, LV_ALIGN_LEFT_MID, ui->checkbox_label_x, ui->checkbox_label_y);

                                        gui.element.cleanPopup.cleanSelectC1CheckBox = create_radiobutton(gui.element.cleanPopup.cleanSelectC1CheckBox, "", 0, 0, ui->checkbox_radio_size, ui->subtitle_font, lv_color_hex(WHITE), lv_palette_main(LV_PALETTE_BLUE));
                                        lv_obj_add_event_cb(gui.element.cleanPopup.cleanSelectC1CheckBox, event_cleanPopup, LV_EVENT_VALUE_CHANGED, gui.element.cleanPopup.cleanSelectC1CheckBox);


                                  gui.element.cleanPopup.cleanSelectC2CheckBox = lv_obj_create(gui.element.cleanPopup.cleanChemicalTanksContainer);
                                  lv_obj_remove_flag(gui.element.cleanPopup.cleanSelectC2CheckBox, LV_OBJ_FLAG_SCROLLABLE); 
                                  lv_obj_align(gui.element.cleanPopup.cleanSelectC2CheckBox, LV_ALIGN_LEFT_MID, ui->chem_c2_checkbox_x, ui->checkbox_y);
                                  lv_obj_set_size(gui.element.cleanPopup.cleanSelectC2CheckBox, ui->checkbox_w, ui->checkbox_h); 
                                  lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSelectC2CheckBox, LV_OPA_TRANSP, 0);

                                        gui.element.cleanPopup.cleanC2CheckBoxLabel = lv_label_create(gui.element.cleanPopup.cleanSelectC2CheckBox);         
                                        lv_label_set_text(gui.element.cleanPopup.cleanC2CheckBoxLabel, tmp_processSourceList[1]); 
                                        lv_obj_set_style_text_font(gui.element.cleanPopup.cleanC2CheckBoxLabel, ui->label_font, 0);              
                                        lv_obj_align(gui.element.cleanPopup.cleanC2CheckBoxLabel, LV_ALIGN_LEFT_MID, ui->checkbox_label_x, ui->checkbox_label_y);

                                        gui.element.cleanPopup.cleanSelectC2CheckBox = create_radiobutton(gui.element.cleanPopup.cleanSelectC2CheckBox, "", 0, 0, ui->checkbox_radio_size, ui->subtitle_font, lv_color_hex(WHITE), lv_palette_main(LV_PALETTE_BLUE));
                                        lv_obj_add_event_cb(gui.element.cleanPopup.cleanSelectC2CheckBox, event_cleanPopup, LV_EVENT_VALUE_CHANGED, gui.element.cleanPopup.cleanSelectC2CheckBox);


                                  gui.element.cleanPopup.cleanSelectC3CheckBox = lv_obj_create(gui.element.cleanPopup.cleanChemicalTanksContainer);
                                  lv_obj_remove_flag(gui.element.cleanPopup.cleanSelectC3CheckBox, LV_OBJ_FLAG_SCROLLABLE); 
                                  lv_obj_align(gui.element.cleanPopup.cleanSelectC3CheckBox, LV_ALIGN_LEFT_MID, ui->chem_c3_checkbox_x, ui->checkbox_y);
                                  lv_obj_set_size(gui.element.cleanPopup.cleanSelectC3CheckBox, ui->checkbox_w, ui->checkbox_h); 
                                  lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSelectC3CheckBox, LV_OPA_TRANSP, 0);

                                        gui.element.cleanPopup.cleanC3CheckBoxLabel = lv_label_create(gui.element.cleanPopup.cleanSelectC3CheckBox);         
                                        lv_label_set_text(gui.element.cleanPopup.cleanC3CheckBoxLabel, tmp_processSourceList[2]); 
                                        lv_obj_set_style_text_font(gui.element.cleanPopup.cleanC3CheckBoxLabel, ui->label_font, 0);              
                                        lv_obj_align(gui.element.cleanPopup.cleanC3CheckBoxLabel, LV_ALIGN_LEFT_MID, ui->checkbox_label_x, ui->checkbox_label_y);

                                        gui.element.cleanPopup.cleanSelectC3CheckBox = create_radiobutton(gui.element.cleanPopup.cleanSelectC3CheckBox, "", 0, 0, ui->checkbox_radio_size, ui->subtitle_font, lv_color_hex(WHITE), lv_palette_main(LV_PALETTE_BLUE));
                                        lv_obj_add_event_cb(gui.element.cleanPopup.cleanSelectC3CheckBox, event_cleanPopup, LV_EVENT_VALUE_CHANGED, gui.element.cleanPopup.cleanSelectC3CheckBox);  


                          
                          gui.element.cleanPopup.cleanSpinBoxContainer = lv_obj_create(gui.element.cleanPopup.cleanSettingsContainer);
                          lv_obj_remove_flag(gui.element.cleanPopup.cleanSpinBoxContainer, LV_OBJ_FLAG_SCROLLABLE); 
                          lv_obj_align(gui.element.cleanPopup.cleanSpinBoxContainer, LV_ALIGN_CENTER, ui->spinbox_container_x, ui->spinbox_container_y);
                          lv_obj_set_size(gui.element.cleanPopup.cleanSpinBoxContainer, ui->spinbox_container_w, ui->spinbox_container_h); 
                          lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanSpinBoxContainer, LV_OPA_TRANSP, 0);


                                gui.element.cleanPopup.cleanSpinBox = lv_spinbox_create(gui.element.cleanPopup.cleanSpinBoxContainer);
                                lv_spinbox_set_range(gui.element.cleanPopup.cleanSpinBox, 1, 5);
                                lv_spinbox_set_digit_format(gui.element.cleanPopup.cleanSpinBox, 1, 0);
                                lv_obj_set_width(gui.element.cleanPopup.cleanSpinBox, ui->spinbox_w);
                                lv_obj_align(gui.element.cleanPopup.cleanSpinBox, LV_ALIGN_LEFT_MID, ui->spinbox_x, ui->spinbox_y);
                                lv_obj_set_style_bg_opa(gui.element.cleanPopup.cleanSpinBox, 0, LV_PART_CURSOR);


                                      gui.element.cleanPopup.cleanRepeatTimesLabel = lv_label_create(gui.element.cleanPopup.cleanSpinBoxContainer);         
                                      lv_label_set_text(gui.element.cleanPopup.cleanRepeatTimesLabel, cleanRoller_text); 
                                      lv_obj_set_style_text_font(gui.element.cleanPopup.cleanRepeatTimesLabel, ui->label_font, 0);              
                                      lv_obj_align(gui.element.cleanPopup.cleanRepeatTimesLabel, LV_ALIGN_LEFT_MID, ui->repeat_label_x, ui->repeat_label_y);

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
                      lv_obj_align(gui.element.cleanPopup.cleanDrainWaterLabelContainer, LV_ALIGN_CENTER, ui->drain_container_x, ui->drain_container_y);
                      lv_obj_set_size(gui.element.cleanPopup.cleanDrainWaterLabelContainer, ui->drain_container_w, ui_get_profile()->clean_drain_container_h);
                      lv_obj_set_style_border_opa(gui.element.cleanPopup.cleanDrainWaterLabelContainer , LV_OPA_TRANSP, 0);


                          gui.element.cleanPopup.cleanDrainWaterLabel = lv_label_create(gui.element.cleanPopup.cleanDrainWaterLabelContainer);         
                          lv_label_set_text(gui.element.cleanPopup.cleanDrainWaterLabel, cleanDrainWater_text); 
                          lv_obj_set_style_text_font(gui.element.cleanPopup.cleanDrainWaterLabel, ui->label_font, 0);              
                          lv_obj_align(gui.element.cleanPopup.cleanDrainWaterLabel, LV_ALIGN_LEFT_MID, ui->checkbox_label_x, ui->drain_label_y);

                          gui.element.cleanPopup.cleanDrainWaterSwitch = lv_switch_create(gui.element.cleanPopup.cleanDrainWaterLabelContainer);
                          lv_obj_set_size(gui.element.cleanPopup.cleanDrainWaterSwitch, ui_get_profile()->settings.toggle_switch_w, ui_get_profile()->settings.toggle_switch_h);
                          lv_obj_add_event_cb(gui.element.cleanPopup.cleanDrainWaterSwitch , event_cleanPopup, LV_EVENT_VALUE_CHANGED, gui.element.cleanPopup.cleanDrainWaterSwitch);
                          lv_obj_align(gui.element.cleanPopup.cleanDrainWaterSwitch , LV_ALIGN_LEFT_MID, ui->drain_switch_x + ui->drain_switch_extra_x, ui->drain_switch_y);
                          lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanDrainWaterSwitch, lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
                          lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanDrainWaterSwitch,  lv_palette_main(LV_PALETTE_BLUE), LV_PART_KNOB | LV_STATE_DEFAULT);
                          lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanDrainWaterSwitch, lv_color_hex(BLUE_DARK) , LV_PART_INDICATOR | LV_STATE_CHECKED);



              
                      gui.element.cleanPopup.cleanRunButton = lv_button_create(gui.element.cleanPopup.cleanContainer);
                      lv_obj_set_size(gui.element.cleanPopup.cleanRunButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
                      lv_obj_align(gui.element.cleanPopup.cleanRunButton, LV_ALIGN_BOTTOM_RIGHT, ui->cancel_button_x , ui->cancel_button_y);
                      lv_obj_add_event_cb(gui.element.cleanPopup.cleanRunButton, event_cleanPopup, LV_EVENT_RELEASED, NULL);
                      lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanRunButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
                      lv_obj_add_state(gui.element.cleanPopup.cleanRunButton, LV_STATE_DISABLED);


                          gui.element.cleanPopup.cleanCancelButtonLabel = lv_label_create(gui.element.cleanPopup.cleanRunButton);
                          lv_label_set_text(gui.element.cleanPopup.cleanCancelButtonLabel, cleanRunButton_text);
                          lv_obj_set_style_text_font(gui.element.cleanPopup.cleanCancelButtonLabel, ui->button_font, 0);
                          lv_obj_align(gui.element.cleanPopup.cleanCancelButtonLabel, LV_ALIGN_CENTER, 0, 0);


                      gui.element.cleanPopup.cleanCancelButton = lv_button_create(gui.element.cleanPopup.cleanContainer);
                      lv_obj_set_size(gui.element.cleanPopup.cleanCancelButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
                      lv_obj_align(gui.element.cleanPopup.cleanCancelButton, LV_ALIGN_BOTTOM_LEFT, ui->run_button_x , ui->run_button_y);
                      lv_obj_add_event_cb(gui.element.cleanPopup.cleanCancelButton, event_cleanPopup, LV_EVENT_RELEASED, NULL);
                      lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanCancelButton, lv_color_hex(RED_DARK), LV_PART_MAIN);

                          gui.element.cleanPopup.cleanCancelButtonLabel = lv_label_create(gui.element.cleanPopup.cleanCancelButton);
                          lv_label_set_text(gui.element.cleanPopup.cleanCancelButtonLabel, cleanCancelButton_text);
                          lv_obj_set_style_text_font(gui.element.cleanPopup.cleanCancelButtonLabel, ui->button_font, 0);
                          lv_obj_align(gui.element.cleanPopup.cleanCancelButtonLabel, LV_ALIGN_CENTER, 0, 0);
        
        

        gui.element.cleanPopup.cleanProcessContainer = lv_obj_create(gui.element.cleanPopup.cleanPopupParent);
        lv_obj_align(gui.element.cleanPopup.cleanProcessContainer, LV_ALIGN_TOP_MID, ui->settings_x, ui->settings_y);
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
              lv_obj_align(gui.element.cleanPopup.cleanProcessArc, LV_ALIGN_CENTER, ui->process_arc_x, ui->process_arc_y);
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
              lv_obj_align(gui.element.cleanPopup.cleanCycleArc, LV_ALIGN_CENTER, ui->process_arc_x, ui->process_arc_y);
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
              lv_obj_align(gui.element.cleanPopup.cleanPumpArc, LV_ALIGN_CENTER, ui->process_arc_x, ui->process_arc_y);
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
              lv_obj_align(gui.element.cleanPopup.cleanRemainingTimeValue, LV_ALIGN_CENTER, ui->remaining_time_x, ui->remaining_time_y);

              gui.element.cleanPopup.cleanNowCleaningLabel = lv_label_create(gui.element.cleanPopup.cleanProcessContainer);         
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanNowCleaningLabel, ui->value_font, 0);              
              lv_obj_align(gui.element.cleanPopup.cleanNowCleaningLabel, LV_ALIGN_CENTER, ui->now_cleaning_label_x, ui->now_cleaning_label_y);
              lv_label_set_text(gui.element.cleanPopup.cleanNowCleaningLabel, cleanCurrentClean_text); 

              gui.element.cleanPopup.cleanNowCleaningValue = lv_label_create(gui.element.cleanPopup.cleanProcessContainer);          
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanNowCleaningValue, ui->value_font, 0);              
              lv_obj_align(gui.element.cleanPopup.cleanNowCleaningValue, LV_ALIGN_CENTER, ui->now_cleaning_value_x, ui->now_cleaning_value_y);


              gui.element.cleanPopup.cleanNowStepLabelValue = lv_label_create(gui.element.cleanPopup.cleanProcessContainer);          
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanNowStepLabelValue, ui->step_font, 0); 
              //lv_label_set_text(gui.element.cleanPopup.cleanNowStepLabelValue,cleanFilling_text);             
              lv_obj_align(gui.element.cleanPopup.cleanNowStepLabelValue, LV_ALIGN_CENTER, ui->now_step_x, ui->now_step_y);


              gui.element.cleanPopup.cleanStopButton = lv_button_create(gui.element.cleanPopup.cleanProcessContainer);
              lv_obj_set_size(gui.element.cleanPopup.cleanStopButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
              lv_obj_align(gui.element.cleanPopup.cleanStopButton, LV_ALIGN_BOTTOM_MID, ui->stop_button_x , ui->stop_button_y);
              lv_obj_add_event_cb(gui.element.cleanPopup.cleanStopButton, event_cleanPopup, LV_EVENT_RELEASED, NULL);
              lv_obj_set_style_bg_color(gui.element.cleanPopup.cleanStopButton, lv_color_hex(RED_DARK), LV_PART_MAIN);

              gui.element.cleanPopup.cleanStopButtonLabel = lv_label_create(gui.element.cleanPopup.cleanStopButton);
              lv_label_set_text(gui.element.cleanPopup.cleanStopButtonLabel, cleanStopButton_text);
              lv_obj_set_style_text_font(gui.element.cleanPopup.cleanStopButtonLabel, ui->button_font, 0);
              lv_obj_align(gui.element.cleanPopup.cleanStopButtonLabel, LV_ALIGN_CENTER, 0, 0);

  }
}

