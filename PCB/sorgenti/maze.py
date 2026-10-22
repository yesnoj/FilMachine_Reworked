# -*- coding: utf-8 -*-
"""Small grid router (A*) to finish the last connections the autorouter left open.
Usage: python3 maze.py board.kicad_pcb  (reads <board>_drc.json; routes every unconnected pair of
non-zone items; for plane nets (+12V, GND) a pad may instead be tied to its plane with a via).
Rasterises all foreign copper per layer at 0.05 mm, inflated by clearance + half track width."""
import sys, json, math, heapq, re
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
import pcbnew

PCB = sys.argv[1]
b = pcbnew.LoadBoard(PCB)
RES = 0.05            # mm per cell
import os
CLR = float(os.environ.get("MAZE_CLR", "0.30"))  # clearance used by the maze (DRC 0.2) + grid-snapping margin
TW = {"Default": 0.25, "Power": 0.6, "Power1": 1.0}
VIA_D, VIA_H = float(os.environ.get("MAZE_VIA", "0.6")), 0.3   # 0.5 is allowed by the rules (annular 0.1) for tight spots
PLANE_OF = {"+12V": "12V_PLANE", "GND": "GND_PLANE"}
LAYERS = [pcbnew.F_Cu, pcbnew.B_Cu]

bb = b.GetBoardEdgesBoundingBox()
X0, Y0 = bb.GetLeft() / 1e6, bb.GetTop() / 1e6
W = int(math.ceil(bb.GetWidth() / 1e6 / RES)) + 1
H = int(math.ceil(bb.GetHeight() / 1e6 / RES)) + 1
def to_c(xn, yn): return ((xn / 1e6 - X0) / RES, (yn / 1e6 - Y0) / RES)
def to_nm(cx, cy): return (int((X0 + cx * RES) * 1e6), int((Y0 + cy * RES) * 1e6))

def refill(): pcbnew.ZONE_FILLER(b).Fill(b.Zones())

# HC-class nets (mkpro.py) need 0.3 mm: their copper is inflated by the difference when CLR is smaller
import re as _re
HC_NETS = set(_re.findall(r'"([^"]+)"', _re.search(r"^HC = \[(.*)\]", open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "mkpro.py")).read(), _re.M).group(1)))
HC_CLR = 0.30 + 0.01

def raster(netcode, layer, extra_mm):
    """bool mask of copper on `layer` NOT belonging to netcode (GND pours count as obstacles too)."""
    hc = raster_one(netcode, layer, extra_mm + max(0.0, HC_CLR - CLR), only_hc=True) if CLR < HC_CLR else None
    m = raster_one(netcode, layer, extra_mm, only_hc=False)
    return m | hc if hc is not None else m

def raster_one(netcode, layer, extra_mm, only_hc):
    im = Image.new("1", (W, H), 0); d = ImageDraw.Draw(im)
    def poly(chain):
        pts = [to_c(chain.CPoint(i).x, chain.CPoint(i).y) for i in range(chain.PointCount())]
        if len(pts) > 2: d.polygon(pts, fill=1)
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.GetNetCode() == netcode or not p.IsOnLayer(layer): continue
            if only_hc and p.GetNetname() not in HC_NETS: continue
            sp = p.GetEffectivePolygon(layer, pcbnew.ERROR_INSIDE)
            for i in range(sp.OutlineCount()): poly(sp.Outline(i))
    for t in b.GetTracks():
        if t.GetNetCode() == netcode: continue
        if only_hc and t.GetNetname() not in HC_NETS: continue
        if t.GetClass() == "PCB_VIA":
            c = to_c(t.GetPosition().x, t.GetPosition().y); r = t.GetWidth(layer) / 2e6 / RES
            d.ellipse([c[0] - r, c[1] - r, c[0] + r, c[1] + r], fill=1)
        elif t.GetLayer() == layer:
            a = to_c(t.GetStart().x, t.GetStart().y); e = to_c(t.GetEnd().x, t.GetEnd().y)
            w = t.GetWidth() / 1e6 / RES
            d.line([a, e], fill=1, width=max(1, int(round(w))))
            for c in (a, e): d.ellipse([c[0] - w / 2, c[1] - w / 2, c[0] + w / 2, c[1] + w / 2], fill=1)
    for z in b.Zones():
        if z.GetIsRuleArea() or z.GetNetCode() == netcode or not z.IsOnLayer(layer): continue
        if only_hc and z.GetNetname() not in HC_NETS: continue
        if z.GetZoneName() in ("GND_F", "GND_B"):   # the pour reflows around new copper
            continue
        fp = z.GetFilledPolysList(layer)
        for i in range(fp.OutlineCount()): poly(fp.Outline(i))
    m = np.array(im, dtype=bool)
    r = int(math.ceil(extra_mm / RES))
    if r > 0:
        yy, xx = np.ogrid[-r:r + 1, -r:r + 1]
        m = ndimage.binary_dilation(m, structure=(xx * xx + yy * yy) <= r * r)
    # board edge
    e = int(math.ceil((0.5 + extra_mm) / RES))
    m[:e, :] = m[-e:, :] = True; m[:, :e] = m[:, -e:] = True
    return m

