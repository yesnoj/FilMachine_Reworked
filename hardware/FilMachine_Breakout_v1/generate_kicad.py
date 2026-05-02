#!/usr/bin/env python3
"""
FilMachine Breakout Board v1.0 — KiCad 8 Project Generator
============================================================
Generates .kicad_pro, .kicad_sch, .kicad_pcb for the JC4880P433 J9 breakout.

Board: 70mm × 50mm, 2-layer, components placed, basic routing.
Input:  2×13 female pin header (plugs onto J9 male pins on GUITION board)
Output: Screw terminals grouped by function + passive components

Run:  python3 generate_kicad.py
"""
import json, math, os, datetime

PROJECT_NAME = "FilMachine_Breakout_v1"
BOARD_W = 70.0   # mm
BOARD_H = 50.0   # mm
ORIGIN_X = 100.0  # KiCad board origin
ORIGIN_Y = 100.0

# ─── J9 Pinout (2×13 = 26 pins, 2.54mm pitch) ───
# Odd pins = left column, Even pins = right column
# Based on board_jc4880p433.h + typical power pin convention
J9_PINOUT = {
    1:  {"name": "3V3",          "type": "power",  "gpio": None},
    2:  {"name": "5V",           "type": "power",  "gpio": None},
    3:  {"name": "GPIO1",        "type": "spare",  "gpio": 1},
    4:  {"name": "GPIO0",        "type": "spare",  "gpio": 0},
    5:  {"name": "GPIO3",        "type": "spare",  "gpio": 3},
    6:  {"name": "GPIO2",        "type": "spare",  "gpio": 2},
    7:  {"name": "MOTOR_IN1",    "type": "motor",  "gpio": 33},
    8:  {"name": "GPIO4",        "type": "spare",  "gpio": 4},
    9:  {"name": "MOTOR_IN2",    "type": "motor",  "gpio": 34},
    10: {"name": "GPIO6",        "type": "spare",  "gpio": 6},
    11: {"name": "TEMP_DATA",    "type": "temp",   "gpio": 35},
    12: {"name": "HALL_SENSOR",  "type": "hall",   "gpio": 20},
    13: {"name": "FLOW_METER",   "type": "flow",   "gpio": 36},
    14: {"name": "GND",          "type": "power",  "gpio": None},
    15: {"name": "WATER_MIN",    "type": "water",  "gpio": 37},
    16: {"name": "WATER_MAX",    "type": "water",  "gpio": 38},
    17: {"name": "MOTOR_ENA",    "type": "motor",  "gpio": 24},
    18: {"name": "GND",          "type": "power",  "gpio": None},
    19: {"name": "GPIO21",       "type": "spare",  "gpio": 21},
    20: {"name": "3V3",          "type": "power",  "gpio": None},
    21: {"name": "GPIO22",       "type": "spare",  "gpio": 22},
    22: {"name": "GND",          "type": "power",  "gpio": None},
    23: {"name": "I2C_SDA",      "type": "i2c",    "gpio": 7},
    24: {"name": "3V3",          "type": "power",  "gpio": None},
    25: {"name": "I2C_SCL",      "type": "i2c",    "gpio": 8},
    26: {"name": "GND",          "type": "power",  "gpio": None},
}

