# -*- coding: utf-8 -*-
"""Placement v4 (JLCPCB parts). Board-local mm, (0,0) = top-left of outline; each entry is the TOP-LEFT corner of
the rotated courtyard. Terminal blocks: wire entry is local +y -> rot 180 = top edge, 0 = bottom, 270 = left, 90 = right.
Physical layout: the three 12 V inputs (heater 2, heater 1, main) and the pump on the left edge; heater outputs,
display ribbon and the 12 V / 5 V service outputs on the top edge; valves + spare outputs (one 2-pole block each) on the
bottom edge; all sensors on the right."""

BOARD_W, BOARD_H = 110.0, 90.0
ORIGIN = (100.0, 100.0)
PLACE = {}
def at(ref, x, y, rot=0, side="F"):
    PLACE[ref] = (x, y, rot, side)

W, H = BOARD_W, BOARD_H
at("H1", 0.5, 0.5); at("H2", W - 7.5, 0.5); at("H3", 0.5, H - 7.5); at("H4", W - 7.5, H - 7.5)

# ── left edge: 12 V inputs (WJ500V 5.08, pins at x = 4.8; pin 1 = GND upper, pin 2 = + lower), then the pump ──
at("J2", 0.0, 8.0, 270)       # 12V IN HEATER 2: pin 2 (4.8, 15.9) -> strip on B/In2 (y 13-19.4) -> F2 in lane 2
at("J1", 0.0, 19.0, 270)      # 12V IN HEATER 1: pin 2 (4.8, 26.9) -> IN_H1 zone (y 24.3-30.2) -> F1 in lane 1
at("J3", 0.0, 36.0, 270)      # 12V IN MAIN:     pin 2 (4.8, 43.9) -> Z7 -> F3 (pump) / F4 (+12V)
at("J25", 0.0, 54.0, 270)     # PUMP: pin 1 OUT_A (4.8, 56.8), pin 2 OUT_B (4.8, 61.9)

# ── heater block: outputs on the top edge, lane 1 (x 11.6-26.8) and lane 2 (x 27.4-42) below ─────────
at("J23", 13.2, 0.0, 180)     # HEATER 1 (pin 2 = + at x 16.06, pin 1 = - at x 21.14)
at("J24", 27.2, 0.0, 180)     # HEATER 2 (pin 2 = + at x 30.06, pin 1 = - at x 35.15)
at("F1", 11.8, 20.7, 90)      # 2410 vertical: pad 2 (13.6, 22.25) in Z1 tab, pad 1 (13.6, 27.75) in IN_H1
at("F2", 31.0, 16.3, 90)      # 2410 vertical: pad 2 (32.8, 17.85) in Z3, pad 1 (32.8, 23.35) in IN_H2_R
at("D41", 16.5, 12.5, 0)
at("D42", 30.5, 12.5, 0)
at("Q9", 20.0, 16.4, 90)
at("Q10", 35.0, 16.4, 90)
at("U5", 44.0, 16.0, 0)
at("C40", 51.6, 16.3, 90)     # 1 uF next to U5 VDD (pin 6), with room below for its +12V via

# ── main input fuses (2410 horizontal), TVS ───────────────────────────────────────────────────────────
at("F3", 14.71, 37.01, 0)     # centre (19.0, 38.8): pad 1 in +12V_IN_M, pad 2 in +12V_PUMP
at("F4", 14.71, 44.91, 0)     # centre (19.0, 46.7): pad 1 in +12V_IN_M, pad 2 in +12V
at("D2", 31.0, 36.0, 0)
at("D1", 31.0, 44.0, 0)

# ── pump bridge ─────────────────────────────────────────────────────────────────────────
at("U8", 13.0, 52.0, 180)
at("U9", 13.0, 67.0, 180)
at("C52", 30.3, 52.2, 270)
at("C57", 33.4, 52.0, 270)
at("C53", 30.3, 67.2, 270)
at("C55", 37.8, 52.0, 0)
at("C56", 37.8, 63.5, 0)

# ── motor (bottom-left) ─────────────────────────────────────────────────────────────────
at("J26", 9.0, H - 7.48, 0)   # MOTOR (DB301V 3.5)
at("U10", 18.0, 81.6, 180)

# ── top edge: display ribbon (right-angle), then the 12 V / 5 V service outputs ─────────
at("J5", 42.0, 0.0, 90)
at("J28", 84.2, 0.0, 180)     # 12V OUT (PTC F8, 2 A)
at("J29", 92.0, 0.0, 180)     # 5V OUT  (PTC F5, 1.1 A)

