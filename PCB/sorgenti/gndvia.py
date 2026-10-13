# -*- coding: utf-8 -*-
"""Add a GND via + short track for an SMD GND pad the fanout/maze could not tie. Usage: gndvia.py board REF PADNUM"""
import sys, math, pcbnew
PCB, REF, NUM = sys.argv[1], sys.argv[2], sys.argv[3]
b = pcbnew.LoadBoard(PCB)
MM = pcbnew.FromMM
fp = b.FindFootprintByReference(REF)
pad = [p for p in fp.Pads() if p.GetNumber() == NUM][0]
gnd = b.FindNet("GND").GetNetCode()
ppos = pad.GetPosition()
pads = [p for f in b.GetFootprints() for p in f.Pads()]
tracks = list(b.GetTracks())
def clear_at(pos, radius, layer):
    for p in pads:
        if p.GetNetCode() == gnd: continue
        if (p.IsOnLayer(layer) or p.GetAttribute() == pcbnew.PAD_ATTRIB_PTH) and p.HitTest(pos, int(radius)): return False
    for t in tracks:
        if t.GetNetCode() == gnd: continue
        if t.GetClass() == "PCB_VIA":
            if t.HitTest(pos, int(radius)): return False
        elif t.GetLayer() == layer and t.HitTest(pos, int(radius)): return False
    return True
cands = sorted((math.hypot(i * 0.2, j * 0.2), i * 0.2, j * 0.2) for i in range(-13, 14) for j in range(-13, 14)
               if 0.75 <= math.hypot(i * 0.2, j * 0.2) <= 3.6)
found = None
for d, dx, dy in cands:
    vpos = pcbnew.VECTOR2I(int(ppos.x + MM(dx)), int(ppos.y + MM(dy)))
    if not all(clear_at(vpos, MM(0.3 + 0.205), L) for L in (pcbnew.F_Cu, pcbnew.B_Cu, pcbnew.In1_Cu, pcbnew.In2_Cu)): continue
    n = 8
    if all(clear_at(pcbnew.VECTOR2I(int(ppos.x + (vpos.x - ppos.x) * k / n), int(ppos.y + (vpos.y - ppos.y) * k / n)), MM(0.125 + 0.205), pcbnew.F_Cu) for k in range(1, n)):
        found = (dx, dy, vpos); break
print("candidate:", found and (round(found[0], 2), round(found[1], 2)))
if found:
    dx, dy, vpos = found
    v = pcbnew.PCB_VIA(b); v.SetPosition(vpos); v.SetWidth(MM(0.6)); v.SetDrill(MM(0.3)); v.SetNet(b.FindNet("GND")); v.SetIsFree(False); b.Add(v)
    t = pcbnew.PCB_TRACK(b); t.SetStart(ppos); t.SetEnd(vpos); t.SetWidth(MM(0.25)); t.SetLayer(pcbnew.F_Cu); t.SetNet(b.FindNet("GND")); b.Add(t)
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    pcbnew.SaveBoard(PCB, b)
    print("added")
