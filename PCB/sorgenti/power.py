# -*- coding: utf-8 -*-
"""Add power zones, stitching vias and hand-made high-current tracks to the board (before autorouting)."""
import sys, math
import pcbnew
from placement import ORIGIN, BOARD_W, BOARD_H

OUT = sys.argv[1]
PCB = f"{OUT}/FilMachine_Mainboard.kicad_pcb"
b = pcbnew.LoadBoard(PCB)
MM = pcbnew.FromMM
def P(x, y): return pcbnew.VECTOR2I(MM(ORIGIN[0] + x), MM(ORIGIN[1] + y))
def rect(x0, y0, x1, y1): return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]

ZONES = []   # (net, layer, pts, priority, name)
F, B_, I1, I2 = pcbnew.F_Cu, pcbnew.B_Cu, pcbnew.In1_Cu, pcbnew.In2_Cu

# ── heater block (F.Cu, duplicated on B.Cu) ─────────────────────────────────────────────
Z1 = [(11.6, 2.3), (18.3, 2.3), (18.3, 9.0), (20.0, 9.0), (20.0, 21.4), (11.6, 21.4)]       # +12V_H1
Z2 = [(18.9, 2.3), (26.8, 2.3), (26.8, 23.9), (20.4, 23.9), (20.4, 9.0), (18.9, 9.0)]       # HEAT1_N
Z3 = [(27.4, 2.3), (32.3, 2.3), (32.3, 9.0), (34.6, 9.0), (34.6, 21.4), (27.4, 21.4)]       # +12V_H2
Z4 = [(32.9, 2.3), (42.0, 2.3), (42.0, 23.9), (35.0, 23.9), (35.0, 9.0), (32.9, 9.0)]       # HEAT2_N
for net, pts in (("+12V_H1", Z1), ("HEAT1_N", Z2), ("+12V_H2", Z3), ("HEAT2_N", Z4)):
    ZONES.append((net, F, pts, 10, net + "_F"))
    ZONES.append((net, B_, pts, 10, net + "_B"))
BUS = rect(2.4, 27.0, 34.6, 34.2)                                                          # +12V_IN_H (J1 pin2 -> F1, F2)
ZONES.append(("+12V_IN_H", B_, BUS, 10, "BUS_B"))
ZONES.append(("+12V_IN_H", I2, BUS, 10, "BUS_I2"))
ZONES.append(("+12V_IN_H", F, rect(2.4, 27.0, 11.8, 34.2), 10, "BUS_F"))
Z5 = rect(24.6, 24.3, 34.4, 26.6)                                                          # GND Q9 source
Z6 = rect(39.4, 24.3, 43.2, 33.6)                                                          # GND Q10 source (gate keeps an exit to the left)
ZONES.append(("GND", F, Z5, 10, "GND_Q9"))
ZONES.append(("GND", F, Z6, 10, "GND_Q10"))

# ── main input / pump ──────────────────────────────────────────────────────────────────
Z7 = [(9.2, 35.0), (18.6, 35.0), (18.6, 50.6), (2.4, 50.6), (2.4, 41.8), (9.2, 41.8)]       # +12V_IN_M (J2 pin 2, lower)
for L, nm in ((F, "F"), (B_, "B"), (I2, "I2")):
    ZONES.append(("+12V_IN_M", L, Z7, 10, "IN_M_" + nm))
Z8 = rect(20.6, 35.0, 36.0, 42.6)                                                          # +12V_PUMP (F3 out, D2)
ZONES.append(("+12V_PUMP", F, Z8, 10, "PUMP_TOP_F"))
Z8b = [(20.6, 35.0), (42.2, 35.0), (42.2, 76.0), (36.4, 76.0), (36.4, 69.6), (29.6, 69.6), (29.6, 65.4),
       (36.4, 65.4), (36.4, 54.6), (29.6, 54.6), (29.6, 50.6), (36.4, 50.6), (36.4, 42.6), (20.6, 42.6)]