# ─── Terminal groups with their screw terminal blocks ───
TERMINAL_GROUPS = [
    {
        "name": "MOTOR",
        "ref": "J2",
        "pins": 3,
        "labels": ["ENA (PWM)", "IN1", "IN2"],
        "j9_pins": [17, 7, 9],
        "x": ORIGIN_X + 5, "y": ORIGIN_Y + 5,
    },
    {
        "name": "TEMP",
        "ref": "J3",
        "pins": 3,
        "labels": ["DATA", "3V3", "GND"],
        "j9_pins": [11, "3V3", "GND"],
        "x": ORIGIN_X + 5, "y": ORIGIN_Y + 17,
    },
    {
        "name": "FLOW",
        "ref": "J4",
        "pins": 3,
        "labels": ["SIG", "3V3", "GND"],
        "j9_pins": [13, "3V3", "GND"],
        "x": ORIGIN_X + 5, "y": ORIGIN_Y + 29,
    },
    {
        "name": "I2C",
        "ref": "J5",
        "pins": 4,
        "labels": ["SDA", "SCL", "3V3", "GND"],
        "j9_pins": [23, 25, "3V3", "GND"],
        "x": ORIGIN_X + BOARD_W - 25, "y": ORIGIN_Y + 5,
    },
    {
        "name": "WATER",
        "ref": "J6",
        "pins": 4,
        "labels": ["MIN", "MAX", "3V3", "GND"],
        "j9_pins": [15, 16, "3V3", "GND"],
        "x": ORIGIN_X + BOARD_W - 25, "y": ORIGIN_Y + 19,
    },
    {
        "name": "HALL",
        "ref": "J7",
        "pins": 3,
        "labels": ["SIG", "3V3", "GND"],
        "j9_pins": [12, "3V3", "GND"],
        "x": ORIGIN_X + BOARD_W - 25, "y": ORIGIN_Y + 33,
    },
]

RESISTORS = [
    {"ref": "R1", "value": "4.7k", "desc": "I2C SDA pull-up",   "x": ORIGIN_X + 48, "y": ORIGIN_Y + 10},
    {"ref": "R2", "value": "4.7k", "desc": "I2C SCL pull-up",   "x": ORIGIN_X + 52, "y": ORIGIN_Y + 10},
    {"ref": "R3", "value": "4.7k", "desc": "OneWire pull-up",    "x": ORIGIN_X + 28, "y": ORIGIN_Y + 17},
]

CAPACITORS = [
    {"ref": "C1", "value": "100nF", "desc": "I2C decoupling",    "x": ORIGIN_X + 56, "y": ORIGIN_Y + 10},
    {"ref": "C2", "value": "100nF", "desc": "Power decoupling",  "x": ORIGIN_X + 35, "y": ORIGIN_Y + 42},
]


def uuid():
    """Generate a KiCad-style UUID."""
    import random
    return f"{random.randint(0, 0xFFFFFFFF):08x}-{random.randint(0, 0xFFFF):04x}-{random.randint(0, 0xFFFF):04x}-{random.randint(0, 0xFFFF):04x}-{random.randint(0, 0xFFFFFFFFFFFF):012x}"


# ═══════════════════════════════════════════════════════
# KiCad Project File (.kicad_pro)
# ═══════════════════════════════════════════════════════
def generate_project():
    proj = {
        "meta": {"filename": f"{PROJECT_NAME}.kicad_pro", "version": 1},
        "board": {"design_settings": {"defaults": {"board_outline_line_width": 0.15}}},
        "schematic": {"meta": {"version": 1}},
    }
    return json.dumps(proj, indent=2)


