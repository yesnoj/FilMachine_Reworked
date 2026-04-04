/**
 * @file page_processDetail.c
 *
 */
#include <string.h>

#include "FilMachine.h"
#if defined(DISPLAY_DRIVER_ST7701)
#include "st7701_lcd.h"
#endif

extern struct gui_components gui;
processNode *existingProcess;

/* Backup snapshot taken when process detail is opened — used for discard */
static processNode *s_processBackup = NULL;

static void process_detail_free_backup(void) {
    if (s_processBackup != NULL) {
        process_node_destroy(s_processBackup);
        s_processBackup = NULL;
    }
}

processNode *getProcessDetailBackup(void) { return s_processBackup; }
void clearProcessDetailBackup(void) { s_processBackup = NULL; /* caller takes ownership */ }

/**
 * Check whether the process detail form is ready to save.
 * Returns true if there's at least one change, one step, and a non-empty name.
 */
static bool process_detail_is_valid(sProcessDetail *pd) {
    if (pd == NULL) return false;
    if (!pd->data.somethingChanged) return false;
    if (pd->stepElementsList.size == 0) return false;
    const char *name = lv_textarea_get_text(pd->processDetailNameTextArea);
    return (name != NULL && strlen(name) > 0);
}

/*--------------------------------------------------------------
 *  Sub-functions for event_processDetail  (Point 3 refactor)
 *-------------------------------------------------------------*/

/**
 * Single teardown point for the process detail screen runtime.
 * Resets all runtime resources, then destroys the UI tree.
 * Future additions (timers, dynamic buffers, handles) go here.
 */
static void process_detail_teardown(processNode *pn) {
    sProcessDetail *pd = pn->process.processDetails;

    /* 1. Reset runtime styles */
    lv_style_reset(&pd->textAreaStyle);

    /* 2. Save the parent pointer, then null out LVGL widget pointers
     *    BEFORE deleting the tree.  This way, async callbacks that fire
     *    after the delete (e.g. calculateTotalTime called from
     *    ws_async_update_process) won't touch freed memory. */
    lv_obj_t *parent = pd->processDetailParent;
    pd->processTotalTimeValue = NULL;
    pd->processDetailParent   = NULL;

    /* 3. Destroy the UI object tree */
    if (parent != NULL) lv_obj_delete(parent);

    /* 4. Free backup snapshot */
    process_detail_free_backup();
}

/** Handle Close button: teardown runtime, return to main menu.
 *  If there are unsaved changes, show a confirmation popup first. */
static void process_detail_close(processNode *pn) {
    sProcessDetail *pd = pn->process.processDetails;
    if (pd != NULL && pd->data.somethingChanged) {
        /* Show "discard unsaved changes?" popup */
        messagePopupCreate(discardChangesTitle_text, discardChangesBody_text,
                           discardChangesNo_text, discardChangesYes_text,
                           getProcessDiscardSentinel());
        return;
    }
    LV_LOG_USER("Close Process Detail (no changes)");
    process_detail_teardown(pn);
#if defined(DISPLAY_DRIVER_ST7701)
    st7701_lcd_fill_screen(0x0000);
#endif
    lv_scr_load(gui.page.menu.screen_mainMenu);
    lv_obj_invalidate(gui.page.menu.screen_mainMenu);
}

