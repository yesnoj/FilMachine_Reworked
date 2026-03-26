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
    int title_line_x;
    int title_line_w;
    int title_line_y;
} ui_common_layout_t;


typedef struct {
    int tab_w;
    int tab_h;
    int tab_gap;
    int tab_processes_y;
    int tab_settings_y;
    int tab_tools_y;
    int tab_icon_offset_x;
    int tab_icon_offset_y;
    int tab_label_offset_x;
    int tab_label_offset_y;
    const lv_font_t *tab_icon_font;
    const lv_font_t *tab_label_font;
} ui_menu_layout_t;

typedef struct {
    int add_btn_w;
    int add_btn_h;
    int add_btn_x;
    int add_btn_y;
    int title_label_x;
    int title_label_y;
    int add_icon_x;
    int add_icon_y;
    int filter_btn_size;
    int filter_btn_x;
    int list_x;
    int list_y;
    int list_w;
    int list_h;
    const lv_font_t *title_font;
    const lv_font_t *header_icon_font;
} ui_processes_layout_t;

typedef struct {
    int row_left_x;
    int row_gap_y;
    int row_h;
    int slider_h;
    int toggle_switch_w;
    int toggle_switch_h;
    int row_w;
    int textarea_w;
    int textarea_h;
    int row_label_x;
    int row_label_y;         /* y offset for row labels (LEFT_MID) */
    int switch_x;            /* x offset for switches (RIGHT_MID) */
    int switch_y;            /* y offset for switches (RIGHT_MID) */
    int slider_x_offset;     /* x offset for sliders in row (TOP_LEFT) */
    int section_label_x;
    int section_label_y;
    int section_line_x;
    int section_line_y;
    int scroll_x;
    int scroll_y;
    int scroll_w;
    int scroll_h;
    int tune_button_x;
    int tune_button_y;
    int slider_y;
    int slider_w;
    int row_value_x;
    int row_value_y;
    int textarea_x;
    int textarea_y;
    int section_padding;
    int help_icon_x;
    int help_icon_y;
    const lv_font_t *section_title_font;
    const lv_font_t *label_font;
    const lv_font_t *value_font;
    const lv_font_t *button_font;
    const lv_font_t *radio_font;
    int radio_size;
    const lv_font_t *textarea_font;
} ui_settings_layout_t;

typedef struct {
    int modal_w;
    int modal_h;
    int title_y;
    int title_line_w;
    int title_line_y;
    int close_w;
    int close_h;
    int close_x;
    int close_y;
    int name_x;
    int name_y;
    int name_w;
    int name_h;
    int name_textarea_x;
    int name_textarea_y;
    int name_textarea_w;
    int steps_label_x;
    int steps_label_y;
    int steps_x;
    int steps_y;
    int steps_w;
    int steps_h;
    int new_step_x;
    int new_step_y;
    int new_step_btn_w;
    int new_step_btn_h;
    int info_label_x;
    int info_label_y;
    int info_x;
    int info_y;
    int info_w;
    int info_h;
    int info_content_x;
    int temp_ctrl_y;
    int temp_ctrl_switch_x;
    int temp_switch_w;
    int temp_switch_h;
    int temp_row_y;
    int temp_row_h;
    int temp_label_x;
    int temp_textarea_x;
    int temp_textarea_w;
    int temp_unit_x;
    int tolerance_row_y;
    int total_time_y;
    int total_time_h;
    int info_padding;
    int color_container_x;
    int color_container_y;
    int color_container_w;
    int color_container_h;
    int color_label_x;
    int bw_icon_x;
    int total_label_x;
    int preferred_x;
    int preferred_y;
    int save_x;
    int save_y;
    int save_w;
    int save_h;
    int run_x;
    int run_y;
    int run_w;
    int run_h;
    int title_x;             /* x offset for title label (TOP_MID) */
    int title_line_x;        /* x offset for title underline (TOP_MID) */
    int form_label_y;        /* y offset for form labels/values (LEFT_MID/RIGHT_MID) */
    int container_x;         /* x offset for modal container (CENTER) */
    int container_y;         /* y offset for modal container (CENTER) */
    const lv_font_t *title_font;
    const lv_font_t *name_font;
    const lv_font_t *section_font;
    const lv_font_t *button_icon_font;
    const lv_font_t *close_icon_font;
    const lv_font_t *label_font;
    const lv_font_t *value_font;
    const lv_font_t *text_area_font;
    const lv_font_t *film_icon_font;
} ui_process_detail_layout_t;