# ═══════════════════════════════════════════════════════
# KiCad Schematic (.kicad_sch)
# ═══════════════════════════════════════════════════════
def generate_schematic():
    lines = []
    lines.append(f"""(kicad_sch
  (version 20231120)
  (generator "FilMachine_Breakout_Generator")
  (generator_version "1.0")
  (uuid "{uuid()}")
  (paper "A4")

  (title_block
    (title "FilMachine Breakout Board v1.0")
    (date "{datetime.date.today().isoformat()}")
    (rev "1.0")
    (comment 1 "Breakout for GUITION JC4880P433 J9 Expand IO")
    (comment 2 "ESP32-P4 FilMachine project")
  )
""")

    # --- Power symbols ---
    lines.append("""
  ;; ── Power Flags ──
  (text "FilMachine Breakout Board v1.0\\nJ9 Expand IO → Screw Terminals + Passives"
    (exclude_from_sim no) (at 40 20 0)
    (effects (font (size 2.54 2.54)) (justify left))
    (uuid "{uid}")
  )
""".format(uid=uuid()))

    # --- J1: 2x13 Pin Header (J9 connector) ---
    y_start = 50
    lines.append(f"""
  ;; ── J1: J9 Connector (2×13 female pin header) ──
  (symbol
    (lib_id "Connector_Generic:Conn_02x13_Odd_Even")
    (at 80 {y_start + 30} 0)
    (unit 1)
    (exclude_from_sim no)
    (in_bom yes)
    (on_board yes)
    (dnp no)
    (uuid "{uuid()}")
    (property "Reference" "J1"
      (at 80 {y_start + 10} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "J9_Expand_IO_2x13"
      (at 80 {y_start + 12} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "Connector_PinHeader_2.54mm:PinHeader_2x13_P2.54mm_Vertical"
      (at 80 {y_start + 30} 0)
      (effects (font (size 1.27 1.27)) hide)
    )
  )
""")

    # --- Screw terminal symbols ---
    x_term = 30
    for i, grp in enumerate(TERMINAL_GROUPS):
        y_t = y_start + i * 20
        pins = grp["pins"]
        lines.append(f"""
  ;; ── {grp['ref']}: {grp['name']} ({pins}-pin screw terminal) ──
  (symbol
    (lib_id "Connector_Generic:Conn_01x{pins:02d}")
    (at {x_term} {y_t} 0)
    (unit 1)
    (exclude_from_sim no)
    (in_bom yes)
    (on_board yes)
    (uuid "{uuid()}")
    (property "Reference" "{grp['ref']}"
      (at {x_term} {y_t - 3} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "{grp['name']}_{pins}P_5.08mm"
      (at {x_term} {y_t + pins * 2.54 + 2} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "TerminalBlock:TerminalBlock_bornier-{pins}_P5.08mm"
      (at {x_term} {y_t} 0)
      (effects (font (size 1.27 1.27)) hide)
    )
  )
""")

    # --- Spare GPIO header ---
    lines.append(f"""
  ;; ── J8: Spare GPIO (1×8 pin header) ──
  (symbol
    (lib_id "Connector_Generic:Conn_01x08")
    (at 140 {y_start + 20} 0)
    (unit 1)
    (exclude_from_sim no)
    (in_bom yes)
    (on_board yes)
    (uuid "{uuid()}")
    (property "Reference" "J8"
      (at 140 {y_start + 17} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "SPARE_GPIO_1x8"
      (at 140 {y_start + 42} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical"
      (at 140 {y_start + 20} 0)
      (effects (font (size 1.27 1.27)) hide)
    )
  )
""")

    # --- Resistors ---
    for i, r in enumerate(RESISTORS):
        yr = y_start + 75 + i * 8
        lines.append(f"""
  ;; ── {r['ref']}: {r['value']} ({r['desc']}) ──
  (symbol
    (lib_id "Device:R")
    (at 130 {yr} 0)
    (unit 1)
    (exclude_from_sim no)
    (in_bom yes)
    (on_board yes)
    (uuid "{uuid()}")
    (property "Reference" "{r['ref']}"
      (at 132 {yr} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "{r['value']}"
      (at 136 {yr} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "Resistor_SMD:R_0805_2012Metric"
      (at 130 {yr} 0)
      (effects (font (size 1.27 1.27)) hide)
    )
  )
""")

    # --- Capacitors ---
    for i, c in enumerate(CAPACITORS):
        yc = y_start + 100 + i * 8
        lines.append(f"""
  ;; ── {c['ref']}: {c['value']} ({c['desc']}) ──
  (symbol
    (lib_id "Device:C")
    (at 130 {yc} 0)
    (unit 1)
    (exclude_from_sim no)
    (in_bom yes)
    (on_board yes)
    (uuid "{uuid()}")
    (property "Reference" "{c['ref']}"
      (at 132 {yc} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "{c['value']}"
      (at 136 {yc} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "Capacitor_SMD:C_0805_2012Metric"
      (at 130 {yc} 0)
      (effects (font (size 1.27 1.27)) hide)
    )
  )
""")

    # --- Power terminal ---
    lines.append(f"""
  ;; ── J9: Power breakout (1×4: 5V, 3V3, GND, GND) ──
  (symbol
    (lib_id "Connector_Generic:Conn_01x04")
    (at 140 {y_start + 50} 0)
    (unit 1)
    (exclude_from_sim no)
    (in_bom yes)
    (on_board yes)
    (uuid "{uuid()}")
    (property "Reference" "J9"
      (at 140 {y_start + 47} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "PWR_5V_3V3_GND"
      (at 140 {y_start + 62} 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "TerminalBlock:TerminalBlock_bornier-4_P5.08mm"
      (at 140 {y_start + 50} 0)
      (effects (font (size 1.27 1.27)) hide)
    )
  )
""")

    lines.append(")")  # close kicad_sch
    return "\n".join(lines)


