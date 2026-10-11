# -*- coding: utf-8 -*-
"""Placement v3. Board-local mm, (0,0) = top-left of outline; each entry is the TOP-LEFT corner of the rotated
courtyard. Terminal blocks: wire entry is local +y -> rot 180 = top edge, 0 = bottom, 270 = left, 90 = right.
Physical layout: PSU inputs together on the left edge; display ribbon + display USB on the top edge;
heaters top-left; pump/motor left; valves bottom edge; all sensors right edge."""

BOARD_W, BOARD_H = 110.0, 90.0
ORIGIN = (100.0, 100.0)
PLACE = {}
def at(ref, x, y, rot=0, side="F"):
    PLACE[ref] = (x, y, rot, side)

W, H = BOARD_W, BOARD_H
at("H1", 0.5, 0.5); at("H2", W - 7.5, 0.5); at("H3", 0.5, H - 7.5); at("H4", W - 7.5, H - 7.5)

# ── left edge: both PSU inputs, then pump ──────────────────────────────────────────────
at("J1", 0.0, 22.0, 270)      # 12V HEAT IN  (pin1 GND upper, pin2 + lower)
at("J2", 0.0, 36.0, 270)      # 12V MAIN IN  (pin1 + upper, pin2 GND lower)
at("J20", 0.0, 54.0, 270)     # PUMP

# ── heater block: terminals on the top edge, fuse | MOSFET under each ─────────────────
at("J18", 13.0, 0.0, 180)     # HEATER 1 (pin2 +, pin1 -)
at("J19", 27.0, 0.0, 180)     # HEATER 2
at("D41", 16.5, 12.5, 0)
at("D42", 30.5, 12.5, 0)
at("F1", 12.43, 16.4, 90)
at("Q9", 20.0, 16.4, 90)
at("F2", 27.4, 16.4, 90)
at("Q10", 35.0, 16.4, 90)
at("U5", 44.0, 16.0, 0)

# ── main input fuses, TVS ───────────────────────────────────────────────────────────────
at("F3", 13.0, 35.5, 0)
at("F4", 13.0, 43.2, 0)
at("D2", 31.0, 35.5, 0)
at("D1", 31.0, 43.2, 0)

# ── pump bridge ─────────────────────────────────────────────────────────────────────────
at("U8", 13.0, 52.0, 180)
at("U9", 13.0, 67.0, 180)
at("C52", 30.3, 52.2, 270)
at("C57", 33.4, 52.0, 270)
at("C53", 30.3, 67.2, 270)
at("C55", 37.8, 52.0, 0)
at("C56", 37.8, 63.5, 0)

# ── motor (bottom-left) ─────────────────────────────────────────────────────────────────
at("J21", 9.0, H - 8.69, 0)
at("U10", 18.0, 81.6, 180)

# ── top edge: display ribbon (right-angle) + display USB ───────────────────────────────
at("J4", 42.0, 0.0, 90)
at("J3", 86.4, -1.075, 180)      # USB-C 5V OUT: mouth 0.575 mm past the top edge

# ── centre: buck, LDO, logic ───────────────────────────────────────────────────────────
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
at("U4", 51.0, 63.4, 0)       # MCP3424, next to the pump it measures
at("C1", 53.0, 54.2, 0)
at("J5", 62.0, 52.6, 90)      # C6 UART (DNP)
at("J22", 77.0, 52.6, 90)     # SERVO

# ── right edge: sensors ─────────────────────────────────────────────────────────────────
at("J6", W - 7.0, 9.0, 90)
at("J7", W - 7.0, 23.0, 90)
at("J8", W - 7.0, 37.0, 90)
at("J9", W - 7.0, 51.0, 90)
at("J10", W - 7.0, 65.0, 90)
at("J11", W - 14.4, 23.0, 90)
at("J12", W - 14.4, 35.0, 90)
at("J13", W - 14.4, 47.0, 90)

# ── bottom edge: valve terminals ───────────────────────────────────────────────────────
for i, ref in enumerate(["J14", "J15", "J16", "J17"]):
    at(ref, 41.3 + i * 15.28, H - 8.69, 0)

BLOCKS = [
    ((52.0, 16.0, 69.0, 22.8), ["R63", "R64", "R65", "R66", "R61", "R62", "C40", "C41", "R67", "D43", "R68", "D44"], 0),
    ((53.0, 40.5, 61.8, 54.0), ["U2", "C11", "C12", "F6", "F5"], 0),
    ((62.2, 41.4, 72.6, 52.2), ["F7", "D8", "C62"], 0),      # display supply via JP1: PTC + Schottky
    ((84.0, 7.6, 102.3, 17.7), ["Q11", "Q12", "C63", "R92", "U11", "C65", "C64", "R95", "R96", "D45", "D46", "R89", "R90", "R91"], 0),
    ((84.0, 0.6, 86.2, 7.4), ["R93", "R94"], 90),   # USB-C threshold divider, between J4 and J3,   # USB-C cold-socket switch
    ((86.0, 41.0, 95.0, 46.8), ["R3", "D3", "R4", "D4", "R5", "D5"], 0),
    ((29.9, 57.0, 36.3, 60.7), ["R80", "R85", "C54"], 0),
    ((29.9, 71.0, 36.3, 75.7), ["R82", "R88", "C61"], 0),
    ((42.4, 44.5, 52.9, 51.7), ["C51", "U7", "C50", "U6"], 0),
    ((26.0, 81.0, 36.3, 89.5), ["R86", "C58", "C59"], 0),
    ((62.0, 57.2, 76.0, 61.0), ["R70", "R71", "R72", "R73", "R74", "R75", "R76", "R84"], 0),
    ((86.0, 52.6, 95.0, 56.5), ["R87", "C60"], 0),
    ((65.4, 37.0, 75.15, 40.85), ["C13", "R8", "C14", "R6", "R7"], 0),
    ((73.3, 41.0, 84.6, 52.3), ["R9", "R10", "C16", "R11", "R12", "C17"], 0),
    ((47.6, 72.9, 58.8, 81.0), ["C15", "R13", "C18", "D6", "R14", "C19", "D7"], 0),
    ((75.2, 18.0, 95.4, 40.6), [x for i in range(10) for x in (f"D{10+i}", f"R{20+i}", f"C{20+i}")] + ["R16"], 0),
] + [
    ((59.0 + (i % 4) * 9.0, 63.0 + (i // 4) * 9.0, 67.8 + (i % 4) * 9.0, 71.7 + (i // 4) * 9.0),
     [f"Q{i+1}", f"R{31+i}", f"R{41+i}", f"D{21+i}", f"R{51+i}", f"D{31+i}"], 0)
    for i in range(8)
]
