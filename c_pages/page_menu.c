
/**
 * @file page_menu.c
 *
 */


//ESSENTIAL INCLUDES
#include "FilMachine.h"

extern struct gui_components gui;

void event_tab_switch(lv_event_t * e) {
	
  lv_event_code_t code = lv_event_get_code(e);
  
  if(gui.page.menu.newSelection == TAB_PROCESSES && code == LV_EVENT_LONG_PRESSED){
      LV_LOG_USER("POPUP DELETE ALL PROCESSES");
      messagePopupCreate(deleteAllProcessPopupTitle_text,deleteAllProcessPopupBody_text, deleteButton_text, stepDetailCancel_text, &gui);
  }

  if(gui.page.menu.newSelection == TAB_SETTINGS && code == LV_EVENT_LONG_PRESSED){
      LV_LOG_USER("LONG PRESS Settings -> Wi-Fi popup");
      wifiPopupCreate();
  }


  if(code == LV_EVENT_CLICKED) {
    if(gui.page.menu.newTabSelected == NULL && gui.page.menu.oldTabSelected == NULL){
      gui.page.menu.newTabSelected = gui.page.menu.oldTabSelected = (lv_obj_t *)lv_event_get_target(e);
      gui.page.menu.newSelection = gui.page.menu.oldSelection = (uint8_t)lv_obj_get_index(gui.page.menu.newTabSelected);
      lv_obj_set_style_bg_color(gui.page.menu.newTabSelected, lv_color_hex(GREEN_DARK), 0);

      LV_LOG_USER("FIRST TIME");
    }
    else {
        gui.page.menu.oldTabSelected = gui.page.menu.newTabSelected;
        gui.page.menu.oldSelection = gui.page.menu.newSelection;
        gui.page.menu.newTabSelected = (lv_obj_t *)lv_event_get_target(e);
        gui.page.menu.newSelection = (uint8_t)lv_obj_get_index(gui.page.menu.newTabSelected);
        LV_LOG_USER("NEXT TIME");
    }
    
  
    if(gui.page.menu.newSelection != gui.page.menu.oldSelection) 
      { 
        //FIRST HIDE THE CORRECT SECTION
        if(gui.page.menu.oldSelection == TAB_PROCESSES){
            lv_obj_add_flag(gui.page.processes.processesSection, LV_OBJ_FLAG_HIDDEN);
        }
        if(gui.page.menu.oldSelection == TAB_SETTINGS){
            lv_obj_add_flag(gui.page.settings.settingsSection, LV_OBJ_FLAG_HIDDEN);
        }
        if(gui.page.menu.oldSelection == TAB_TOOLS){
            lv_obj_add_flag(gui.page.tools.toolsSection, LV_OBJ_FLAG_HIDDEN);
            tools_pause_timer();
        }

        //THEN SHOW THE NEW SECTION
        if(gui.page.menu.newSelection == TAB_PROCESSES){
            lv_obj_set_style_bg_color(gui.page.menu.newTabSelected, lv_color_hex(GREEN_DARK), 0);
            lv_obj_remove_local_style_prop(gui.page.menu.oldTabSelected, LV_STYLE_BG_COLOR, 0);

            lv_obj_set_style_border_color(gui.page.menu.processesTab, lv_color_hex(GREEN_LIGHT), 0);
            lv_obj_set_style_border_opa(gui.page.menu.processesTab, LV_OPA_MAX, 0);
            lv_obj_set_style_border_color(gui.page.menu.toolsTab, lv_color_hex(GREY), 0);
            lv_obj_set_style_border_opa(gui.page.menu.toolsTab, LV_OPA_50, 0);
            lv_obj_set_style_border_color(gui.page.menu.settingsTab, lv_color_hex(GREY), 0);
            lv_obj_set_style_border_opa(gui.page.menu.settingsTab, LV_OPA_50, 0);
            
            lv_obj_clear_flag(gui.page.processes.processesSection, LV_OBJ_FLAG_HIDDEN);
            lv_obj_scroll_to_y(gui.page.processes.processesListContainer, 0, LV_ANIM_OFF);
        }
        if(gui.page.menu.newSelection == TAB_SETTINGS){
            lv_obj_set_style_bg_color(gui.page.menu.newTabSelected, lv_color_hex(ORANGE_DARK), 0);
            lv_obj_remove_local_style_prop(gui.page.menu.oldTabSelected, LV_STYLE_BG_COLOR, 0); 

            lv_obj_set_style_border_color(gui.page.menu.processesTab, lv_color_hex(GREY), 0);
            lv_obj_set_style_border_opa(gui.page.menu.processesTab, LV_OPA_50, 0);
            lv_obj_set_style_border_color(gui.page.menu.toolsTab, lv_color_hex(GREY), 0);
            lv_obj_set_style_border_opa(gui.page.menu.toolsTab, LV_OPA_50, 0);
            lv_obj_set_style_border_color(gui.page.menu.settingsTab, lv_color_hex(ORANGE_LIGHT), 0);
            lv_obj_set_style_border_opa(gui.page.menu.settingsTab, LV_OPA_MAX, 0);

            lv_obj_clear_flag(gui.page.settings.settingsSection, LV_OBJ_FLAG_HIDDEN);
            lv_obj_scroll_to_y(gui.page.settings.settingsContainer, 0, LV_ANIM_OFF);
        }
        if(gui.page.menu.newSelection == TAB_TOOLS){
            lv_obj_set_style_bg_color(gui.page.menu.newTabSelected, lv_color_hex(BLUE_DARK), 0);
            lv_obj_remove_local_style_prop(gui.page.menu.oldTabSelected, LV_STYLE_BG_COLOR, 0);

            lv_obj_set_style_border_color(gui.page.menu.processesTab, lv_color_hex(GREY), 0);
            lv_obj_set_style_border_opa(gui.page.menu.processesTab, LV_OPA_50, 0);
            lv_obj_set_style_border_color(gui.page.menu.toolsTab, lv_color_hex(LIGHT_BLUE), 0);
            lv_obj_set_style_border_opa(gui.page.menu.toolsTab, LV_OPA_MAX, 0);
            lv_obj_set_style_border_color(gui.page.menu.settingsTab, lv_color_hex(GREY), 0);
            lv_obj_set_style_border_opa(gui.page.menu.settingsTab, LV_OPA_50, 0);

            lv_obj_clear_flag(gui.page.tools.toolsSection, LV_OBJ_FLAG_HIDDEN);
            lv_obj_scroll_to_y(gui.page.tools.toolsSection, 0, LV_ANIM_OFF);
            tools_resume_timer();
        }
      }
    
    LV_LOG_USER("OLD PRESSED %"PRIi32"", lv_obj_get_index(gui.page.menu.oldTabSelected));
    LV_LOG_USER("NEW PRESSED %"PRIi32"", lv_obj_get_index(gui.page.menu.newTabSelected));
     
  }   
}    


