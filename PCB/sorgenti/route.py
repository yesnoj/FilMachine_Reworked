# -*- coding: utf-8 -*-
"""Autoroute on a temporary copy of the board in which every hand-made power zone on F/B (and the HC
copper on In2) is also a keepout, and In1 is a power layer (plane only). Result: <board>.ses"""
import sys, pcbnew, subprocess, os, time, shutil
PCB = sys.argv[1]; passes = sys.argv[2] if len(sys.argv) > 2 else "40"; mt = sys.argv[3] if len(sys.argv) > 3 else "0"
base = PCB[:-10]
tmp = base + "_route.kicad_pcb"
shutil.copy(PCB, tmp)
shutil.copy(base + ".kicad_pro", base + "_route.kicad_pro")   # same name as the board, or KiCad drops the net classes
b = pcbnew.LoadBoard(tmp)
if not os.environ.get('INNER_NOTRACK'):
    b.SetLayerType(pcbnew.In1_Cu, pcbnew.LT_POWER)
if os.environ.get('IN2_POWER'):
    b.SetLayerType(pcbnew.In2_Cu, pcbnew.LT_POWER)
n = 0
for z in list(b.Zones()):
    if z.GetIsRuleArea():
        continue
    name = z.GetZoneName()
    if name in ("GND_PLANE", "12V_PLANE"):
        continue
    layer = z.GetLayer()
    if layer in (pcbnew.F_Cu, pcbnew.B_Cu, pcbnew.In2_Cu):
        k = pcbnew.ZONE(b)
        k.SetIsRuleArea(True)
        k.SetLayer(layer)
        k.SetDoNotAllowTracks(True); k.SetDoNotAllowVias(True)
        k.SetDoNotAllowPads(False); k.SetDoNotAllowFootprints(False); k.SetDoNotAllowCopperPour(False)
        k.SetZoneName("KO_" + name)
        so = z.Outline()
        ko = k.Outline(); ko.NewOutline()
        for i in range(so.Outline(0).PointCount()):
            pt = so.Outline(0).CPoint(i)
            ko.Append(pt.x, pt.y)
        b.Add(k); n += 1
if os.environ.get("INNER_NOTRACK"):
    from placement import ORIGIN, BOARD_W, BOARD_H
    for layer in (pcbnew.In1_Cu, pcbnew.In2_Cu):
        k = pcbnew.ZONE(b); k.SetIsRuleArea(True); k.SetLayer(layer)
        k.SetDoNotAllowTracks(True); k.SetDoNotAllowVias(False); k.SetDoNotAllowPads(False)
        k.SetDoNotAllowFootprints(False); k.SetDoNotAllowCopperPour(False); k.SetZoneName("KO_INNER")
        ko = k.Outline(); ko.NewOutline()
        for x, y in ((0, 0), (BOARD_W, 0), (BOARD_W, BOARD_H), (0, BOARD_H)):
            ko.Append(pcbnew.FromMM(ORIGIN[0] + x), pcbnew.FromMM(ORIGIN[1] + y))
        b.Add(k); n += 1
print("keepouts", n, flush=True)
print("dsn", pcbnew.ExportSpecctraDSN(b, base + ".dsn"), flush=True)
if os.path.exists(base + ".ses"):
    os.remove(base + ".ses")
t = time.time()
with open(base + "_fr.log", "w") as log:
    r = subprocess.run(["xvfb-run", "-a", "java", "-jar", "/opt/freerouting19.jar", "-de", base + ".dsn", "-do", base + ".ses",
                        "-mp", passes, "-mt", mt, "-inc", "HC"], stdout=log, stderr=subprocess.STDOUT, timeout=7200)
print("freerouting rc", r.returncode, "%.0fs" % (time.time() - t), "ses:", os.path.exists(base + ".ses"), flush=True)
os.remove(tmp); os.remove(base + "_route.kicad_pro")
