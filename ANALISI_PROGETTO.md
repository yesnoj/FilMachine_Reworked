# FilMachine Simulator v2 — Analisi Completa del Progetto

## Panoramica Architettura

Il progetto è un **simulatore desktop** (SDL2 + LVGL 9.2.2) del firmware di una macchina per lo sviluppo di pellicole fotografiche basata su **ESP32-S3**. Il simulatore riproduce fedelmente l'interfaccia utente della macchina reale, sostituendo l'hardware (relè, motori, sensori di temperatura, SD card, I2C) con stub software.

### Stack Tecnologico
- **Target reale:** ESP32-S3 con display ILI9488 480×320, touch FT5x06, SD card SPI, MCP23017 I2C expander
- **Simulatore:** C11, CMake, SDL2 (display/touch), LVGL 9.2.2, stub POSIX per FatFS/FreeRTOS
- **Build:** `cmake .. && make` produce `filmachine_sim`
- **Workflow:** Sviluppo nel simulatore → `flash.sh` copia i sorgenti nel repo firmware e flasha la board

### Struttura Directory
```
FilMachine_Simulator_v2/
├── main/include/FilMachine.h   ← Header principale (struct, define, prototipi)
├── main/accessories.c          ← Utility: keyboard, config I/O, linked list, deep copy
├── c_pages/                    ← Pagine dell'interfaccia (8 file)
│   ├── page_home.c             ← Splash screen e gestione errori boot
│   ├── page_menu.c             ← Tab bar (Processes / Settings / Tools)
│   ├── page_processes.c        ← Lista processi con filtri
│   ├── page_processDetail.c    ← Dettaglio/modifica processo
│   ├── page_stepDetail.c       ← Dettaglio/modifica step
│   ├── page_settings.c         ← Impostazioni macchina
│   ├── page_tools.c            ← Manutenzione, statistiche, import/export
│   └── page_checkup.c          ← Esecuzione processo (filling, temp, rotation)
├── c_elements/                 ← Componenti UI riutilizzabili (6 file)
│   ├── element_process.c       ← Elemento processo nella lista (linked list)
│   ├── element_step.c          ← Elemento step con drag & drop
│   ├── element_filterPopup.c   ← Popup filtro processi
│   ├── element_messagePopup.c  ← Popup messaggi/conferme
│   ├── element_rollerPopup.c   ← Popup roller numerico
│   └── element_cleanPopup.c    ← Popup pulizia macchina con timer
├── c_fonts/                    ← Font icone custom (5 dimensioni)
├── c_graphics/splash.c         ← Immagine splash screen
├── src/main.c                  ← Entry point simulatore (SDL loop)
├── stub/                       ← Stub ESP32 (GPIO, FreeRTOS, FatFS, I2C, ecc.)
├── sd/                         ← File configurazione simulati (SD card)
├── scripts/genFilMachineCFG.py ← Generatore .cfg binario
└── CMakeLists.txt              ← Build system
```

---

## RIEPILOGO STATO BUG

| # | Descrizione | Stato | Note |
|---|-------------|-------|------|
| 1 | Condizione I2C impossibile — page_home.c | BY DESIGN | Disabilita errori I2C in fase di test senza hardware collegato |
| 2 | Buffer overflow strcpy() | RISOLTO | strcpy → strncpy + null-termination in 3 file |
| 3 | Use-After-Free lv_obj_delete_async() | RISOLTO | lv_obj_delete_async → lv_obj_delete in element_process.c e element_step.c |
| 4 | Puntatore dangling in duplicazione processo | RISOLTO | Rimosso free(whoCallMe) su fallimento deepCopy in element_messagePopup.c |
| 5 | Shallow copy stepDetails (memcpy) | RISOLTO | Azzerati puntatori LVGL dopo memcpy + aggiornata deepCopyStepDetail() |
| 6 | Timer non azzerati dopo lv_timer_del | RISOLTO | Aggiunto = NULL dopo ogni lv_timer_del in element_cleanPopup.c |
| 7 | Null pointer dereference in event_keyboard | RISOLTO | Aggiunto null-check su tempProcessNode/tempStepNode in tutti gli handler |
| 8 | Nessun check ritorno allocateAndInitializeNode | RISOLTO | Aggiunto NULL check + return in page_processDetail.c, page_stepDetail.c, accessories.c |
| 9 | atoi() senza validazione | RISOLTO | atoi → strtol con base 10 in page_stepDetail.c |
| 10 | Timer mai cancellato in page_tools.c | RISOLTO | Timer pause/resume con tools_pause_timer() in page_menu.c |
| 11 | Buffer troppo piccolo nel roller | RISOLTO | tempBuffer[10] → tempBuffer[24] in element_rollerPopup.c |
| 12 | Variabili statiche non thread-safe | BASSO RISCHIO | page_settings.c — LVGL single-thread mitiga |
| 13 | Macro duplicate nel header | RISOLTO | Definizioni duplicate rimosse |
| 14 | Messaggio errore fuorviante writeConfigFile | RISOLTO | Log condizionato al risultato di f_unlink (res != FR_OK && res != FR_NO_FILE) |
| 15 | Memory leak in copyAndRenameFile | RISOLTO | Ora usa goto per liberare il buffer |
| 16 | NULL dereference chain in element_messagePopup.c | RISOLTO | Guard con NULL check su tempProcessNode, processDetails, checkup |
| 17 | Memory leak getRollerStringIndex() | RISOLTO | Risultato malloc'd salvato in variabile temp e poi free()'d |
| 18 | Dereference senza NULL check in page_processDetail.c | RISOLTO | Guard all'inizio di event_processDetail() |
| 19 | **CRITICAL** BUS ERROR — lv_msgbox_close su screen non-msgbox | RISOLTO | Rimosso lv_msgbox_close, aggiunto cleanup timer, delete checkupParent dopo lv_scr_load |

