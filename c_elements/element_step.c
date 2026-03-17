/**
 * @file element_step.c
 *
 */


//ESSENTIAL INCLUDES
#include "FilMachine.h"
#include <string.h>

extern struct gui_components gui;

/* Drag state shared between handleLongPress, handleDrag, and drop */
static lv_point_t drag_last_point;
static bool drag_active = false;
static int drag_slot = -1;
static int drag_original_slot = -1;

//ACCESSORY INCLUDES


/******************************
*  LINKED LIST IMPLEMENTATION
******************************/
stepNode *addStepElement(stepNode * stepToAdd, processNode * processReference) {
	
  if(processReference->process.processDetails->stepElementsList.size == MAX_STEP_ELEMENTS){
      messagePopupCreate(warningPopupTitle_text, maxNumberEntryStepsPopupBody_text, NULL, NULL, NULL);
      return NULL;
  }
	if(isNodeInList((void*)&(processReference->process.processDetails->stepElementsList), stepToAdd, STEP_NODE) != NULL) 
      return NULL;		// Put some limit on things!
	
  if(processReference->process.processDetails->stepElementsList.start == NULL) {					/* Deals with the first entry */
		processReference->process.processDetails->stepElementsList.start = stepToAdd;
		stepToAdd->prev = NULL;
	} else {
		processReference->process.processDetails->stepElementsList.end->next = stepToAdd;				/* Do this after the first */
		stepToAdd->prev = processReference->process.processDetails->stepElementsList.end;
	}
	processReference->process.processDetails->stepElementsList.end = stepToAdd;
	processReference->process.processDetails->stepElementsList.end->next = NULL;
	processReference->process.processDetails->stepElementsList.size++;
  
  LV_LOG_USER("stepElementsList.size: %d", processReference->process.processDetails->stepElementsList.size);

  processReference->process.processDetails->data.somethingChanged = true;
  lv_obj_send_event(processReference->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL);

  LV_LOG_USER("Process address 0x%p, with n:%d steps",processReference, processReference->process.processDetails->stepElementsList.size); 
	return stepToAdd;
}



bool deleteStepElement( stepNode	*stepToDelete, processNode * processReference , bool isDeleteProcess) {

	stepNode 	*adjust_y_ptr = NULL;
	lv_coord_t		container_y_prev, container_y_new ;

	if( stepToDelete ) {
    if(stepToDelete->step.stepElement != NULL ) {
		  adjust_y_ptr = stepToDelete->next;
		  container_y_prev = stepToDelete->step.container_y;
    }
		if( stepToDelete == processReference->process.processDetails->stepElementsList.start ) {
			if( stepToDelete->next ) {
				processReference->process.processDetails->stepElementsList.start = stepToDelete->next;
			} else processReference->process.processDetails->stepElementsList.start = processReference->process.processDetails->stepElementsList.end = NULL;

		} else if( stepToDelete == processReference->process.processDetails->stepElementsList.end ) {

			if( stepToDelete->prev ) {		// Check the end is not the beginning!
				stepToDelete->prev->next = NULL;
				processReference->process.processDetails->stepElementsList.end = stepToDelete->prev;
			}

		} else if( stepToDelete->prev ) {
			stepToDelete->prev->next = stepToDelete->next;	// Re-join the linked list if not at beginning
			stepToDelete->next->prev = stepToDelete->prev;
		}
    if(!isDeleteProcess){
      while( adjust_y_ptr) {
        if( adjust_y_ptr->next ) container_y_new = adjust_y_ptr->step.container_y;
        adjust_y_ptr->step.container_y = container_y_prev;
        lv_obj_set_y(adjust_y_ptr->step.stepElement, adjust_y_ptr->step.container_y);
        if( adjust_y_ptr->next ) container_y_prev = container_y_new;
        adjust_y_ptr = adjust_y_ptr->next;
      }
      /* Only delete all LVGL objects associated with entry if called from process detail screen */
      if(stepToDelete->step.stepElement) lv_obj_delete( stepToDelete->step.stepElement );
      /* Free the allocated memory for the list entry*/
      step_node_destroy( stepToDelete );
      processReference->process.processDetails->stepElementsList.size--;  // Update list size
      lv_obj_send_event(processReference->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL); // Refresh Screen and states

    LV_LOG_USER("Process address %p, with n:%d steps",processReference, processReference->process.processDetails->stepElementsList.size); 
    }
		return true;
	}
	return false;
}

