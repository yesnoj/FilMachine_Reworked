# -*- coding: utf-8 -*-
"""Check and add a hand-made route. Usage: handroute.py board NET W  "L:x,y;x,y;..." ["L:x,y;..." ...]  (L = F or B;
consecutive polylines on different layers are joined by a via at the shared point). Coordinates in board mm.
Prints conflicts with other-net copper (pads, tracks, vias, zone outlines are not checked) and adds the route only
if --add is given and no conflict was found."""
import sys, math, pcbnew
from placement import ORIGIN
PCB, NET, W = sys.argv[1], sys.argv[2], float(sys.argv[3])
specs = [a for a in sys.argv[4:] if not a.startswith("--")]
ADD = "--add" in sys.argv
b = pcbnew.LoadBoard(PCB)
MM = pcbnew.FromMM
def P(x, y): return pcbnew.VECTOR2I(MM(ORIGIN[0] + x), MM(ORIGIN[1] + y))
net = b.FindNet(NET); nc = net.GetNetCode()
pads = [p for f in b.GetFootprints() for p in f.Pads()]
items = list(b.GetTracks())
LAY = {"F": pcbnew.F_Cu, "B": pcbnew.B_Cu}
def conflicts(pos, radius, layer, allvia=False):
    out = []
    for p in pads:
        if p.GetNetCode() == nc: continue
        if (p.IsOnLayer(layer) or p.GetAttribute() == pcbnew.PAD_ATTRIB_PTH or allvia) and p.HitTest(pos, int(radius)):
            out.append("pad %s.%s [%s]" % (p.GetParentFootprint().GetReference(), p.GetNumber(), p.GetNetname()))
    for t in items:
        if t.GetNetCode() == nc: continue
        if t.GetClass() == "PCB_VIA":
            if t.HitTest(pos, int(radius)): out.append("via [%s] at (%.2f,%.2f)" % (t.GetNetname(), t.GetPosition().x/1e6-ORIGIN[0], t.GetPosition().y/1e6-ORIGIN[1]))
        elif (allvia or t.GetLayer() == layer) and t.HitTest(pos, int(radius)):
            out.append("track [%s] %s" % (t.GetNetname(), pcbnew.LayerName(t.GetLayer())))
    return out
polys = []
for s in specs:
    L, pts = s.split(":")
    polys.append((LAY[L], [tuple(float(v) for v in q.split(",")) for q in pts.split(";")]))
bad = 0
for layer, pts in polys:
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        n = max(2, int(math.hypot(x2 - x1, y2 - y1) / 0.1))
        seen = set()
        for k in range(n + 1):
            x, y = x1 + (x2 - x1) * k / n, y1 + (y2 - y1) * k / n
            for c in conflicts(P(x, y), MM(W / 2 + 0.2), layer):
                if c not in seen:
                    seen.add(c); bad += 1
                    print("CONFLICT %s seg (%.2f,%.2f)-(%.2f,%.2f): %s" % (pcbnew.LayerName(layer), x1, y1, x2, y2, c))
for (la, pa), (lb, pb) in zip(polys, polys[1:]):
    assert pa[-1] == pb[0], "polylines must share the via point"
    for c in conflicts(P(*pa[-1]), MM(0.3 + 0.2), pcbnew.F_Cu, allvia=True):
        bad += 1; print("CONFLICT via at", pa[-1], ":", c)
print("conflicts:", bad)
if ADD and bad == 0:
    for layer, pts in polys:
        for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
            t = pcbnew.PCB_TRACK(b); t.SetStart(P(x1, y1)); t.SetEnd(P(x2, y2)); t.SetWidth(MM(W)); t.SetLayer(layer); t.SetNet(net); b.Add(t)
    for (la, pa), (lb, pb) in zip(polys, polys[1:]):
        v = pcbnew.PCB_VIA(b); v.SetPosition(P(*pa[-1])); v.SetWidth(MM(0.6)); v.SetDrill(MM(0.3)); v.SetNet(net); v.SetIsFree(False); b.Add(v)
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    pcbnew.SaveBoard(PCB, b); print("added")
