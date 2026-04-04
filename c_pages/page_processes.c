/**
 * @file page_processes.c
 *
 */


//ESSENTIAL INCLUDE
#include "FilMachine.h"

extern struct gui_components gui;

//ACCESSORY INCLUDES


void event_processes_style_delete(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_DELETE) {
        //list of all styles to be reset, so clean the memory.
        lv_style_reset(&gui.page.processes.style_sectionTitleLine);
    }
}

void event_tabProcesses(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);

  if(obj == gui.page.processes.processFilterButton){
    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("New Filter Creation popup");
        if(gui.element.filterPopup.mBoxFilterPopupParent == NULL){
          filterPopupCreate();
          LV_LOG_USER("New Filter created!");
        }
        else{
              LV_LOG_USER("Filter already created!");
              lv_obj_remove_flag(gui.element.filterPopup.mBoxFilterPopupParent, LV_OBJ_FLAG_HIDDEN);
          }
    }
  }

  
  if(obj == gui.page.processes.newProcessButton){
    if(code == LV_EVENT_CLICKED) {
      gui.tempProcessNode = NULL;
      processDetail(gui.page.processes.processesListContainer);
      LV_LOG_USER("New Process Creation popup");
    }
  }
}
 

/* ── Update "N Processes" title label and reposition "+" button ── */
void refreshProcessesLabel(void) {
    if (gui.page.processes.processesLabel) {
        int32_t count = gui.page.processes.processElementsList.size;
        if (count > 0)
            lv_label_set_text_fmt(gui.page.processes.processesLabel,
                                  "%"PRIi32" " Processes_text, count);
        else
            lv_label_set_text(gui.page.processes.processesLabel, Processes_text);

        /* Keep the "+" button right after the label text */
        if (gui.page.processes.newProcessButton) {
            lv_obj_set_width(gui.page.processes.processesLabel, LV_SIZE_CONTENT);
            lv_obj_update_layout(gui.page.processes.processesLabel);
            lv_obj_align_to(gui.page.processes.newProcessButton,
                            gui.page.processes.processesLabel,
                            LV_ALIGN_OUT_RIGHT_MID, 8, 0);
        }
    }
}

