/**
 * @file element_process.c
 *
 */


//ESSENTIAL INCLUDES
#include "FilMachine.h"

extern struct gui_components gui;

//ACCESSORY INCLUDES

static uint16_t process_card_fill_time_seconds(void) {
    uint8_t volume_index = gui.page.settings.settingsParams.chemistryVolume;
    uint8_t tank_size = gui.page.settings.settingsParams.tankSize;
    uint16_t table[2][3][2] = tanksSizesAndTimes;

    if(volume_index < 1 || volume_index > 2) volume_index = 2;
    if(tank_size < 1 || tank_size > 3) tank_size = 2;

    return table[volume_index - 1][tank_size - 1][1];
}

static uint32_t process_card_overlap_credit_seconds(uint16_t fill_time_seconds) {
    uint32_t overlap_pct = gui.page.settings.settingsParams.drainFillOverlapSetpoint;
    if (overlap_pct > 100U) overlap_pct = 100U;
    return (((uint32_t)fill_time_seconds * 2U) * overlap_pct) / 100U;
}

static uint32_t process_card_adjusted_processing_seconds(const stepNode *step, uint16_t fill_time_seconds) {
    if(step == NULL || step->step.stepDetails == NULL) return 0U;

    uint32_t recipe_seconds = ((uint32_t)step->step.stepDetails->data.timeMins * 60U) +
                              (uint32_t)step->step.stepDetails->data.timeSecs;
    uint32_t overlap_credit = process_card_overlap_credit_seconds(fill_time_seconds);

    return (recipe_seconds > overlap_credit) ? (recipe_seconds - overlap_credit) : 0U;
}

static uint32_t process_card_estimated_runtime_seconds(const processNode *process) {
    if(process == NULL || process->process.processDetails == NULL) return 0U;

    uint16_t fill_time_seconds = process_card_fill_time_seconds();
    uint32_t total_seconds = 0U;

    for(stepNode *step = process->process.processDetails->stepElementsList.start; step != NULL; step = step->next) {
        total_seconds += ((uint32_t)fill_time_seconds * 2U);
        total_seconds += process_card_adjusted_processing_seconds(step, fill_time_seconds);
    }

    return total_seconds;
}

static void process_card_set_time_label(processNode *process) {
    if(process == NULL || process->process.processTime == NULL || process->process.processDetails == NULL) return;

    uint32_t estimated_seconds = process_card_estimated_runtime_seconds(process);
    lv_label_set_text_fmt(process->process.processTime, "%" PRIu32 "m%" PRIu8 "s / ~%" PRIu32 "m%" PRIu32 "s",
        process->process.processDetails->data.timeMins,
        process->process.processDetails->data.timeSecs,
        estimated_seconds / 60U,
        estimated_seconds % 60U);
}


/******************************
*  LINKED LIST IMPLEMENTATION
******************************/

processNode *addProcessElement(processNode	*processToAdd) {
	if( gui.page.processes.processElementsList.size == MAX_PROC_ELEMENTS ) {
    messagePopupCreate(warningPopupTitle_text, maxNumberEntryProcessPopupBody_text, NULL, NULL, NULL);
    return NULL;
    }		// Put some limit on things!

  LV_LOG_USER("Processes available %"PRIi32" first",gui.page.processes.processElementsList.size);
      if( gui.page.processes.processElementsList.start == NULL) {					/* Deals with the first entry */
        gui.page.processes.processElementsList.start = processToAdd;
        processToAdd->prev = NULL;
      } else {
        gui.page.processes.processElementsList.end->next = processToAdd;				/* Do this after the first */
        processToAdd->prev = gui.page.processes.processElementsList.end;
      }
      gui.page.processes.processElementsList.end = processToAdd;
      gui.page.processes.processElementsList.end->next = NULL;
      gui.page.processes.processElementsList.size++;

      LV_LOG_USER("Processes available %"PRIi32" after",gui.page.processes.processElementsList.size -1); 
      return processToAdd;
}



