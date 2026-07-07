#!/usr/bin/env python3
"""
gen_lang.py — i18n generator for FilMachine
============================================
Reads the `#define xxx_text "..."` / `#define xxxList "..."` string macros in
main/include/FilMachine.h and:

  1. generates main/include/lang.h  (string-ID enum + tr()/lang_set() API)
  2. generates main/lang.c          (EN/IT string table)
  3. rewrites FilMachine.h so every macro expands to tr(STR_<name>)

The Italian translations live in the IT dict below (ASCII only — accented
vowels are written with a trailing apostrophe because the LVGL fonts are
converted with --range 0x20-0x7F).

Idempotent: macros already pointing at tr() are left alone, but their table
entry is preserved via the EN dict snapshot stored in lang.c itself.
Run from the repo root:  python3 scripts/gen_lang.py
"""
import re, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HDR  = os.path.join(ROOT, "main/include/FilMachine.h")
LANG_H = os.path.join(ROOT, "main/include/lang.h")
LANG_C = os.path.join(ROOT, "main/lang.c")

# Macros that must stay plain compile-time literals (version/serial aliases,
# values also used where a constant literal is required).
SKIP = {
    "softwareVersionValue_text",
    "softwareSerialNumValue_text",
    "softwareCreditsValue_text",
}