stepNode *getStepElementEntryByObject(lv_obj_t *obj, processNode *processReference) {
  
	stepNode	*currentNode  = processReference->process.processDetails->stepElementsList.start;

	while( currentNode != NULL ) {
		if( obj == currentNode->step.stepElement ||				// Check all objects if any match return element pointer, not styles! 
        obj == currentNode->step.stepElementSummary ||
        obj == currentNode->step.stepName ||
        obj == currentNode->step.stepTime ||
        obj == currentNode->step.stepTimeIcon ||
        obj == currentNode->step.stepTypeIcon ||
        obj == currentNode->step.discardAfterIcon ||
        obj == currentNode->step.sourceLabel ||
        obj == currentNode->step.deleteButton ||
        obj == currentNode->step.deleteButtonLabel ||
        obj == currentNode->step.editButton ||   
        obj == currentNode->step.editButtonLabel ||
        obj == (lv_obj_t*)currentNode ) {
           break;
    }
		currentNode = currentNode->next;
	}
  return currentNode;   // Will Return NULL if no matching stepNode is found
}



/******************************
*  LVGL ELEMENTS IMPLEMENTATION
******************************/

void removeStepElementFromList(processNode *data, stepNode *node) {

    /* Remove element from the list */
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        data->process.processDetails->stepElementsList.start = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        data->process.processDetails->stepElementsList.end = node->prev;
    }

    /* Update list size */
    data->process.processDetails->stepElementsList.size--;

    /* Clean up node pointers */
    node->prev = NULL;
    node->next = NULL;
}

void insertStepElementAfter(processNode *data, stepNode *afterNode, stepNode *node) {

    /* Insert element after afterNode in the list */
    if (afterNode == NULL) {
        /* Insert at the beginning of the list */
        node->next = data->process.processDetails->stepElementsList.start;
        if (data->process.processDetails->stepElementsList.start != NULL) {
            data->process.processDetails->stepElementsList.start->prev = node;
        }
        data->process.processDetails->stepElementsList.start = node;
    } else {
        node->next = afterNode->next;
        node->prev = afterNode;
        afterNode->next = node;
        if (node->next != NULL) {
            node->next->prev = node;
        }
    }

    /* Update the end of the list if needed */
    if (afterNode == data->process.processDetails->stepElementsList.end) {
        data->process.processDetails->stepElementsList.end = node;
    }

    /* Update list size */
    data->process.processDetails->stepElementsList.size++;
}

