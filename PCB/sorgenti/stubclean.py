# -*- coding: utf-8 -*-
"""Delete pre-routed escape stubs whose free end the autorouter did not use (DRC 'track_dangling'). Usage: stubclean.py board"""
import sys, pcbnew
PCB = sys.argv[1]
b = pcbnew.LoadBoard(PCB)
tracks = [t for t in b.GetTracks() if t.GetClass() == "PCB_TRACK"]
vias = [t for t in b.GetTracks() if t.GetClass() == "PCB_VIA"]
def touched(pt, layer, me):
    for t in tracks:
        if t is me or t.GetLayer() != layer: continue
        if t.GetStart() == pt or t.GetEnd() == pt: return True
    for v in vias:
        if v.GetPosition() == pt: return True
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.IsOnLayer(layer) and p.HitTest(pt): return True
    return False
removed = 0
for t in list(tracks):
    if abs(t.GetWidth() / 1e6 - 0.2) > 1e-3: continue
    if not touched(t.GetEnd(), t.GetLayer(), t) or not touched(t.GetStart(), t.GetLayer(), t):
        print("remove dangling", t.GetNetname(), t.GetLength() / 1e6)
        b.Remove(t); removed += 1
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(PCB, b)
print("removed", removed)
