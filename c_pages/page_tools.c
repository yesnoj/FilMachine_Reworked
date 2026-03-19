/**
 * @file page_tools.c
 *
 */


//ESSENTIAL INCLUDE
#include "FilMachine.h"

extern struct gui_components gui;
static lv_timer_t * guiUpdaterTimer;

//ACCESSORY INCLUDES


void event_toolsPopup(lv_event_t * e) {

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * data = (lv_obj_t *)lv_event_get_user_data(e);
  
  if(code == LV_EVENT_CLICKED) {  
    if(data == gui.page.tools.toolsImportLabel){
        LV_LOG_USER("PRESSED gui.page.tools.toolsImportLabel");
        messagePopupCreate(messagePopupDetailTitle_text,importConfigAndProcessesMBox_text,NULL, NULL, NULL);
    }
    if(data == gui.page.tools.toolsExportLabel){
        LV_LOG_USER("PRESSED gui.page.tools.toolsExportLabel");
        messagePopupCreate(messagePopupDetailTitle_text,exportConfigAndProcessesMBox_text, NULL, NULL, NULL);
    }
    if(data == gui.page.tools.toolCreditButton){
        LV_LOG_USER("PRESSED gui.page.tools.toolCreditButton");
        messagePopupCreate(softwareCredits_text,softwareCreditsValue_text, NULL, NULL, NULL);
    }
    if(data == gui.page.tools.toolsUpdateSDLabel){
        LV_LOG_USER("PRESSED gui.page.tools.toolsUpdateSDLabel (info)");
        messagePopupCreate(otaUpdateFromSD_text, otaUpdateFromSDMBox_text, NULL, NULL, NULL);
    }
    if(data == gui.page.tools.toolsUpdateWifiLabel){
        LV_LOG_USER("PRESSED gui.page.tools.toolsUpdateWifiLabel (info)");
        messagePopupCreate(otaWifiUpdate_text, otaWifiUpdateMBox_text, NULL, NULL, NULL);
    }
  }
}

void event_toolsElement(lv_event_t * e) {

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
  
  if(code == LV_EVENT_CLICKED) { 
      if(obj == gui.page.tools.toolsCleaningButton) { 
            LV_LOG_USER("PRESSED gui.page.tools.toolsCleaningButton");
            if(gui.element.cleanPopup.cleanPopupParent == NULL) {
                cleanPopup();
                LV_LOG_USER("New clean popup created!");
            } else {
                  LV_LOG_USER("Clean popup already created!");
                  lv_obj_remove_flag(gui.element.cleanPopup.cleanSettingsContainer, LV_OBJ_FLAG_HIDDEN);
                  lv_obj_add_flag(gui.element.cleanPopup.cleanProcessContainer, LV_OBJ_FLAG_HIDDEN);
                  lv_obj_remove_flag(gui.element.cleanPopup.cleanPopupParent, LV_OBJ_FLAG_HIDDEN);
            }
      }
      if(obj == gui.page.tools.toolsDrainingButton){
              LV_LOG_USER("PRESSED gui.page.tools.toolsDrainingButton");
              if(gui.element.drainPopup.drainPopupParent == NULL) {
                  drainPopupCreate();
                  LV_LOG_USER("New drain popup created!");
              } else {
                  LV_LOG_USER("Drain popup already created!");
                  lv_obj_remove_flag(gui.element.drainPopup.drainConfirmContainer, LV_OBJ_FLAG_HIDDEN);
                  lv_obj_add_flag(gui.element.drainPopup.drainProcessContainer, LV_OBJ_FLAG_HIDDEN);
                  lv_obj_remove_flag(gui.element.drainPopup.drainPopupParent, LV_OBJ_FLAG_HIDDEN);
              }
          }
      if(obj == gui.page.tools.toolsSelfcheckButton){
              LV_LOG_USER("PRESSED gui.page.tools.toolsSelfcheckButton");
              if(gui.element.selfcheckPopup.selfcheckPopupParent == NULL) {
                  selfcheckPopupCreate();
                  LV_LOG_USER("New selfcheck popup created!");
              } else {
                  LV_LOG_USER("Selfcheck popup already created!");
                  lv_obj_remove_flag(gui.element.selfcheckPopup.selfcheckPopupParent, LV_OBJ_FLAG_HIDDEN);
              }
          }
      if(obj == gui.page.tools.toolsExportButton){
              LV_LOG_USER("PRESSED gui.page.tools.toolsExportButton");
			  messagePopupCreate(exportConfigAndProcesses_text,exportConfigAndProcessesMBox_text, checkupNo_text, checkupYes_text, obj);
          }
      if(obj == gui.page.tools.toolsImportButton){
          LV_LOG_USER("PRESSED gui.page.tools.toolsImportButton");
          messagePopupCreate(importConfigAndProcesses_text,importConfigAndProcessesMBox2_text, checkupNo_text, checkupYes_text, obj);
        }
      if(obj == gui.page.tools.toolsUpdateSDButton){
          LV_LOG_USER("PRESSED gui.page.tools.toolsUpdateSDButton");
          if (ota_is_running()) return;
          char ver[32];
          if (ota_check_sd(ver, sizeof(ver))) {
              char msg[128];
              snprintf(msg, sizeof(msg), otaConfirmUpdate_text, ver);
              messagePopupCreate(otaUpdateFromSD_text, msg, checkupNo_text, checkupYes_text, obj);
          } else {
              messagePopupCreate(otaUpdateFromSD_text, otaNoFirmware_text, NULL, NULL, NULL);
          }
      }
      if(obj == gui.page.tools.toolsUpdateWifiButton){
          LV_LOG_USER("PRESSED gui.page.tools.toolsUpdateWifiButton");
          if (ota_is_running()) return;
          otaWifiPopupCreate();
      }
    }
}