ZONES.append(("+12V_PUMP", B_, Z8b, 10, "PUMP_B"))
Z9 = rect(20.6, 43.2, 36.0, 50.2)                                                          # +12V (F4 out, D1)
ZONES.append(("+12V", F, Z9, 10, "12V_F"))
Z10 = [(25.9, 50.8), (42.2, 50.8), (42.2, 54.0), (29.4, 54.0), (29.4, 54.35), (25.9, 54.35)]  # VS U8
Z11 = rect(25.9, 61.05, 36.0, 65.0)                                                        # GND U8
Z12 = [(25.9, 65.6), (42.2, 65.6), (42.2, 69.0), (29.4, 69.0), (29.4, 69.35), (25.9, 69.35)]  # VS U9
Z13 = rect(25.9, 76.05, 36.0, 80.6)                                                        # GND U9
ZONES += [("+12V_PUMP", F, Z10, 10, "VS_U8"), ("GND", F, Z11, 10, "GND_U8"),
          ("+12V_PUMP", F, Z12, 10, "VS_U9"), ("GND", F, Z13, 10, "GND_U9")]
Z14 = rect(2.4, 51.4, 24.0, 60.3)                                                          # PUMP_OUT_A
Z15 = [(2.4, 61.0), (12.0, 61.0), (12.0, 63.7), (24.0, 63.7), (24.0, 78.6), (2.4, 78.6)]    # PUMP_OUT_B
for L, nm in ((F, "F"), (B_, "B")):
    ZONES.append(("PUMP_OUT_A", L, Z14, 10, "OUTA_" + nm))
    ZONES.append(("PUMP_OUT_B", L, Z15, 10, "OUTB_" + nm))

# ── inner planes ───────────────────────────────────────────────────────────────────────
ZONES.append(("GND", I1, rect(0.5, 0.5, BOARD_W - 0.5, BOARD_H - 0.5), 1, "GND_PLANE"))
ZI_12V = [(20.6, 43.2), (42.6, 43.2), (42.6, 35.0), (43.8, 35.0), (43.8, 0.8), (BOARD_W - 0.8, 0.8),
          (BOARD_W - 0.8, BOARD_H - 0.8), (17.4, BOARD_H - 0.8), (17.4, 81.0), (42.6, 81.0), (42.6, 50.9), (20.6, 50.9)]
ZONES.append(("+12V", I2, ZI_12V, 5, "12V_PLANE"))

for net, layer, pts, prio, name in ZONES:
    z = pcbnew.ZONE(b)
    z.SetLayer(layer)
    ni = b.FindNet(net)
    assert ni is not None, net
    z.SetNet(ni)
    z.SetAssignedPriority(prio)
    z.SetZoneName(name)
    z.SetLocalClearance(MM(0.3))
    z.SetMinThickness(MM(0.25))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL if net != "GND" or layer != I1 else pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(MM(0.3))
    z.SetThermalReliefSpokeWidth(MM(0.6))
    ol = z.Outline(); ol.NewOutline()
    for x, y in pts:
        ol.Append(MM(ORIGIN[0] + x), MM(ORIGIN[1] + y))
    b.Add(z)

# ── stitching vias ─────────────────────────────────────────────────────────────────────
VIA_D, VIA_H = 0.6, 0.3
pads = [(p, p.GetBoundingBox()) for f in b.GetFootprints() for p in f.Pads()]
placed = []
def inside(pts, x, y):
    c = False
    n = len(pts)
    for i in range(n):
        x1, y1 = pts[i]; x2, y2 = pts[(i + 1) % n]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            c = not c
    return c
def edge_dist(pts, x, y):
    d = 1e9
    n = len(pts)
    for i in range(n):
        x1, y1 = pts[i]; x2, y2 = pts[(i + 1) % n]
        dx, dy = x2 - x1, y2 - y1
        t = max(0, min(1, ((x - x1) * dx + (y - y1) * dy) / (dx * dx + dy * dy)))
        d = min(d, math.hypot(x - (x1 + t * dx), y - (y1 + t * dy)))
    return d
