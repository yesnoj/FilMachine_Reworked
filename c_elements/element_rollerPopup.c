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
            if((lv_obj_t *)data == gui.page.settings.tempSensorTuneButton){
              LV_LOG_USER("SET BUTTON from tempSensorTuneButton value %"PRIu32":",rollerSelected);
              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(godFatherCont);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              gui.page.settings.settingsParams.calibratedTemp = rollerSelected;  
              qSysAction( SAVE_PROCESS_CONFIG );
              return;   
            }
            if(gui.tempProcessNode != NULL && gui.tempProcessNode->process.processDetails != NULL
               && (lv_obj_t *)data == gui.tempProcessNode->process.processDetails->processTempTextArea){
              
              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              if(isScrolled == 0){
                  itoa(lv_roller_get_selected(gui.element.rollerPopup.roller), tempBuffer, 10);
                  LV_LOG_USER("SET BUTTON from processTempTextArea value %"PRIu32":",lv_roller_get_selected(gui.element.rollerPopup.roller));
                  gui.tempProcessNode->process.processDetails->data.temp = lv_roller_get_selected(gui.element.rollerPopup.roller);

                  if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP) {
                    lv_textarea_set_text(gui.tempProcessNode->process.processDetails->processTempTextArea,tempBuffer);
                  } else {
                        sprintf(tempBuffer, "%"PRIi32"", convertCelsiusToFahrenheit(gui.tempProcessNode->process.processDetails->data.temp));
                        lv_textarea_set_text(gui.tempProcessNode->process.processDetails->processTempTextArea, tempBuffer);
                  }
              } else {
                      itoa(rollerSelected, tempBuffer, 10);
                      LV_LOG_USER("SET BUTTON from processTempTextArea value %"PRIu32":",rollerSelected);
                      gui.tempProcessNode->process.processDetails->data.temp = rollerSelected;

                      if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP) {
                        lv_textarea_set_text(gui.tempProcessNode->process.processDetails->processTempTextArea,tempBuffer);
                      } else {
                            sprintf(tempBuffer, "%"PRIi32"", convertCelsiusToFahrenheit(gui.tempProcessNode->process.processDetails->data.temp));
                            lv_textarea_set_text(gui.tempProcessNode->process.processDetails->processTempTextArea, tempBuffer);
                      }
                   isScrolled = false;
              }

              gui.tempProcessNode->process.processDetails->data.somethingChanged = true;
              lv_obj_send_event(gui.tempProcessNode->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL);

 
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(godFatherCont);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              return; 
            }


            if(gui.tempProcessNode != NULL && gui.tempProcessNode->process.processDetails != NULL
               && (lv_obj_t *)data == gui.tempProcessNode->process.processDetails->processToleranceTextArea){
             lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);

              if(isScrolled == 1){
                gui.tempProcessNode->process.processDetails->data.tempTolerance = atof(tempBuffer);
                LV_LOG_USER("SET BUTTON from processToleranceTextArea value: %s, test: %.1f, test2: %.1f",tempBuffer,atof(tempBuffer),gui.tempProcessNode->process.processDetails->data.tempTolerance);
                
                { char *tmp = getRollerStringIndex(rollerSelected, gui.element.rollerPopup.tempToleranceOptions);
                lv_textarea_set_text(gui.tempProcessNode->process.processDetails->processToleranceTextArea, tmp != NULL ? tmp : "");
                free(tmp); }
                isScrolled = false;
              }

              gui.tempProcessNode->process.processDetails->data.somethingChanged = true;
              lv_obj_send_event(gui.tempProcessNode->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL);

              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(godFatherCont);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              return; 
            }
            if(gui.tempStepNode != NULL && gui.tempStepNode->step.stepDetails != NULL && (lv_obj_t *)data == gui.tempStepNode->step.stepDetails->stepDetailMinTextArea){
              if(isScrolled == 0) {
                  itoa(lv_roller_get_selected(gui.element.rollerPopup.roller), tempBuffer, 10);
                  LV_LOG_USER("SET BUTTON from stepDetailMinTextArea value %"PRIu32":",lv_roller_get_selected(gui.element.rollerPopup.roller));
                  gui.tempStepNode->step.stepDetails->data.timeMins = lv_roller_get_selected(gui.element.rollerPopup.roller);
                  lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailMinTextArea, tempBuffer);
              } else {
                    itoa(rollerSelected, tempBuffer, 10);
                    LV_LOG_USER("SET BUTTON from stepDetailMinTextArea value %"PRIu32":",rollerSelected);
                    gui.tempStepNode->step.stepDetails->data.timeMins = rollerSelected;
                    { char *tmp = getRollerStringIndex(rollerSelected, gui.element.rollerPopup.minutesOptions);
                    lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailMinTextArea, tmp != NULL ? tmp : "");
                    free(tmp); }
                    isScrolled = false;
              }

              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              gui.tempStepNode->step.stepDetails->data.somethingChanged = true;
              lv_obj_send_event( gui.tempStepNode->step.stepDetails->stepSaveButton, LV_EVENT_REFRESH, NULL);
              return;
            }
            if(gui.tempStepNode != NULL && gui.tempStepNode->step.stepDetails != NULL && (lv_obj_t *)data == gui.tempStepNode->step.stepDetails->stepDetailSecTextArea){
              if(isScrolled == 0) {
                  itoa(lv_roller_get_selected(gui.element.rollerPopup.roller), tempBuffer, 10);
                  LV_LOG_USER("SET BUTTON from stepDetailMinTextArea value %"PRIu32":",lv_roller_get_selected(gui.element.rollerPopup.roller));
                  gui.tempStepNode->step.stepDetails->data.timeSecs = lv_roller_get_selected(gui.element.rollerPopup.roller);
                  lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailSecTextArea, tempBuffer);
              } else {
                    itoa(rollerSelected, tempBuffer, 10);
                    LV_LOG_USER("SET BUTTON from stepDetailMinTextArea value %"PRIu32":",rollerSelected);
                    gui.tempStepNode->step.stepDetails->data.timeSecs = rollerSelected;
                    { char *tmp = getRollerStringIndex(rollerSelected, gui.element.rollerPopup.secondsOptions);
                    lv_textarea_set_text(gui.tempStepNode->step.stepDetails->stepDetailSecTextArea, tmp != NULL ? tmp : "");
                    free(tmp); }
                    isScrolled = false;
              }

              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
              gui.tempStepNode->step.stepDetails->data.somethingChanged = true;
              lv_obj_send_event( gui.tempStepNode->step.stepDetails->stepSaveButton, LV_EVENT_REFRESH, NULL);
              return; 
            }
            /* ── Settings Tank Size roller ── */
            if((lv_obj_t *)data == gui.page.settings.tankSizeTextArea) {
              const char *sizes[] = {"500ml", "700ml", "1000ml"};
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
              const char *vols[] = {"Low", "High"};
              uint32_t sel = isScrolled ? rollerSelected : lv_roller_get_selected(gui.element.rollerPopup.roller);
              if(sel > 1) sel = 1;
              LV_LOG_USER("SET Chemistry Volume: %s (value=%d)", vols[sel], sel + 1);
              gui.page.settings.settingsParams.chemistryVolume = sel + 1;
              lv_textarea_set_text(gui.page.settings.chemVolumeTextArea, vols[sel]);
              isScrolled = false;
              lv_style_reset(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
              lv_style_reset(&gui.element.rollerPopup.style_roller);
              lv_msgbox_close(gui.element.rollerPopup.mBoxRollerParent);
              gui.element.rollerPopup.mBoxRollerParent = NULL;
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

void rollerPopupCreate(const char * tempOptions,const char * popupTitle, void *whoCallMe, uint32_t currentVal){
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

   gui.element.rollerPopup.mBoxRollerParent = lv_obj_class_create_obj(&lv_msgbox_backdrop_class, lv_layer_top());
   lv_obj_class_init_obj(gui.element.rollerPopup.mBoxRollerParent);
   lv_obj_remove_flag(gui.element.rollerPopup.mBoxRollerParent, LV_OBJ_FLAG_IGNORE_LAYOUT);
   lv_obj_set_size(gui.element.rollerPopup.mBoxRollerParent, LV_PCT(100), LV_PCT(100));

         gui.element.rollerPopup.mBoxRollerContainer = lv_obj_create(gui.element.rollerPopup.mBoxRollerParent);
         lv_obj_align(gui.element.rollerPopup.mBoxRollerContainer, LV_ALIGN_CENTER, 0, 0);
         lv_obj_set_size(gui.element.rollerPopup.mBoxRollerContainer, 250, 220); 
         lv_obj_remove_flag(gui.element.rollerPopup.mBoxRollerContainer, LV_OBJ_FLAG_SCROLLABLE); 

         gui.element.rollerPopup.mBoxRollerTitle = lv_label_create(gui.element.rollerPopup.mBoxRollerContainer);         
         lv_label_set_text(gui.element.rollerPopup.mBoxRollerTitle, popupTitle); 
         lv_obj_set_style_text_font(gui.element.rollerPopup.mBoxRollerTitle, &lv_font_montserrat_22, 0);              
         lv_obj_align(gui.element.rollerPopup.mBoxRollerTitle, LV_ALIGN_TOP_MID, 0, - 10);


   /*Create style*/
   lv_style_init(&gui.element.rollerPopup.style_mBoxRollerTitleLine);
   lv_style_set_line_width(&gui.element.rollerPopup.style_mBoxRollerTitleLine, 2);
   lv_style_set_line_rounded(&gui.element.rollerPopup.style_mBoxRollerTitleLine, true);

   /*Create a line and apply the new style*/
   gui.element.rollerPopup.mBoxRollerTitleLine = lv_line_create(gui.element.rollerPopup.mBoxRollerContainer);
   lv_line_set_points(gui.element.rollerPopup.mBoxRollerTitleLine, gui.element.rollerPopup.titleLinePoints, 2);
   lv_obj_add_style(gui.element.rollerPopup.mBoxRollerTitleLine, &gui.element.rollerPopup.style_mBoxRollerTitleLine, 0);
   lv_obj_align(gui.element.rollerPopup.mBoxRollerTitleLine, LV_ALIGN_TOP_MID, 0, 23);


  /*********************
  *    PAGE ELEMENTS
  *********************/

  gui.element.rollerPopup.mBoxRollerRollerContainer = lv_obj_create(gui.element.rollerPopup.mBoxRollerContainer);
  lv_obj_align(gui.element.rollerPopup.mBoxRollerRollerContainer, LV_ALIGN_TOP_MID, 0, 30);
  lv_obj_set_size(gui.element.rollerPopup.mBoxRollerRollerContainer, 235, 170); 
  lv_obj_set_style_border_opa(gui.element.rollerPopup.mBoxRollerRollerContainer, LV_OPA_TRANSP, 0);
  lv_obj_remove_flag(gui.element.rollerPopup.mBoxRollerContainer, LV_OBJ_FLAG_SCROLLABLE);

  
  lv_style_init(&gui.element.rollerPopup.style_roller);
  lv_style_set_text_font(&gui.element.rollerPopup.style_roller, &lv_font_montserrat_30);
  lv_style_set_bg_color(&gui.element.rollerPopup.style_roller, lv_color_hex(LIGHT_BLUE));
  lv_style_set_border_width(&gui.element.rollerPopup.style_roller, 2);
  lv_style_set_border_color(&gui.element.rollerPopup.style_roller, lv_color_hex(LIGHT_BLUE_DARK));
  
  gui.element.rollerPopup.roller = lv_roller_create(gui.element.rollerPopup.mBoxRollerRollerContainer);

  lv_roller_set_options(gui.element.rollerPopup.roller, tempOptions, LV_ROLLER_MODE_NORMAL);

  lv_roller_set_visible_row_count(gui.element.rollerPopup.roller, 4);
  lv_obj_set_width(gui.element.rollerPopup.roller, 140);
  lv_obj_set_height(gui.element.rollerPopup.roller, 100);
  lv_obj_align(gui.element.rollerPopup.roller, LV_ALIGN_CENTER, 0, -30);
  lv_obj_add_event_cb(gui.element.rollerPopup.roller, event_Roller, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_style(gui.element.rollerPopup.roller, &gui.element.rollerPopup.style_roller, LV_PART_SELECTED);  
  lv_obj_set_style_border_color(gui.element.rollerPopup.roller, lv_color_hex(WHITE), LV_PART_MAIN);


   gui.element.rollerPopup.mBoxRollerButton = lv_button_create(gui.element.rollerPopup.mBoxRollerRollerContainer);
   lv_obj_set_size(gui.element.rollerPopup.mBoxRollerButton, BUTTON_TUNE_WIDTH, BUTTON_TUNE_HEIGHT);
   lv_obj_align(gui.element.rollerPopup.mBoxRollerButton, LV_ALIGN_BOTTOM_MID, 0 , 0);
   lv_obj_add_event_cb(gui.element.rollerPopup.mBoxRollerButton, event_Roller, LV_EVENT_CLICKED, gui.element.rollerPopup.whoCallMe);
   lv_obj_add_state(gui.element.rollerPopup.mBoxRollerButton, LV_STATE_DISABLED);
   lv_roller_set_selected(gui.element.rollerPopup.roller, currentVal, LV_ANIM_OFF);


  if(currentVal == 0)  
      lv_obj_add_state(gui.element.rollerPopup.mBoxRollerButton, LV_STATE_DISABLED);
  else
      lv_obj_clear_state(gui.element.rollerPopup.mBoxRollerButton, LV_STATE_DISABLED);
        

         gui.element.rollerPopup.mBoxRollerButtonLabel = lv_label_create(gui.element.rollerPopup.mBoxRollerButton);         
         lv_label_set_text(gui.element.rollerPopup.mBoxRollerButtonLabel, tuneRollerButton_text); 
         lv_obj_set_style_text_font(gui.element.rollerPopup.mBoxRollerButtonLabel, &lv_font_montserrat_18, 0);              
         lv_obj_align(gui.element.rollerPopup.mBoxRollerButtonLabel, LV_ALIGN_CENTER, 0, 0);
}

