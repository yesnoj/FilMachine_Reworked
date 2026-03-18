
/**
 * @file page_stepDetail.c
 *
 */
#include <string.h>

#include "FilMachine.h"

extern struct gui_components gui;

/**
 * Check whether the step detail form has valid inputs for saving.
 * Returns true if the name is non-empty AND at least one of mins/secs > 0.
 */
static bool step_detail_is_valid(sStepDetail *sd) {
    if (sd == NULL) return false;
    const char *name = lv_textarea_get_text(sd->stepDetailNameTextArea);
    if (name == NULL || strlen(name) == 0) return false;
    long mins = strtol(lv_textarea_get_text(sd->stepDetailMinTextArea), NULL, 10);
    long secs = strtol(lv_textarea_get_text(sd->stepDetailSecTextArea), NULL, 10);
    /* Minimum step duration: 30 seconds */
    long totalSecs = mins * 60 + secs;
    return (totalSecs >= 30);
}

/*--------------------------------------------------------------
 *  Sub-functions for event_stepDetail  (Point 3 refactor)
 *-------------------------------------------------------------*/

/** Reset swipe visual state on the step element row */
static void step_detail_reset_swipe(stepNode *sn) {
    if (sn->step.swipedLeft == true && sn->step.swipedRight == false) {
        uint32_t x = lv_obj_get_x_aligned(sn->step.stepElement) + 50;
        uint32_t y = lv_obj_get_y_aligned(sn->step.stepElement);
        lv_obj_set_pos(sn->step.stepElement, x, y);
        sn->step.swipedLeft = false;
        sn->step.swipedRight = false;
        lv_obj_add_flag(sn->step.deleteButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sn->step.editButton, LV_OBJ_FLAG_HIDDEN);
    }
}

/** Handle Save button click: commit step data, update UI, close popup */
static void step_detail_save(stepNode *sn, processNode *proc) {
    LV_LOG_USER("Pressed stepSaveButton");

    snprintf(sn->step.stepDetails->data.stepNameString,
             sizeof(sn->step.stepDetails->data.stepNameString), "%s",
             lv_textarea_get_text(sn->step.stepDetails->stepDetailNameTextArea));

    if (addStepElement(sn, proc) != NULL) {
        LV_LOG_USER("Step %p element created!Now process %p has n: %d steps",
                     sn, proc, ((processNode *)proc)->process.processDetails->stepElementsList.size);
        stepElementCreate(sn, proc, -1);
    } else {
        LV_LOG_USER("Step element creation failed, maximum entries reached");
    }

    sn->step.stepDetails->data.somethingChanged = false;
    lv_obj_send_event(sn->step.stepDetails->stepSaveButton, LV_EVENT_REFRESH, NULL);

    proc->process.processDetails->data.somethingChanged = true;
    lv_obj_send_event(proc->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL);

    step_detail_reset_swipe(sn);
    calculateTotalTime(proc);
    updateStepElement(proc, sn);
    lv_msgbox_close(sn->step.stepDetails->stepDetailParent);
}

/** Handle Cancel button click: reset swipe, close popup */
static void step_detail_cancel(stepNode *sn) {
    LV_LOG_USER("Pressed gui.tempStepNode->step.stepDetails->stepCancelButton");
    step_detail_reset_swipe(sn);
    lv_msgbox_close(sn->step.stepDetails->stepDetailParent);
}

