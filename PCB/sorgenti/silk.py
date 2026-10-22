# -*- coding: utf-8 -*-
"""Silkscreen: references, connector names, input polarity legends, board name, the warranty line. Idempotent.
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
def text(s, x, y, size=1.0, layer=pcbnew.F_SilkS, angle=0, bold=False):
    t = pcbnew.PCB_TEXT(b); t.SetText(s); t.SetPosition(P(x, y)); t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(MM(size), MM(size))); t.SetTextThickness(MM(size * (0.2 if bold else 0.15)))
    t.SetTextAngle(pcbnew.EDA_ANGLE(angle, pcbnew.DEGREES_T)); t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
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
    "J23": ("HEATER 1", 25.6, 6.0, 90, 0.9), "J24": ("HEATER 2", 40.0, 6.0, 90, 0.9),
    "J5": ("DISPLAY JP1  (pin 1 = quadrato)", 63.0, 15.3, 0, 0.8),
    "J26": ("MOTOR", 12.8, 81.7, 0, 0.9),
}
VALVE = {"J15": "C1", "J16": "C2", "J17": "C3", "J18": "WB", "J19": "WASTE", "J20": "INLET", "J21": "SPARE 1", "J22": "SPARE 2"}
for f in b.GetFootprints():
    r = f.GetReference()
    c = f.GetCourtyard(pcbnew.F_CrtYd).BBox()
    cx = (c.GetLeft() + c.GetRight()) / 2e6 - ORIGIN[0]
    if r in LABEL:
        s, x, y, a, sz = LABEL[r]; text(s, x, y, sz, angle=a)
    elif r in VALVE:
        text(VALVE[r], cx, 81.6, 0.8)
    elif r in ("J7", "J11", "J12", "J13", "J14"):   # right-edge JST column, name to the left, vertical
        text(f.GetValue(), c.GetLeft() / 1e6 - ORIGIN[0] - 0.8, (c.GetTop() + c.GetBottom()) / 2e6 - ORIGIN[1], 0.8, angle=90)
    elif r in ("J27", "J6"):                        # name above
        text(f.GetValue(), cx, c.GetTop() / 1e6 - ORIGIN[1] - 0.6, 0.8)

# supply inputs and pump on the left edge: vertical legend right next to the terminal (x 11.1, between the
# terminal body and F1/F3), reading bottom-to-top so that the first character sits at the LOWER pin (pin 2 = +)
for ref, s in (("J2", "+ 12V IN HEATER 2 -"), ("J1", "+ 12V IN HEATER 1 -"), ("J3", "+ 12V IN MAIN -"), ("J25", "B  PUMP  A")):
    (x1, y1), (x2, y2) = pad(ref, 1), pad(ref, 2)
    text(s, 11.1, (y1 + y2) / 2, 0.7, angle=90, bold=True)
# heater outputs: polarity marks under the terminal, between it and D41/D42
for ref in ("J23", "J24"):
    text("+", pad(ref, 2)[0], 11.3, 0.8, bold=True)
    text("-", pad(ref, 1)[0], 11.3, 0.8, bold=True)
text("VALVOLE 12V:  + sx   - dx", 33.0, 80.0, 0.7)
# service outputs on the top edge: name + polarity under each terminal (pin 2 = + on the left, pin 1 = GND)
f8 = b.FindFootprintByReference("F8").GetCourtyard(pcbnew.F_CrtYd).BBox().GetTop() / 1e6 - ORIGIN[1]
if f8 > 9.0:      # PTC block lowered (placement >= v18): label in the gap between terminal body and PTCs
    for ref, s in (("J28", "+ 12V OUT 2A -"), ("J29", "+ 5V OUT 1A -")):
        f = b.FindFootprintByReference(ref); c = f.GetCourtyard(pcbnew.F_CrtYd).BBox()
        cx = (c.GetLeft() + c.GetRight()) / 2e6 - ORIGIN[0]
        text(s, cx, 8.45, 0.65, bold=True)     # pin 2 = + on the left
else:             # PTC block right under the terminals: two lines below the PTCs
    text("sx 12V OUT 2A  dx 5V OUT 1A", 91.6, 15.0, 0.7, bold=True)
    text("+ a sinistra, - a destra", 91.6, 16.1, 0.7)
# spare GPIO header (horizontal, pin 1 on the left): pin names under the pins, title under the names
for n, nm in enumerate(("B0", "B1", "B2", "B3", "B4", "B5", "3V3", "5V", "GND")):
    x, y = pad("J30", n + 1)
    text(nm, x, y + 2.1, 0.6)
(x1, y1), (x9, _) = pad("J30", 1), pad("J30", 9)
text("GPIO SPARE  (3,3 V)", (x1 + x9) / 2, y1 + 3.4, 0.7, bold=True)
for ref, s in (("F1", "15A"), ("F2", "15A"), ("F3", "15A"), ("F4", "8A")):
    f = b.FindFootprintByReference(ref); c = f.GetCourtyard(pcbnew.F_CrtYd).BBox()
    if abs(f.GetOrientationDegrees()) == 90:    # vertical fuse: value beside it (F2: on the left, away from Q10's reference)
        xs = c.GetLeft() / 1e6 - ORIGIN[0] - 0.6 if ref == "F2" else c.GetRight() / 1e6 - ORIGIN[0] + 0.6
        text(s, xs, (c.GetTop() + c.GetBottom()) / 2e6 - ORIGIN[1], 0.7, angle=90)
    else:
        text(s, (c.GetLeft() + c.GetRight()) / 2e6 - ORIGIN[0], c.GetTop() / 1e6 - ORIGIN[1] - 0.6, 0.7)

text("FilMachine", 6.6, 70.8, 1.4, bold=True)
text("Board v1.2", 6.6, 73.2, 1.4, bold=True)
text("2026-10-22", 6.6, 75.2, 0.8)
text("If you can read this,", 97.2, 68.0, 0.9, angle=90)
text("you have voided your warranty", 98.9, 68.0, 0.9, angle=90)

text("FilMachine Board v1.2 - 2026-10-22", 55.0, 45.0, 1.5, layer=pcbnew.B_SilkS)
text("4 layers 1.6 mm - outer 2 oz / inner 1 oz", 55.0, 48.0, 1.0, layer=pcbnew.B_SilkS)
text("JLCJLCJLCJLC", 55.0, 42.5, 1.2, layer=pcbnew.B_SilkS)   # JLCPCB replaces this with its order number (choose "specify a location")
text("L2 GND  L3 +12V", 55.0, 50.0, 1.0, layer=pcbnew.B_SilkS)
pcbnew.SaveBoard(PCB, b)
print("silk ok")
