/**
 * @file page_home.c
 *
 */

#include "FilMachine.h"


extern struct gui_components		gui;
extern uint8_t						initErrors;

void event_btn_start(lv_event_t * e) {
	
	lv_event_code_t code = lv_event_get_code(e);
	LV_LOG_USER("initErrors %d",initErrors);
	if(code == LV_EVENT_CLICKED) {
	 	if(initErrors == 0 || ENABLE_BOOT_ERRORS == 0) {
			menu();
		} else {
			rebootBoard();
		}
	}
} 

void homePage(void) {

    const ui_home_layout_t *ui = &ui_get_profile()->home;

//    lv_obj_del(lv_screen_active());  // Shouldn't be required

    gui.page.home.screen_home = lv_obj_create(NULL);

    lv_scr_load(gui.page.home.screen_home);
    
    if(initErrors == 0 || ENABLE_BOOT_ERRORS == 0){
        gui.page.home.splashImage = lv_img_create(gui.page.home.screen_home);
        lv_img_set_src(gui.page.home.splashImage, &splash_img);
        lv_obj_align(gui.page.home.splashImage, LV_ALIGN_CENTER, 0 , 0);
        //lv_img_set_zoom(logo, 128);

        gui.page.home.startButton = lv_obj_create(gui.page.home.screen_home);
        lv_obj_align(gui.page.home.startButton, LV_ALIGN_BOTTOM_RIGHT, ui->normal_start_x , ui->normal_start_y);                  
        lv_obj_set_size(gui.page.home.startButton, ui->normal_start_w, ui->normal_start_h);   
        lv_obj_add_event_cb(gui.page.home.startButton, event_btn_start, LV_EVENT_CLICKED, gui.page.home.startButton);
        lv_obj_set_style_bg_opa(gui.page.home.startButton, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(gui.page.home.startButton, LV_OPA_TRANSP, 0);
        lv_obj_remove_flag(gui.page.home.startButton, LV_OBJ_FLAG_SCROLLABLE);   
      } else{
        if(ENABLE_BOOT_ERRORS){          
          lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(RED), LV_PART_MAIN);

          gui.page.home.startButton = lv_obj_create(gui.page.home.screen_home);
          lv_obj_align(gui.page.home.startButton, LV_ALIGN_CENTER, 0 , ui->error_start_y);                  
          lv_obj_set_size(gui.page.home.startButton, ui->error_start_size, ui->error_start_size);   
          lv_obj_add_event_cb(gui.page.home.startButton, event_btn_start, LV_EVENT_CLICKED, gui.page.home.startButton);
          lv_obj_set_style_bg_color(gui.page.home.startButton, lv_color_hex(RED), LV_PART_MAIN);
          lv_obj_set_style_border_opa(gui.page.home.startButton, LV_OPA_TRANSP, 0);



          gui.page.home.errorButtonLabel = lv_label_create(gui.page.home.startButton);         
          lv_obj_set_style_text_font(gui.page.home.errorButtonLabel, ui->error_icon_font, 0);              
          lv_obj_align(gui.page.home.errorButtonLabel, LV_ALIGN_CENTER, 0, 0);
          
          gui.page.home.errorLabel = lv_label_create(gui.page.home.screen_home);
          lv_obj_align(gui.page.home.errorLabel, LV_ALIGN_CENTER, 0 , ui->error_label_y); 

          if(initErrors == INIT_ERROR_SD && ENABLE_BOOT_ERRORS)
          {
            lv_label_set_text(gui.page.home.errorButtonLabel, sdCard_icon);
            lv_label_set_text(gui.page.home.errorLabel, initSDError_text);
          }   
          if(initErrors == INIT_ERROR_WIRE && ENABLE_BOOT_ERRORS)
          {
            lv_label_set_text(gui.page.home.errorButtonLabel, alert_icon);
            lv_label_set_text(gui.page.home.errorLabel, initWIREError_text);
          }  
          if((initErrors == INIT_ERROR_I2C_MCP || initErrors == INIT_ERROR_I2C_ADS) && ENABLE_BOOT_ERRORS)
          {
            lv_label_set_text(gui.page.home.errorButtonLabel, chip_icon);
            lv_label_set_text(gui.page.home.errorLabel, initI2CError_text);
          }  
          lv_obj_set_style_text_font(gui.page.home.errorLabel, ui->error_text_font, 0);              
          lv_obj_set_style_text_align(gui.page.home.errorLabel , LV_TEXT_ALIGN_CENTER, 0);
        }
      }
}
