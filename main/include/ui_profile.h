#ifndef UI_PROFILE_H
#define UI_PROFILE_H

#include "lvgl.h"
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int top_margin;
    int sidebar_x;
    int sidebar_w;
    int content_x;
    int content_y;
    int content_w;
    int content_h;
    int title_line_w;
    int title_line_y;
} ui_common_layout_t;


typedef struct {
    int normal_start_x;
    int normal_start_y;
    int normal_start_w;
    int normal_start_h;
    int error_start_y;
    int error_start_size;
    int error_label_y;
    const lv_font_t *error_icon_font;
    const lv_font_t *error_text_font;
} ui_home_layout_t;

typedef struct {
    int tab_w;
    int tab_h;
    int tab_gap;
    int tab1_y;
    int tab2_y;
    int tab3_y;
    int tab_icon_offset_y;
    int tab_label_offset_y;
    const lv_font_t *icon_font;
    const lv_font_t *label_font;
} ui_menu_layout_t;

typedef struct {
    int header_btn_w;
    int header_btn_h;
    int header_btn_y;
    int title_label_x;
    int filter_btn_size;
    int list_x;
    int list_y;
    int list_w;
    int list_h;
    const lv_font_t *title_font;
    const lv_font_t *icon_font;
} ui_processes_layout_t;

typedef struct {
    int left_x;
    int gap_y;
    int row_h;
    int slider_h;
    int row_w;
    int textarea_w;
    int textarea_h;
    int section_label_x;
    int section_label_y;
    int section_line_y;
    int scroll_x;
    int scroll_y;
    int scroll_w;
    int scroll_h;
    int tune_button_x;
    int tune_button_y;
    int slider_y;
    int value_x;
    int value_y;
    int text_area_x;
    int text_area_y;
    const lv_font_t *section_title_font;
    const lv_font_t *label_font;
    const lv_font_t *value_font;
    const lv_font_t *button_font;
    const lv_font_t *radio_font;
    const lv_font_t *text_area_font;
} ui_settings_layout_t;

typedef struct {
    int container_w;
    int container_h;
    int title_y;
    int title_line_w;
    int title_line_y;
    int close_scale_pct;
    int close_x;
    int close_y;
    int name_x;
    int name_y;
    int name_w;
    int name_h;
    int name_ta_x;
    int name_ta_y;
    int name_ta_w;
    int steps_label_x;
    int steps_label_y;
    int steps_x;
    int steps_y;
    int steps_w;
    int steps_h;
    int new_step_x;
    int new_step_y;
    int info_label_x;
    int info_label_y;
    int info_x;
    int info_y;
    int info_w;
    int info_h;
    int info_inner_x;
    int temp_ctrl_y;
    int temp_row_h;
    int temp_label_x;
    int temp_ta_x;
    int temp_unit_x;
    int total_label_x;
    int preferred_x;
    int preferred_y;
    int save_x;
    int save_y;
    int run_x;
    int run_y;
    const lv_font_t *title_font;
    const lv_font_t *name_font;
    const lv_font_t *section_font;
    const lv_font_t *button_icon_font;
    const lv_font_t *label_font;
    const lv_font_t *value_font;
    const lv_font_t *text_area_font;
    const lv_font_t *film_icon_font;
} ui_process_detail_layout_t;

typedef struct {
    int container_w;
    int container_h;
    int title_y;
    int title_line_w;
    int title_line_y;
    int row_x;
    int row_w;
    int row_h;
    int name_y;
    int duration_y;
    int type_y;
    int source_y;
    int discard_y;
    int name_ta_x;
    int name_ta_w;
    int min_ta_x;
    int sec_ta_x;
    int time_ta_w;
    int type_dd_x;
    int type_dd_w;
    int type_icon_x;
    int source_dd_x;
    int source_dd_w;
    int source_temp_label_x;
    int source_temp_icon_x;
    int source_temp_value_x;
    int discard_switch_x;
    int save_x;
    int save_y;
    int cancel_x;
    int cancel_y;
    const lv_font_t *title_font;
    const lv_font_t *label_font;
    const lv_font_t *value_font;
    const lv_font_t *button_font;
    const lv_font_t *icon_font;
} ui_step_detail_layout_t;

