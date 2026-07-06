# FilMachine — Audit Report

**Data:** 6 luglio 2026 · **Firmware:** v0.0.0.50 (git clean, HEAD `932eeb8`) · **App Flutter:** filmachine_app

---

## 1. Stato generale

Il progetto è in buona salute: repo git pulito, nessun TODO/FIXME nei sorgenti, ultimo run dei test firmware **180/180 PASS** (03/04/2026) e test Flutter **240/240 PASS**. I problemi trovati sono per lo più pulizia e disallineamento app↔firmware.

## 2. Pendenze e pulizia (firmware)

| # | Gravità | Problema | Dettaglio |
|---|---------|----------|-----------|
| 1 | Media | **App Flutter senza repository git** | `filmachine_app` non è versionata: nessun backup/storia. Consigliato `git init` + remote. |
| 2 | Bassa | **README.md cita `pca9685.c` rimosso** | README righe 117 e 119: il driver legacy non esiste più in `drivers/`. Da aggiornare la directory structure. |
| 3 | Bassa | **README incompleto sulle settings WS** | La sezione WebSocket non elenca `lineRinseEnabled`, `lineRinseTime`, `invertPump`, `brightness`, `volume`, splash*. |
| 4 | Bassa | **Clutter locale (già gitignored)** | `build800/` (124 MB, target obsoleto), `sdkconfig.old`, `dependencies.lock.simv2_bak`. Eliminabili senza rischio. |
| 5 | Bassa | **`test_results/` con ~30 file storici** | Solo l'ultimo run è utile; i vecchi (marzo 2026) sono eliminabili. |
| 6 | Info | **`resources/` pesa 791 MB** | Contiene il dump demo del vendor (JC4880P443C). Utile come riferimento ma appesantisce il repo/backup. |

**Falsi allarmi verificati:** `audio.c` è correttamente board-only (escluso dal simulatore by design); `page_splash.c` è dichiarato una sola volta nel CMakeLists; nessun test attualmente fallito.

### 2b. Residui di vecchie implementazioni (scan approfondito)

Nessun TODO/FIXME/HACK nei sorgenti. Residui trovati:

| Elemento | Dove | Stato |
|----------|------|-------|
| `chemContainerMl*` / `wbContainerMl*` (testi UI, liste roller, 6 puntatori `lv_obj_t`) | `FilMachine.h` righe 320-329 e ~1500-1506 | Vecchia impostazione "container in ml", sostituita da `chemCalibFillSecs`/`wbCalibFillSecs`. Quasi tutti a 0 riferimenti nei .c → **rimovibili** |
| `pump_brake()` | `accessories.c:172` | Definita, `__attribute__((unused))`, mai chiamata → rimovibile |
| `isPumping` | `page_checkup.c:44` | Commentata, "reserved for future" |
| Blocco "Reserved Icons" | `FilMachine.h:228` | Commentato, intenzionale |
| Commento stantio su `random_switch_y` | `ui_profile.h:942` | Dice "unused" ma È usato in `element_splashPopup.c:351` → correggere il commento |

Rinominazioni intenzionali per compatibilità binaria del config (da NON toccare): `volume` (ex `dimTimeout`), `chemCalibFillSecs`/`wbCalibFillSecs` (ex `chemContainerMl`/`wbContainerMl`).

## 3. Gap firmware → app (cose da aggiungere all'app)

Feature presenti nel firmware e già trasmesse via WebSocket, ma **ignorate dall'app**:

| Feature | Firmware | App | Note |
|---------|----------|-----|------|
| **Line rinse** (`lineRinseEnabled`, `lineRinseTime`) | ✓ inviati nello stato + `set_setting` | ✗ assenti da `MachineState` e UI | È l'ultimo commit del firmware (`932eeb8`) |
| **Calibrazione riempimento** (`chemCalibFillSecs`, `wbCalibFillSecs`) | ✓ inviati nello stato | ✗ non gestiti | Solo visualizzazione/uso, la calibrazione si fa a bordo |
| **`wifi_scan`** | ✓ implementato (ws_server.c ~824) | ✗ mai invocato | L'app mostra l'SSID in sola lettura |