static void initProcesses(void){
  /*********************
  *    PAGE HEADER
  *********************/
  const ui_profile_t *ui = ui_get_profile();
  LV_LOG_USER("Processes Creation");
  gui.page.processes.processesSection = lv_obj_create(gui.page.menu.screen_mainMenu);
  lv_obj_set_pos(gui.page.processes.processesSection, ui->common.content_x, ui->common.content_y);
  lv_obj_set_size(gui.page.processes.processesSection, ui->common.content_w, ui->common.content_h); 
  lv_obj_remove_flag(gui.page.processes.processesSection, LV_OBJ_FLAG_SCROLLABLE); 
  lv_obj_set_style_border_color(gui.page.processes.processesSection, lv_color_hex(GREEN_LIGHT), 0);
  

  /*********************
  *    PAGE ELEMENTS
  *********************/
  //ADD NEW PROCESS BUTTON
  gui.page.processes.newProcessButton = lv_obj_create(gui.page.processes.processesSection);
  lv_obj_align(gui.page.processes.newProcessButton, LV_ALIGN_TOP_LEFT, ui->processes.add_btn_x, ui->processes.add_btn_y);
  lv_obj_set_size(gui.page.processes.newProcessButton, ui->processes.add_btn_w, ui->processes.add_btn_h); 
  lv_obj_remove_flag(gui.page.processes.newProcessButton, LV_OBJ_FLAG_SCROLLABLE); 
  lv_obj_set_style_border_opa(gui.page.processes.newProcessButton, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(gui.page.processes.newProcessButton, event_tabProcesses, LV_EVENT_CLICKED, gui.page.processes.newProcessButton);

      //PROCESSES LABEL (direct child of section, like Settings/Tools)
      gui.page.processes.processesLabel = lv_label_create(gui.page.processes.processesSection);
      lv_label_set_text(gui.page.processes.processesLabel, Processes_text);
      lv_obj_set_style_text_font(gui.page.processes.processesLabel, ui->processes.title_font, 0);
      lv_obj_align(gui.page.processes.processesLabel, LV_ALIGN_TOP_LEFT, ui->processes.title_label_x, ui->processes.title_label_y);

      gui.page.processes.iconNewProcessLabel = lv_label_create(gui.page.processes.newProcessButton);
      lv_label_set_text(gui.page.processes.iconNewProcessLabel, newProcess_icon);
      lv_obj_set_style_text_font(gui.page.processes.iconNewProcessLabel, ui->processes.header_icon_font, 0);
      lv_obj_align(gui.page.processes.iconNewProcessLabel, LV_ALIGN_CENTER, 0, 0);


  //FILTER BUTTON CONTAINER
  gui.page.processes.processFilterButton = lv_obj_create(gui.page.processes.processesSection);
  lv_obj_align(gui.page.processes.processFilterButton, LV_ALIGN_TOP_RIGHT, ui->processes.filter_btn_x, ui->processes.add_btn_y);
  lv_obj_set_size(gui.page.processes.processFilterButton, ui->processes.filter_btn_size, ui->processes.filter_btn_size); 
  lv_obj_remove_flag(gui.page.processes.processFilterButton, LV_OBJ_FLAG_SCROLLABLE); 
  lv_obj_set_style_border_opa(gui.page.processes.processFilterButton, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(gui.page.processes.processFilterButton, event_tabProcesses, LV_EVENT_CLICKED, gui.page.processes.processFilterButton);
                
      //FILTER BUTTON ICON
      gui.page.processes.iconFilterLabel = lv_label_create(gui.page.processes.processFilterButton);          
      lv_label_set_text(gui.page.processes.iconFilterLabel, funnel_icon);                  
      lv_obj_set_style_text_font(gui.page.processes.iconFilterLabel, ui->processes.header_icon_font, 0);
      lv_obj_align(gui.page.processes.iconFilterLabel, LV_ALIGN_CENTER, 0, 0);


  gui.page.processes.processesListContainer = lv_obj_create(gui.page.processes.processesSection);
  lv_obj_set_pos(gui.page.processes.processesListContainer, ui->processes.list_x, ui->processes.list_y);
  lv_obj_set_size(gui.page.processes.processesListContainer, ui->processes.list_w, ui->processes.list_h);
  lv_obj_set_style_border_opa(gui.page.processes.processesListContainer, LV_OPA_TRANSP, 0);
  lv_obj_set_style_bg_opa(gui.page.processes.processesListContainer, LV_OPA_COVER, 0);
  lv_obj_set_scroll_dir(gui.page.processes.processesListContainer, LV_DIR_VER);

  //CREATE STYLE AND LINE FOR THE TITLE
  /*Create style*/
  lv_style_init(&gui.page.processes.style_sectionTitleLine);
  lv_style_set_line_width(&gui.page.processes.style_sectionTitleLine, ui_get_profile()->title_line_width);
  lv_style_set_line_color(&gui.page.processes.style_sectionTitleLine, lv_palette_main(LV_PALETTE_GREEN));
  lv_style_set_line_rounded(&gui.page.processes.style_sectionTitleLine, true);

  /*Create a line and apply the new style*/
  gui.page.processes.sectionTitleLine = lv_line_create(gui.page.processes.processesSection);
  lv_line_set_points(gui.page.processes.sectionTitleLine, gui.page.processes.titleLinePoints, 2);
  lv_obj_add_style(gui.page.processes.sectionTitleLine, &gui.page.processes.style_sectionTitleLine, 0);
  gui.page.processes.titleLinePoints[1].x = ui->common.title_line_w;
  lv_line_set_points(gui.page.processes.sectionTitleLine, gui.page.processes.titleLinePoints, 2);
  lv_obj_align(gui.page.processes.sectionTitleLine, LV_ALIGN_TOP_MID, ui->common.title_line_x, ui->common.title_line_y);

  //lv_obj_update_layout(gui.page.processes.processesSection);

  /* Ensure buttons, title label and title line stay on top of the list container (Z-order) */
  lv_obj_move_foreground(gui.page.processes.newProcessButton);
  lv_obj_move_foreground(gui.page.processes.processFilterButton);
  lv_obj_move_foreground(gui.page.processes.processesLabel);
  lv_obj_move_foreground(gui.page.processes.sectionTitleLine);
}


void processes(void)
{   
  if(gui.page.processes.processesSection == NULL){
    initProcesses();
    loadSDCardProcesses();
  }

  lv_obj_clear_flag(gui.page.processes.processesSection, LV_OBJ_FLAG_HIDDEN);
  lv_style_set_line_color(&gui.page.processes.style_sectionTitleLine, lv_palette_main(LV_PALETTE_GREEN));
  refreshProcessesLabel();
}