typedef struct {
    int stage_panel_w;
    int stage_panel_h;
    int stage_panel_x;
    int stage_panel_y;
    int close_w;
    int close_h;
    int close_x;
    int close_y;
    int process_name_x;
    int process_name_y;
    int process_name_w;
    int process_name_h;
    int process_name_label_x;
    int process_name_label_y;
    int process_name_label_w;
    int left_panel_x;
    int left_panel_y;
    int left_panel_w;
    int left_panel_h;
    int step_panel_x;
    int step_panel_y;
    int step_panel_w;
    int step_panel_h;
    int left_title_x;
    int left_title_y;
    int left_body_intro_y;
    int left_row_x;
    int left_row_w;
    int left_row_h;
    int left_row1_y;
    int left_row2_y;
    int left_row3_y;
    int left_status_icon_x;
    int left_status_label_x;
    int left_status_label_w;
    int processing_row_w;
    int processing_row1_y;
    int processing_row2_y;
    int processing_row3_y;
    int processing_row4_y;
    int processing_value_x;
    int processing_next_step_w;
    int processing_stop_y;
    int processing_stop_left_x;
    int processing_stop_right_x;
    int stage_title_w;
    int stage_title_x;
    int stage_title_y;
    int stage_textarea_y1;
    int stage_textarea_y2;
    int stage_textarea_w;
    int stage_button_y;
    int fill_label_w;
    int fill_label_y;
    int reach_title_w;
    int reach_title_y;
    int target_title_w;
    int target_title_h;
    int target_title_y;
    int target_title_value_y;
    int target_tolerance_y;
    int continue_button_w;
    int target_temp_y;
    int target_temp_w;
    int target_temp_h;
    int target_temp_left_x;
    int target_temp_right_x;
    int target_temp_label_y;
    int target_temp_value_y;
    int heater_status_y;
    int film_box_w;
    int film_box_h;
    int film_box1_y;
    int film_box2_y;
    int film_label_y;
    int film_value_y;
    int processing_container_x;
    int processing_container_y;
    int processing_container_w;
    int processing_container_h;
    int processing_arc_size;
    int processing_arc_center_y;
    int processing_time_y;
    int processing_step_name_y;
    int processing_step_name_w;
    int processing_step_time_y;
    int processing_step_arc_size;
    int processing_step_arc_center_y;
    int processing_kind_y;
    int processing_pump_arc_size;
    int processing_complete_y;
    const lv_font_t *close_icon_font;
    const lv_font_t *process_name_font;
    const lv_font_t *left_title_font;
    const lv_font_t *left_body_font;
    const lv_font_t *left_value_font;
    const lv_font_t *button_font;
    const lv_font_t *stage_title_font;
    const lv_font_t *stage_label_font;
    const lv_font_t *stage_value_font;
    const lv_font_t *small_font;
    const lv_font_t *status_icon_font;
    const lv_font_t *target_title_font;
    const lv_font_t *target_value_font;
} ui_checkup_layout_t;


typedef struct {
    int section_label_x;
    int section_title_y_maintenance;
    int section_title_y_utilities;
    int section_title_y_statistics;
    int section_title_y_update;
    int section_title_y_software;
    int section_line_y_maintenance;
    int section_line_y_utilities;
    int section_line_y_statistics;
    int section_line_y_update;
    int section_line_y_software;
    int row_x;
    int row_w;
    int action_row_h;
    int stat_row_h;
    int row_label_x;
    int row_button_x;
    int maintenance_clean_y;
    int maintenance_drain_y;
    int maintenance_selfcheck_y;
    int utilities_import_y;
    int utilities_export_y;
    int statistics_completed_y;
    int statistics_total_time_y;
    int statistics_clean_cycles_y;
    int statistics_stopped_y;
    int update_sd_y;
    int update_wifi_y;
    int software_version_y;
    int software_serial_y;
    int software_credits_y;
    int question_mark_x;
    int question_mark_y;
    int credit_button_x;
    const lv_font_t *section_font;
    const lv_font_t *row_font;
    const lv_font_t *button_icon_font;
    const lv_font_t *credit_button_font;
} ui_tools_layout_t;


typedef struct {
    int title_y;
    int title_line_y;
    int inner_y;
    int wheel_w;
    int wheel_visible_rows;
    int button_y;
    const lv_font_t *title_font;
    const lv_font_t *button_font;
} ui_roller_popup_layout_t;

typedef struct {
    int title_y;
    int title_line_y;
    int settings_y;
    int subtitle_y;
    int chem_container_x;
    int chem_container_y;
    int chem_container_w;
    int chem_container_h;
    int checkbox_w;
    int checkbox_h;
    int checkbox1_x;
    int checkbox2_x;
    int checkbox3_x;
    int checkbox_label_x;
    int checkbox_radio_size;
    int spinbox_container_y;
    int spinbox_container_w;
    int spinbox_container_h;
    int spinbox_w;
    int spinbox_x;
    int repeat_label_x;
    int drain_container_y;
    int drain_container_w;
    int drain_label_x;
    int drain_switch_x;
    int run_button_x;
    int run_button_y;
    int cancel_button_x;
    int cancel_button_y;
    int process_arc_size;
    int cycle_arc_size;
    int pump_arc_size;
    int process_arc_y;
    int remaining_y;
    int now_cleaning_label_y;
    int now_cleaning_value_y;
    int now_step_y;
    int stop_button_y;
    const lv_font_t *title_font;
    const lv_font_t *subtitle_font;
    const lv_font_t *label_font;
    const lv_font_t *button_font;
    const lv_font_t *time_font;
    const lv_font_t *value_font;
    const lv_font_t *step_font;
} ui_clean_popup_layout_t;

