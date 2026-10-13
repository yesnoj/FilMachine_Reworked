# -*- coding: utf-8 -*-
"""Add power zones, stitching vias and hand-made high-current tracks to the board (before autorouting).
v1.1: three separate 12 V inputs (J1 heater 1, J2 heater 2, J3 main), 2410 SMD fuses F1-F4."""
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

# ── heater block: two lanes under the top-edge terminals J23 (lane 1) and J24 (lane 2) ───────────────
# All heavy copper is on F.Cu (2 oz). B.Cu is kept for the GND return (zone GB below) and for the +12V_IN_H2
# strip; In1 (GND plane) is left as intact as possible: the only vias of other nets in this region are those of
# IN_H2_R, Z8 and Z9.
Z1 = [(11.6, 2.3), (18.3, 2.3), (18.3, 9.0), (20.0, 9.0), (20.0, 21.4), (15.9, 21.4), (15.9, 23.9), (11.6, 23.9)]  # +12V_H1
Z2 = [(18.9, 2.3), (26.8, 2.3), (26.8, 23.9), (20.4, 23.9), (20.4, 9.0), (18.9, 9.0)]       # HEAT1_N
Z3 = [(27.4, 2.3), (32.3, 2.3), (32.3, 9.0), (34.6, 9.0), (34.6, 20.6), (27.4, 20.6)]       # +12V_H2
Z4 = [(32.9, 2.3), (42.0, 2.3), (42.0, 23.9), (35.0, 23.9), (35.0, 9.0), (32.9, 9.0)]       # HEAT2_N
for net, pts in (("+12V_H1", Z1), ("HEAT1_N", Z2), ("+12V_H2", Z3), ("HEAT2_N", Z4)):
    ZONES.append((net, F, pts, 10, net + "_F"))
# heater 1 input: J1 (left edge, lower) pin 2 -> F1 pad 1 (F1 vertical at x 12.1-15.1, pad 1 at y 26.5-29.0)
IN_H1 = rect(2.4, 24.3, 15.9, 30.2)
ZONES.append(("+12V_IN_H1", F, IN_H1, 10, "IN_H1_F"))
# heater 2 input: J2 (left edge, upper) pin 2 (THT: all layers) -> strip on B + In2 under lane 1 -> vias -> F2 pad 1
IN_H2_R = rect(30.9, 21.2, 34.6, 30.5)                                                     # F, F2 pad 1 + vias
IN_H2_S = [(2.4, 13.0), (34.6, 13.0), (34.6, 30.5), (30.9, 30.5), (30.9, 19.4), (2.4, 19.4)]
ZONES.append(("+12V_IN_H2", F, IN_H2_R, 10, "IN_H2_R"))
ZONES.append(("+12V_IN_H2", B_, IN_H2_S, 10, "IN_H2_B"))
ZONES.append(("+12V_IN_H2", I2, IN_H2_S, 10, "IN_H2_I2"))
Z5 = rect(24.6, 24.3, 30.5, 30.5)                                                          # GND Q9 source (F)
Z6 = rect(39.4, 24.3, 43.2, 33.6)                                                          # GND Q10 source (F; gate keeps an exit to the left)
ZONES.append(("GND", F, Z5, 10, "GND_Q9"))
ZONES.append(("GND", F, Z6, 10, "GND_Q10"))
# GND return on B.Cu (2 oz): J1 pin 1 -> band under lane 1 -> Q9 source vias (Z5). The In1 plane, intact around
# J1/J2/J3, carries the rest (Q10 and the pump bridge).
GB = [(2.4, 19.9), (30.5, 19.9), (30.5, 30.5), (24.6, 30.5), (24.6, 23.8), (2.4, 23.8)]
ZONES.append(("GND", B_, GB, 10, "GND_RET_B"))

# ── main input / pump ──────────────────────────────────────────────────────────────────
Z7 = [(9.2, 35.0), (18.6, 35.0), (18.6, 50.6), (2.4, 50.6), (2.4, 41.8), (9.2, 41.8)]       # +12V_IN_M (J3 pin 2, lower) -> F3, F4 pad 1
ZONES.append(("+12V_IN_M", F, Z7, 10, "IN_M_F"))
Z8 = rect(20.6, 35.0, 36.0, 42.6)                                                          # +12V_PUMP (F3 pad 2, D2)
ZONES.append(("+12V_PUMP", F, Z8, 10, "PUMP_TOP_F"))
Z8b = [(20.6, 35.0), (42.2, 35.0), (42.2, 76.0), (36.4, 76.0), (36.4, 69.6), (29.6, 69.6), (29.6, 65.4),
       (36.4, 65.4), (36.4, 54.6), (29.6, 54.6), (29.6, 50.6), (36.4, 50.6), (36.4, 42.6), (20.6, 42.6)]
