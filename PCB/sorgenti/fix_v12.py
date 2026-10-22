# -*- coding: utf-8 -*-
"""Post-routing fixes applied to the v1.2 board (out_v21 -> out_v21t, 22/10/2026). They depend on that routing run:
after a new make.sh run, check the residual DRC items again rather than replaying the coordinates blindly.
  1. HEAT1_DRV (U5.7 -> R63.1): the corridor above C40/C41 is now used by the hand-routed heater gates (power.py);
     routed with the grid router using 0.5 mm vias (allowed: annular 0.1 mm):  MAZE_VIA=0.5 MAZE_CLR=0.22 maze.py
  2. +3V3 into U7 pin 8 (0.65 mm pitch): the last two 0.6 mm segments narrowed to 0.3 mm (clearance to MOT_A_CTL).
  3. A 0.7 mm2 floating GND sliver on F.Cu next to C54: rule area without copper pour.
  4. R62 (HEAT2_CMD pull-down) rotated 180 deg so that its GND pad sits next to ground; HEAT2_CMD re-joined from its
     via, the GND pad tied to the GND pad of D44 (no room for a via next to the old GND pad).
  5. C56 GND pad: solid zone connection (also in build_pcb.py), like C55.
Usage: fix_v12.py board.kicad_pcb  (steps 2-5; step 1 is the maze command above)."""
import sys, pcbnew
from placement import ORIGIN
B = sys.argv[1]
b = pcbnew.LoadBoard(B)
MM = pcbnew.FromMM
def P(x, y): return pcbnew.VECTOR2I(MM(ORIGIN[0] + x), MM(ORIGIN[1] + y))
def L(v): return (round(v.x / 1e6 - ORIGIN[0], 3), round(v.y / 1e6 - ORIGIN[1], 3))
for t in b.GetTracks():                                            # 2
    if t.GetClass() == "PCB_TRACK" and t.GetNetname() == "+3V3" and t.GetLayer() == pcbnew.F_Cu \
            and (50.195, 45.218) in (L(t.GetStart()), L(t.GetEnd())) and t.GetWidth() > MM(0.31):
        t.SetWidth(MM(0.3))
z = pcbnew.ZONE(b); z.SetIsRuleArea(True); z.SetDoNotAllowCopperPour(True)   # 3
z.SetDoNotAllowVias(False); z.SetDoNotAllowTracks(False); z.SetDoNotAllowPads(False); z.SetDoNotAllowFootprints(False)
z.SetLayer(pcbnew.F_Cu); z.SetZoneName("NO_GND_SLIVER_C54")
ol = z.Outline(); ol.NewOutline()
for x, y in ((30.0, 60.3), (31.9, 60.3), (31.9, 61.1), (30.0, 61.1)): ol.Append(P(x, y).x, P(x, y).y)
b.Add(z)
r62 = b.FindFootprintByReference("R62")                            # 4
r62.SetOrientationDegrees((r62.GetOrientationDegrees() + 180) % 360)
for t in [t for t in b.GetTracks() if t.GetClass() == "PCB_TRACK" and t.GetNetname() == "HEAT2_CMD"
          and t.GetLayer() == pcbnew.F_Cu and {L(t.GetStart()), L(t.GetEnd())} & {(61.2, 18.951), (61.2, 18.875)}]:
    b.Remove(t)
for net, a, c in (("HEAT2_CMD", (61.931, 19.682), (62.85, 18.875)), ("GND", (61.2, 18.875), (61.238, 20.675))):
    t = pcbnew.PCB_TRACK(b); t.SetStart(P(*a)); t.SetEnd(P(*c)); t.SetWidth(MM(0.25)); t.SetLayer(pcbnew.F_Cu)
    t.SetNet(b.FindNet(net)); b.Add(t)
for p in b.FindFootprintByReference("C56").Pads():                 # 5
    if p.GetNetname() == "GND": p.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(B, b)
print("fix_v12 applied")