typedef struct {
    int modal_w;
    int modal_h;
    int title_y;
    int title_line_w;
    int title_line_y;
    int form_row_x;
    int form_row_w;
    int form_row_h;
    int name_y;
    int duration_y;
    int type_y;
    int source_y;
    int discard_y;
    int name_textarea_x;
    int name_textarea_w;
    int minutes_textarea_x;
    int seconds_textarea_x;
    int time_textarea_w;
    int row_label_x;
    int row_label_y;        /* y offset for form row labels (LEFT_MID) */
    int dropdown_list_h;
    int source_row_extra_w;
    int type_dropdown_x;
    int type_dropdown_w;
    int type_icon_x;
    int source_dropdown_x;
    int source_dropdown_w;
    int source_temp_label_x;
    int source_temp_icon_x;
    int source_temp_value_x;
    int source_temp_value_y;
    int type_dropdown_y;
    int source_dropdown_y;
    int discard_label_x;
    int discard_switch_x;
    int discard_switch_w;
    int discard_switch_h;
    int save_x;
    int save_y;
    int save_w;
    int save_h;
    int cancel_x;
    int cancel_y;
    int cancel_w;
    int cancel_h;
    int title_x;             /* x offset for title label (TOP_MID) */
    int title_line_x;        /* x offset for title underline (TOP_MID) */
    int container_x;         /* x offset for modal container (CENTER) */
    int container_y;         /* y offset for modal container (CENTER) */
    const lv_font_t *title_font;
    const lv_font_t *label_font;
    const lv_font_t *value_font;
    const lv_font_t *button_font;
    const lv_font_t *info_icon_font;
} ui_step_detail_layout_t;

typedef struct {
    int stage_panel_w;
    int stage_panel_h;
    int stage_panel_x;
    int stage_panel_y;
    int stage_panel_inset;  /* negative inset for stage containers (e.g. -22) */
    int close_btn_w;
    int close_btn_h;
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
    int left_water_fill_row_y;
    int left_reach_temp_row_y;
    int left_tank_film_row_y;
    int left_status_icon_x;
    int left_status_label_x;
    int left_status_label_w;
    int processing_row_w;
    int processing_source_row_y;
    int processing_temp_ctrl_row_y;
    int processing_water_temp_row_y;
    int processing_next_step_row_y;
    int processing_row_value_x;
    int processing_next_step_w;
    int processing_stop_y;
    int processing_stop_left_x;
    int processing_stop_right_x;
    int stage_title_w;
    int stage_title_x;
    int stage_title_y;
    int tank_size_label_x;
    int tank_size_label_y;
    int chem_volume_label_x;
    int chem_volume_label_y;
    int stage_tank_size_textarea_x;
    int stage_tank_size_textarea_y;
    int stage_chem_volume_textarea_x;
    int stage_chem_volume_textarea_y;
    int stage_textarea_w;
    int stage_button_x;
    int stage_button_y;
    int fill_label_x;
    int fill_label_w;
    int fill_label_y;
    int reach_title_w;
    int reach_title_y;
    int target_temp_container_x;
    int target_temp_container_y;
    int target_title_w;
    int target_title_h;
    int target_title_x;
    int target_title_y;
    int target_title_value_x;
    int target_title_value_y;
    int target_tolerance_x;
    int target_tolerance_y;
    int continue_button_w;
    int target_temp_y;
    int target_temp_w;
    int target_temp_h;
    int target_temp_left_x;
    int target_temp_right_x;
    int target_temp_label_x;
    int target_temp_label_y;
    int target_temp_value_x;
    int target_temp_value_y;
    int heater_status_x;
    int heater_status_y;
    int film_box_w;
    int film_box_h;
    int film_tank_present_box_x;
    int film_tank_present_box_y;
    int film_rotating_box_x;
    int film_rotating_box_y;
    int film_label_x;
    int film_label_y;
    int film_value_x;
    int film_value_y;
    int processing_container_x;
    int processing_container_y;
    int processing_container_w;
    int processing_container_h;
    int processing_arc_size;
    int processing_arc_width;
    int processing_arc_center_x;
    int processing_arc_center_y;
    int processing_time_x;
    int processing_time_y;
    int processing_step_name_x;
    int processing_step_name_y;
    int processing_step_name_w;
    int processing_step_time_y;
    int processing_step_arc_size;
    int processing_step_arc_center_x;
    int processing_step_arc_center_y;
    int processing_step_kind_x;
    int processing_step_kind_y;
    int processing_pump_arc_size;
    int processing_complete_x;
    int processing_complete_y;
    int left_status_icon_y;
    int left_status_label_y;
    int processing_row_value_y;
    int action_btn_w;
    int action_btn_h;
    const lv_font_t *close_icon_font;
    const lv_font_t *process_name_font;
    const lv_font_t *left_title_font;
    const lv_font_t *left_body_font;
    const lv_font_t *left_value_font;
    const lv_font_t *action_btn_font;
    const lv_font_t *stage_title_font;
    const lv_font_t *stage_label_font;
    const lv_font_t *stage_value_font;
    const lv_font_t *heater_status_font;
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
    int item_row_x;
    int item_row_w;
    int action_row_h;
    int stat_row_h;
    int item_label_x;
    int item_label_y;       /* y offset for item labels/values (LEFT_MID/RIGHT_MID) */
    int item_btn_x;
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
    int help_icon_x;
    int help_icon_y;
    int credit_button_x;
    int action_btn_w;
    int action_btn_h;
    int credit_button_h;
    int credit_button_w;
    int section_line_x;      /* x offset for section lines (TOP_MID) */
    const lv_font_t *section_font;
    const lv_font_t *item_font;
    const lv_font_t *button_icon_font;
    const lv_font_t *credit_button_font;
} ui_tools_layout_t;


