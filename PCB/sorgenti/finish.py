# -*- coding: utf-8 -*-
"""After autorouting: import SES, add GND pours on outer layers, silkscreen labels, fill, save."""
import sys, os
import pcbnew
from placement import ORIGIN, BOARD_W, BOARD_H

PCB = sys.argv[1]
SES = PCB[:-10] + ".ses"
b = pcbnew.LoadBoard(PCB)
MM = pcbnew.FromMM
def P(x, y): return pcbnew.VECTOR2I(MM(ORIGIN[0] + x), MM(ORIGIN[1] + y))

if os.path.exists(SES) and "--no-ses" not in sys.argv:
    print("import ses", pcbnew.ImportSpecctraSES(b, SES))

# ---- GND pours on F.Cu / B.Cu (lowest priority, fill everything else) -------------------
for layer, name in ((pcbnew.F_Cu, "GND_F"), (pcbnew.B_Cu, "GND_B")):
    if any(z.GetZoneName() == name for z in b.Zones()):
        continue
    z = pcbnew.ZONE(b)
    z.SetLayer(layer); z.SetNet(b.FindNet("GND")); z.SetAssignedPriority(0); z.SetZoneName(name)
    z.SetLocalClearance(MM(0.3)); z.SetMinThickness(MM(0.25))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THT_THERMAL)
    z.SetThermalReliefGap(MM(0.3)); z.SetThermalReliefSpokeWidth(MM(0.5))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    ol = z.Outline(); ol.NewOutline()
    for x, y in ((0.5, 0.5), (BOARD_W - 0.5, 0.5), (BOARD_W - 0.5, BOARD_H - 0.5), (0.5, BOARD_H - 0.5)):
        ol.Append(MM(ORIGIN[0] + x), MM(ORIGIN[1] + y))
    b.Add(z)

filler = pcbnew.ZONE_FILLER(b)
filler.Fill(b.Zones())
pcbnew.SaveBoard(PCB, b)
print("finished")