ZONES.append(("+12V_PUMP", B_, Z8b, 10, "PUMP_B"))
Z9 = rect(20.6, 43.2, 36.0, 50.2)                                                          # +12V (F4 pad 2, D1) -> vias -> In2 plane
ZONES.append(("+12V", F, Z9, 10, "12V_F"))
Z10 = [(25.9, 50.8), (42.2, 50.8), (42.2, 54.0), (29.4, 54.0), (29.4, 54.35), (25.9, 54.35)]  # VS U8
Z11 = rect(25.9, 61.05, 36.0, 65.0)                                                        # GND U8
Z12 = [(25.9, 65.6), (42.2, 65.6), (42.2, 69.0), (29.4, 69.0), (29.4, 69.35), (25.9, 69.35)]  # VS U9
Z13 = rect(25.9, 76.05, 36.0, 80.6)                                                        # GND U9
ZONES += [("+12V_PUMP", F, Z10, 10, "VS_U8"), ("GND", F, Z11, 10, "GND_U8"),
          ("+12V_PUMP", F, Z12, 10, "VS_U9"), ("GND", F, Z13, 10, "GND_U9")]
Z14 = rect(2.4, 51.4, 24.0, 60.0)                                                          # PUMP_OUT_A (J25 pin 1), F only
Z15 = [(2.4, 61.0), (12.0, 61.0), (12.0, 63.7), (24.0, 63.7), (24.0, 78.6), (2.4, 78.6)]    # PUMP_OUT_B (J25 pin 2), F only
ZONES.append(("PUMP_OUT_A", F, Z14, 10, "OUTA_F"))
ZONES.append(("PUMP_OUT_B", F, Z15, 10, "OUTB_F"))

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
counts["IN_H2_R"] = stitch("+12V_IN_H2", IN_H2_R, pitch=1.0, margin=0.6)
counts["GND_Q9"] = stitch("GND", Z5, pitch=1.0, margin=0.6)
counts["GND_Q10"] = stitch("GND", Z6, pitch=1.0, margin=0.6)
counts["PUMP_TOP"] = stitch("+12V_PUMP", Z8, pitch=2.0)
counts["12V_F"] = stitch("+12V", Z9, pitch=2.0, xmax=35.6)
counts["VS_U8"] = stitch("+12V_PUMP", Z10, pitch=1.0, margin=0.5, xmin=29.9)
counts["VS_U9"] = stitch("+12V_PUMP", Z12, pitch=1.0, margin=0.5, xmin=29.9)
counts["GND_U8"] = stitch("GND", Z11, pitch=1.0, margin=0.6, xmax=35.6)
counts["GND_U9"] = stitch("GND", Z13, pitch=1.0, margin=0.6, xmax=35.6)
print("vias:", counts)

# ── hand-made tracks: BTN8962 pin 4 (OUT) to its own tab, under the package ─────────────
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

# ── ADS1115 (VSSOP-10, 0.5 mm pitch): pads 1 (ADDR) and 3 (GND) are both GND with the unconnected pad 2
# between them. A U-shaped stub joins them outside the package and one via ties it to the GND plane, so the
# autorouter never has to escape a 0.3 mm pad for GND. ─────────────────────────────────────────────────
u4 = b.FindFootprintByReference("U4")
pos = {p.GetNumber(): (p.GetPosition().x / 1e6 - ORIGIN[0], p.GetPosition().y / 1e6 - ORIGIN[1]) for p in u4.Pads()}
cx4 = u4.GetPosition().x / 1e6 - ORIGIN[0]; cy4 = u4.GetPosition().y / 1e6 - ORIGIN[1]
(x1, y1), (x3, y3) = pos["1"], pos["3"]
assert abs(y1 - y3) < 1e-3, "U4 pads 1/3 expected on one row (U4 rotated 90/270)"
out = 1.0 if y1 > cy4 else -1.0                      # outward = away from the package centre
yo = y1 + out * (1.45 / 2 + 0.65)                    # 0.65 mm past the pad end (pad 2 stays 0.35 mm clear of the via)
track("GND", [(x1, y1), (x1, yo), (x3, yo), (x3, y3)], 0.25)
v = pcbnew.PCB_VIA(b)
v.SetPosition(P((x1 + x3) / 2, yo)); v.SetWidth(MM(VIA_D)); v.SetDrill(MM(VIA_H)); v.SetNet(b.FindNet("GND")); v.SetIsFree(False)
b.Add(v)
print("U4 GND stub at", round((x1 + x3) / 2, 2), round(yo, 2))
# Staggered 0.2 mm escape stubs for the other U4 pads (0.5 mm pitch), so the autorouter only has to pick them
# up at their ends; pad 8 (VDD) is wired straight into C15 pad 1 (its decoupling capacitor right above).
pad_w = {p.GetNumber(): p for p in u4.Pads()}
def stub(num, length, w=0.2):
    px, py = pos[num]
    o = 1.0 if py > cy4 else -1.0
    end = py + o * (1.45 / 2 + length)
    track(pad_w[num].GetNetname(), [(px, py), (px, end)], w)
    return px, end
