# -*- coding: utf-8 -*-
"""Revisione del 23/10/2026 applicata alla scheda v1.2 già instradata (dopo fix_v12.py). Idempotente.
  1. R80/R82 (serie IN dei BTN8962): 10k -> 4,7k (campi Value/MPN/LCSC/Description del footprint; lo schema viene da
     gen_sch.py con design.py aggiornato). Gerber e CPL non cambiano.
  2. L1 (SLO0630H): piazzole secondo il datasheet Sunltech (1,95 x 3,0 mm, spazio interno 3,6, ingombro 7,5) al posto
     delle 3,0 x 3,6 a +-2,25 (H e I del disegno erano scambiati). Stessa origine: il CPL non cambia.
  3. Piazzole passanti: rilievi termici al posto del rame pieno (saldatura a onda su 4 strati con 2 oz esterni):
     raggi da 1,0 mm sui pin di potenza (J1-J3, J23-J25), 0,8 mm sugli elettrolitici, 0,6 mm sugli altri.
     Eccezione J6 (header C6, non montato): piazzola GND a rame pieno, i raggi finirebbero su isole isolate.
     Nel .kicad_pro min_resolved_spokes = 1: J5.16 e J14.3 hanno un solo raggio su F/B (più 4 sul piano In1).
  4. Vie di cucitura dei rail +12V_IN_H2 / +12V_PUMP: 0,6/0,4 -> 0,8/0,4 (anello 0,2 mm; la netclass HC prevede 0,8/0,4).
Uso: fix_v12b.py board.kicad_pcb"""
import sys, json, os, pcbnew
B = sys.argv[1]
b = pcbnew.LoadBoard(B)
MM = pcbnew.FromMM

# 1. R80 / R82
for ref in ("R80", "R82"):
    fp = b.FindFootprintByReference(ref)
    for name, val in (("Value", "4.7k"), ("MPN", "0603 4.7k 1%"), ("LCSC", "C23162"),
                      ("Description", "Serie ingresso IN (4,7k: livello alto >= 2,5 V con logica 3,3 V e I_IN(H) max 150 uA)")):
        fp.GetFieldByName(name).SetText(val)
    fp.SetValue("4.7k")

# 2. L1 pads (footprint at 0 deg; positions are footprint-relative)
l1 = b.FindFootprintByReference("L1")
assert l1.GetOrientationDegrees() == 0
for pad in l1.Pads():
    sgn = -1 if pad.GetNumber() == "1" else 1
    pad.SetSize(pcbnew.VECTOR2I(MM(1.95), MM(3.0)))
    pad.SetFPRelativePosition(pcbnew.VECTOR2I(MM(sgn * 2.775), 0))

# 3. thermal reliefs on every through-hole pad that touches a zone (gap 0,3 mm from the zones)
POWER = {"J1", "J2", "J3", "J23", "J24", "J25"}
ELCO = {"C1", "C55", "C56"}
n = 0
for fp in b.GetFootprints():
    ref = fp.GetReference()
    for pad in fp.Pads():
        if pad.GetAttribute() != pcbnew.PAD_ATTRIB_PTH or not pad.GetNetname():
            continue
        if ref == "J6":
            pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL if pad.GetNetname() == "GND" else pcbnew.ZONE_CONNECTION_INHERITED)
            pad.SetLocalThermalSpokeWidthOverride(None)
            continue
        pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        pad.SetLocalThermalSpokeWidthOverride(MM(1.0 if ref in POWER else 0.8 if ref in ELCO else 0.6)); n += 1
print("thermal reliefs set on", n, "THT pads")
pro = B[:-10] + ".kicad_pro"
if os.path.exists(pro):
    p = json.load(open(pro)); p["board"]["design_settings"]["rules"]["min_resolved_spokes"] = 1
    json.dump(p, open(pro, "w"), indent=2)

# 4. stitching vias of the HC rails
m = 0
for t in b.GetTracks():
    if t.GetClass() == "PCB_VIA" and t.GetNetname() in ("+12V_IN_H2", "+12V_PUMP") and t.GetDrillValue() == MM(0.4):
        t.SetWidth(MM(0.8)); m += 1
print("vias 0.8/0.4:", m)

pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(B, b)
print("fix_v12b applied")
