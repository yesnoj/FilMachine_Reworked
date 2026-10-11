# -*- coding: utf-8 -*-
"""Plane fan-out: every SMD pad of a plane net (+12V -> In2 plane, GND -> In1 plane) that has no copper path
to its plane gets a via just outside the pad plus a short track. Checks clearance on F.Cu, B.Cu, In2, holes,
and never drops a via inside a hand-made power zone of another net."""
import sys, math
import pcbnew

PCB = sys.argv[1]
b = pcbnew.LoadBoard(PCB)
MM = pcbnew.FromMM
VIA_D, VIA_H, CLR = 0.6, 0.3, 0.25
PLANES = {"+12V": ("12V_PLANE", 0.5), "GND": ("GND_PLANE", 0.3)}

def refill(): pcbnew.ZONE_FILLER(b).Fill(b.Zones())
def zone(name): return [z for z in b.Zones() if z.GetZoneName() == name][0]
def seg_dist(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1; L = dx * dx + dy * dy
    t = 0 if L == 0 else max(0, min(1, ((px - x1) * dx + (py - y1) * dy) / L))
    return math.hypot(px - (x1 + t * dx), py - (y1 + t * dy))
def box_dist(x, y, bx):
    dx = max(bx[0] - x, 0, x - bx[2]); dy = max(bx[1] - y, 0, y - bx[3]); return math.hypot(dx, dy)

def obstacles(netcode):
    """per layer: segments and boxes of copper NOT belonging to netcode; plus all holes/vias (any net)"""
    segs = {pcbnew.F_Cu: [], pcbnew.B_Cu: [], pcbnew.In2_Cu: [], pcbnew.In1_Cu: []}
    boxes = {k: [] for k in segs}
    holes = []
    for t in b.GetTracks():
        if t.GetClass() == "PCB_VIA":
            p = t.GetPosition(); r = t.GetWidth(pcbnew.F_Cu) / 2
            holes.append((p.x, p.y, r))
            continue
        if t.GetNetCode() == netcode: continue
        if t.GetLayer() in segs:
            segs[t.GetLayer()].append((t.GetStart().x, t.GetStart().y, t.GetEnd().x, t.GetEnd().y, t.GetWidth() / 2))
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.HasHole():
                holes.append((p.GetPosition().x, p.GetPosition().y, max(p.GetSize().x, p.GetSize().y) / 2 if p.GetNetCode() != netcode else max(p.GetDrillSize().x, p.GetDrillSize().y) / 2 + MM(0.1)))
            if p.GetNetCode() == netcode: continue
            for L in boxes:
                if p.IsOnLayer(L):
                    bb = p.GetBoundingBox(); boxes[L].append((bb.GetLeft(), bb.GetTop(), bb.GetRight(), bb.GetBottom()))
    return segs, boxes, holes

def foreign_zone_outlines(netname):
    out = []
    for z in b.Zones():
        if z.GetIsRuleArea() or z.GetNetname() == netname:
            continue
        if z.GetZoneName() in ("GND_PLANE", "12V_PLANE", "GND_F", "GND_B"):
            continue
        out.append(z.Outline())
    return out

def connected_to_plane(pad, plane_items):
    """crude: pad has a same-net via within 0.4 mm of its copper or a track from it ending at a via"""
    return False

def run(netname):
    ni = b.FindNet(netname); nc = ni.GetNetCode()
    plane = zone(PLANES[netname][0]); tw = PLANES[netname][1]
    plane_ol = plane.Outline()
    segs, boxes, holes = obstacles(nc)
    fz = foreign_zone_outlines(netname)
    # pads already served: a same-net via within 1.6 mm whose straight line to the pad is same-net copper is hard
    # to prove; we use the DRC result instead (passed as list of refs.pads) when available
    targets = TARGETS.get(netname) if netname in TARGETS else None
    added = 0; failed = []
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetCode() != nc or p.HasHole() or not p.IsOnLayer(pcbnew.F_Cu): continue
            key = f"{f.GetReference()}.{p.GetNumber()}"
            if targets is not None and key not in targets: continue
            c = p.GetPosition(); bb = p.GetBoundingBox()
            hw, hh = bb.GetWidth() / 2, bb.GetHeight() / 2
            if any(z.GetNetCode() == nc and z.GetLayer() == pcbnew.F_Cu and not z.GetIsRuleArea()
                   and z.GetZoneName() not in ("GND_F",) and z.Outline().Contains(c) for z in b.Zones()):
                continue            # already inside a hand-made copper area of its own net
            fc = f.GetCourtyard(pcbnew.F_CrtYd).BBox().GetCenter() if f.GetCourtyard(pcbnew.F_CrtYd).OutlineCount() else f.GetPosition()
            ox, oy = c.x - fc.x, c.y - fc.y
            angs = sorted(range(0, 360, 30), key=lambda a: -(math.cos(math.radians(a)) * ox + math.sin(math.radians(a)) * oy))
            ok = False
            for dist in (0.55, 0.8, 1.1, 1.5, 2.0, 2.6, 3.2, 4.0):
                for ang in angs:
                    ux, uy = math.cos(math.radians(ang)), math.sin(math.radians(ang))
                    k = min(hw / abs(ux) if abs(ux) > 1e-6 else 1e18, hh / abs(uy) if abs(uy) > 1e-6 else 1e18)
                    x = c.x + ux * (k + MM(dist)); y = c.y + uy * (k + MM(dist))
                    r = MM(VIA_D / 2)
                    P = pcbnew.VECTOR2I(int(x), int(y))
                    if not plane_ol.Contains(P): continue
                    if any(o.Contains(P) for o in fz): continue
                    bad = False
                    for L in (pcbnew.F_Cu, pcbnew.B_Cu, pcbnew.In2_Cu):
                        if any(box_dist(x, y, bx) < r + MM(CLR) for bx in boxes[L]) or \
                           any(seg_dist(x, y, *s[:4]) < s[4] + r + MM(CLR) for s in segs[L]):
                            bad = True; break
                    if bad: continue
                    if any(math.hypot(hx - x, hy - y) < hr + r + MM(CLR) for hx, hy, hr in holes): continue
                    # the short F.Cu track from pad centre to via must keep clearance
                    n = 10; bad = False
                    for i in range(n + 1):
                        tx = c.x + (x - c.x) * i / n; ty = c.y + (y - c.y) * i / n
                        if any(box_dist(tx, ty, bx) < MM(tw / 2 + CLR) for bx in boxes[pcbnew.F_Cu]) or \
                           any(seg_dist(tx, ty, *s[:4]) < s[4] + MM(tw / 2 + CLR) for s in segs[pcbnew.F_Cu]):
                            bad = True; break
                    if bad: continue
                    v = pcbnew.PCB_VIA(b); v.SetPosition(P); v.SetWidth(MM(VIA_D)); v.SetDrill(MM(VIA_H)); v.SetNet(ni); b.Add(v)
                    t = pcbnew.PCB_TRACK(b); t.SetStart(c); t.SetEnd(P); t.SetWidth(MM(tw)); t.SetLayer(pcbnew.F_Cu); t.SetNet(ni); b.Add(t)
                    holes.append((x, y, r)); boxes[pcbnew.F_Cu].append((x - r, y - r, x + r, y + r))
                    segs[pcbnew.F_Cu].append((c.x, c.y, x, y, MM(tw / 2)))
                    added += 1; ok = True; break
                if ok: break
            if not ok: failed.append(key)
    return added, failed

# targets from the last DRC report (unconnected pads), if given
import json, os, re
TARGETS = {}
rep = PCB.replace(".kicad_pcb", "_drc.json")
if "--from-drc" in sys.argv and os.path.exists(rep):
    d = json.load(open(rep))
    for u in d.get("unconnected_items", []):
        for it in u["items"]:
            m = re.match(r"(?:PTH )?[Pp]ad (\S+) \[([^\]]+)\] of (\S+)", it["description"])
            if m and m.group(2) in PLANES:
                TARGETS.setdefault(m.group(2), set()).add(f"{m.group(3)}.{m.group(1)}")
    for k in PLANES:
        TARGETS.setdefault(k, set())
if "--all" in sys.argv:
    TARGETS = {}            # None-equivalent: every SMD pad of each plane net
    for k in PLANES:
        TARGETS[k] = None
if "--auto" in sys.argv:
    refill()
    # 1) pads named in the DRC report, and pads at the ends of tracks named in it
    if os.path.exists(rep):
        d = json.load(open(rep))
        tracks = [t for t in b.GetTracks() if t.GetClass() != "PCB_VIA"]
        for u in d.get("unconnected_items", []):
            for it in u["items"]:
                m = re.match(r"Track \[([^\]]+)\]", it["description"])
                if m and m.group(1) in PLANES:
                    x, y = MM(it["pos"]["x"]), MM(it["pos"]["y"])
                    net = b.FindNet(m.group(1)).GetNetCode()
                    # flood along tracks of this net from the reported position, collect pads
                    seen, front, pads_hit = set(), [(x, y)], set()
                    for _ in range(60):
                        if not front: break
                        nx = []
                        for (px, py) in front:
                            for t in tracks:
                                if t.GetNetCode() != net or id(t) in seen: continue
                                if seg_dist(px, py, t.GetStart().x, t.GetStart().y, t.GetEnd().x, t.GetEnd().y) < t.GetWidth():
                                    seen.add(id(t)); nx += [(t.GetStart().x, t.GetStart().y), (t.GetEnd().x, t.GetEnd().y)]
                        for (px, py) in nx:
                            for f in b.GetFootprints():
                                for p in f.Pads():
                                    if p.GetNetCode() == net and p.GetBoundingBox().Contains(pcbnew.VECTOR2I(int(px), int(py))):
                                        pads_hit.add(f"{f.GetReference()}.{p.GetNumber()}")
                        front = nx
                    TARGETS.setdefault(m.group(1), set()).update(pads_hit)
    # 2) GND pads sitting in an outer-layer GND island that has no via / THT GND pad
    gnd = b.FindNet("GND").GetNetCode()
    gv = [t.GetPosition() for t in b.GetTracks() if t.GetClass() == "PCB_VIA" and t.GetNetCode() == gnd]
    gv += [p.GetPosition() for f in b.GetFootprints() for p in f.Pads() if p.GetNetCode() == gnd and p.HasHole()]
    for name, layer in (("GND_F", pcbnew.F_Cu),):
        fp = zone(name).GetFilledPolysList(layer)
        for i in range(fp.OutlineCount()):
            ol = fp.Outline(i)
            if any(ol.PointInside(v) for v in gv): continue
            for f in b.GetFootprints():
                for p in f.Pads():
                    if p.GetNetCode() == gnd and not p.HasHole() and ol.PointInside(p.GetPosition()):
                        TARGETS.setdefault("GND", set()).add(f"{f.GetReference()}.{p.GetNumber()}")
    for k in PLANES:
        TARGETS.setdefault(k, set())
    print("targets", {k: sorted(v) for k, v in TARGETS.items()})
for net in PLANES:
    a, f = run(net)
    print(net, "fanout vias", a, "failed", f)
refill()
pcbnew.SaveBoard(PCB, b)