typedef struct {
    int title_x;
    int title_y;
    int title_line_x;
    int title_line_y;
    int wheel_container_x;
    int wheel_container_y;
    int wheel_x;
    int wheel_w;
    int wheel_visible_rows;
    int confirm_btn_x;
    int confirm_btn_y;
    const lv_font_t *title_font;
    const lv_font_t *confirm_btn_font;
    int confirm_btn_w;
    int confirm_btn_h;
    const lv_font_t *wheel_font;
    const lv_font_t *wheel_normal_font;
} ui_roller_popup_layout_t;

typedef struct {
    int title_x;
    int title_y;
    int title_line_x;
    int title_line_y;
    int settings_x;
    int settings_y;
    int subtitle_x;
    int subtitle_y;
    int checkbox_y;         /* y offset for checkboxes in LEFT_MID rows */
    int checkbox_label_y;   /* y offset for checkbox labels in LEFT_MID rows */
    int spinbox_container_x;
    int spinbox_y;
    int repeat_label_y;
    int drain_container_x;
    int drain_label_y;
    int drain_switch_y;
    int process_arc_x;
    int remaining_time_x;
    int now_cleaning_label_x;
    int now_cleaning_value_x;
    int now_step_x;
    int stop_button_x;
    int chem_container_x;
    int chem_container_y;
    int chem_container_w;
    int chem_container_h;
    int checkbox_w;
    int checkbox_h;
    int chem_c1_checkbox_x;
    int chem_c2_checkbox_x;
    int chem_c3_checkbox_x;
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
    int drain_switch_extra_x;
    int run_button_x;
    int run_button_y;
    int run_btn_w;
    int run_btn_h;
    int cancel_button_x;
    int cancel_button_y;
    int cancel_btn_w;
    int cancel_btn_h;
    int process_arc_size;
    int cycle_arc_size;
    int pump_arc_size;
    int progress_arc_width;
    int process_arc_y;
    int remaining_time_y;
    int now_cleaning_label_y;
    int now_cleaning_value_y;
    int now_step_y;
    int stop_button_y;
    int stop_btn_w;
    int stop_btn_h;
    const lv_font_t *title_font;
    const lv_font_t *subtitle_font;
    const lv_font_t *label_font;
    const lv_font_t *button_font;
    const lv_font_t *time_font;
    const lv_font_t *value_font;
    const lv_font_t *step_font;
} ui_clean_popup_layout_t;

