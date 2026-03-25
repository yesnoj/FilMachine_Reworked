
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
        sd->minRollerCtx.textArea = sd->stepDetailMinTextArea;
        sd->minRollerCtx.saveButton = sd->stepSaveButton;
        rollerPopupCreate(gui.element.rollerPopup.minutesOptions,
                          setMinutesPopupTitle_text,
                          &sd->minRollerCtx,
                          findRollerStringIndex(
                              lv_textarea_get_text(sd->stepDetailMinTextArea),
                              gui.element.rollerPopup.minutesOptions), GREEN_DARK);
    }
    if (focusedWidget == sd->stepDetailSecTextArea) {
        LV_LOG_USER("Set seconds");
        sd->secRollerCtx.textArea = sd->stepDetailSecTextArea;
        sd->secRollerCtx.saveButton = sd->stepSaveButton;
        rollerPopupCreate(gui.element.rollerPopup.secondsOptions,
                          setSecondsPopupTitle_text,
                          &sd->secRollerCtx,
                          findRollerStringIndex(
                              lv_textarea_get_text(sd->stepDetailSecTextArea),
                              gui.element.rollerPopup.secondsOptions), GREEN_DARK);
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

      /* Local aliases */
      stepNode *sn = gui.tempStepNode;
      sStepDetail *sd = sn->step.stepDetails;
      const ui_step_detail_layout_t *ui = &ui_get_profile()->step_detail;

      LV_LOG_USER("Step detail creation");

      sd->stepDetailParent = lv_obj_class_create_obj(&lv_msgbox_backdrop_class, lv_layer_top());
      lv_obj_class_init_obj(sd->stepDetailParent);
      lv_obj_remove_flag(sd->stepDetailParent, LV_OBJ_FLAG_IGNORE_LAYOUT);
      lv_obj_set_size(sd->stepDetailParent, LV_PCT(100), LV_PCT(100));

      memset(&sd->nameKeyboardCtx, 0, sizeof(sd->nameKeyboardCtx));
      sd->nameKeyboardCtx.owner = KB_OWNER_STEP;
      sd->nameKeyboardCtx.parentScreen = sd->stepDetailParent;
      sd->nameKeyboardCtx.ownerData = sd;
      memset(&sd->minRollerCtx, 0, sizeof(sd->minRollerCtx));
      sd->minRollerCtx.owner = ROLLER_OWNER_STEP_MIN;
      sd->minRollerCtx.ownerData = sd;
      memset(&sd->secRollerCtx, 0, sizeof(sd->secRollerCtx));
      sd->secRollerCtx.owner = ROLLER_OWNER_STEP_SEC;
      sd->secRollerCtx.ownerData = sd;


            sd->stepDetailContainer = lv_obj_create(sd->stepDetailParent);
            lv_obj_align(sd->stepDetailContainer, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_size(sd->stepDetailContainer, ui->modal_w, ui->modal_h);
            lv_obj_remove_flag(sd->stepDetailContainer, LV_OBJ_FLAG_SCROLLABLE);

                    sd->stepDetailLabel = lv_label_create(sd->stepDetailContainer);
                    lv_label_set_text(sd->stepDetailLabel, stepDetailTitle_text);
                    lv_obj_set_style_text_font(sd->stepDetailLabel, ui->title_font, 0);
                    lv_obj_align(sd->stepDetailLabel, LV_ALIGN_TOP_MID, 0, ui->title_y);


                          lv_style_init(&sd->style_mBoxStepPopupTitleLine);
                          lv_style_set_line_width(&sd->style_mBoxStepPopupTitleLine, ui_get_profile()->title_line_width);
                          lv_style_set_line_color(&sd->style_mBoxStepPopupTitleLine, lv_palette_main(LV_PALETTE_GREEN));
                          lv_style_set_line_rounded(&sd->style_mBoxStepPopupTitleLine, true);


                    sd->mBoxStepPopupTitleLine = lv_line_create(sd->stepDetailContainer);
                    sd->titleLinePoints[1].x = ui->title_line_w;
                    lv_line_set_points(sd->mBoxStepPopupTitleLine, sd->titleLinePoints, 2);
                    lv_obj_add_style(sd->mBoxStepPopupTitleLine, &sd->style_mBoxStepPopupTitleLine, 0);
                    lv_obj_align(sd->mBoxStepPopupTitleLine, LV_ALIGN_TOP_MID, 0, ui->title_line_y);


            sd->stepDetailNameContainer = lv_obj_create(sd->stepDetailContainer);
            lv_obj_remove_flag(sd->stepDetailNameContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(sd->stepDetailNameContainer, LV_ALIGN_TOP_LEFT, ui->form_row_x, ui->name_y);
            lv_obj_set_size(sd->stepDetailNameContainer, ui->form_row_w, ui->form_row_h);
            lv_obj_set_style_border_opa(sd->stepDetailNameContainer, LV_OPA_TRANSP, 0);

                  sd->stepDetailNamelLabel = lv_label_create(sd->stepDetailNameContainer);
                  lv_label_set_text(sd->stepDetailNamelLabel, stepDetailName_text);
                  lv_obj_set_style_text_font(sd->stepDetailNamelLabel, ui->label_font, 0);
                  lv_obj_align(sd->stepDetailNamelLabel, LV_ALIGN_LEFT_MID, ui->row_label_x, 0);

                  sd->stepDetailNameTextArea = lv_textarea_create(sd->stepDetailNameContainer);
                  lv_textarea_set_one_line(sd->stepDetailNameTextArea, true);
                  lv_textarea_set_placeholder_text(sd->stepDetailNameTextArea, stepDetailPlaceHolder_text);
                  lv_obj_align(sd->stepDetailNameTextArea, LV_ALIGN_LEFT_MID, ui->name_textarea_x, 0);
                  lv_obj_set_width(sd->stepDetailNameTextArea, ui->name_textarea_w);
                  lv_obj_set_style_text_font(sd->stepDetailNameTextArea, ui->value_font, 0);
                  sd->nameKeyboardCtx.textArea = sd->stepDetailNameTextArea;
                  lv_obj_add_event_cb(sd->stepDetailNameTextArea, event_keyboard, LV_EVENT_CLICKED, &sd->nameKeyboardCtx);
                  lv_obj_add_event_cb(sd->stepDetailNameTextArea, event_keyboard, LV_EVENT_DEFOCUSED, &sd->nameKeyboardCtx);
                  lv_obj_add_event_cb(sd->stepDetailNameTextArea, event_keyboard, LV_EVENT_CANCEL, &sd->nameKeyboardCtx);
                  lv_obj_add_event_cb(sd->stepDetailNameTextArea, event_keyboard, LV_EVENT_READY, &sd->nameKeyboardCtx);
                  lv_obj_add_state(sd->stepDetailNameTextArea, LV_STATE_FOCUSED);
                  lv_obj_set_style_bg_color(sd->stepDetailNameTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
                  lv_obj_set_style_border_color(sd->stepDetailNameTextArea, lv_color_hex(WHITE), 0);
                  lv_textarea_set_max_length(sd->stepDetailNameTextArea, MAX_PROC_NAME_LEN);
                  lv_textarea_set_text(sd->stepDetailNameTextArea, sd->data.stepNameString);



            sd->stepDurationContainer = lv_obj_create(sd->stepDetailContainer);
            lv_obj_remove_flag(sd->stepDurationContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(sd->stepDurationContainer, LV_ALIGN_TOP_LEFT, ui->form_row_x, ui->duration_y);
            lv_obj_set_size(sd->stepDurationContainer, ui->form_row_w, ui->form_row_h);
            lv_obj_set_style_border_opa(sd->stepDurationContainer, LV_OPA_TRANSP, 0);

                  sd->stepDurationLabel = lv_label_create(sd->stepDurationContainer);
                  lv_label_set_text(sd->stepDurationLabel, stepDetailDuration_text);
                  lv_obj_set_style_text_font(sd->stepDurationLabel, ui->label_font, 0);
                  lv_obj_align(sd->stepDurationLabel, LV_ALIGN_LEFT_MID, ui->row_label_x, 0);

                  sd->stepDetailMinTextArea = lv_textarea_create(sd->stepDurationContainer);
                  lv_textarea_set_one_line(sd->stepDetailMinTextArea, true);
                  lv_textarea_set_placeholder_text(sd->stepDetailMinTextArea, stepDetailDurationMinPlaceHolder_text);
                  lv_obj_align(sd->stepDetailMinTextArea, LV_ALIGN_LEFT_MID, ui->minutes_textarea_x, 0);
                  lv_obj_set_width(sd->stepDetailMinTextArea, ui->time_textarea_w);
                  lv_obj_set_style_text_font(sd->stepDetailMinTextArea, ui->value_font, 0);

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
                  lv_obj_align(sd->stepDetailSecTextArea, LV_ALIGN_LEFT_MID, ui->seconds_textarea_x, 0);
                  lv_obj_set_width(sd->stepDetailSecTextArea, ui->time_textarea_w);
                  lv_obj_set_style_text_font(sd->stepDetailSecTextArea, ui->value_font, 0);

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
            lv_obj_align(sd->stepTypeContainer, LV_ALIGN_TOP_LEFT, ui->form_row_x, ui->type_y);
            lv_obj_set_size(sd->stepTypeContainer, ui->form_row_w, ui->form_row_h);
            lv_obj_set_style_border_opa(sd->stepTypeContainer, LV_OPA_TRANSP, 0);


                  sd->stepTypeLabel = lv_label_create(sd->stepTypeContainer);
                  lv_label_set_text(sd->stepTypeLabel, stepDetailType_text);
                  lv_obj_set_style_text_font(sd->stepTypeLabel, ui->label_font, 0);
                  lv_obj_align(sd->stepTypeLabel, LV_ALIGN_LEFT_MID, ui->row_label_x, 0);


                  sd->stepTypeDropDownList = lv_dropdown_create(sd->stepTypeContainer);
                  lv_obj_set_style_border_opa(sd->stepTypeDropDownList, LV_OPA_TRANSP, 0);
                  lv_dropdown_set_options(sd->stepTypeDropDownList, stepTypeList);
                  lv_obj_align(sd->stepTypeDropDownList, LV_ALIGN_LEFT_MID, ui->type_dropdown_x, ui->type_dropdown_y);
                  lv_obj_set_size(sd->stepTypeDropDownList, ui->type_dropdown_w, ui->dropdown_list_h);
                  lv_obj_set_style_text_font(sd->stepTypeDropDownList, ui->value_font, 0);
                  lv_obj_set_style_text_font(lv_dropdown_get_list(sd->stepTypeDropDownList), ui->value_font, 0);
                  lv_obj_add_event_cb(sd->stepTypeDropDownList, event_stepDetail, LV_EVENT_VALUE_CHANGED, sn);
                  lv_obj_set_style_bg_color(lv_dropdown_get_list(sd->stepTypeDropDownList), lv_palette_main(LV_PALETTE_GREEN), LV_PART_SELECTED | LV_STATE_CHECKED);
                  lv_dropdown_set_selected(sd->stepTypeDropDownList, sd->data.type);

                  sd->stepTypeHelpIcon = lv_label_create(sd->stepTypeContainer);
                  lv_label_set_text(sd->stepTypeHelpIcon, chemical_icon);
                  lv_obj_set_style_text_font(sd->stepTypeHelpIcon, ui->info_icon_font, 0);
                  lv_obj_align(sd->stepTypeHelpIcon, LV_ALIGN_LEFT_MID, ui->type_icon_x, 0);



            sd->stepSourceContainer = lv_obj_create(sd->stepDetailContainer);
            lv_obj_remove_flag(sd->stepSourceContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(sd->stepSourceContainer, LV_ALIGN_TOP_LEFT, ui->form_row_x, ui->source_y);
            lv_obj_set_size(sd->stepSourceContainer, ui->form_row_w + ui->source_row_extra_w, ui->form_row_h);
            lv_obj_set_style_border_opa(sd->stepSourceContainer, LV_OPA_TRANSP, 0);


                  sd->stepSourceLabel = lv_label_create(sd->stepSourceContainer);
                  lv_label_set_text(sd->stepSourceLabel, stepDetailSource_text);
                  lv_obj_set_style_text_font(sd->stepSourceLabel, ui->label_font, 0);
                  lv_obj_align(sd->stepSourceLabel, LV_ALIGN_LEFT_MID, ui->row_label_x, 0);


                  sd->stepSourceDropDownList = lv_dropdown_create(sd->stepSourceContainer);
                  lv_obj_set_style_border_opa(sd->stepSourceDropDownList, LV_OPA_TRANSP, 0);
                  lv_dropdown_set_options(sd->stepSourceDropDownList, stepSourceList);
                  lv_obj_align(sd->stepSourceDropDownList, LV_ALIGN_LEFT_MID, ui->source_dropdown_x, ui->source_dropdown_y);
                  lv_obj_set_size(sd->stepSourceDropDownList, ui->source_dropdown_w, ui->dropdown_list_h);
                  lv_obj_set_style_text_font(sd->stepSourceDropDownList, ui->value_font, 0);
                  lv_obj_set_style_text_font(lv_dropdown_get_list(sd->stepSourceDropDownList), ui->value_font, 0);
                  lv_obj_add_event_cb(sd->stepSourceDropDownList, event_stepDetail, LV_EVENT_VALUE_CHANGED, sn);
                  lv_obj_set_style_bg_color(lv_dropdown_get_list(sd->stepSourceDropDownList), lv_palette_main(LV_PALETTE_GREEN), LV_PART_SELECTED | LV_STATE_CHECKED);
                  lv_dropdown_set_selected(sd->stepSourceDropDownList, sd->data.source);

                  sd->stepSourceTempLabel = lv_label_create(sd->stepSourceContainer);
                  lv_label_set_text(sd->stepSourceTempLabel, stepDetailCurrentTemp_text);
                  lv_obj_set_style_text_font(sd->stepSourceTempLabel, ui->value_font, 0);
                  lv_obj_align(sd->stepSourceTempLabel, LV_ALIGN_LEFT_MID, ui->source_temp_label_x, 0);

                  sd->stepSourceTempHelpIcon = lv_label_create(sd->stepSourceContainer);
                  lv_label_set_text(sd->stepSourceTempHelpIcon, temp_icon);
                  lv_obj_set_style_text_font(sd->stepSourceTempHelpIcon, ui->info_icon_font, 0);
                  lv_obj_align(sd->stepSourceTempHelpIcon, LV_ALIGN_LEFT_MID, ui->source_temp_icon_x, 0);

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
                  lv_obj_set_style_text_font(sd->stepSourceTempValue, ui->value_font, 0);
                  lv_obj_align(sd->stepSourceTempValue, LV_ALIGN_LEFT_MID, ui->source_temp_value_x, ui->source_temp_value_y);


            sd->stepDiscardAfterContainer = lv_obj_create(sd->stepDetailContainer);
            lv_obj_remove_flag(sd->stepDiscardAfterContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(sd->stepDiscardAfterContainer, LV_ALIGN_TOP_LEFT, ui->form_row_x, ui->discard_y);
            lv_obj_set_size(sd->stepDiscardAfterContainer, ui->form_row_w - ui->source_row_extra_w, ui->form_row_h);
            lv_obj_set_style_border_opa(sd->stepDiscardAfterContainer, LV_OPA_TRANSP, 0);


                  sd->stepDiscardAfterLabel = lv_label_create(sd->stepDiscardAfterContainer);
                  lv_label_set_text(sd->stepDiscardAfterLabel, stepDetailDiscardAfter_text);
                  lv_obj_set_style_text_font(sd->stepDiscardAfterLabel, ui->label_font, 0);
                  lv_obj_align(sd->stepDiscardAfterLabel, LV_ALIGN_LEFT_MID, ui->discard_label_x, 0);


                  sd->stepDiscardAfterSwitch = lv_switch_create(sd->stepDiscardAfterContainer);
                  lv_obj_set_size(sd->stepDiscardAfterSwitch, ui->discard_switch_w, ui->discard_switch_h);
                  lv_obj_add_event_cb(sd->stepDiscardAfterSwitch, event_stepDetail, LV_EVENT_VALUE_CHANGED, sn);
                  lv_obj_align(sd->stepDiscardAfterSwitch, LV_ALIGN_LEFT_MID, ui->discard_switch_x, 0);
                  lv_obj_set_style_bg_color(sd->stepDiscardAfterSwitch, lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
                  lv_obj_set_style_bg_color(sd->stepDiscardAfterSwitch,  lv_palette_main(LV_PALETTE_GREEN), LV_PART_KNOB | LV_STATE_DEFAULT);
                  lv_obj_set_style_bg_color(sd->stepDiscardAfterSwitch, lv_color_hex(GREEN_DARK) , LV_PART_INDICATOR | LV_STATE_CHECKED);
                  lv_obj_add_state(sd->stepDiscardAfterSwitch, sd->data.discardAfterProc);


      sd->stepSaveButton = lv_button_create(sd->stepDetailContainer);
      sd->nameKeyboardCtx.saveButton = sd->stepSaveButton;
      lv_obj_set_size(sd->stepSaveButton, ui->save_w, ui->save_h);
      lv_obj_align(sd->stepSaveButton, LV_ALIGN_BOTTOM_LEFT, ui->save_x, ui->save_y);
      lv_obj_add_event_cb(sd->stepSaveButton, event_stepDetail, LV_EVENT_CLICKED, sn);
      lv_obj_add_event_cb(sd->stepSaveButton, event_stepDetail, LV_EVENT_REFRESH, sn);
      lv_obj_set_style_bg_color(sd->stepSaveButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
      lv_obj_add_state(sd->stepSaveButton, LV_STATE_DISABLED);

          sd->stepSaveLabel = lv_label_create(sd->stepSaveButton);
          lv_label_set_text(sd->stepSaveLabel, stepDetailSave_text);
          lv_obj_set_style_text_font(sd->stepSaveLabel, ui->button_font, 0);
          lv_obj_align(sd->stepSaveLabel, LV_ALIGN_CENTER, 0, 0);


      sd->stepCancelButton = lv_button_create(sd->stepDetailContainer);
      lv_obj_set_size(sd->stepCancelButton, ui->cancel_w, ui->cancel_h);
      lv_obj_align(sd->stepCancelButton, LV_ALIGN_BOTTOM_RIGHT, ui->cancel_x, ui->cancel_y);
      lv_obj_add_event_cb(sd->stepCancelButton, event_stepDetail, LV_EVENT_CLICKED, sn);
      lv_obj_set_style_bg_color(sd->stepCancelButton, lv_color_hex(RED_DARK), LV_PART_MAIN);

            sd->stepCancelLabel = lv_label_create(sd->stepCancelButton);
            lv_label_set_text(sd->stepCancelLabel, stepDetailCancel_text);
            lv_obj_set_style_text_font(sd->stepCancelLabel, ui->button_font, 0);
            lv_obj_align(sd->stepCancelLabel, LV_ALIGN_CENTER, 0, 0);

}

