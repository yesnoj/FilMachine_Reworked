# -*- coding: utf-8 -*-
"""Apply to an already routed board the same solid zone connection build_pcb.py now gives to GND THT pads of
J1-J3, J5, J6, C1, C55, J27-J30 (avoids starved thermals where tracks pass close to the pad), then refill. Usage: fullconn.py board"""
import sys, pcbnew
b = pcbnew.LoadBoard(sys.argv[1])
for ref in ("J1", "J2", "J3", "J5", "J6", "C1", "C55", "J27", "J28", "J29", "J30"):
    for pad in b.FindFootprintByReference(ref).Pads():
        if pad.GetNetname() == "GND":
            pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(sys.argv[1], b)
print("full connection set")