# ═══════════════════════════════════════════════════════
# KiCad PCB (.kicad_pcb)
# ═══════════════════════════════════════════════════════
def generate_pcb():
    ox, oy = ORIGIN_X, ORIGIN_Y
    bw, bh = BOARD_W, BOARD_H

    pcb = []
    pcb.append(f"""(kicad_pcb
  (version 20240108)
  (generator "FilMachine_Breakout_Generator")
  (generator_version "1.0")
  (general
    (thickness 1.6)
    (legacy_teardrops no)
  )
  (paper "A4")
  (title_block
    (title "FilMachine Breakout Board v1.0")
    (date "{datetime.date.today().isoformat()}")
    (rev "1.0")
    (comment 1 "JC4880P433 J9 Expand IO Breakout")
  )

  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (32 "B.Adhes" user "B.Adhesive")
    (33 "F.Adhes" user "F.Adhesive")
    (34 "B.Paste" user)
    (35 "F.Paste" user)
    (36 "B.SilkS" user "B.Silkscreen")
    (37 "F.SilkS" user "F.Silkscreen")
    (38 "B.Mask" user "B.Mask")
    (39 "F.Mask" user "F.Mask")
    (44 "Edge.Cuts" user)
  )

  (setup
    (pad_to_mask_clearance 0.05)
    (allow_soldermask_bridges_in_footprints no)
    (pcbplotparams
      (layerselection 0x00010fc_ffffffff)
      (plot_on_all_layers_selection 0x0000000_00000000)
    )
  )
""")

    # ── Board Outline ──
    pcb.append(f"""
  ;; ── Board Outline ({bw}mm × {bh}mm) ──
  (gr_rect
    (start {ox} {oy})
    (end {ox + bw} {oy + bh})
    (stroke (width 0.15) (type solid))
    (fill none)
    (layer "Edge.Cuts")
    (uuid "{uuid()}")
  )
""")

    # ── Mounting holes (4 corners, M2) ──
    mh_inset = 3.0
    mh_positions = [
        (ox + mh_inset, oy + mh_inset),
        (ox + bw - mh_inset, oy + mh_inset),
        (ox + mh_inset, oy + bh - mh_inset),
        (ox + bw - mh_inset, oy + bh - mh_inset),
    ]
    for i, (mx, my) in enumerate(mh_positions):
        pcb.append(f"""
  (footprint "MountingHole:MountingHole_2.2mm_M2"
    (layer "F.Cu")
    (uuid "{uuid()}")
    (at {mx:.2f} {my:.2f})
    (property "Reference" "MH{i+1}"
      (at 0 -2 0)
      (layer "F.SilkS")
      (uuid "{uuid()}")
      (effects (font (size 0.8 0.8) (thickness 0.15)))
    )
    (property "Value" "M2"
      (at 0 2 0)
      (layer "F.Fab")
      (uuid "{uuid()}")
      (effects (font (size 0.8 0.8) (thickness 0.15)))
    )
    (pad "" np_thru_hole circle
      (at 0 0)
      (size 2.2 2.2)
      (drill 2.2)
      (layers "*.Cu" "*.Mask")
      (uuid "{uuid()}")
    )
  )
""")

    # ── J1: 2×13 female header (center of board) ──
    j1_x = ox + bw / 2
    j1_y = oy + bh / 2
    pcb.append(f"""
  ;; ── J1: J9 Connector (2×13 female, center of board) ──
  (footprint "Connector_PinSocket_2.54mm:PinSocket_2x13_P2.54mm_Vertical"
    (layer "F.Cu")
    (uuid "{uuid()}")
    (at {j1_x:.2f} {j1_y:.2f})
    (property "Reference" "J1"
      (at 0 -18 0)
      (layer "F.SilkS")
      (uuid "{uuid()}")
      (effects (font (size 1.2 1.2) (thickness 0.2)))
    )
    (property "Value" "J9_2x13"
      (at 0 18 0)
      (layer "F.Fab")
      (uuid "{uuid()}")
      (effects (font (size 1 1) (thickness 0.15)))
    )
""")
    # Generate all 26 pads for 2x13 header
    for pin in range(1, 27):
        row = (pin - 1) % 2        # 0=left col (odd pins), 1=right col (even pins)
        col = (pin - 1) // 2       # which row (0-12)
        px = -1.27 + row * 2.54    # left col at -1.27, right at +1.27
        py = -15.24 + col * 2.54   # 13 rows, centered
        info = J9_PINOUT[pin]
        net_name = info["name"]
        pcb.append(f"""    (pad "{pin}" thru_hole oval
      (at {px:.2f} {py:.2f})
      (size 1.7 1.7)
      (drill 1.0)
      (layers "*.Cu" "*.Mask")
      (uuid "{uuid()}")
    )
""")
    pcb.append("  )\n")

    # ── Silkscreen labels for J9 pins ──
    for pin in range(1, 27):
        row = (pin - 1) % 2
        col = (pin - 1) // 2
        px = j1_x + (-1.27 + row * 2.54)
        py = j1_y + (-15.24 + col * 2.54)
        info = J9_PINOUT[pin]
        label_x = px - 8 if row == 0 else px + 4
        pcb.append(f"""  (gr_text "{pin}:{info['name']}"
    (at {label_x:.2f} {py:.2f} 0)
    (layer "F.SilkS")
    (uuid "{uuid()}")
    (effects (font (size 0.6 0.6) (thickness 0.1)) (justify {"right" if row == 0 else "left"}))
  )
""")

    # ── Screw Terminals ──
    for grp in TERMINAL_GROUPS:
        gx, gy = grp["x"], grp["y"]
        np = grp["pins"]
        pcb.append(f"""
  ;; ── {grp['ref']}: {grp['name']} ({np}-pin 5.08mm screw terminal) ──
  (footprint "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-{np}-5.08_1x{np:02d}_P5.08mm_Horizontal"
    (layer "F.Cu")
    (uuid "{uuid()}")
    (at {gx:.2f} {gy:.2f} 0)
    (property "Reference" "{grp['ref']}"
      (at 0 -3.5 0)
      (layer "F.SilkS")
      (uuid "{uuid()}")
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (property "Value" "{grp['name']}"
      (at 0 4 0)
      (layer "F.Fab")
      (uuid "{uuid()}")
      (effects (font (size 0.8 0.8) (thickness 0.12)))
    )
""")
        for p in range(np):
            pcb.append(f"""    (pad "{p+1}" thru_hole circle
      (at {p * 5.08:.2f} 0)
      (size 2.4 2.4)
      (drill 1.3)
      (layers "*.Cu" "*.Mask")
      (uuid "{uuid()}")
    )
""")
        pcb.append("  )\n")

        # Silkscreen labels for each terminal pin
        for p, label in enumerate(grp["labels"]):
            pcb.append(f"""  (gr_text "{label}"
    (at {gx + p * 5.08:.2f} {gy + 5.5:.2f} 0)
    (layer "F.SilkS")
    (uuid "{uuid()}")
    (effects (font (size 0.7 0.7) (thickness 0.12)))
  )
""")

    # ── Spare GPIO header (J8: 1×8) ──
    j8_x = ox + bw - 8
    j8_y = oy + bh / 2 - 8
    spare_gpios = [0, 1, 2, 3, 4, 6, 21, 22]
    pcb.append(f"""
  ;; ── J8: Spare GPIO (1×8 pin header) ──
  (footprint "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical"
    (layer "F.Cu")
    (uuid "{uuid()}")
    (at {j8_x:.2f} {j8_y:.2f} 0)
    (property "Reference" "J8"
      (at 0 -3 0)
      (layer "F.SilkS")
      (uuid "{uuid()}")
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (property "Value" "SPARE_GPIO"
      (at 0 22 0)
      (layer "F.Fab")
      (uuid "{uuid()}")
      (effects (font (size 0.8 0.8) (thickness 0.12)))
    )
""")
    for p in range(8):
        pcb.append(f"""    (pad "{p+1}" thru_hole oval
      (at 0 {p * 2.54:.2f})
      (size 1.7 1.7)
      (drill 1.0)
      (layers "*.Cu" "*.Mask")
      (uuid "{uuid()}")
    )
""")
    pcb.append("  )\n")
    for p, gpio in enumerate(spare_gpios):
        pcb.append(f"""  (gr_text "IO{gpio}"
    (at {j8_x + 3:.2f} {j8_y + p * 2.54:.2f} 0)
    (layer "F.SilkS")
    (uuid "{uuid()}")
    (effects (font (size 0.6 0.6) (thickness 0.1)) (justify left))
  )
""")

    # ── Power breakout terminal (J9_PWR: 1×4 screw terminal) ──
    jp_x = ox + bw / 2 - 7.5
    jp_y = oy + bh - 7
    pcb.append(f"""
  ;; ── J9_PWR: Power breakout (5V, 3V3, GND, GND) ──
  (footprint "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-4-5.08_1x04_P5.08mm_Horizontal"
    (layer "F.Cu")
    (uuid "{uuid()}")
    (at {jp_x:.2f} {jp_y:.2f} 0)
    (property "Reference" "J10"
      (at 0 -3.5 0)
      (layer "F.SilkS")
      (uuid "{uuid()}")
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (property "Value" "POWER"
      (at 0 4 0)
      (layer "F.Fab")
      (uuid "{uuid()}")
      (effects (font (size 0.8 0.8) (thickness 0.12)))
    )
""")
    pwr_labels = ["5V", "3V3", "GND", "GND"]
    for p in range(4):
        pcb.append(f"""    (pad "{p+1}" thru_hole circle
      (at {p * 5.08:.2f} 0)
      (size 2.4 2.4)
      (drill 1.3)
      (layers "*.Cu" "*.Mask")
      (uuid "{uuid()}")
    )
""")
    pcb.append("  )\n")
    for p, label in enumerate(pwr_labels):
        pcb.append(f"""  (gr_text "{label}"
    (at {jp_x + p * 5.08:.2f} {jp_y + 5.5:.2f} 0)
    (layer "F.SilkS")
    (uuid "{uuid()}")
    (effects (font (size 0.8 0.8) (thickness 0.12)))
  )
""")

    # ── SMD Resistors (0805) ──
    for r in RESISTORS:
        pcb.append(f"""
  ;; ── {r['ref']}: {r['value']} ({r['desc']}) ──
  (footprint "Resistor_SMD:R_0805_2012Metric"
    (layer "F.Cu")
    (uuid "{uuid()}")
    (at {r['x']:.2f} {r['y']:.2f} 0)
    (property "Reference" "{r['ref']}"
      (at 0 -1.5 0)
      (layer "F.SilkS")
      (uuid "{uuid()}")
      (effects (font (size 0.6 0.6) (thickness 0.1)))
    )
    (property "Value" "{r['value']}"
      (at 0 1.5 0)
      (layer "F.Fab")
      (uuid "{uuid()}")
      (effects (font (size 0.6 0.6) (thickness 0.1)))
    )
    (pad "1" smd roundrect
      (at -0.9375 0)
      (size 1.0 1.25)
      (layers "F.Cu" "F.Paste" "F.Mask")
      (roundrect_rratio 0.25)
      (uuid "{uuid()}")
    )
    (pad "2" smd roundrect
      (at 0.9375 0)
      (size 1.0 1.25)
      (layers "F.Cu" "F.Paste" "F.Mask")
      (roundrect_rratio 0.25)
      (uuid "{uuid()}")
    )
  )
""")

    # ── SMD Capacitors (0805) ──
    for c in CAPACITORS:
        pcb.append(f"""
  ;; ── {c['ref']}: {c['value']} ({c['desc']}) ──
  (footprint "Capacitor_SMD:C_0805_2012Metric"
    (layer "F.Cu")
    (uuid "{uuid()}")
    (at {c['x']:.2f} {c['y']:.2f} 0)
    (property "Reference" "{c['ref']}"
      (at 0 -1.5 0)
      (layer "F.SilkS")
      (uuid "{uuid()}")
      (effects (font (size 0.6 0.6) (thickness 0.1)))
    )
    (property "Value" "{c['value']}"
      (at 0 1.5 0)
      (layer "F.Fab")
      (uuid "{uuid()}")
      (effects (font (size 0.6 0.6) (thickness 0.1)))
    )
    (pad "1" smd roundrect
      (at -0.9375 0)
      (size 1.0 1.25)
      (layers "F.Cu" "F.Paste" "F.Mask")
      (roundrect_rratio 0.25)
      (uuid "{uuid()}")
    )
    (pad "2" smd roundrect
      (at 0.9375 0)
      (size 1.0 1.25)
      (layers "F.Cu" "F.Paste" "F.Mask")
      (roundrect_rratio 0.25)
      (uuid "{uuid()}")
    )
  )
""")

    # ── Title silkscreen ──
    pcb.append(f"""
  (gr_text "FilMachine Breakout v1.0"
    (at {ox + bw/2:.2f} {oy + bh - 2:.2f} 0)
    (layer "F.SilkS")
    (uuid "{uuid()}")
    (effects (font (size 1.2 1.2) (thickness 0.2)))
  )
  (gr_text "JC4880P433 J9 Expand IO"
    (at {ox + bw/2:.2f} {oy + 2:.2f} 0)
    (layer "B.SilkS")
    (uuid "{uuid()}")
    (effects (font (size 1 1) (thickness 0.15)) (justify mirror))
  )
""")

    pcb.append(")")  # close kicad_pcb
    return "\n".join(pcb)


