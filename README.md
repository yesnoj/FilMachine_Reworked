# FilMachine — Automated Film Development Machine

FilMachine is an automated film processing machine designed for photographic film development. It handles the entire process: chemical baths, water rinses, temperature regulation, and motor-driven film agitation — all controlled through a 3.5" color touchscreen display.

The project includes a **desktop simulator** (SDL2 + LVGL) that reproduces the full touchscreen UI on macOS/Linux, enabling rapid development and testing without physical hardware, plus an **automated test suite** with 141 tests.

---

## Table of Contents

1. [What is FilMachine?](#what-is-filmachine)
2. [Project Architecture](#project-architecture)
3. [Directory Structure](#directory-structure)
4. [Building the Project](#building-the-project)
5. [Running the Simulator](#running-the-simulator)
6. [Running the Tests](#running-the-tests)
7. [User Interface Guide](#user-interface-guide)
8. [Firmware Update (OTA)](#firmware-update-ota)
9. [Hardware Specifications](#hardware-specifications)
10. [Configuration & Persistence](#configuration--persistence)
11. [Typical Workflows](#typical-workflows)
12. [Developer Quick Reference](#developer-quick-reference)
13. [Authors](#authors)

---

## What is FilMachine?

FilMachine takes the guesswork out of developing photographic film at home or in a small lab. Instead of manually timing each step, pouring chemicals by hand, and constantly checking the thermometer, FilMachine automates everything. You load your film in the developing tank, select a process, and press Start. The machine takes care of the rest: filling the right chemical at the right time, keeping the temperature stable, rotating the film at the correct speed, rinsing between steps, and alerting you when the process is complete.

It supports both **color negative** (C41, E6) and **black & white** film development workflows, with full customization of every parameter.

---

## Project Architecture

The project has a **dual-target build system** that produces either ESP32-S3 firmware or a native desktop application from the same codebase.

### Technology Stack

| Layer | Firmware (ESP32-S3) | Simulator (macOS/Linux) |
|-------|-------------------|------------------------|
| **UI Library** | LVGL 9.2.2 | LVGL 9.2.2 (identical) |
| **Display** | ILI9488 480x320 TFT | SDL2 window 480x320 |
| **Touch Input** | FT6236 capacitive | SDL2 mouse events |
| **Storage** | FatFS on MicroSD | POSIX file I/O (sd/ directory) |
| **RTOS** | FreeRTOS (ESP-IDF) | Stub (queues work, tasks are no-ops) |
| **Temp Sensors** | DS18B20 OneWire | Simulated (20°C ambient, heater model) |
| **Motor/Relays** | GPIO + MCP23017 I2C | printf stubs |
| **OTA Updates** | esp_ota_ops + esp_http_server | Simulated (progress timer) |
| **Build Tool** | ESP-IDF (`idf.py`) | CMake + Make |
| **Compiler** | xtensa-esp32s3-elf-gcc | Native cc/gcc/clang |

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
│   ├── FilMachine.c               #   ESP32 entry point (app_main, sysMan task, motorMan task)
│   ├── accessories.c              #   Utilities — linked lists, deep copy, config I/O, keyboard
│   ├── ota_update.c               #   OTA firmware update (SD card + Wi-Fi web server)
│   ├── ds18b20.c                  #   DS18B20 OneWire temperature sensor driver
│   ├── mcp23017.c                 #   MCP23017 I2C GPIO expander driver
│   └── pca9685.c                  #   PCA9685 I2C PWM controller driver
│
├── c_pages/                       # UI pages (screens)
│   ├── page_home.c                #   Splash screen & boot error display
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
│   ├── element_selfcheckPopup.c   #   Self-check diagnostic wizard (7-phase hardware test)
│   └── element_otaWifiPopup.c     #   Wi-Fi OTA update popup (IP + PIN + progress)
│
├── c_fonts/                       # Custom icon fonts (5 sizes: 15/20/30/40/100px)
├── c_graphics/                    # Splash screen image data
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
├── CMakeLists.txt                 # Dual-purpose build (ESP-IDF + simulator + tests)
├── sdkconfig                      # ESP-IDF configuration (partition table, Wi-Fi, etc.)
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
mkdir -p build2 && cd build2
cmake .. -DTARGET=sim
make filmachine_sim
```

This produces the `filmachine_sim` executable. On first build, if `sd/FilMachine.cfg` doesn't exist, the Python script `genFilMachineCFG.py` runs automatically to generate sample data.

### Build the Tests

```bash
mkdir -p build2 && cd build2
cmake .. -DTARGET=sim
make filmachine_test
```

### Build the Firmware (ESP32-S3)

Requires the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/) toolchain installed and configured.

**Important:** run `idf.py` from the **project root**, not from `build2/`.

```bash
cd FilMachine_Simulator_v2
idf.py build
```

The firmware binary is produced at `build/FilMachine.bin`. To flash it directly to a connected board:

```bash
idf.py flash
```

Or use the helper script:
```bash
./flash.sh
```

To check the firmware size (useful for verifying OTA partition fit):
```bash
idf.py size
```

---

## Running the Simulator

```bash
cd build2
./filmachine_sim
```

The simulator opens a 480x320 window that reproduces the exact touchscreen interface. Use the mouse to simulate touch input (click = tap, click-drag = swipe/scroll).

### What works in the simulator

- Full UI navigation (all pages, popups, dialogs)
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
| **Destroy & Lifecycle** | Memory cleanup, object destruction |

The persistence tests verify that every field (process name, temperature, tolerance, film type, preferred flag, step names, durations, types, sources, discard flags) survives a full save-and-reload cycle.

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

### The Settings Tab

| Setting | Description | Range |
|---------|-------------|-------|
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
- **Software info** — Firmware version (read from the running binary) and serial number
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

### Board

ESP32-S3-WROOM-1-N16R8 with integrated 3.5" TFT display module.

| Component | Detail |
|-----------|--------|
| **Controller** | ESP32-S3-WROOM-1-N16R8 (PCB antenna) |
| **Flash** | 16MB |
| **PSRAM** | 8MB |
| **Wireless** | Wi-Fi 802.11 b/g/n + Bluetooth 5.0 |
| **Display** | 3.5" ILI9488 TFT LCD, 480×320, 16-bit parallel interface |
| **Touch** | FT6236 capacitive touchscreen (I2C) |
| **Storage** | MicroSD card slot |
| **USB** | Dual USB Type-C (CP2104 UART-to-USB + native USB) |
| **I/O Expander** | MCP23017 (I2C) — 8 relay outputs |
| **PWM Controller** | PCA9685 (I2C) — pump speed control |
| **Temperature** | 2× DS18B20 OneWire sensors (chemical bath + water bath) |
| **Motor** | DC motor with H-bridge (GPIO 8/9), PWM speed (GPIO 18) |
| **Relays** | Heater, 3 chemical valves (C1/C2/C3), water bath, waste, pump in, pump out |
| **Mabee Interface** | 1× I2C, 1× GPIO |
| **Power** | USB Type-C 5.0V (4.0V–5.25V) |
| **Operating Temp** | -40°C to +85°C |
| **Dimensions** | 66mm × 84.3mm × 12mm |

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

All data is stored on the SD card in binary format (`FilMachine.cfg`). The file contains the complete machine state: settings (including calibration offset), all processes with their steps, and machine statistics (completed processes, total processing time, stopped processes, completed cleaning cycles).

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
idf.py build                     # Build firmware → build/FilMachine.bin
idf.py flash                     # Flash to connected board
idf.py flash monitor             # Flash + open serial monitor
idf.py monitor                   # Serial monitor only (Ctrl+] to exit)
idf.py size                      # Show firmware size breakdown
idf.py menuconfig                # Edit sdkconfig (partition table, Wi-Fi, etc.)
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
