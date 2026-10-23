# -*- coding: utf-8 -*-
"""Silkscreen: references, connector names, input polarity legends, board name, the warranty line. Idempotent.
23/10/2026: every legend >= 1,0 mm high with 0,15 mm stroke (JLCPCB minimum); 12 V inputs: name + separate +/- marks at the pins.
v1.2 refs: J1-J3 12 V inputs (left), J5 display, J28/J29 service outputs (top), J7/J11-J14 sensors (right), J15-J22 valves (bottom),
J23/J24 heaters (top), J25 pump (left), J26 motor (bottom-left), J27 servo."""
import sys
import pcbnew
from placement import ORIGIN, BOARD_W, BOARD_H
PCB = sys.argv[1]
b = pcbnew.LoadBoard(PCB)
MM = pcbnew.FromMM
def P(x, y): return pcbnew.VECTOR2I(MM(ORIGIN[0] + x), MM(ORIGIN[1] + y))
old = [d for d in b.GetDrawings() if d.GetClass() == "PCB_TEXT" and d.GetLayer() in (pcbnew.F_SilkS, pcbnew.B_SilkS)]
for d in old:
    b.Delete(d)
pcbnew.SaveBoard(PCB, b)
b = pcbnew.LoadBoard(PCB)
def text(s, x, y, size=1.0, layer=pcbnew.F_SilkS, angle=0, bold=False, hjust=pcbnew.GR_TEXT_H_ALIGN_CENTER):
    t = pcbnew.PCB_TEXT(b); t.SetText(s); t.SetPosition(P(x, y)); t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(MM(size), MM(size))); t.SetTextThickness(MM(size * (0.2 if bold else 0.15)))
    t.SetTextAngle(pcbnew.EDA_ANGLE(angle, pcbnew.DEGREES_T)); t.SetHorizJustify(hjust)
    if layer == pcbnew.B_SilkS: t.SetMirrored(True)
    b.Add(t)

SMALL = ("R_0603", "C_0603", "C_0805", "LED_0603", "LED_0805", "D_SOD-323", "D_SOD-123", "C_1206", "C_1210",
         "Fuse_1206", "Fuse_1812", "SOT-23", "SolderJumper", "D_SMA")
for f in b.GetFootprints():
    name = str(f.GetFPID().GetLibItemName())
    ref = f.Reference(); ref.SetTextSize(pcbnew.VECTOR2I(MM(0.8), MM(0.8))); ref.SetTextThickness(MM(0.13))
    small = any(s in name for s in SMALL) and not name.startswith("SOT-23-5")
    ref.SetVisible(not (small or f.GetReference().startswith(("H", "J"))))
    v = f.Value(); v.SetVisible(False)

def pad(ref, num):
    f = b.FindFootprintByReference(ref)
    p = [q for q in f.Pads() if q.GetNumber() == str(num)][0].GetPosition()
    return p.x / 1e6 - ORIGIN[0], p.y / 1e6 - ORIGIN[1]

# connector names, placed where they stay visible after assembly
LABEL = {  # ref: (text, x, y, angle, size)
    "J23": ("HEATER 1", 25.6, 6.0, 90, 1.0), "J24": ("HEATER 2", 40.0, 6.0, 90, 1.0),
    "J5": ("DISPLAY JP1 (pin 1 = quadrato)", 70.0, 15.6, 0, 1.0),      # between the pin row and U5/R63-R65 (y >= 16.3)
    "J26": ("MOTOR", 12.8, 81.6, 0, 1.0),
}
VALVE = {"J15": "C1", "J16": "C2", "J17": "C3", "J18": "WB", "J19": "WASTE", "J20": "INLET", "J21": "SPARE 1", "J22": "SPARE 2"}
for f in b.GetFootprints():
    r = f.GetReference()
    c = f.GetCourtyard(pcbnew.F_CrtYd).BBox()
    cx = (c.GetLeft() + c.GetRight()) / 2e6 - ORIGIN[0]
    if r in LABEL:
        s, x, y, a, sz = LABEL[r]; text(s, x, y, sz, angle=a)
    elif r in VALVE:
        text(VALVE[r], cx, 81.6, 1.0)
    elif r in ("J7", "J11", "J12", "J13", "J14"):   # right-edge JST column, name to the left, vertical
        text(f.GetValue(), c.GetLeft() / 1e6 - ORIGIN[0] - 0.9, (c.GetTop() + c.GetBottom()) / 2e6 - ORIGIN[1], 1.0, angle=90)
    elif r in ("J27", "J6"):                        # name above (J6: shifted right, clear of C62)
        text(f.GetValue(), cx + (1.6 if r == "J6" else 0), c.GetTop() / 1e6 - ORIGIN[1] - 0.9, 1.0)

