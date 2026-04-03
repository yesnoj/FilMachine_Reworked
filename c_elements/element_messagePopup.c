/**
 * @file element_messagePopup.c
 *
 */
#include <string.h>
#include "FilMachine.h"

extern struct gui_components gui;

/* Keep popup context local to this module so button handlers do not need to
 * infer the owner from gui.tempProcessNode / gui.tempStepNode. */
typedef enum {
    MSGPOP_OWNER_NONE = 0,
    MSGPOP_OWNER_INFO,
    MSGPOP_OWNER_PROCESS,
    MSGPOP_OWNER_STEP,
    MSGPOP_OWNER_CHECKUP_STOP_NOW,
    MSGPOP_OWNER_CHECKUP_STOP_AFTER,
    MSGPOP_OWNER_DELETE_ALL,
    MSGPOP_OWNER_IMPORT,
    MSGPOP_OWNER_EXPORT,
    MSGPOP_OWNER_OTA_SD,
    MSGPOP_OWNER_WIFI_FORGET,
    MSGPOP_OWNER_PROCESS_DISCARD,
} message_popup_owner_t;

typedef struct {
    message_popup_owner_t type;
    void        *whoCallMe;
    processNode *process;
    stepNode    *step;
    sCheckup    *checkup;
    lv_obj_t    *triggerObj;
} message_popup_ctx_t;

static message_popup_ctx_t g_msg_ctx = {0};

/* Sentinel address used by process_detail to trigger the discard popup */
static char s_process_discard_sentinel;

static void message_popup_ctx_clear(void)
{
    memset(&g_msg_ctx, 0, sizeof(g_msg_ctx));
}

