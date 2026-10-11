# -*- coding: utf-8 -*-
"""Apply to an already routed board the same solid zone connection build_pcb.py now gives to GND THT pads of
J3, J4, C55, J22 (avoids starved thermals where tracks pass close to the pad), then refill. Usage: fullconn.py board"""
import sys, pcbnew
b = pcbnew.LoadBoard(sys.argv[1])
for ref in ("J3", "J4", "C55", "J22"):
    for pad in b.FindFootprintByReference(ref).Pads():
        if pad.GetNetname() == "GND":
            pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(sys.argv[1], b)
print("full connection set")
