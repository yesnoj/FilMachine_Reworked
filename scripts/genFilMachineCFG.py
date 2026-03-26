#!/usr/bin/env python3
"""
genFilMachineCFG.py — Generate FilMachine configuration files

Generates binary .cfg files compatible with readConfigFile/writeConfigFile,
plus a human-readable .json for inspection.

Usage:
    python3 genFilMachineCFG.py                    # Random data
    python3 genFilMachineCFG.py --realistic        # Realistic film dev processes
    python3 genFilMachineCFG.py --processes 10     # Custom number of processes
    python3 genFilMachineCFG.py --output sd/       # Custom output directory

Place the generated files in the simulator's sd/ folder.
"""

import struct
import random
import os
import json
import string
import argparse

# ═══════════════════════════════════════════════
# Constants matching FilMachine.h
# ═══════════════════════════════════════════════
MAX_PROC_NAME_LEN = 20      # Must match #define in FilMachine.h
MAX_STEP_ELEMENTS = 30       # Max steps per process
MAX_PROC_ELEMENTS = 50       # Max processes

# Step types (chemicalType_t enum)
CHEMICAL_TYPE_DEVELOP = 0
CHEMICAL_TYPE_RINSE = 1
CHEMICAL_TYPE_MULTI_RINSE = 2

# Film types (filmType_t enum)
FILM_TYPE_COLOR = 0
FILM_TYPE_BW = 1

# Chemical sources
SOURCE_C1 = 0
SOURCE_C2 = 1
SOURCE_C3 = 2
SOURCE_WB = 3