void reorderStepElements(processNode *data) {
	
    int y_offset = -13;
    uint32_t child_idx = 0;
    stepNode *current = data->process.processDetails->stepElementsList.start;
    while (current) {
        /* Reset X to default position (-63) and set correct Y */
        lv_obj_set_pos(current->step.stepElement, -63, y_offset);
        current->step.container_y = y_offset;
        /* Restore LVGL child index to match linked list order.
           This fixes Z-order without triggering spurious events
           that lv_obj_move_foreground can cause. */
        lv_obj_move_to_index(current->step.stepElement, child_idx);
        /* Reset swipe/gesture state to prevent stale flags */
        current->step.swipedLeft = false;
        current->step.swipedRight = false;
        current->step.gestureHandled = false;
        lv_obj_add_flag(current->step.deleteButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(current->step.editButton, LV_OBJ_FLAG_HIDDEN);
        y_offset += lv_obj_get_height(current->step.stepElement);
        child_idx++;
        current = current->next;
    }
    /* Force the container to recalculate its scrollable content area
       (needed after dynamically adding/removing children) */
    lv_obj_update_layout(data->process.processDetails->processStepsContainer);
    /* Force full redraw of the container */
    lv_obj_invalidate(data->process.processDetails->processStepsContainer);
}

bool hasListChanged(processNode *data) {

    stepNode *current = data->process.processDetails->stepElementsList.start;
    lv_coord_t original_y = -13; /* Must match reorderStepElements starting y_offset */
    lv_coord_t current_y = 0;  /* Current Y position */

    while (current) {
        /* Check if Y position changed from expected original position */
        current_y = lv_obj_get_y_aligned(current->step.stepElement);
        if (current_y != original_y) {
            /* If there is a difference, the list has changed */
            return true;
        }

        /* Update expected original Y position for next element */
        original_y += lv_obj_get_height(current->step.stepElement);

        /* Move to next element in the list */
        current = current->next;
    }

    /* If we reach here, the list has not changed */
    return false;
}

static void stepElement_handleGesture(stepNode *currentNode, lv_obj_t *obj, lv_dir_t dir, processNode *data) {
    int8_t x;

    if (currentNode->step.swipedRight == true) {
        return;
    }
    currentNode->step.gestureHandled = true;
    switch (dir) {
        case LV_DIR_LEFT:
            if (currentNode->step.swipedLeft == false && currentNode->step.swipedRight == false) {
                LV_LOG_USER("Left gesture for duplicate popup");
                if (data->process.processDetails->stepElementsList.size >= MAX_STEP_ELEMENTS) {
                    messagePopupCreate(warningPopupTitle_text, maxNumberEntryStepsPopupBody_text, NULL, NULL, NULL);
                    break;
                }
                currentNode->step.swipedLeft = true;
                currentNode->step.swipedRight = false;
                gui.tempStepNode = currentNode;
                messagePopupCreate(duplicateStepPopupTitle_text, duplicateStepPopupBody_text, checkupNo_text, checkupYes_text, gui.tempStepNode);
            }
            break;

        case LV_DIR_RIGHT:
            if (currentNode->step.swipedLeft == false && currentNode->step.swipedRight == false) {
                LV_LOG_USER("Right gesture for delete");
                x = lv_obj_get_x_aligned(currentNode->step.stepElement) + 50;
                lv_obj_set_pos(currentNode->step.stepElement, x, lv_obj_get_y_aligned(currentNode->step.stepElement));
                currentNode->step.swipedRight = true;
                currentNode->step.swipedLeft = false;
                lv_obj_remove_flag(currentNode->step.deleteButton, LV_OBJ_FLAG_HIDDEN);
            }
            break;

        default:
            break;
    }
}

static void stepElement_handleClick(stepNode *currentNode, lv_obj_t *obj, processNode *data, lv_indev_t *indev, lv_point_t press_point) {
    int8_t x;

    bool is_action_button = (obj == currentNode->step.deleteButton ||
                             obj == currentNode->step.deleteButtonLabel ||
                             obj == currentNode->step.editButton ||
                             obj == currentNode->step.editButtonLabel);
    if (!is_action_button) {
        lv_point_t release_point;
        lv_indev_get_point(indev, &release_point);
        int32_t dx = release_point.x - press_point.x;
        int32_t dy = release_point.y - press_point.y;
        if ((dx * dx + dy * dy) > (10 * 10)) {
            LV_LOG_USER("SHORT_CLICKED ignored: cursor moved %"PRId32",%"PRId32" from press", dx, dy);
            return;
        }
    }

    if (currentNode->step.swipedRight == true && obj != currentNode->step.deleteButton
        && obj != currentNode->step.deleteButtonLabel
        && obj != currentNode->step.editButton
        && obj != currentNode->step.editButtonLabel) {
        LV_LOG_USER("Tap to close panel");
        x = lv_obj_get_x_aligned(currentNode->step.stepElement) - 50;
        lv_obj_set_pos(currentNode->step.stepElement, x, lv_obj_get_y_aligned(currentNode->step.stepElement));
        currentNode->step.swipedLeft = false;
        currentNode->step.swipedRight = false;
        lv_obj_add_flag(currentNode->step.deleteButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(currentNode->step.editButton, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (obj == currentNode->step.stepElementSummary && currentNode->step.swipedLeft == false && currentNode->step.swipedRight == false && currentNode->step.gestureHandled == false) {
        LV_LOG_USER("Click Edit button step address 0x%p", currentNode);
        stepDetail(data, currentNode);
        return;
    }
    if (obj == currentNode->step.deleteButton && currentNode->step.swipedLeft == false && currentNode->step.swipedRight == true) {
        if (gui.element.messagePopup.mBoxPopupParent == NULL) {
            LV_LOG_USER("Click Delete button step address %p", currentNode);
            gui.tempStepNode = currentNode;
            messagePopupCreate(deletePopupTitle_text, deletePopupBody_text, deleteButton_text, stepDetailCancel_text, gui.tempStepNode);
            return;
        }
    }
    if ((obj == currentNode->step.editButton || obj == currentNode->step.editButtonLabel) && currentNode->step.swipedRight == true) {
        LV_LOG_USER("Click Duplicate button step address %p", currentNode);

        if (data->process.processDetails->stepElementsList.size >= MAX_STEP_ELEMENTS) {
            messagePopupCreate(warningPopupTitle_text, maxNumberEntryStepsPopupBody_text, NULL, NULL, NULL);
            return;
        }

        stepNode *newStep = (stepNode *)allocateAndInitializeNode(STEP_NODE);
        if (newStep == NULL) {
            LV_LOG_USER("Failed to allocate memory for duplicate step");
            return;
        }
        /* Zero UI pointers, copy only business data */
        memset(newStep->step.stepDetails, 0, sizeof(sStepDetail));
        newStep->step.stepDetails->data = currentNode->step.stepDetails->data;

        size_t nameLen = strlen(newStep->step.stepDetails->data.stepNameString);
        if (nameLen + 2 <= MAX_PROC_NAME_LEN) {
            strcat(newStep->step.stepDetails->data.stepNameString, "_c");
        } else if (nameLen > 0) {
            newStep->step.stepDetails->data.stepNameString[MAX_PROC_NAME_LEN - 2] = '_';
            newStep->step.stepDetails->data.stepNameString[MAX_PROC_NAME_LEN - 1] = 'c';
            newStep->step.stepDetails->data.stepNameString[MAX_PROC_NAME_LEN] = '\0';
        }

        newStep->step.stepElement = NULL;
        newStep->step.stepStyle.values_and_props = NULL;
        newStep->next = NULL;
        newStep->prev = NULL;

        addStepElement(newStep, data);

        LV_LOG_USER("Duplicate step created at %p, process now has %d steps", newStep, data->process.processDetails->stepElementsList.size);

        stepElementCreate(newStep, data, -1);

        reorderStepElements(data);

        lv_obj_scroll_to_view(newStep->step.stepElement, LV_ANIM_ON);

        lv_obj_set_pos(currentNode->step.stepElement,
            lv_obj_get_x_aligned(currentNode->step.stepElement) - 50,
            lv_obj_get_y_aligned(currentNode->step.stepElement));
        currentNode->step.swipedRight = false;
        lv_obj_add_flag(currentNode->step.deleteButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(currentNode->step.editButton, LV_OBJ_FLAG_HIDDEN);

        calculateTotalTime(data);

        return;
    }
}

static void stepElement_handleLongPress(stepNode *currentNode, lv_obj_t *obj, processNode *data) {
    lv_indev_t *indev = lv_indev_get_act();

    if (indev == NULL)
        return;

    lv_obj_t *stepObj = currentNode->step.stepElement;

    currentNode->step.longPressHandled = true;
    LV_LOG_USER("LV_EVENT_LONG_PRESSED");
    lv_obj_move_foreground(stepObj);
    lv_indev_get_point(indev, &drag_last_point);
    lv_style_set_shadow_spread(&currentNode->step.stepStyle, 3);
    lv_obj_remove_flag(lv_obj_get_parent(stepObj), LV_OBJ_FLAG_SCROLLABLE);
    drag_active = true;
    drag_slot = 0;
    stepNode *tmp = data->process.processDetails->stepElementsList.start;
    while(tmp && tmp != currentNode) { drag_slot++; tmp = tmp->next; }
    drag_original_slot = drag_slot;
    LV_LOG_USER("DRAG START drag_slot=%d step_y=%"PRIi32"", drag_slot, lv_obj_get_y_aligned(stepObj));
}

static void stepElement_handleDrag(stepNode *currentNode, lv_obj_t *obj, processNode *data) {
    lv_indev_t *indev = lv_indev_get_act();

    if (indev == NULL)
        return;

    lv_obj_t *stepObj = currentNode->step.stepElement;

    #define DRAG_AUTO_SCROLL_ZONE       60
    #define DRAG_AUTO_SCROLL_SPEED_MIN  4
    #define DRAG_AUTO_SCROLL_SPEED_MAX  18

    if (gui.tempProcessNode->process.processDetails->stepElementsList.size > 1) {
        if (drag_active) {
            lv_point_t current_point;
            lv_indev_get_point(indev, &current_point);

            lv_coord_t dy = current_point.y - drag_last_point.y;

            lv_obj_set_pos(stepObj, lv_obj_get_x_aligned(stepObj),
                           lv_obj_get_y_aligned(stepObj) + dy);
            drag_last_point = current_point;

            lv_obj_t *container = lv_obj_get_parent(stepObj);
            lv_area_t cont_coords;
            lv_obj_get_coords(container, &cont_coords);

            lv_coord_t dist_bottom = cont_coords.y2 - current_point.y;
            lv_coord_t dist_top    = current_point.y - cont_coords.y1;
            lv_coord_t scroll_speed = 0;

            if(dist_bottom < DRAG_AUTO_SCROLL_ZONE && dist_bottom >= 0) {
                scroll_speed = DRAG_AUTO_SCROLL_SPEED_MIN +
                    (DRAG_AUTO_SCROLL_SPEED_MAX - DRAG_AUTO_SCROLL_SPEED_MIN) *
                    (DRAG_AUTO_SCROLL_ZONE - dist_bottom) / DRAG_AUTO_SCROLL_ZONE;
                lv_coord_t can_scroll = lv_obj_get_scroll_bottom(container);
                if(can_scroll <= 0) { scroll_speed = 0; }
                else if(scroll_speed > can_scroll) { scroll_speed = can_scroll; }
                if(scroll_speed > 0) {
                    lv_obj_scroll_by(container, 0, -scroll_speed, LV_ANIM_OFF);
                    lv_obj_set_pos(stepObj, lv_obj_get_x_aligned(stepObj),
                                   lv_obj_get_y_aligned(stepObj) + scroll_speed);
                }
            } else if(dist_top < DRAG_AUTO_SCROLL_ZONE && dist_top >= 0) {
                scroll_speed = DRAG_AUTO_SCROLL_SPEED_MIN +
                    (DRAG_AUTO_SCROLL_SPEED_MAX - DRAG_AUTO_SCROLL_SPEED_MIN) *
                    (DRAG_AUTO_SCROLL_ZONE - dist_top) / DRAG_AUTO_SCROLL_ZONE;
                lv_coord_t can_scroll = lv_obj_get_scroll_top(container);
                if(can_scroll <= 0) { scroll_speed = 0; }
                else if(scroll_speed > can_scroll) { scroll_speed = can_scroll; }
                if(scroll_speed > 0) {
                    lv_obj_scroll_by(container, 0, scroll_speed, LV_ANIM_OFF);
                    lv_obj_set_pos(stepObj, lv_obj_get_x_aligned(stepObj),
                                   lv_obj_get_y_aligned(stepObj) - scroll_speed);
                }
            }

            lv_coord_t step_y = lv_obj_get_y_aligned(stepObj);
            lv_coord_t step_center_y = step_y + STEP_HEIGHT / 2;
            int total = (int)data->process.processDetails->stepElementsList.size;
            int new_slot = (step_center_y - STEP_Y_START) / STEP_HEIGHT;
            if(new_slot < 0) new_slot = 0;
            if(new_slot >= total) new_slot = total - 1;

            LV_LOG_USER("REPEAT dy=%"PRIi32" step_y=%"PRIi32" center=%"PRIi32" new_slot=%d drag_slot=%d scroll_spd=%"PRIi32"",
                        dy, step_y, step_center_y, new_slot, drag_slot, scroll_speed);

            if(new_slot != drag_slot) {
                LV_LOG_USER("SLOT CHANGED %d -> %d", drag_slot, new_slot);
                drag_slot = new_slot;

                int slot = 0;
                stepNode *n = data->process.processDetails->stepElementsList.start;
                while(n) {
                    if(n != currentNode) {
                        if(slot == drag_slot) slot++;
                        lv_coord_t target_y = STEP_Y_START + slot * STEP_HEIGHT;
                        lv_anim_t a;
                        lv_anim_init(&a);
                        lv_anim_set_var(&a, n->step.stepElement);
                        lv_anim_set_values(&a, lv_obj_get_y_aligned(n->step.stepElement), target_y);
                        lv_anim_set_duration(&a, 150);
                        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
                        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
                        lv_anim_start(&a);
                        slot++;
                    }
                    n = n->next;
                }
            }

            lv_obj_invalidate(stepObj);
        }
    }
}

void event_stepElement(lv_event_t *e) {

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    processNode *data = (processNode *)lv_event_get_user_data(e);
    stepNode *currentNode = getStepElementEntryByObject(obj, data);

    lv_indev_t *indev = lv_indev_active();
    lv_dir_t dir = indev ? lv_indev_get_gesture_dir(indev) : LV_DIR_NONE;

    static lv_point_t press_point;
    static bool ignore_click = false;

    if (indev == NULL)
        return;

    if (currentNode == NULL) {
        LV_LOG_USER("Bad object passed to eventProcessElement!");
        return;
    }

    lv_obj_t *stepObj = currentNode->step.stepElement;

    if (code == LV_EVENT_PRESSED) {
        ignore_click = false;
        currentNode->step.gestureHandled = false;
        lv_indev_get_point(indev, &press_point);
    }

    if (code == LV_EVENT_RELEASED) {
        LV_LOG_USER("LV_EVENT_RELEASED");

        /* Handle drop after drag — must come before gesture check */
        if (currentNode->step.longPressHandled == true) {
            currentNode->step.longPressHandled = false;

            if (drag_active) {
                drag_active = false;

                /* Remove drag shadow */
                lv_style_set_shadow_spread(&currentNode->step.stepStyle, 0);

                /* Re-enable scrolling on the container */
                lv_obj_t *container = lv_obj_get_parent(currentNode->step.stepElement);
                lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);

                /* Reorder the linked list if position changed */
                if (drag_slot != drag_original_slot && drag_slot >= 0) {
                    stepList *list = &data->process.processDetails->stepElementsList;

                    /* Remove node from current position */
                    if (currentNode->prev)
                        currentNode->prev->next = currentNode->next;
                    else
                        list->start = currentNode->next;

                    if (currentNode->next)
                        currentNode->next->prev = currentNode->prev;
                    else
                        list->end = currentNode->prev;

                    /* Insert at drag_slot position */
                    if (drag_slot == 0) {
                        currentNode->prev = NULL;
                        currentNode->next = list->start;
                        if (list->start)
                            list->start->prev = currentNode;
                        list->start = currentNode;
                    } else {
                        stepNode *target = list->start;
                        for (int i = 0; i < drag_slot - 1 && target != NULL; i++)
                            target = target->next;
                        currentNode->prev = target;
                        currentNode->next = target->next;
                        if (target->next)
                            target->next->prev = currentNode;
                        else
                            list->end = currentNode;
                        target->next = currentNode;
                    }

                    LV_LOG_USER("DROP: moved from slot %d to slot %d", drag_original_slot, drag_slot);

                    /* Mark process as changed so Save button enables */
                    data->process.processDetails->data.somethingChanged = true;
                    lv_obj_send_event(data->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL);
                }

                /* Snap all steps to their correct positions with animation */
                int slot = 0;
                stepNode *n = data->process.processDetails->stepElementsList.start;
                while (n) {
                    lv_coord_t target_y = STEP_Y_START + slot * STEP_HEIGHT;
                    lv_anim_t a;
                    lv_anim_init(&a);
                    lv_anim_set_var(&a, n->step.stepElement);
                    lv_anim_set_values(&a, lv_obj_get_y_aligned(n->step.stepElement), target_y);
                    lv_anim_set_duration(&a, 150);
                    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
                    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
                    lv_anim_start(&a);
                    n->step.container_y = target_y;
                    slot++;
                    n = n->next;
                }

                drag_slot = -1;
                drag_original_slot = -1;
                return;
            }
        }

        if (currentNode->step.gestureHandled == true && currentNode->step.swipedLeft == false) {
            currentNode->step.gestureHandled = false;
            return;
        }
    }

    if (code == LV_EVENT_GESTURE && currentNode->step.longPressHandled == false) {
        stepElement_handleGesture(currentNode, obj, dir, data);
    }

    if (code == LV_EVENT_SHORT_CLICKED) {
        stepElement_handleClick(currentNode, obj, data, indev, press_point);
    }

    if (code == LV_EVENT_LONG_PRESSED && currentNode->step.swipedLeft == false && currentNode->step.swipedRight == false && data->process.processDetails->stepElementsList.size > 1) {
        stepElement_handleLongPress(currentNode, obj, data);
    }

    if (code == LV_EVENT_LONG_PRESSED_REPEAT && currentNode->step.swipedLeft == false && currentNode->step.swipedRight == false) {
        currentNode->step.longPressHandled = true;
        stepElement_handleDrag(currentNode, obj, data);
    }

    if (code == LV_EVENT_DELETE) {
        lv_style_reset(&currentNode->step.stepStyle);
        return;
    }
}


void stepElementCreate(stepNode * newStep,processNode * processReference, int8_t tempSize){

	char *tmp_processSourceList[] = processSourceList;

  /*********************
  *    PAGE HEADER
  *********************/

  LV_LOG_USER("Step Creation");


  gui.tempProcessNode = processReference;
  
  calculateTotalTime(processReference);
  
  LV_LOG_USER("Step element created with address 0x%p", newStep);
  LV_LOG_USER("Process element associated with address 0x%p", processReference);

	if( newStep->step.stepStyle.values_and_props == NULL ) {		//Only initialise the style once! 
		lv_style_init(&newStep->step.stepStyle);

    lv_style_set_bg_color(&newStep->step.stepStyle, lv_color_hex(GREY));
    lv_style_set_border_color(&newStep->step.stepStyle, lv_color_hex(GREEN_DARK));
    lv_style_set_border_width(&newStep->step.stepStyle, 4);
    lv_style_set_border_opa(&newStep->step.stepStyle, LV_OPA_50);
    lv_style_set_border_side(&newStep->step.stepStyle, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT);
    lv_style_set_shadow_width(&newStep->step.stepStyle, 5);
    lv_style_set_shadow_spread(&newStep->step.stepStyle, 0);
    lv_style_set_shadow_color(&newStep->step.stepStyle, lv_palette_main(LV_PALETTE_RED));
		LV_LOG_USER("First call to processElementCreate style now initialised");
	}

  newStep->step.swipedLeft = false;
  newStep->step.swipedRight = false;
  newStep->step.gestureHandled = false;
  newStep->step.longPressHandled = false;
  newStep->step.stepElement = lv_obj_create(processReference->process.processDetails->processStepsContainer);
  
  if(tempSize == -1){
		LV_LOG_USER("New Step");
    newStep->step.container_y = -13 + ((processReference->process.processDetails->stepElementsList.size - 1) * 70);
  }
  else{
		LV_LOG_USER("Previous Step");
    newStep->step.container_y = -13 + ((tempSize-1) * 70);
  }
  lv_obj_set_pos(newStep->step.stepElement, -13, newStep->step.container_y);        
  lv_obj_set_size(newStep->step.stepElement, 340, 70);
  lv_obj_remove_flag(newStep->step.stepElement, LV_OBJ_FLAG_SCROLLABLE); 
  lv_obj_set_style_border_opa(newStep->step.stepElement, LV_OPA_TRANSP, 0);
  lv_obj_remove_flag(newStep->step.stepElement, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_set_style_bg_opa(newStep->step.stepElement, LV_OPA_TRANSP, 0);

  lv_obj_add_event_cb(newStep->step.stepElement, event_stepElement, LV_EVENT_GESTURE, processReference);
  lv_obj_add_event_cb(newStep->step.stepElement, event_stepElement, LV_EVENT_LONG_PRESSED_REPEAT, processReference);
  lv_obj_add_event_cb(newStep->step.stepElement, event_stepElement, LV_EVENT_RELEASED, processReference);
  lv_obj_add_event_cb(newStep->step.stepElement, event_stepElement, LV_EVENT_SHORT_CLICKED, processReference);
  lv_obj_add_event_cb(newStep->step.stepElement, event_stepElement, LV_EVENT_LONG_PRESSED, processReference);
  lv_obj_add_event_cb(newStep->step.stepElement, event_stepElement, LV_EVENT_PRESSED, processReference);


  lv_obj_set_pos(newStep->step.stepElement,lv_obj_get_x_aligned(newStep->step.stepElement) - 50,lv_obj_get_y_aligned(newStep->step.stepElement));

  /*********************
  *    PAGE ELEMENTS
  *********************/


        newStep->step.deleteButton = lv_obj_create(newStep->step.stepElement);
        lv_obj_set_style_bg_color(newStep->step.deleteButton, lv_color_hex(RED_DARK), LV_PART_MAIN);
        lv_obj_set_size(newStep->step.deleteButton, 60, 70);
        lv_obj_align(newStep->step.deleteButton, LV_ALIGN_TOP_LEFT, -16, -18);
        lv_obj_add_flag(newStep->step.deleteButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(newStep->step.deleteButton, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(newStep->step.deleteButton, event_stepElement, LV_EVENT_SHORT_CLICKED, processReference);
        lv_obj_add_flag(newStep->step.deleteButton, LV_OBJ_FLAG_CLICKABLE);

                newStep->step.deleteButtonLabel = lv_label_create(newStep->step.deleteButton);         
                lv_label_set_text(newStep->step.deleteButtonLabel, trash_icon); 
                lv_obj_set_style_text_font(newStep->step.deleteButtonLabel, &FilMachineFontIcons_30, 0);              
                lv_obj_align(newStep->step.deleteButtonLabel, LV_ALIGN_CENTER, -5 , 0);

        //Duplicate button - shown on swipe right alongside delete
        newStep->step.editButton = lv_obj_create(newStep->step.stepElement);
        lv_obj_set_style_bg_color(newStep->step.editButton, lv_color_hex(LIGHT_BLUE_DARK), LV_PART_MAIN);
        lv_obj_set_size(newStep->step.editButton, 60, 70);
        lv_obj_align(newStep->step.editButton, LV_ALIGN_TOP_LEFT, 260, -18);
        lv_obj_add_flag(newStep->step.editButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(newStep->step.editButton, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(newStep->step.editButton, event_stepElement, LV_EVENT_SHORT_CLICKED, processReference);
        lv_obj_add_flag(newStep->step.editButton, LV_OBJ_FLAG_CLICKABLE);

                newStep->step.editButtonLabel = lv_label_create(newStep->step.editButton);         
                lv_label_set_text(newStep->step.editButtonLabel, newProcess_icon); 
                lv_obj_set_style_text_font(newStep->step.editButtonLabel, &FilMachineFontIcons_30, 0);              
                lv_obj_align(newStep->step.editButtonLabel, LV_ALIGN_CENTER, 5, 0);

        newStep->step.stepElementSummary = lv_obj_create(newStep->step.stepElement);
        lv_obj_set_size(newStep->step.stepElementSummary, 235, 66);
        lv_obj_align(newStep->step.stepElementSummary, LV_ALIGN_TOP_LEFT, 34, -16);
        lv_obj_remove_flag(newStep->step.stepElementSummary, LV_OBJ_FLAG_SCROLLABLE);  
        lv_obj_add_style(newStep->step.stepElementSummary, &newStep->step.stepStyle, 0);
        lv_obj_add_flag(newStep->step.stepElementSummary, LV_OBJ_FLAG_EVENT_BUBBLE);
                
                newStep->step.stepTypeIcon = lv_label_create(newStep->step.stepElementSummary);

                if(newStep->step.stepDetails->data.type == CHEMISTRY)
                    lv_label_set_text(newStep->step.stepTypeIcon, chemical_icon);
                if(newStep->step.stepDetails->data.type == RINSE)
                    lv_label_set_text(newStep->step.stepTypeIcon, rinse_icon);           
                if(newStep->step.stepDetails->data.type == MULTI_RINSE)
                    lv_label_set_text(newStep->step.stepTypeIcon, multiRinse_icon); 

                lv_obj_set_style_text_font(newStep->step.stepTypeIcon, &FilMachineFontIcons_20, 0);              
                lv_obj_align(newStep->step.stepTypeIcon, LV_ALIGN_LEFT_MID, -9, -12);


                newStep->step.stepName = lv_label_create(newStep->step.stepElementSummary);         
                lv_label_set_text(newStep->step.stepName, newStep->step.stepDetails->data.stepNameString); 
                lv_obj_set_style_text_font(newStep->step.stepName, &lv_font_montserrat_22, 0);      
                lv_label_set_long_mode(newStep->step.stepName, LV_LABEL_LONG_SCROLL_CIRCULAR);
                lv_obj_set_width(newStep->step.stepName, 175);        
                lv_obj_align(newStep->step.stepName, LV_ALIGN_LEFT_MID, 12, -12);
                lv_obj_remove_flag(newStep->step.stepName, LV_OBJ_FLAG_SCROLLABLE); 

                newStep->step.stepTimeIcon = lv_label_create(newStep->step.stepElementSummary);
                lv_label_set_text(newStep->step.stepTimeIcon, clock_icon);
                lv_obj_set_style_text_font(newStep->step.stepTimeIcon, &FilMachineFontIcons_20, 0);
                lv_obj_align(newStep->step.stepTimeIcon, LV_ALIGN_LEFT_MID, -10, 17);
                
                newStep->step.stepTime = lv_label_create(newStep->step.stepElementSummary);    
                lv_label_set_text_fmt(newStep->step.stepTime, "%dm%ds", newStep->step.stepDetails->data.timeMins, newStep->step.stepDetails->data.timeSecs); 
                lv_obj_set_style_text_font(newStep->step.stepTime, &lv_font_montserrat_18, 0);              
                lv_obj_align(newStep->step.stepTime, LV_ALIGN_LEFT_MID, 12, 17);

                newStep->step.sourceLabel = lv_label_create(newStep->step.stepElementSummary); 
                lv_label_set_text_fmt(newStep->step.sourceLabel, "From:%s", tmp_processSourceList[newStep->step.stepDetails->data.source]); 
                lv_obj_set_style_text_font(newStep->step.sourceLabel, &lv_font_montserrat_18, 0);      
                lv_obj_set_width(newStep->step.sourceLabel, 120);        
                lv_obj_align(newStep->step.sourceLabel, LV_ALIGN_LEFT_MID, 85, 17);
                lv_obj_remove_flag(newStep->step.sourceLabel, LV_OBJ_FLAG_SCROLLABLE); 

                newStep->step.discardAfterIcon = lv_label_create(newStep->step.stepElementSummary);        
                lv_label_set_text(newStep->step.discardAfterIcon, discardAfter_icon); 
                lv_obj_set_style_text_font(newStep->step.discardAfterIcon, &FilMachineFontIcons_20, 0);            
                lv_obj_align(newStep->step.discardAfterIcon, LV_ALIGN_RIGHT_MID, 13, 17);

                if(newStep->step.stepDetails->data.discardAfterProc){
                    lv_obj_set_style_text_color(newStep->step.discardAfterIcon, lv_color_hex(WHITE), LV_PART_MAIN);
                  } else {
                    lv_obj_set_style_text_color(newStep->step.discardAfterIcon, lv_color_hex(GREY), LV_PART_MAIN);
                  }


}