# ── Italian translations ─────────────────────────────────────────────
# Keys = macro name. Values = C-literal content (\n stays as backslash-n).
# Missing key → English is used for both languages (reported at the end).
IT = {
 "initSDError_text": r"ERRORE DI INIZIALIZZAZIONE!\nGUASTO SD-CARD!\nSISTEMA LA SD-CARD!\nPOI TOCCA L'ICONA PER RIAVVIARE!",
 "initI2CError_text": r"ERRORE DI INIZIALIZZAZIONE!\nGUASTO MODULO I2C\nRISOLVI IL PROBLEMA!\nPOI TOCCA L'ICONA PER RIAVVIARE!",
 "initWIREError_text": r"ERRORE DI INIZIALIZZAZIONE!\nGUASTO BUS ONEWIRE\nRISOLVI IL PROBLEMA!\nPOI TOCCA L'ICONA PER RIAVVIARE!",
 "splashTitle_text": r"FILMACHINE",
 "splashSubtitle_text": r"Sviluppo digitale pellicole",
 "splashPopupTitle_text": r"Schermata di avvio",
 "splashPopupUseDefault_text": r"Usa predefinita",
 "splashPopupRandom_text": r"Casuale al riavvio",
 "splashPopupPalette_text": r"Palette",
 "splashPopupShapeStyle_text": r"Stile forme",
 "splashPopupComplexity_text": r"Complessita'",
 "settingsSplashScreen_text": r"Schermata di avvio",
 "settingsWifi_text": r"Wi-Fi",
 "settingsReset_text": r"Ripristina predefiniti",
 "settingsResetPopupTitle_text": r"Reset impostazioni",
 "settingsResetPopupBody_text": r"Tutte le impostazioni ripristinate\nai valori di fabbrica.",
 "settingsResetConfirmTitle_text": r"Ripristinare le impostazioni?",
 "settingsResetConfirmBody_text": r"Ripristinare TUTTE le impostazioni\nai valori di fabbrica?",
 "buttonOk_text": r"OK",
 "wifiPopupTitle_text": r"Wi-Fi",
 "wifiScan_text": r"Cerca",
 "wifiConnect_text": r"Connetti",
 "wifiDisconnect_text": r"Disconnetti",
 "wifiConnected_text": r"Connesso a:",
 "wifiDisconnected_text": r"Non connesso",
 "wifiConnecting_text": r"Connessione...",
 "wifiScanning_text": r"Ricerca...",
 "wifiAutoConnect_text": r"Auto-connect",
 "wifiEnterPassword_text": r"Inserisci password",
 "wifiNoNetworks_text": r"Nessuna rete trovata",
 "wifiErrorTitle_text": r"Errore Wi-Fi",
 "wifiErrAuthFailed_text": r"Autenticazione fallita\n(password errata)",
 "wifiErrHandshakeTimeout_text": r"Timeout handshake\n(password errata o WPA3 non compatibile)",
 "wifiErrMicFailure_text": r"Errore MIC\n(password errata)",
 "wifiErrGroupKeyTimeout_text": r"Timeout handshake\n(chiave di gruppo)",
 "wifiErrApNotFound_text": r"Rete non trovata\n(AP_NOT_FOUND)",
 "wifiErrApNotFoundGeneric_text": r"Rete non trovata",
 "wifiErrAuthExpired_text": r"Autenticazione scaduta\n(AUTH_EXPIRE)",
 "wifiErrClass2Frame_text": r"Frame classe 2 da\nstazione non autenticata",
 "wifiErrClass3Frame_text": r"Frame classe 3 da\nstazione non associata",
 "wifiErrConnectionFail_text": r"Connessione fallita\n(CONNECTION_FAIL)",
 "wifiErrBeaconTimeout_text": r"Timeout beacon\n(segnale perso)",
 "wifiErrUnknownFmt_text": r"Connessione fallita\n(codice: %d)",
 "wifiForgetTitle_text": r"Dimentica rete",
 "wifiForgetBody_text": r"Rimuovere le credenziali salvate\nper questa rete?",
 "wifiForgetYes_text": r"Dimentica",
 "wifiForgetNo_text": r"Annulla",
 "checkupEllipsis_text": r"...",
 "discardChangesTitle_text": r"Modifiche non salvate",
 "discardChangesBody_text": r"Chiudere senza salvare?\nTutte le modifiche andranno perse.",
 "discardChangesNo_text": r"Annulla",
 "discardChangesYes_text": r"Scarta",
 "stepSourceFmt_text": r"Da:%s",
 "processModify_text": r"Modifica",
 "buttonFilter_text": r"Filtri",
 "Processes_text": r"Processi",
 "keyboard_placeholder_text": r"Scrivi nome pellicola...",
 "filterPopupTitle_text": r"Filtro lista processi",
 "filterPopupNamePlaceHolder_text": r"Parte del nome da filtrare",
 "filterPopupName_text": r"Nome",
 "filterPopupColor_text": r"Colore",
 "filterPopupBnW_text": r"B/N",
 "filterPopupBoth_text": r"Entrambi",
 "filterPopupPreferred_text": r"Preferiti",
 "filterPopupApplyButton_text": r"Applica",
 "filterPopupResetButton_text": r"Reset",
 "Settings_text": r"Impostazioni macchina",
 "tempUnit_text": r"Unita' di temperatura",
 "tempSensorTuning_text": r"Temperatura calibrata",
 "tuneButton_text": r"TARA",
 "tempAlertMBox_text": r"Assicurati che la temperatura della macchina sia stabile, misura l'aria ambiente, inserisci il valore e premi 'Tara'. Per azzerare, tieni premuto 'Tara'.",
 "soundAlertMBox_text": r"Continua a suonare l'avviso quando un processo sta per terminare o quando la fase di riscaldamento raggiunge la temperatura desiderata.",
 "autostartAlertMBox_text": r"Al raggiungimento della temperatura desiderata, avvia automaticamente il processo a temperatura controllata.",
 "filmRotationSpeedAlertMBox_text": r"Espressa in giri/min, con un intervallo da 20 a 90 giri al minuto.",
 "rotationInverseIntervalAlertMBox_text": r"La durata, regolabile tra 5 e 60 secondi, per cui la pellicola ruota in una direzione prima di invertire il senso.",
 "filmRotationRandomAlertMBox_text": r"Introduce una variazione casuale nei tempi di inversione per uno sviluppo uniforme; es. 10% su un'inversione di 10 secondi produce tempi tra 8 e 10 secondi.",
 "drainFillTimeAlertMBox_text": r"Regola la sovrapposizione dei tempi di riempimento e scarico con i tempi di processo: queste operazioni non possono essere accelerate.",
 "multiRinseTimeAlertMBox_text": r"Imposta la durata di un singolo ciclo di multi-risciacquo (non dell'intero step). Regolabile da 30 secondi a 3 minuti. Cicli piu' lunghi per tank grandi, piu' corti per tank piccoli. 2 minuti e' un buon valore.",
 "waterInletAlertMBox_text": r"Indica alla macchina se e' collegata a una fonte d'acqua in pressione. Se si', la vasca d'acqua verra' riempita automaticamente. Se no, dovrai riempirla manualmente.",
 "rotationSpeed_text": r"Velocita' rotazione",
 "rotationInversionInterval_text": r"Intervallo inversione",
 "rotationRandom_text": r"Variazione casuale",
 "waterInlet_text": r"Ingresso acqua",
 "persistentAlarm_text": r"Allarmi persistenti",
 "autostart_text": r"Avvio automatico",
 "drainFillTime_text": r"Sovrapp. carico/scarico",
 "multiRinseTime_text": r"Durata ciclo multi-risciacquo",
 "lineRinse_text": r"Lavaggio linea dopo chimica",
 "lineRinseTime_text": r"Durata risciacquo linea",
 "lineRinseAlertMBox_text": r"Dopo ogni step di chimica, lava la linea condivisa della pompa con acqua pulita (dalla vasca, scaricata nello scarico) prima di aspirare il liquido successivo. Elimina i residui di chimica dai tubi comuni per evitare contaminazioni incrociate. Consuma acqua extra: controlla il livello della vasca se l'ingresso acqua non e' collegato.",
 "tankSize_text": r"Dimensione tank",
 "tankSizeAlertMBox_text": r"Seleziona la dimensione tank predefinita.\nPiccola, Media o Grande.",
 "pumpSpeed_text": r"Velocita' pompa",
 "pumpSpeedAlertMBox_text": r"Imposta la velocita' della pompa in percentuale.\nValori alti = carico/scarico piu' rapidi.",
 "speedTestSwitch_text": r"Test",
 "speedSetPopupTitle_text": r"Imposta velocita'",
 "volume_text": r"Volume",
 "volumeAlertMBox_text": r"Imposta il volume dell'altoparlante.\nIl tono suona mentre scorri\ncosi' puoi sentirlo.",
 "volumeSetPopupTitle_text": r"Imposta volume",
 "invertPump_text": r"Inverti pompa",
 "invertPumpAlertMBox_text": r"Inverte il senso di rotazione della pompa.\nUsalo per compensare la posizione\nfisica dell'interruttore sulla pompa.",
 "brightness_text": r"Luminosita'",
 "brightnessAlertMBox_text": r"Imposta la luminosita' del display.\nAuto-dim: 1min \xe2\x86\x92 50%, 5min \xe2\x86\x92 20%,\n10min \xe2\x86\x92 off. Tocca per riattivare.",
 "chemistryVolume_text": r"Volume chimica",
 "splashScreenAlertMBox_text": r"Personalizza la schermata di avvio.\n\nUsa predefinita: mostra la splash\nstandard Deep Ocean.\n\nCasuale al riavvio: genera una\nsplash casuale a ogni avvio.\n\nEntrambi off: scegli Palette, Stile\nforme e Complessita' manualmente.\nPremi Casuale per mescolare.",
 "chemistryVolumeAlertMBox_text": r"Basso: usa meta' della chimica.\nAlto: riempie completamente il tank.",
 "checkupNexStepsTitle_text": r"Prossimi passi:",
 "checkupProcessReady_text": r"Avvio processo...",
 "checkupTheMachineWillDo_text": r"La macchina eseguira':",
 "checkupFillWater_text": r"Riempimento vasca d'acqua",
 "checkupTankRotation_text": r"Verifica presenza tank e rotazione pellicola",
 "checkupReachTemp_text": r"Raggiungimento temperatura chimica",
 "checkupStop_text": r"Stop",
 "checkupStart_text": r"Avvia",
 "checkupSkip_text": r"Salta",
 "checkupStopNow_text": r"Stop subito!",
 "checkupStopAfter_text": r"Stop dopo!",
 "checkupProcessingTitle_text": r"In lavorazione:",
 "checkupStepSource_text": r"Sorgente step:",
 "checkupTempControl_text": r"Controllo temp.:",
 "checkupWaterTemp_text": r"Temp. acqua:",
 "checkupNextStep_text": r"Prossimo step:",
 "checkupSelectBeforeStart_text": r"Seleziona dimensione tank e quantita' di chimica e premi 'Avvia'",
 "checkupTankSize_text": r"Dimensione tank",
 "checkupChemistryVolume_text": r"Volume selezionato",
 "checkupMinimumChemistry_text": r"Chimica minima richiesta : 500ml",
 "checkupFillWaterMachine_text": r"La macchina non e' collegata a una fonte d'acqua.\n\nRiempi manualmente la vasca fino al sensore di livello superiore",
 "checkupTargetTemp_text": r"Temperatura target",
 "checkupWater_text": r"Acqua",
 "checkupChemistry_text": r"Chimica",
 "checkupTankPosition_text": r"Tank in posizione:",
 "checkupFilmRotation_text": r"In rotazione:",
 "checkupYes_text": r"Si'",
 "checkupNo_text": r"No",
 "checkupChecking_text": r"Verifica...",
 "checkupTargetToleranceTemp_text": r"tolleranza",
 "checkupProcessComplete_text": r"Processo\nCOMPLETATO!",
 "checkupProcessStopped_text": r"Processo\nFERMATO!",
 "checkupTankSizePlaceHolder_text": r"Misura",
 "checkupChemistryLowVol_text": r"Basso",
 "checkupChemistryHighVol_text": r"Alto",
 "checkupFilling_text": r"Riempimento",
 "checkupDraining_text": r"Scarico",
 "checkupProcessing_text": r"Lavorazione",
 "checkupRinsingLine_text": r"Lavaggio linea",
 "checkupDrainingComplete_text": r"Completato",
 "checkupHeaterStatusFmt_text": r"Riscaldatore: %s",
 "checkupHeaterOn_text": r"ON",
 "checkupHeaterOff_text": r"OFF",
 "checkupTempReached_text": r"Temp ok!",
 "checkupTempTimedOut_text": r"Timeout!",
 "checkupContinue_text": r"Continua",
 "checkupNoTempControl_text": r"No controllo temp.",
 "cleanPopupTitle_text": r"Configurazione pulizia",
 "cleanCleanProcess_text": r"Pulizia macchina",
 "cleanPopupSubTitle_text": r"Seleziona i contenitori da pulire",
 "cleanRoller_text": r"Cicli di pulizia",
 "cleanDrainWater_text": r"Scarica l'acqua alla fine",
 "cleanCancelButton_text": r"Annulla",
 "cleanCanceled_text": r"Annullato",
 "cleanRunButton_text": r"Avvia",
 "cleanStopButton_text": r"Stop",
 "cleanCloseButton_text": r"Chiudi",
 "cleanCycleFmt_text": r"%s ciclo:%d",
 "cleanCurrentClean_text": r"Pulizia",
 "cleanCompleteClean_text": r"COMPLETATA",
 "cleanWaste_text": r"Scarico",
 "cleanDraining_text": r"Svuotamento",
 "cleanFilling_text": r"Riempimento",
 "drainStopped_text": r"Scarico fermato",
 "drainComplete_text": r"Scarico completato!",
 "drainWasteIndicator_text": r">> SCARICO <<",
 "drainDrainingFmt_text": r"Svuotamento: %s",
 "drainDrainingC1_text": r"Svuotamento: C1",
 "fillBath_text": r"Riempi vasca",
 "fillChem_text": r"Riempi chimica",
 "fillPopupTitle_text": r"Riempimento vasca",
 "fillChemPopupTitle_text": r"Riempimento chimica",
 "fillChemFilling_text": r"Riempimento chimica...",
 "fillStatusReady_text": r"Premi Avvia per riempire",
 "fillManualPour_text": r"Versa l'acqua nella vasca",
 "fillManualFilling_text": r"Riempimento manuale...",
 "fillStart_text": r"Avvia",
 "fillCancel_text": r"Annulla",
 "fillStatusRunning_text": r"Riempimento... si ferma quando piena",
 "fillStatusFull_text": r"Vasca d'acqua piena",
 "fillStatusStopped_text": r"Riempimento fermato",
 "fillStatusTimeout_text": r"Timeout - controlla il sensore",
 "fillStatusNoFlow_text": r"Nessun flusso - controlla l'ingresso",
 "fillStatusNoLevel_text": r"Flusso OK ma la vasca non si riempie",
 "fillStatusDoneNoMax_text": r"Target raggiunto - MAX non confermato",
 "fillStop_text": r"Stop",
 "fillClose_text": r"Chiudi",
 "selfCheck_text": r"Autodiagnosi",
 "selfCheckTasks_text": r"Attivita':",
 "selfCheckTempSensors_text": r"Sensori temp.",
 "selfCheckWaterPump_text": r"Pompa acqua",
 "selfCheckHeater_text": r"Riscaldatore",
 "selfCheckValves_text": r"Valvole",
 "selfCheckContainer1_text": r"Contenitore C1",
 "selfCheckContainer2_text": r"Contenitore C2",
 "selfCheckContainer3_text": r"Contenitore C3",
 "selfCheckMotor_text": r"Motore agit.",
 "selfCheckRunning_text": r"In esecuzione...",
 "selfCheckDone_text": r"Fatto",
 "selfCheckComplete_text": r"Autodiagnosi completata!",
 "selfCheckFinished_text": r"Autodiagnosi terminata",
 "selfCheckSkip_text": r"Salta",
 "selfCheckNext_text": r"Avanti",
 "selfCheckRerun_text": r"Ripeti",
 "selfCheckStopped_text": r"Fermato",
 "selfCheckSkipped_text": r"Saltato",
 "selfCheckPumpRunning_text": r"Pompa in funzione...",
 "selfCheckTimeFmt_text": r"Tempo: %lds",
 "selfCheckTempFmt_text": r"Temp: %d.%d C",
 "selfCheckValveFmt_text": r"Valvola: %s",
 "buttonClose_text": r"Chiudi",
 "buttonStop_text": r"Stop",
 "buttonStart_text": r"Avvia",
 "buttonCancel_text": r"Annulla",
 "stopProcessPopupTitle_text": r"Ferma processo",
 "warningPopupTitle_text": r"Attenzione!",
 "setMinutesPopupTitle_text": r"Imposta minuti",
 "setSecondsPopupTitle_text": r"Imposta secondi",
 "tuneTempPopupTitle_text": r"Imposta temperatura",
 "tuneTolerancePopupTitle_text": r"Imposta tolleranza",
 "tuneRollerButton_text": r"Imposta",
 "calibResetButton_text": r"Reset",
 "messagePopupDetailTitle_text": r"Dettaglio",
 "deleteButton_text": r"Elimina",
 "deletePopupTitle_text": r"Elimina elemento",
 "duplicatePopupTitle_text": r"Duplica processo",
 "duplicateProcessPopupBody_text": r"Vuoi duplicare il processo selezionato?",
 "duplicateStepPopupTitle_text": r"Duplica step",
 "duplicateStepPopupBody_text": r"Vuoi duplicare lo step selezionato?",
 "deleteAllProcessPopupTitle_text": r"Elimina tutti i processi",
 "deletePopupBody_text": r"Sei sicuro di voler eliminare\nl'elemento selezionato?",
 "deleteAllProcessPopupBody_text": r"Sei sicuro di voler eliminare\ntutti i processi creati?",
 "warningPopupLowWaterTitle_text": r"Livello acqua troppo basso! Controllo temperatura sospeso\nRiempi subito la vasca per riprendere correttamente il controllo temperatura",
 "stopNowProcessPopupBody_text": r"Fermare un processo rovinera' la pellicola nel tank e lascera' la chimica all'interno!\nVuoi fermare il processo adesso?",
 "stopAfterProcessPopupBody_text": r"Vuoi fermare il processo al termine di questo step?",
 "maxNumberEntryProcessPopupBody_text": r"Numero massimo di PROCESSI raggiunto!",
 "maxNumberEntryStepsPopupBody_text": r"Numero massimo di STEP raggiunto!",
 "Maintenance_text": r"Manutenzione",
 "Utilities_text": r"Utilita'",
 "Statistics_text": r"Statistiche",
 "Software_text": r"Software",
 "cleanMachine_text": r"Pulisci macchina",
 "drainMachine_text": r"Svuota macchina",
 "importConfigAndProcesses_text": r"Importa",
 "importConfigAndProcessesMBox_text": r"Importa configurazione e processi dalla micro SD",
 "importConfigAndProcessesMBox2_text": r"Questo riavviera' la tua FilMachine! Sei sicuro?!",
 "exportConfigAndProcesses_text": r"Esporta",
 "exportConfigAndProcessesMBox_text": r"Esporta configurazione e processi sulla micro SD",
 "statCompleteProcesses_text": r"Processi completati",
 "statTotalProcessTime_text": r"Tempo totale di processo",
 "statCompleteCleanProcess_text": r"Cicli di pulizia completati",
 "statStoppedProcess_text": r"Processi fermati",
 "softwareVersion_text": r"Versione software",
 "softwareSerialNum_text": r"Numero seriale",
 "softwareCredits_text": r"Crediti",
 "calibrationPopupTitle_text": r"Calibrazione",
 "calibBath_text": r"Vasca",
 "calibChem_text": r"Chimica",
 "calibrationResetPopupTitle_text": r"Reset calibrazione",
 "calibrationResetPopupBody_text": r"La calibrazione della temperatura e' stata\nriportata ai valori predefiniti.",
 "otaConnecting_text": r"Connessione...",
 "otaStartingServer_text": r"Avvio server...",
 "otaStarting_text": r"Avvio...",
 "otaZeroPercent_text": r"0%",
 "otaUpdate_text": r"Aggiorna",
 "otaUpdateFromSD_text": r"Aggiorna da SD",
 "otaUpdateFromSDMBox_text": r"Aggiorna il firmware da\nfile su scheda SD.",
 "otaWifiUpdate_text": r"Aggiornamento Wi-Fi",
 "otaWifiUpdateMBox_text": r"Avvia un server web locale.\nCarica il firmware dal browser.",
 "otaUpdating_text": r"Aggiornamento...",
 "otaNoFirmware_text": r"Nessun firmware trovato su SD",
 "otaConfirmUpdate_text": r"Aggiornare il firmware a %s?\nNon spegnere la macchina!",
 "otaRebootNow_text": r"Riavviare ora per applicare?",
 "otaWifiSSID_text": r"SSID Wi-Fi",
 "otaWifiSSIDAlert_text": r"Inserisci il nome della rete\nWi-Fi per gli aggiornamenti OTA.",
 "otaWifiPassword_text": r"Password Wi-Fi",
 "otaWifiPasswordAlert_text": r"Inserisci la password\ndella rete Wi-Fi.",
 "processDetailStep_text": r"Step",
 "processDetailInfo_text": r"Dettagli",
 "processDetailIsColor_text": r"Per negativo a colori",
 "processDetailIsBnW_text": r"Per negativo b/n",
 "processDetailIsTempControl_text": r"Controllo temp.",
 "processDetailTemp_text": r"Temperatura:",
 "processDetailIsPreferred_text": r"Preferito:",
 "processDetailTotalTime_text": r"Tempo totale:",
 "processDetailTempPlaceHolder_text": r"Tocca",
 "processDetailTempTolerance_text": r"Tolleranza:",
 "processDetailPlaceHolder_text": r"Inserisci nome",
 "stepDetailTitle_text": r"Nuovo step",
 "stepDetailName_text": r"Nome:",
 "stepDetailDuration_text": r"Durata:             :",
 "stepDetailDurationMinPlaceHolder_text": r"min",
 "stepDetailDurationSecPlaceHolder_text": r"sec",
 "stepDetailType_text": r"Tipo:",
 "stepDetailSource_text": r"Sorgente:",
 "stepDetailDiscardAfter_text": r"Scarta dopo:",
 "stepDetailPlaceHolder_text": r"Nome del nuovo step",
 "stepDetailSave_text": r"Salva",
 "stepDetailCancel_text": r"Annulla",
 "stepDetailCurrentTemp_text": r"Ora:",
 "tankSizeSmall_text": r"S",
 "tankSizeMedium_text": r"M",
 "tankSizeLarge_text": r"L",
 # roller option lists
 "chemistryVolumeList": r"Basso\nAlto",
 "stepTypeList": r"Chimica\nRisciacquo\nMultiRisciacquo",
 "checkupTankSizesList": r"500ml\n700ml\n1000ml",
 "stepSourceList": r"C1\nC2\nC3\nWB",
 # new language-feature strings (added to FilMachine.h by this script)
 "language_text": r"Lingua",
 "languageAlertMBox_text": r"Imposta la lingua dell'interfaccia.\nLa macchina si riavvia per applicarla.",
 "languageSetPopupTitle_text": r"Imposta lingua",
 "languageRebootTitle_text": r"Cambio lingua",
 "languageRebootBody_text": r"La macchina si riavviera'\nper applicare la nuova lingua.",
 "languageList": r"English\nItaliano",
 "screenOff_text": r"Spegnimento schermo",
 "screenOffAlertMBox_text": r"Dopo quanto tempo dall'ultimo tocco lo schermo si spegne. Prima si attenua in due passi, poi si spegne del tutto. Tocca lo schermo per riattivarlo. 'Mai' lo tiene sempre acceso. Durante un processo lo schermo non si spegne.",
 "screenOffNever_text": r"Mai",
}

