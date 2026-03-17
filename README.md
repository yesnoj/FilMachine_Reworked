# FilMachine — Automated Film Development Machine

FilMachine is an automated film processing machine designed for photographic film development. It handles the entire process: chemical baths, water rinses, temperature regulation, and motor-driven film agitation — all controlled through a 3.5" color touchscreen display.

## What is FilMachine?

FilMachine takes the guesswork out of developing photographic film at home or in a small lab. Instead of manually timing each step, pouring chemicals by hand, and constantly checking the thermometer, FilMachine automates everything. You load your film in the developing tank, select a process, and press Start. The machine takes care of the rest: filling the right chemical at the right time, keeping the temperature stable, rotating the film at the correct speed, rinsing between steps, and alerting you when the process is complete.

It supports both **color negative** (C41, E6) and **black & white** film development workflows, with full customization of every parameter.

## Getting Started

When you turn on the machine, you will see a splash screen followed by the main interface. The interface is divided into three tabs accessible from the bottom navigation bar: **Processes**, **Settings**, and **Tools**.

### The Processes Tab

This is where you manage your film development recipes. Each process is a sequence of steps that the machine will execute in order.

When you open this tab you will see a list of all your saved processes. Each entry shows the process name, the total development time, and a star icon if the process is marked as preferred. Color processes are displayed with a color palette icon, while black & white processes show a B&W icon.

**Creating a new process:** Tap the "+" button in the top right corner. You will be taken to the process detail screen, where you can set the process name, choose the film type (Color or B&W), set the target temperature and tolerance, and mark it as preferred. You must add at least one step before saving.

**Editing a process:** Tap on any process in the list to open its detail view. From there you can modify any parameter, add or remove steps, and save your changes.

**Duplicating a process:** Swipe left on a process to reveal the duplicate option. This creates an exact copy that you can then customize — useful when you want to create a variation of an existing recipe (for example, a push-processed version).

**Deleting a process:** Open the process detail, then tap the trash icon. A confirmation popup will ask you to confirm the deletion.

**Filtering the list:** If you have many processes saved, tap the "Filters" button to narrow the list. You can filter by name (partial match), film type (Color, B&W, or Both), and whether the process is marked as preferred.

### Working with Steps

Each process contains a list of steps. A step represents one phase of the development workflow, such as "Developer", "Stop Bath", "Fixer", or "Wash".

For each step, you can configure:

- **Name** — A descriptive label (e.g., "Pre-wash", "Developer", "Bleach", "Final Wash")
- **Duration** — Time in minutes and seconds for this step
- **Chemical type** — Chemistry (a chemical bath), Rinse (a single water rinse), or Multi-Rinse (multiple consecutive water rinses)
- **Source** — Which container supplies the liquid: C1, C2, C3 (three chemical containers), or WB (water bath)
- **Discard after use** — Whether the chemical should be sent to the waste container after this step rather than returned to its source

**Adding a step:** In the process detail view, tap the "+" button below the step list.

**Reordering steps:** Long-press on a step and drag it up or down to change its position in the sequence.

**Duplicating a step:** Swipe left on a step to duplicate it. The copy appears immediately after the original.

**Editing a step:** Tap on a step to open its detail editor, where you can modify all parameters.

### Running a Process (Checkup)

To start developing film, open a process and tap the Play button. This takes you to the Checkup screen, where the machine walks you through the pre-flight checks and then executes each step automatically.

**Before the process starts, the machine will:**

1. Ask you to select the tank size and chemistry volume
2. Fill the water bath (if connected to a water source; otherwise prompt you to fill it manually)
3. Verify that the developing tank is in position
4. Check that the film rotation motor is working
5. If the process is temperature-controlled, heat the water bath to the target temperature (within the configured tolerance)

**During the process, the screen shows:**

- The name of the current step and remaining time
- The chemical source being used
- Current water temperature and target temperature
- The next step in the queue
- A list of upcoming steps

**Stopping a process:** You have two options. "Stop now!" immediately halts the process (the film may be ruined if chemicals are in the tank). "Stop after!" waits until the current step finishes before stopping — a safer choice that allows the current chemical to be properly drained.

### The Settings Tab

Here you configure the machine's operating parameters.

**Temperature unit** — Switch between Celsius (°C) and Fahrenheit (°F). All temperatures throughout the interface will update accordingly.

**Temperature sensor calibration** — To ensure accurate readings, you can calibrate the temperature sensor. Make sure the machine has reached a stable ambient temperature, measure the actual air temperature with a reliable thermometer, then enter that value and press "Tune". To reset the calibration, long-press the "Tune" button.