bool deleteProcessElement( processNode	*processToDelete ) {

	processNode 	*adjust_y_ptr = NULL;
	lv_coord_t		container_y_prev, container_y_new ;

	if( processToDelete ) {
		adjust_y_ptr = processToDelete->next;
		container_y_prev = processToDelete->process.container_y;
		if( processToDelete == gui.page.processes.processElementsList.start ) {
			if( processToDelete->next ) {
				gui.page.processes.processElementsList.start = processToDelete->next;
			} else gui.page.processes.processElementsList.start = gui.page.processes.processElementsList.end = NULL;

		} else if( processToDelete == gui.page.processes.processElementsList.end ) {

			if( processToDelete->prev ) {		// Check the end is not the beginning!
				processToDelete->prev->next = NULL;
				gui.page.processes.processElementsList.end = processToDelete->prev;
			}

		} else if( processToDelete->prev ) {
			processToDelete->prev->next = processToDelete->next;	// Re-join the linked list if not at beginning
			processToDelete->next->prev = processToDelete->prev;
		}

		while( adjust_y_ptr ) {
			if( adjust_y_ptr->next ) container_y_new = adjust_y_ptr->process.container_y;
			adjust_y_ptr->process.container_y = container_y_prev;
			lv_obj_set_y(adjust_y_ptr->process.processElement, adjust_y_ptr->process.container_y);
			if( adjust_y_ptr->next ) container_y_prev = container_y_new;
			adjust_y_ptr = adjust_y_ptr->next;
		}
    /* Clear global pointer BEFORE freeing */
    if (gui.tempProcessNode == processToDelete) {
        gui.tempProcessNode = NULL;
    }

    if(processToDelete->process.processDetails->stepElementsList.size > 0) {
        while(processToDelete->process.processDetails->stepElementsList.start != NULL) {
            deleteStepElement( processToDelete->process.processDetails->stepElementsList.start, processToDelete, true);
        }
    }

    lv_obj_delete( processToDelete->process.processElement );
    process_node_destroy( processToDelete );                                      // Free the list entry itself
		gui.page.processes.processElementsList.size--;

    LV_LOG_USER("Processes available %"PRIi32"",gui.page.processes.processElementsList.size); 
		return true;
	}
	return false;
}

processNode* getProcElementEntryByObject(lv_obj_t* obj) {

    processNode* currentNode = gui.page.processes.processElementsList.start;

    while( currentNode !=  NULL ) {
        // Check all objects if any match, return element pointer, not styles!
        if( obj == currentNode->process.processElement || 
            obj == currentNode->process.preferredIcon || 
            obj == currentNode->process.processElementSummary || 
            obj == currentNode->process.processName || 
            obj == currentNode->process.processTemp || 
            obj == currentNode->process.processTempIcon || 
            obj == currentNode->process.processTime || 
            obj == currentNode->process.processTimeIcon || 
            obj == currentNode->process.processTypeIcon ||
            obj == currentNode->process.deleteButton ||
            obj == currentNode->process.deleteButtonLabel ||
            obj == (lv_obj_t*)currentNode) {
              break;
        }
        currentNode = currentNode->next;
    }
    return currentNode;   // Will Return NULL if no matching processNode is found
}



/******************************
*  LVGL ELEMENTS IMPLEMENTATION
******************************/