# supply inputs and pump on the left edge (pin 1 = GND above, pin 2 = +12 V below): a bold "-" / "+" mark at the
# level of each pin in the corridor right of the terminal body (x 11.1, before F1/F3), and the name of the input
# where there is room: J2 vertical in the corridor (marks moved to x 12.7), J1 below it, J3 above it, J25 above it.
for ref, up, dn, xm in (("J2", "-", "+", 12.7), ("J1", "-", "+", 11.1), ("J3", "-", "+", 11.1), ("J25", "A", "B", 11.1)):
    text(up, xm, pad(ref, 1)[1], 1.0, bold=True)
    text(dn, xm, pad(ref, 2)[1], 1.0, bold=True)
text("12V IN H2", 11.1, (pad("J2", 1)[1] + pad("J2", 2)[1]) / 2, 1.0, angle=90)
text("12V IN H1", 5.6, 31.2, 1.0)
text("12V IN MAIN", 5.6, 34.6, 1.0)
text("PUMP", 5.6, 50.4, 1.0)
# heater outputs: polarity marks under the terminal, between it and D41/D42
for ref in ("J23", "J24"):
    text("+", pad(ref, 2)[0], 11.3, 1.0, bold=True)
    text("-", pad(ref, 1)[0], 11.3, 1.0, bold=True)
text("VALVOLE 12V: + sx  - dx", 32.0, 80.0, 1.0)
# service outputs on the top edge: name + polarity under each terminal (pin 2 = + on the left, pin 1 = GND)
f8 = b.FindFootprintByReference("F8").GetCourtyard(pcbnew.F_CrtYd).BBox().GetTop() / 1e6 - ORIGIN[1]
if f8 > 9.0:      # PTC block lowered (placement >= v18): label in the gap between terminal body and PTCs
    for ref, s in (("J28", "+ 12V -"), ("J29", "+ 5V -")):
        f = b.FindFootprintByReference(ref); c = f.GetCourtyard(pcbnew.F_CrtYd).BBox()
        cx = (c.GetLeft() + c.GetRight()) / 2e6 - ORIGIN[0]
        text(s, cx, 8.45, 1.0, bold=True)      # pin 2 = + on the left
    # name and current limit (PTC) of the two outputs: vertical, right of J29 (between it and the mounting hole)
    text("12V OUT 2A", 100.6, 5.0, 0.9, angle=90)
    text("5V OUT 1A", 102.0, 5.0, 0.9, angle=90)
else:             # PTC block right under the terminals: two lines below the PTCs
    text("sx 12V OUT 2A  dx 5V OUT 1A", 91.6, 15.0, 0.7, bold=True)
    text("+ a sinistra, - a destra", 91.6, 16.1, 0.7)
# spare GPIO header (horizontal, pin 1 on the left): pin names under the pins on two staggered rows (1,0 mm text
# does not fit the 2,54 mm pitch on one row), title under the names
for n, nm in enumerate(("B0", "B1", "B2", "B3", "B4", "B5", "3V3", "5V", "GND")):
    x, y = pad("J30", n + 1)
    text(nm, x, y + (3.0 if n % 2 == 0 else 4.7), 1.0)
(x1, y1), (x9, _) = pad("J30", 1), pad("J30", 9)
text("GPIO SPARE 3V3", (x1 + x9) / 2, y1 + 6.5, 1.0, bold=True)
for ref, s in (("F1", "15A"), ("F2", "15A"), ("F3", "15A"), ("F4", "8A")):
    f = b.FindFootprintByReference(ref); c = f.GetCourtyard(pcbnew.F_CrtYd).BBox()
    if abs(f.GetOrientationDegrees()) == 90:    # vertical fuse: value beside it (F2: on the left, away from Q10's reference)
        xs = c.GetLeft() / 1e6 - ORIGIN[0] - 0.8 if ref == "F2" else c.GetRight() / 1e6 - ORIGIN[0] + 0.8
        text(s, xs, (c.GetTop() + c.GetBottom()) / 2e6 - ORIGIN[1], 1.0, angle=90)
    else:
        text(s, (c.GetLeft() + c.GetRight()) / 2e6 - ORIGIN[0], c.GetTop() / 1e6 - ORIGIN[1] - 0.85, 1.0)

text("FilMachine", 6.6, 70.8, 1.4, bold=True)
text("Board v1.2", 6.6, 73.2, 1.4, bold=True)
text("2026-10-23", 6.6, 75.3, 1.0)
text("If you can read this,", 97.1, 68.0, 1.0, angle=90)
text("you have voided your warranty", 98.9, 68.0, 1.0, angle=90)

text("FilMachine Board v1.2 - 2026-10-23", 55.0, 45.0, 1.5, layer=pcbnew.B_SilkS)
text("4 layers 1.6 mm - outer 2 oz / inner 1 oz", 55.0, 48.0, 1.0, layer=pcbnew.B_SilkS)
text("JLCJLCJLCJLC", 55.0, 42.5, 1.2, layer=pcbnew.B_SilkS)   # JLCPCB replaces this with its order number (choose "specify a location")
text("L2 GND  L3 +12V", 55.0, 50.0, 1.0, layer=pcbnew.B_SilkS)
pcbnew.SaveBoard(PCB, b)
print("silk ok")
