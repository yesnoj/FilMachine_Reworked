# -*- coding: utf-8 -*-
"""Silkscreen: references, connector names, input polarity legends, board name, the warranty line. Idempotent."""
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

SMALL = ("R_0603", "C_0603", "C_0805", "LED_0603", "D_SOD-323", "D_SOD-123", "C_1206", "C_1210", "Fuse_1206",
         "Fuse_1812", "SOT-23", "SolderJumper", "D_SMA")
for f in b.GetFootprints():
    name = str(f.GetFPID().GetLibItemName())
    ref = f.Reference(); ref.SetTextSize(pcbnew.VECTOR2I(MM(0.8), MM(0.8))); ref.SetTextThickness(MM(0.13))
    small = any(s in name for s in SMALL) and not name.startswith("SOT-23-5")
    ref.SetVisible(not (small or f.GetReference().startswith(("H", "J"))))
    v = f.Value(); v.SetVisible(False)

# connector names, placed where they stay visible after assembly
LABEL = {  # ref: (text, x, y, angle, size)
    "J18": ("HEATER 1", 25.65, 6.0, 90, 0.9), "J19": ("HEATER 2", 40.15, 6.0, 90, 0.9),
    "J4": ("DISPLAY JP1  (pin 1 = quadrato)", 63.0, 15.3, 0, 0.8),
    "J21": ("MOTOR", 13.05, 80.4, 0, 0.9), "J14": ("VALVE C1 | C2", 48.8, 80.4, 0, 0.9),
    "J15": ("VALVE C3 | WB", 64.1, 80.4, 0, 0.9), "J16": ("WASTE | INLET", 79.4, 80.4, 0, 0.9),
    "J17": ("SPARE A6 | A7", 94.6, 80.4, 0, 0.9),
}
for f in b.GetFootprints():
    r = f.GetReference()
    if r in LABEL:
        s, x, y, a, sz = LABEL[r]; text(s, x, y, sz, angle=a)
    elif r in ("J6", "J7", "J8", "J9", "J10"):
        c = f.GetCourtyard(pcbnew.F_CrtYd).BBox()
        text(f.GetValue(), c.GetLeft() / 1e6 - ORIGIN[0] - 0.8, (c.GetTop() + c.GetBottom()) / 2e6 - ORIGIN[1], 0.8, angle=90)
    elif r in ("J11", "J12", "J13", "J22", "J5"):
        c = f.GetCourtyard(pcbnew.F_CrtYd).BBox()
        text(f.GetValue(), (c.GetLeft() + c.GetRight()) / 2e6 - ORIGIN[0], c.GetTop() / 1e6 - ORIGIN[1] - 0.6, 0.8)

# supply inputs: the only connectors where polarity matters
text("12V HEAT IN", 6.3, 17.0, 0.8, bold=True)
text("-  sopra", 6.3, 18.5, 0.8)
text("+  sotto", 6.3, 19.9, 0.8)
text("12V MAIN IN", 6.3, 48.4, 0.8, bold=True)
text("- sopra + sotto", 6.3, 49.9, 0.8)
text("PUMP", 6.3, 66.4, 0.9, bold=True)
text("USB-C", 99.8, 2.4, 0.8, bold=True)   # J3: 5 V output only
text("5V OUT", 99.8, 3.9, 0.8)

text("FilMachine", 6.6, 70.8, 1.4, bold=True)
text("Board v1.0", 6.6, 73.2, 1.4, bold=True)
text("2026-10", 6.6, 75.2, 0.8)
text("If you can read this,", 97.2, 68.0, 0.9, angle=90)
text("you have voided your warranty", 98.9, 68.0, 0.9, angle=90)

text("FilMachine Board v1.0 - 2026-10", 55.0, 45.0, 1.5, layer=pcbnew.B_SilkS)
text("4 layers 1.6 mm - outer 2 oz / inner 1 oz", 55.0, 48.0, 1.0, layer=pcbnew.B_SilkS)
text("L2 GND  L3 +12V", 55.0, 50.0, 1.0, layer=pcbnew.B_SilkS)
pcbnew.SaveBoard(PCB, b)
print("silk ok")
