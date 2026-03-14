/**
 * @file element_step.c
 *
 */


//ESSENTIAL INCLUDES
#include "FilMachine.h"
#include <string.h>

extern struct gui_components gui;



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

  processReference->process.processDetails->somethingChanged = true;
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
      free( stepToDelete );	
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

/* Unused - kept for reference
static bool deleteStepElementByObj( lv_obj_t *obj, processNode * processReference ) {
	stepNode	*step_ptr  = getStepElementEntryByObject(obj,processReference);
	return deleteStepElement(step_ptr,processReference, false);
}
*/


/******************************
*  LVGL ELEMENTS IMPLEMENTATION
******************************/

void removeStepElementFromList(processNode *data, stepNode *node) {
	
    // Rimuovi l'elemento dalla lista
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

    // Aggiorna la dimensione della lista
    data->process.processDetails->stepElementsList.size--;

    // Pulizia dei puntatori del nodo
    node->prev = NULL;
    node->next = NULL;
}

void insertStepElementAfter(processNode *data, stepNode *afterNode, stepNode *node) {
	
    // Inserisci l'elemento dopo afterNode nella lista
    if (afterNode == NULL) {
        // Inserisci all'inizio della lista
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

    // Aggiorna l'elemento finale della lista se necessario
    if (afterNode == data->process.processDetails->stepElementsList.end) {
        data->process.processDetails->stepElementsList.end = node;
    }

    // Aggiorna la dimensione della lista
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
    lv_coord_t original_y = -13; // Must match reorderStepElements starting y_offset
    lv_coord_t current_y = 0;  // Posizione Y attuale

    while (current) {
        // Controlla se la posizione Y è cambiata rispetto alla posizione originale prevista
        current_y = lv_obj_get_y_aligned(current->step.stepElement);
        if (current_y != original_y) {
            // Se c'è una differenza, significa che la lista è cambiata
            return true;
        }

        // Aggiorna la posizione Y originale prevista per il prossimo elemento
        original_y += lv_obj_get_height(current->step.stepElement);

        // Passa all'elemento successivo nella lista
        current = current->next;
    }

    // Se si arriva qui, significa che la lista non è cambiata
    return false;
}

void event_stepElement(lv_event_t *e) {
	
    int8_t x;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    /* objElement removed - using stepObj from currentNode instead */
    processNode *data = (processNode *)lv_event_get_user_data(e);
    stepNode *currentNode = getStepElementEntryByObject(obj, data);

    lv_indev_t *indev = lv_indev_active();
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

    static lv_point_t last_point;
    static lv_point_t press_point;       /* Position where press started */
    static bool dragging = false;
    static bool ignore_click = false;    /* Flag to ignore click */

    if (indev == NULL)
        return;

    if (currentNode == NULL) {
        LV_LOG_USER("Bad object passed to eventProcessElement!");
        return;
    }

    /* Use the stepElement from the node - objElement from lv_obj_get_parent(obj)
       is unreliable because obj could be stepElement itself or a child widget */
    lv_obj_t *stepObj = currentNode->step.stepElement;

    if (code == LV_EVENT_PRESSED) {
        ignore_click = false;
        currentNode->step.gestureHandled = false;  /* Reset gesture flag on new press */
        lv_indev_get_point(indev, &press_point);   /* Remember where the press started */
    }

    if (code == LV_EVENT_RELEASED) {
        LV_LOG_USER("LV_EVENT_RELEASED");
        if (currentNode->step.gestureHandled == true && currentNode->step.swipedLeft == false) {
            currentNode->step.gestureHandled = false;
            return;
        }
        if (currentNode->step.longPressHandled == true) {
            /* If a gesture (swipe) was also detected, skip reorder — the gesture takes priority */
            if (currentNode->step.gestureHandled == true) {
                currentNode->step.longPressHandled = false;
                dragging = false;
                lv_style_set_shadow_spread(&currentNode->step.stepStyle, 0);
                lv_obj_add_flag(lv_obj_get_parent(stepObj), LV_OBJ_FLAG_SCROLLABLE);
                LV_LOG_USER("Long press cancelled by gesture");
                return;
            }
            if (data->process.processDetails->stepElementsList.size > 1) {
                lv_style_set_shadow_spread(&currentNode->step.stepStyle, 0);
                lv_obj_add_flag(lv_obj_get_parent(stepObj), LV_OBJ_FLAG_SCROLLABLE);

                stepNode *previous = NULL;
                stepNode *next = data->process.processDetails->stepElementsList.start;
                lv_coord_t obj_y = lv_obj_get_y_aligned(stepObj);

                bool moveUp = true;
                lv_indev_get_point(lv_indev_get_act(), &last_point);

                if (last_point.y > obj_y) {
                    moveUp = false;
                }

                while (next) {
                    /* Skip currentNode to avoid self-referencing circular link */
                    if (next == currentNode) {
                        next = next->next;
                        continue;
                    }
                    lv_coord_t next_y = lv_obj_get_y_aligned(next->step.stepElement);
                    if ((moveUp && next_y >= obj_y) || (!moveUp && next_y > obj_y)) {
                        break;
                    }
                    previous = next;
                    next = next->next;
                }

                removeStepElementFromList(data, currentNode);

                if (previous == NULL) {
                    insertStepElementAfter(data, NULL, currentNode);
                } else {
                    insertStepElementAfter(data, previous, currentNode);
                }

                reorderStepElements(data);
                lv_obj_invalidate(stepObj);

                if (hasListChanged(data)) {
                    gui.tempProcessNode->process.processDetails->somethingChanged = true;
                    lv_obj_send_event(gui.tempProcessNode->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL);
                }

                dragging = false;
            }
            currentNode->step.longPressHandled = false;  /* Reset flag after reorder */
        }
        /* ignore_click is reset in PRESSED handler, not here */
    }

    if (code == LV_EVENT_GESTURE && currentNode->step.longPressHandled == false) {
        /* Block ALL gestures when panel is open — prevents accidental closure on touch */
        if (currentNode->step.swipedRight == true) {
            return;
        }
        currentNode->step.gestureHandled = true;
        switch (dir) {
            case LV_DIR_LEFT:
                if (currentNode->step.swipedLeft == false && currentNode->step.swipedRight == false) {
                    LV_LOG_USER("Left gesture for duplicate popup");
                    /* Check max step limit first */
                    if (data->process.processDetails->stepElementsList.size >= MAX_STEP_ELEMENTS) {
                        messagePopupCreate(warningPopupTitle_text, maxNumberEntryStepsPopupBody_text, NULL, NULL, NULL);
                        ignore_click = true;
                        break;
                    }
                    currentNode->step.swipedLeft = true;
                    currentNode->step.swipedRight = false;
                    gui.tempStepNode = currentNode;
                    messagePopupCreate(duplicateStepPopupTitle_text, duplicateStepPopupBody_text, checkupNo_text, checkupYes_text, gui.tempStepNode);
                    ignore_click = true;
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
                    /* No lv_obj_move_foreground — button stays behind summary
                       (same approach as process elements) */
                    ignore_click = true;
                    break;
                }
                break;
                
            default:
            	break;
        }
    }

      if (code == LV_EVENT_SHORT_CLICKED ) {
        /* If the cursor moved from the press point, this was a swipe attempt — not a tap */
        lv_point_t release_point;
        lv_indev_get_point(indev, &release_point);
        int32_t dx = release_point.x - press_point.x;
        int32_t dy = release_point.y - press_point.y;
        if ((dx * dx + dy * dy) > (10 * 10)) {  /* ~10px threshold */
            LV_LOG_USER("SHORT_CLICKED ignored: cursor moved %"PRId32",%"PRId32" from press", dx, dy);
            return;
        }
        /* When panel is open: tap on summary/step area closes it */
        if (currentNode->step.swipedRight == true && obj != currentNode->step.deleteButton
            && obj != currentNode->step.deleteButtonLabel
            && obj != currentNode->step.editButton
            && obj != currentNode->step.editButtonLabel && !ignore_click) {
            LV_LOG_USER("Tap to close panel");
            x = lv_obj_get_x_aligned(currentNode->step.stepElement) - 50;
            lv_obj_set_pos(currentNode->step.stepElement, x, lv_obj_get_y_aligned(currentNode->step.stepElement));
            currentNode->step.swipedLeft = false;
            currentNode->step.swipedRight = false;
            lv_obj_add_flag(currentNode->step.deleteButton, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(currentNode->step.editButton, LV_OBJ_FLAG_HIDDEN);
            return;
        }
        if (obj == currentNode->step.stepElementSummary && currentNode->step.swipedLeft == false && currentNode->step.swipedRight == false && currentNode->step.gestureHandled == false && !ignore_click) {
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
        if ((obj == currentNode->step.editButton || obj == currentNode->step.editButtonLabel) && currentNode->step.swipedRight == true && !ignore_click) {
            LV_LOG_USER("Click Duplicate button step address %p", currentNode);

            /* Check max step limit */
            if (data->process.processDetails->stepElementsList.size >= MAX_STEP_ELEMENTS) {
                messagePopupCreate(warningPopupTitle_text, maxNumberEntryStepsPopupBody_text, NULL, NULL, NULL);
                return;
            }

            /* Allocate and deep copy the step */
            stepNode *newStep = (stepNode *)allocateAndInitializeNode(STEP_NODE);
            if (newStep == NULL) {
                LV_LOG_USER("Failed to allocate memory for duplicate step");
                return;
            }
            memcpy(newStep->step.stepDetails, currentNode->step.stepDetails, sizeof(sStepDetail));

            /* Null-out LVGL widget pointers to avoid sharing with the original step */
            newStep->step.stepDetails->stepDetailParent = NULL;
            newStep->step.stepDetails->mBoxStepPopupTitleLine = NULL;
            newStep->step.stepDetails->stepDetailNameContainer = NULL;
            newStep->step.stepDetails->stepDetailContainer = NULL;
            newStep->step.stepDetails->stepDurationContainer = NULL;
            newStep->step.stepDetails->stepTypeContainer = NULL;
            newStep->step.stepDetails->stepSourceContainer = NULL;
            newStep->step.stepDetails->stepDiscardAfterContainer = NULL;
            newStep->step.stepDetails->stepDetailLabel = NULL;
            newStep->step.stepDetails->stepDetailNamelLabel = NULL;
            newStep->step.stepDetails->stepDurationLabel = NULL;
            newStep->step.stepDetails->stepDurationMinLabel = NULL;
            newStep->step.stepDetails->stepSaveLabel = NULL;
            newStep->step.stepDetails->stepCancelLabel = NULL;
            newStep->step.stepDetails->stepTypeLabel = NULL;
            newStep->step.stepDetails->stepSourceLabel = NULL;
            newStep->step.stepDetails->stepTypeHelpIcon = NULL;
            newStep->step.stepDetails->stepSourceTempLabel = NULL;
            newStep->step.stepDetails->stepDiscardAfterLabel = NULL;
            newStep->step.stepDetails->stepSourceTempHelpIcon = NULL;
            newStep->step.stepDetails->stepSourceTempValue = NULL;
            newStep->step.stepDetails->stepDiscardAfterSwitch = NULL;
            newStep->step.stepDetails->stepSaveButton = NULL;
            newStep->step.stepDetails->stepCancelButton = NULL;
            newStep->step.stepDetails->stepSourceDropDownList = NULL;
            newStep->step.stepDetails->stepTypeDropDownList = NULL;
            newStep->step.stepDetails->dropDownListStyle = NULL;
            newStep->step.stepDetails->stepDetailSecTextArea = NULL;
            newStep->step.stepDetails->stepDetailMinTextArea = NULL;
            newStep->step.stepDetails->stepDetailNamelTextArea = NULL;

            /* Append suffix to name to distinguish the copy */
            size_t nameLen = strlen(newStep->step.stepDetails->stepNameString);
            if (nameLen + 2 <= MAX_PROC_NAME_LEN) {
                strcat(newStep->step.stepDetails->stepNameString, "_c");
            } else if (nameLen > 0) {
                /* Truncate and add suffix */
                newStep->step.stepDetails->stepNameString[MAX_PROC_NAME_LEN - 2] = '_';
                newStep->step.stepDetails->stepNameString[MAX_PROC_NAME_LEN - 1] = 'c';
                newStep->step.stepDetails->stepNameString[MAX_PROC_NAME_LEN] = '\0';
            }

            /* Reset LVGL widget pointers in the new node (will be created by stepElementCreate) */
            newStep->step.stepElement = NULL;
            newStep->step.stepStyle.values_and_props = NULL;
            newStep->next = NULL;
            newStep->prev = NULL;

            /* Append to the END of the list (addStepElement handles
               size++, somethingChanged, and processSaveButton refresh) */
            addStepElement(newStep, data);

            LV_LOG_USER("Duplicate step created at %p, process now has %d steps", newStep, data->process.processDetails->stepElementsList.size);

            /* Create the GUI element for the new step */
            stepElementCreate(newStep, data, -1);

            /* Reorder all step elements to correct positions */
            reorderStepElements(data);

            /* Scroll to the new step at the bottom */
            lv_obj_scroll_to_view(newStep->step.stepElement, LV_ANIM_ON);

            /* Close the swipe panel on the original step */
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

    if (code == LV_EVENT_LONG_PRESSED && currentNode->step.swipedLeft == false && currentNode->step.swipedRight == false && data->process.processDetails->stepElementsList.size > 1) {
        currentNode->step.longPressHandled = true;
        LV_LOG_USER("LV_EVENT_LONG_PRESSED");
        lv_obj_move_foreground(stepObj);
        lv_indev_get_point(lv_indev_get_act(), &last_point);
        lv_style_set_shadow_spread(&currentNode->step.stepStyle, 3);
        lv_obj_remove_flag(lv_obj_get_parent(stepObj), LV_OBJ_FLAG_SCROLLABLE);
        dragging = true;
    }

    if (code == LV_EVENT_LONG_PRESSED_REPEAT && currentNode->step.swipedLeft == false && currentNode->step.swipedRight == false) {
        currentNode->step.longPressHandled = true;
        LV_LOG_USER("LV_EVENT_LONG_PRESSED_REPEAT");

        if (gui.tempProcessNode->process.processDetails->stepElementsList.size > 1) {
            if (dragging) {
                lv_point_t current_point;
                lv_indev_get_point(lv_indev_get_act(), &current_point);

                lv_coord_t dy = current_point.y - last_point.y;

                if (gui.tempProcessNode->process.processDetails->stepElementsList.start == currentNode) {
                    LV_LOG_USER("IS FIRST STEP IN LIST %"PRIi32" %"PRIi32"", dy, lv_obj_get_y_aligned(stepObj));
                    if ((dy + lv_obj_get_y_aligned(stepObj)) >= -16) {
                        lv_obj_set_pos(stepObj, lv_obj_get_x_aligned(stepObj), lv_obj_get_y_aligned(stepObj) + dy);
                        last_point = current_point;
                        lv_obj_invalidate(stepObj);
                    }
                } else if (gui.tempProcessNode->process.processDetails->stepElementsList.end == currentNode) {
                    LV_LOG_USER("IS LAST STEP IN LIST %"PRIi32" %"PRIi32"", dy, lv_obj_get_y_aligned(stepObj));
                    if ((dy + lv_obj_get_y_aligned(stepObj)) <= (((gui.tempProcessNode->process.processDetails->stepElementsList.size) * 70) - 53)) {
                        lv_obj_set_pos(stepObj, lv_obj_get_x_aligned(stepObj), lv_obj_get_y_aligned(stepObj) + dy);
                        last_point = current_point;
                        lv_obj_invalidate(stepObj);
                    }
                } else {
                    LV_LOG_USER("IS MIDDLE STEP IN LIST %"PRIi32" %"PRIi32"", dy, lv_obj_get_y_aligned(stepObj));
                    lv_obj_set_pos(stepObj, lv_obj_get_x_aligned(stepObj), lv_obj_get_y_aligned(stepObj) + dy);
                    last_point = current_point;
                    lv_obj_invalidate(stepObj);
                }
            }
        }
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
        //lv_obj_set_style_border_color(newStep->step.stepElementSummary, lv_color_hex(LV_PALETTE_ORANGE), 0);
        lv_obj_set_size(newStep->step.stepElementSummary, 235, 66);
        lv_obj_align(newStep->step.stepElementSummary, LV_ALIGN_TOP_LEFT, 34, -16);
        lv_obj_remove_flag(newStep->step.stepElementSummary, LV_OBJ_FLAG_SCROLLABLE);  
        lv_obj_add_style(newStep->step.stepElementSummary, &newStep->step.stepStyle, 0);
        lv_obj_add_flag(newStep->step.stepElementSummary, LV_OBJ_FLAG_EVENT_BUBBLE);
                
                newStep->step.stepTypeIcon = lv_label_create(newStep->step.stepElementSummary);

                if(newStep->step.stepDetails->type == CHEMISTRY)
                    lv_label_set_text(newStep->step.stepTypeIcon, chemical_icon);
                if(newStep->step.stepDetails->type == RINSE)
                    lv_label_set_text(newStep->step.stepTypeIcon, rinse_icon);           
                if(newStep->step.stepDetails->type == MULTI_RINSE)
                    lv_label_set_text(newStep->step.stepTypeIcon, multiRinse_icon); 

                lv_obj_set_style_text_font(newStep->step.stepTypeIcon, &FilMachineFontIcons_20, 0);              
                lv_obj_align(newStep->step.stepTypeIcon, LV_ALIGN_LEFT_MID, -9, -12);


                newStep->step.stepName = lv_label_create(newStep->step.stepElementSummary);         
                lv_label_set_text(newStep->step.stepName, newStep->step.stepDetails->stepNameString); 
                lv_obj_set_style_text_font(newStep->step.stepName, &lv_font_montserrat_22, 0);      
                lv_label_set_long_mode(newStep->step.stepName, LV_LABEL_LONG_SCROLL_CIRCULAR);
                lv_obj_set_width(newStep->step.stepName, 175);        
                lv_obj_align(newStep->step.stepName, LV_ALIGN_LEFT_MID, 12, -12);
                lv_obj_remove_flag(newStep->step.stepName, LV_OBJ_FLAG_SCROLLABLE); 

                newStep->step.stepTimeIcon = lv_label_create(newStep->step.stepElementSummary);          
                lv_label_set_text(newStep->step.stepTimeIcon, clock_icon);                  
                lv_obj_set_style_text_font(newStep->step.stepTimeIcon, &FilMachineFontIcons_20, 0);
                //lv_obj_set_style_text_color(newStep->step.stepTimeIcon, lv_color_hex(GREY), LV_PART_MAIN);
                lv_obj_align(newStep->step.stepTimeIcon, LV_ALIGN_LEFT_MID, -10, 17);
                
                newStep->step.stepTime = lv_label_create(newStep->step.stepElementSummary);    
                lv_label_set_text_fmt(newStep->step.stepTime, "%dm%ds", newStep->step.stepDetails->timeMins, newStep->step.stepDetails->timeSecs); 
                lv_obj_set_style_text_font(newStep->step.stepTime, &lv_font_montserrat_18, 0);              
                lv_obj_align(newStep->step.stepTime, LV_ALIGN_LEFT_MID, 12, 17);

                newStep->step.sourceLabel = lv_label_create(newStep->step.stepElementSummary); 
                lv_label_set_text_fmt(newStep->step.sourceLabel, "From:%s", tmp_processSourceList[newStep->step.stepDetails->source]); 
                lv_obj_set_style_text_font(newStep->step.sourceLabel, &lv_font_montserrat_18, 0);      
                lv_obj_set_width(newStep->step.sourceLabel, 120);        
                lv_obj_align(newStep->step.sourceLabel, LV_ALIGN_LEFT_MID, 85, 17);
                lv_obj_remove_flag(newStep->step.sourceLabel, LV_OBJ_FLAG_SCROLLABLE); 

                newStep->step.discardAfterIcon = lv_label_create(newStep->step.stepElementSummary);        
                lv_label_set_text(newStep->step.discardAfterIcon, discardAfter_icon); 
                lv_obj_set_style_text_font(newStep->step.discardAfterIcon, &FilMachineFontIcons_20, 0);            
                lv_obj_align(newStep->step.discardAfterIcon, LV_ALIGN_RIGHT_MID, 13, 17);

                if(newStep->step.stepDetails->discardAfterProc){
                    lv_obj_set_style_text_color(newStep->step.discardAfterIcon, lv_color_hex(WHITE), LV_PART_MAIN);
                  } else {
                    lv_obj_set_style_text_color(newStep->step.discardAfterIcon, lv_color_hex(GREY), LV_PART_MAIN);
                  }


}