# ── centre: buck, logic ────────────────────────────────────────────────────────────────
at("U1", 46.3, 24.4, 0)       # LMR33630: VIN/PGND left, SW/BOOT right
at("C4", 44.1, 24.2, 90)      # Cin HF right at VIN/PGND
at("C2", 43.3, 30.2, 90)
at("C3", 43.3, 35.0, 90)
at("C5", 54.0, 24.4, 90)      # bootstrap BOOT-SW
at("C6", 54.0, 27.7, 270)     # VCC
at("R1", 47.0, 30.6, 0)
at("R2", 50.2, 30.6, 0)
at("L1", 55.6, 23.4, 0)
at("C7", 55.6, 30.9, 0); at("C8", 60.4, 30.9, 0); at("C9", 55.6, 33.5, 0); at("C10", 60.4, 33.5, 0)
at("U3", 65.4, 26.0, 0)       # MCP23017
at("U4", 52.4, 65.0, 90)      # ADS1115 (0.5 mm pitch): pins 6-10 (+3V3, I2C, pump IS) on top, 1-5 below; room all round
at("C15", 54.0, 61.7, 0)      # ADS1115 decoupling, right above VDD (pin 8)
at("C1", 53.0, 54.5, 0)
at("J6", 62.0, 52.6, 90)      # C6 UART (DNP)
at("J27", 77.0, 52.6, 90)     # SERVO

# ── right edge: sensors, ONE column between the two M3 holes (v1.2: no chemistry level sensors) ──
# JST-XH courtyards 13.5 (4 pin) / 11.0 mm (3 pin), 3 mm apart: y 9.0 ... 78.5 (hole H4 starts at 82.5)
_y = 9.0
for _ref, _h in (("J7", 13.5), ("J11", 11.0), ("J12", 11.0), ("J13", 11.0), ("J14", 11.0)):
    at(_ref, W - 7.0, _y, 90); _y += _h + 3.0
# GPIO SPARE header (B0-B5, 3V3, 5V, GND), horizontal, in the area freed by the chemistry inputs
at("J30", 76.0, 31.6, 90)

# ── bottom edge: one 2-pole terminal per valve ─────────────────────────────────────────
for i in range(8):
    at(f"J{15+i}", 41.0 + i * 7.6, H - 7.48, 0)

BLOCKS = [
    ((53.9, 16.3, 70.0, 23.4), ["C41", "R63", "R65", "R64", "R66", "R61", "R62", "R67", "D43", "R68", "D44"], 0),
    ((59.2, 40.5, 62.0, 54.0), ["F6"], 90),                  # right of J30 and its pin labels
    ((75.2, 25.4, 100.6, 31.3), [x for k in range(6) for x in (f"R{97+k}", f"D{47+k}")], 0),   # J30 series R + clamps, above J30
    ((62.2, 41.4, 72.6, 52.2), ["F7", "D8", "C62"], 0),      # display supply via JP1: PTC + Schottky
    ((84.0, 9.4, 102.3, 17.9), ["F8", "C63", "F5", "C64"], 0),   # PTCs + caps of the service outputs (label gap above)
    ((86.0, 41.0, 95.0, 48.6), ["R3", "D3", "R4", "D4", "R5", "D5"], 0),   # 0805 LEDs: three rows
    ((29.9, 57.0, 36.3, 60.7), ["R80", "R85", "C54"], 0),
    ((29.9, 71.0, 36.3, 75.7), ["R82", "R88", "C61"], 0),
    ((42.4, 44.5, 52.9, 51.7), ["C51", "U7", "C50", "U6"], 0),
    ((26.0, 81.0, 36.3, 89.5), ["R86", "C58", "C59"], 0),
    ((62.0, 57.2, 76.0, 61.0), ["R70", "R71", "R72", "R73", "R74", "R75", "R76", "R84"], 0),
    ((86.0, 52.6, 95.0, 56.5), ["R87", "C60"], 0),
    ((65.4, 37.0, 75.15, 40.85), ["C13", "R8", "C14", "R6", "R7"], 0),
    ((73.3, 41.0, 84.6, 52.3), ["R9", "R10", "C16", "R11", "R12", "C17"], 0),
    ((47.6, 72.9, 58.8, 81.0), ["R13", "C18", "D6", "R14", "C19", "D7"], 0),
    ((75.2, 18.0, 95.4, 25.2), [x for i in range(4) for x in (f"D{10+i}", f"R{20+i}", f"C{20+i}")] + ["R16"], 0),
] + [
    ((59.0 + (i % 4) * 9.0, 63.0 + (i // 4) * 9.0, 67.8 + (i % 4) * 9.0, 71.7 + (i // 4) * 9.0),
     [f"Q{i+1}", f"R{31+i}", f"R{41+i}", f"D{21+i}", f"R{51+i}", f"D{31+i}"], 0)
    for i in range(8)
]