---

## BUG RISOLTI (sessioni precedenti)

| # | Bug | Stato | Note |
|---|-----|-------|------|
| 1 | Condizione logica I2C `page_home.c` | BY DESIGN | Disabilita errore I2C durante lo sviluppo |
| 2 | Buffer overflow `strcpy()` | RISOLTO | Sostituito con `strncpy()` + null terminator |
| 3 | Use-After-Free `lv_obj_delete_async()` | RISOLTO | Cambiato in `lv_obj_delete()` sincrono |
| 4 | Puntatore dangling in duplicazione | RISOLTO | `free(newProcessName)` al posto di `free(whoCallMe)` |
| 5 | Shallow copy `stepDetails` | RISOLTO | NULL-out di tutti i 30 puntatori LVGL dopo memcpy |
| 6 | Timer non azzerati a NULL | RISOLTO | Aggiunto `= NULL` dopo ogni `lv_timer_del()` |
| 7 | Null dereference in `event_keyboard()` | RISOLTO | Aggiunto guard `gui.tempProcessNode != NULL` |
| 8 | Nessun check `allocateAndInitializeNode()` | RISOLTO | Aggiunto NULL check + return |
| 9 | `atoi()` senza validazione | RISOLTO | Cambiato in `strtol()` |
| 10 | Timer leak `page_tools.c` | RISOLTO | Pattern pause/resume + `tools_pause_timer()` |
| 11 | Roller buffer troppo piccolo | RISOLTO | `tempBuffer[10]` → `tempBuffer[24]` |
| 12 | Variabili statiche non thread-safe | BASSO RISCHIO | LVGL single-thread mitiga |
| 13 | Macro duplicate nel header | RISOLTO | Definizioni duplicate rimosse |
| 14 | Messaggio errore fuorviante writeConfigFile | RISOLTO | Log condizionato a `res != FR_OK && res != FR_NO_FILE` |
| 15 | Memory leak in copyAndRenameFile | RISOLTO | Usa `goto` per liberare il buffer |

### Ulteriori bug risolti nelle sessioni di lavoro:
- **Linked list corruption on reorder** — `element_step.c`: skip `currentNode` nel loop
- **Gesture priority over long press** — `element_step.c`: swipe ha priorità
- **hasListChanged() falso positivo** — `element_step.c`: corretto `original_y` a -13
- **Delete button covering step summary** — `element_step.c`: rimosso `lv_obj_move_foreground`
- **Green line Z-order** — `page_processes.c`: linea verde in cima allo Z-order
- **deepCopyStepList crash** — `accessories.c`: corretta deep copy lista step
- **deleteProcess crash** — `element_process.c`: corretta cancellazione processo
- **Filter logic** — `element_filterPopup.c`: corretta logica di filtraggio
- **caseInsensitiveStrstr leak** — `accessories.c`: corretta perdita di memoria
- **deepCopy leaks** — `accessories.c`: corrette perdite in deep copy
- **Double `size++` in duplicate step** — `element_messagePopup.c` e `element_step.c`: `insertStepElementAfter` già incrementa size, rimosso il `size++` duplicato
- **Step duplicato non appariva** — GUI creata DOPO chiusura popup modale + `lv_obj_update_layout()` in `reorderStepElements`
- **Swipe non sempre rilevato** — Soglie gesture abbassate (`gesture_limit` 50→30, `gesture_min_velocity` 3→2) + distance check in SHORT_CLICKED
- **Duplicate step inserito in coda** — Cambiato da `insertStepElementAfter` a `addStepElement` per inserire sempre in fondo alla lista (risolve problema visuale)

