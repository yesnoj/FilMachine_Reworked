# -*- coding: utf-8 -*-
"""Post-route GND stitching: tie every GND pour island on F/B to the In1 plane, then add a stitching grid.
A via is placed only where both outer layers are GND fill (with margin) and no In2 copper of another net is near."""
import sys, math
import pcbnew

PCB = sys.argv[1]
b = pcbnew.LoadBoard(PCB)
MM = pcbnew.FromMM
VIA_D, VIA_H = 0.6, 0.3
CLR = 0.25
gnd = b.FindNet("GND")

def refill():
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())

def zone(name):
    return [z for z in b.Zones() if z.GetZoneName() == name][0]

def in2_obstacles():
    segs, vias = [], []
    for t in b.GetTracks():
        if t.GetClass() == "PCB_VIA":
            vias.append((t.GetPosition().x, t.GetPosition().y, t.GetWidth(pcbnew.F_Cu) / 2))
            continue
        if t.GetNetCode() == gnd.GetNetCode():
            continue
        if t.GetLayer() == pcbnew.In2_Cu:
            segs.append((t.GetStart().x, t.GetStart().y, t.GetEnd().x, t.GetEnd().y, t.GetWidth() / 2))
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.HasHole():
                vias.append((p.GetPosition().x, p.GetPosition().y, max(p.GetDrillSize().x, p.GetDrillSize().y) / 2 + MM(0.15)))
    return segs, vias

