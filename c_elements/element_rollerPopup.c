/**
 * @file element_rollerPopup.c
 *
 */


//ESSENTIAL INCLUDES
#include "FilMachine.h"

extern struct gui_components gui;

//ACCESSORY INCLUDES

static uint32_t rollerSelected;
static bool isScrolled = false;
static char tempBuffer[24];

static void roller_popup_close(void)
{
  lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
  lv_style_reset(&gui.element.rollerPopup.style_roller);
  lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
  gui.element.rollerPopup.mBoxRollerParent = NULL;
}

static bool roller_apply_context(sRollerOwnerContext *ctx)
{
  if(ctx == NULL || ctx->ownerData == NULL || ctx->textArea == NULL) return false;
  switch(ctx->owner) {
    case ROLLER_OWNER_PROCESS_TEMP: {
      sProcessDetail *pd = (sProcessDetail *)ctx->ownerData;
      uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
      uint32_t newTemp = sel + TEMP_ROLLER_MIN;  /* roller index → actual °C */
      if (newTemp != pd->data.temp) {
          pd->data.temp = newTemp;
          if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP) snprintf(tempBuffer, sizeof(tempBuffer), "%lu", (unsigned long)pd->data.temp);
          else snprintf(tempBuffer, sizeof(tempBuffer), "%" PRIi32, convertCelsiusToFahrenheit(pd->data.temp));
          lv_textarea_set_text(ctx->textArea, tempBuffer);
          pd->data.somethingChanged = true;
          if(ctx->saveButton) lv_obj_send_event(ctx->saveButton, LV_EVENT_REFRESH, NULL);
      }
      isScrolled = false;
      return true;
    }
    case ROLLER_OWNER_PROCESS_TOLERANCE: {
      sProcessDetail *pd = (sProcessDetail *)ctx->ownerData;
      if(isScrolled) {
        float newTolerance = atof(tempBuffer);
        if (newTolerance != pd->data.tempTolerance) {
            pd->data.tempTolerance = newTolerance;
            char *tmp = getRollerStringIndex(rollerSelected, gui.element.rollerPopup.tempToleranceOptions);
            lv_textarea_set_text(ctx->textArea, tmp != NULL ? tmp : "");
            free(tmp);
            pd->data.somethingChanged = true;
            if(ctx->saveButton) lv_obj_send_event(ctx->saveButton, LV_EVENT_REFRESH, NULL);
        }
      }
      isScrolled = false;
      return true;
    }
    case ROLLER_OWNER_STEP_MIN: {
      sStepDetail *sd = (sStepDetail *)ctx->ownerData;
      uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
      if (sel != sd->data.timeMins) {
          sd->data.timeMins = sel;
          if(isScrolled) {
            char *tmp = getRollerStringIndex(sel, gui.element.rollerPopup.minutesOptions);
            lv_textarea_set_text(ctx->textArea, tmp != NULL ? tmp : "");
            free(tmp);
          } else { snprintf(tempBuffer, sizeof(tempBuffer), "%lu", (unsigned long)sel); lv_textarea_set_text(ctx->textArea, tempBuffer); }
          sd->data.somethingChanged = true;
          if(ctx->saveButton) lv_obj_send_event(ctx->saveButton, LV_EVENT_REFRESH, NULL);
      }
      isScrolled = false;
      return true;
    }
    case ROLLER_OWNER_STEP_SEC: {
      sStepDetail *sd = (sStepDetail *)ctx->ownerData;
      uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
      if (sel != sd->data.timeSecs) {
          sd->data.timeSecs = sel;
          if(isScrolled) {
            char *tmp = getRollerStringIndex(sel, gui.element.rollerPopup.secondsOptions);
            lv_textarea_set_text(ctx->textArea, tmp != NULL ? tmp : "");
            free(tmp);
          } else { snprintf(tempBuffer, sizeof(tempBuffer), "%lu", (unsigned long)sel); lv_textarea_set_text(ctx->textArea, tempBuffer); }
          sd->data.somethingChanged = true;
          if(ctx->saveButton) lv_obj_send_event(ctx->saveButton, LV_EVENT_REFRESH, NULL);
      }
      isScrolled = false;
      return true;
    }
    default: return false;
  }
}


