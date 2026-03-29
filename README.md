# FilMachine — Automated Film Development Machine

FilMachine is an automated film processing machine designed for photographic film development. It handles the entire process: chemical baths, water rinses, temperature regulation, and motor-driven film agitation — all controlled through a 3.5" color touchscreen display.

The project includes a **desktop simulator** (SDL2 + LVGL) that reproduces the full touchscreen UI on macOS/Linux, enabling rapid development and testing without physical hardware, plus an **automated test suite** with 147 tests. All three supported boards compile from the same codebase with board selection at compile time. A **Flutter companion app** connects via WebSocket and provides full remote control from any mobile device on the same network.

---

## Table of Contents

1. [What is FilMachine?](#what-is-filmachine)
2. [Project Architecture](#project-architecture)
3. [Directory Structure](#directory-structure)
4. [Building the Project](#building-the-project)
5. [Running the Simulator](#running-the-simulator)
6. [Running the Tests](#running-the-tests)
7. [WebSocket Server & Remote Control](#websocket-server--remote-control)
8. [Flutter Companion App](#flutter-companion-app)
9. [User Interface Guide](#user-interface-guide)
10. [Firmware Update (OTA)](#firmware-update-ota)
11. [Hardware Specifications](#hardware-specifications)
12. [Configuration & Persistence](#configuration--persistence)
13. [Typical Workflows](#typical-workflows)
14. [Developer Quick Reference](#developer-quick-reference)
15. [Authors](#authors)

---

## What is FilMachine?

FilMachine takes the guesswork out of developing photographic film at home or in a small lab. Instead of manually timing each step, pouring chemicals by hand, and constantly checking the thermometer, FilMachine automates everything. You load your film in the developing tank, select a process, and press Start. The machine takes care of the rest: filling the right chemical at the right time, keeping the temperature stable, rotating the film at the correct speed, rinsing between steps, and alerting you when the process is complete.

It supports both **color negative** (C41, E6) and **black & white** film development workflows, with full customization of every parameter.

---

## Project Architecture

The project has a **multi-target build system** that produces ESP32-S3 firmware, ESP32-P4 firmware, or a native desktop application from the same codebase.

### Technology Stack

| Layer | Firmware (ESP32-S3) | Firmware (ESP32-P4) | Simulator (macOS/Linux) |
|-------|-------------------|-------------------|------------------------|
| **UI Library** | LVGL 9.2.2 | LVGL 9.2.2 (identical) | LVGL 9.2.2 (identical) |
| **Display** | ILI9488 480×320 (Makerfabs) / NV3041A 480×272 (JC4827W543) | ST7701S 480×800 MIPI-DSI → landscape 480×320 via PPA (JC4880P433) | SDL2 window (any resolution) |
| **Touch Input** | FT6236 (Makerfabs) / GT911 (JC4827W543) | GT911 (physical 480×800 → remapped to 480×320) | SDL2 mouse events |
| **Storage** | FatFS on MicroSD (SPI) | FatFS on MicroSD (SDMMC 4-bit, ~10× faster) | POSIX file I/O (sd/ directory) |
| **RTOS** | FreeRTOS (ESP-IDF) | FreeRTOS (ESP-IDF) | Stub (queues work, tasks are no-ops) |
| **2D Acceleration** | — (CPU only) | PPA hardware engine (rotate, scale, blend, fill) | — |
| **Audio** | — (Makerfabs) / I2S speaker (JC4827W543) | ES8311 codec via I2S + power amplifier | — |
| **Temp Sensors** | DS18B20 OneWire | DS18B20 OneWire | Simulated (20°C ambient, heater model) |
| **Motor/Relays** | GPIO + MCP23017 I2C | GPIO + MCP23017 I2C | printf stubs |
| **OTA Updates** | esp_ota_ops + esp_http_server | esp_ota_ops + esp_http_server | Simulated (progress timer) |
| **Build Tool** | ESP-IDF (`idf.py`) | ESP-IDF (`idf.py`) | CMake + Make |
| **Compiler** | xtensa-esp32s3-elf-gcc | riscv32-esp-elf-gcc | Native cc/gcc/clang |

### How the Simulator Works

The simulator replaces all hardware-specific code with a **stub layer** that provides compatible implementations:

- **FatFS stubs** map firmware paths (e.g., `/FilMachine.cfg`) to files inside the `sd/` subdirectory relative to the executable, so read/write operations work transparently.
- **FreeRTOS stubs** provide working queue implementations (xQueueCreate, xQueueSend, xQueueReceive) using simple circular buffers. Task creation is a no-op since there are no real threads — instead, the main loop drains the system queue every frame.
- **Hardware stubs** (GPIO, I2C, SPI, relay, motor, temperature) are either no-ops or printf-based logging functions. Temperature readings come from a simple thermal model that simulates heating/cooling.
- **OTA stubs** simulate the firmware update flow with a progress timer (0→100%) and fake IP address, allowing full UI testing without real hardware.
- **LVGL's SDL2 driver** provides the display backend and mouse input, which simulates the capacitive touchscreen.

The key architectural principle is that **all UI code is shared 1:1** between firmware and simulator. Pages, elements, event handlers, and business logic are identical — only the hardware abstraction layer changes.

---

## Directory Structure

```
FilMachine_Simulator_v2/
│
├── main/                          # Core source (shared with ESP32 firmware)
│   ├── include/
│   │   ├── FilMachine.h           #   Main header — all structs, enums, constants, prototypes
│   │   ├── ds18b20.h              #   Temperature sensor driver API
│   │   └── pca9685.h              #   PWM controller driver API
│   ├── FilMachine.c               #   ESP32 entry point — board-conditional display/touch init
│   ├── accessories.c              #   Utilities — linked lists, deep copy, config I/O, keyboard
│   ├── ota_update.c               #   OTA firmware update (SD card + Wi-Fi web server)
│   ├── sensors.c                  #   Additional sensors (flow meter, water level, hall effect)
│   ├── ui_profile.c               #   Centralized UI layout constants for dual-resolution support
│   ├── ds18b20.c                  #   DS18B20 OneWire temperature sensor driver
│   ├── mcp23017.c                 #   MCP23017 I2C GPIO expander driver
│   └── pca9685.c                  #   PCA9685 I2C PWM controller driver
│
├── c_pages/                       # UI pages (screens)
│   ├── page_splash.c              #   Splash screen — standard, random, and custom modes with
│   │                              #     10 palettes, 6 shape styles, 9 title fonts, PRNG engine
│   ├── page_home.c                #   Home screen & boot error display
│   ├── page_menu.c                #   Tab bar navigation (Processes / Settings / Tools)
│   ├── page_processes.c           #   Process list with filtering
│   ├── page_processDetail.c       #   Process creation & editing
│   ├── page_stepDetail.c          #   Step creation & editing with validation
│   ├── page_settings.c            #   Machine settings (temp, speed, alarms, timers, Wi-Fi)
│   ├── page_tools.c               #   Maintenance tools, import/export, statistics, OTA
│   └── page_checkup.c             #   Process execution — the most complex page
│
├── c_elements/                    # Reusable UI components
│   ├── element_process.c          #   Process list item (drag, delete, duplicate)
│   ├── element_step.c             #   Step list item (swipe gestures, edit/delete)
│   ├── element_filterPopup.c      #   Filter dialog (name, type, preferred)
│   ├── element_messagePopup.c     #   Generic confirmation/alert popups
│   ├── element_rollerPopup.c      #   Numeric selector (roller) widget
│   ├── element_cleanPopup.c       #   Cleaning process UI with timer
│   ├── element_drainPopup.c       #   Drain process UI with animated tank bars
│   ├── element_splashPopup.c      #   Splash screen config popup with live preview
│   ├── element_selfcheckPopup.c   #   Self-check diagnostic wizard (7-phase hardware test)
│   └── element_otaWifiPopup.c     #   Wi-Fi OTA update popup (IP + PIN + progress)
│
├── c_fonts/                       # Custom icon fonts (5 sizes: 15/20/30/40/100px)
│   │                              #   + 8 custom splash title fonts (size 48px each)
├── drivers/                       # Custom peripheral drivers (MCP23017, PCA9685, DS18B20, sensors)
│   ├── include/                   #   Driver headers
│   ├── mcp23017.c                 #   I2C 16-bit I/O expander (relay control)
│   ├── pca9685.c                  #   I2C PWM controller (pump speed)
│   ├── ds18b20.c                  #   OneWire temperature sensor
│   └── sensors.c                  #   Flow meter, water level, hall effect sensors
│
├── components/                    # ESP32-P4 specific hardware drivers
│   ├── st7701_lcd/                #   ST7701S MIPI-DSI LCD driver (480×800)
│   └── ppa_engine/                #   PPA hardware 2D accelerator (rotate, scale, fill, blend)
│
├── src/
│   └── main.c                     # Simulator entry point (SDL2 display, main loop,
│                                  #   demo data generator, system queue drain)
│
├── stub/                          # Hardware abstraction for simulator
│   ├── esp_stubs.h/c              #   ESP32 GPIO, timer, heap stubs
│   ├── fatfs_stubs.h/c            #   FatFS → POSIX filesystem mapping
│   ├── freertos_stubs.h/c         #   Queue stubs (circular buffer), task stubs (no-op)
│   ├── driver/                    #   GPIO, I2C, LEDC, SDMMC driver stubs
│   └── (redirect headers)         #   Thin #include wrappers for ESP-IDF compatibility
│
├── tests/                         # Automated test suite
│   ├── test_runner.h/c            #   Test framework & entry point
│   ├── test_helpers.c             #   Touch input simulation
│   ├── test_navigation.c          #   Splash, menu, tab switching
│   ├── test_processes.c           #   Process list display & creation
│   ├── test_process_crud.c        #   Process create/read/update/delete
│   ├── test_steps.c               #   Step creation, swipe, deletion
│   ├── test_step_crud.c           #   Step lifecycle
│   ├── test_execution.c           #   Process checkup execution flow
│   ├── test_persistence.c         #   Config save/load/restore
│   ├── test_settings.c            #   Settings UI & validation
│   ├── test_filter.c              #   Filter logic
│   ├── test_tools.c               #   Maintenance tools
│   ├── test_edge_cases.c          #   Boundary conditions & error paths
│   ├── test_utilities.c           #   Helper function tests
│   └── test_destroy_and_lifecycle.c # Memory cleanup & object destruction
│
├── lvgl/                          # LVGL 9.2.2 library (auto-cloned on first build)
├── lvgl_config/
│   └── lv_conf.h                  # LVGL configuration (RGB565, 256KB heap, dark theme)
│
├── sd/                            # SD card simulation directory
│   ├── FilMachine.cfg             #   Binary config (processes + settings + stats)
│   ├── FilMachine_Backup.cfg      #   Backup copy (created by Export)
│   └── FilMachine.json            #   Human-readable JSON export
│
├── scripts/
│   └── genFilMachineCFG.py        # Config generator (realistic film recipes)
├── resources/                     # Hardware datasheets & font usage docs
│
├── CMakeLists.txt                 # Multi-target build (ESP-IDF S3/P4 + simulator + tests)
├── sdkconfig                      # ESP-IDF configuration (partition table, Wi-Fi, etc.)
├── sdkconfig.defaults.esp32p4     # ESP-IDF defaults for ESP32-P4 target
├── setup.sh                       # Project initialization script
├── flash.sh                       # Flash firmware to ESP32 board
├── ANALISI_PROGETTO.md            # Technical analysis (Italian)
└── wiring_philosophy.md           # Hardware design philosophy
```

---

## Building the Project

### Prerequisites

**macOS:**
```bash
brew install cmake sdl2 pkg-config
```

**Ubuntu / Debian:**
```bash
sudo apt install cmake libsdl2-dev pkg-config build-essential
```

LVGL 9.2.2 is automatically cloned from GitHub on the first build if not present.

### Build the Simulator

```bash
# 480×320 (default — Makerfabs S3 and JC4880P433 P4)
mkdir -p build320 && cd build320
cmake ..
make filmachine_sim

# 480×272 (JC4827W543 resolution)
mkdir -p build272 && cd build272
cmake .. -DCMAKE_C_FLAGS="-DSIM_RESOLUTION=272"
make filmachine_sim

# 800×480 landscape (JC4880P433 physical panel — layout exploration)
mkdir -p build800 && cd build800
cmake .. -DCMAKE_C_FLAGS="-DSIM_RESOLUTION=800"
make filmachine_sim
```

The first two resolution modes match the two distinct LVGL resolutions used across the three boards. The JC4880P433 (ESP32-P4) has a 480×800 physical panel used in **landscape (800×480)**. On real hardware the PPA engine rotates and scales the 480×320 LVGL framebuffer to fill the panel; that pipeline doesn't exist in the simulator, so both the Makerfabs and the P4 map to the default 320 build. The 800 mode opens a full 800×480 landscape window matching the physical panel, useful for designing a native-resolution layout — the current UI renders on the left and the empty space to the right shows how much room is available for expansion.

This produces the `filmachine_sim` executable.

### Build the Tests

```bash
mkdir -p build320 && cd build320
cmake ..
make filmachine_test
./filmachine_test
```

### Build the Firmware

Requires the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/) toolchain installed and configured. ESP32-S3 boards require ESP-IDF 5.x; the ESP32-P4 board requires **ESP-IDF 5.5.x** or later.

**Important:** run `idf.py` from the **project root**, not from `build320/`.

#### ESP32-S3 boards (Makerfabs / JC4827W543)

```bash
cd FilMachine_Simulator_v2
. $HOME/esp/esp-idf/export.sh

# Makerfabs S3 (default board — ILI9488 480×320)
idf.py build

# JC4827W543 (NV3041A 480×272)
idf.py fullclean
idf.py -D CMAKE_C_FLAGS="-DBOARD_JC4827W543" build
```

#### ESP32-P4 board (JC4880P433)

```bash
cd FilMachine_Simulator_v2
. $HOME/esp/esp-idf-v5.5/export.sh     # Must be ESP-IDF 5.5.x or later

# First-time setup: set target to esp32p4 (creates sdkconfig from defaults)
idf.py set-target esp32p4

# Build with P4 board flag
idf.py -D CMAKE_C_FLAGS="-DBOARD_JC4880P433" build
```

Note: switching between ESP32-S3 and ESP32-P4 targets requires `idf.py fullclean` because the toolchains are different (Xtensa vs RISC-V).

#### Flash and monitor

The firmware binary is produced at `build/FilMachine.bin`. To flash it directly to a connected board:

```bash
idf.py flash
```

Or use the helper script:
```bash
./flash.sh
```

**P4 note:** the bootloader offset on ESP32-P4 is `0x2000` (different from ESP32-S3's `0x0`). This is handled automatically by `idf.py` when the target is set correctly.

To check the firmware size (useful for verifying OTA partition fit):
```bash
idf.py size
```

---

## Running the Simulator

### Simulator UI debug overlay

The simulator includes a built-in UI inspection mode to speed up layout tuning.

- Press **F2** to enable or disable the overlay.
- When enabled, moving the mouse over the UI shows:
  - the resolved component name when available (for example `gui.page.processes.newProcessButton`)
  - the object position (`x`, `y`)
  - the object size (`w`, `h`)
- While the overlay is enabled, **right click** dumps the hovered object to the terminal/console.

This is especially useful when adjusting coordinates inside `ui_profile.c`, because you can identify the exact widget and immediately see its current geometry in the simulator.


```bash
cd build2
./filmachine_sim
```

The simulator opens a 480x320 window that reproduces the exact touchscreen interface. Use the mouse to simulate touch input (click = tap, click-drag = swipe/scroll).

### What works in the simulator

- Full UI navigation (all pages, popups, dialogs)
- Splash screen with live preview popup, 10 palettes, 6 shape styles, 9 custom fonts
- Process creation, editing, duplication, deletion
- Step management with swipe gestures
- Settings with slider/switch/radio controls
- Filter and search functionality
- Configuration save/load (reads/writes `sd/FilMachine.cfg`)
- Export/Import (backup to `sd/FilMachine_Backup.cfg`)
- Drain machine with animated tank-level bars and relay management
- Clean machine with per-container rinse cycles and arc progress
- Self-check diagnostic wizard with 7-phase hardware test simulation
- OTA update UI (SD card check + Wi-Fi popup with simulated IP/PIN/progress)
- Simulated temperature readings with heater model and calibration offset
- Persistent alarm sound (SDL audio 880Hz beep, repeats every 10s until dismissed)
- Drain/fill overlap processing (adjusts step timing based on overlap percentage)
- Machine statistics persistence (saved in config file, survives restart)
- Temperature sensor calibration (Tune button calculates and applies offset)
- Console logging of all system actions (`[SIM] sysAction: ...`)
- WebSocket server on port 81 for Flutter companion app remote control
- Full remote process start, stop, and checkup advance via WebSocket
- Remote CRUD for processes and steps (create, edit, delete from Flutter)

### What is simulated (no real hardware)

- Temperature sensors return simulated values (20°C ambient, gradual heating/cooling)
- Motor control outputs to console only
- Relay switching outputs to console only
- OTA writes to a fake partition (progress simulated, no actual flash)
- Wi-Fi connection is simulated (fake IP 192.168.1.42, no real server)
- Reboot is a no-op (prints message)

### Generating sample data

To generate realistic film development recipes:

```bash
python3 scripts/genFilMachineCFG.py --realistic --output build2/sd/
```

This creates config files with authentic C41, E6, and B&W development processes.

---

## Running the Tests

```bash
cd build2
./filmachine_test
```

Results are displayed in the terminal and saved to `test_results/test_results_YYYYMMDD_HHMMSS.txt`.

### Test Suites

| Suite | Description |
|-------|-------------|
| **Navigation** | Splash screen, menu display, tab switching |
| **Processes** | Process list rendering, creation UI |
| **Process CRUD** | Full create/read/update/delete lifecycle |
| **Steps** | Step creation, swipe gestures, deletion |
| **Step CRUD** | Step lifecycle with validation |
| **Execution** | Process checkup and execution flow |
| **Persistence** | Config save → reload → verify all fields, stats persistence, calibration offset |
| **Settings** | Settings UI, default values, slider/switch behavior, calibration, alarm |
| **Filter** | Filter by name, film type, preferred flag |
| **Tools** | Cleaning, draining, statistics display, timer pause/resume, alarm functions |
| **Edge Cases** | Boundary conditions, max limits, error recovery |
| **Utilities** | Helper functions, linked list operations, drain/fill overlap calculation |
| **UI Profile & Sensors** | Profile values match original, popup dimensions, font pointers, sensor stubs, board constants |
| **Destroy & Lifecycle** | Memory cleanup, object destruction |

The persistence tests verify that every field (process name, temperature, tolerance, film type, preferred flag, step names, durations, types, sources, discard flags) survives a full save-and-reload cycle.

---

## WebSocket Server & Remote Control

The simulator and firmware include a built-in WebSocket server (`ws_server.c`) that enables remote control from the Flutter companion app or any WebSocket client. The server starts automatically and listens on **port 81** at path `/ws`.

### Supported Commands

All commands use JSON format: `{"cmd":"command_name", ...params}`.

| Command | Parameters | Description |
|---------|-----------|-------------|
| `get_state` | — | Returns full machine state (80+ fields: settings, runtime, temperatures, progress, alarms) |
| `get_processes` | — | Returns complete process list with steps, indexed for remote referencing |
| `start_process` | `index` | Start a process by list index; initializes checkup and begins execution |
| `checkup_advance` | — | Advance to next checkup phase (Setup → Fill → Temp → Check → Processing) |
| `stop_now` | — | Immediately halt the running process and drain |
| `stop_after` | — | Stop after the current step completes |
| `close_process` | — | Close a finished/stopped process and free checkup resources |
| `create_process` | `name, temp, tolerance, filmType, tempControlled, preferred` | Create a new process |
| `edit_process` | `index, name, temp, tolerance, filmType, tempControlled, preferred` | Edit an existing process |
| `delete_process` | `index` | Delete a process |
| `add_step` | `processIndex, name, mins, secs, type, source, discard` | Add a step to a process |
| `edit_step` | `processIndex, stepIndex, name, mins, secs, type, source, discard` | Edit a step |
| `delete_step` | `processIndex, stepIndex` | Delete a step |
| `set_setting` | `key, value` | Update a machine setting |

### Implementation Details

The server uses simple `strstr()` JSON parsing with no external library (no cJSON, no heap allocation for parsing). All commands that modify LVGL state are dispatched via `lv_async_call()` to ensure thread-safe UI updates. State changes are automatically broadcast to all connected clients.

---

## Flutter Companion App

The **filmachine_app** is a Flutter application that provides full remote control of FilMachine from any mobile device or desktop on the same network. See the [filmachine_app README](../filmachine_app/README.md) for detailed documentation.

### Key Features

- **Device Discovery**: Automatic mDNS discovery of FilMachine devices on the local network (`_filmachine._tcp`), plus manual IP/port entry
- **Process Management**: Create, edit, duplicate, and delete processes and steps — all changes sync to the machine in real-time
- **Live Execution Monitoring**: Real-time dashboard showing current step, progress bars, temperatures, tank fill status, and elapsed/remaining time
- **Process Control**: Start processes, advance through checkup phases, Stop Now / Stop After with confirmation dialogs
- **Filtering**: Client-side filtering by name, film type (B&W / Color), and preferred flag
- **Statistics**: View completed processes, stopped processes, total development time, cleaning cycles
- **Settings**: Full access to all machine settings (temperature unit, rotation speed, autostart, alarms, Wi-Fi config, etc.)
- **Theme**: Dark/light theme toggle with custom FilMachine color palette
- **Persistent Connection**: Remembers last successful connection for quick reconnect

### Architecture

The app uses **Provider** for state management. A central `MachineService` (ChangeNotifier) maintains the WebSocket connection and holds the current `MachineState` — a data class with 80+ fields deserialized from the JSON state broadcast. All screens rebuild reactively when state changes.

---

## User Interface Guide

### The Processes Tab

This is where you manage your film development recipes. Each process is a sequence of steps the machine executes in order. The list shows each process with its name, total time, film type icon, and a star if marked as preferred.

- **Create:** Tap "+" to add a new process
- **Edit:** Tap a process to open its detail view
- **Duplicate:** Swipe left to reveal the duplicate button
- **Delete:** Open the detail, tap the trash icon
- **Filter:** Tap the filter icon to search by name, film type, or preferred status. The filter icon turns green when a filter is active, and returns to white when reset.

### Working with Steps

Each process contains steps. A step represents one phase: "Developer", "Stop Bath", "Fixer", "Wash", etc.

For each step you configure: name, duration (minimum 30 seconds), chemical type (Chemistry / Rinse / Multi-Rinse), source container (C1, C2, C3, or WB), and whether to discard the chemical after use.

- **Add:** Tap "+" below the step list
- **Edit:** Tap a step to modify it
- **Duplicate:** Swipe left on a step
- **Delete:** Swipe left to reveal the delete button
- **Reorder:** Long-press and drag

### Running a Process (Checkup)

Open a process and tap Play. The machine walks through pre-flight checks, then executes each step automatically.

**Pre-flight checks:** tank size selection, water bath fill, tank presence verification, motor check, and temperature stabilization (for temperature-controlled processes).

**During execution:** the screen shows the current step name and remaining time, chemical source, current/target temperature, and upcoming steps.

**Stopping:** "Stop now!" halts immediately (potentially unsafe). "Stop after!" waits for the current step to finish — the safer choice.

### Splash Screen

The splash screen appears at boot and supports three modes, configured via Settings → Splash Screen:

**Use Default** — The built-in "Deep Ocean" splash: a hand-tuned composition with a teal-to-navy gradient, geometric shapes, and the "FILMACHINE" title in Montserrat 48. This is the factory default.

**Random next boot** — Each boot generates a completely new splash from scratch using a tick-derived seed. The PRNG (xorshift32) deterministically selects a palette, shape style, complexity, title position, and title font — producing a unique visual identity every time the machine starts.

**Custom** — Fine-tune the splash parameters manually: Palette (10 options), Shape Style (6 options), and Complexity (20–100 in steps of 20). The popup shows a live preview of the current configuration as the background behind its own controls, with a dark overlay for readability. Press the Random button to regenerate all parameters at once; if you like the result, it will be applied at next boot. The preview shows only the shape background (no title or play button) so you can evaluate the pattern clearly.

**Title fonts** — In Random and Custom modes, the title "FILMACHINE" is rendered with one of 9 fonts selected by the PRNG:

| Index | Font Name | Style |
|-------|-----------|-------|
| 0 | Montserrat 48 | Default sans-serif (same as Deep Ocean) |
| 1 | Air Americana | Bold italic display |
| 2 | Decaying Felt Pen | Hand-drawn brush effect |
| 3 | DS Digital | LED/LCD digital display |
| 4 | Evanescent | Ethereal and elegant |
| 5 | Nerdropol Lattice | Geometric/tech |
| 6 | Retrolight | Retro/vintage |
| 7 | Tropical Leaves | Decorative botanical |
| 8 | Wishful Melisande | Calligraphic script |

All fonts are converted from TTF/OTF to LVGL `.c` bitmap arrays using `lv_font_conv` with `--no-compress --bpp 4 --range 0x20-0x7F` (ASCII only, ~20–40KB each in flash).

**Palettes** — Cyberpunk, Aurora, Lava, Deep Ocean, Forest, Sunset, Machinery, Arctic, Neon, Pastel. Each defines a top/bottom gradient, 3 shape accent colors, and text/accent colors for the title.

**Shape Styles** — Overlapping Rects, Circles & Arcs, Mixed Shapes, Diagonal Bands, Grid Blocks, Radial Arcs.

### The Settings Tab

| Setting | Description | Range |
|---------|-------------|-------|
| Splash Screen | Opens a popup to configure the boot splash (see above) | Default / Random / Custom |
| Temperature unit | °C or °F | — |
| Water inlet | Automatic water fill if connected | On/Off |
| Temp sensor calibration | Calibrate against a reference thermometer. Short-press Tune to set ambient temp, long-press to reset. | Tune button |
| Rotation speed | Film agitation motor RPM | 10–100% |
| Inversion interval | Seconds between motor direction changes | 10–60s |
| Randomness | Random variation on inversion interval | 0–100% |
| Persistent alarm | Alarm sounds until acknowledged | On/Off |
| Process autostart | Auto-start when temperature reached | On/Off |
| Drain/fill overlap | How much of fill/drain time counts as processing time (100% recommended) | 0–100% |
| Multi-rinse cycle time | Duration of each rinse in multi-rinse steps | 60–180s |
| Pump speed | Water pump speed percentage | 0–100% |
| Tank size | Default developing tank size | S (500ml) / M (700ml) / L (1000ml) |
| Container size | Chemistry container capacity | 250–2000 ml |
| Water bath size | Water bath capacity | 1000–5000 ml |
| Chemistry volume | Amount of chemistry used per step | Low / High |
| Wi-Fi SSID | Network name for OTA updates | Text (max 32 chars) |
| Wi-Fi password | Network password for OTA updates | Text (max 64 chars) |

All settings are saved automatically to the SD card when changed. Slider values are saved only when you release the slider (not during dragging) to reduce SD card wear.

### The Tools Tab

- **Clean machine** — Automated cleaning cycle: select which containers to clean (C1, C2, C3), set the number of rinse cycles, and optionally drain the water bath when done. Each cycle fills the container with water from the water bath and then drains it back, with real-time progress shown via arc animations and remaining-time countdown.
- **Drain machine** — Drains all containers (C1, C2, C3, WB) to waste sequentially. A confirmation screen lists the affected containers; once started, four colored tank bars animate from full to empty in real time, showing which container is currently draining, a ">> WASTE <<" indicator, and a countdown timer. The drain can be stopped at any time via the Stop button.
- **Self-check** — Guided hardware diagnostic wizard that tests all machine components in 7 phases: temperature sensors (5s), water pump (20s), heater (30s), valves (10s), and the three containers C1/C2/C3 (10s each). The UI is split in two panels: a task list on the left showing icons per phase (check for done, dot for pending/skipped/stopped) and a detail panel on the right with phase description, real-time sensor data, countdown timer, and a progress bar. Three buttons control the flow: Stop (halts current phase), Start/Re-run (begins or repeats a phase), and Next (skips to the next phase). Each phase's state (done, skipped, stopped) is saved and visible when revisiting. When all phases complete successfully the title shows "Self-check complete!" in green; if any were skipped or stopped it shows "Self-check finished" in orange.
- **Import/Export** — Backup and restore configuration to SD card
- **Statistics** — Completed processes, total time, cleaning cycles, stopped processes
- **Software info** — Firmware version (read from the running binary, also shown on the splash screen) and serial number
- **Update from SD** — Firmware OTA update from a `.bin` file on the SD card. The system reads the firmware version from the binary header, asks for confirmation, then writes it to the secondary OTA partition. After completion, a reboot applies the new firmware. If the update fails, the bootloader automatically rolls back to the previous version.
- **Wi-Fi update** — Starts a local web server on the board. A popup shows the board's IP address (e.g. `http://192.168.1.42`) and a randomly generated 5-digit PIN for security. The user opens the URL in any browser on the same network and sees a drag-and-drop upload page styled with the FilMachine branding. After uploading the `.bin` firmware file, it is streamed directly to the OTA partition. The board reboots automatically when complete. Wi-Fi credentials (SSID/password) are configured in the Settings tab.

---

## Firmware Update (OTA)

The board supports two methods for over-the-air firmware updates. Both use the ESP-IDF dual-partition OTA mechanism: the new firmware is written to a secondary partition while the current one keeps running, and the bootloader swaps at reboot. If the new firmware fails to boot, automatic rollback restores the previous version.

### Method 1: SD Card

1. Build the firmware: `idf.py build`
2. Copy `build/FilMachine.bin` to the SD card root, renamed as `FilMachine_fw.bin`
3. Insert the SD card into the machine
4. Go to Tools → "Update from SD" and press the play button
5. Confirm the version shown in the popup
6. Wait for the progress to reach 100%, then reboot

### Method 2: Wi-Fi (Web Upload)

1. Configure Wi-Fi SSID and password in Settings
2. Go to Tools → "Wi-Fi update" and press the play button
3. The popup shows the board's local IP and a 5-digit PIN
4. On any device on the same network, open `http://<board-ip>` in a browser
5. You'll see a drag-and-drop page — drop the `.bin` file or click to select it
6. The firmware uploads in streaming and the board reboots automatically

### Firmware File Format

The `.bin` file is produced by `idf.py build` at `build/FilMachine.bin`. It contains an ESP32 application image with an `esp_app_desc_t` header that stores the firmware version, build date, and SHA-256 hash for integrity verification.

### Partition Table

The project uses a custom partition table (`partitions.csv`) optimized for the 16MB flash chip. Each OTA slot is 6MB, giving ~68% free space with the current firmware size (~1.9MB).

```
# Name      Type  SubType  Offset      Size
nvs         data  nvs      0x9000      24KB     NVS key-value storage
otadata     data  ota      0xF000       8KB     OTA boot flag (which slot to boot)
phy_init    data  phy      0x11000      4KB     Wi-Fi PHY calibration data
ota_0       app   ota_0    0x20000      6MB     Firmware slot A (active)
ota_1       app   ota_1    0x620000     6MB     Firmware slot B (updated via OTA)
(free)      —     —        0xC20000    ~4MB     Unallocated
```

The bootloader alternates between `ota_0` and `ota_1`: the new firmware is written to the inactive slot, and at reboot the bootloader swaps. If the new firmware crashes at startup, automatic rollback restores the previous working version.

### Setting the Firmware Version

The version displayed in Tools → Software version is read at runtime from the running binary via `esp_app_get_description()->version`. To set it, create a `version.txt` file in the project root containing the version string (e.g., `v1.0.0`), or set it in the main `CMakeLists.txt` via `project(FilMachine VERSION 1.0.0)`.

---

## Hardware Specifications

### Supported Boards

The firmware supports three boards selected at compile time via `-DBOARD_xxx`:

**Makerfabs MaTouch ESP32-S3** (`-DBOARD_MAKERFABS_S3`, default) — 3.5" ILI9488 480×320 parallel TFT, FT6236 touch, 16MB flash, 2MB PSRAM. The original board.

**Guition JC4827W543C** (`-DBOARD_JC4827W543`) — 4.3" NV3041A 480×272 IPS QSPI, GT911 touch, 4MB flash, 8MB PSRAM, I2S speaker, flow meter, water level sensor, hall sensor.

**Guition JC4880P433** (`-DBOARD_JC4880P433`) — 4.3" ST7701S 480×800 IPS via MIPI-DSI, GT911 touch, 16MB flash, 32MB PSRAM, ES8311 audio codec, SDMMC 4-bit SD, PPA hardware accelerator. Uses the **ESP32-P4** (RISC-V dual-core @ 400 MHz), a significant step up in processing power and memory.

### Board Comparison

| Feature | Makerfabs S3 | JC4827W543 (S3) | JC4880P433 (P4) |
|---------|-------------|-----------------|-----------------|
| **MCU** | ESP32-S3 (Xtensa, 240 MHz) | ESP32-S3 (Xtensa, 240 MHz) | ESP32-P4 (RISC-V, 400 MHz) |
| **Flash / PSRAM** | 16 MB / 2 MB | 4 MB / 8 MB | 16 MB / 32 MB |
| **Display** | ILI9488 3.5" 480×320 | NV3041A 4.3" 480×272 | ST7701S 4.3" 480×800 |
| **Display bus** | 16-bit parallel (I80) | QSPI | MIPI-DSI (2-lane) |
| **LVGL resolution** | 480×320 (native) | 480×272 (native) | 480×320 (PPA rotated+scaled) |
| **Touch** | FT6236 (I2C) | GT911 (I2C) | GT911 (I2C) |
| **SD card** | SPI (~4 MB/s) | SPI (~4 MB/s) | SDMMC 4-bit (~40 MB/s) |
| **Audio** | — | I2S speaker (3 pins) | ES8311 codec + PA |
| **2D accelerator** | — | — | PPA (rotate, scale, blend, fill) |
| **Sensors** | — | Flow, water level, hall | Flow, water level, hall |
| **Compile flag** | `BOARD_MAKERFABS_S3` | `BOARD_JC4827W543` | `BOARD_JC4880P433` |
| **ESP-IDF version** | 5.x | 5.x | 5.5.x+ |

### P4 Landscape Mode

The JC4880P433's physical display is 480×800 in portrait orientation. FilMachine uses it in **landscape mode at 480×320**, matching the original Makerfabs resolution. The display pipeline works as follows:

1. LVGL renders the UI into a 480×320 framebuffer (identical to the Makerfabs S3)
2. The PPA hardware engine rotates the buffer 90° → 320×480
3. PPA scales ×1.5 → 480×720
4. The scaled image is drawn centred on the 480×800 panel (40 px border top and bottom)

Touch coordinates from the GT911 (which reports in 480×800 portrait) are inverse-mapped back to 480×320 LVGL coordinates. All of this is transparent to the UI code — the same `ui_profile_480x320` profile is used.

### Shared Peripherals (all boards)

| Component | Detail |
|-----------|--------|
| **I/O Expander** | MCP23017 (I2C) — 8 relay outputs |
| **PWM Controller** | PCA9685 (I2C) — pump speed control |
| **Temperature** | 2× DS18B20 OneWire sensors (chemical bath + water bath) |
| **Motor** | DC motor with H-bridge, PWM speed control |
| **Relays** | Heater, 3 chemical valves (C1/C2/C3), water bath, waste, pump in, pump out |
| **Power** | USB Type-C 5.0V |

### Chemical Container Layout

| Label | Purpose | Typical Use |
|-------|---------|-------------|
| **C1** | Chemical container 1 | Developer |
| **C2** | Chemical container 2 | Bleach / Stop bath |
| **C3** | Chemical container 3 | Fixer |
| **WB** | Water bath | Rinse water (temperature-controlled) |
| **WASTE** | Waste drain | Discarded chemicals |

### Constraints

| Parameter | Limit |
|-----------|-------|
| Max processes | 50 |
| Max steps per process | 30 |
| Process/step name length | 20 characters |
| Minimum step duration | 30 seconds |

---

## Configuration & Persistence

All data is stored on the SD card in binary format (`FilMachine.cfg`). The file contains the complete machine state: settings (including calibration offset and splash screen configuration — default/random flags, palette, shape style, complexity, seed), all processes with their steps, and machine statistics (completed processes, total processing time, stopped processes, completed cleaning cycles).

**Auto-save triggers:** creating/editing/deleting a process, changing any setting, toggling preferred, duplicating processes or steps.

**Export** creates a backup copy (`FilMachine_Backup.cfg`) on the same SD card. **Import** restores from the backup and reboots the machine.

In the simulator, these files live in the `sd/` subdirectory inside the build folder (e.g., `build2/sd/FilMachine.cfg`).

---

## Typical Workflows

### C41 Color Negative (38°C)

A standard C41 process: Pre-wash with water (1:00), Developer from C1 (3:15), Bleach from C2 (6:30), Wash (3:00), Fixer from C3 (6:30), Final Wash (3:00). Temperature control is critical — the machine maintains 38°C ± 0.3°C throughout.

### Black & White (20°C)

B&W processes are more flexible: Developer from C1 (8:00), Stop Bath from C2 (1:00), Fixer from C3 (5:00), Wash (5:00). Temperature control can be disabled for ambient-temperature development, or set to 20°C ± 0.5°C for consistency.

### E6 Slide Film (38°C)

E6 requires precise timing and temperature: First Developer (6:00), Wash (2:00), Color Developer (6:00), Wash (2:00), Bleach (6:00), Fixer (4:00), Final Wash (4:00). All at 38°C with tight ±0.3°C tolerance.

---

## Developer Quick Reference

### Build Commands

```bash
# ── Simulator (macOS/Linux) ──────────────────────────────
mkdir -p build2 && cd build2
cmake .. -DTARGET=sim            # Configure (first time only)
make filmachine_sim              # Build simulator
./filmachine_sim                 # Run simulator

# ── Tests ────────────────────────────────────────────────
make filmachine_test             # Build tests
./filmachine_test                # Run all tests

# ── Firmware (ESP32-S3) ──────────────────────────────────
cd FilMachine_Simulator_v2       # Must be in project root!
idf.py build                     # Makerfabs S3 (default)
idf.py -D CMAKE_C_FLAGS="-DBOARD_JC4827W543" build  # JC4827W543
idf.py flash                     # Flash to connected board
idf.py flash monitor             # Flash + open serial monitor
idf.py monitor                   # Serial monitor only (Ctrl+] to exit)
idf.py size                      # Show firmware size breakdown

# ── Firmware (ESP32-P4) ──────────────────────────────────
idf.py set-target esp32p4       # First time: set RISC-V target
idf.py -D CMAKE_C_FLAGS="-DBOARD_JC4880P433" build
idf.py flash                     # Flash (bootloader offset 0x2000)

# ── Switching between S3 and P4 ─────────────────────────
idf.py fullclean                 # Required when switching target
idf.py set-target esp32s3        # Back to S3 (or esp32p4 for P4)
```

### Rebuilding After Code Changes

```bash
# Simulator: just run make again (incremental)
cd build2 && make filmachine_sim

# If you added new .c files to CMakeLists.txt, re-run cmake first:
cd build2 && cmake .. -DTARGET=sim && make filmachine_sim

# Firmware: idf.py handles incremental builds automatically
idf.py build
```

### Generating a Firmware Update File

```bash
idf.py build
# The .bin file is at: build/FilMachine.bin
# For SD card update, rename and copy:
cp build/FilMachine.bin /Volumes/SDCARD/FilMachine_fw.bin
```

### Generating Sample Data

```bash
python3 scripts/genFilMachineCFG.py --realistic --output build2/sd/
```

### Common Troubleshooting

```bash
# "CMakeLists.txt not found" when running idf.py
# → You're in the wrong directory. Run from the project root, not build2/

# Simulator won't compile after adding a new .c file
# → Re-run cmake: cd build2 && cmake .. -DTARGET=sim

# LVGL not found
# → Run setup.sh or clone manually: git clone https://github.com/lvgl/lvgl.git

# Serial monitor garbled output
# → Check baud rate: idf.py monitor -b 115200
```

---

## License

Private project. All rights reserved.

## Authors

**PeterB** — Hardware design & firmware
**FrankP** — Software development & testing
