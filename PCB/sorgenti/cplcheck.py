# -*- coding: utf-8 -*-
"""Check a JLCPCB CPL against the board: place every EasyEDA footprint pad where JLCPCB will put it (Mid X/Y,
Rotation of the CPL) and measure the distance to the nearest board pad of that part; for U/J/Q also check that
EasyEDA pad 1 lands on board pad 1. Usage: cplcheck.py board.kicad_pcb CPL.csv BOM.csv"""
import sys, csv, math, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pcbnew
from jlcpos import eda_fp, rot
PCB, CPL, BOM = sys.argv[1:4]
b = pcbnew.LoadBoard(PCB)
lcsc = {}
for r in csv.DictReader(open(BOM)):
    for d in r["Designator"].split(","): lcsc[d.strip()] = r["LCSC Part #"]
worst = []; bad = []
for r in csv.DictReader(open(CPL)):
    ref = r["Designator"]; mx, my, th = float(r["Mid X"]), -float(r["Mid Y"]), float(r["Rotation"])
    _, epads, _ = eda_fp(lcsc[ref])
    kp = [(p.GetNumber(), p.GetPosition().x / 1e6, p.GetPosition().y / 1e6) for p in b.FindFootprintByReference(ref).Pads() if p.GetNumber()]
    dmax = 0.0
    for e in epads:
        dx, dy = rot(e["x"], e["y"], th); x, y = mx + dx, my + dy
        n, d = min(((k[0], math.hypot(k[1] - x, k[2] - y)) for k in kp), key=lambda t: t[1])
        dmax = max(dmax, d)
        if e["num"] == "1" and ref[0] in "UJQ" and n != "1" and d < 0.3:
            bad.append(f"{ref}: EasyEDA pad 1 on board pad {n}")
    worst.append((dmax, ref))
worst.sort(reverse=True)
print("parts:", len(worst), " worst pad offsets (mm):", [(r, round(d, 3)) for d, r in worst[:6]])
print("over 0.10 mm:", [r for d, r in worst if d > 0.10] or "none", "| pin-1 problems:", bad or "none")