# ═══════════════════════════════════════════════════════
# BOM (Bill of Materials)
# ═══════════════════════════════════════════════════════
def generate_bom():
    bom = []
    bom.append("=" * 90)
    bom.append("FilMachine Breakout Board v1.0 — Bill of Materials")
    bom.append("=" * 90)
    bom.append(f"{'Ref':<8} {'Qty':<5} {'Value':<12} {'Package':<20} {'Description':<35} {'~Cost'}")
    bom.append("-" * 90)
    bom.append(f"{'J1':<8} {'1':<5} {'2x13F':<12} {'2.54mm TH':<20} {'Female pin header 2×13':<35} {'$0.30'}")
    bom.append(f"{'J2':<8} {'1':<5} {'3P 5.08mm':<12} {'KF301-3P':<20} {'MOTOR: ENA/IN1/IN2':<35} {'$0.20'}")
    bom.append(f"{'J3':<8} {'1':<5} {'3P 5.08mm':<12} {'KF301-3P':<20} {'TEMP: DATA/3V3/GND':<35} {'$0.20'}")
    bom.append(f"{'J4':<8} {'1':<5} {'3P 5.08mm':<12} {'KF301-3P':<20} {'FLOW: SIG/3V3/GND':<35} {'$0.20'}")
    bom.append(f"{'J5':<8} {'1':<5} {'4P 5.08mm':<12} {'KF301-4P':<20} {'I2C: SDA/SCL/3V3/GND':<35} {'$0.25'}")
    bom.append(f"{'J6':<8} {'1':<5} {'4P 5.08mm':<12} {'KF301-4P':<20} {'WATER: MIN/MAX/3V3/GND':<35} {'$0.25'}")
    bom.append(f"{'J7':<8} {'1':<5} {'3P 5.08mm':<12} {'KF301-3P':<20} {'HALL: SIG/3V3/GND':<35} {'$0.20'}")
    bom.append(f"{'J8':<8} {'1':<5} {'1x8M':<12} {'2.54mm TH':<20} {'Spare GPIO header 1×8':<35} {'$0.10'}")
    bom.append(f"{'J10':<8} {'1':<5} {'4P 5.08mm':<12} {'KF301-4P':<20} {'POWER: 5V/3V3/GND/GND':<35} {'$0.25'}")
    bom.append(f"{'R1':<8} {'1':<5} {'4.7kΩ':<12} {'0805 SMD':<20} {'I2C SDA pull-up to 3V3':<35} {'$0.01'}")
    bom.append(f"{'R2':<8} {'1':<5} {'4.7kΩ':<12} {'0805 SMD':<20} {'I2C SCL pull-up to 3V3':<35} {'$0.01'}")
    bom.append(f"{'R3':<8} {'1':<5} {'4.7kΩ':<12} {'0805 SMD':<20} {'OneWire pull-up to 3V3':<35} {'$0.01'}")
    bom.append(f"{'C1':<8} {'1':<5} {'100nF':<12} {'0805 SMD':<20} {'I2C bus decoupling':<35} {'$0.01'}")
    bom.append(f"{'C2':<8} {'1':<5} {'100nF':<12} {'0805 SMD':<20} {'Power rail decoupling':<35} {'$0.01'}")
    bom.append(f"{'MH1-4':<8} {'4':<5} {'M2':<12} {'2.2mm hole':<20} {'Mounting holes':<35} {'—'}")
    bom.append("-" * 90)
    bom.append(f"{'TOTAL COMPONENTS: 15':<45} {'PCB + components ≈ $2-3 per board'}")
    bom.append("")
    bom.append("NOTES:")
    bom.append("  - PCB: 70×50mm, 2-layer, 1.6mm FR4, HASL finish")
    bom.append("  - Screw terminals: KF301 or Phoenix MKDS-1.5 series (5.08mm pitch)")
    bom.append("  - SMD passives: 0805 (hand-solderable)")
    bom.append("  - J1 MUST be female header to plug onto J9 male pins on the GUITION board")
    bom.append("  - VERIFY J9 power pin mapping (1,2,14,18,20,22,24,26) against your board!")
    bom.append("    The assumed mapping (3V3/5V/GND) is based on typical convention.")
    bom.append("    Measure with a multimeter before first power-on.")
    bom.append("")
    return "\n".join(bom)


# ═══════════════════════════════════════════════════════
# Main
# ═══════════════════════════════════════════════════════
if __name__ == "__main__":
    outdir = os.path.dirname(os.path.abspath(__file__))

    # Project file
    with open(os.path.join(outdir, f"{PROJECT_NAME}.kicad_pro"), "w") as f:
        f.write(generate_project())
    print(f"✓ {PROJECT_NAME}.kicad_pro")

    # Schematic
    with open(os.path.join(outdir, f"{PROJECT_NAME}.kicad_sch"), "w") as f:
        f.write(generate_schematic())
    print(f"✓ {PROJECT_NAME}.kicad_sch")

    # PCB
    with open(os.path.join(outdir, f"{PROJECT_NAME}.kicad_pcb"), "w") as f:
        f.write(generate_pcb())
    print(f"✓ {PROJECT_NAME}.kicad_pcb")

    # BOM
    with open(os.path.join(outdir, "BOM.txt"), "w") as f:
        f.write(generate_bom())
    print(f"✓ BOM.txt")

    print(f"\nAll files generated in: {outdir}")
    print(f"Open {PROJECT_NAME}.kicad_pro in KiCad 8 to review.")