def via_forbidden(netcode):
    """cells where a via of netcode may NOT be centred"""
    im = Image.new("1", (W, H), 0); d = ImageDraw.Draw(im)
    rr = (VIA_H / 2 + 0.3) / RES
    for t in b.GetTracks():
        if t.GetClass() == "PCB_VIA":
            c = to_c(t.GetPosition().x, t.GetPosition().y); r = rr + t.GetDrillValue() / 2e6 / RES
            d.ellipse([c[0] - r, c[1] - r, c[0] + r, c[1] + r], fill=1)
    for f in b.GetFootprints():
        for p in f.Pads():
            if p.HasHole():
                c = to_c(p.GetPosition().x, p.GetPosition().y); r = rr + max(p.GetDrillSize().x, p.GetDrillSize().y) / 2e6 / RES
                d.ellipse([c[0] - r, c[1] - r, c[0] + r, c[1] + r], fill=1)
    for z in b.Zones():   # hand-made power copper and other-net inner copper
        if z.GetIsRuleArea() or z.GetNetCode() == netcode: continue
        if z.GetZoneName() in ("GND_F", "GND_B", "GND_PLANE", "12V_PLANE"): continue
        ol = z.Outline()
        for i in range(ol.OutlineCount()):
            ch = ol.Outline(i)
            pts = [to_c(ch.CPoint(k).x, ch.CPoint(k).y) for k in range(ch.PointCount())]
            d.polygon(pts, fill=1)
    m = np.array(im, dtype=bool)
    # inner-layer copper of other nets that is not a plane (e.g. bus on In2)
    return m

def plane_mask(netname):
    zn = PLANE_OF.get(netname)
    if not zn: return None
    z = [z for z in b.Zones() if z.GetZoneName() == zn][0]
    im = Image.new("1", (W, H), 0); d = ImageDraw.Draw(im)
    fp = z.GetFilledPolysList(z.GetLayer())
    for i in range(fp.OutlineCount()):
        ch = fp.Outline(i)
        d.polygon([to_c(ch.CPoint(k).x, ch.CPoint(k).y) for k in range(ch.PointCount())], fill=1)
    m = np.array(im, dtype=bool)
    r = int(math.ceil((VIA_D / 2 + 0.05) / RES)); yy, xx = np.ogrid[-r:r + 1, -r:r + 1]
    return ndimage.binary_erosion(m, structure=(xx * xx + yy * yy) <= r * r)

def astar(starts, goal_fn, blocked, viaok, via_cost=60, target=None):
    """starts: list of (layer, cx, cy). blocked[layer] bool masks. returns path list of (layer,x,y)"""
    INF = 1 << 60
    def hfun(s):
        return 0 if target is None else math.hypot(s[1] - target[0], s[2] - target[1])
    dist = {}
    pq = []
    for s in starts:
        dist[s] = 0; heapq.heappush(pq, (0, 0, s, None))
    prev = {}
    nb = [(1, 0, 1.0), (-1, 0, 1.0), (0, 1, 1.0), (0, -1, 1.0), (1, 1, 1.414), (1, -1, 1.414), (-1, 1, 1.414), (-1, -1, 1.414)]
    n = 0
    while pq:
        f_, g, s, pr = heapq.heappop(pq)
        if dist.get(s, INF) < g or s in prev: continue
        prev[s] = pr
        n += 1
        if n > int(os.environ.get("MAZE_MAXN", "6000000")): return None
        if goal_fn(s):
            path = [s]
            while prev[path[-1]] is not None: path.append(prev[path[-1]])
            return path[::-1]
        L, x, y = s
        for dx, dy, c in nb:
            nx, ny = x + dx, y + dy
            if 0 <= nx < W and 0 <= ny < H and not blocked[L][ny, nx]:
                t = (L, nx, ny); ng = g + c
                if ng < dist.get(t, INF):
                    dist[t] = ng; heapq.heappush(pq, (ng + hfun(t), ng, t, s))
        if viaok[y, x]:
            t = (1 - L, x, y); ng = g + via_cost
            if not blocked[1 - L][y, x] and ng < dist.get(t, INF):
                dist[t] = ng; heapq.heappush(pq, (ng + hfun(t), ng, t, s))
    return None