void event_Roller(lv_event_t * e)
{
  
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t * objCont = (lv_obj_t *)lv_obj_get_parent(obj);
  lv_obj_t * mboxCont = (lv_obj_t *)lv_obj_get_parent(objCont);
  lv_obj_t * godFatherCont = (lv_obj_t *)lv_obj_get_parent(mboxCont);
  void * data = lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED){
        LV_LOG_USER("LV_EVENT_CLICKED");
        lv_obj_clear_state(gui.element.rollerPopup.mBoxRollerButton, LV_STATE_DISABLED);
 
        if(obj == gui.element.rollerPopup.mBoxRollerButton)
        {
            if (roller_apply_context((sRollerOwnerContext *)data)) {
              roller_popup_close();
              rollerSelected = 0;
              return;
            }
            if((lv_obj_t *)data == gui.page.settings.tempSensorTuneButton){
              LV_LOG_USER("SET BUTTON from tempSensorTuneButton value %"PRIu32":",rollerSelected);
              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(godFatherCont);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              gui.page.settings.settingsParams.calibratedTemp = rollerSelected + TEMP_ROLLER_MIN;

              /* Calculate calibration offset */
              float rawTemp = sim_getTemperature(TEMPERATURE_SENSOR_BATH);
              /* Remove any existing offset to get the true raw reading */
              float rawWithoutOffset = rawTemp - (gui.page.settings.settingsParams.tempCalibOffset / 10.0f);
              float newOffset = (float)gui.page.settings.settingsParams.calibratedTemp - rawWithoutOffset;
              gui.page.settings.settingsParams.tempCalibOffset = (int8_t)(newOffset * 10.0f);
              LV_LOG_USER("Calibration: entered=%u, raw=%.1f, offset=%.2f, offsetTenths=%d",
                         gui.page.settings.settingsParams.calibratedTemp, rawWithoutOffset, newOffset,
                         gui.page.settings.settingsParams.tempCalibOffset);

              qSysAction( SAVE_PROCESS_CONFIG );

              /* Show calibration result in a popup */
              {
                static char calMsg[80];
                float offsetVal = gui.page.settings.settingsParams.tempCalibOffset / 10.0f;
                snprintf(calMsg, sizeof(calMsg), "Calibration complete.\nOffset: %+.1f C", offsetVal);
                messagePopupCreate(calibrationPopupTitle_text, calMsg, NULL, NULL, NULL);
              }
              return;
            }
            if(gui.tempProcessNode != NULL && gui.tempProcessNode->process.processDetails != NULL
               && (lv_obj_t *)data == gui.tempProcessNode->process.processDetails->processTempTextArea){

              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              uint32_t newTemp;
              if(isScrolled == 0){
                  uint32_t idx = lv_roller_get_selected(gui.element.rollerPopup.roller);
                  newTemp = idx + TEMP_ROLLER_MIN;
                  LV_LOG_USER("SET BUTTON from processTempTextArea index %"PRIu32" value %"PRIu32":", idx, newTemp);
              } else {
                  newTemp = rollerSelected + TEMP_ROLLER_MIN;
                  LV_LOG_USER("SET BUTTON from processTempTextArea index %"PRIu32" value %"PRIu32":", rollerSelected, newTemp);
                  isScrolled = false;
              }

              if (newTemp != gui.tempProcessNode->process.processDetails->data.temp) {
                  gui.tempProcessNode->process.processDetails->data.temp = newTemp;
                  snprintf(tempBuffer, sizeof(tempBuffer), "%lu", (unsigned long)newTemp);
                  if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP) {
                    lv_textarea_set_text(gui.tempProcessNode->process.processDetails->processTempTextArea, tempBuffer);
                  } else {
                        sprintf(tempBuffer, "%"PRIi32"", convertCelsiusToFahrenheit(gui.tempProcessNode->process.processDetails->data.temp));
                        lv_textarea_set_text(gui.tempProcessNode->process.processDetails->processTempTextArea, tempBuffer);
                  }
                  gui.tempProcessNode->process.processDetails->data.somethingChanged = true;
                  lv_obj_send_event(gui.tempProcessNode->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL);
              }

              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(godFatherCont);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              return;
            }


            if(gui.tempProcessNode != NULL && gui.tempProcessNode->process.processDetails != NULL
               && (lv_obj_t *)data == gui.tempProcessNode->process.processDetails->processToleranceTextArea){
             lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);

              if(isScrolled == 1){
                float newTolerance = atof(tempBuffer);
                if (newTolerance != gui.tempProcessNode->process.processDetails->data.tempTolerance) {
                    gui.tempProcessNode->process.processDetails->data.tempTolerance = newTolerance;
                    LV_LOG_USER("SET BUTTON from processToleranceTextArea value: %s, test: %.1f, test2: %.1f",tempBuffer,atof(tempBuffer),gui.tempProcessNode->process.processDetails->data.tempTolerance);

                    { char *tmp = getRollerStringIndex(rollerSelected, gui.element.rollerPopup.tempToleranceOptions);
                    lv_textarea_set_text(gui.tempProcessNode->process.processDetails->processToleranceTextArea, tmp != NULL ? tmp : "");
                    free(tmp); }
                    gui.tempProcessNode->process.processDetails->data.somethingChanged = true;
                    lv_obj_send_event(gui.tempProcessNode->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL);
                }
                isScrolled = false;
              }

              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(godFatherCont);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              return;
            }
            if(gui.tempStepNode != NULL && gui.tempStepNode->step.stepDetails != NULL && (lv_obj_t *)data == gui.tempStepNode->step.stepDetails->stepDetailMinTextArea){
              uint32_t newMins = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
              LV_LOG_USER("SET BUTTON from stepDetailMinTextArea value %"PRIu32":", newMins);

              if (newMins != gui.tempStepNode->step.stepDetails->data.timeMins) {
                  gui.tempStepNode->step.stepDetails->data.timeMins = newMins;
                  if(isScrolled) {
                    { char *tmp = getRollerStringIndex(newMins, gui.element.rollerPopup.minutesOptions);
                    lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailMinTextArea, tmp != NULL ? tmp : "");
                    free(tmp); }
                  } else {
                    itoa(newMins, tempBuffer, 10);
                    lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailMinTextArea, tempBuffer);
                  }
                  gui.tempStepNode->step.stepDetails->data.somethingChanged = true;
                  lv_obj_send_event( gui.tempStepNode->step.stepDetails->stepSaveButton, LV_EVENT_REFRESH, NULL);
              }
              isScrolled = false;

              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              return;
            }
            if(gui.tempStepNode != NULL && gui.tempStepNode->step.stepDetails != NULL && (lv_obj_t *)data == gui.tempStepNode->step.stepDetails->stepDetailSecTextArea){
              uint32_t newSecs = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
              LV_LOG_USER("SET BUTTON from stepDetailSecTextArea value %"PRIu32":", newSecs);

              if (newSecs != (uint32_t)gui.tempStepNode->step.stepDetails->data.timeSecs) {
                  gui.tempStepNode->step.stepDetails->data.timeSecs = newSecs;
                  if(isScrolled) {
                    { char *tmp = getRollerStringIndex(newSecs, gui.element.rollerPopup.secondsOptions);
                    lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailSecTextArea, tmp != NULL ? tmp : "");
                    free(tmp); }
                  } else {
                    itoa(newSecs, tempBuffer, 10);
                    lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailSecTextArea, tempBuffer);
                  }
                  gui.tempStepNode->step.stepDetails->data.somethingChanged = true;
                  lv_obj_send_event( gui.tempStepNode->step.stepDetails->stepSaveButton, LV_EVENT_REFRESH, NULL);
              }
              isScrolled = false;

              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              return;
            }
            /* ── Settings Tank Size roller ── */
            if((lv_obj_t *)data == gui.page.settings.tankSizeTextArea) {
              const char *sizes[] = tankSizeValues;
              uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
              if(sel > 2) sel = 1;
              LV_LOG_USER("SET BUTTON from settingsTankSize value %"PRIu32":", sel + 1);
              gui.page.settings.settingsParams.tankSize = sel + 1;
              gui.page.settings.tankSize_active_index = sel;
              lv_textarea_set_text(gui.page.settings.tankSizeTextArea, sizes[sel]);
              isScrolled = false;

              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              qSysAction(SAVE_PROCESS_CONFIG);
              for(uint32_t i = 0; gui.page.processes.processesListContainer != NULL && i < lv_obj_get_child_cnt(gui.page.processes.processesListContainer); i++) {
                  lv_obj_send_event(lv_obj_get_child(gui.page.processes.processesListContainer, i), LV_EVENT_REFRESH, NULL);
              }
              return;
            }
            /* ── Settings Chemistry Container Capacity roller ── */
            if((lv_obj_t *)data == gui.page.settings.chemContainerMlTextArea) {
              uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
              uint16_t vals[] = {250, 500, 750, 1000, 1250, 1500};
              if(sel > 5) sel = 1;
              LV_LOG_USER("SET Chemistry Container: %d ml", vals[sel]);
              gui.page.settings.settingsParams.chemContainerMl = vals[sel];
              { char buf[16]; snprintf(buf, sizeof(buf), "%dml", vals[sel]); lv_textarea_set_text(gui.page.settings.chemContainerMlTextArea, buf); }
              isScrolled = false;
              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              qSysAction(SAVE_PROCESS_CONFIG);
              return;
            }
            /* ── Settings Water Bath Capacity roller ── */
            if((lv_obj_t *)data == gui.page.settings.wbContainerMlTextArea) {
              uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
              uint16_t vals[] = {1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000};
              if(sel > 7) sel = 1;
              LV_LOG_USER("SET Water Bath: %d ml", vals[sel]);
              gui.page.settings.settingsParams.wbContainerMl = vals[sel];
              { char buf[16]; snprintf(buf, sizeof(buf), "%dml", vals[sel]); lv_textarea_set_text(gui.page.settings.wbContainerMlTextArea, buf); }
              isScrolled = false;
              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              qSysAction(SAVE_PROCESS_CONFIG);
              return;
            }
            /* ── Settings Chemistry Volume roller ── */
            if((lv_obj_t *)data == gui.page.settings.chemVolumeTextArea) {
              const char *vols[] = chemVolumeValues;
              uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
              if(sel > 1) sel = 1;
              LV_LOG_USER("SET Chemistry Volume: %s (value=%lu)", vols[sel], (unsigned long)(sel + 1));
              gui.page.settings.settingsParams.chemistryVolume = sel + 1;
              lv_textarea_set_text(gui.page.settings.chemVolumeTextArea, vols[sel]);
              isScrolled = false;
              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              qSysAction(SAVE_PROCESS_CONFIG);
              for(uint32_t i = 0; gui.page.processes.processesListContainer != NULL && i < lv_obj_get_child_cnt(gui.page.processes.processesListContainer); i++) {
                  lv_obj_send_event(lv_obj_get_child(gui.page.processes.processesListContainer, i), LV_EVENT_REFRESH, NULL);
              }
              return;
            }
            /* ── Splash Popup: Palette roller ── */
            if((lv_obj_t *)data == gui.element.splashPopup.paletteTextArea) {
              extern const char * const palette_display[];
              uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
              if(sel > 9) sel = 0;
              LV_LOG_USER("SET Splash Palette: %lu", (unsigned long)sel);
              gui.page.settings.settingsParams.splashPalette = (uint8_t)sel;
              lv_textarea_set_text(gui.element.splashPopup.paletteTextArea, palette_display[sel]);
              lv_textarea_set_cursor_pos(gui.element.splashPopup.paletteTextArea, 0);
              isScrolled = false;
              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              splashPopupRefreshPreview();
              qSysAction(SAVE_PROCESS_CONFIG);
              return;
            }
            /* ── Splash Popup: Shape Style roller ── */
            if((lv_obj_t *)data == gui.element.splashPopup.shapeTextArea) {
              extern const char * const shape_display[];
              uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
              if(sel > 5) sel = 0;
              LV_LOG_USER("SET Splash Shape: %lu", (unsigned long)sel);
              gui.page.settings.settingsParams.splashShapeStyle = (uint8_t)sel;
              lv_textarea_set_text(gui.element.splashPopup.shapeTextArea, shape_display[sel]);
              lv_textarea_set_cursor_pos(gui.element.splashPopup.shapeTextArea, 0);
              isScrolled = false;
              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              splashPopupRefreshPreview();
              qSysAction(SAVE_PROCESS_CONFIG);
              return;
            }
          rollerSelected = 0;
        }
        //free(tempBuffer);
    }

    if(code == LV_EVENT_VALUE_CHANGED){
      if(obj == gui.element.rollerPopup.roller){
          isScrolled = true;
          lv_obj_clear_state(gui.element.rollerPopup.mBoxRollerButton, LV_STATE_DISABLED);
          //if we want to want the index of the selected element
          rollerSelected = lv_roller_get_selected(obj) ;
          lv_roller_get_selected_str(obj, tempBuffer, sizeof(tempBuffer));

          LV_LOG_USER("ROLLER index: %"PRIu32", value: %s", rollerSelected ,tempBuffer);

          
          //if gui.element.rollerPopup.roller has strings to be read
          //lv_roller_get_selected_str(obj, rollerElementSelected, sizeof(rollerElementSelected));
          //LV_LOG_USER("ROLLER value: %s", rollerElementSelected);
      }
    }
    
}       