def seg_dist(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    L = dx * dx + dy * dy
    t = 0 if L == 0 else max(0, min(1, ((px - x1) * dx + (py - y1) * dy) / L))
    return math.hypot(px - (x1 + t * dx), py - (y1 + t * dy))

def hc_outlines():
    out = []
    for z in b.Zones():
        if z.GetIsRuleArea() or z.GetNetname() == "GND" or z.GetZoneName() == "12V_PLANE":
            continue
        out.append(z.Outline())
    return out

def fill_contains(poly, x, y, r):
    for a in range(0, 360, 45):
        p = pcbnew.VECTOR2I(int(x + r * math.cos(math.radians(a))), int(y + r * math.sin(math.radians(a))))
        if not poly.Contains(p):
            return False
    return poly.Contains(pcbnew.VECTOR2I(int(x), int(y)))

def candidate_ok(x, y, fF, fB, segs, vias, hcs, placed):
    r = MM(VIA_D / 2 + 0.03)
    if not fill_contains(fF, x, y, r) or not fill_contains(fB, x, y, r):
        return False
    for (x1, y1, x2, y2, w) in segs:
        if seg_dist(x, y, x1, y1, x2, y2) < w + MM(VIA_D / 2 + CLR):
            return False
    for (vx, vy, vr) in vias:
        if math.hypot(vx - x, vy - y) < vr + MM(VIA_D / 2 + CLR):
            return False
    for (vx, vy) in placed:
        if math.hypot(vx - x, vy - y) < MM(VIA_D + CLR):
            return False
    for o in hcs:
        if o.Contains(pcbnew.VECTOR2I(int(x), int(y))):
            return False
    return True

def add_via(x, y):
    v = pcbnew.PCB_VIA(b)
    v.SetPosition(pcbnew.VECTOR2I(int(x), int(y))); v.SetWidth(MM(VIA_D)); v.SetDrill(MM(VIA_H)); v.SetNet(gnd)
    b.Add(v)

def stitch_islands():
    """one via (or two) per GND_F / GND_B island that has no via yet"""
    added = 0
    segs, vias = in2_obstacles()
    hcs = hc_outlines()
    fF = zone("GND_F").GetFilledPolysList(pcbnew.F_Cu)
    fB = zone("GND_B").GetFilledPolysList(pcbnew.B_Cu)
    gvias = [(t.GetPosition().x, t.GetPosition().y) for t in b.GetTracks() if t.GetClass() == "PCB_VIA" and t.GetNetCode() == gnd.GetNetCode()]
    placed = []
    for layer_poly, other in ((fF, fB), (fB, fF)):
        for i in range(layer_poly.OutlineCount()):
            ol = layer_poly.Outline(i)
            # island already has a GND via?
            if any(ol.PointInside(pcbnew.VECTOR2I(int(x), int(y))) for x, y in gvias + placed):
                continue
            bb = ol.BBox()
            step = MM(0.25)
            done = 0
            y = bb.GetTop() + step
            while y < bb.GetBottom() and done < 2:
                x = bb.GetLeft() + step
                while x < bb.GetRight() and done < 2:
                    if ol.PointInside(pcbnew.VECTOR2I(int(x), int(y))) and candidate_ok(x, y, fF, fB, segs, vias, hcs, placed):
                        add_via(x, y); placed.append((x, y)); done += 1; added += 1
                        x += MM(3)
                    x += step
                y += step
    return added

def stitch_grid(pitch_mm=4.0):
    segs, vias = in2_obstacles()
    hcs = hc_outlines()
    fF = zone("GND_F").GetFilledPolysList(pcbnew.F_Cu)
    fB = zone("GND_B").GetFilledPolysList(pcbnew.B_Cu)
    allv = [(t.GetPosition().x, t.GetPosition().y) for t in b.GetTracks() if t.GetClass() == "PCB_VIA"]
    added = 0
    bb = b.GetBoardEdgesBoundingBox()
    y = bb.GetTop() + MM(2)
    while y < bb.GetBottom() - MM(2):
        x = bb.GetLeft() + MM(2)
        while x < bb.GetRight() - MM(2):
            near = any(math.hypot(vx - x, vy - y) < MM(pitch_mm * 0.6) for vx, vy in allv)
            if not near and candidate_ok(x, y, fF, fB, segs, vias, hcs, []):
                add_via(x, y); allv.append((x, y)); added += 1
            x += MM(pitch_mm)
        y += MM(pitch_mm)
    return added

def f_obstacles():
    """foreign copper on F.Cu: (segments) and (pad bboxes)"""
    segs, boxes = [], []
    for t in b.GetTracks():
        if t.GetNetCode() == gnd.GetNetCode():
            continue
        if t.GetClass() == "PCB_VIA":
            p = t.GetPosition(); r = t.GetWidth(pcbnew.F_Cu) / 2
            boxes.append((p.x - r, p.y - r, p.x + r, p.y + r))
        elif t.GetLayer() == pcbnew.F_Cu:
            segs.append((t.GetStart().x, t.GetStart().y, t.GetEnd().x, t.GetEnd().y, t.GetWidth() / 2))
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetCode() == gnd.GetNetCode() or not p.IsOnLayer(pcbnew.F_Cu):
                continue
            bb = p.GetBoundingBox()
            boxes.append((bb.GetLeft(), bb.GetTop(), bb.GetRight(), bb.GetBottom()))
    return segs, boxes

def box_dist(x, y, bx):
    dx = max(bx[0] - x, 0, x - bx[2]); dy = max(bx[1] - y, 0, y - bx[3])
    return math.hypot(dx, dy)

def seg_seg_clear(a, segs, boxes, w, clr):
    # sample along the segment
    (x1, y1, x2, y2) = a
    n = 8
    for i in range(n + 1):
        x = x1 + (x2 - x1) * i / n; y = y1 + (y2 - y1) * i / n
        for (sx1, sy1, sx2, sy2, sw) in segs:
            if seg_dist(x, y, sx1, sy1, sx2, sy2) < sw + w + clr: return False
        for bx in boxes:
            if box_dist(x, y, bx) < w + clr: return False
    return True

def fanout_pads():
    """GND SMD pads whose F.Cu fill island has no GND via: add a via + short track."""
    refill()
    fF = zone("GND_F").GetFilledPolysList(pcbnew.F_Cu)
    fB = zone("GND_B").GetFilledPolysList(pcbnew.B_Cu)
    gv = [t.GetPosition() for t in b.GetTracks() if t.GetClass() == "PCB_VIA" and t.GetNetCode() == gnd.GetNetCode()]
    segs2, vias2 = in2_obstacles()
    fsegs, fboxes = f_obstacles()
    hcs = hc_outlines()
    added = 0
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetCode() != gnd.GetNetCode() or p.GetAttribute() != pcbnew.PAD_ATTRIB_SMD or not p.IsOnLayer(pcbnew.F_Cu):
                continue
            c = p.GetPosition()
            isl = [i for i in range(fF.OutlineCount()) if fF.Outline(i).PointInside(c)]
            if isl and any(fF.Outline(isl[0]).PointInside(v) for v in gv):
                continue
            if not isl:
                # pad not touching the pour at all: check if a via is already within 1.2 mm
                if any(math.hypot(v.x - c.x, v.y - c.y) < MM(1.2) for v in gv):
                    continue
            bb = p.GetBoundingBox()
            hw, hh = bb.GetWidth() / 2, bb.GetHeight() / 2
            done = False
            for dist in (0.75, 1.0, 1.3, 1.6, 2.0):
                for ang in range(0, 360, 30):
                    ux, uy = math.cos(math.radians(ang)), math.sin(math.radians(ang))
                    # point just outside the pad bbox along the direction
                    k = min(hw / abs(ux) if abs(ux) > 1e-6 else 1e18, hh / abs(uy) if abs(uy) > 1e-6 else 1e18)
                    x = c.x + ux * (k + MM(dist)); y = c.y + uy * (k + MM(dist))
                    r = MM(VIA_D / 2)
                    # via on F.Cu: clear of foreign F copper
                    if any(box_dist(x, y, bx) < r + MM(CLR) for bx in fboxes): continue
                    if any(seg_dist(x, y, *s[:4]) < s[4] + r + MM(CLR) for s in fsegs): continue
                    # B.Cu: inside GND_B fill
                    if not fill_contains(fB, x, y, r + MM(0.03)): continue
                    # In2 and other vias
                    if any(seg_dist(x, y, *s[:4]) < s[4] + r + MM(CLR) for s in segs2): continue
                    if any(math.hypot(vx - x, vy - y) < vr + r + MM(CLR) for vx, vy, vr in vias2): continue
                    if any(o.Contains(pcbnew.VECTOR2I(int(x), int(y))) for o in hcs): continue
                    # track from pad edge to via
                    ex, ey = c.x + ux * k * 0.9, c.y + uy * k * 0.9
                    if not seg_seg_clear((ex, ey, x, y), fsegs, fboxes, MM(0.15), MM(CLR)):
                        continue
                    add_via(x, y)
                    tr = pcbnew.PCB_TRACK(b)
                    tr.SetStart(c); tr.SetEnd(pcbnew.VECTOR2I(int(x), int(y))); tr.SetWidth(MM(0.3))
                    tr.SetLayer(pcbnew.F_Cu); tr.SetNet(gnd); b.Add(tr)
                    gv.append(pcbnew.VECTOR2I(int(x), int(y))); vias2.append((x, y, r))
                    fboxes.append((x - r, y - r, x + r, y + r))
                    added += 1; done = True
                    break
                if done: break
            if not done:
                print("  no fanout for", f.GetReference(), p.GetNumber())
    return added

refill()
print("fanout vias", fanout_pads())
refill()
for it in range(4):
    n = stitch_islands()
    print("island vias", n)
    refill()
    if n == 0:
        break
print("grid vias", stitch_grid())
refill()
pcbnew.SaveBoard(PCB, b)