def emit(path, net, tw, final_via=False):
    ni = b.FindNet(net)
    # compress straight runs
    segs = []
    cur = [path[0]]
    for p in path[1:]:
        if p[0] != cur[-1][0]:
            segs.append(cur); segs.append(("VIA", p[1], p[2])); cur = [p]
        else:
            cur.append(p)
    segs.append(cur)
    nv = 0
    for sg in segs:
        if isinstance(sg, tuple):
            v = pcbnew.PCB_VIA(b); x, y = to_nm(sg[1], sg[2]); v.SetPosition(pcbnew.VECTOR2I(x, y))
            v.SetWidth(pcbnew.FromMM(VIA_D)); v.SetDrill(pcbnew.FromMM(VIA_H)); v.SetNet(ni); b.Add(v); nv += 1
            continue
        if len(sg) < 2: continue
        L = LAYERS[sg[0][0]]
        pts = [sg[0]]
        for i in range(1, len(sg) - 1):
            d1 = (sg[i][1] - sg[i - 1][1], sg[i][2] - sg[i - 1][2]); d2 = (sg[i + 1][1] - sg[i][1], sg[i + 1][2] - sg[i][2])
            if d1 != d2: pts.append(sg[i])
        pts.append(sg[-1])
        for a, e in zip(pts, pts[1:]):
            t = pcbnew.PCB_TRACK(b); t.SetLayer(L); t.SetNet(ni); t.SetWidth(pcbnew.FromMM(tw))
            t.SetStart(pcbnew.VECTOR2I(*to_nm(a[1], a[2]))); t.SetEnd(pcbnew.VECTOR2I(*to_nm(e[1], e[2]))); b.Add(t)
    if final_via:
        L, x, y = path[-1]
        v = pcbnew.PCB_VIA(b); X, Y = to_nm(x, y); v.SetPosition(pcbnew.VECTOR2I(X, Y))
        v.SetWidth(pcbnew.FromMM(VIA_D)); v.SetDrill(pcbnew.FromMM(VIA_H)); v.SetNet(ni); b.Add(v); nv += 1
    return nv

def item_cells(desc, pos, net):
    """starting cells for a DRC item: pads -> their copper, tracks/vias -> their position"""
    m = re.match(r"(?:PTH )?[Pp]ad (\S+) \[[^\]]+\] of (\S+)", desc)
    cells = []
    if m:
        fp = b.FindFootprintByReference(m.group(2))
        pad = [p for p in fp.Pads() if p.GetNumber() == m.group(1)][0]
        c = to_c(pad.GetPosition().x, pad.GetPosition().y)
        for li, L in enumerate(LAYERS):
            if pad.IsOnLayer(L): cells.append((li, int(round(c[0])), int(round(c[1]))))
        return cells, pad
    c = to_c(pcbnew.FromMM(pos["x"]), pcbnew.FromMM(pos["y"]))
    if desc.startswith("Via"):
        return [(0, int(round(c[0])), int(round(c[1]))), (1, int(round(c[0])), int(round(c[1])))], None
    lay = 0 if "F.Cu" in desc else 1
    return [(lay, int(round(c[0])), int(round(c[1])))], None

def netclass_width(net):
    nc = b.FindNet(net).GetNetClassName()
    return TW.get(str(nc).split(",")[0], 0.25)

def tie_to_plane(ref, num):
    fp = b.FindFootprintByReference(ref)
    pad = [p for p in fp.Pads() if p.GetNumber() == num][0]
    net = pad.GetNetname(); nc = pad.GetNetCode(); tw = netclass_width(net)
    blocked = [raster(nc, L, CLR + tw / 2) for L in LAYERS]
    vf = via_forbidden(nc)
    viaok = ~vf & ~raster(nc, LAYERS[0], CLR + VIA_D / 2) & ~raster(nc, LAYERS[1], CLR + VIA_D / 2)
    okv = viaok & plane_mask(net)
    # existing vias of the same net also count as arrival points (they reach the plane)
    sv = np.zeros_like(okv)
    for t in b.GetTracks():
        if t.GetClass() == "PCB_VIA" and t.GetNetCode() == nc:
            cx, cy = to_c(t.GetPosition().x, t.GetPosition().y)
            r = 3; sv[max(0, int(cy) - r):int(cy) + r + 1, max(0, int(cx) - r):int(cx) + r + 1] = True
    for L in (0, 1):
        blocked[L] &= ~sv
    c = to_c(pad.GetPosition().x, pad.GetPosition().y)
    A = [(li, int(round(c[0])), int(round(c[1]))) for li, L in enumerate(LAYERS) if pad.IsOnLayer(L)]
    for s in A:
        r = int((tw / 2 + CLR) / RES) + 2
        blocked[s[0]][max(0, s[2] - r):s[2] + r, max(0, s[1] - r):s[1] + r] = False
    bbp = pad.GetBoundingBox()
    hw = max(bbp.GetWidth(), bbp.GetHeight()) / 2e6 / RES
    far = lambda s: math.hypot(s[1] - A[0][1], s[2] - A[0][2]) > hw + (VIA_D / 2 + 0.1) / RES
    path = astar(A, lambda s: sv[s[2], s[1]] or (okv[s[2], s[1]] and far(s)), blocked, np.zeros_like(viaok))
    if path is None:
        return False
    L, x, y = path[-1]
    emit(path, net, tw, final_via=not sv[y, x])
    refill()
    return True