for num, ln in (("10", 0.8), ("9", 1.6), ("7", 1.6), ("6", 0.8), ("4", 1.2), ("5", 0.6)):
    stub(num, ln)
c15 = b.FindFootprintByReference("C15")
c15p = [p for p in c15.Pads() if p.GetNetname() == "+3V3"][0].GetPosition()
cx15, cy15 = c15p.x / 1e6 - ORIGIN[0], c15p.y / 1e6 - ORIGIN[1]
x8, y8 = pos["8"]
o8 = 1.0 if y8 > cy4 else -1.0
mid = y8 + o8 * (1.45 / 2 + 1.9)
track("+3V3", [(x8, y8), (x8, mid), (cx15, cy15)], 0.2)
print("U4 VDD -> C15 at", round(cx15, 2), round(cy15, 2))

# ── hand-routed signal paths the autorouter failed on in previous runs ───────────────────────────────
def padpos(ref, num):
    f = b.FindFootprintByReference(ref)
    p = [q for q in f.Pads() if q.GetNumber() == str(num)][0].GetPosition()
    return p.x / 1e6 - ORIGIN[0], p.y / 1e6 - ORIGIN[1]
def via(net, x, y):
    v = pcbnew.PCB_VIA(b); v.SetPosition(P(x, y)); v.SetWidth(MM(VIA_D)); v.SetDrill(MM(VIA_H)); v.SetNet(b.FindNet(net)); v.SetIsFree(False); b.Add(v)
# Heater gate drives: R63/R65 (gate resistors, block right of U5) -> F corridor above U5 (y 15.15 / 15.65, between
# J5's pad row and the block) -> down the F gap between Z4 and U5 (x 43.0 / 43.6) -> B -> west under the heater
# block (y 33.6 / 34.5, between the Q10 GND vias and the pump zone) -> up to the gate pads of Q10 / Q9.
r63, r65 = padpos("R63", 2), padpos("R65", 2)
q9g, q10g = padpos("Q9", 1), padpos("Q10", 1)
assert 56 < r63[0] < 61 and 60 < r65[0] < 65 and abs(r63[1] - 17.08) < 0.3, (r63, r65)
assert abs(q9g[0] - 21.27) < 0.1 and abs(q9g[1] - 26.19) < 0.1 and abs(q10g[0] - 36.26) < 0.1, (q9g, q10g)
track("HEAT1_G", [r63, (r63[0], 15.65), (43.6, 15.65), (43.6, 29.0)], 0.25)
via("HEAT1_G", 43.6, 29.0)
track("HEAT1_G", [(43.6, 29.0), (43.6, 34.5), (21.5, 34.5), (21.5, 28.4)], 0.25, B_)
via("HEAT1_G", 21.5, 28.4)
track("HEAT1_G", [(21.5, 28.4), (q9g[0], q9g[1] + 0.4), q9g], 0.25)
track("HEAT2_G", [r65, (r65[0], 15.15), (42.9, 15.15), (42.9, 23.6)], 0.25)
via("HEAT2_G", 42.9, 23.6)
track("HEAT2_G", [(42.9, 23.6), (42.9, 33.6), (37.5, 33.6)], 0.25, B_)
via("HEAT2_G", 37.5, 33.6)
track("HEAT2_G", [(37.5, 33.6), (37.5, 27.6), (q10g[0], q10g[1] + 0.5), q10g], 0.25)
# MCP23017 VDD (pin 9, mid left column, 0.65 mm pitch): 0.25 mm link down the left side of the pads to C13 pad 1,
# which the 0.6 mm +3V3 track can reach.
u3v, c13 = padpos("U3", 9), padpos("C13", 1)
assert abs(u3v[0] - 66.65) < 0.1 and abs(c13[1] - 37.78) < 0.3 and 65.5 < c13[0] < 67, (u3v, c13)
track("+3V3", [u3v, (65.3, u3v[1]), (65.3, c13[1]), c13], 0.25)
print("hand-routed HEAT1_G, HEAT2_G, U3 VDD")


pcbnew.SaveBoard(PCB, b)
print("zones:", len(ZONES))