typedef struct {
    int title_x;
    int title_y;
    int title_line_x;
    int title_line_y;
    int confirm_x;
    int confirm_y;
    int confirm_info_x;
    int confirm_info_w;
    int confirm_info_y;
    int progress_container_x;
    int drain_status_x;
    int waste_label_x;
    int remaining_time_x;
    int stop_btn_x;
    int start_cancel_btn_x;
    int start_cancel_btn_y;
    int action_btn_w;
    int action_btn_h;
    int progress_container_y;
    int progress_container_pad;
    int tank_bar_w;
    int tank_bar_h;
    int tank_bar_gap;
    int tank_bars_area_w;
    int tank_bar_y;
    int tank_label_y;
    int drain_status_w;
    int drain_status_y;
    int waste_label_y;
    int remaining_time_y;
    int stop_btn_y;
    int stop_btn_w;
    int stop_btn_h;
    int tank_bar_radius;
    const lv_font_t *title_font;
    const lv_font_t *info_font;
    const lv_font_t *bar_label_font;
    const lv_font_t *status_font;
    const lv_font_t *secondary_font;
    const lv_font_t *button_font;
} ui_drain_popup_layout_t;

typedef struct {
    int title_x;
    int title_y;
    int title_line_x;
    int title_line_y;
    int close_x;
    int close_y;
    int close_w;
    int close_h;
    int close_icon_size;
    int name_container_x;
    int name_container_y;
    int name_container_w;
    int name_container_h;
    int name_label_x;
    int name_label_y;
    int name_textarea_x;
    int name_textarea_y;
    int name_textarea_w;
    int filter_label_offset_y;
    int color_row_y;
    int bw_row_y;
    int preferred_row_y;
    int filter_options_container_x;
    int filter_options_container_w;
    int filter_options_container_h;
    int color_box_x;
    int color_box_y;
    int bw_box_x;
    int bw_box_y;
    int preferred_box_x;
    int filter_box_default_w;
    int color_box_w;
    int bw_box_w;
    int preferred_box_w;
    int filter_box_h;
    int filter_label_x;
    int color_radio_x;
    int bw_radio_x;
    int preferred_radio_x;
    int option_radio_size;
    int preferred_switch_x;
    int filter_label_offset_x;
    int apply_reset_btn_y;
    int apply_reset_btn_x;
    int action_btn_w;
    int action_btn_h;
    const lv_font_t *title_font;
    const lv_font_t *label_font;
    const lv_font_t *button_font;
    const lv_font_t *close_icon_font;
    const lv_font_t *textarea_font;
    const lv_font_t *radio_font;
} ui_filter_popup_layout_t;

typedef struct {
    int title_x;
    int title_y;
    int title_line_x;
    int title_line_y;
    int close_x;
    int close_y;
    int close_w;
    int close_h;
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
    int phase_title_x;
    int phase_title_w;
    int phase_title_y;
    int phase_desc_x;
    int phase_desc_w;
    int phase_desc_y;
    int phase_status_x;
    int phase_status_w;
    int phase_status_y;
    int phase_timer_x;
    int phase_timer_y;
    int progress_bar_x;
    int progress_bar_w;
    int progress_bar_h;
    int progress_bar_y;
    int control_btn_w;
    int control_btn_h;
    int control_btn_y;
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
    const lv_font_t *control_btn_font;
    const lv_font_t *advance_button_font;
    const lv_font_t *close_icon_font;
} ui_selfcheck_popup_layout_t;

typedef struct {
    int title_x;
    int title_y;
    int title_line_x;
    int title_line_y;
    int close_x;
    int close_y;
    int close_w;
    int close_h;
    int close_icon_size;
    int ip_label_x;
    int ip_label_w;
    int ip_label_y;
    int pin_label_x;
    int pin_label_w;
    int pin_label_y;
    int status_label_x;
    int status_label_w;
    int status_label_y;
    int progress_x;
    int progress_h;
    int progress_y;
    int progress_radius;
    int progress_status_x;
    int progress_status_y;
    int progress_bar_y;
    int percent_x;
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
    int title_x;
    int title_y;
    int title_line_x;
    int title_line_y;
    int text_container_x;
    int text_container_y;
    int text_container_w;
    int text_container_h;
    int text_w;
    int close_btn_x;
    int close_btn_y;
    int close_w;
    int close_h;
    int secondary_btn_x;
    int primary_btn_x;
    int action_btn_y;
    int action_btn_w;
    int action_btn_h;
    const lv_font_t *title_font;
    const lv_font_t *text_font;
    const lv_font_t *button_font;
    const lv_font_t *close_icon_font;
} ui_message_popup_layout_t;