void menu(void) {
	
    lv_obj_del(lv_screen_active());
    gui.page.menu.screen_mainMenu = lv_obj_create(NULL);
    lv_scr_load(gui.page.menu.screen_mainMenu);

    processes();
    settings();
    tools();
    
    gui.page.menu.processesTab = lv_obj_create(gui.page.menu.screen_mainMenu);
    const ui_profile_t *ui = ui_get_profile();
    lv_obj_set_pos(gui.page.menu.processesTab, ui->common.sidebar_x, ui->menu.tab_processes_y);
    lv_obj_set_size(gui.page.menu.processesTab, ui->menu.tab_w, ui->menu.tab_h);
    lv_obj_add_event_cb(gui.page.menu.processesTab, event_tab_switch, LV_EVENT_CLICKED, gui.page.menu.processesTab);
    lv_obj_add_event_cb(gui.page.menu.processesTab, event_tab_switch, LV_EVENT_LONG_PRESSED, gui.page.menu.processesTab);
    lv_obj_set_style_border_color(gui.page.menu.processesTab, lv_color_hex(GREEN_LIGHT), 0);
    lv_obj_set_style_border_opa(gui.page.menu.processesTab, LV_OPA_MAX, 0);
    lv_obj_remove_flag(gui.page.menu.processesTab, LV_OBJ_FLAG_SCROLLABLE);      

          gui.page.menu.iconLabel = lv_label_create(gui.page.menu.processesTab);
          lv_label_set_text(gui.page.menu.iconLabel, tabProcess_icon);
          lv_obj_set_style_text_font(gui.page.menu.iconLabel, ui->menu.tab_icon_font, 0);
          lv_obj_align(gui.page.menu.iconLabel, LV_ALIGN_CENTER, ui->menu.tab_icon_offset_x, ui->menu.tab_icon_offset_y);


          gui.page.menu.label = lv_label_create(gui.page.menu.processesTab);
          lv_label_set_text(gui.page.menu.label, tabProcess_label);
          lv_obj_set_style_text_font(gui.page.menu.label, ui->menu.tab_label_font, 0);
          lv_obj_align(gui.page.menu.label, LV_ALIGN_CENTER, ui->menu.tab_label_offset_x, ui->menu.tab_label_offset_y);
          lv_obj_send_event(gui.page.menu.processesTab, LV_EVENT_CLICKED, gui.page.menu.processesTab);



    gui.page.menu.settingsTab = lv_obj_create(gui.page.menu.screen_mainMenu);
    lv_obj_set_pos(gui.page.menu.settingsTab, ui->common.sidebar_x, ui->menu.tab_settings_y);
    lv_obj_set_size(gui.page.menu.settingsTab, ui->menu.tab_w, ui->menu.tab_h);   
    lv_obj_add_event_cb(gui.page.menu.settingsTab, event_tab_switch, LV_EVENT_CLICKED, gui.page.menu.settingsTab);
    lv_obj_add_event_cb(gui.page.menu.settingsTab, event_tab_switch, LV_EVENT_LONG_PRESSED, gui.page.menu.settingsTab);
    lv_obj_set_style_border_color(gui.page.menu.settingsTab, lv_color_hex(GREY), 0);
    lv_obj_set_style_border_opa(gui.page.menu.settingsTab, LV_OPA_50, 0);
    lv_obj_remove_flag(gui.page.menu.settingsTab, LV_OBJ_FLAG_SCROLLABLE);      

          gui.page.menu.iconLabel = lv_label_create(gui.page.menu.settingsTab);
          lv_label_set_text(gui.page.menu.iconLabel, tabSetting_icon);
          lv_obj_set_style_text_font(gui.page.menu.iconLabel, ui->menu.tab_icon_font, 0);
          lv_obj_align(gui.page.menu.iconLabel, LV_ALIGN_CENTER, ui->menu.tab_icon_offset_x, ui->menu.tab_icon_offset_y);

          gui.page.menu.label = lv_label_create(gui.page.menu.settingsTab);
          lv_label_set_text(gui.page.menu.label, tabSetting_label);
          lv_obj_set_style_text_font(gui.page.menu.label, ui->menu.tab_label_font, 0);
          lv_obj_align(gui.page.menu.label, LV_ALIGN_CENTER, ui->menu.tab_label_offset_x, ui->menu.tab_label_offset_y);

          /* Wi-Fi connected indicator — top-left corner, hidden by default */
          gui.page.menu.wifiStatusIcon = lv_label_create(gui.page.menu.settingsTab);
          lv_label_set_text(gui.page.menu.wifiStatusIcon, LV_SYMBOL_WIFI);
          lv_obj_set_style_text_font(gui.page.menu.wifiStatusIcon, ui->menu.wifi_icon_font, 0);
          lv_obj_set_style_text_color(gui.page.menu.wifiStatusIcon, lv_color_hex(GREEN), 0);
          lv_obj_set_style_transform_rotation(gui.page.menu.wifiStatusIcon, 1350, 0);
          lv_obj_align(gui.page.menu.wifiStatusIcon, LV_ALIGN_TOP_LEFT, ui->menu.wifi_icon_x, ui->menu.wifi_icon_y);
          lv_obj_add_flag(gui.page.menu.wifiStatusIcon, LV_OBJ_FLAG_HIDDEN);

    gui.page.menu.toolsTab = lv_obj_create(gui.page.menu.screen_mainMenu);
    lv_obj_set_pos(gui.page.menu.toolsTab, ui->common.sidebar_x, ui->menu.tab_tools_y);
    lv_obj_set_size(gui.page.menu.toolsTab, ui->menu.tab_w, ui->menu.tab_h);   
    lv_obj_add_event_cb(gui.page.menu.toolsTab, event_tab_switch, LV_EVENT_CLICKED, gui.page.menu.toolsTab);
    lv_obj_set_style_border_color(gui.page.menu.toolsTab, lv_color_hex(GREY), 0);
    lv_obj_set_style_border_opa(gui.page.menu.toolsTab, LV_OPA_50, 0);
    lv_obj_remove_flag(gui.page.menu.toolsTab, LV_OBJ_FLAG_SCROLLABLE);      

          gui.page.menu.iconLabel = lv_label_create(gui.page.menu.toolsTab);
          lv_label_set_text(gui.page.menu.iconLabel, tabTools_icon);
          lv_obj_set_style_text_font(gui.page.menu.iconLabel, ui->menu.tab_icon_font, 0);
          lv_obj_align(gui.page.menu.iconLabel, LV_ALIGN_CENTER, ui->menu.tab_icon_offset_x, ui->menu.tab_icon_offset_y);


          gui.page.menu.label = lv_label_create(gui.page.menu.toolsTab);
          lv_label_set_text(gui.page.menu.label, tabTools_label);
          lv_obj_set_style_text_font(gui.page.menu.label, ui->menu.tab_label_font, 0);
          lv_obj_align(gui.page.menu.label, LV_ALIGN_CENTER, ui->menu.tab_label_offset_x, ui->menu.tab_label_offset_y);
}