typedef struct {
    int title_y;
    int title_line_y;
    int confirm_y;
    int info_w;
    int info_y;
    int button_x;
    int button_y;
    int process_y;
    int process_pad;
    int bar_w;
    int bar_h;
    int bar_gap;
    int bars_area_w;
    int bar_y;
    int label_y;
    int status_w;
    int status_y;
    int waste_y;
    int time_y;
    int stop_y;
    int bar_radius;
    const lv_font_t *title_font;
    const lv_font_t *info_font;
    const lv_font_t *bar_label_font;
    const lv_font_t *status_font;
    const lv_font_t *secondary_font;
    const lv_font_t *button_font;
} ui_drain_popup_layout_t;

typedef struct {
    int title_y;
    int title_line_y;
    int close_x;
    int close_y;
    int close_icon_size;
    int name_container_x;
    int name_container_y;
    int name_container_w;
    int name_container_h;
    int name_label_x;
    int name_textarea_x;
    int name_textarea_w;
    int options_row1_y;
    int options_row2_y;
    int options_container_x;
    int options_container_w;
    int options_container_h;
    int option_box1_x;
    int option_box2_x;
    int option_box3_x;
    int option_box_w;
    int option_box_h;
    int option_label_x;
    int option_radio_x;
    int option_radio_size;
    int preferred_switch_x;
    int button_y;
    int button_x;
    const lv_font_t *title_font;
    const lv_font_t *label_font;
    const lv_font_t *button_font;
    const lv_font_t *close_icon_font;
} ui_filter_popup_layout_t;

typedef struct {
    int title_y;
    int title_line_y;
    int close_x;
    int close_y;
    int close_icon_size;
    int left_panel_x;
    int left_panel_y;
    int left_panel_w;
    int left_panel_h;
    int tasks_label_x;
    int tasks_label_y;
    int phase_icon_x;
    int phase_name_x;
    int phase_row_y;
    int phase_row_gap;
    int right_panel_x;
    int right_panel_y;
    int right_panel_w;
    int right_panel_h;
    int phase_title_w;
    int phase_title_y;
    int phase_desc_w;
    int phase_desc_y;
    int phase_status_w;
    int phase_status_y;
    int phase_timer_y;
    int progress_w;
    int progress_h;
    int progress_y;
    int button_w;
    int button_h;
    int button_y;
    int stop_button_x;
    int start_button_x;
    int advance_button_x;
    const lv_font_t *title_font;
    const lv_font_t *tasks_font;
    const lv_font_t *phase_icon_font;
    const lv_font_t *phase_name_font;
    const lv_font_t *phase_title_font;
    const lv_font_t *phase_desc_font;
    const lv_font_t *phase_status_font;
    const lv_font_t *button_font;
    const lv_font_t *advance_button_font;
    const lv_font_t *close_icon_font;
} ui_selfcheck_popup_layout_t;

typedef struct {
    int title_y;
    int title_line_y;
    int close_x;
    int close_y;
    int close_icon_size;
    int ip_w;
    int ip_y;
    int pin_w;
    int pin_y;
    int status_w;
    int status_y;
    int progress_h;
    int progress_y;
    int progress_radius;
    int progress_status_y;
    int progress_bar_y;
    int percent_y;
    int progress_popup_radius;
    const lv_font_t *title_font;
    const lv_font_t *close_icon_font;
    const lv_font_t *ip_font;
    const lv_font_t *pin_font;
    const lv_font_t *status_font;
    const lv_font_t *progress_status_font;
    const lv_font_t *percent_font;
} ui_ota_popup_layout_t;

typedef struct {
    int text_container_y;
    int text_container_w;
    int text_container_h;
    int text_w;
    int info_close_x;
    int info_close_y;
    int button1_x;
    int button2_x;
    int button_y;
    int title_y;           /* was hardcoded -10 */
    int title_line_y;      /* was hardcoded 23  */
    const lv_font_t *title_font;
    const lv_font_t *text_font;
    const lv_font_t *button_font;
    const lv_font_t *close_icon_font;
} ui_message_popup_layout_t;