static processNode *message_popup_find_process_node(void *ptr)
{
    processNode *current = gui.page.processes.processElementsList.start;
    while (current != NULL) {
        if ((void *)current == ptr) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static bool message_popup_find_step_in_process(void *ptr, processNode *process, processNode **out_process, stepNode **out_step)
{
    if (process == NULL || process->process.processDetails == NULL)
        return false;

    stepNode *step = process->process.processDetails->stepElementsList.start;
    while (step != NULL) {
        if ((void *)step == ptr) {
            if (out_process) *out_process = process;
            if (out_step) *out_step = step;
            return true;
        }
        step = step->next;
    }
    return false;
}

static bool message_popup_find_step_owner(void *ptr, processNode **out_process, stepNode **out_step)
{
    /* First search the saved process list */
    processNode *process = gui.page.processes.processElementsList.start;
    while (process != NULL) {
        if (message_popup_find_step_in_process(ptr, process, out_process, out_step))
            return true;
        process = process->next;
    }

    /* Also check the currently-edited (possibly unsaved) process */
    if (gui.tempProcessNode != NULL) {
        if (message_popup_find_step_in_process(ptr, gui.tempProcessNode, out_process, out_step))
            return true;
    }

    return false;
}

static bool message_popup_find_checkup_owner(void *ptr, processNode **out_process, sCheckup **out_checkup, bool *out_stop_now)
{
    processNode *process = gui.page.processes.processElementsList.start;

    while (process != NULL) {
        if (process->process.processDetails != NULL && process->process.processDetails->checkup != NULL) {
            sCheckup *ckup = process->process.processDetails->checkup;
            if ((void *)ckup->checkupStopNowButton == ptr) {
                if (out_process) *out_process = process;
                if (out_checkup) *out_checkup = ckup;
                if (out_stop_now) *out_stop_now = true;
                return true;
            }
            if ((void *)ckup->checkupStopAfterButton == ptr) {
                if (out_process) *out_process = process;
                if (out_checkup) *out_checkup = ckup;
                if (out_stop_now) *out_stop_now = false;
                return true;
            }
        }
        process = process->next;
    }

    return false;
}

static void message_popup_prepare_ctx(void *whoCallMe)
{
    processNode *process = NULL;
    stepNode *step = NULL;
    sCheckup *checkup = NULL;
    bool stop_now = false;

    message_popup_ctx_clear();
    g_msg_ctx.whoCallMe = whoCallMe;
    g_msg_ctx.triggerObj = (lv_obj_t *)whoCallMe;
    gui.element.messagePopup.whoCallMe = whoCallMe;

    if (whoCallMe == NULL) {
        g_msg_ctx.type = MSGPOP_OWNER_INFO;
        return;
    }

    if (whoCallMe == &gui) {
        g_msg_ctx.type = MSGPOP_OWNER_DELETE_ALL;
        return;
    }

    if (whoCallMe == gui.page.tools.toolsImportButton) {
        g_msg_ctx.type = MSGPOP_OWNER_IMPORT;
        return;
    }

    if (whoCallMe == gui.page.tools.toolsExportButton) {
        g_msg_ctx.type = MSGPOP_OWNER_EXPORT;
        return;
    }

    if (whoCallMe == gui.page.tools.toolsUpdateSDButton) {
        g_msg_ctx.type = MSGPOP_OWNER_OTA_SD;
        return;
    }

    if (whoCallMe == gui.element.wifiPopup.listContainer) {
        g_msg_ctx.type = MSGPOP_OWNER_WIFI_FORGET;
        return;
    }

    if (whoCallMe == &s_process_discard_sentinel) {
        g_msg_ctx.type = MSGPOP_OWNER_PROCESS_DISCARD;
        g_msg_ctx.process = gui.tempProcessNode;
        return;
    }

    if (message_popup_find_checkup_owner(whoCallMe, &process, &checkup, &stop_now)) {
        g_msg_ctx.process = process;
        g_msg_ctx.checkup = checkup;
        g_msg_ctx.type = stop_now ? MSGPOP_OWNER_CHECKUP_STOP_NOW : MSGPOP_OWNER_CHECKUP_STOP_AFTER;
        LV_LOG_USER("message_popup_prepare_ctx: CHECKUP_%s", stop_now ? "STOP_NOW" : "STOP_AFTER");
        return;
    }

    if (message_popup_find_step_owner(whoCallMe, &process, &step)) {
        g_msg_ctx.process = process;
        g_msg_ctx.step = step;
        g_msg_ctx.type = MSGPOP_OWNER_STEP;
        return;
    }

    process = message_popup_find_process_node(whoCallMe);
    if (process != NULL) {
        g_msg_ctx.process = process;
        g_msg_ctx.type = MSGPOP_OWNER_PROCESS;
        return;
    }

    g_msg_ctx.type = MSGPOP_OWNER_NONE;
    LV_LOG_USER("message_popup_prepare_ctx: OWNER_NONE — whoCallMe=%p not recognized", whoCallMe);
}

static void message_popup_close(lv_obj_t *mboxCont)
{
    lv_style_reset(&gui.element.messagePopup.style_mBoxPopupTitleLine);
    lv_msgbox_close(mboxCont);
    gui.element.messagePopup.mBoxPopupParent = NULL;
    message_popup_ctx_clear();
}

static void message_popup_handle_stop_now(void)
{
    processNode *pn = g_msg_ctx.process;
    sCheckup *ckup = g_msg_ctx.checkup;

    if (pn == NULL || ckup == NULL) {
        return;
    }

    ckup->data.stopNow = true;
    ckup->data.stopAfter = false;
    ckup->data.isDeveloping = false;
    lv_obj_add_state(ckup->checkupStopAfterButton, LV_STATE_DISABLED);
    lv_obj_add_state(ckup->checkupStopNowButton, LV_STATE_DISABLED);

    lv_label_set_text(ckup->checkupProcessCompleteLabel, checkupProcessStopped_text);
    lv_obj_remove_flag(ckup->checkupProcessCompleteLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ckup->checkupStepNameValue, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ckup->checkupStepTimeLeftValue, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ckup->checkupProcessTimeLeftValue, LV_OBJ_FLAG_HIDDEN);

    gui.page.tools.machineStats.stopped++;
    safeTimerDelete(&ckup->processTimer);
    if (ckup->pumpTimer) lv_timer_resume(ckup->pumpTimer);
    qSysAction(SAVE_MACHINE_STATS);
}

static void message_popup_handle_stop_after(void)
{
    sCheckup *ckup = g_msg_ctx.checkup;

    if (ckup == NULL) {
        return;
    }

    ckup->data.stopAfter = true;
    lv_obj_add_state(ckup->checkupStopAfterButton, LV_STATE_DISABLED);
    lv_obj_clear_state(ckup->checkupStopNowButton, LV_STATE_DISABLED);

    gui.page.tools.machineStats.stopped++;
    qSysAction(SAVE_MACHINE_STATS);
}

static void message_popup_button1_clicked(lv_obj_t *mboxCont)
{
    processNode *pn = g_msg_ctx.process;
    stepNode *sn = g_msg_ctx.step;

    switch (g_msg_ctx.type) {
        case MSGPOP_OWNER_PROCESS:
            if (pn != NULL && pn->process.swipedRight == true && pn->process.swipedLeft == false) {
                LV_LOG_USER("Delete process!");

                if (!deleteProcessElement(pn)) {
                    LV_LOG_USER("Delete process element instance at address 0x%p Failed!", pn);
                } else {
                    LV_LOG_USER("Delete process element instance at address 0x%p", pn);
                    qSysAction(SAVE_PROCESS_CONFIG);
                }

                message_popup_close(mboxCont);
            }
            else if (pn != NULL && pn->process.swipedRight == false && pn->process.swipedLeft == true) {
                LV_LOG_USER("Cancel duplicate");
                message_popup_close(mboxCont);
                pn->process.longPressHandled = false;
            }
            break;

        case MSGPOP_OWNER_STEP:
            if (pn != NULL && sn != NULL && sn->step.swipedLeft == false) {
                LV_LOG_USER("Delete step!");
                sn->step.longPressHandled = false;

                if (!deleteStepElement(sn, pn, false)) {
                    LV_LOG_USER("Delete step element instance at address 0x%p Failed!", sn);
                } else {
                    calculateTotalTime(pn);
                    LV_LOG_USER("Delete step element instance at address 0x%p", sn);
                    pn->process.processDetails->data.somethingChanged = true;
                    lv_obj_send_event(pn->process.processDetails->processSaveButton, LV_EVENT_REFRESH, NULL);
                    qSysAction(SAVE_PROCESS_CONFIG);
                }

                message_popup_close(mboxCont);
            }
            else if (sn != NULL && sn->step.swipedLeft == true) {
                LV_LOG_USER("Cancel duplicate step");
                sn->step.swipedLeft = false;
                message_popup_close(mboxCont);
            }
            break;

        case MSGPOP_OWNER_CHECKUP_STOP_NOW:
            LV_LOG_USER("Stop process NOW!");
            message_popup_handle_stop_now();
            message_popup_close(mboxCont);
            break;

        case MSGPOP_OWNER_CHECKUP_STOP_AFTER:
            LV_LOG_USER("Stop process AFTER!");
            message_popup_handle_stop_after();
            message_popup_close(mboxCont);
            break;

        case MSGPOP_OWNER_DELETE_ALL: {
            LV_LOG_USER("Delete ALL PROCESS!");
            message_popup_close(mboxCont);
            lv_obj_clean(gui.page.processes.processesListContainer);
            processList *processElementsList = &(gui.page.processes.processElementsList);
            emptyList((void *)processElementsList, PROCESS_NODE);
            qSysAction(SAVE_PROCESS_CONFIG);
            break;
        }

        case MSGPOP_OWNER_IMPORT:
            LV_LOG_USER("Cancel import from SD");
            message_popup_close(mboxCont);
            break;

        case MSGPOP_OWNER_EXPORT:
            LV_LOG_USER("Cancel export to SD");
            message_popup_close(mboxCont);
            break;

        case MSGPOP_OWNER_OTA_SD:
            LV_LOG_USER("Cancel OTA from SD");
            message_popup_close(mboxCont);
            break;

        case MSGPOP_OWNER_PROCESS_DISCARD:
            /* Cancel = keep editing, just close the popup */
            LV_LOG_USER("Cancel discard — keep editing");
            message_popup_close(mboxCont);
            break;

        case MSGPOP_OWNER_WIFI_FORGET:
            /* Forget WiFi — clear saved credentials */
            gui.page.settings.settingsParams.wifiSSID[0] = '\0';
            gui.page.settings.settingsParams.wifiPassword[0] = '\0';
            gui.page.settings.settingsParams.wifiEnabled = false;
            qSysAction(SAVE_PROCESS_CONFIG);
            /* Disconnect if currently connected */
            if (wifi_is_connected()) {
                wifi_disconnect();
            }
            /* Refresh scan list to remove ✓ indicator */
            wifi_popup_refresh_list();
            LV_LOG_USER("WiFi credentials forgotten");
            message_popup_close(mboxCont);
            break;

        default:
            LV_LOG_USER("button1: unhandled type %d — closing popup", g_msg_ctx.type);
            message_popup_close(mboxCont);
            break;
    }
}

static void message_popup_button2_clicked(lv_obj_t *mboxCont)
{
    processNode *pn = g_msg_ctx.process;
    stepNode *sn = g_msg_ctx.step;
    stepNode *newStep = NULL;

    switch (g_msg_ctx.type) {
        case MSGPOP_OWNER_STEP:
            if (sn != NULL && sn->step.swipedLeft == false && sn->step.swipedRight == true) {
                LV_LOG_USER("Cancel delete step element!");
                uint32_t x = lv_obj_get_x_aligned(sn->step.stepElement) - ui_get_profile()->step_element.swipe_offset;
                uint32_t y = lv_obj_get_y_aligned(sn->step.stepElement);
                lv_obj_set_pos(sn->step.stepElement, x, y);
                sn->step.swipedLeft = false;
                sn->step.swipedRight = false;
                lv_obj_add_flag(sn->step.deleteButton, LV_OBJ_FLAG_HIDDEN);
                sn->step.longPressHandled = false;
            }

            if (pn != NULL && sn != NULL && sn->step.swipedLeft == true && sn->step.swipedRight == false) {
                LV_LOG_USER("Duplicate step!");

                newStep = (stepNode *)allocateAndInitializeNode(STEP_NODE);
                if (newStep == NULL) {
                    LV_LOG_USER("Failed to allocate memory for duplicate step");
                } else {
                    memset(newStep->step.stepDetails, 0, sizeof(sStepDetail));
                    newStep->step.stepDetails->data = sn->step.stepDetails->data;

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

                    addStepElement(newStep, pn);
                    LV_LOG_USER("Duplicate step created at %p, process now has %d steps",
                        newStep, pn->process.processDetails->stepElementsList.size);
                    calculateTotalTime(pn);
                }

                sn->step.swipedLeft = false;
            }

            message_popup_close(mboxCont);

            if (pn != NULL && newStep != NULL && newStep->step.stepElement == NULL) {
                stepElementCreate(newStep, pn, -1);
                reorderStepElements(pn);
                lv_obj_scroll_to_view(newStep->step.stepElement, LV_ANIM_ON);
            }
            break;

        case MSGPOP_OWNER_PROCESS:
            if (pn != NULL && pn->process.swipedRight == true && pn->process.swipedLeft == false) {
                LV_LOG_USER("Cancel delete process element!");
                uint32_t x = lv_obj_get_x_aligned(pn->process.processElement) - ui_get_profile()->process_element.swipe_offset;
                uint32_t y = lv_obj_get_y_aligned(pn->process.processElement);
                lv_obj_set_pos(pn->process.processElement, x, y);
                pn->process.swipedLeft = true;
                pn->process.swipedRight = false;
                lv_obj_add_flag(pn->process.deleteButton, LV_OBJ_FLAG_HIDDEN);
                pn->process.gestureHandled = false;
            }

            if (pn != NULL && pn->process.swipedRight == false && pn->process.swipedLeft == true && pn->process.longPressHandled == true) {
                LV_LOG_USER("Duplicate process");
                char *newProcessName = generateRandomSuffix(pn->process.processDetails->data.processNameString);
                LV_LOG_USER("New name %s", newProcessName);

                struct processNode *duplicatedNode = deepCopyProcessNode(pn);
                if (duplicatedNode == NULL) {
                    fprintf(stderr, "Failed to allocate memory for duplicatedNode\n");
                    /* newProcessName is a static buffer — do NOT free */
                    message_popup_close(mboxCont);
                    break;
                }

                snprintf(duplicatedNode->process.processDetails->data.processNameString,
                         sizeof(duplicatedNode->process.processDetails->data.processNameString),
                         "%s", newProcessName);

                duplicatedNode->process.longPressHandled = false;
                pn->process.longPressHandled = false;
                /* newProcessName is a static buffer from generateRandomSuffix — do NOT free */

                /* Close popup FIRST — so the modal overlay is gone before
                   we create GUI elements underneath (LVGL memory reuse) */
                message_popup_close(mboxCont);

                if (addProcessElement(duplicatedNode) != NULL) {
                    LV_LOG_USER("Create GUI entry");
                    processElementCreate(duplicatedNode, -1);
                    lv_obj_scroll_to_view(duplicatedNode->process.processElement, LV_ANIM_ON);
                    qSysAction(SAVE_PROCESS_CONFIG);
                }
            } else {
                message_popup_close(mboxCont);
            }
            break;

        case MSGPOP_OWNER_CHECKUP_STOP_NOW:
        case MSGPOP_OWNER_CHECKUP_STOP_AFTER:
            message_popup_close(mboxCont);
            break;

        case MSGPOP_OWNER_IMPORT:
            message_popup_close(mboxCont);
            LV_LOG_USER("Import process from SD");
            qSysAction(RELOAD_CFG);
            break;

        case MSGPOP_OWNER_EXPORT:
            message_popup_close(mboxCont);
            LV_LOG_USER("Export process to SD");
            qSysAction(EXPORT_CFG);
            break;

        case MSGPOP_OWNER_DELETE_ALL:
            LV_LOG_USER("Cancel delete all process!");
            message_popup_close(mboxCont);
            break;

        case MSGPOP_OWNER_OTA_SD:
            message_popup_close(mboxCont);
            LV_LOG_USER("Confirmed OTA from SD — starting update");
            ota_start_sd();
            otaProgressPopupCreate(otaUpdateFromSD_text);
            break;

        case MSGPOP_OWNER_WIFI_FORGET:
            /* Cancel forget — just close */
            message_popup_close(mboxCont);
            break;

        case MSGPOP_OWNER_PROCESS_DISCARD: {
            /* Discard = close process detail without saving */
            LV_LOG_USER("Discard unsaved changes — closing process detail");
            processNode *discard_pn = g_msg_ctx.process;
            message_popup_close(mboxCont);
            if (discard_pn != NULL && discard_pn->process.processDetails != NULL) {
                sProcessDetail *pd = discard_pn->process.processDetails;

                if (isNodeInList((void *)&(gui.page.processes.processElementsList), discard_pn, PROCESS_NODE) == NULL) {
                    /* New process never saved — destroy UI and free node */
                    lv_style_reset(&pd->textAreaStyle);
                    lv_obj_t *parent = pd->processDetailParent;
                    pd->processTotalTimeValue = NULL;
                    pd->processDetailParent   = NULL;
                    lv_obj_delete(parent);
                    process_node_destroy(discard_pn);
                    gui.tempProcessNode = NULL;
                } else {
                    /* Existing process — restore data + steps from deep copy backup */
                    processNode *backup = getProcessDetailBackup();
                    if (backup != NULL && backup->process.processDetails != NULL) {
                        sProcessDetail *bpd = backup->process.processDetails;

                        /* 1. Restore business data */
                        pd->data = bpd->data;
                        pd->data.somethingChanged = false;

                        /* 2. Destroy current (modified) steps */
                        stepNode *s = pd->stepElementsList.start;
                        while (s != NULL) {
                            stepNode *next = s->next;
                            step_node_destroy(s);
                            s = next;
                        }

                        /* 3. Move original steps from backup → live process */
                        pd->stepElementsList = bpd->stepElementsList;
                        bpd->stepElementsList.start = NULL;
                        bpd->stepElementsList.end   = NULL;
                        bpd->stepElementsList.size  = 0;

                        /* 4. Restore checkup data if applicable */
                        if (pd->checkup != NULL && bpd->checkup != NULL) {
                            pd->checkup->data = bpd->checkup->data;
                        }

                        LV_LOG_USER("Restored process data + %d steps from backup",
                                    pd->stepElementsList.size);
                    } else {
                        pd->data.somethingChanged = false;
                        LV_LOG_USER("No backup available — closing without full restore");
                    }

                    /* Recalculate total time and update list element */
                    calculateTotalTime(discard_pn);
                    updateProcessElement(discard_pn);

                    /* Close detail UI */
                    lv_style_reset(&pd->textAreaStyle);
                    lv_obj_t *parent = pd->processDetailParent;
                    pd->processTotalTimeValue = NULL;
                    pd->processDetailParent   = NULL;
                    lv_obj_delete(parent);
                }

                /* Free backup node (steps already transferred, only shell remains) */
                processNode *bk = getProcessDetailBackup();
                if (bk != NULL) {
                    clearProcessDetailBackup();   /* detach pointer */
                    process_node_destroy(bk);     /* free the shell */
                }
            }
            lv_scr_load(gui.page.menu.screen_mainMenu);
            break;
        }

        default:
            LV_LOG_USER("button2: unhandled type %d — closing popup", g_msg_ctx.type);
            message_popup_close(mboxCont);
            break;
    }
}

void event_messagePopup(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *objCont = (lv_obj_t *)lv_obj_get_parent(obj);
    lv_obj_t *mboxCont = (lv_obj_t *)lv_obj_get_parent(objCont);

    if (code != LV_EVENT_CLICKED) {
        return;
    }

    if (obj == gui.element.messagePopup.mBoxPopupButtonClose) {
        LV_LOG_USER("Pressed gui.element.messagePopup.mBoxPopupButtonClose");
        message_popup_close(mboxCont);
        return;
    }

    if (obj == gui.element.messagePopup.mBoxPopupButton1) {
        LV_LOG_USER("Pressed gui.element.messagePopup.mBoxPopupButton1");
        message_popup_button1_clicked(mboxCont);
        return;
    }

    if (obj == gui.element.messagePopup.mBoxPopupButton2) {
        LV_LOG_USER("Pressed gui.element.messagePopup.mBoxPopupButton2");
        message_popup_button2_clicked(mboxCont);
        return;
    }
}

void messagePopupCreate(const char * popupTitleText,const char * popupText,const char * textButton1, const char * textButton2, void * whoCallMe){
  const ui_message_popup_layout_t *ui = &ui_get_profile()->message_popup;
  /*********************
  *    PAGE HEADER
  *********************/
   message_popup_prepare_ctx(whoCallMe);

   /* Clear stale button pointers so they cannot cause false matches
      in event_messagePopup when LVGL reuses memory addresses */
   gui.element.messagePopup.mBoxPopupButtonClose = NULL;
   gui.element.messagePopup.mBoxPopupButton1 = NULL;
   gui.element.messagePopup.mBoxPopupButton2 = NULL;

   LV_LOG_USER("Message popup create");
   createPopupBackdrop(&gui.element.messagePopup.mBoxPopupParent, &gui.element.messagePopup.mBoxPopupContainer, ui_get_profile()->popups.message_w, ui_get_profile()->popups.message_h);

         gui.element.messagePopup.mBoxPopupTitle = lv_label_create(gui.element.messagePopup.mBoxPopupContainer);
         lv_label_set_text(gui.element.messagePopup.mBoxPopupTitle, popupTitleText);
         lv_obj_set_style_text_font(gui.element.messagePopup.mBoxPopupTitle, ui->title_font, 0);
         lv_obj_align(gui.element.messagePopup.mBoxPopupTitle, LV_ALIGN_TOP_MID, ui->title_x, ui->title_y);


   /*Create style*/
   lv_style_init(&gui.element.messagePopup.style_mBoxPopupTitleLine);
   lv_style_set_line_width(&gui.element.messagePopup.style_mBoxPopupTitleLine, ui_get_profile()->title_line_width);
   lv_style_set_line_rounded(&gui.element.messagePopup.style_mBoxPopupTitleLine, true);

   /*Create a line and apply the new style*/
   gui.element.messagePopup.mBoxPopupTitleLine = lv_line_create(gui.element.messagePopup.mBoxPopupContainer);
   lv_line_set_points(gui.element.messagePopup.mBoxPopupTitleLine, gui.element.messagePopup.titleLinePoints, 2);
   lv_obj_add_style(gui.element.messagePopup.mBoxPopupTitleLine, &gui.element.messagePopup.style_mBoxPopupTitleLine, 0);
   lv_obj_align(gui.element.messagePopup.mBoxPopupTitleLine, LV_ALIGN_TOP_MID, ui->title_line_x, ui->title_line_y);


  /*********************
  *    PAGE ELEMENTS
  *********************/

   /* For popups with bottom buttons (No/Yes), shrink the text area so it
      fits between the title line and the buttons.  Info popups (close X
      only) can use the full height. */
   bool has_bottom_buttons = (g_msg_ctx.type != MSGPOP_OWNER_INFO);
   int32_t text_area_h = has_bottom_buttons
       ? (ui->text_container_h - BUTTON_MBOX_HEIGHT)
       : ui->text_container_h;

   gui.element.messagePopup.mBoxPopupTextContainer = lv_obj_create(gui.element.messagePopup.mBoxPopupContainer);
   lv_obj_align(gui.element.messagePopup.mBoxPopupTextContainer, LV_ALIGN_TOP_MID, ui->text_container_x, ui->text_container_y);
   lv_obj_set_size(gui.element.messagePopup.mBoxPopupTextContainer, ui->text_container_w, text_area_h);
   lv_obj_set_style_border_opa(gui.element.messagePopup.mBoxPopupTextContainer, LV_OPA_TRANSP, 0);
   lv_obj_set_style_pad_all(gui.element.messagePopup.mBoxPopupTextContainer, 0, 0);
   lv_obj_set_scroll_dir(gui.element.messagePopup.mBoxPopupTextContainer, LV_DIR_VER);
   lv_obj_clear_flag(gui.element.messagePopup.mBoxPopupTextContainer, LV_OBJ_FLAG_SCROLL_ELASTIC);
   lv_obj_clear_flag(gui.element.messagePopup.mBoxPopupTextContainer, LV_OBJ_FLAG_SCROLL_MOMENTUM);
   lv_obj_add_flag(gui.element.messagePopup.mBoxPopupTextContainer, LV_OBJ_FLAG_SCROLLABLE);

         gui.element.messagePopup.mBoxPopupText = lv_label_create(gui.element.messagePopup.mBoxPopupTextContainer);
         lv_label_set_text(gui.element.messagePopup.mBoxPopupText, popupText);
         lv_obj_set_style_text_font(gui.element.messagePopup.mBoxPopupText, ui->text_font, 0);
         lv_obj_set_width(gui.element.messagePopup.mBoxPopupText, ui->text_w);
         lv_label_set_long_mode(gui.element.messagePopup.mBoxPopupText, LV_LABEL_LONG_WRAP);
         lv_obj_set_style_text_align(gui.element.messagePopup.mBoxPopupText , LV_TEXT_ALIGN_CENTER, 0);
         lv_obj_update_layout(gui.element.messagePopup.mBoxPopupText);
         int32_t text_h = lv_obj_get_height(gui.element.messagePopup.mBoxPopupText);
         if (text_h <= text_area_h) {
             /* Text fits — vertically center it within the container */
             lv_obj_align(gui.element.messagePopup.mBoxPopupText, LV_ALIGN_CENTER, 0, 0);
             lv_obj_remove_flag(gui.element.messagePopup.mBoxPopupTextContainer, LV_OBJ_FLAG_SCROLLABLE);
         } else {
             /* Text overflows — top-align and let the container scroll */
             lv_obj_align(gui.element.messagePopup.mBoxPopupText, LV_ALIGN_TOP_MID, 0, 0);
         }

  if(g_msg_ctx.type == MSGPOP_OWNER_INFO){
      gui.element.messagePopup.mBoxPopupButtonClose = lv_button_create(gui.element.messagePopup.mBoxPopupContainer);
      lv_obj_set_size(gui.element.messagePopup.mBoxPopupButtonClose, BUTTON_POPUP_CLOSE_WIDTH, BUTTON_POPUP_CLOSE_HEIGHT);
      lv_obj_align(gui.element.messagePopup.mBoxPopupButtonClose, LV_ALIGN_TOP_RIGHT, ui->close_btn_x , ui->close_btn_y);
      lv_obj_add_event_cb(gui.element.messagePopup.mBoxPopupButtonClose, event_messagePopup, LV_EVENT_CLICKED, NULL);

            gui.element.messagePopup.mBoxPopupButtonLabel = lv_label_create(gui.element.messagePopup.mBoxPopupButtonClose);
            lv_label_set_text(gui.element.messagePopup.mBoxPopupButtonLabel, closePopup_icon);
            lv_obj_set_style_text_font(gui.element.messagePopup.mBoxPopupButtonLabel, ui->close_icon_font, 0);
            lv_obj_align(gui.element.messagePopup.mBoxPopupButtonLabel, LV_ALIGN_CENTER, 0, 0);
  }
  else{
      gui.element.messagePopup.mBoxPopupButton1 = lv_button_create(gui.element.messagePopup.mBoxPopupContainer);
      lv_obj_set_size(gui.element.messagePopup.mBoxPopupButton1, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
      lv_obj_align(gui.element.messagePopup.mBoxPopupButton1, LV_ALIGN_BOTTOM_LEFT, ui->secondary_btn_x , ui->action_btn_y);
      lv_obj_add_event_cb(gui.element.messagePopup.mBoxPopupButton1, event_messagePopup, LV_EVENT_CLICKED, NULL);
      lv_obj_set_style_bg_color(gui.element.messagePopup.mBoxPopupButton1, lv_color_hex(RED_DARK), LV_PART_MAIN);

          gui.element.messagePopup.mBoxPopupButton1Label = lv_label_create(gui.element.messagePopup.mBoxPopupButton1);
          lv_label_set_text(gui.element.messagePopup.mBoxPopupButton1Label, textButton1);
          lv_obj_set_style_text_font(gui.element.messagePopup.mBoxPopupButton1Label, ui->button_font, 0);
          lv_obj_align(gui.element.messagePopup.mBoxPopupButton1Label, LV_ALIGN_CENTER, 0, 0);


      gui.element.messagePopup.mBoxPopupButton2 = lv_button_create(gui.element.messagePopup.mBoxPopupContainer);
      lv_obj_set_size(gui.element.messagePopup.mBoxPopupButton2, BUTTON_MBOX_WIDTH, BUTTON_MBOX_HEIGHT);
      lv_obj_align(gui.element.messagePopup.mBoxPopupButton2, LV_ALIGN_BOTTOM_RIGHT, - ui->primary_btn_x , ui->action_btn_y);
      lv_obj_add_event_cb(gui.element.messagePopup.mBoxPopupButton2, event_messagePopup, LV_EVENT_CLICKED, NULL);
      lv_obj_set_style_bg_color(gui.element.messagePopup.mBoxPopupButton2, lv_color_hex(GREEN_DARK), LV_PART_MAIN);

            gui.element.messagePopup.mBoxPopupButton2Label = lv_label_create(gui.element.messagePopup.mBoxPopupButton2);
            lv_label_set_text(gui.element.messagePopup.mBoxPopupButton2Label, textButton2);
            lv_obj_set_style_text_font(gui.element.messagePopup.mBoxPopupButton2Label, ui->button_font, 0);
            lv_obj_align(gui.element.messagePopup.mBoxPopupButton2Label, LV_ALIGN_CENTER, 0, 0);
  }
}

void *getProcessDiscardSentinel(void) {
    return &s_process_discard_sentinel;
}