def stitch(net, pts, pitch=1.3, margin=0.75, xmin=-1e9, xmax=1e9, ymin=-1e9, ymax=1e9, clear=0.45, maxn=999):
    ni = b.FindNet(net)
    xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
    n = 0
    y = min(ys) + margin
    while y <= max(ys) - margin + 1e-6:
        x = min(xs) + margin
        while x <= max(xs) - margin + 1e-6:
            ok = (xmin <= x <= xmax and ymin <= y <= ymax and inside(pts, x, y) and edge_dist(pts, x, y) >= margin)
            if ok:
                X, Y = MM(ORIGIN[0] + x), MM(ORIGIN[1] + y)
                r = MM(VIA_D / 2 + clear)
                for p, bb in pads:
                    if bb.GetLeft() - r < X < bb.GetRight() + r and bb.GetTop() - r < Y < bb.GetBottom() + r:
                        ok = False; break
            if ok:
                for (vx, vy) in placed:
                    if math.hypot(vx - x, vy - y) < VIA_D + 0.3:
                        ok = False; break
            if ok and n < maxn:
                v = pcbnew.PCB_VIA(b)
                v.SetPosition(P(x, y)); v.SetWidth(MM(VIA_D)); v.SetDrill(MM(VIA_H)); v.SetNet(ni)
                v.SetIsFree(False)
                b.Add(v); placed.append((x, y)); n += 1
            x += pitch
        y += pitch
    return n

counts = {}
counts["HEAT1_N"] = stitch("HEAT1_N", Z2)
counts["HEAT2_N"] = stitch("HEAT2_N", Z4)
counts["+12V_H1"] = stitch("+12V_H1", Z1)
counts["+12V_H2"] = stitch("+12V_H2", Z3)
counts["GND_Q9"] = stitch("GND", Z5, pitch=1.0, margin=0.6)
counts["GND_Q10"] = stitch("GND", Z6, pitch=1.0, margin=0.6)
counts["IN_M"] = stitch("+12V_IN_M", Z7)
counts["PUMP_TOP"] = stitch("+12V_PUMP", Z8)
counts["12V_F"] = stitch("+12V", Z9, xmax=35.6)
counts["VS_U8"] = stitch("+12V_PUMP", Z10, pitch=1.0, margin=0.5, xmin=29.9)
counts["VS_U9"] = stitch("+12V_PUMP", Z12, pitch=1.0, margin=0.5, xmin=29.9)
counts["GND_U8"] = stitch("GND", Z11, pitch=1.0, margin=0.6, xmax=35.6)
counts["GND_U9"] = stitch("GND", Z13, pitch=1.0, margin=0.6, xmax=35.6)
counts["OUT_A"] = stitch("PUMP_OUT_A", Z14)
counts["OUT_B"] = stitch("PUMP_OUT_B", Z15)
print("vias:", counts)

# ── hand-made tracks: BTN8982 pin 4 (OUT) to its own tab, under the package ─────────────
def track(net, pts, w, layer=F):
    ni = b.FindNet(net)
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(b)
        t.SetStart(P(x1, y1)); t.SetEnd(P(x2, y2)); t.SetWidth(MM(w)); t.SetLayer(layer); t.SetNet(ni)
        b.Add(t)
for ref, net in (("U8", "PUMP_OUT_A"), ("U9", "PUMP_OUT_B")):
    fp = b.FindFootprintByReference(ref)
    p4 = [p for p in fp.Pads() if p.GetNumber() == "4"][0].GetPosition()
    x = p4.x / 1e6 - ORIGIN[0]; y = p4.y / 1e6 - ORIGIN[1]
    track(net, [(x, y), (21.5, y)], 0.8)


pcbnew.SaveBoard(PCB, b)
print("zones:", len(ZONES))