void guiUpdater_timer(lv_timer_t * timer) {

  lv_label_set_text_fmt(gui.page.tools.toolStatTotalTimeValue, "%"PRIu64"", gui.page.tools.machineStats.totalMins);
  lv_label_set_text_fmt(gui.page.tools.toolStatCompletedProcessesValue, "%"PRIu32"", gui.page.tools.machineStats.completed);
  lv_label_set_text_fmt(gui.page.tools.toolStatStoppedProcessesValue, "%"PRIu32"", gui.page.tools.machineStats.stopped);
  lv_label_set_text_fmt(gui.page.tools.toolStatCompleteCycleValue, "%"PRIu32"", gui.page.tools.machineStats.clean);
}

static void initTools_maintenance(lv_obj_t *parent) {
  gui.page.tools.toolsMaintenanceLabel = lv_label_create(parent);
  lv_label_set_text(gui.page.tools.toolsMaintenanceLabel, Maintenance_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsMaintenanceLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(gui.page.tools.toolsMaintenanceLabel, LV_ALIGN_TOP_LEFT, -7, -5);

  initTitleLineStyle(&gui.page.tools.style_sectionTitleLine, LIGHT_BLUE);

  gui.page.tools.sectionTitleLine = lv_line_create(parent);
  lv_line_set_points(gui.page.tools.sectionTitleLine, gui.page.tools.titleLinePoints, 2);
  lv_obj_add_style(gui.page.tools.sectionTitleLine, &gui.page.tools.style_sectionTitleLine, 0);
  lv_obj_align(gui.page.tools.sectionTitleLine, LV_ALIGN_TOP_MID, 0, 30);

  gui.page.tools.toolsCleaningContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsCleaningContainer, LV_ALIGN_TOP_LEFT, -15, 35);
  lv_obj_set_size(gui.page.tools.toolsCleaningContainer, 330, 50);
  lv_obj_remove_flag(gui.page.tools.toolsCleaningContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsCleaningContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsCleaningContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolsCleaningLabel = lv_label_create(gui.page.tools.toolsCleaningContainer);
  lv_label_set_text(gui.page.tools.toolsCleaningLabel, cleanMachine_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsCleaningLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolsCleaningLabel, LV_ALIGN_LEFT_MID, -5, 0);

  gui.page.tools.toolsCleaningButton =  lv_button_create(gui.page.tools.toolsCleaningContainer);
  lv_obj_set_size(gui.page.tools.toolsCleaningButton, BUTTON_PROCESS_WIDTH * 0.8, BUTTON_PROCESS_HEIGHT);
  lv_obj_align(gui.page.tools.toolsCleaningButton, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(gui.page.tools.toolsCleaningButton, event_toolsElement, LV_EVENT_CLICKED, gui.page.tools.toolsCleaningButton);
  lv_obj_set_style_bg_color(gui.page.tools.toolsCleaningButton, lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);

  gui.page.tools.toolsCleaningButtonLabel = lv_label_create(gui.page.tools.toolsCleaningButton);
  lv_label_set_text(gui.page.tools.toolsCleaningButtonLabel, play_icon);
  lv_obj_set_style_text_font(gui.page.tools.toolsCleaningButtonLabel, &FilMachineFontIcons_30, 0);
  lv_obj_align(gui.page.tools.toolsCleaningButtonLabel, LV_ALIGN_CENTER, 0, 0);

  gui.page.tools.toolsDrainingContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsDrainingContainer, LV_ALIGN_TOP_LEFT, -15, 85);
  lv_obj_set_size(gui.page.tools.toolsDrainingContainer, 330, 50);
  lv_obj_remove_flag(gui.page.tools.toolsDrainingContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsDrainingContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsDrainingContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolsDrainingLabel = lv_label_create(gui.page.tools.toolsDrainingContainer);
  lv_label_set_text(gui.page.tools.toolsDrainingLabel, drainMachine_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsDrainingLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolsDrainingLabel, LV_ALIGN_LEFT_MID, -5, 0);

  gui.page.tools.toolsDrainingButton =  lv_button_create(gui.page.tools.toolsDrainingContainer);
  lv_obj_set_size(gui.page.tools.toolsDrainingButton, BUTTON_PROCESS_WIDTH * 0.8, BUTTON_PROCESS_HEIGHT);
  lv_obj_align(gui.page.tools.toolsDrainingButton, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(gui.page.tools.toolsDrainingButton, event_toolsElement, LV_EVENT_CLICKED, gui.page.tools.toolsDrainingButton);
  lv_obj_set_style_bg_color(gui.page.tools.toolsDrainingButton, lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);

  gui.page.tools.toolsDrainingButtonLabel = lv_label_create(gui.page.tools.toolsDrainingButton);
  lv_label_set_text(gui.page.tools.toolsDrainingButtonLabel, play_icon);
  lv_obj_set_style_text_font(gui.page.tools.toolsDrainingButtonLabel, &FilMachineFontIcons_30, 0);
  lv_obj_align(gui.page.tools.toolsDrainingButtonLabel, LV_ALIGN_CENTER, 0, 0);

  gui.page.tools.toolsSelfcheckContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsSelfcheckContainer, LV_ALIGN_TOP_LEFT, -15, 135);
  lv_obj_set_size(gui.page.tools.toolsSelfcheckContainer, 330, 50);
  lv_obj_remove_flag(gui.page.tools.toolsSelfcheckContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsSelfcheckContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsSelfcheckContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolsSelfcheckLabel = lv_label_create(gui.page.tools.toolsSelfcheckContainer);
  lv_label_set_text(gui.page.tools.toolsSelfcheckLabel, selfCheck_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsSelfcheckLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolsSelfcheckLabel, LV_ALIGN_LEFT_MID, -5, 0);

  gui.page.tools.toolsSelfcheckButton =  lv_button_create(gui.page.tools.toolsSelfcheckContainer);
  lv_obj_set_size(gui.page.tools.toolsSelfcheckButton, BUTTON_PROCESS_WIDTH * 0.8, BUTTON_PROCESS_HEIGHT);
  lv_obj_align(gui.page.tools.toolsSelfcheckButton, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(gui.page.tools.toolsSelfcheckButton, event_toolsElement, LV_EVENT_CLICKED, gui.page.tools.toolsSelfcheckButton);
  lv_obj_set_style_bg_color(gui.page.tools.toolsSelfcheckButton, lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);

  gui.page.tools.toolsSelfcheckButtonLabel = lv_label_create(gui.page.tools.toolsSelfcheckButton);
  lv_label_set_text(gui.page.tools.toolsSelfcheckButtonLabel, play_icon);
  lv_obj_set_style_text_font(gui.page.tools.toolsSelfcheckButtonLabel, &FilMachineFontIcons_30, 0);
  lv_obj_align(gui.page.tools.toolsSelfcheckButtonLabel, LV_ALIGN_CENTER, 0, 0);
}

static void initTools_utilities(lv_obj_t *parent) {
  gui.page.tools.toolsUtilitiesLabel = lv_label_create(parent);
  lv_label_set_text(gui.page.tools.toolsUtilitiesLabel, Utilities_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsUtilitiesLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(gui.page.tools.toolsUtilitiesLabel, LV_ALIGN_TOP_LEFT, -7, 185);

  lv_style_set_line_width(&gui.page.tools.style_sectionTitleLine, 2);
  lv_style_set_line_color(&gui.page.tools.style_sectionTitleLine, lv_color_hex(LIGHT_BLUE));
  lv_style_set_line_rounded(&gui.page.tools.style_sectionTitleLine, true);

  gui.page.tools.sectionTitleLine = lv_line_create(parent);
  lv_line_set_points(gui.page.tools.sectionTitleLine, gui.page.tools.titleLinePoints, 2);
  lv_obj_add_style(gui.page.tools.sectionTitleLine, &gui.page.tools.style_sectionTitleLine, 0);
  lv_obj_align(gui.page.tools.sectionTitleLine, LV_ALIGN_TOP_MID, 0, 220);

  gui.page.tools.toolsImportContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsImportContainer, LV_ALIGN_TOP_LEFT, -15, 225);
  lv_obj_set_size(gui.page.tools.toolsImportContainer, 330, 50);
  lv_obj_remove_flag(gui.page.tools.toolsImportContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsImportContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsImportContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolsImportLabel = lv_label_create(gui.page.tools.toolsImportContainer);
  lv_label_set_text(gui.page.tools.toolsImportLabel, importConfigAndProcesses_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsImportLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolsImportLabel, LV_ALIGN_LEFT_MID, -5, 0);

  createQuestionMark(gui.page.tools.toolsImportContainer,gui.page.tools.toolsImportLabel,event_toolsPopup, 2, -3);

  gui.page.tools.toolsImportButton =  lv_button_create(gui.page.tools.toolsImportContainer);
  lv_obj_set_size(gui.page.tools.toolsImportButton, BUTTON_PROCESS_WIDTH * 0.8, BUTTON_PROCESS_HEIGHT);
  lv_obj_align(gui.page.tools.toolsImportButton, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(gui.page.tools.toolsImportButton, event_toolsElement, LV_EVENT_CLICKED, gui.page.tools.toolsImportButton);
  lv_obj_set_style_bg_color(gui.page.tools.toolsImportButton, lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);

  gui.page.tools.toolsImportButtonLabel = lv_label_create(gui.page.tools.toolsImportButton);
  lv_label_set_text(gui.page.tools.toolsImportButtonLabel, play_icon);
  lv_obj_set_style_text_font(gui.page.tools.toolsImportButtonLabel, &FilMachineFontIcons_30, 0);
  lv_obj_align(gui.page.tools.toolsImportButtonLabel, LV_ALIGN_CENTER, 0, 0);

  gui.page.tools.toolsExportContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsExportContainer, LV_ALIGN_TOP_LEFT, -15, 275);
  lv_obj_set_size(gui.page.tools.toolsExportContainer, 330, 50);
  lv_obj_remove_flag(gui.page.tools.toolsExportContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsExportContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsExportContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolsExportLabel = lv_label_create(gui.page.tools.toolsExportContainer);
  lv_label_set_text(gui.page.tools.toolsExportLabel, exportConfigAndProcesses_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsExportLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolsExportLabel, LV_ALIGN_LEFT_MID, -5, 0);

  createQuestionMark(gui.page.tools.toolsExportContainer,gui.page.tools.toolsExportLabel,event_toolsPopup, 2, -3);

  gui.page.tools.toolsExportButton =  lv_button_create(gui.page.tools.toolsExportContainer);
  lv_obj_set_size(gui.page.tools.toolsExportButton, BUTTON_PROCESS_WIDTH * 0.8, BUTTON_PROCESS_HEIGHT);
  lv_obj_align(gui.page.tools.toolsExportButton, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(gui.page.tools.toolsExportButton, event_toolsElement, LV_EVENT_CLICKED, gui.page.tools.toolsExportButton);
  lv_obj_set_style_bg_color(gui.page.tools.toolsExportButton, lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);

  gui.page.tools.toolsExportButtonLabel = lv_label_create(gui.page.tools.toolsExportButton);
  lv_label_set_text(gui.page.tools.toolsExportButtonLabel, play_icon);
  lv_obj_set_style_text_font(gui.page.tools.toolsExportButtonLabel, &FilMachineFontIcons_30, 0);
  lv_obj_align(gui.page.tools.toolsExportButtonLabel, LV_ALIGN_CENTER, 0, 0);
}

static void initTools_statistics(lv_obj_t *parent) {
  gui.page.tools.toolsStatisticsLabel = lv_label_create(parent);
  lv_label_set_text(gui.page.tools.toolsStatisticsLabel, Statistics_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsStatisticsLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(gui.page.tools.toolsStatisticsLabel, LV_ALIGN_TOP_LEFT, -7, 325);

  lv_style_set_line_width(&gui.page.tools.style_sectionTitleLine, 2);
  lv_style_set_line_color(&gui.page.tools.style_sectionTitleLine, lv_color_hex(LIGHT_BLUE));
  lv_style_set_line_rounded(&gui.page.tools.style_sectionTitleLine, true);

  gui.page.tools.sectionTitleLine = lv_line_create(parent);
  lv_line_set_points(gui.page.tools.sectionTitleLine, gui.page.tools.titleLinePoints, 2);
  lv_obj_add_style(gui.page.tools.sectionTitleLine, &gui.page.tools.style_sectionTitleLine, 0);
  lv_obj_align(gui.page.tools.sectionTitleLine, LV_ALIGN_TOP_MID, 0, 360);

  gui.page.tools.toolsStatCompleteProcessesContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsStatCompleteProcessesContainer, LV_ALIGN_TOP_LEFT, -15, 365);
  lv_obj_set_size(gui.page.tools.toolsStatCompleteProcessesContainer, 330, 40);
  lv_obj_remove_flag(gui.page.tools.toolsStatCompleteProcessesContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsStatCompleteProcessesContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsStatCompleteProcessesContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolStatCompletedProcessesLabel = lv_label_create(gui.page.tools.toolsStatCompleteProcessesContainer);
  lv_label_set_text(gui.page.tools.toolStatCompletedProcessesLabel, statCompleteProcesses_text);
  lv_obj_set_style_text_font(gui.page.tools.toolStatCompletedProcessesLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolStatCompletedProcessesLabel, LV_ALIGN_LEFT_MID, -5, 0);

  gui.page.tools.toolStatCompletedProcessesValue = lv_label_create(gui.page.tools.toolsStatCompleteProcessesContainer);
  lv_label_set_text_fmt(gui.page.tools.toolStatCompletedProcessesValue, "%"PRIu32"", gui.page.tools.machineStats.completed);
  lv_obj_set_style_text_font(gui.page.tools.toolStatCompletedProcessesValue, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolStatCompletedProcessesValue, LV_ALIGN_RIGHT_MID, -5, 0);

  gui.page.tools.toolsStatTotalTimeContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsStatTotalTimeContainer, LV_ALIGN_TOP_LEFT, -15, 405);
  lv_obj_set_size(gui.page.tools.toolsStatTotalTimeContainer, 330, 40);
  lv_obj_remove_flag(gui.page.tools.toolsStatTotalTimeContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsStatTotalTimeContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsStatTotalTimeContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolStatTotalTimeLabel = lv_label_create(gui.page.tools.toolsStatTotalTimeContainer);
  lv_label_set_text(gui.page.tools.toolStatTotalTimeLabel, statTotalProcessTime_text);
  lv_obj_set_style_text_font(gui.page.tools.toolStatTotalTimeLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolStatTotalTimeLabel, LV_ALIGN_LEFT_MID, -5, 0);

  gui.page.tools.toolStatTotalTimeValue = lv_label_create(gui.page.tools.toolsStatTotalTimeContainer);
  lv_label_set_text_fmt(gui.page.tools.toolStatTotalTimeValue, "%"PRIu64"", gui.page.tools.machineStats.totalMins);
  lv_obj_set_style_text_font(gui.page.tools.toolStatTotalTimeValue, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolStatTotalTimeValue, LV_ALIGN_RIGHT_MID, -5, 0);

  gui.page.tools.toolsStatCompleteCycleContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsStatCompleteCycleContainer, LV_ALIGN_TOP_LEFT, -15, 445);
  lv_obj_set_size(gui.page.tools.toolsStatCompleteCycleContainer, 330, 40);
  lv_obj_remove_flag(gui.page.tools.toolsStatCompleteCycleContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsStatCompleteCycleContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsStatCompleteCycleContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolStatCompleteCycleLabel = lv_label_create(gui.page.tools.toolsStatCompleteCycleContainer);
  lv_label_set_text(gui.page.tools.toolStatCompleteCycleLabel, statCompleteCleanProcess_text);
  lv_obj_set_style_text_font(gui.page.tools.toolStatCompleteCycleLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolStatCompleteCycleLabel, LV_ALIGN_LEFT_MID, -5, 0);

  gui.page.tools.toolStatCompleteCycleValue = lv_label_create(gui.page.tools.toolsStatCompleteCycleContainer);
  lv_label_set_text_fmt(gui.page.tools.toolStatCompleteCycleValue, "%"PRIu32"", gui.page.tools.machineStats.clean);
  lv_obj_set_style_text_font(gui.page.tools.toolStatCompleteCycleValue, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolStatCompleteCycleValue, LV_ALIGN_RIGHT_MID, -5, 0);

  gui.page.tools.toolsStatStoppedProcessesContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsStatStoppedProcessesContainer, LV_ALIGN_TOP_LEFT, -15, 485);
  lv_obj_set_size(gui.page.tools.toolsStatStoppedProcessesContainer, 330, 40);
  lv_obj_remove_flag(gui.page.tools.toolsStatStoppedProcessesContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsStatStoppedProcessesContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsStatStoppedProcessesContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolStatStoppedProcessesLabel = lv_label_create(gui.page.tools.toolsStatStoppedProcessesContainer);
  lv_label_set_text(gui.page.tools.toolStatStoppedProcessesLabel, statStoppedProcess_text);
  lv_obj_set_style_text_font(gui.page.tools.toolStatStoppedProcessesLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolStatStoppedProcessesLabel, LV_ALIGN_LEFT_MID, -5, 0);

  gui.page.tools.toolStatStoppedProcessesValue = lv_label_create(gui.page.tools.toolsStatStoppedProcessesContainer);
  lv_label_set_text_fmt(gui.page.tools.toolStatStoppedProcessesValue, "%"PRIu32"", gui.page.tools.machineStats.stopped);
  lv_obj_set_style_text_font(gui.page.tools.toolStatStoppedProcessesValue, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolStatStoppedProcessesValue, LV_ALIGN_RIGHT_MID, -5, 0);
}

static void initTools_software(lv_obj_t *parent) {
  gui.page.tools.toolsSoftwareLabel = lv_label_create(parent);
  lv_label_set_text(gui.page.tools.toolsSoftwareLabel, Software_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsSoftwareLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(gui.page.tools.toolsSoftwareLabel, LV_ALIGN_TOP_LEFT, -7, 685);

  lv_style_set_line_width(&gui.page.tools.style_sectionTitleLine, 2);
  lv_style_set_line_color(&gui.page.tools.style_sectionTitleLine, lv_color_hex(LIGHT_BLUE));
  lv_style_set_line_rounded(&gui.page.tools.style_sectionTitleLine, true);

  gui.page.tools.sectionTitleLine = lv_line_create(parent);
  lv_line_set_points(gui.page.tools.sectionTitleLine, gui.page.tools.titleLinePoints, 2);
  lv_obj_add_style(gui.page.tools.sectionTitleLine, &gui.page.tools.style_sectionTitleLine, 0);
  lv_obj_align(gui.page.tools.sectionTitleLine, LV_ALIGN_TOP_MID, 0, 720);

  gui.page.tools.toolsSoftwareVersionContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsSoftwareVersionContainer, LV_ALIGN_TOP_LEFT, -15, 725);
  lv_obj_set_size(gui.page.tools.toolsSoftwareVersionContainer, 330, 40);
  lv_obj_remove_flag(gui.page.tools.toolsSoftwareVersionContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsSoftwareVersionContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsSoftwareVersionContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolSoftWareVersionLabel = lv_label_create(gui.page.tools.toolsSoftwareVersionContainer);
  lv_label_set_text(gui.page.tools.toolSoftWareVersionLabel, softwareVersion_text);
  lv_obj_set_style_text_font(gui.page.tools.toolSoftWareVersionLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolSoftWareVersionLabel, LV_ALIGN_LEFT_MID, -5, 0);

  gui.page.tools.toolSoftwareVersionValue = lv_label_create(gui.page.tools.toolsSoftwareVersionContainer);
  lv_label_set_text(gui.page.tools.toolSoftwareVersionValue, ota_get_running_version());
  lv_obj_set_style_text_font(gui.page.tools.toolSoftwareVersionValue, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolSoftwareVersionValue, LV_ALIGN_RIGHT_MID, -5, 0);

  gui.page.tools.toolsSoftwareSerialContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsSoftwareSerialContainer, LV_ALIGN_TOP_LEFT, -15, 765);
  lv_obj_set_size(gui.page.tools.toolsSoftwareSerialContainer, 330, 40);
  lv_obj_remove_flag(gui.page.tools.toolsSoftwareSerialContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsSoftwareSerialContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsSoftwareSerialContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolSoftwareSerialLabel = lv_label_create(gui.page.tools.toolsSoftwareSerialContainer);
  lv_label_set_text(gui.page.tools.toolSoftwareSerialLabel, softwareSerialNum_text);
  lv_obj_set_style_text_font(gui.page.tools.toolSoftwareSerialLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolSoftwareSerialLabel, LV_ALIGN_LEFT_MID, -5, 0);

  gui.page.tools.toolSoftwareSerialValue = lv_label_create(gui.page.tools.toolsSoftwareSerialContainer);
  lv_label_set_text(gui.page.tools.toolSoftwareSerialValue, softwareSerialNumValue_text);
  lv_obj_set_style_text_font(gui.page.tools.toolSoftwareSerialValue, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolSoftwareSerialValue, LV_ALIGN_RIGHT_MID, -5, 0);

  gui.page.tools.toolCreditButton = lv_button_create(parent);
  lv_obj_set_size(gui.page.tools.toolCreditButton, BUTTON_TUNE_WIDTH, BUTTON_TUNE_HEIGHT);
  lv_obj_align(gui.page.tools.toolCreditButton, LV_ALIGN_TOP_MID, 5 , 805);
  lv_obj_add_event_cb(gui.page.tools.toolCreditButton, event_toolsPopup, LV_EVENT_CLICKED, gui.page.tools.toolCreditButton);
  lv_obj_set_style_bg_color(gui.page.tools.toolCreditButton, lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);

  gui.page.tools.toolCreditButtonLabel = lv_label_create(gui.page.tools.toolCreditButton);
  lv_label_set_text(gui.page.tools.toolCreditButtonLabel, softwareCredits_text);
  lv_obj_set_style_text_font(gui.page.tools.toolCreditButtonLabel, &lv_font_montserrat_18, 0);
  lv_obj_align(gui.page.tools.toolCreditButtonLabel, LV_ALIGN_CENTER, 0, 0);
}

static void initTools_update(lv_obj_t *parent) {
  /* Section header: "Update" */
  lv_obj_t *updateLabel = lv_label_create(parent);
  lv_label_set_text(updateLabel, otaUpdate_text);
  lv_obj_set_style_text_font(updateLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(updateLabel, LV_ALIGN_TOP_LEFT, -7, 525);

  lv_obj_t *updateLine = lv_line_create(parent);
  lv_line_set_points(updateLine, gui.page.tools.titleLinePoints, 2);
  lv_obj_add_style(updateLine, &gui.page.tools.style_sectionTitleLine, 0);
  lv_obj_align(updateLine, LV_ALIGN_TOP_MID, 0, 560);

  /* Update from SD */
  gui.page.tools.toolsUpdateContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsUpdateContainer, LV_ALIGN_TOP_LEFT, -15, 565);
  lv_obj_set_size(gui.page.tools.toolsUpdateContainer, 330, 50);
  lv_obj_remove_flag(gui.page.tools.toolsUpdateContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsUpdateContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsUpdateContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolsUpdateSDLabel = lv_label_create(gui.page.tools.toolsUpdateContainer);
  lv_label_set_text(gui.page.tools.toolsUpdateSDLabel, otaUpdateFromSD_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsUpdateSDLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolsUpdateSDLabel, LV_ALIGN_LEFT_MID, -5, 0);

  createQuestionMark(gui.page.tools.toolsUpdateContainer, gui.page.tools.toolsUpdateSDLabel, event_toolsPopup, 2, -3);

  gui.page.tools.toolsUpdateSDButton = lv_button_create(gui.page.tools.toolsUpdateContainer);
  lv_obj_set_size(gui.page.tools.toolsUpdateSDButton, BUTTON_PROCESS_WIDTH * 0.8, BUTTON_PROCESS_HEIGHT);
  lv_obj_align(gui.page.tools.toolsUpdateSDButton, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(gui.page.tools.toolsUpdateSDButton, event_toolsElement, LV_EVENT_CLICKED, gui.page.tools.toolsUpdateSDButton);
  lv_obj_set_style_bg_color(gui.page.tools.toolsUpdateSDButton, lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);

  gui.page.tools.toolsUpdateSDButtonLabel = lv_label_create(gui.page.tools.toolsUpdateSDButton);
  lv_label_set_text(gui.page.tools.toolsUpdateSDButtonLabel, play_icon);
  lv_obj_set_style_text_font(gui.page.tools.toolsUpdateSDButtonLabel, &FilMachineFontIcons_30, 0);
  lv_obj_align(gui.page.tools.toolsUpdateSDButtonLabel, LV_ALIGN_CENTER, 0, 0);

  /* Check for updates (Wi-Fi) */
  gui.page.tools.toolsUpdateWifiContainer = lv_obj_create(parent);
  lv_obj_align(gui.page.tools.toolsUpdateWifiContainer, LV_ALIGN_TOP_LEFT, -15, 615);
  lv_obj_set_size(gui.page.tools.toolsUpdateWifiContainer, 330, 50);
  lv_obj_remove_flag(gui.page.tools.toolsUpdateWifiContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(gui.page.tools.toolsUpdateWifiContainer, LV_DIR_VER);
  lv_obj_set_style_border_opa(gui.page.tools.toolsUpdateWifiContainer, LV_OPA_TRANSP, 0);

  gui.page.tools.toolsUpdateWifiLabel = lv_label_create(gui.page.tools.toolsUpdateWifiContainer);
  lv_label_set_text(gui.page.tools.toolsUpdateWifiLabel, otaWifiUpdate_text);
  lv_obj_set_style_text_font(gui.page.tools.toolsUpdateWifiLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(gui.page.tools.toolsUpdateWifiLabel, LV_ALIGN_LEFT_MID, -5, 0);

  createQuestionMark(gui.page.tools.toolsUpdateWifiContainer, gui.page.tools.toolsUpdateWifiLabel, event_toolsPopup, 2, -3);

  gui.page.tools.toolsUpdateWifiButton = lv_button_create(gui.page.tools.toolsUpdateWifiContainer);
  lv_obj_set_size(gui.page.tools.toolsUpdateWifiButton, BUTTON_PROCESS_WIDTH * 0.8, BUTTON_PROCESS_HEIGHT);
  lv_obj_align(gui.page.tools.toolsUpdateWifiButton, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(gui.page.tools.toolsUpdateWifiButton, event_toolsElement, LV_EVENT_CLICKED, gui.page.tools.toolsUpdateWifiButton);
  lv_obj_set_style_bg_color(gui.page.tools.toolsUpdateWifiButton, lv_color_hex(LIGHT_BLUE), LV_PART_MAIN);

  gui.page.tools.toolsUpdateWifiButtonLabel = lv_label_create(gui.page.tools.toolsUpdateWifiButton);
  lv_label_set_text(gui.page.tools.toolsUpdateWifiButtonLabel, play_icon);
  lv_obj_set_style_text_font(gui.page.tools.toolsUpdateWifiButtonLabel, &FilMachineFontIcons_30, 0);
  lv_obj_align(gui.page.tools.toolsUpdateWifiButtonLabel, LV_ALIGN_CENTER, 0, 0);
}

void initTools(void) {
  LV_LOG_USER("Tools Creation");
  gui.page.tools.toolsSection = lv_obj_create(lv_screen_active());
  lv_obj_set_pos(gui.page.tools.toolsSection, 140, 7);
  lv_obj_set_size(gui.page.tools.toolsSection, 335, 303);
  lv_obj_set_scroll_dir(gui.page.tools.toolsSection, LV_DIR_VER);

  initTools_maintenance(gui.page.tools.toolsSection);
  initTools_utilities(gui.page.tools.toolsSection);
  initTools_statistics(gui.page.tools.toolsSection);
  initTools_update(gui.page.tools.toolsSection);
  initTools_software(gui.page.tools.toolsSection);
}

void tools(void)
{
  if(gui.page.tools.toolsSection == NULL){
    initTools();
    guiUpdaterTimer = lv_timer_create(guiUpdater_timer, 1000,  NULL);
    lv_timer_pause(guiUpdaterTimer);  /* Start paused — will resume when tab is shown */
    lv_obj_add_flag(gui.page.tools.toolsSection, LV_OBJ_FLAG_HIDDEN);
  }
  /* Resume the timer when entering the Tools page */
  if(guiUpdaterTimer != NULL) lv_timer_resume(guiUpdaterTimer);
  lv_style_set_line_color(&gui.page.tools.style_sectionTitleLine, lv_palette_main(LV_PALETTE_BLUE));
}

void tools_pause_timer(void)
{
  if(guiUpdaterTimer != NULL) lv_timer_pause(guiUpdaterTimer);
}

void tools_resume_timer(void)
{
  if(guiUpdaterTimer != NULL) {
    guiUpdater_timer(guiUpdaterTimer);  /* Immediate refresh */
    lv_timer_resume(guiUpdaterTimer);
  }
}

void tools_delete_timer(void)
{
  safeTimerDelete(&guiUpdaterTimer);
}

