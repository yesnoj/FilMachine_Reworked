# FilMachine — Automated Film Development Machine

FilMachine is an automated film processing machine designed for photographic film development. It handles the entire process: chemical baths, water rinses, temperature regulation, and motor-driven film agitation — all controlled through a 3.5" color touchscreen display.

The project includes a **desktop simulator** (SDL2 + LVGL) that reproduces the full touchscreen UI on macOS/Linux, enabling rapid development and testing without physical hardware, plus an **automated test suite** with 100+ tests.

---

## Table of Contents

1. [What is FilMachine?](#what-is-filmachine)
2. [Project Architecture](#project-architecture)
3. [Directory Structure](#directory-structure)
4. [Building the Project](#building-the-project)
5. [Running the Simulator](#running-the-simulator)
6. [Running the Tests](#running-the-tests)
7. [User Interface Guide](#user-interface-guide)
8. [Hardware Specifications](#hardware-specifications)
9. [Configuration & Persistence](#configuration--persistence)
10. [Typical Workflows](#typical-workflows)
11. [Authors](#authors)

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
| **Touch Input** | FT5x06 capacitive | SDL2 mouse events |
| **Storage** | FatFS on MicroSD | POSIX file I/O (sd/ directory) |
| **RTOS** | FreeRTOS (ESP-IDF) | Stub (queues work, tasks are no-ops) |
| **Temp Sensors** | DS18B20 OneWire | Simulated (20°C ambient, heater model) |
| **Motor/Relays** | GPIO + MCP23017 I2C | printf stubs |
| **Build Tool** | ESP-IDF (`idf.py`) | CMake + Make |
| **Compiler** | xtensa-esp32s3-elf-gcc | Native cc/gcc/clang |

### How the Simulator Works

The simulator replaces all hardware-specific code with a **stub layer** that provides compatible implementations:

- **FatFS stubs** map firmware paths (e.g., `/FilMachine.cfg`) to files inside the `sd/` subdirectory relative to the executable, so read/write operations work transparently.
- **FreeRTOS stubs** provide working queue implementations (xQueueCreate, xQueueSend, xQueueReceive) using simple circular buffers. Task creation is a no-op since there are no real threads — instead, the main loop drains the system queue every frame.
- **Hardware stubs** (GPIO, I2C, SPI, relay, motor, temperature) are either no-ops or printf-based logging functions. Temperature readings come from a simple thermal model that simulates heating/cooling.
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
│   ├── page_settings.c            #   Machine settings (temp, speed, alarms, timers)
│   ├── page_tools.c               #   Maintenance tools, import/export, statistics
│   └── page_checkup.c             #   Process execution — the most complex page
│
├── c_elements/                    # Reusable UI components
│   ├── element_process.c          #   Process list item (drag, delete, duplicate)
│   ├── element_step.c             #   Step list item (swipe gestures, edit/delete)
│   ├── element_filterPopup.c      #   Filter dialog (name, type, preferred)
│   ├── element_messagePopup.c     #   Generic confirmation/alert popups
│   ├── element_rollerPopup.c      #   Numeric selector (roller) widget
│   └── element_cleanPopup.c       #   Cleaning process UI with timer
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

```bash
idf.py build
idf.py flash
```

Or use the helper script:
```bash
./flash.sh
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
- Simulated temperature readings with heater model
- Console logging of all system actions (`[SIM] sysAction: ...`)

### What is simulated (no real hardware)

- Temperature sensors return simulated values (20°C ambient, gradual heating/cooling)
- Motor control outputs to console only
- Relay switching outputs to console only
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
| **Persistence** | Config save → reload → verify all fields |
| **Settings** | Settings UI, default values, slider/switch behavior |
| **Filter** | Filter by name, film type, preferred flag |
| **Tools** | Cleaning, draining, statistics display |
| **Edge Cases** | Boundary conditions, max limits, error recovery |
| **Utilities** | Helper functions, linked list operations |
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
| Temp sensor calibration | Calibrate against a reference thermometer | Tune button |
| Rotation speed | Film agitation motor RPM | 10–100% |
| Inversion interval | Seconds between motor direction changes | 10–60s |
| Randomness | Random variation on inversion interval | 0–100% |
| Persistent alarm | Alarm sounds until acknowledged | On/Off |
| Process autostart | Auto-start when temperature reached | On/Off |
| Drain/fill overlap | Overlap between drain and fill operations | 0–100% |
| Multi-rinse cycle time | Duration of each rinse in multi-rinse steps | 60–180s |

All settings are saved automatically to the SD card when changed. Slider values are saved only when you release the slider (not during dragging) to reduce SD card wear.

### The Tools Tab

- **Clean machine** — Automated cleaning cycle for chemical containers
- **Drain machine** — Manual drain of all liquids
- **Import/Export** — Backup and restore configuration to SD card
- **Statistics** — Completed processes, total time, cleaning cycles, stopped processes
- **Software info** — Firmware version and serial number

---

## Hardware Specifications

| Component | Detail |
|-----------|--------|
| **Controller** | ESP32-S3 |
| **Display** | 3.5" ILI9488 TFT LCD, 480×320, 16-bit parallel interface |
| **Touch** | FT5x06 capacitive touchscreen (I2C) |
| **Storage** | MicroSD card via SPI3 |
| **I/O Expander** | MCP23017 (I2C) — 8 relay outputs |
| **PWM Controller** | PCA9685 (I2C) — pump speed control |
| **Temperature** | 2× DS18B20 OneWire sensors (chemical bath + water bath) |
| **Motor** | DC motor with H-bridge (GPIO 8/9), PWM speed (GPIO 18) |
| **Relays** | Heater, 3 chemical valves (C1/C2/C3), water bath, waste, pump in, pump out |

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

All data is stored on the SD card in binary format (`FilMachine.cfg`). The file contains the complete machine state: settings, statistics, all processes with their steps.

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

## License

Private project. All rights reserved.

## Authors

**PeterB** — Hardware design & firmware
**FrankP** — Software development & testing