/* ── Process list element layout (was in element_process.c magic numbers) ── */
typedef struct {
    int card_w;
    int card_h;
    int card_x;
    int list_start_y;
    int swipe_offset;
    int delete_btn_w;
    int delete_btn_h;
    int delete_btn_x;
    int delete_btn_y;
    int delete_icon_x;
    int card_content_w;
    int card_content_h;
    int card_content_x;
    int card_content_y;
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
    int type_icon_y;
    int preferred_icon_x;
    int preferred_icon_y;
    int delete_icon_y;
    int time_width_margin;      /* right-side margin subtracted from time label width */
    const lv_font_t *name_font;
    const lv_font_t *detail_font;
    const lv_font_t *detail_icon_font;
    const lv_font_t *type_icon_font;
    const lv_font_t *delete_icon_font;
} ui_process_element_layout_t;

/* ── Step list element layout (was in element_step.c magic numbers) ── */
typedef struct {
    int card_w;
    int card_h;
    int card_x;
    int list_start_y;
    int swipe_offset;
    int delete_btn_w;
    int delete_btn_h;
    int delete_btn_x;
    int delete_btn_y;
    int delete_icon_x;
    int delete_icon_y;
    int edit_btn_w;
    int edit_btn_h;
    int edit_btn_x;
    int edit_btn_y;
    int edit_icon_x;
    int edit_icon_y;
    int card_content_w;
    int card_content_h;
    int card_content_x;
    int card_content_y;
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
    const lv_font_t *detail_icon_font;
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
    int msgbox_btn_h;
    int msgbox_btn_w;
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
    int roller_content_w;
    int roller_content_h;
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
    int splash_w;
    int splash_h;
    int splash_title_line_w;
} ui_popup_layout_t;

typedef struct {
    int title_y;
    int title_line_y;
    int default_switch_y;
    int random_switch_y;
    int switch_row_w;       /* width of switch rows (Default / Random) */
    int options_x;
    int options_y;
    int options_h;
    int options_w;          /* width of options container */
    int row_h;
    int row_gap;
    int label_x;
    int label_y_offset;     /* vertical offset for labels inside a row */
    int roller_x;
    int roller_w;
    int roller_h;
    int roller_visible_rows;
    int slider_w;
    int slider_x;
    int slider_y_offset;    /* vertical offset for slider inside its row */
    int title_x;            /* x offset for title label */
    int title_line_x;       /* x offset for title underline */
    int switch_row_x;       /* x offset for Default / Random switch rows */
    int switch_x;           /* x offset for switch inside its row */
    int switch_y;           /* y offset for switch inside its row */
    int close_btn_x;        /* x offset for close button */
    int close_btn_w;
    int close_btn_h;
    int close_btn_y;
    int switch_label_y;      /* y offset for switch row labels (LEFT_MID) */
    const lv_font_t *title_font;
    const lv_font_t *label_font;
    const lv_font_t *button_font;
    const lv_font_t *value_font;
} ui_splash_popup_layout_t;

typedef struct {
    int subtitle_x;         /* x offset for subtitle relative to title */
    int subtitle_y;         /* y gap between title and subtitle */
    int version_x;          /* version label x offset (bottom-right) */
    int version_y;          /* version label y offset (bottom-right) */
    int play_icon_x;        /* play icon x offset (bottom-right) */
    int play_icon_y;        /* play icon y offset (bottom-right) */
    int play_hit_w;         /* play button hit area width */
    int play_hit_h;         /* play button hit area height */
    int play_hit_x;         /* play button hit area x offset (bottom-right) */
    int play_hit_y;         /* play button hit area y offset (bottom-right) */
} ui_splash_screen_layout_t;

typedef struct {
    ui_common_layout_t common;
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
    ui_splash_popup_layout_t splash_popup;
    ui_splash_screen_layout_t splash_screen;
    ui_process_element_layout_t process_element;
    ui_step_element_layout_t step_element;
    ui_button_sizes_t buttons;
    ui_keyboard_layout_t keyboard;
    int clean_drain_container_h;       /* was hardcoded 40 in cleanPopup */
    int clean_spinbox_btn_offset;      /* was hardcoded 5 in cleanPopup */
    int title_line_width;              /* common line width for title separators (was 2) */
    int element_border_width;          /* border width for process/step elements (was 4) */
    int element_shadow_width;          /* shadow width for step elements (was 5) */
    int element_shadow_spread;         /* shadow spread for step hover (was 3) */
    int selfcheck_progress_radius;     /* progress bar radius in selfcheck (was 4) */
} ui_profile_t;

const ui_profile_t *ui_get_profile(void);

#ifdef __cplusplus
}
#endif

#endif