# ═══════════════════════════════════════════════
# Realistic process templates
# ═══════════════════════════════════════════════
REALISTIC_PROCESSES = [
    {
        "name": "C41 Color Std",
        "temp": 38, "tolerance": 0.3, "tempControlled": 1,
        "preferred": 1, "filmType": FILM_TYPE_COLOR,
        "steps": [
            {"name": "Pre-wash", "mins": 1, "secs": 0, "type": CHEMICAL_TYPE_RINSE, "source": SOURCE_WB, "discard": 1},
            {"name": "Developer", "mins": 3, "secs": 15, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C1, "discard": 0},
            {"name": "Bleach", "mins": 6, "secs": 30, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C2, "discard": 0},
            {"name": "Wash", "mins": 3, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
            {"name": "Fix", "mins": 6, "secs": 30, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C3, "discard": 0},
            {"name": "Final Wash", "mins": 3, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
        ]
    },
    {
        "name": "C41 Push +1",
        "temp": 38, "tolerance": 0.3, "tempControlled": 1,
        "preferred": 0, "filmType": FILM_TYPE_COLOR,
        "steps": [
            {"name": "Pre-wash", "mins": 1, "secs": 0, "type": CHEMICAL_TYPE_RINSE, "source": SOURCE_WB, "discard": 1},
            {"name": "Developer", "mins": 3, "secs": 45, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C1, "discard": 0},
            {"name": "Bleach", "mins": 6, "secs": 30, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C2, "discard": 0},
            {"name": "Wash", "mins": 3, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
            {"name": "Fix", "mins": 6, "secs": 30, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C3, "discard": 0},
            {"name": "Final Wash", "mins": 3, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
        ]
    },
    {
        "name": "B&W D76 1+1",
        "temp": 20, "tolerance": 0.5, "tempControlled": 1,
        "preferred": 1, "filmType": FILM_TYPE_BW,
        "steps": [
            {"name": "Pre-wash", "mins": 1, "secs": 0, "type": CHEMICAL_TYPE_RINSE, "source": SOURCE_WB, "discard": 1},
            {"name": "Developer", "mins": 9, "secs": 45, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C1, "discard": 1},
            {"name": "Stop Bath", "mins": 1, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C2, "discard": 0},
            {"name": "Fixer", "mins": 5, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C3, "discard": 0},
            {"name": "Wash", "mins": 5, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
        ]
    },
    {
        "name": "B&W Rodinal 1+50",
        "temp": 20, "tolerance": 0.5, "tempControlled": 0,
        "preferred": 0, "filmType": FILM_TYPE_BW,
        "steps": [
            {"name": "Developer", "mins": 12, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C1, "discard": 1},
            {"name": "Stop Bath", "mins": 1, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C2, "discard": 0},
            {"name": "Fixer", "mins": 5, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C3, "discard": 0},
            {"name": "Wash", "mins": 10, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
        ]
    },
    {
        "name": "E6 Slide",
        "temp": 38, "tolerance": 0.3, "tempControlled": 1,
        "preferred": 0, "filmType": FILM_TYPE_COLOR,
        "steps": [
            {"name": "First Dev", "mins": 6, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C1, "discard": 0},
            {"name": "Wash", "mins": 2, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
            {"name": "Color Dev", "mins": 6, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C2, "discard": 0},
            {"name": "Wash", "mins": 2, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
            {"name": "Bleach Fix", "mins": 10, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C3, "discard": 0},
            {"name": "Final Wash", "mins": 4, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
        ]
    },
    {
        "name": "B&W HC110 DilB",
        "temp": 20, "tolerance": 0.5, "tempControlled": 1,
        "preferred": 1, "filmType": FILM_TYPE_BW,
        "steps": [
            {"name": "Pre-wash", "mins": 1, "secs": 0, "type": CHEMICAL_TYPE_RINSE, "source": SOURCE_WB, "discard": 1},
            {"name": "Developer", "mins": 6, "secs": 30, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C1, "discard": 1},
            {"name": "Stop", "mins": 0, "secs": 30, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C2, "discard": 0},
            {"name": "Fixer", "mins": 4, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C3, "discard": 0},
            {"name": "Wash", "mins": 5, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
        ]
    },
    {
        "name": "B&W XTOL Stock",
        "temp": 20, "tolerance": 0.5, "tempControlled": 1,
        "preferred": 0, "filmType": FILM_TYPE_BW,
        "steps": [
            {"name": "Pre-wash", "mins": 1, "secs": 0, "type": CHEMICAL_TYPE_RINSE, "source": SOURCE_WB, "discard": 1},
            {"name": "Developer", "mins": 7, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C1, "discard": 0},
            {"name": "Stop Bath", "mins": 1, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C2, "discard": 0},
            {"name": "Fixer", "mins": 5, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C3, "discard": 0},
            {"name": "Wash", "mins": 8, "secs": 0, "type": CHEMICAL_TYPE_MULTI_RINSE, "source": SOURCE_WB, "discard": 1},
        ]
    },
    {
        "name": "Quick B&W Test",
        "temp": 24, "tolerance": 1.0, "tempControlled": 0,
        "preferred": 0, "filmType": FILM_TYPE_BW,
        "steps": [
            {"name": "Developer", "mins": 5, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C1, "discard": 1},
            {"name": "Fixer", "mins": 3, "secs": 0, "type": CHEMICAL_TYPE_DEVELOP, "source": SOURCE_C2, "discard": 0},
            {"name": "Rinse", "mins": 3, "secs": 0, "type": CHEMICAL_TYPE_RINSE, "source": SOURCE_WB, "discard": 1},
        ]
    },
]

# ═══════════════════════════════════════════════
# Settings
# ═══════════════════════════════════════════════
DEFAULT_SETTINGS = {
    "tempUnit": 0,  # Celsius
    "waterInlet": 1,
    "calibratedTemp": 20,
    "filmRotationSpeedSetpoint": 50,
    "rotationIntervalSetpoint": 10,
    "randomSetpoint": 20,
    "isPersistentAlarm": 1,
    "isProcessAutostart": 0,
    "drainFillOverlapSetpoint": 100,
    "multiRinseTime": 60,
    "tankSize": 2,
    "pumpSpeed": 30,
    "chemContainerMl": 500,
    "wbContainerMl": 2000,
    "chemistryVolume": 2,
    "splashRandom": 0,
    "splashPalette": 3,       # Deep Ocean
    "splashShapeStyle": 0,
    "splashComplexity": 40,
    "splashSeed": 0,
    "splashDefault": 1        # Use default splash
}

def random_settings():
    return {
        "tempUnit": random.randint(0, 1),
        "waterInlet": random.randint(0, 1),
        "calibratedTemp": random.randint(0, 40),
        "filmRotationSpeedSetpoint": random.randrange(10, 100, 10),
        "rotationIntervalSetpoint": random.randrange(10, 60, 10),
        "randomSetpoint": random.randrange(0, 101, 20),
        "isPersistentAlarm": random.randint(0, 1),
        "isProcessAutostart": random.randint(0, 1),
        "drainFillOverlapSetpoint": random.randrange(0, 100, 50),
        "multiRinseTime": random.randrange(60, 181, 30),
        "tankSize": random.randint(1, 3),
        "pumpSpeed": random.randrange(10, 101, 10),
        "chemContainerMl": random.choice([250, 500, 750, 1000, 1250, 1500]),
        "wbContainerMl": random.choice([1000, 1500, 2000, 2500, 3000]),
        "chemistryVolume": random.randint(1, 2),
        "splashRandom": random.randint(0, 1),
        "splashPalette": random.randint(0, 9),
        "splashShapeStyle": random.randint(0, 5),
        "splashComplexity": random.randrange(20, 101, 20),
        "splashSeed": random.randint(1, 0xFFFFFFFF),
        "splashDefault": random.randint(0, 1)
    }

# ═══════════════════════════════════════════════
# Random process generator
# ═══════════════════════════════════════════════
def random_string(length=MAX_PROC_NAME_LEN):
    return ''.join(random.choice(string.ascii_letters) for _ in range(length))

def generate_random_steps(count=None):
    if count is None:
        count = random.randint(2, 6)
    steps = []
    total_mins = 0
    total_secs = 0
    for _ in range(count):
        step_mins = random.randint(0, 15)
        step_secs = random.randint(0, 59)
        steps.append({
            "stepNameString": random_string(random.randint(5, MAX_PROC_NAME_LEN)),
            "timeMins": step_mins,
            "timeSecs": step_secs,
            "type": random.randint(0, 2),
            "source": random.randint(0, 3),
            "discardAfterProc": random.randint(0, 1)
        })
        total_mins += step_mins
        total_secs += step_secs
    total_mins += total_secs // 60
    total_secs = total_secs % 60
    return steps, total_mins, total_secs

def generate_random_process():
    steps, total_mins, total_secs = generate_random_steps()
    return {
        "processNameString": random_string(random.randint(5, MAX_PROC_NAME_LEN)),
        "temp": random.randint(20, 40),
        "tempTolerance": float(random.randint(0, 10)) / 10,
        "isTempControlled": random.randint(0, 1),
        "isPreferred": random.randint(0, 1),
        "filmType": random.randint(0, 1),
        "timeMins": total_mins,
        "timeSecs": total_secs,
        "steps": steps
    }

def convert_realistic_process(p):
    """Convert a realistic template to the internal format"""
    steps = []
    total_mins = 0
    total_secs = 0
    for s in p["steps"]:
        steps.append({
            "stepNameString": s["name"][:MAX_PROC_NAME_LEN],
            "timeMins": s["mins"],
            "timeSecs": s["secs"],
            "type": s["type"],
            "source": s["source"],
            "discardAfterProc": s["discard"]
        })
        total_mins += s["mins"]
        total_secs += s["secs"]
    total_mins += total_secs // 60
    total_secs = total_secs % 60
    return {
        "processNameString": p["name"][:MAX_PROC_NAME_LEN],
        "temp": p["temp"],
        "tempTolerance": p["tolerance"],
        "isTempControlled": p["tempControlled"],
        "isPreferred": p["preferred"],
        "filmType": p["filmType"],
        "timeMins": total_mins,
        "timeSecs": total_secs,
        "steps": steps
    }

# ═══════════════════════════════════════════════
# Binary writers (match readConfigFile/writeConfigFile format)
# ═══════════════════════════════════════════════
def write_settings(f, s):
    f.write(struct.pack('<L', s["tempUnit"]))        # tempUnit_t (enum = 4 bytes)
    f.write(struct.pack('<B', s["waterInlet"]))       # bool
    f.write(struct.pack('<B', s["calibratedTemp"]))   # uint8_t
    f.write(struct.pack('<B', s["filmRotationSpeedSetpoint"]))
    f.write(struct.pack('<B', s["rotationIntervalSetpoint"]))
    f.write(struct.pack('<B', s["randomSetpoint"]))
    f.write(struct.pack('<B', s["isPersistentAlarm"]))
    f.write(struct.pack('<B', s["isProcessAutostart"]))
    f.write(struct.pack('<B', s["drainFillOverlapSetpoint"]))
    f.write(struct.pack('<B', s["multiRinseTime"]))
    f.write(struct.pack('<B', s["tankSize"]))          # uint8_t (1=Small, 2=Medium, 3=Large)
    f.write(struct.pack('<B', s["pumpSpeed"]))
    f.write(struct.pack('<H', s["chemContainerMl"]))   # uint16_t
    f.write(struct.pack('<H', s["wbContainerMl"]))     # uint16_t
    f.write(struct.pack('<B', s["chemistryVolume"]))    # uint8_t (1=Low, 2=High)
    f.write(struct.pack('<b', s.get("tempCalibOffset", 0)))  # int8_t (tenths of degree)
    # ── Splash screen settings ──
    f.write(struct.pack('<B', s.get("splashRandom", 0)))      # bool
    f.write(struct.pack('<B', s.get("splashPalette", 3)))     # uint8_t (0–9, default 3 = Deep Ocean)
    f.write(struct.pack('<B', s.get("splashShapeStyle", 0)))  # uint8_t (0–5)
    f.write(struct.pack('<B', s.get("splashComplexity", 40))) # uint8_t (20–100)
    f.write(struct.pack('<L', s.get("splashSeed", 0)))        # uint32_t
    f.write(struct.pack('<B', s.get("splashDefault", 1)))     # bool (default = true)

def write_process(f, p):
    f.write(p["processNameString"].encode('ASCII').ljust(MAX_PROC_NAME_LEN + 1, b'\x00'))
    f.write(struct.pack('<L', p["temp"]))             # uint32_t
    f.write(struct.pack('<f', p["tempTolerance"]))    # float
    f.write(struct.pack('<B', p["isTempControlled"])) # bool
    f.write(struct.pack('<B', p["isPreferred"]))      # bool
    f.write(struct.pack('<L', p["filmType"]))         # filmType_t (enum = 4 bytes)
    f.write(struct.pack('<L', p["timeMins"]))         # uint32_t
    f.write(struct.pack('<B', p["timeSecs"]))         # uint8_t
    f.write(struct.pack('<H', len(p["steps"])))       # uint16_t (stepList.size)
    for step in p["steps"]:
        write_step(f, step)

def write_step(f, s):
    f.write(s["stepNameString"].encode('ASCII').ljust(MAX_PROC_NAME_LEN + 1, b'\x00'))
    f.write(struct.pack('<B', s["timeMins"]))         # uint8_t
    f.write(struct.pack('<B', s["timeSecs"]))         # uint8_t
    f.write(struct.pack('<L', s["type"]))             # chemicalType_t (enum = 4 bytes)
    f.write(struct.pack('<B', s["source"]))           # uint8_t
    f.write(struct.pack('<B', s["discardAfterProc"])) # uint8_t

def write_stats(f, stats=None):
    """Write machine statistics at the end of the config file"""
    if stats is None:
        stats = {"completed": 0, "totalMins": 0, "stopped": 0, "clean": 0}
    f.write(struct.pack('<I', stats["completed"]))   # uint32_t
    f.write(struct.pack('<Q', stats["totalMins"]))    # uint64_t
    f.write(struct.pack('<I', stats["stopped"]))      # uint32_t
    f.write(struct.pack('<I', stats["clean"]))         # uint32_t

def write_config(filename, settings, processes):
    with open(filename, "wb") as f:
        write_settings(f, settings)
        f.write(struct.pack('<l', len(processes)))    # int32_t (processList.size)
        for p in processes:
            write_process(f, p)
        write_stats(f)  # Machine statistics (zeroed)
    print(f"  Written: {filename} ({os.path.getsize(filename)} bytes, {len(processes)} processes)")

# ═══════════════════════════════════════════════
# Main
# ═══════════════════════════════════════════════
def main():
    parser = argparse.ArgumentParser(description='Generate FilMachine configuration files')
    parser.add_argument('--realistic', action='store_true', help='Use realistic film process names')
    parser.add_argument('--processes', type=int, default=None, help='Number of processes to generate')
    parser.add_argument('--output', type=str, default='.', help='Output directory')
    parser.add_argument('--random-settings', action='store_true', help='Randomize machine settings')
    args = parser.parse_args()

    os.makedirs(args.output, exist_ok=True)

    # Settings
    settings = random_settings() if args.random_settings else DEFAULT_SETTINGS

    # Processes
    if args.realistic:
        processes = [convert_realistic_process(p) for p in REALISTIC_PROCESSES]
        if args.processes and args.processes > len(processes):
            # Add random processes to fill up
            for _ in range(args.processes - len(processes)):
                processes.append(generate_random_process())
    else:
        count = args.processes or 4
        processes = [generate_random_process() for _ in range(count)]

    # Limit to max
    processes = processes[:MAX_PROC_ELEMENTS]

    print(f"\nGenerating FilMachine configuration:")
    print(f"  Settings: {'random' if args.random_settings else 'default'}")
    print(f"  Processes: {len(processes)} ({'realistic' if args.realistic else 'random'})")
    print()

    # Write files
    cfg_path = os.path.join(args.output, 'FilMachine.cfg')
    backup_path = os.path.join(args.output, 'FilMachine_Backup.cfg')
    json_path = os.path.join(args.output, 'FilMachine.json')

    write_config(cfg_path, settings, processes)
    write_config(backup_path, settings, processes)

    # JSON for human inspection
    data = {"settingsParams": settings, "processes": processes}
    with open(json_path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"  Written: {json_path}")

    print(f"\nDone! Files are in: {os.path.abspath(args.output)}/")

if __name__ == "__main__":
    main()