/** Handle dropdown and switch value changes */
static void step_detail_handle_value_changed(stepNode *sn, lv_obj_t *obj) {
    sStepDetail *sd = sn->step.stepDetails;

    if (obj == sd->stepTypeDropDownList) {
        uint32_t sel = lv_dropdown_get_selected(sd->stepTypeDropDownList);
        if (sel == CHEMISTRY) {
            sd->data.type = CHEMISTRY;
            LV_LOG_USER("Selected stepTypeDropDownList: CHEMISTRY (%d)", sd->data.type);
            lv_label_set_text(sd->stepTypeHelpIcon, chemical_icon);
        } else if (sel == RINSE) {
            sd->data.type = RINSE;
            LV_LOG_USER("Selected stepTypeDropDownList: RINSE (%d)", sd->data.type);
            lv_label_set_text(sd->stepTypeHelpIcon, rinse_icon);
        } else if (sel == MULTI_RINSE) {
            sd->data.type = MULTI_RINSE;
            LV_LOG_USER("Selected stepTypeDropDownList: MULTI_RINSE (%d)", sd->data.type);
            lv_label_set_text(sd->stepTypeHelpIcon, multiRinse_icon);
        }
        sd->data.somethingChanged = true;
        lv_obj_send_event(sd->stepSaveButton, LV_EVENT_REFRESH, NULL);
    }

    if (obj == sd->stepDiscardAfterSwitch) {
        sd->data.discardAfterProc = lv_obj_has_state(obj, LV_STATE_CHECKED);
        sd->data.somethingChanged = true;
        lv_obj_send_event(sd->stepSaveButton, LV_EVENT_REFRESH, NULL);
        LV_LOG_USER("Discard After : %s", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
    }

    if (obj == sd->stepSourceDropDownList) {
        sd->data.source = lv_dropdown_get_selected(sd->stepSourceDropDownList);
        sd->data.somethingChanged = true;
        lv_obj_send_event(sd->stepSaveButton, LV_EVENT_REFRESH, NULL);
        LV_LOG_USER("Selected gui.tempStepNode->step.stepDetails->stepSourceDropDownList %d", sd->data.source);
    }
}

/** Handle FOCUSED event: open roller popup for minutes/seconds */
static void step_detail_open_roller(stepNode *sn, lv_obj_t *focusedWidget) {
    /* Guard: prevent double-fire of FOCUSED event from creating two popups */
    if (gui.element.rollerPopup.mBoxRollerParent != NULL) return;

    sStepDetail *sd = sn->step.stepDetails;

    if (focusedWidget == sd->stepDetailMinTextArea) {
        LV_LOG_USER("Set minutes");
        rollerPopupCreate(gui.element.rollerPopup.minutesOptions,
                          setMinutesPopupTitle_text,
                          sd->stepDetailMinTextArea,
                          findRollerStringIndex(
                              lv_textarea_get_text(sd->stepDetailMinTextArea),
                              gui.element.rollerPopup.minutesOptions));
    }
    if (focusedWidget == sd->stepDetailSecTextArea) {
        LV_LOG_USER("Set seconds");
        rollerPopupCreate(gui.element.rollerPopup.secondsOptions,
                          setSecondsPopupTitle_text,
                          sd->stepDetailSecTextArea,
                          findRollerStringIndex(
                              lv_textarea_get_text(sd->stepDetailSecTextArea),
                              gui.element.rollerPopup.secondsOptions));
    }
}

/** Handle REFRESH event: enable/disable Save button based on validation */
static void step_detail_refresh_save_button(stepNode *sn, lv_obj_t *obj) {
    sStepDetail *sd = sn->step.stepDetails;
    if (obj == sd->stepSaveButton) {
        if (step_detail_is_valid(sd)) {
            lv_obj_clear_state(sd->stepSaveButton, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(sd->stepSaveButton, LV_STATE_DISABLED);
        }
    }
}

/*--------------------------------------------------------------
 *  Main event dispatcher
 *
 *  Points 1/5 refactor: the stepNode* is passed as user_data in
 *  every non-DELETE event registration.  Widget identification
 *  uses lv_event_get_current_target() instead of user_data.
 *  The parent processNode is stored in sStepDetail->parentProcess.
 *
 *  DELETE events still use sStepDetail* as user_data (set during
 *  registration) to avoid stale-global issues.
 *-------------------------------------------------------------*/
void event_stepDetail(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * widget = lv_event_get_current_target(e);

  /*
   * LV_EVENT_DELETE uses the stored sStepDetail* context (passed as user_data
   * during registration) instead of the stepNode.  This avoids resetting the
   * wrong style if the global has already been reassigned.
   */
  if (code == LV_EVENT_DELETE) {
      sStepDetail *sd = (sStepDetail *)lv_event_get_user_data(e);
      if (sd != NULL) {
          LV_LOG_USER("Delete Styles");
          lv_style_reset(&sd->style_mBoxStepPopupTitleLine);
      }
      return;
  }

  /* Context: stepNode* stored during event registration */
  stepNode *sn = (stepNode *)lv_event_get_user_data(e);
  if (sn == NULL || sn->step.stepDetails == NULL) return;

  sStepDetail *sd = sn->step.stepDetails;

  if (code == LV_EVENT_CLICKED) {
      if (widget == sd->stepSaveButton)
          step_detail_save(sn, sd->parentProcess);
      if (widget == sd->stepCancelButton)
          step_detail_cancel(sn);
  }

  if (code == LV_EVENT_VALUE_CHANGED) {
      step_detail_handle_value_changed(sn, widget);
  }

  if (code == LV_EVENT_FOCUSED) {
      step_detail_open_roller(sn, widget);
  }

  if (code == LV_EVENT_REFRESH) {
      step_detail_refresh_save_button(sn, widget);
  }
}

/*********************
*    STEP DETAIL
*********************/

void stepDetail(processNode * referenceNode, stepNode * currentNode)
{
/*********************
  *    PAGE ELEMENTS
*********************/

      char formatted_string[20];

      stepNode* existingStep = (stepNode*)isNodeInList((void*)&(referenceNode->process.processDetails->stepElementsList), currentNode, STEP_NODE);
      if(existingStep != NULL) {
          LV_LOG_USER("Step already exist with address 0x%p", currentNode);
          gui.tempStepNode = existingStep; /* Use existing node instead of allocating a new one */
          gui.tempStepNode->step.stepDetails->data.isEditMode = true;

      } else {
          gui.tempStepNode = (stepNode*)allocateAndInitializeNode(STEP_NODE);
          if (gui.tempStepNode == NULL) {
              LV_LOG_USER("Failed to allocate tempStepNode");
              return;
          }
          gui.tempStepNode->step.stepDetails->data.isEditMode = false;
          LV_LOG_USER("New stepNode created at address 0x%p", gui.tempStepNode);
      }

      /* Store parent process reference for context-based event handling */
      gui.tempStepNode->step.stepDetails->parentProcess = referenceNode;

      /* Local aliases — gui.tempStepNode is kept in sync for event_keyboard compat */
      stepNode *sn = gui.tempStepNode;
      sStepDetail *sd = sn->step.stepDetails;

      LV_LOG_USER("Step detail creation");

      sd->stepDetailParent = lv_obj_class_create_obj(&lv_msgbox_backdrop_class, lv_layer_top());
      lv_obj_class_init_obj(sd->stepDetailParent);
      lv_obj_remove_flag(sd->stepDetailParent, LV_OBJ_FLAG_IGNORE_LAYOUT);
      lv_obj_set_size(sd->stepDetailParent, LV_PCT(100), LV_PCT(100));


            sd->stepDetailContainer = lv_obj_create(sd->stepDetailParent);
            lv_obj_align(sd->stepDetailContainer, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_size(sd->stepDetailContainer, 350, 300);
            lv_obj_remove_flag(sd->stepDetailContainer, LV_OBJ_FLAG_SCROLLABLE);

                    sd->stepDetailLabel = lv_label_create(sd->stepDetailContainer);
                    lv_label_set_text(sd->stepDetailLabel, stepDetailTitle_text);
                    lv_obj_set_style_text_font(sd->stepDetailLabel, &lv_font_montserrat_22, 0);
                    lv_obj_align(sd->stepDetailLabel, LV_ALIGN_TOP_MID, 0, - 10);


                          lv_style_init(&sd->style_mBoxStepPopupTitleLine);
                          lv_style_set_line_width(&sd->style_mBoxStepPopupTitleLine, 2);
                          lv_style_set_line_color(&sd->style_mBoxStepPopupTitleLine, lv_palette_main(LV_PALETTE_GREEN));
                          lv_style_set_line_rounded(&sd->style_mBoxStepPopupTitleLine, true);


                    sd->mBoxStepPopupTitleLine = lv_line_create(sd->stepDetailContainer);
                    sd->titleLinePoints[1].x = 310;
                    lv_line_set_points(sd->mBoxStepPopupTitleLine, sd->titleLinePoints, 2);
                    lv_obj_add_style(sd->mBoxStepPopupTitleLine, &sd->style_mBoxStepPopupTitleLine, 0);
                    lv_obj_align(sd->mBoxStepPopupTitleLine, LV_ALIGN_TOP_MID, 0, 23);


            sd->stepDetailNameContainer = lv_obj_create(sd->stepDetailContainer);
            lv_obj_remove_flag(sd->stepDetailNameContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(sd->stepDetailNameContainer, LV_ALIGN_TOP_LEFT, -15, 30);
            lv_obj_set_size(sd->stepDetailNameContainer, 325, 40);
            lv_obj_set_style_border_opa(sd->stepDetailNameContainer, LV_OPA_TRANSP, 0);

                  sd->stepDetailNamelLabel = lv_label_create(sd->stepDetailNameContainer);
                  lv_label_set_text(sd->stepDetailNamelLabel, stepDetailName_text);
                  lv_obj_set_style_text_font(sd->stepDetailNamelLabel, &lv_font_montserrat_22, 0);
                  lv_obj_align(sd->stepDetailNamelLabel, LV_ALIGN_LEFT_MID, -10, 0);

                  sd->stepDetailNameTextArea = lv_textarea_create(sd->stepDetailNameContainer);
                  lv_textarea_set_one_line(sd->stepDetailNameTextArea, true);
                  lv_textarea_set_placeholder_text(sd->stepDetailNameTextArea, stepDetailPlaceHolder_text);
                  lv_obj_align(sd->stepDetailNameTextArea, LV_ALIGN_LEFT_MID, 70, 0);
                  lv_obj_set_width(sd->stepDetailNameTextArea, 210);
                  lv_obj_add_event_cb(sd->stepDetailNameTextArea, event_keyboard, LV_EVENT_CLICKED, NULL);
                  lv_obj_add_event_cb(sd->stepDetailNameTextArea, event_keyboard, LV_EVENT_DEFOCUSED, NULL);
                  lv_obj_add_event_cb(sd->stepDetailNameTextArea, event_keyboard, LV_EVENT_CANCEL, NULL);
                  lv_obj_add_event_cb(sd->stepDetailNameTextArea, event_keyboard, LV_EVENT_READY, NULL);
                  lv_obj_add_state(sd->stepDetailNameTextArea, LV_STATE_FOCUSED);
                  lv_obj_set_style_bg_color(sd->stepDetailNameTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
                  lv_obj_set_style_border_color(sd->stepDetailNameTextArea, lv_color_hex(WHITE), 0);
                  lv_textarea_set_max_length(sd->stepDetailNameTextArea, MAX_PROC_NAME_LEN);
                  lv_textarea_set_text(sd->stepDetailNameTextArea, sd->data.stepNameString);



            sd->stepDurationContainer = lv_obj_create(sd->stepDetailContainer);
            lv_obj_remove_flag(sd->stepDurationContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(sd->stepDurationContainer, LV_ALIGN_TOP_LEFT, -15, 70);
            lv_obj_set_size(sd->stepDurationContainer, 325, 40);
            lv_obj_set_style_border_opa(sd->stepDurationContainer, LV_OPA_TRANSP, 0);

                  sd->stepDurationLabel = lv_label_create(sd->stepDurationContainer);
                  lv_label_set_text(sd->stepDurationLabel, stepDetailDuration_text);
                  lv_obj_set_style_text_font(sd->stepDurationLabel, &lv_font_montserrat_22, 0);
                  lv_obj_align(sd->stepDurationLabel, LV_ALIGN_LEFT_MID, -10, 0);

                  sd->stepDetailMinTextArea = lv_textarea_create(sd->stepDurationContainer);
                  lv_textarea_set_one_line(sd->stepDetailMinTextArea, true);
                  lv_textarea_set_placeholder_text(sd->stepDetailMinTextArea, stepDetailDurationMinPlaceHolder_text);
                  lv_obj_align(sd->stepDetailMinTextArea, LV_ALIGN_LEFT_MID, 100, 0);
                  lv_obj_set_width(sd->stepDetailMinTextArea, 60);

                  lv_obj_add_event_cb(sd->stepDetailMinTextArea, event_stepDetail, LV_EVENT_CLICKED, sn);
                  lv_obj_add_event_cb(sd->stepDetailMinTextArea, event_stepDetail, LV_EVENT_VALUE_CHANGED, sn);
                  lv_obj_add_event_cb(sd->stepDetailMinTextArea, event_stepDetail, LV_EVENT_REFRESH, sn);
                  lv_obj_add_event_cb(sd->stepDetailMinTextArea, event_stepDetail, LV_EVENT_DELETE, sd);
                  lv_obj_add_event_cb(sd->stepDetailMinTextArea, event_stepDetail, LV_EVENT_FOCUSED, sn);
                  lv_obj_set_style_bg_color(sd->stepDetailMinTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
                  lv_obj_set_style_text_align(sd->stepDetailMinTextArea , LV_TEXT_ALIGN_CENTER, 0);
                  lv_obj_set_style_border_color(sd->stepDetailMinTextArea, lv_color_hex(WHITE), 0);
                  lv_snprintf(formatted_string, sizeof(formatted_string), "%d", sd->data.timeMins);
                  lv_textarea_set_text(sd->stepDetailMinTextArea, formatted_string);


                  sd->stepDetailSecTextArea = lv_textarea_create(sd->stepDurationContainer);
//                  lv_textarea_set_cursor_hidden(sd->stepDetailSecTextArea, true);
                  lv_textarea_set_one_line(sd->stepDetailSecTextArea, true);
                  lv_textarea_set_placeholder_text(sd->stepDetailSecTextArea, stepDetailDurationSecPlaceHolder_text);
                  lv_obj_align(sd->stepDetailSecTextArea, LV_ALIGN_LEFT_MID, 187, 0);
                  lv_obj_set_width(sd->stepDetailSecTextArea, 60);

                  lv_obj_add_event_cb(sd->stepDetailSecTextArea, event_stepDetail, LV_EVENT_CLICKED, sn);
                  lv_obj_add_event_cb(sd->stepDetailSecTextArea, event_stepDetail, LV_EVENT_VALUE_CHANGED, sn);
                  lv_obj_add_event_cb(sd->stepDetailSecTextArea, event_stepDetail, LV_EVENT_REFRESH, sn);
                  lv_obj_add_event_cb(sd->stepDetailSecTextArea, event_stepDetail, LV_EVENT_DELETE, sd);
                  lv_obj_add_event_cb(sd->stepDetailSecTextArea, event_stepDetail, LV_EVENT_FOCUSED, sn);
                  lv_obj_set_style_bg_color(sd->stepDetailSecTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
                  lv_obj_set_style_text_align(sd->stepDetailSecTextArea , LV_TEXT_ALIGN_CENTER, 0);
                  lv_obj_set_style_border_color(sd->stepDetailSecTextArea, lv_color_hex(WHITE), 0);
                  lv_snprintf(formatted_string, sizeof(formatted_string), "%d", sd->data.timeSecs);
                  lv_textarea_set_text(sd->stepDetailSecTextArea, formatted_string);



            sd->stepTypeContainer = lv_obj_create(sd->stepDetailContainer);
            lv_obj_remove_flag(sd->stepTypeContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(sd->stepTypeContainer, LV_ALIGN_TOP_LEFT, -15, 110);
            lv_obj_set_size(sd->stepTypeContainer, 325, 40);
            lv_obj_set_style_border_opa(sd->stepTypeContainer, LV_OPA_TRANSP, 0);


                  sd->stepTypeLabel = lv_label_create(sd->stepTypeContainer);
                  lv_label_set_text(sd->stepTypeLabel, stepDetailType_text);
                  lv_obj_set_style_text_font(sd->stepTypeLabel, &lv_font_montserrat_22, 0);
                  lv_obj_align(sd->stepTypeLabel, LV_ALIGN_LEFT_MID, -10, 0);


                  sd->stepTypeDropDownList = lv_dropdown_create(sd->stepTypeContainer);
                  lv_obj_set_style_border_opa(sd->stepTypeDropDownList, LV_OPA_TRANSP, 0);
                  lv_dropdown_set_options(sd->stepTypeDropDownList, stepTypeList);
                  lv_obj_align(sd->stepTypeDropDownList, LV_ALIGN_LEFT_MID, 50, 2);
                  lv_obj_set_size(sd->stepTypeDropDownList, 165, 50);
                  lv_obj_add_event_cb(sd->stepTypeDropDownList, event_stepDetail, LV_EVENT_VALUE_CHANGED, sn);
                  lv_obj_set_style_bg_color(lv_dropdown_get_list(sd->stepTypeDropDownList), lv_palette_main(LV_PALETTE_GREEN), LV_PART_SELECTED | LV_STATE_CHECKED);
                  lv_dropdown_set_selected(sd->stepTypeDropDownList, sd->data.type);

                  sd->stepTypeHelpIcon = lv_label_create(sd->stepTypeContainer);
                  lv_label_set_text(sd->stepTypeHelpIcon, chemical_icon);
                  lv_obj_set_style_text_font(sd->stepTypeHelpIcon, &FilMachineFontIcons_20, 0);
                  lv_obj_align(sd->stepTypeHelpIcon, LV_ALIGN_LEFT_MID, 210, 0);



            sd->stepSourceContainer = lv_obj_create(sd->stepDetailContainer);
            lv_obj_remove_flag(sd->stepSourceContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(sd->stepSourceContainer, LV_ALIGN_TOP_LEFT, -15, 150);
            lv_obj_set_size(sd->stepSourceContainer, 340, 40);
            lv_obj_set_style_border_opa(sd->stepSourceContainer, LV_OPA_TRANSP, 0);


                  sd->stepSourceLabel = lv_label_create(sd->stepSourceContainer);
                  lv_label_set_text(sd->stepSourceLabel, stepDetailSource_text);
                  lv_obj_set_style_text_font(sd->stepSourceLabel, &lv_font_montserrat_22, 0);
                  lv_obj_align(sd->stepSourceLabel, LV_ALIGN_LEFT_MID, -10, 0);


                  sd->stepSourceDropDownList = lv_dropdown_create(sd->stepSourceContainer);
                  lv_obj_set_style_border_opa(sd->stepSourceDropDownList, LV_OPA_TRANSP, 0);
                  lv_dropdown_set_options(sd->stepSourceDropDownList, stepSourceList);
                  lv_obj_align(sd->stepSourceDropDownList, LV_ALIGN_LEFT_MID, 71, 2);
                  lv_obj_set_size(sd->stepSourceDropDownList, 83, 50);
                  lv_obj_add_event_cb(sd->stepSourceDropDownList, event_stepDetail, LV_EVENT_VALUE_CHANGED, sn);
                  lv_obj_set_style_bg_color(lv_dropdown_get_list(sd->stepSourceDropDownList), lv_palette_main(LV_PALETTE_GREEN), LV_PART_SELECTED | LV_STATE_CHECKED);
                  lv_dropdown_set_selected(sd->stepSourceDropDownList, sd->data.source);

                  sd->stepSourceTempLabel = lv_label_create(sd->stepSourceContainer);
                  lv_label_set_text(sd->stepSourceTempLabel, stepDetailCurrentTemp_text);
                  lv_obj_set_style_text_font(sd->stepSourceTempLabel, &lv_font_montserrat_22, 0);
                  lv_obj_align(sd->stepSourceTempLabel, LV_ALIGN_LEFT_MID, 165, 0);

                  sd->stepSourceTempHelpIcon = lv_label_create(sd->stepSourceContainer);
                  lv_label_set_text(sd->stepSourceTempHelpIcon, temp_icon);
                  lv_obj_set_style_text_font(sd->stepSourceTempHelpIcon, &FilMachineFontIcons_20, 0);
                  lv_obj_align(sd->stepSourceTempHelpIcon, LV_ALIGN_LEFT_MID, 227, 0);

                  sd->stepSourceTempValue = lv_label_create(sd->stepSourceContainer);
                  {
                      char tempBuf[16];
                      float chemTemp = sim_getTemperature(TEMPERATURE_SENSOR_CHEMICAL);
                      if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
                          snprintf(tempBuf, sizeof(tempBuf), "%.1f°C", chemTemp);
                      } else {
                          snprintf(tempBuf, sizeof(tempBuf), "%.1f°F", chemTemp * 1.8f + 32.0f);
                      }
                      lv_label_set_text(sd->stepSourceTempValue, tempBuf);
                  }
                  lv_obj_set_style_text_font(sd->stepSourceTempValue, &lv_font_montserrat_22, 0);
                  lv_obj_align(sd->stepSourceTempValue, LV_ALIGN_LEFT_MID, 245, 1);


            sd->stepDiscardAfterContainer = lv_obj_create(sd->stepDetailContainer);
            lv_obj_remove_flag(sd->stepDiscardAfterContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(sd->stepDiscardAfterContainer, LV_ALIGN_TOP_LEFT, -15, 190);
            lv_obj_set_size(sd->stepDiscardAfterContainer, 315, 40);
            lv_obj_set_style_border_opa(sd->stepDiscardAfterContainer, LV_OPA_TRANSP, 0);


                  sd->stepDiscardAfterLabel = lv_label_create(sd->stepDiscardAfterContainer);
                  lv_label_set_text(sd->stepDiscardAfterLabel, stepDetailDiscardAfter_text);
                  lv_obj_set_style_text_font(sd->stepDiscardAfterLabel, &lv_font_montserrat_22, 0);
                  lv_obj_align(sd->stepDiscardAfterLabel, LV_ALIGN_LEFT_MID, -10, 0);


                  sd->stepDiscardAfterSwitch = lv_switch_create(sd->stepDiscardAfterContainer);
                  lv_obj_add_event_cb(sd->stepDiscardAfterSwitch, event_stepDetail, LV_EVENT_VALUE_CHANGED, sn);
                  lv_obj_align(sd->stepDiscardAfterSwitch, LV_ALIGN_LEFT_MID, 140, 0);
                  lv_obj_set_style_bg_color(sd->stepDiscardAfterSwitch, lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
                  lv_obj_set_style_bg_color(sd->stepDiscardAfterSwitch,  lv_palette_main(LV_PALETTE_GREEN), LV_PART_KNOB | LV_STATE_DEFAULT);
                  lv_obj_set_style_bg_color(sd->stepDiscardAfterSwitch, lv_color_hex(GREEN_DARK) , LV_PART_INDICATOR | LV_STATE_CHECKED);
                  lv_obj_add_state(sd->stepDiscardAfterSwitch, sd->data.discardAfterProc);


      sd->stepSaveButton = lv_button_create(sd->stepDetailContainer);
      lv_obj_set_size(sd->stepSaveButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
      lv_obj_align(sd->stepSaveButton, LV_ALIGN_BOTTOM_LEFT, 10 , 10);
      lv_obj_add_event_cb(sd->stepSaveButton, event_stepDetail, LV_EVENT_CLICKED, sn);
      lv_obj_add_event_cb(sd->stepSaveButton, event_stepDetail, LV_EVENT_REFRESH, sn);
      lv_obj_set_style_bg_color(sd->stepSaveButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
      lv_obj_add_state(sd->stepSaveButton, LV_STATE_DISABLED);

          sd->stepSaveLabel = lv_label_create(sd->stepSaveButton);
          lv_label_set_text(sd->stepSaveLabel, stepDetailSave_text);
          lv_obj_set_style_text_font(sd->stepSaveLabel, &lv_font_montserrat_22, 0);
          lv_obj_align(sd->stepSaveLabel, LV_ALIGN_CENTER, 0, 0);


      sd->stepCancelButton = lv_button_create(sd->stepDetailContainer);
      lv_obj_set_size(sd->stepCancelButton, BUTTON_PROCESS_WIDTH, BUTTON_PROCESS_HEIGHT);
      lv_obj_align(sd->stepCancelButton, LV_ALIGN_BOTTOM_RIGHT, - 10 , 10);
      lv_obj_add_event_cb(sd->stepCancelButton, event_stepDetail, LV_EVENT_CLICKED, sn);
      lv_obj_set_style_bg_color(sd->stepCancelButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);

            sd->stepCancelLabel = lv_label_create(sd->stepCancelButton);
            lv_label_set_text(sd->stepCancelLabel, stepDetailCancel_text);
            lv_obj_set_style_text_font(sd->stepCancelLabel, &lv_font_montserrat_22, 0);
            lv_obj_align(sd->stepCancelLabel, LV_ALIGN_CENTER, 0, 0);

}

