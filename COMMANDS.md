# FilMachine_Reworked — Comandi utili

Riferimento rapido per build, flash e gestione config.
Tutti i comandi si lanciano dalla root del repo salvo diversa indicazione:

```bash
cd ~/Documents/GitHub/FilMachine_Reworked
```

---

## 1. Simulatore (PC, macOS — SDL2 + LVGL)

Prerequisiti (una tantum):

```bash
brew install sdl2 pkg-config
```

Build ed esecuzione (uso una build dir dedicata per non collidere con la cache ESP-IDF):

```bash
cmake -B build_sim -S .
cmake --build build_sim -j
./build_sim/filmachine_sim
```

- Stop simulatore: **Ctrl+C** nel terminale (oppure chiudi la finestra SDL / `pkill filmachine_sim`).
- `F2` = overlay di debug UI, click destro = dump widget.
- LVGL v9.5 viene clonato in automatico se manca.

---

## 2. Build per la board (ESP32-P4 / JC4880P433)

```bash
source ~/esp/esp-idf-v5.5/export.sh
idf.py set-target esp32p4        # solo la 1ª volta o se cambi target (fa fullclean)
idf.py -D CMAKE_C_FLAGS="-DBOARD_JC4880P433" reconfigure
idf.py -D CMAKE_C_FLAGS="-DBOARD_JC4880P433" build
```

Note:
- Richiede **ESP-IDF 5.5.x** (l'ESP32-P4 non è supportato da versioni precedenti).
- `set-target` serve solo la prima volta o dopo aver cambiato target; le volte successive bastano `reconfigure` + `build`.

---

## 3. Flash e monitor

```bash
idf.py -p /dev/cu.usbmodem101 flash monitor      # flash + seriale (Ctrl+] per uscire dal monitor)
idf.py -p /dev/cu.usbmodem101 flash              # solo flash
idf.py -p /dev/cu.usbmodem101 monitor            # solo monitor
```

Se non conosci la porta:

```bash
ls /dev/cu.usb*
```

### Script all-in-one

`./flash_p4.sh` esegue tutto: set-target (se serve), patch sdkconfig per P4 (flash 16MB, PSRAM 32MB HEX, WebSocket…), build, detect porta, flash e monitor.

```bash
./flash_p4.sh
```

---

## 4. Config / processi (genFilMachineCFG.py)

Genera i file `FilMachine.cfg` + `_Backup.cfg` + `.json` nel formato letto da `readConfigFile`.

```bash
# 8 processi realistici di sviluppo pellicola (C41, D-76, Rodinal, E6, HC110…)
python3 scripts/genFilMachineCFG.py --realistic --output sd/

# realistici + extra per arrivare a N processi
python3 scripts/genFilMachineCFG.py --realistic --processes 12 --output sd/

# dati random (nomi a caratteri casuali)
python3 scripts/genFilMachineCFG.py --output sd/

# import da un JSON esistente
python3 scripts/genFilMachineCFG.py --from-json scripts/FilMachine.json --output sd/
```

Il simulatore legge `sd/FilMachine.cfg` relativo alla dir da cui lo lanci.
Se all'avvio compaiono warning "File may be corrupt", il `.cfg` in `sd/` è di un formato struct vecchio: rigeneralo con lo script.

### Copiare il config sulla microSD della board

Dopo aver rigenerato `sd/FilMachine.json` (config JSON-only), copialo sulla microSD (il nome del volume di default è `NO NAME` — adatta se diverso). **Il `sync` + `eject` sono obbligatori**, altrimenti macOS non flusha la scrittura e la board rilegge il file vecchio:

```bash
cp ~/Documents/GitHub/FilMachine_Reworked/sd/FilMachine.json        "/Volumes/NO NAME/FilMachine.json"
cp ~/Documents/GitHub/FilMachine_Reworked/sd/FilMachine_Backup.json "/Volumes/NO NAME/FilMachine_Backup.json"
sync && diskutil eject "/Volumes/NO NAME"
```

Verifica all'avvio nel monitor: `Config JSON loaded: <N> processes`.
`settingsSize` deve combaciare col firmware e `rawProcessCount` col numero di processi nel file (se leggi un numero enorme, firmware e config non sono allineati → rigenera + rebuild pulito).

---

## 5. Pulizia

```bash
rm -rf build_sim          # cache simulatore
idf.py fullclean          # cache build ESP-IDF
rm -rf build800           # cache vecchia ereditata da Simulator_v2 (rigenerabile)
```

---

## 6. Git

```bash
git status
git add -A && git commit -m "messaggio"
git log --oneline -10
```

---

## Versione firmware

Fonte unica di verità: **`version.txt`** nella root (formato `MAJOR.MINOR.PATCH.BUILD`).
Lo stesso valore viene mostrato identico su **Splash** e su **Tools → Software version**, sia sul board che nel simulatore.

- Sul board: ESP-IDF usa `version.txt` come versione dell'app (`ota_get_running_version()`).
- Nel simulatore: iniettata via `-DFW_VERSION_STR` dal CMakeLists.
- **Auto-incremento**: `./flash_p4.sh` incrementa il campo BUILD a ogni build.

```bash
./scripts/bump_version.sh          # incrementa BUILD (0.0.0.1 -> 0.0.0.2)
./scripts/bump_version.sh --show   # mostra la versione corrente
```

Per un rilascio "vero" (major/minor/patch) modifica a mano `version.txt`, es. `0.1.0.0`.

---

## Costanti di taratura utili (main/include/FilMachine.h)

| Costante | Valore | Effetto |
|---|---|---|
| `MOTOR_MIN_ANALOG_VAL` | 90 | Duty motore allo slider al minimo (10%). Alza se il motore stenta a partire. |
| `MOTOR_MAX_ANALOG_VAL` | 255 | Duty motore al massimo (100%). |
| `PUMP_MIN_ANALOG_VAL` | 120 | Duty pompa allo slider al minimo (10%). Alza se la pompa non parte. |
| `PUMP_MAX_ANALOG_VAL` | 250 | Duty pompa al massimo (100%, limite sicuro DBH-12V). |
| `PUMP_DEFAULT_SPEED` | 200 | Fallback (azioni manuali/checkup). |