/* ── Process list element layout (was in element_process.c magic numbers) ── */
typedef struct {
    int item_w;
    int item_h;
    int item_x;
    int y_start;
    int swipe_offset;
    int delete_btn_w;
    int delete_btn_h;
    int delete_btn_x;
    int delete_btn_y;
    int delete_icon_x;
    int summary_w;
    int summary_h;
    int summary_x;
    int summary_y;
    int name_w;
    int name_x;
    int name_y;
    int temp_icon_x;
    int temp_icon_y;
    int temp_value_x;
    int temp_value_y;
    int time_icon_x;
    int time_icon_y;
    int time_value_x;
    int time_value_y;
    int time_icon_no_temp_x;
    int time_value_no_temp_x;
    int type_icon_x;
    int preferred_icon_x;
    const lv_font_t *name_font;
    const lv_font_t *detail_font;
    const lv_font_t *icon_font;
    const lv_font_t *type_icon_font;
    const lv_font_t *delete_icon_font;
} ui_process_element_layout_t;

/* ── Step list element layout (was in element_step.c magic numbers) ── */
typedef struct {
    int item_w;
    int item_h;
    int item_x;
    int y_start;
    int swipe_offset;
    int delete_btn_w;
    int delete_btn_h;
    int delete_btn_x;
    int delete_btn_y;
    int delete_icon_x;
    int edit_btn_w;
    int edit_btn_h;
    int edit_btn_x;
    int edit_btn_y;
    int edit_icon_x;
    int summary_w;
    int summary_h;
    int summary_x;
    int summary_y;
    int type_icon_x;
    int type_icon_y;
    int name_w;
    int name_x;
    int name_y;
    int time_icon_x;
    int time_icon_y;
    int time_value_x;
    int time_value_y;
    int source_w;
    int source_x;
    int source_y;
    int discard_icon_x;
    int discard_icon_y;
    const lv_font_t *name_font;
    const lv_font_t *detail_font;
    const lv_font_t *icon_font;
    const lv_font_t *type_icon_font;
    const lv_font_t *delete_icon_font;
    const lv_font_t *edit_icon_font;
} ui_step_element_layout_t;

/* ── Button sizes (was in FilMachine.h #defines) ── */
typedef struct {
    int process_h;
    int process_w;
    int start_h;
    int start_w;
    int mbox_h;
    int mbox_w;
    int popup_close_h;
    int popup_close_w;
    int tune_h;
    int tune_w;
    int logo_h;
    int logo_w;
} ui_button_sizes_t;

/* ── Keyboard popup layout (was in accessories.c magic numbers) ── */
typedef struct {
    int textarea_y;
    int textarea_h;
    const lv_font_t *keyboard_font;
    const lv_font_t *textarea_font;
} ui_keyboard_layout_t;

/* ── Clean popup extras (drain container height was hardcoded) ── */

typedef struct {
    int message_w;
    int message_h;
    int message_title_line_w;
    int roller_w;
    int roller_h;
    int roller_title_line_w;
    int roller_inner_w;
    int roller_inner_h;
    int roller_wheel_h;
    int roller_wheel_y;
    int filter_w;
    int filter_h;
    int filter_title_line_w;
    int clean_w;
    int clean_h;
    int clean_title_line_w;
    int clean_settings_w;
    int clean_settings_h;
    int clean_process_w;
    int clean_process_h;
    int drain_w;
    int drain_h;
    int drain_title_line_w;
    int drain_confirm_w;
    int drain_confirm_h;
    int drain_process_w;
    int drain_process_h;
    int selfcheck_w;
    int selfcheck_h;
    int selfcheck_title_line_w;
    int ota_wifi_w;
    int ota_wifi_h;
    int ota_progress_w;
    int ota_progress_h;
    int ota_status_w;
    int ota_percent_w;
    int ota_progress_bar_w;
    int ota_progress_bar_h;
} ui_popup_layout_t;

typedef struct {
    ui_common_layout_t common;
    ui_home_layout_t home;
    ui_menu_layout_t menu;
    ui_processes_layout_t processes;
    ui_settings_layout_t settings;
    ui_process_detail_layout_t process_detail;
    ui_step_detail_layout_t step_detail;
    ui_checkup_layout_t checkup;
    ui_tools_layout_t tools;
    ui_roller_popup_layout_t roller_popup;
    ui_clean_popup_layout_t clean_popup;
    ui_drain_popup_layout_t drain_popup;
    ui_popup_layout_t popups;
    ui_filter_popup_layout_t filter_popup;
    ui_selfcheck_popup_layout_t selfcheck_popup;
    ui_ota_popup_layout_t ota_popup;
    ui_message_popup_layout_t message_popup;
    ui_process_element_layout_t process_element;
    ui_step_element_layout_t step_element;
    ui_button_sizes_t buttons;
    ui_keyboard_layout_t keyboard;
    int clean_drain_container_h;       /* was hardcoded 40 in cleanPopup */
    int clean_spinbox_btn_offset;      /* was hardcoded 5 in cleanPopup */
} ui_profile_t;

const ui_profile_t *ui_get_profile(void);

#ifdef __cplusplus
}
#endif

#endif