# New macros to append to FilMachine.h (EN text). Order matters for the enum.
NEW_STRINGS = [
    ("language_text",              r"Language"),
    ("languageAlertMBox_text",     r"Set the interface language.\nThe machine reboots to apply it."),
    ("languageSetPopupTitle_text", r"Set language"),
    ("languageRebootTitle_text",   r"Language change"),
    ("languageRebootBody_text",    r"The machine will restart\nto apply the new language."),
    ("languageList",               r"English\nItaliano"),
]

def parse_defines(src):
    """Return list of (name, en_content) preserving order; joins adjacent literals."""
    pat = re.compile(r'#define\s+(\w+(?:_text|List))\s+((?:"(?:[^"\\]|\\.)*"\s*)+)')
    out = []
    for m in pat.finditer(src):
        name = m.group(1)
        if name in SKIP:
            continue
        lits = re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2))
        out.append((name, "".join(lits)))
    return out

def main():
    src = open(HDR).read()
    entries = parse_defines(src)
    names = [n for n, _ in entries]

    # append new strings
    for name, en in NEW_STRINGS:
        if name not in names:
            entries.append((name, en))
            names.append(name)

    # ── lang.h ──
    with open(LANG_H, "w") as f:
        f.write("/**\n * @file lang.h — generated by scripts/gen_lang.py, do not edit by hand.\n"
                " * UI language support (EN/IT). Strings resolved at runtime via tr().\n */\n"
                "#ifndef LANG_H\n#define LANG_H\n\n#include <stdint.h>\n\n"
                "#define LANG_EN 0\n#define LANG_IT 1\n#define LANG_COUNT 2\n\n"
                "enum lang_str_id {\n")
        for n in names:
            f.write(f"    STR_{n},\n")
        f.write("    STR__COUNT\n};\n\n"
                "const char *tr(int id);\n"
                "void lang_set(uint8_t lang);\n"
                "uint8_t lang_get(void);\n\n#endif /* LANG_H */\n")

    # ── lang.c ──
    with open(LANG_C, "w") as f:
        f.write("/**\n * @file lang.c — generated by scripts/gen_lang.py, do not edit by hand.\n"
                " * EN/IT string table. Italian uses ASCII apostrophes (fonts are 0x20-0x7F).\n */\n"
                "#include \"lang.h\"\n#include <stddef.h>\n\n"
                "typedef struct { const char *en; const char *it; } lang_pair_t;\n\n"
                "static const lang_pair_t strings[STR__COUNT] = {\n")
        missing = []
        for n, en in entries:
            it = IT.get(n)
            if it is None:
                missing.append(n)
                itc = "NULL"
            else:
                itc = f'"{it}"'
            f.write(f'    [STR_{n}] = {{ "{en}", {itc} }},\n')
        f.write("};\n\nstatic uint8_t s_lang = LANG_EN;\n\n"
                "void lang_set(uint8_t lang) { s_lang = (lang == LANG_IT) ? LANG_IT : LANG_EN; }\n"
                "uint8_t lang_get(void) { return s_lang; }\n\n"
                "const char *tr(int id) {\n"
                "    if (id < 0 || id >= STR__COUNT) return \"\";\n"
                "    if (s_lang == LANG_IT && strings[id].it != NULL) return strings[id].it;\n"
                "    return strings[id].en;\n}\n")

    # ── rewrite FilMachine.h ──
    def repl(m):
        name = m.group(1)
        if name in SKIP:
            return m.group(0)
        return f"#define {name}\ttr(STR_{name})"
    pat = re.compile(r'#define\s+(\w+(?:_text|List))\s+((?:"(?:[^"\\]|\\.)*"\s*(?=\n|"))+)')
    new_src, nsub = pat.subn(repl, src)

    # include lang.h once, right after the first #include block guard
    if '#include "lang.h"' not in new_src:
        anchor = new_src.find('\n', new_src.find('#define MAIN_FILMACHINE_H_'))
        new_src = new_src[:anchor+1] + '\n#include "lang.h"\n' + new_src[anchor+1:]

    # append the new language macros next to chemistryVolume_text if absent
    if 'language_text' not in new_src:
        block = "\n/* Language setting (EN/IT) — added by gen_lang.py */\n"
        for name, _ in NEW_STRINGS:
            block += f"#define {name}\ttr(STR_{name})\n"
        marker = "#define chemistryVolumeList"
        idx = new_src.find(marker)
        idx = new_src.find('\n', idx) + 1
        new_src = new_src[:idx] + block + new_src[idx:]

    open(HDR, "w").write(new_src)

    print(f"defines transformed: {nsub}")
    print(f"table entries: {len(entries)}")
    if missing:
        print(f"MISSING IT ({len(missing)}):")
        for n in missing: print("  ", n)

if __name__ == "__main__":
    main()