---

## NUOVI BUG TROVATI — RISOLTI

### 16. RISOLTO — NULL dereference chain in `element_messagePopup.c` riga 154
La catena `||` con dereference di `gui.tempProcessNode->...->checkup->checkupStopAfterButton` veniva valutata anche se `gui.tempProcessNode` era NULL. Ristrutturata la condizione con `gui.tempProcessNode != NULL && processDetails != NULL && checkup != NULL` come guard prima di accedere ai campi checkup.

### 17. RISOLTO — Memory leak in `getRollerStringIndex()` — `element_rollerPopup.c`
`getRollerStringIndex()` restituisce memoria malloc'd. I 3 chiamanti passavano il risultato direttamente a `lv_textarea_set_text()` senza fare `free()`. Ora il risultato è salvato in variabile temporanea, passato a `lv_textarea_set_text()`, e poi `free()`'d.

### 18. RISOLTO — `page_processDetail.c`: dereference senza NULL check
Aggiunto guard `if(gui.tempProcessNode == NULL || processDetails == NULL) return;` all'inizio di `event_processDetail()`, prima di tutti gli handler (CLICKED, REFRESH, VALUE_CHANGED).

### 19. RISOLTO — **CRITICAL BUS ERROR** in `page_checkup.c`: `lv_msgbox_close()` su screen non-msgbox
**Sintomo:** Crash `zsh: bus error` quando l'utente clicca il pulsante Close alla fine di un processo completo (dopo filling, processing, draining di tutti gli step). Il file di configurazione SD veniva corrotto dal crash.

**Causa root:** In `event_checkup()`, il close handler calcolava `mboxCont = lv_obj_get_parent(lv_obj_get_parent(obj))` e poi chiamava `lv_msgbox_close(mboxCont)`. Per il `checkupCloseButton`, il grandparent è `checkupParent`, uno screen creato con `lv_obj_create(NULL)` — NON un `lv_msgbox`. `lv_msgbox_close()` tenta di accedere a dati interni specifici del msgbox, causando accesso a memoria non valida → BUS ERROR.

**Fix applicato:**
- Rimosso `lv_msgbox_close(mboxCont)` — non va usato su screen regolari
- Aggiunto cleanup sicuro dei timer (`processTimer` e `pumpTimer`) con NULL check
- Chiamata `resetStuffBeforeNextProcess()` per azzerare lo stato statico
- `lv_scr_load(menu)` PRIMA di `lv_obj_delete(checkupParent)` per evitare accessi a oggetti eliminati
- `checkupParent` e `checkupContainer` settati a NULL dopo la delete
- Aggiunto `= NULL` dopo ogni `lv_timer_delete()` in 3 ulteriori posizioni nel file (processTimer e pumpTimer nelle callback dei timer)

---

## FUNZIONALITÀ AGGIUNTE

- **Duplicazione step via LEFT swipe** — Swipe sinistro sullo step mostra popup di conferma per duplicare
- **Tastiera numeri/simboli** — Aggiunto tasto "123" sulla tastiera lettere per switchare a numeri e caratteri speciali. La tastiera si resetta sempre alle lettere alla riapertura.
- **Fix: step duplicato non appariva visivamente** — Aggiunto `lv_obj_update_layout()` in `reorderStepElements` per forzare il ricalcolo dell'area scrollabile del container dopo l'aggiunta di figli dinamici. Aggiunto `lv_obj_scroll_to_view()` nel handler di duplicazione per scrollare automaticamente al nuovo step.
- **Fix: swipe non sempre rilevato (GESTURE)** — Abbassate le soglie gesture nel simulatore (`gesture_limit` da 50 a 30, `gesture_min_velocity` da 3 a 2) in `src/main.c` per migliorare la rilevazione di swipe corti/lenti. Per l'hardware ESP32, la stessa modifica va applicata dove il touch indev viene creato.

---

## SISTEMA DI TEST AUTOMATICO

### Architettura
Framework di test leggero integrato nel build system CMake. Nessuna dipendenza esterna (no Unity, no Ruby). Simula touch input via un `lv_indev` custom con callback controllato dal test.