## 4. Gap interni al firmware (settings non esposte via WebSocket)

Presenti in `machineSettings` (FilMachine.h righe 624-643) e usate a bordo, ma **non incluse in `build_state_json()` né gestite da `set_setting`** — quindi impossibili da controllare da app:

- `brightness` (luminosità LCD 10-100%)
- `volume` (audio 0-100%)
- `invertPump` (inversione pompa)
- `chemCalibOffset` (offset sensore temp. chimica)
- Splash screen: `splashRandom`, `splashPalette`, `splashShapeStyle`, `splashComplexity`, `splashSeed`, `splashDefault`

Per esporle all'app serve toccare **anche il firmware** (aggiunta campi in `build_state_json()` + rami in `set_setting`).

## 5. Piano consigliato

1. **App (solo lato app, nessun rischio):** supporto line rinse (state + UI settings), campi calibrazione riempimento, pulsante scan Wi-Fi.
2. **Firmware + app (opzionale):** esporre brightness/volume/invertPump/chemCalibOffset via WS e aggiungere i controlli nell'app; eventualmente anche le impostazioni splash.
3. **Pulizia:** fix README (pca9685 + elenco settings), rimozione `build800/`, `sdkconfig.old`, `*_bak`, vecchi `test_results`.
4. **Versionamento:** `git init` per l'app Flutter.

---

## 6. Interventi eseguiti (6 luglio 2026)

**Firmware (`main/ws_server.c`):** aggiunti a `build_state_json()` e a `set_setting` i campi `invertPump`, `brightness` (con applicazione live `st7701_lcd_set_user_brightness`), `volume` (con `audio_set_volume`), `chemCalibOffset`, `splashDefault`, `splashRandom`, `splashPalette`, `splashShapeStyle`, `splashComplexity` (tutti con clamp dei range). `reset_defaults` ora ripristina anche invertPump/brightness/volume/chemCalibOffset.

**App Flutter:** `MachineState` esteso con tutti i nuovi campi + `lineRinseEnabled/Time` e `chemCalibFillSecs`/`wbCalibFillSecs`; rimossi i campi rotti `calibratedTemp`, `chemContainerMl`, `wbContainerMl` (il firmware li rifiutava come chiavi sconosciute). Settings screen: aggiunti line rinse, invert pump, brightness, volume, sezione splash completa, righe read-only per la calibrazione riempimento e pulsante **Scan Wi-Fi** (nuovo `wifiScan()` nel service con gestione dell'evento `wifi_scan_results`). Tools screen aggiornata. Test del modello aggiornati.

**Residui rimossi:** `chemContainerMl*`/`wbContainerMl*` (testi, liste, puntatori GUI, handler popup, voci in `ui_debug_registry.inc`), `pump_brake()` (entrambe le definizioni), commento stantio su `random_switch_y`. README corretto (pca9685, tabella settings, tabella comandi WS con `reorder_step`/`reset_defaults`/`wifi_scan` e elenco chiavi `set_setting`).

**Verifica:** simulatore + test binary compilano senza errori con tutte le modifiche; suite WebSocket (serializzazione JSON stato) verde. Il run completo della suite non è riproducibile nell'ambiente di verifica usato (segfault del driver video SDL dummy, presente identico anche sul codice originale non modificato) — consigliato un run completo di `filmachine_test` e `flutter test` in locale.
- `main/ws_server.c` — protocollo WS (comandi riga ~678, stato riga ~479)
- `main/include/FilMachine.h` — struct settings (624-643)
- App: `lib/models/machine_state.dart`, `lib/services/machine_service.dart`, `lib/screens/settings_screen.dart`