def gnd_island_pads():
    refill()
    gnd = b.FindNet("GND").GetNetCode()
    gv = [t.GetPosition() for t in b.GetTracks() if t.GetClass() == "PCB_VIA" and t.GetNetCode() == gnd]
    gv += [p.GetPosition() for f in b.GetFootprints() for p in f.Pads() if p.GetNetCode() == gnd and p.HasHole()]
    out = []
    z = [z for z in b.Zones() if z.GetZoneName() == "GND_F"][0]
    fp = z.GetFilledPolysList(pcbnew.F_Cu)
    for i in range(fp.OutlineCount()):
        ol = fp.Outline(i)
        if any(ol.PointInside(v) for v in gv): continue
        for f in b.GetFootprints():
            for p in f.Pads():
                if p.GetNetCode() == gnd and not p.HasHole() and ol.PointInside(p.GetPosition()):
                    out.append((f.GetReference(), p.GetNumber())); break
            else:
                continue
            break
    return out

if "--tie" in sys.argv:
    refill()
    todo = gnd_island_pads()
    extra = sys.argv[sys.argv.index("--tie") + 1] if len(sys.argv) > sys.argv.index("--tie") + 1 else ""
    todo += [tuple(x.split(".")) for x in extra.split(",") if "." in x]
    # plane-net pads named in the DRC report that are still loose
    rep = PCB.replace(".kicad_pcb", "_drc.json")
    for u in json.load(open(rep)).get("unconnected_items", []):
        for it in u["items"]:
            m = re.match(r"(?:PTH )?[Pp]ad (\S+) \[([^\]]+)\] of (\S+)", it["description"])
            if m and m.group(2) in PLANE_OF and (m.group(3), m.group(1)) not in todo:
                todo.append((m.group(3), m.group(1)))
    ok = []; ko = []
    for ref, num in todo:
        (ok if tie_to_plane(ref, num) else ko).append(f"{ref}.{num}")
    pcbnew.SaveBoard(PCB, b)
    print("tied", ok, "failed", ko)
    sys.exit(0)

rep = PCB.replace(".kicad_pcb", "_drc.json")
d = json.load(open(rep))
done = 0; failed = []
refill()
for u in d.get("unconnected_items", []):
    its = u["items"]
    m = re.search(r"\[([^\]]+)\]", its[0]["description"])
    if not m: continue
    net = m.group(1)
    if any(i["description"].startswith("Zone") for i in its):
        continue
    nc = b.FindNet(net).GetNetCode()
    tw = netclass_width(net)
    blocked = [raster(nc, L, CLR + tw / 2) for L in LAYERS]
    vf = via_forbidden(nc)
    viaok = ~vf & ~raster(nc, LAYERS[0], CLR + VIA_D / 2) & ~raster(nc, LAYERS[1], CLR + VIA_D / 2)
    A, padA = item_cells(its[0]["description"], its[0]["pos"], net)
    B, padB = item_cells(its[1]["description"], its[1]["pos"], net)
    for s in A + B:   # free the endpoints themselves
        r = int((tw / 2 + CLR) / RES) + 2
        blocked[s[0]][max(0, s[2] - r):s[2] + r, max(0, s[1] - r):s[1] + r] = False
    goal = set(B)
    def near_goal(s):
        return any(s[0] == g[0] and abs(s[1] - g[1]) <= 3 and abs(s[2] - g[2]) <= 3 for g in goal)
    path = astar(A, near_goal, blocked, viaok, target=(B[0][1], B[0][2]))
    if path is None and net in PLANE_OF:
        pm = plane_mask(net)
        okv = viaok & pm
        path = astar(A, lambda s: okv[s[2], s[1]] and math.hypot(s[1] - A[0][1], s[2] - A[0][2]) > 12, blocked, viaok)
        if path is not None:
            emit(path, net, tw, final_via=True); done += 1; print("plane-tied", net, its[0]["description"][:40]); refill(); continue
    if path is None:
        failed.append((net, its[0]["description"][:40], its[1]["description"][:40])); continue
    emit(path, net, tw); done += 1
    print("routed", net, its[0]["description"][:40], "->", its[1]["description"][:40], "len", len(path))
    refill()
pcbnew.SaveBoard(PCB, b)
print("maze routed", done, "failed", failed)