**Rotation speed** — Controls the film agitation motor speed in RPM (range: 20–90 RPM). Proper agitation is critical for even development.

**Rotation inversion interval** — The time in seconds (5–60s) between motor direction changes. The motor alternates between clockwise and counterclockwise rotation to prevent uneven development patterns.

**Randomness** — Adds random variation to the inversion interval (0–100%). For example, 10% randomness on a 10-second interval means the actual interval will vary between 9 and 11 seconds. This helps prevent standing wave patterns on the film.

**Water inlet link** — Enable this if the machine is connected to a water source for automatic water bath filling.

**Persistent alarms** — When enabled, the alarm sound continues until acknowledged. When disabled, it sounds once and stops.

**Process autostart** — When enabled, a temperature-controlled process starts automatically once the water bath reaches the target temperature. When disabled, you must press Start manually.

**Drain/fill time overlap** — Controls how much the draining of one chemical overlaps with the filling of the next (0–100%). Higher values reduce total process time but require the plumbing to support simultaneous drain and fill operations.

**Multi-rinse cycle time** — The duration of each rinse cycle in a multi-rinse step (60–180 seconds, in 30-second increments).

### The Tools Tab

This section provides maintenance and diagnostic features.

**Clean machine** — Runs an automated cleaning cycle. You select which chemical containers to flush, the number of cleaning cycles, and whether to drain the water bath when finished. The machine will pump water through the selected containers to clean out residual chemicals.

**Drain machine** — Manually drains all liquids from the machine.

**Import/Export** — Back up your configuration (all processes and settings) to the SD card, or restore a previously saved configuration. Importing will overwrite your current data and reboot the machine.

**Statistics** — View machine usage data: completed processes, total processing time, completed cleaning cycles, and stopped processes.

**Software info** — Shows the firmware version and serial number.

**Credits** — Information about the development team.

## Chemical Container Layout

The machine has four liquid containers and a waste output:

| Label | Purpose | Example use |
|-------|---------|-------------|
| **C1** | Chemical container 1 | Developer |
| **C2** | Chemical container 2 | Bleach / Stop bath |
| **C3** | Chemical container 3 | Fixer |
| **WB** | Water bath | Rinse water (temperature-controlled) |
| **WASTE** | Waste drain | Discarded chemicals |

You can assign any chemical to any container — the labels above are just typical usage examples. The important thing is that your process steps reference the correct source container for each chemical.

## Typical Workflows

### C41 Color Negative Development

A standard C41 process at 38°C typically looks like this: Pre-wash with water, Developer from C1 (3:15), Bleach from C2 (6:30), Multi-rinse wash, Fixer from C3 (6:30), and a final multi-rinse wash. The machine maintains precise temperature control throughout, which is critical for color development.

### Black & White Development

B&W processes are more flexible in temperature (usually 20°C) and timing. A typical recipe: Developer from C1, Stop bath from C2, Fixer from C3, and a final wash. Since B&W development is less temperature-sensitive, you can optionally disable temperature control.

## Data Storage

All your processes, settings, and statistics are saved to the SD card in a binary configuration file. The machine automatically saves when you create, modify, or delete a process, and when you change settings. A backup copy is maintained alongside the main file for safety.

## Hardware Specifications

| Component | Detail |
|-----------|--------|
| Controller | ESP32-S3 microcontroller |
| Display | 3.5" ILI9488 TFT LCD, 480x320 resolution |
| Touch | FT5x06 capacitive touchscreen |
| Storage | MicroSD card |
| Motor | DC motor with PWM speed control for film agitation |
| Relays | 8-channel relay board (heater, 3 chemicals, water bath, waste, pump in, pump out) |
| Sensors | SHT31 temperature sensors for chemical bath and water bath |

## Building the Simulator

A desktop simulator is available for testing and development. It reproduces the full touchscreen interface on macOS or Linux using SDL2.

**Requirements:** CMake 3.14+, SDL2, pkg-config, and a C compiler (GCC or Clang).

```bash
# macOS
brew install cmake sdl2 pkg-config

# Ubuntu / Debian
sudo apt install cmake libsdl2-dev pkg-config build-essential
```

**Build and run:**

```bash
mkdir -p build && cd build
cmake ..
make filmachine_sim
./filmachine_sim
```

To generate sample process data for the simulator:

```bash
python3 scripts/genFilMachineCFG.py --realistic --output build/sd/
```

## License

Private project. All rights reserved.

## Authors

**PeterB** — Hardware design & firmware
**FrankP** — Software development & testing
