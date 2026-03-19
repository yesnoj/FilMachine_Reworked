
/**
 * @file element_filterPopup.c
 *
 */
#include <string.h>

#include "FilMachine.h"

extern struct gui_components gui;
//uint8_t filterFilmType = 0;
static bool filter_is_empty(void) {
    return strlen(gui.element.filterPopup.filterName) == 0
        && !gui.element.filterPopup.isColorFilter
        && !gui.element.filterPopup.isBnWFilter
        && !gui.element.filterPopup.preferredOnly;
}

void event_filterMBox(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);

  if(obj == gui.element.filterPopup.mBoxCloseButton){
      if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Close filter popup");
        lv_obj_add_flag(gui.element.filterPopup.mBoxFilterPopupParent, LV_OBJ_FLAG_HIDDEN);
      }
  }

  if(obj == gui.element.filterPopup.mBoxApplyFilterButton || obj == gui.element.filterPopup.mBoxResetFilterButton){
      if(code == LV_EVENT_CLICKED) {
        if(obj == gui.element.filterPopup.mBoxApplyFilterButton){
          LV_LOG_USER("Apply BUTTON");

          if(!filter_is_empty()){
            lv_obj_set_style_text_color(gui.page.processes.iconFilterLabel, lv_color_hex(GREEN), LV_PART_MAIN);
            filterAndDisplayProcesses();
          } else {
            lv_obj_set_style_text_color(gui.page.processes.iconFilterLabel, lv_color_hex(WHITE), LV_PART_MAIN);
            gui.page.processes.isFiltered = false;
            removeFiltersAndDisplayAllProcesses();
          }
          lv_obj_add_flag(gui.element.filterPopup.mBoxFilterPopupParent, LV_OBJ_FLAG_HIDDEN);
        }
        if(obj == gui.element.filterPopup.mBoxResetFilterButton){
          LV_LOG_USER("Reset BUTTON");

          lv_textarea_set_text(gui.element.filterPopup.mBoxNameTextArea, "");
          lv_obj_remove_state(gui.element.filterPopup.mBoxOnlyPreferredSwitch, LV_STATE_CHECKED);
          lv_obj_remove_state(gui.element.filterPopup.mBoxSelectColorRadioButton, LV_STATE_CHECKED);
          lv_obj_remove_state(gui.element.filterPopup.mBoxSelectBnWRadioButton, LV_STATE_CHECKED);
          gui.element.filterPopup.isColorFilter = false;
          gui.element.filterPopup.isBnWFilter = false;
          gui.element.filterPopup.preferredOnly = false;
          gui.element.filterPopup.filterName[0] = '\0';

          /* Restore filter icon to white and show all processes */
          lv_obj_set_style_text_color(gui.page.processes.iconFilterLabel, lv_color_hex(WHITE), LV_PART_MAIN);
          gui.page.processes.isFiltered = false;
          removeFiltersAndDisplayAllProcesses();
        }
      }
  }

  if(obj == gui.element.filterPopup.mBoxSelectColorRadioButton || obj == gui.element.filterPopup.mBoxSelectBnWRadioButton){
    if(code == LV_EVENT_VALUE_CHANGED) {
        if(obj == gui.element.filterPopup.mBoxSelectColorRadioButton){
          LV_LOG_USER("State color: %s", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
          gui.element.filterPopup.isColorFilter = lv_obj_has_state(obj, LV_STATE_CHECKED);
          }
        if(obj == gui.element.filterPopup.mBoxSelectBnWRadioButton){
          LV_LOG_USER("State bnw: %s", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
          gui.element.filterPopup.isBnWFilter = lv_obj_has_state(obj, LV_STATE_CHECKED);
          }
      }
    }
  if(obj == gui.element.filterPopup.mBoxOnlyPreferredSwitch){
    if(code == LV_EVENT_VALUE_CHANGED) {
        LV_LOG_USER("State preferred: %s", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
        gui.element.filterPopup.preferredOnly = lv_obj_has_state(obj, LV_STATE_CHECKED);
      }
    }
}


void filterPopupCreate (void){
  /*********************
   *    PAGE ELEMENTS
   *********************/
  if (gui.element.filterPopup.mBoxFilterPopupParent == NULL)
  {
      createPopupBackdrop(&gui.element.filterPopup.mBoxFilterPopupParent, &gui.element.filterPopup.mBoxContainer, 320, 240); 

      gui.element.filterPopup.mBoxTitle = lv_label_create(gui.element.filterPopup.mBoxContainer);         
      lv_label_set_text(gui.element.filterPopup.mBoxTitle, filterPopupTitle_text); 
      lv_obj_set_style_text_font(gui.element.filterPopup.mBoxTitle, &lv_font_montserrat_22, 0);              
      lv_obj_align(gui.element.filterPopup.mBoxTitle, LV_ALIGN_TOP_MID, 0, - 10);

      /*Create style*/
      lv_style_init(&gui.element.filterPopup.style_mBoxTitleLine);
      lv_style_set_line_width(&gui.element.filterPopup.style_mBoxTitleLine, 2);
      lv_style_set_line_color(&gui.element.filterPopup.style_mBoxTitleLine, lv_palette_main(LV_PALETTE_GREEN));
      lv_style_set_line_rounded(&gui.element.filterPopup.style_mBoxTitleLine, true);

      /*Create a line and apply the new style*/
      gui.element.filterPopup.mBoxStepPopupTitleLine = lv_line_create(gui.element.filterPopup.mBoxContainer);
      lv_line_set_points(gui.element.filterPopup.mBoxStepPopupTitleLine, gui.element.filterPopup.titleLinePoints, 2);
      lv_obj_add_style(gui.element.filterPopup.mBoxStepPopupTitleLine, &gui.element.filterPopup.style_mBoxTitleLine, 0);
      lv_obj_align(gui.element.filterPopup.mBoxStepPopupTitleLine, LV_ALIGN_TOP_MID, 0, 23);

      //CLOSE BUTTON
      gui.element.filterPopup.mBoxCloseButton = lv_button_create(gui.element.filterPopup.mBoxContainer);
      lv_obj_set_size(gui.element.filterPopup.mBoxCloseButton, BUTTON_POPUP_CLOSE_WIDTH, BUTTON_POPUP_CLOSE_HEIGHT);
      lv_obj_align(gui.element.filterPopup.mBoxCloseButton, LV_ALIGN_TOP_RIGHT, 7, -10);
      lv_obj_add_event_cb(gui.element.filterPopup.mBoxCloseButton, event_filterMBox, LV_EVENT_CLICKED, gui.element.filterPopup.mBoxCloseButton);
      lv_obj_set_style_bg_color(gui.element.filterPopup.mBoxCloseButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
      lv_obj_move_foreground(gui.element.filterPopup.mBoxCloseButton);

            gui.element.filterPopup.mBoxCloseButtonLabel = lv_label_create(gui.element.filterPopup.mBoxCloseButton);
            lv_label_set_text(gui.element.filterPopup.mBoxCloseButtonLabel, closePopup_icon);
            lv_obj_set_style_text_font(gui.element.filterPopup.mBoxCloseButtonLabel, &FilMachineFontIcons_20, 0);
            lv_obj_align(gui.element.filterPopup.mBoxCloseButtonLabel, LV_ALIGN_CENTER, 0, 0);

      //NAME TO FILTER
      gui.element.filterPopup.mBoxNameContainer = lv_obj_create(gui.element.filterPopup.mBoxContainer);
      lv_obj_remove_flag(gui.element.filterPopup.mBoxNameContainer, LV_OBJ_FLAG_SCROLLABLE); 
      lv_obj_align(gui.element.filterPopup.mBoxNameContainer, LV_ALIGN_TOP_LEFT, -7, 30);
      lv_obj_set_size(gui.element.filterPopup.mBoxNameContainer, 300, 40); 
      lv_obj_set_style_border_opa(gui.element.filterPopup.mBoxNameContainer, LV_OPA_TRANSP, 0);
      //lv_obj_set_style_border_color(gui.element.filterPopup.mBoxNameContainer, lv_color_hex(GREEN_DARK), 0);

          gui.element.filterPopup.mBoxNameLabel = lv_label_create(gui.element.filterPopup.mBoxNameContainer);         
          lv_label_set_text(gui.element.filterPopup.mBoxNameLabel, filterPopupName_text); 
          lv_obj_set_style_text_font(gui.element.filterPopup.mBoxNameLabel, &lv_font_montserrat_22, 0);              
          lv_obj_align(gui.element.filterPopup.mBoxNameLabel, LV_ALIGN_LEFT_MID, -10, 0);
      
          gui.element.filterPopup.mBoxNameTextArea = lv_textarea_create(gui.element.filterPopup.mBoxNameContainer);
          lv_textarea_set_one_line(gui.element.filterPopup.mBoxNameTextArea, true);
          lv_textarea_set_placeholder_text(gui.element.filterPopup.mBoxNameTextArea, filterPopupNamePlaceHolder_text);
          lv_obj_align(gui.element.filterPopup.mBoxNameTextArea, LV_ALIGN_LEFT_MID, 65, 0);
          lv_obj_set_width(gui.element.filterPopup.mBoxNameTextArea, 215);
          memset(&gui.element.filterPopup.nameKeyboardCtx, 0, sizeof(gui.element.filterPopup.nameKeyboardCtx));
          gui.element.filterPopup.nameKeyboardCtx.owner = KB_OWNER_FILTER;
          gui.element.filterPopup.nameKeyboardCtx.textArea = gui.element.filterPopup.mBoxNameTextArea;
          gui.element.filterPopup.nameKeyboardCtx.parentScreen = gui.element.filterPopup.mBoxFilterPopupParent;
          gui.element.filterPopup.nameKeyboardCtx.ownerData = &gui.element.filterPopup;
          lv_obj_add_event_cb(gui.element.filterPopup.mBoxNameTextArea, event_keyboard, LV_EVENT_CLICKED, &gui.element.filterPopup.nameKeyboardCtx);
          lv_obj_add_event_cb(gui.element.filterPopup.mBoxNameTextArea, event_keyboard, LV_EVENT_DEFOCUSED, &gui.element.filterPopup.nameKeyboardCtx);
          lv_obj_add_event_cb(gui.element.filterPopup.mBoxNameTextArea, event_keyboard, LV_EVENT_CANCEL, &gui.element.filterPopup.nameKeyboardCtx);
          lv_obj_add_event_cb(gui.element.filterPopup.mBoxNameTextArea, event_keyboard, LV_EVENT_READY, &gui.element.filterPopup.nameKeyboardCtx);
          lv_obj_add_state(gui.element.filterPopup.mBoxNameTextArea, LV_STATE_FOCUSED); /*To be sure the cursor is visible*/
          lv_obj_set_style_bg_color(gui.element.filterPopup.mBoxNameTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
          lv_obj_set_style_border_color(gui.element.filterPopup.mBoxNameTextArea, lv_color_hex(WHITE), 0);
          lv_textarea_set_max_length(gui.element.filterPopup.mBoxNameTextArea, MAX_PROC_NAME_LEN);

      //CHOOSE COLOR/B&W/Preferred — all on the same row
      gui.element.filterPopup.selectColorContainerRadioButton = lv_obj_create(gui.element.filterPopup.mBoxContainer);
      lv_obj_remove_flag(gui.element.filterPopup.selectColorContainerRadioButton, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_align(gui.element.filterPopup.selectColorContainerRadioButton, LV_ALIGN_LEFT_MID, -7, -5);
      lv_obj_set_size(gui.element.filterPopup.selectColorContainerRadioButton, 300, 40);
      lv_obj_set_style_border_opa(gui.element.filterPopup.selectColorContainerRadioButton, LV_OPA_TRANSP, 0);

          //COLOR RADIO BUTTON WITH LABEL
          gui.element.filterPopup.mBoxSelectColorRadioButton = lv_obj_create(gui.element.filterPopup.selectColorContainerRadioButton);
          lv_obj_remove_flag(gui.element.filterPopup.mBoxSelectColorRadioButton, LV_OBJ_FLAG_SCROLLABLE);
          lv_obj_align(gui.element.filterPopup.mBoxSelectColorRadioButton, LV_ALIGN_LEFT_MID, -20, 0);
          lv_obj_set_size(gui.element.filterPopup.mBoxSelectColorRadioButton, 100, 40);
          lv_obj_set_style_border_opa(gui.element.filterPopup.mBoxSelectColorRadioButton, LV_OPA_TRANSP, 0);

          gui.element.filterPopup.mBoxColorLabel = lv_label_create(gui.element.filterPopup.mBoxSelectColorRadioButton);
          lv_label_set_text(gui.element.filterPopup.mBoxColorLabel, filterPopupColor_text);
          lv_obj_set_style_text_font(gui.element.filterPopup.mBoxColorLabel, &lv_font_montserrat_22, 0);
          lv_obj_align(gui.element.filterPopup.mBoxColorLabel, LV_ALIGN_LEFT_MID, - 10, 0);

          gui.element.filterPopup.mBoxSelectColorRadioButton = create_radiobutton(gui.element.filterPopup.mBoxSelectColorRadioButton, "", 25, 0, 27, &lv_font_montserrat_18, lv_color_hex(GREEN_DARK), lv_palette_main(LV_PALETTE_GREEN));
          lv_obj_add_event_cb(gui.element.filterPopup.mBoxSelectColorRadioButton, event_filterMBox, LV_EVENT_VALUE_CHANGED, gui.element.filterPopup.mBoxSelectColorRadioButton);

          //B&W RADIO BUTTON WITH LABEL
          gui.element.filterPopup.mBoxSelectBnWRadioButton = lv_obj_create(gui.element.filterPopup.selectColorContainerRadioButton);
          lv_obj_remove_flag(gui.element.filterPopup.mBoxSelectBnWRadioButton, LV_OBJ_FLAG_SCROLLABLE);
          lv_obj_align(gui.element.filterPopup.mBoxSelectBnWRadioButton, LV_ALIGN_LEFT_MID, 80, 0);
          lv_obj_set_size(gui.element.filterPopup.mBoxSelectBnWRadioButton, 100, 40);
          lv_obj_set_style_border_opa(gui.element.filterPopup.mBoxSelectBnWRadioButton, LV_OPA_TRANSP, 0);

          gui.element.filterPopup.mBoxBnWLabel = lv_label_create(gui.element.filterPopup.mBoxSelectBnWRadioButton);
          lv_label_set_text(gui.element.filterPopup.mBoxBnWLabel, filterPopupBnW_text);
          lv_obj_set_style_text_font(gui.element.filterPopup.mBoxBnWLabel, &lv_font_montserrat_22, 0);
          lv_obj_align(gui.element.filterPopup.mBoxBnWLabel, LV_ALIGN_LEFT_MID, - 10, 0);

          gui.element.filterPopup.mBoxSelectBnWRadioButton = create_radiobutton(gui.element.filterPopup.mBoxSelectBnWRadioButton, "", 25, 0, 27, &lv_font_montserrat_18, lv_color_hex(GREEN_DARK), lv_palette_main(LV_PALETTE_GREEN));
          lv_obj_add_event_cb(gui.element.filterPopup.mBoxSelectBnWRadioButton, event_filterMBox, LV_EVENT_VALUE_CHANGED, gui.element.filterPopup.mBoxSelectBnWRadioButton);


      //ONLY PREFERRED FILTER — same row, right after B&W
      gui.element.filterPopup.mBoxPreferredContainer = lv_obj_create(gui.element.filterPopup.mBoxContainer);
      lv_obj_remove_flag(gui.element.filterPopup.mBoxPreferredContainer, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_align(gui.element.filterPopup.mBoxPreferredContainer, LV_ALIGN_LEFT_MID, -7, 35);
      lv_obj_set_size(gui.element.filterPopup.mBoxPreferredContainer, 300, 40);
      lv_obj_set_style_border_opa(gui.element.filterPopup.mBoxPreferredContainer, LV_OPA_TRANSP, 0);

          gui.element.filterPopup.mBoxPreferredLabel = lv_label_create(gui.element.filterPopup.mBoxPreferredContainer);
          lv_label_set_text(gui.element.filterPopup.mBoxPreferredLabel, filterPopupPreferred_text);
          lv_obj_set_style_text_font(gui.element.filterPopup.mBoxPreferredLabel, &lv_font_montserrat_22, 0);
          lv_obj_align(gui.element.filterPopup.mBoxPreferredLabel, LV_ALIGN_LEFT_MID, -10, 0);

          gui.element.filterPopup.mBoxOnlyPreferredSwitch = create_radiobutton(gui.element.filterPopup.mBoxPreferredContainer, "", 25, 0, 27, &lv_font_montserrat_18, lv_color_hex(GREEN_DARK), lv_palette_main(LV_PALETTE_GREEN));
          lv_obj_align(gui.element.filterPopup.mBoxOnlyPreferredSwitch, LV_ALIGN_LEFT_MID, 95, 0);
          lv_obj_add_event_cb(gui.element.filterPopup.mBoxOnlyPreferredSwitch, event_filterMBox, LV_EVENT_VALUE_CHANGED, gui.element.filterPopup.mBoxOnlyPreferredSwitch);



      //RESET FILTER
      gui.element.filterPopup.mBoxResetFilterButton = lv_button_create(gui.element.filterPopup.mBoxContainer);
      lv_obj_set_size(gui.element.filterPopup.mBoxResetFilterButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
      lv_obj_align(gui.element.filterPopup.mBoxResetFilterButton, LV_ALIGN_BOTTOM_LEFT, 10 , 10);
      lv_obj_add_event_cb(gui.element.filterPopup.mBoxResetFilterButton, event_filterMBox, LV_EVENT_CLICKED, gui.element.filterPopup.mBoxResetFilterButton);
      lv_obj_set_style_bg_color(gui.element.filterPopup.mBoxResetFilterButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);

          gui.element.filterPopup.mBoxResetFilterLabel = lv_label_create(gui.element.filterPopup.mBoxResetFilterButton);
          lv_label_set_text(gui.element.filterPopup.mBoxResetFilterLabel, filterPopupResetButton_text);
          lv_obj_set_style_text_font(gui.element.filterPopup.mBoxResetFilterLabel, &lv_font_montserrat_22, 0);
          lv_obj_align(gui.element.filterPopup.mBoxResetFilterLabel, LV_ALIGN_CENTER, 0, 0);



      //APPLY FILTER
      gui.element.filterPopup.mBoxApplyFilterButton = lv_button_create(gui.element.filterPopup.mBoxContainer);
      lv_obj_set_size(gui.element.filterPopup.mBoxApplyFilterButton, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
      lv_obj_align(gui.element.filterPopup.mBoxApplyFilterButton, LV_ALIGN_BOTTOM_RIGHT, - 10 , 10);
      lv_obj_add_event_cb(gui.element.filterPopup.mBoxApplyFilterButton, event_filterMBox, LV_EVENT_CLICKED, gui.element.filterPopup.mBoxApplyFilterButton);
      lv_obj_set_style_bg_color(gui.element.filterPopup.mBoxApplyFilterButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);

            gui.element.filterPopup.mBoxApplyFilterLabel = lv_label_create(gui.element.filterPopup.mBoxApplyFilterButton);
            lv_label_set_text(gui.element.filterPopup.mBoxApplyFilterLabel, filterPopupApplyButton_text);
            lv_obj_set_style_text_font(gui.element.filterPopup.mBoxApplyFilterLabel, &lv_font_montserrat_22, 0);
            lv_obj_align(gui.element.filterPopup.mBoxApplyFilterLabel, LV_ALIGN_CENTER, 0, 0);
  }
  //create_keyboard(keyboardFilter, gui.element.filterPopup.mBoxFilterPopupParent);


}