void event_processElement(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
//    lv_obj_t *cont = (lv_obj_t *)lv_event_get_current_target(e);
    lv_obj_t *data = (lv_obj_t *)lv_event_get_user_data(e);
    processNode *currentNode = getProcElementEntryByObject(data);
    /* Fix #21: Guard against NULL indev — lv_obj_send_event() bypasses indev
     * processing, so lv_indev_active() returns NULL in that context. */
    lv_indev_t *active_indev = lv_indev_active();
    lv_dir_t dir = active_indev ? lv_indev_get_gesture_dir(active_indev) : LV_DIR_NONE;

    int8_t x;

    static bool ignore_click = false;  // Flag to ignore click

    if (currentNode == NULL) {
        LV_LOG_USER("Bad object passed to eventProcessElement!");
        return;
    }

    if (code == LV_EVENT_RELEASED) {
        LV_LOG_USER("LV_EVENT_RELEASED");
        if (currentNode->process.gestureHandled == true && currentNode->process.swipedLeft == true) {
            currentNode->process.gestureHandled = false;
            return;
        }
        ignore_click = false;  // Reset ignore click flag on release
    }

    if (code == LV_EVENT_GESTURE && currentNode->process.longPressHandled == false) {
        currentNode->process.gestureHandled = true;

        switch (dir) {
            case LV_DIR_LEFT:
                if (currentNode->process.swipedLeft == false && currentNode->process.swipedRight == true) {
                    LV_LOG_USER("Left gesture to return");
                    x = lv_obj_get_x_aligned(obj) - ui_get_profile()->process_element.swipe_offset;
                    lv_obj_set_pos(obj, x, lv_obj_get_y_aligned(obj));
                    currentNode->process.swipedRight = false;
                    currentNode->process.swipedLeft = true;
                    lv_obj_add_flag(currentNode->process.deleteButton, LV_OBJ_FLAG_HIDDEN);
                    ignore_click = true;  // Set ignore click flag
                }
                break;

            case LV_DIR_RIGHT:
                if (currentNode->process.swipedLeft == true && currentNode->process.swipedRight == false) {
                    LV_LOG_USER("Right gesture for delete");
                    x = lv_obj_get_x_aligned(obj) + ui_get_profile()->process_element.swipe_offset;
                    lv_obj_set_pos(obj, x, lv_obj_get_y_aligned(obj));
                    currentNode->process.swipedRight = true;
                    currentNode->process.swipedLeft = false;
                    lv_obj_remove_flag(currentNode->process.deleteButton, LV_OBJ_FLAG_HIDDEN);
                    ignore_click = true;  // Set ignore click flag
                }
                break;
                
            default:
            	break;
        }
    }

    if (code == LV_EVENT_SHORT_CLICKED && currentNode->process.longPressHandled == false && !ignore_click) {
        LV_LOG_USER("LV_EVENT_CLICKED");

        if (obj == currentNode->process.preferredIcon) {
            if (lv_color_eq(lv_obj_get_style_text_color(currentNode->process.preferredIcon, LV_PART_MAIN), lv_color_hex(RED))) {
                lv_obj_set_style_text_color(currentNode->process.preferredIcon, lv_color_hex(WHITE), LV_PART_MAIN);
                currentNode->process.processDetails->data.isPreferred = false;
            } else {
                lv_obj_set_style_text_color(currentNode->process.preferredIcon, lv_color_hex(RED), LV_PART_MAIN);
                currentNode->process.processDetails->data.isPreferred = true;
            }
            LV_LOG_USER("Process is preferred: %d", currentNode->process.processDetails->data.isPreferred);
            if (gui.page.processes.isFiltered == true)
                filterAndDisplayProcesses();
            qSysAction(SAVE_PROCESS_CONFIG);
        }

        if (obj == currentNode->process.deleteButton) {
            if (gui.element.messagePopup.mBoxPopupParent == NULL) {
                LV_LOG_USER("Process Element click for popup delete");
                gui.tempProcessNode = currentNode;
                messagePopupCreate(deletePopupTitle_text, deletePopupBody_text, deleteButton_text, stepDetailCancel_text, currentNode);
            }
        }

        if (obj == currentNode->process.processElementSummary && currentNode->process.swipedLeft == true) {
            LV_LOG_USER("Process Element Details address %p", currentNode);
            gui.tempProcessNode = currentNode;
            processDetail(currentNode->process.processDetails->processesContainer); // currentNode
        }
    }

    if (code == LV_EVENT_LONG_PRESSED && currentNode->process.swipedLeft == true) {
        LV_LOG_USER("LV_EVENT_LONG_PRESSED");
        currentNode->process.longPressHandled = true;
        gui.tempProcessNode = currentNode;
        LV_LOG_USER("Duplicate process element %p", currentNode);
        messagePopupCreate(duplicatePopupTitle_text, duplicateProcessPopupBody_text, checkupNo_text, checkupYes_text, currentNode);
    }

    if(code == LV_EVENT_REFRESH){
      if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
			lv_label_set_text_fmt(currentNode->process.processTemp, "%"PRIi32"°C", currentNode->process.processDetails->data.temp); 
      } else{
			lv_label_set_text_fmt(currentNode->process.processTemp, "%"PRIi32"°F", convertCelsiusToFahrenheit(currentNode->process.processDetails->data.temp)); 
      }
      process_card_set_time_label(currentNode);
    }

    if (code == LV_EVENT_DELETE) {
        lv_style_reset(&currentNode->process.processStyle);
    }
}