### File
```
tests/
├── test_runner.h          ← Macro (TEST_ASSERT, TEST_BEGIN/END), coordinate UI, prototipi
├── test_runner.c          ← Entry point (sostituisce main.c), init LVGL + SDL, esegue suite
├── test_helpers.c         ← Simulazione touch: test_click, test_swipe, test_long_press, test_pump
├── test_navigation.c      ← 5 test: splash, start→menu, switch Processes/Settings/Tools
├── test_processes.c       ← 5 test: lista processi, new process, click→detail, UI elements, close
├── test_steps.c           ← 4 test: add step, verify UI, swipe-left duplicate, close
└── test_settings.c        ← 3 test: UI elements, defaults validi, return to processes
```

### Come compilare e lanciare
```bash
cd build
cmake ..
make filmachine_test
./filmachine_test
```

### Output
Stampa PASS/FAIL per ogni test + riepilogo finale. Exit code 0 = tutto ok, 1 = fallimenti.

---

## DIFFERENZE SIMULATORE vs HARDWARE

| Aspetto | Hardware Reale | Simulatore | Impatto |
|---------|---------------|------------|---------|
| **GPIO reads** | Stato reale dei pin | Sempre 0 | Firmware che legge pulsanti/sensori si comporta diversamente |
| **Timer hardware** | Callback reali | No-op (ritorna success) | Operazioni time-driven non eseguono |
| **Task FreeRTOS** | Esecuzione concorrente | Solo printf, mai eseguiti | Task in background non funzionano |
| **Queue blocking** | Blocco con timeout | Fallisce immediatamente | Loop che attendono dati dalla coda deadlockano |
| **FatFS FA_OPEN_ALWAYS** | Crea se mancante | Fallisce con FR_NO_FILE | Config load fallisce al primo avvio |
| **Heap/PSRAM** | Limitata (4MB PSRAM) | malloc illimitata | OOM non testabile |
| **Reboot** | Riavvio sistema | Ignorato | Il firmware non può auto-ripararsi |
| **Temperatura** | Lettura sensori reali | Valori dalla config | Il firmware non reagisce all'ambiente |

---

## PROBLEMI DI QUALITÀ DEL CODICE

### Architetturali
- **Stato globale massiccio:** tutto passa attraverso `gui` e `sys` (struct globali)
- **Event handler monolitici:** `event_messagePopup()` è 220+ righe; `event_processDetail()` simile
- **Void pointer per context:** `whoCallMe` è `void*` con cast runtime — nessuna type safety
- **Nesting eccessivo:** alcuni handler hanno 8+ livelli di indentazione

### Manutenibilità
- **DRY violation:** la logica di arrotondamento slider è duplicata 5 volte in `page_settings.c`
- **Codice commentato:** versioni vecchie di funzioni lasciate come commenti (accessories.c, page_tools.c)
- **Commenti misti italiano/inglese:** rende il codice meno leggibile per un team internazionale
- **Magic numbers:** valori come -13, 70, 200 per posizionamento UI senza costanti nominate
- **Typo nel nome funzione:** `convertCelsiusoToFahrenheit` (extra "o")
- **Typo nei nomi campo:** `stepDetailNamelTextArea` (extra "l")

---

## RACCOMANDAZIONI PRIORITARIE

### Immediato (bloccanti per stabilità)
1. Sostituire tutti i `strcpy()` con `strncpy()` + null-termination (#2)
2. Sincronizzare `free()` con `lv_obj_delete_async()` — usare `lv_obj_delete()` sincrono o callback (#3)
3. Correggere `free(whoCallMe)` nel fallimento di deepCopy (#4)
4. Aggiungere null-check su `gui.tempProcessNode` e `gui.tempStepNode` nel keyboard handler (#7)
5. Impostare timer a NULL dopo `lv_timer_del()` in cleanPopup (#6)

### Alta priorità (stabilità)
6. Deep copy corretta per stepDetails invece di memcpy (#5)
7. Verificare il ritorno di `allocateAndInitializeNode()` ovunque (#8)
8. Cancellare/riusare il timer in `page_tools.c` (#10)
9. Aumentare `tempBuffer` a 20+ byte nel roller popup (#11)

### Medio termine (qualità)
10. Sostituire `atoi()` con `strtol()` + validazione (#9)
11. Correggere messaggio errore in writeConfigFile (#14)
12. Estrarre la logica slider in una funzione helper
13. Standardizzare i commenti in inglese