/** Handle film type selection (Color / B&W labels) */
static void process_detail_set_film_type(processNode *pn, filmType_t ft) {
    sProcessDetail *pd = pn->process.processDetails;
    if (ft == pd->data.filmType) return;  /* no change */
    pd->data.filmType = ft;
    pd->data.somethingChanged = true;

    if (ft == COLOR_FILM) {
        lv_obj_set_style_text_color(pd->processColorLabel, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_set_style_text_color(pd->processBnWLabel, lv_color_hex(WHITE), LV_PART_MAIN);
        LV_LOG_USER("Pressed processColorLabel %d", pd->data.filmType);
    } else {
        lv_obj_set_style_text_color(pd->processBnWLabel, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
        lv_obj_set_style_text_color(pd->processColorLabel, lv_color_hex(WHITE), LV_PART_MAIN);
        LV_LOG_USER("Pressed processBnWLabel %d", pd->data.filmType);
    }

    lv_obj_send_event(pd->processSaveButton, LV_EVENT_REFRESH, NULL);
}

/** Toggle the preferred flag on the process */
static void process_detail_toggle_preferred(processNode *pn) {
    sProcessDetail *pd = pn->process.processDetails;

    if (lv_color_eq(lv_obj_get_style_text_color(pd->processPreferredLabel, LV_PART_MAIN), lv_color_hex(RED))) {
        lv_obj_set_style_text_color(pd->processPreferredLabel, lv_color_hex(WHITE), LV_PART_MAIN);
        pd->data.isPreferred = 0;
    } else {
        lv_obj_set_style_text_color(pd->processPreferredLabel, lv_color_hex(RED), LV_PART_MAIN);
        pd->data.isPreferred = 1;
    }
    pd->data.somethingChanged = true;

    lv_obj_send_event(pd->processSaveButton, LV_EVENT_REFRESH, NULL);
}

/** Handle Save button: persist process data and update UI */
static void process_detail_save(processNode *pn) {
    sProcessDetail *pd = pn->process.processDetails;

    if (pd->stepElementsList.size > 0) {
        pd->data.somethingChanged = false;

        snprintf(pd->data.processNameString, sizeof(pd->data.processNameString),
                 "%s", lv_textarea_get_text(pd->processDetailNameTextArea));
        lv_obj_clear_state(pd->processRunButton, LV_STATE_DISABLED);
        lv_obj_clear_state(pd->processDetailCloseButton, LV_STATE_DISABLED);
        lv_obj_add_state(pd->processSaveButton, LV_STATE_DISABLED);

        if (isNodeInList((void *)&(gui.page.processes.processElementsList), pn, PROCESS_NODE) == NULL) {
            LV_LOG_USER("Process not in list");
            if (addProcessElement(pn) != NULL) {
                LV_LOG_USER("Create GUI entry");
                processElementCreate(pn, -1);
            } else {
                LV_LOG_USER("Process list is full");
            }
        } else {
            LV_LOG_USER("Process element exists so edited");
        }
        qSysAction(SAVE_PROCESS_CONFIG);

        /* Save succeeded — update the backup to reflect new saved state */
        process_detail_free_backup();
        s_processBackup = deepCopyProcessNode(pn);

        updateProcessElement(pn);
        if (gui.page.processes.isFiltered == 1)
            filterAndDisplayProcesses();
        LV_LOG_USER("Pressed processSaveButton");
    } else {
        lv_obj_add_state(pd->processSaveButton, LV_STATE_DISABLED);
        lv_obj_add_state(pd->processRunButton, LV_STATE_DISABLED);
        lv_obj_clear_state(pd->processDetailCloseButton, LV_STATE_DISABLED);
    }
}

/** Handle Run button: reset styles, then hand off to checkup.
 *  Note: do NOT call process_detail_teardown() here — initCheckup()
 *  deletes processDetailParent itself as part of the screen transition.
 */
static void process_detail_run(processNode *pn) {
    LV_LOG_USER("Pressed processRunButton");
    /* Clear any leftover keyboard context from processDetail to avoid
       stale focus events interfering with the checkup screen */
    hideKeyboard(pn->process.processDetails->processDetailParent);
    pn->process.processDetails->checkup->checkupParent = NULL;
    lv_style_reset(&pn->process.processDetails->textAreaStyle);
    /* Null out before checkup deletes the parent — prevents stale access */
    pn->process.processDetails->processTotalTimeValue = NULL;
    checkup(pn);
}

/** Handle temperature control switch value change */
static void process_detail_handle_temp_control(processNode *pn, lv_obj_t *obj) {
    sProcessDetail *pd = pn->process.processDetails;

    bool newVal = lv_obj_has_state(obj, LV_STATE_CHECKED);
    LV_LOG_USER("Temperature controlled : %s", newVal ? "On" : "Off");
    if (newVal != pd->data.isTempControlled) {
        pd->data.somethingChanged = true;
    }
    pd->data.isTempControlled = newVal;

    /* Enable/disable temperature and tolerance fields based on temp control state */
    if (pd->data.isTempControlled) {
        lv_obj_remove_state(pd->processTempTextArea, LV_STATE_DISABLED);
        lv_obj_remove_state(pd->processToleranceTextArea, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(pd->processTempTextArea, LV_STATE_DISABLED);
        lv_obj_add_state(pd->processToleranceTextArea, LV_STATE_DISABLED);
    }

    lv_obj_send_event(pd->processSaveButton, LV_EVENT_REFRESH, NULL);
}

/** Handle FOCUSED event: open roller popup for temperature / tolerance */
static void process_detail_open_roller(processNode *pn, lv_obj_t *target) {
    /* Guard: prevent double-fire of FOCUSED event from creating two popups */
    if (gui.element.rollerPopup.mBoxRollerParent != NULL) return;

    sProcessDetail *pd = pn->process.processDetails;

    /* Guard: don't open roller popups if temp control is disabled */
    if (!pd->data.isTempControlled
        && (target == pd->processTempTextArea || target == pd->processToleranceTextArea)) {
        return;
    }

    if (target == pd->processTempTextArea) {
        LV_LOG_USER("Set Temperature");
        pd->tempRollerCtx.textArea = pd->processTempTextArea;
        pd->tempRollerCtx.saveButton = pd->processSaveButton;
        if (gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP) {
            rollerPopupCreate(gui.element.rollerPopup.tempCelsiusOptions,
                              tuneTempPopupTitle_text, &pd->tempRollerCtx,
                              findRollerStringIndex(lv_textarea_get_text(pd->processTempTextArea),
                                                    gui.element.rollerPopup.tempCelsiusOptions), GREEN_DARK);
        } else {
            rollerPopupCreate(gui.element.rollerPopup.tempFahrenheitOptions,
                              tuneTempPopupTitle_text, &pd->tempRollerCtx,
                              findRollerStringIndex(lv_textarea_get_text(pd->processTempTextArea),
                                                    gui.element.rollerPopup.tempFahrenheitOptions), GREEN_DARK);
        }
    }
    if (target == pd->processToleranceTextArea) {
        LV_LOG_USER("Set Tolerance");
        pd->toleranceRollerCtx.textArea = pd->processToleranceTextArea;
        pd->toleranceRollerCtx.saveButton = pd->processSaveButton;
        rollerPopupCreate(gui.element.rollerPopup.tempToleranceOptions,
                          tuneTolerancePopupTitle_text, &pd->toleranceRollerCtx,
                          findRollerStringIndex(lv_textarea_get_text(pd->processToleranceTextArea),
                                                gui.element.rollerPopup.tempToleranceOptions), GREEN_DARK);
    }
}

/** Handle REFRESH event: enable/disable Save button based on validation */
static void process_detail_refresh_save_button(processNode *pn, lv_obj_t *obj) {
    sProcessDetail *pd = pn->process.processDetails;
    if (obj == pd->processSaveButton) {
        if (process_detail_is_valid(pd)) {
            lv_obj_clear_state(pd->processSaveButton, LV_STATE_DISABLED);
            /* Close button stays enabled — confirmation popup handles unsaved changes */
            lv_obj_add_state(pd->processRunButton, LV_STATE_DISABLED);
            LV_LOG_USER("Updated SAVE button : ENABLED");
        }
    }
}

/*--------------------------------------------------------------
 *  Main event dispatcher
 *
 *  Points 1/5 refactor: the processNode* is passed as user_data
 *  in every event registration, so the handler never reads
 *  gui.tempProcessNode.  Widget identification uses
 *  lv_event_get_current_target() (the object the handler is
 *  registered on) instead of user_data.
 *-------------------------------------------------------------*/

void event_processDetail(lv_event_t * e) {

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * widget = lv_event_get_current_target(e);

  /* Context: processNode* stored during event registration */
  processNode *pn = (processNode *)lv_event_get_user_data(e);
  if(pn == NULL || pn->process.processDetails == NULL) return;

  sProcessDetail *pd = pn->process.processDetails;

  if(code == LV_EVENT_CLICKED) {
    if(widget == pd->processDetailCloseButton)   process_detail_close(pn);
    if(widget == pd->processColorLabel)           process_detail_set_film_type(pn, COLOR_FILM);
    if(widget == pd->processBnWLabel)             process_detail_set_film_type(pn, BLACK_AND_WHITE_FILM);
    if(widget == pd->processPreferredLabel)       process_detail_toggle_preferred(pn);
    if(widget == pd->processSaveButton)           process_detail_save(pn);
    if(widget == pd->processRunButton)            process_detail_run(pn);
    if(widget == pd->processNewStepButton) {
        LV_LOG_USER("Pressed processNewStepButton");
        stepDetail(pn, NULL);
    }
  }

  if(code == LV_EVENT_REFRESH) {
    process_detail_refresh_save_button(pn, widget);
  }

  if(code == LV_EVENT_VALUE_CHANGED) {
    if(widget == pd->processTempControlSwitch)
        process_detail_handle_temp_control(pn, widget);
  }

  if(code == LV_EVENT_FOCUSED) {
    process_detail_open_roller(pn, widget);
  }
}



/* ═══════════════════════════════════════════════════════════════════
 *  Live-update the open process detail popup when data changes
 *  externally (e.g. via Flutter / WebSocket).
 *
 *  Safety: skipped when the user has unsaved local edits
 *  (somethingChanged == true) to avoid overwriting their work.
 *  Called from the LVGL thread only (via ws_async_update_process).
 * ═══════════════════════════════════════════════════════════════════ */
void process_detail_live_update(processNode *pn) {
    if (pn == NULL || pn->process.processDetails == NULL) return;
    sProcessDetail *pd = pn->process.processDetails;

    /* Only update if the process detail popup is actually open */
    if (pd->processDetailParent == NULL) return;

    /* Don't overwrite if the user is editing locally */
    if (pd->data.somethingChanged) {
        LV_LOG_USER("[LiveUpdate] Skipped — process-level somethingChanged=true");
        return;
    }

    LV_LOG_USER("[LiveUpdate] ENTER — process '%s', somethingChanged=false",
        pd->data.processNameString);

    char buf[20];

    /* 1. Name */
    if (pd->processDetailNameTextArea != NULL) {
        const char *cur = lv_textarea_get_text(pd->processDetailNameTextArea);
        if (cur == NULL || strcmp(cur, pd->data.processNameString) != 0)
            lv_textarea_set_text(pd->processDetailNameTextArea, pd->data.processNameString);
    }

    /* 2. Temperature */
    if (pd->processTempTextArea != NULL) {
        if (gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP)
            snprintf(buf, sizeof(buf), "%"PRIi32"", pd->data.temp);
        else
            snprintf(buf, sizeof(buf), "%"PRIi32"", convertCelsiusToFahrenheit(pd->data.temp));
        const char *cur = lv_textarea_get_text(pd->processTempTextArea);
        if (cur == NULL || strcmp(cur, buf) != 0)
            lv_textarea_set_text(pd->processTempTextArea, buf);
    }

    /* 3. Tolerance */
    if (pd->processToleranceTextArea != NULL) {
        snprintf(buf, sizeof(buf), "%.1f", pd->data.tempTolerance);
        const char *cur = lv_textarea_get_text(pd->processToleranceTextArea);
        if (cur == NULL || strcmp(cur, buf) != 0)
            lv_textarea_set_text(pd->processToleranceTextArea, buf);
    }

    /* 4. Temp-controlled switch */
    if (pd->processTempControlSwitch != NULL) {
        bool isChecked = lv_obj_has_state(pd->processTempControlSwitch, LV_STATE_CHECKED);
        if (pd->data.isTempControlled && !isChecked)
            lv_obj_add_state(pd->processTempControlSwitch, LV_STATE_CHECKED);
        else if (!pd->data.isTempControlled && isChecked)
            lv_obj_clear_state(pd->processTempControlSwitch, LV_STATE_CHECKED);

        /* Enable/disable temp and tolerance fields */
        if (pd->data.isTempControlled) {
            lv_obj_clear_state(pd->processTempTextArea, LV_STATE_DISABLED);
            lv_obj_clear_state(pd->processToleranceTextArea, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(pd->processTempTextArea, LV_STATE_DISABLED);
            lv_obj_add_state(pd->processToleranceTextArea, LV_STATE_DISABLED);
        }
    }

    /* 5. Film type */
    if (pd->processColorLabel != NULL && pd->processBnWLabel != NULL) {
        if (pd->data.filmType == COLOR_FILM) {
            lv_obj_set_style_text_color(pd->processColorLabel, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
            lv_obj_set_style_text_color(pd->processBnWLabel, lv_color_hex(WHITE), LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_color(pd->processBnWLabel, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
            lv_obj_set_style_text_color(pd->processColorLabel, lv_color_hex(WHITE), LV_PART_MAIN);
        }
    }

    /* 6. Preferred */
    if (pd->processPreferredLabel != NULL) {
        lv_obj_set_style_text_color(pd->processPreferredLabel,
            pd->data.isPreferred ? lv_color_hex(RED) : lv_color_hex(WHITE),
            LV_PART_MAIN);
    }

    /* 7. Total time */
    if (pd->processTotalTimeValue != NULL) {
        lv_label_set_text_fmt(pd->processTotalTimeValue, "%"PRIu32"m%"PRIu8"s",
            pd->data.timeMins, pd->data.timeSecs);
    }

    /* 8. Step list — destroy all step LVGL elements and recreate from
     *    the current linked list.  This handles add / edit / delete /
     *    reorder that arrived via WebSocket from Flutter.
     *
     *    Skip the rebuild if a step-detail popup is currently open:
     *    the popup lives on lv_layer_top() but stepElementCreate
     *    resets swipe/gesture state on the stepNode, which breaks
     *    the popup's Cancel flow. */
    if (pd->processStepsContainer != NULL) {
        /* Check whether any step detail popup is open */
        bool stepPopupOpen = false;
        {
            stepNode *chk = pd->stepElementsList.start;
            while (chk) {
                if (chk->step.stepDetails != NULL &&
                    chk->step.stepDetails->stepDetailParent != NULL) {
                    stepPopupOpen = true;
                    break;
                }
                chk = chk->next;
            }
        }

        LV_LOG_USER("[LiveUpdate] stepPopupOpen=%d, stepList size=%d",
            stepPopupOpen, pd->stepElementsList.size);

        if (stepPopupOpen) {
            /* Don't rebuild step elements — but DO live-update the open
             * step detail popup AND update existing step element labels
             * in-place (no destroy/recreate). */
            stepNode *sn = pd->stepElementsList.start;
            while (sn) {
                LV_LOG_USER("[LiveUpdate] Calling step_detail_live_update for step '%s'",
                    sn->step.stepDetails ? sn->step.stepDetails->data.stepNameString : "NULL");
                step_detail_live_update(sn);
                if (sn->step.stepElement != NULL)
                    updateStepElement(pn, sn);
                sn = sn->next;
            }
            LV_LOG_USER("[LiveUpdate] Step popup updated in-place (rebuild skipped)");
        } else {
            /* a) Delete every step's LVGL widget tree.
             *    lv_obj_clean removes all children; the LV_EVENT_DELETE
             *    handler in element_step.c calls lv_style_reset for each
             *    live stepNode it can still find in the list. */
            lv_obj_clean(pd->processStepsContainer);

            /* b) Null out stepElement pointers on live nodes (the LVGL
             *    objects they pointed to are now gone). */
            stepNode *sn = pd->stepElementsList.start;
            while (sn) {
                sn->step.stepElement = NULL;
                sn = sn->next;
            }

            /* c) Recreate step element cards from the current list order. */
            int8_t idx = 1;
            sn = pd->stepElementsList.start;
            while (sn) {
                stepElementCreate(sn, pn, idx);
                sn = sn->next;
                idx++;
            }

            LV_LOG_USER("[LiveUpdate] Step list rebuilt (%d steps)",
                         pd->stepElementsList.size);
        }
    }

    LV_LOG_USER("[LiveUpdate] Process detail refreshed from external data");
}


/*********************
  *    PROCESS DETAIL
*********************/

void processDetail(lv_obj_t * processContainer)
{
/*********************
  *    PAGE HEADER
*********************/
  char formatted_string[20];
  char tempBuffer[10];
  processNode* existingProcess;


  existingProcess = (processNode*)isNodeInList((void*)&(gui.page.processes.processElementsList), gui.tempProcessNode, PROCESS_NODE);


if(existingProcess != NULL) {
    LV_LOG_USER("Process already present");
    gui.tempProcessNode = existingProcess; /* Use existing node instead of allocating a new one */

  } else {
      gui.tempProcessNode = (processNode*)allocateAndInitializeNode(PROCESS_NODE);
      if (gui.tempProcessNode == NULL) {
          LV_LOG_USER("Failed to allocate tempProcessNode");
          return;
      }
      gui.tempProcessNode->process.processDetails->data.filmType = BLACK_AND_WHITE_FILM;
      gui.tempProcessNode->process.processDetails->data.temp = 20;  /* default 20°C */
      gui.tempProcessNode->process.isFiltered = false;
  }

  gui.tempProcessNode->process.processDetails->processesContainer = processContainer;

  /* Local aliases */
  const ui_process_detail_layout_t *ui = &ui_get_profile()->process_detail;
  processNode *pn = gui.tempProcessNode;
  sProcessDetail *pd = pn->process.processDetails;

  /* Snapshot current process so we can restore on discard (data + steps) */
  process_detail_free_backup();
  if (existingProcess != NULL) {
      s_processBackup = deepCopyProcessNode(pn);
      if (s_processBackup == NULL) {
          LV_LOG_USER("Warning: failed to create process backup for discard");
      }
  }

  memset(&pd->nameKeyboardCtx, 0, sizeof(pd->nameKeyboardCtx));
  pd->nameKeyboardCtx.owner = KB_OWNER_PROCESS;
  pd->nameKeyboardCtx.ownerData = pd;
  memset(&pd->tempRollerCtx, 0, sizeof(pd->tempRollerCtx));
  pd->tempRollerCtx.owner = ROLLER_OWNER_PROCESS_TEMP;
  pd->tempRollerCtx.ownerData = pd;
  memset(&pd->toleranceRollerCtx, 0, sizeof(pd->toleranceRollerCtx));
  pd->toleranceRollerCtx.owner = ROLLER_OWNER_PROCESS_TOLERANCE;
  pd->toleranceRollerCtx.ownerData = pd;

  LV_LOG_USER("Processes available %"PRIi32"",gui.page.processes.processElementsList.size);
  LV_LOG_USER("Process address 0x%p, with n:%"PRIu16" steps",pn, pd->stepElementsList.size);

  pd->processDetailParent = lv_obj_create(NULL);
#if defined(DISPLAY_DRIVER_ST7701)
      st7701_lcd_fill_screen(0x0000);
#endif
      lv_scr_load(pd->processDetailParent);
      lv_obj_invalidate(pd->processDetailParent);
      pd->nameKeyboardCtx.parentScreen = pd->processDetailParent;

  lv_style_init(&pd->textAreaStyle);


      pd->processDetailContainer = lv_obj_create(pd->processDetailParent);
      lv_obj_align(pd->processDetailContainer, LV_ALIGN_CENTER, ui->container_x, ui->container_y);
      lv_obj_set_size(pd->processDetailContainer, LV_PCT(100), LV_PCT(100));
      lv_obj_remove_flag(pd->processDetailContainer, LV_OBJ_FLAG_SCROLLABLE);

            pd->processDetailCloseButton = lv_button_create(pd->processDetailContainer);
            lv_obj_set_size(pd->processDetailCloseButton, ui->close_w, ui->close_h);
            lv_obj_align(pd->processDetailCloseButton, LV_ALIGN_TOP_RIGHT, ui->close_x, ui->close_y);
            lv_obj_add_event_cb(pd->processDetailCloseButton, event_processDetail, LV_EVENT_CLICKED, pn);
            lv_obj_set_style_bg_color(pd->processDetailCloseButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
            lv_obj_move_foreground(pd->processDetailCloseButton);


                  pd->processDetailCloseButtonLabel = lv_label_create(pd->processDetailCloseButton);
                  lv_label_set_text(pd->processDetailCloseButtonLabel, closePopup_icon);
                  lv_obj_set_style_text_font(pd->processDetailCloseButtonLabel, ui->close_icon_font, 0);
                  lv_obj_align(pd->processDetailCloseButtonLabel, LV_ALIGN_CENTER, 0, 0);



            pd->processDetailNameContainer = lv_obj_create(pd->processDetailContainer);
            lv_obj_remove_flag(pd->processDetailNameContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_align(pd->processDetailNameContainer, LV_ALIGN_TOP_LEFT, ui->name_x, ui->name_y);
            lv_obj_set_size(pd->processDetailNameContainer, ui->name_w, ui->name_h);
            lv_obj_set_style_border_opa(pd->processDetailNameContainer, LV_OPA_TRANSP, 0);


                  pd->processDetailNameTextArea = lv_textarea_create(pd->processDetailNameContainer);
                  lv_textarea_set_one_line(pd->processDetailNameTextArea, true);
                  lv_textarea_set_placeholder_text(pd->processDetailNameTextArea, processDetailPlaceHolder_text);
                  lv_obj_set_width(pd->processDetailNameTextArea, ui->name_textarea_w);
                  lv_obj_set_style_text_font(pd->processDetailNameTextArea, ui->name_font, 0);
                  lv_obj_align(pd->processDetailNameTextArea, LV_ALIGN_TOP_LEFT, ui->name_textarea_x, ui->name_textarea_y);
                  pd->nameKeyboardCtx.textArea = pd->processDetailNameTextArea;
                  lv_obj_add_event_cb(pd->processDetailNameTextArea, event_keyboard, LV_EVENT_CLICKED, &pd->nameKeyboardCtx);
                  lv_obj_add_event_cb(pd->processDetailNameTextArea, event_keyboard, LV_EVENT_DEFOCUSED, &pd->nameKeyboardCtx);
                  lv_obj_add_event_cb(pd->processDetailNameTextArea, event_keyboard, LV_EVENT_CANCEL, &pd->nameKeyboardCtx);
                  lv_obj_add_event_cb(pd->processDetailNameTextArea, event_keyboard, LV_EVENT_READY, &pd->nameKeyboardCtx);
                  lv_obj_add_state(pd->processDetailNameTextArea, LV_STATE_FOCUSED);
                  lv_obj_set_style_border_opa(pd->processDetailNameTextArea, LV_OPA_TRANSP, 0);
                  lv_textarea_set_max_length(pd->processDetailNameTextArea, MAX_PROC_NAME_LEN);
                  lv_textarea_set_text(pd->processDetailNameTextArea, pd->data.processNameString);


            pd->processDetailStepsLabel = lv_label_create(pd->processDetailContainer);
            lv_label_set_text(pd->processDetailStepsLabel, processDetailStep_text);
            lv_obj_set_width(pd->processDetailStepsLabel, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(pd->processDetailStepsLabel, ui->section_font, 0);
            lv_obj_align(pd->processDetailStepsLabel, LV_ALIGN_TOP_LEFT, ui->steps_label_x, ui->steps_label_y);

            pd->processStepsContainer = lv_obj_create(pd->processDetailContainer);
            lv_obj_align(pd->processStepsContainer, LV_ALIGN_TOP_LEFT, ui->steps_x, ui->steps_y);
            lv_obj_set_size(pd->processStepsContainer, ui->steps_w, ui->steps_h);
            lv_obj_set_scroll_dir(pd->processStepsContainer, LV_DIR_VER);
            lv_obj_set_style_border_color(pd->processStepsContainer, lv_palette_main(LV_PALETTE_GREEN), 0);


            pd->processNewStepButton = lv_button_create(pd->processDetailContainer);
            lv_obj_set_size(pd->processNewStepButton, ui->new_step_btn_w, ui->new_step_btn_h);
            lv_obj_align(pd->processNewStepButton, LV_ALIGN_TOP_LEFT, ui->new_step_x, ui->new_step_y);
            lv_obj_add_event_cb(pd->processNewStepButton, event_processDetail, LV_EVENT_CLICKED, pn);
            lv_obj_set_style_bg_color(pd->processNewStepButton, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);

                    pd->processNewStepLabel = lv_label_create(pd->processNewStepButton);
                    lv_label_set_text(pd->processNewStepLabel, LV_SYMBOL_PLUS);
                    lv_obj_set_style_text_font(pd->processNewStepLabel, ui->value_font, 0);
                    lv_obj_align(pd->processNewStepLabel, LV_ALIGN_CENTER, 0, 0);


            pd->processDetailInfoLabel = lv_label_create(pd->processDetailContainer);
            lv_label_set_text(pd->processDetailInfoLabel, processDetailInfo_text);
            lv_obj_set_width(pd->processDetailInfoLabel, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(pd->processDetailInfoLabel, ui->section_font, 0);
            lv_obj_align(pd->processDetailInfoLabel, LV_ALIGN_TOP_RIGHT, ui->info_label_x, ui->info_label_y);

            pd->processInfoContainer = lv_obj_create(pd->processDetailContainer);
            lv_obj_align(pd->processInfoContainer, LV_ALIGN_TOP_LEFT, ui->info_x, ui->info_y);
            lv_obj_set_size(pd->processInfoContainer, ui->info_w, ui->info_h);
            lv_obj_remove_flag(pd->processInfoContainer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_color(pd->processInfoContainer, lv_color_hex(WHITE), 0);


                  pd->processTempControlContainer = lv_obj_create(pd->processInfoContainer);
                  lv_obj_remove_flag(pd->processTempControlContainer, LV_OBJ_FLAG_SCROLLABLE);
                  lv_obj_align(pd->processTempControlContainer, LV_ALIGN_TOP_LEFT, ui->info_content_x, ui->temp_ctrl_y);
                  lv_obj_set_size(pd->processTempControlContainer, ui->info_w - ui->info_padding, ui->color_container_h);
                  lv_obj_set_style_border_opa(pd->processTempControlContainer, LV_OPA_TRANSP, 0);

                          pd->processTempControlLabel = lv_label_create(pd->processTempControlContainer);
                          lv_label_set_text(pd->processTempControlLabel, processDetailIsTempControl_text);
                          lv_obj_set_style_text_font(pd->processTempControlLabel, ui->label_font, 0);
                          lv_obj_align(pd->processTempControlLabel, LV_ALIGN_LEFT_MID, ui->temp_label_x, ui->form_label_y);

                          pd->processTempControlSwitch = lv_switch_create(pd->processTempControlContainer);
                          lv_obj_set_size(pd->processTempControlSwitch, ui->temp_switch_w, ui->temp_switch_h);
                          lv_obj_add_event_cb(pd->processTempControlSwitch, event_processDetail, LV_EVENT_VALUE_CHANGED, pn);
                          lv_obj_align(pd->processTempControlSwitch, LV_ALIGN_RIGHT_MID, ui->temp_ctrl_switch_x, ui->form_label_y);
                          lv_obj_set_style_bg_color(pd->processTempControlSwitch, lv_palette_darken(LV_PALETTE_GREY, 3), LV_STATE_DEFAULT);
                          lv_obj_set_style_bg_color(pd->processTempControlSwitch,  lv_palette_main(LV_PALETTE_GREEN), LV_PART_KNOB | LV_STATE_DEFAULT);
                          lv_obj_set_style_bg_color(pd->processTempControlSwitch, lv_color_hex(GREEN_DARK) , LV_PART_INDICATOR | LV_STATE_CHECKED);
                          lv_obj_add_state(pd->processTempControlSwitch, pd->data.isTempControlled);

                  pd->processTempContainer = lv_obj_create(pd->processInfoContainer);
                  lv_obj_remove_flag(pd->processTempContainer, LV_OBJ_FLAG_SCROLLABLE);
                  lv_obj_align(pd->processTempContainer, LV_ALIGN_TOP_LEFT, ui->info_content_x, ui->temp_row_y);
                  lv_obj_set_size(pd->processTempContainer, ui->info_w - ui->info_padding, ui->temp_row_h);
                  lv_obj_set_style_border_opa(pd->processTempContainer, LV_OPA_TRANSP, 0);

                          pd->processTempLabel = lv_label_create(pd->processTempContainer);
                          lv_label_set_text(pd->processTempLabel, processDetailTemp_text);
                          lv_obj_set_style_text_font(pd->processTempLabel, ui->label_font, 0);
                          lv_obj_align(pd->processTempLabel, LV_ALIGN_LEFT_MID, ui->temp_label_x, ui->form_label_y);

                          pd->processTempTextArea = lv_textarea_create(pd->processTempContainer);
                          lv_textarea_set_one_line(pd->processTempTextArea, true);
                          lv_textarea_set_placeholder_text(pd->processTempTextArea, processDetailTempPlaceHolder_text);
                          lv_obj_align(pd->processTempTextArea, LV_ALIGN_LEFT_MID, ui->temp_textarea_x, ui->form_label_y);
                          lv_obj_set_width(pd->processTempTextArea, ui->temp_textarea_w);
                          lv_obj_add_event_cb(pd->processTempTextArea, event_processDetail, LV_EVENT_CLICKED, pn);
                          lv_obj_add_event_cb(pd->processTempTextArea, event_processDetail, LV_EVENT_REFRESH, pn);
                          lv_obj_add_event_cb(pd->processTempTextArea, event_processDetail, LV_EVENT_VALUE_CHANGED, pn);
                          lv_obj_add_event_cb(pd->processTempTextArea, event_processDetail, LV_EVENT_FOCUSED, pn);
                          lv_obj_set_style_border_opa(pd->processTempTextArea, LV_OPA_TRANSP, LV_PART_CURSOR);
                          lv_obj_set_style_bg_color(pd->processTempTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
                          lv_obj_set_style_text_align(pd->processTempTextArea , LV_TEXT_ALIGN_CENTER, 0);
                          lv_style_set_text_font(&pd->textAreaStyle, ui->text_area_font);
                          lv_obj_add_style(pd->processTempTextArea, &pd->textAreaStyle, LV_PART_MAIN);
                          lv_obj_set_style_border_color(pd->processTempTextArea, lv_color_hex(WHITE), 0);


                          pd->processTempUnitLabel = lv_label_create(pd->processTempContainer);
                          if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
                              lv_label_set_text(pd->processTempUnitLabel, celsius_text);
                              sprintf(formatted_string, "%"PRIi32"", pd->data.temp);
                              lv_textarea_set_text(pd->processTempTextArea, formatted_string);
                          } else{
                              lv_label_set_text(pd->processTempUnitLabel, fahrenheit_text);
                              sprintf(formatted_string, "%"PRIi32"", convertCelsiusToFahrenheit(pd->data.temp));
                              lv_textarea_set_text(pd->processTempTextArea, formatted_string);
                          }

                          lv_obj_set_style_text_font(pd->processTempUnitLabel, ui->value_font, 0);
                          lv_obj_align(pd->processTempUnitLabel, LV_ALIGN_LEFT_MID, ui->temp_unit_x, ui->form_label_y);


                  pd->processToleranceContainer = lv_obj_create(pd->processInfoContainer);
                  lv_obj_remove_flag(pd->processToleranceContainer, LV_OBJ_FLAG_SCROLLABLE);
                  lv_obj_align(pd->processToleranceContainer, LV_ALIGN_TOP_LEFT, ui->info_content_x, ui->tolerance_row_y);
                  lv_obj_set_size(pd->processToleranceContainer, ui->info_w - ui->info_padding, ui->temp_row_h);
                  lv_obj_set_style_border_opa(pd->processToleranceContainer, LV_OPA_TRANSP, 0);

                          pd->processToleranceLabel = lv_label_create(pd->processToleranceContainer);
                          lv_label_set_text(pd->processToleranceLabel, processDetailTempTolerance_text);
                          lv_obj_set_style_text_font(pd->processToleranceLabel, ui->label_font, 0);
                          lv_obj_align(pd->processToleranceLabel, LV_ALIGN_LEFT_MID, ui->temp_label_x, ui->form_label_y);

                          pd->processToleranceTextArea = lv_textarea_create(pd->processToleranceContainer);
                          lv_textarea_set_one_line(pd->processToleranceTextArea, true);
                          lv_textarea_set_placeholder_text(pd->processToleranceTextArea, processDetailTempPlaceHolder_text);
                          lv_obj_align(pd->processToleranceTextArea, LV_ALIGN_LEFT_MID, ui->temp_textarea_x, ui->form_label_y);
                          lv_obj_set_width(pd->processToleranceTextArea, ui->temp_textarea_w);
                          lv_obj_set_style_border_opa(pd->processToleranceTextArea, LV_OPA_TRANSP, LV_PART_CURSOR);



                          lv_obj_add_event_cb(pd->processToleranceTextArea, event_processDetail, LV_EVENT_CLICKED, pn);
                          lv_obj_add_event_cb(pd->processToleranceTextArea, event_processDetail, LV_EVENT_REFRESH, pn);
                          lv_obj_add_event_cb(pd->processToleranceTextArea, event_processDetail, LV_EVENT_VALUE_CHANGED, pn);
                          lv_obj_add_event_cb(pd->processToleranceTextArea, event_processDetail, LV_EVENT_FOCUSED, pn);
                          lv_obj_set_style_bg_color(pd->processToleranceTextArea, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
                          lv_obj_set_style_text_align(pd->processToleranceTextArea , LV_TEXT_ALIGN_CENTER, 0);
                          lv_style_set_text_font(&pd->textAreaStyle, ui->text_area_font);
                          lv_obj_add_style(pd->processToleranceTextArea, &pd->textAreaStyle, LV_PART_MAIN);
                          lv_obj_set_style_border_color(pd->processToleranceTextArea, lv_color_hex(WHITE), 0);


                          pd->processTempUnitLabel = lv_label_create(pd->processToleranceContainer);
                          lv_label_set_text(pd->processTempUnitLabel, celsius_text);
                          lv_obj_set_style_text_font(pd->processTempUnitLabel, ui->value_font, 0);
                          lv_obj_align(pd->processTempUnitLabel, LV_ALIGN_LEFT_MID, ui->temp_unit_x, ui->form_label_y);

                          if(gui.page.settings.settingsParams.tempUnit == CELSIUS_TEMP){
                              lv_label_set_text(pd->processTempUnitLabel, celsius_text);
                          } else{
                              lv_label_set_text(pd->processTempUnitLabel, fahrenheit_text);
                          }
                          snprintf( tempBuffer, sizeof(tempBuffer), "%.1f", pd->data.tempTolerance);
                          lv_textarea_set_text(pd->processToleranceTextArea, tempBuffer);

                  /* Disable temperature and tolerance fields if process is not temp-controlled */
                  if (!pd->data.isTempControlled) {
                      lv_obj_add_state(pd->processTempTextArea, LV_STATE_DISABLED);
                      lv_obj_add_state(pd->processToleranceTextArea, LV_STATE_DISABLED);
                  }

                  pd->processTotalTimeContainer = lv_obj_create(pd->processInfoContainer);
                  lv_obj_remove_flag(pd->processTotalTimeContainer, LV_OBJ_FLAG_SCROLLABLE);
                  lv_obj_align(pd->processTotalTimeContainer, LV_ALIGN_TOP_LEFT, ui->info_content_x, ui->total_time_y);
                  lv_obj_set_size(pd->processTotalTimeContainer, ui->info_w - ui->info_padding, ui->total_time_h);
                  lv_obj_set_style_border_opa(pd->processTotalTimeContainer, LV_OPA_TRANSP, 0);

                          pd->processTotalTimeLabel = lv_label_create(pd->processTotalTimeContainer);
                          lv_label_set_text(pd->processTotalTimeLabel, processDetailTotalTime_text);
                          lv_obj_set_style_text_font(pd->processTotalTimeLabel, ui->label_font, 0);
                          lv_obj_align(pd->processTotalTimeLabel, LV_ALIGN_LEFT_MID, ui->temp_label_x, ui->form_label_y);

                          pd->processTotalTimeValue = lv_label_create(pd->processTotalTimeContainer);
                          lv_obj_set_style_text_font(pd->processTotalTimeValue, ui->value_font, 0);
                          lv_obj_align(pd->processTotalTimeValue, LV_ALIGN_LEFT_MID, ui->temp_textarea_x, ui->form_label_y);
                          lv_label_set_text_fmt(pd->processTotalTimeValue, "%"PRIu32"m%"PRIu8"s",
                            pd->data.timeMins, pd->data.timeSecs);


                  pd->processColorOrBnWContainer = lv_obj_create(pd->processInfoContainer);
                  lv_obj_remove_flag(pd->processColorOrBnWContainer, LV_OBJ_FLAG_SCROLLABLE);
                  lv_obj_align(pd->processColorOrBnWContainer, LV_ALIGN_TOP_LEFT, ui->color_container_x, ui->color_container_y);
                  lv_obj_set_size(pd->processColorOrBnWContainer, ui->color_container_w, ui->color_container_h);
                  lv_obj_set_style_border_color(pd->processColorOrBnWContainer, lv_color_hex(WHITE), 0);


                          pd->processColorLabel = lv_label_create(pd->processColorOrBnWContainer);
                          lv_label_set_text(pd->processColorLabel, colorpalette_icon);
                          lv_obj_set_style_text_font(pd->processColorLabel, ui->film_icon_font, 0);
                          lv_obj_align(pd->processColorLabel, LV_ALIGN_LEFT_MID, ui->color_label_x, ui->form_label_y);
                          lv_obj_add_flag(pd->processColorLabel, LV_OBJ_FLAG_CLICKABLE);
                          lv_obj_add_event_cb(pd->processColorLabel, event_processDetail, LV_EVENT_CLICKED, pn);


                          pd->processBnWLabel = lv_label_create(pd->processColorOrBnWContainer);
                          lv_label_set_text(pd->processBnWLabel, blackwhite_icon);
                          lv_obj_set_style_text_font(pd->processBnWLabel, ui->film_icon_font, 0);
                          lv_obj_align(pd->processBnWLabel, LV_ALIGN_LEFT_MID, ui->bw_icon_x, ui->form_label_y);
                          lv_obj_add_flag(pd->processBnWLabel, LV_OBJ_FLAG_CLICKABLE);
                          lv_obj_add_event_cb(pd->processBnWLabel, event_processDetail, LV_EVENT_CLICKED, pn);
                          if(pd->data.filmType == COLOR_FILM){
                              lv_obj_set_style_text_color(pd->processColorLabel, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
                              lv_obj_set_style_text_color(pd->processBnWLabel, lv_color_hex(WHITE), LV_PART_MAIN);
                          }
                          else if(pd->data.filmType == BLACK_AND_WHITE_FILM){
                              lv_obj_set_style_text_color(pd->processBnWLabel, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
                              lv_obj_set_style_text_color(pd->processColorLabel, lv_color_hex(WHITE), LV_PART_MAIN);
                          }
                          else if(pd->data.filmType == FILM_TYPE_NA){
                              lv_obj_set_style_text_color(pd->processBnWLabel, lv_color_hex(WHITE), LV_PART_MAIN);
                              lv_obj_set_style_text_color(pd->processColorLabel, lv_color_hex(WHITE), LV_PART_MAIN);
                          }

                  pd->processPreferredLabel = lv_label_create(pd->processInfoContainer);
                  lv_label_set_text(pd->processPreferredLabel, preferred_icon);
                  lv_obj_set_style_text_font(pd->processPreferredLabel, ui->film_icon_font, 0);
                  lv_obj_align(pd->processPreferredLabel, LV_ALIGN_TOP_LEFT, ui->preferred_x, ui->preferred_y);
                  lv_obj_add_flag(pd->processPreferredLabel, LV_OBJ_FLAG_CLICKABLE);
                  lv_obj_add_event_cb(pd->processPreferredLabel, event_processDetail, LV_EVENT_CLICKED, pn);
                  if(pd->data.isPreferred == 0){
                    lv_obj_set_style_text_color(pd->processPreferredLabel, lv_color_hex(WHITE), LV_PART_MAIN);
                  }
                  else{
                    lv_obj_set_style_text_color(pd->processPreferredLabel, lv_color_hex(RED), LV_PART_MAIN);
                  }

                  pd->processSaveButton = lv_button_create(pd->processDetailContainer);
  pd->nameKeyboardCtx.saveButton = pd->processSaveButton;
                  lv_obj_set_size(pd->processSaveButton, ui->save_w, ui->save_h);
                  lv_obj_align(pd->processSaveButton, LV_ALIGN_BOTTOM_RIGHT, ui->save_x, ui->save_y);
                  lv_obj_add_event_cb(pd->processSaveButton, event_processDetail, LV_EVENT_REFRESH, pn);
                  lv_obj_add_event_cb(pd->processSaveButton, event_processDetail, LV_EVENT_CLICKED, pn);
                  lv_obj_set_style_bg_color(pd->processSaveButton, lv_color_hex(BLUE_DARK), LV_PART_MAIN);
                  lv_obj_add_state(pd->processSaveButton, LV_STATE_DISABLED);


                          pd->processSaveLabel = lv_label_create(pd->processSaveButton);
                          lv_label_set_text(pd->processSaveLabel, save_icon);
                          lv_obj_set_style_text_font(pd->processSaveLabel, ui->button_icon_font, 0);
                          lv_obj_align(pd->processSaveLabel, LV_ALIGN_CENTER, 0, 0);


                  pd->processRunButton = lv_button_create(pd->processDetailContainer);
                  lv_obj_set_size(pd->processRunButton, ui->run_w, ui->run_h);
                  lv_obj_align(pd->processRunButton, LV_ALIGN_BOTTOM_RIGHT, ui->run_x, ui->run_y);
                  lv_obj_add_event_cb(pd->processRunButton, event_processDetail, LV_EVENT_CLICKED, pn);
                  lv_obj_set_style_bg_color(pd->processRunButton, lv_color_hex(GREEN_DARK), LV_PART_MAIN);
                  if(pd->stepElementsList.size == 0)
                      lv_obj_add_state(pd->processRunButton, LV_STATE_DISABLED);
                  else
                      lv_obj_clear_state(pd->processRunButton, LV_STATE_DISABLED);

                          pd->processRunLabel = lv_label_create(pd->processRunButton);
                          lv_label_set_text(pd->processRunLabel, play_icon);
                          lv_obj_set_style_text_font(pd->processRunLabel, ui->button_icon_font, 0);
                          lv_obj_align(pd->processRunLabel, LV_ALIGN_CENTER, 0, 0);
                          lv_obj_remove_flag(pd->processRunLabel, LV_OBJ_FLAG_CLICKABLE);
                          lv_obj_add_flag(pd->processRunLabel, LV_OBJ_FLAG_EVENT_BUBBLE);


              if(pd->stepElementsList.size > 0){
                stepNode *currentNode = pd->stepElementsList.start;
                uint8_t tempSize = 1;
                while(currentNode != NULL){
                  LV_LOG_USER("Adding to process with address 0x%p n:%d steps", pn,pd->stepElementsList.size);
                  stepElementCreate(currentNode, pn, tempSize);
                  currentNode = currentNode->next;
                  tempSize ++;
                }
            }
}