void processElementCreate(processNode *newProcess, int32_t tempSize) {
  const ui_process_element_layout_t *pe = &ui_get_profile()->process_element;
  gui.tempProcessNode = newProcess;

	if(newProcess->process.processStyle.values_and_props == NULL ) {		/* Only initialise the style once! */
		lv_style_init(&newProcess->process.processStyle);

		lv_style_set_bg_color(&newProcess->process.processStyle, lv_color_hex(GREY));
		lv_style_set_border_color(&newProcess->process.processStyle, lv_color_hex(GREEN_DARK));
		lv_style_set_border_width(&newProcess->process.processStyle, ui_get_profile()->element_border_width);
		lv_style_set_border_opa(&newProcess->process.processStyle, LV_OPA_50);
		lv_style_set_border_side(&newProcess->process.processStyle, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT);
		LV_LOG_USER("First call to processElementCreate style now initialised");
	}
  int32_t positionIndex;
  
  if(tempSize == -1)
    positionIndex = gui.page.processes.processElementsList.size; // New entry add to the end of the list
  else 
    positionIndex = tempSize; // Use the index position passed into the function
        
  LV_LOG_USER("Process size :%"PRIi32"",gui.page.processes.processElementsList.size);
 
  newProcess->process.swipedLeft = true;
  newProcess->process.swipedRight = false;
  newProcess->process.gestureHandled = false;

	newProcess->process.processElement = lv_obj_create(gui.page.processes.processesListContainer);
	newProcess->process.container_y = pe->list_start_y + ((positionIndex - 1) * pe->card_h);
	lv_obj_set_pos(newProcess->process.processElement, pe->card_x, newProcess->process.container_y);
  lv_obj_set_size(newProcess->process.processElement, pe->card_w, pe->card_h);
	lv_obj_remove_flag(newProcess->process.processElement, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_border_opa(newProcess->process.processElement, LV_OPA_TRANSP, 0);
  lv_obj_add_event_cb(newProcess->process.processElement, event_processElement, LV_EVENT_GESTURE, newProcess);
  lv_obj_add_event_cb(newProcess->process.processElement, event_processElement, LV_EVENT_REFRESH, newProcess);
  lv_obj_add_event_cb(newProcess->process.processElement, event_processElement, LV_EVENT_RELEASED, newProcess);
  lv_obj_add_event_cb(newProcess->process.processElement, event_processElement, LV_EVENT_SHORT_CLICKED, newProcess);
  lv_obj_add_event_cb(newProcess->process.processElement, event_processElement, LV_EVENT_LONG_PRESSED, newProcess);

  lv_obj_remove_flag(newProcess->process.processElement, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_set_pos(newProcess->process.processElement,lv_obj_get_x_aligned(newProcess->process.processElement) - pe->swipe_offset,lv_obj_get_y_aligned(newProcess->process.processElement));


  /*********************
	*    PAGE ELEMENTS			
	*********************/
        newProcess->process.deleteButton = lv_obj_create(newProcess->process.processElement);
        lv_obj_set_style_bg_color(newProcess->process.deleteButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
        lv_obj_set_size(newProcess->process.deleteButton, pe->delete_btn_w, pe->delete_btn_h);
        lv_obj_align(newProcess->process.deleteButton, LV_ALIGN_TOP_LEFT, pe->delete_btn_x, pe->delete_btn_y);
        lv_obj_add_flag(newProcess->process.deleteButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(newProcess->process.deleteButton, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(newProcess->process.deleteButton, event_processElement, LV_EVENT_SHORT_CLICKED, newProcess);

                newProcess->process.deleteButtonLabel = lv_label_create(newProcess->process.deleteButton);
                lv_label_set_text(newProcess->process.deleteButtonLabel, trash_icon);
                lv_obj_set_style_text_font(newProcess->process.deleteButtonLabel, pe->delete_icon_font, 0);
                lv_obj_align(newProcess->process.deleteButtonLabel, LV_ALIGN_CENTER, pe->delete_icon_x, 0);


        newProcess->process.processElementSummary = lv_obj_create(newProcess->process.processElement);
        //lv_obj_set_style_border_color(proc_ptr->process.processElementSummary, lv_color_hex(LV_PALETTE_ORANGE), 0);
        lv_obj_set_size(newProcess->process.processElementSummary, pe->card_content_w, pe->card_content_h);
        lv_obj_align(newProcess->process.processElementSummary, LV_ALIGN_TOP_LEFT, pe->card_content_x, pe->card_content_y);
        lv_obj_remove_flag(newProcess->process.processElementSummary, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(newProcess->process.processElementSummary, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(newProcess->process.processElementSummary, &newProcess->process.processStyle, 0);

        newProcess->process.processName = lv_label_create(newProcess->process.processElementSummary);
        lv_label_set_text(newProcess->process.processName, newProcess->process.processDetails->data.processNameString);
        lv_obj_set_style_text_font(newProcess->process.processName, pe->name_font, 0);
        lv_label_set_long_mode(newProcess->process.processName, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(newProcess->process.processName, pe->name_w);
        lv_obj_align(newProcess->process.processName, LV_ALIGN_LEFT_MID, pe->name_x, pe->name_y);
        lv_obj_remove_flag(newProcess->process.processName, LV_OBJ_FLAG_SCROLLABLE);

        newProcess->process.processTempIcon = lv_label_create(newProcess->process.processElementSummary);
        lv_label_set_text(newProcess->process.processTempIcon, temp_icon);
        lv_obj_set_style_text_font(newProcess->process.processTempIcon, pe->detail_icon_font, 0);
        //lv_obj_set_style_text_color(newProcess->process.tempIcon, lv_color_hex(GREY), LV_PART_MAIN);
        lv_obj_align(newProcess->process.processTempIcon, LV_ALIGN_LEFT_MID, pe->temp_icon_x, pe->temp_icon_y);

        newProcess->process.processTemp = lv_label_create(newProcess->process.processElementSummary);

        if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
           lv_label_set_text_fmt(newProcess->process.processTemp, "%"PRIi32"°C", newProcess->process.processDetails->data.temp); 
        } else{
            lv_label_set_text_fmt(newProcess->process.processTemp, "%"PRIi32"°F", convertCelsiusToFahrenheit(newProcess->process.processDetails->data.temp)); 
        }
        
        lv_obj_set_style_text_font(newProcess->process.processTemp, pe->detail_font, 0);
        lv_obj_align(newProcess->process.processTemp, LV_ALIGN_LEFT_MID, pe->temp_value_x, pe->temp_value_y);

        newProcess->process.processTimeIcon = lv_label_create(newProcess->process.processElementSummary);
        lv_label_set_text(newProcess->process.processTimeIcon, clock_icon);
        lv_obj_set_style_text_font(newProcess->process.processTimeIcon, pe->detail_icon_font, 0);
        //lv_obj_set_style_text_color(newStep->step.stepTimeIcon, lv_color_hex(GREY), LV_PART_MAIN);
        lv_obj_align(newProcess->process.processTimeIcon, LV_ALIGN_LEFT_MID, pe->time_icon_x, pe->time_icon_y);

        newProcess->process.processTime = lv_label_create(newProcess->process.processElementSummary);    
        lv_obj_set_style_text_font(newProcess->process.processTime, pe->detail_font, 0);
        lv_obj_set_width(newProcess->process.processTime, pe->card_content_w - pe->time_value_x - 18);
        lv_label_set_long_mode(newProcess->process.processTime, LV_LABEL_LONG_CLIP);
        lv_obj_align(newProcess->process.processTime, LV_ALIGN_LEFT_MID, pe->time_value_x, pe->time_value_y);
        process_card_set_time_label(newProcess);

        newProcess->process.processTypeIcon = lv_label_create(newProcess->process.processElementSummary);
        lv_label_set_text(newProcess->process.processTypeIcon, newProcess->process.processDetails->data.filmType == BLACK_AND_WHITE_FILM ? blackwhite_icon : colorpalette_icon);
        newProcess->process.processDetails->data.filmType = newProcess->process.processDetails->data.filmType;
        lv_obj_set_style_text_font(newProcess->process.processTypeIcon, pe->type_icon_font, 0);
        lv_obj_align(newProcess->process.processTypeIcon, LV_ALIGN_RIGHT_MID, pe->type_icon_x, 0);
        

        if(newProcess->process.processDetails->data.isTempControlled == false)
          {
            lv_obj_add_flag(newProcess->process.processTempIcon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(newProcess->process.processTemp, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(newProcess->process.processTimeIcon, LV_ALIGN_LEFT_MID, pe->time_icon_no_temp_x, pe->time_icon_y);
            lv_obj_align(newProcess->process.processTime, LV_ALIGN_LEFT_MID, pe->time_value_no_temp_x, pe->time_value_y);
            lv_obj_set_width(newProcess->process.processTime, pe->card_content_w - pe->time_value_no_temp_x - 18);
          }


        newProcess->process.preferredIcon = lv_label_create(newProcess->process.processElement);
        lv_label_set_text(newProcess->process.preferredIcon, preferred_icon);
        lv_obj_add_flag(newProcess->process.preferredIcon, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_font(newProcess->process.preferredIcon, pe->type_icon_font, 0);
        lv_obj_set_style_text_color(newProcess->process.preferredIcon, lv_color_hex(WHITE), LV_PART_MAIN);
        lv_obj_align(newProcess->process.preferredIcon, LV_ALIGN_RIGHT_MID, pe->preferred_icon_x, 0);
        
        if(newProcess->process.processDetails->data.isPreferred == true){
            lv_obj_set_style_text_color(newProcess->process.preferredIcon, lv_color_hex(RED), LV_PART_MAIN);
        }
        else{
            lv_obj_set_style_text_color(newProcess->process.preferredIcon, lv_color_hex(WHITE), LV_PART_MAIN);
        }
        
        lv_obj_add_event_cb(newProcess->process.preferredIcon, event_processElement, LV_EVENT_SHORT_CLICKED, newProcess);
}

