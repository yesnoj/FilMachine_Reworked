# -*- coding: utf-8 -*-
"""Apply to an already routed board the zone-connection policy of build_pcb.py for through-hole pads (23/10/2026):
thermal reliefs with wide spokes (1.0 mm on J1-J3/J23-J25, 0.8 mm on C1/C55/C56, 0.6 mm elsewhere; J6 GND solid),
then refill. Until 22/10/2026 this script set the GND pads to solid copper instead. Usage: fullconn.py board"""
import sys, pcbnew
b = pcbnew.LoadBoard(sys.argv[1])
MM = pcbnew.FromMM
n = 0
for fp in b.GetFootprints():
    ref = fp.GetReference()
    for pad in fp.Pads():
        if pad.GetAttribute() != pcbnew.PAD_ATTRIB_PTH or not pad.GetNetname():
            continue
        if ref == "J6":
            pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL if pad.GetNetname() == "GND" else pcbnew.ZONE_CONNECTION_INHERITED)
            pad.SetLocalThermalSpokeWidthOverride(None); continue
        pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        pad.SetLocalThermalSpokeWidthOverride(MM(1.0 if ref in ("J1", "J2", "J3", "J23", "J24", "J25") else 0.8 if ref in ("C1", "C55", "C56") else 0.6)); n += 1
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(sys.argv[1], b)
print("full connection", "-> thermal reliefs on", n, "THT pads")