void rollerPopupCreate(const char * tempOptions,const char * popupTitle, void *whoCallMe, uint32_t currentVal, uint32_t accentColor){
   const ui_roller_popup_layout_t *ui = &ui_get_profile()->roller_popup;
  /*********************
  *    PAGE HEADER
  *********************/
   /* Guard: prevent creating a second popup if one already exists */
   if(gui.element.rollerPopup.mBoxRollerParent != NULL) {
       LV_LOG_USER("Roller popup already exists, skipping duplicate creation");
       return;
   }
   LV_LOG_USER("Roller popup create");
   gui.element.rollerPopup.whoCallMe = whoCallMe;

   createPopupBackdrop(&gui.element.rollerPopup.mBoxRollerParent, &gui.element.rollerPopup.mBoxRollerContainer, ui_get_profile()->popups.roller_w, ui_get_profile()->popups.roller_h); 

         gui.element.rollerPopup.mBoxRollerTitle = lv_label_create(gui.element.rollerPopup.mBoxRollerContainer);         
         lv_label_set_text(gui.element.rollerPopup.mBoxRollerTitle, popupTitle); 
         lv_obj_set_style_text_font(gui.element.rollerPopup.mBoxRollerTitle, ui->title_font, 0);              
         lv_obj_align(gui.element.rollerPopup.mBoxRollerTitle, LV_ALIGN_TOP_MID, ui->title_x, ui->title_y);


   /*Create style*/
   lv_style_init(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
   lv_style_set_line_width(&gui.element.rollerPopup.style_mBoxRollerTitleLine, ui_get_profile()->title_line_width);
   lv_style_set_line_rounded(&gui.element.rollerPopup.style_mBoxRollerTitleLine, true);

   /*Create a line and apply the new style*/
   gui.element.rollerPopup.mBoxRollerTitleLine = lv_line_create(gui.element.rollerPopup.mBoxRollerContainer);
   lv_line_set_points(gui.element.rollerPopup.mBoxRollerTitleLine, gui.element.rollerPopup.titleLinePoints, 2);
   lv_obj_add_style(gui.element.rollerPopup.mBoxRollerTitleLine, &gui.element.rollerPopup.style_mBoxRollerTitleLine, 0);
   lv_obj_align(gui.element.rollerPopup.mBoxRollerTitleLine, LV_ALIGN_TOP_MID, ui->title_line_x, ui->title_line_y);


  /*********************
  *    PAGE ELEMENTS
  *********************/

  gui.element.rollerPopup.mBoxRollerRollerContainer = lv_obj_create(gui.element.rollerPopup.mBoxRollerContainer);
  lv_obj_align(gui.element.rollerPopup.mBoxRollerRollerContainer, LV_ALIGN_TOP_MID, ui->wheel_container_x, ui->wheel_container_y);
  lv_obj_set_size(gui.element.rollerPopup.mBoxRollerRollerContainer, ui_get_profile()->popups.roller_content_w, ui_get_profile()->popups.roller_content_h);
  lv_obj_set_style_border_opa(gui.element.rollerPopup.mBoxRollerRollerContainer, LV_OPA_TRANSP, 0);
  lv_obj_remove_flag(gui.element.rollerPopup.mBoxRollerContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(gui.element.rollerPopup.mBoxRollerRollerContainer, LV_OBJ_FLAG_SCROLLABLE);

  
  lv_style_init(&gui.element.rollerPopup.style_roller);
  lv_style_set_text_font(&gui.element.rollerPopup.style_roller, ui->wheel_font);
  lv_style_set_bg_color(&gui.element.rollerPopup.style_roller, lv_color_hex(accentColor));
  lv_style_set_border_width(&gui.element.rollerPopup.style_roller, ui_get_profile()->title_line_width);
  lv_style_set_border_color(&gui.element.rollerPopup.style_roller, lv_color_hex(accentColor));
  
  gui.element.rollerPopup.roller = lv_roller_create(gui.element.rollerPopup.mBoxRollerRollerContainer);

  lv_roller_set_options(gui.element.rollerPopup.roller, tempOptions, LV_ROLLER_MODE_NORMAL);

  lv_roller_set_visible_row_count(gui.element.rollerPopup.roller, ui->wheel_visible_rows);
  lv_obj_set_width(gui.element.rollerPopup.roller, ui->wheel_w);
  lv_obj_set_height(gui.element.rollerPopup.roller, ui_get_profile()->popups.roller_wheel_h);
  lv_obj_align(gui.element.rollerPopup.roller, LV_ALIGN_CENTER, ui->wheel_x, ui_get_profile()->popups.roller_wheel_y);
  lv_obj_add_event_cb(gui.element.rollerPopup.roller, event_Roller, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_style(gui.element.rollerPopup.roller, &gui.element.rollerPopup.style_roller, LV_PART_SELECTED);
  lv_obj_set_style_text_font(gui.element.rollerPopup.roller, ui->wheel_normal_font, LV_PART_MAIN);
  lv_obj_set_style_border_color(gui.element.rollerPopup.roller, lv_color_hex(WHITE), LV_PART_MAIN);


   gui.element.rollerPopup.mBoxRollerButton = lv_button_create(gui.element.rollerPopup.mBoxRollerRollerContainer);
   lv_obj_set_size(gui.element.rollerPopup.mBoxRollerButton, ui->confirm_btn_w, ui->confirm_btn_h);
   lv_obj_align(gui.element.rollerPopup.mBoxRollerButton, LV_ALIGN_BOTTOM_MID, ui->confirm_btn_x , ui->confirm_btn_y);
   lv_obj_add_event_cb(gui.element.rollerPopup.mBoxRollerButton, event_Roller, LV_EVENT_CLICKED, gui.element.rollerPopup.whoCallMe);
   lv_roller_set_selected(gui.element.rollerPopup.roller, currentVal, LV_ANIM_OFF);
        

         gui.element.rollerPopup.mBoxRollerButtonLabel = lv_label_create(gui.element.rollerPopup.mBoxRollerButton);         
         lv_label_set_text(gui.element.rollerPopup.mBoxRollerButtonLabel, tuneRollerButton_text); 
         lv_obj_set_style_text_font(gui.element.rollerPopup.mBoxRollerButtonLabel, ui->confirm_btn_font, 0);              
         lv_obj_align(gui.element.rollerPopup.mBoxRollerButtonLabel, LV_ALIGN_CENTER, 0, 0);
}

