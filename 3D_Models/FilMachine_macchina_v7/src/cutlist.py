"""Tagli dei tubi e dei flessibili, raccordi, fori nei pannelli e nei contenitori, ricavati dalle quote del modello (rilascio 7)."""
import math
from machine import *
import mrefs as R, hydro as H

rows = []
print("== come sono posate le linee ==")
print(f"   valvole sdraiate sulla culla C02, bocca posteriore nel collettore C01 (spezzone 3/8\"); da sinistra: WB, C1, SCARICO, C2, RECUPERO, C3 (x = {[round(v) for v in sorted(VALVE_X)]})")
print("   C1, C2, C3: gomito davanti alla valvola, in su' nella camera di livello C03 (spezzone), dalla camera in su' nel raccordo a innesto del passaparete nel fondo del "
      "contenitore (spezzone). Bocca laterale della camera (+X): adattatore a portagomma e silicone 8x12 fino al rubinetto sul fronte")
print(f"   WB: gomito, su' a z = {WB_LANE_Z:.0f}, verso il retro fino a y = {WB_Y:.0f} (oltre la bobina), a destra fino a x = {WB_X:.0f}, su' nel raccordo del passaparete WB (fondo del bagno, vicino alla parete posteriore)")
print(f"   SCARICO: gomito, su' a z = {LINE_Z:.0f}, dritto all'attacco sul pannello posteriore")
print(f"   troppopieno del bagno: dal raccordo del passaparete (tubo verticale a filo dell'acqua, staffa C14) giu' a z = {LINE_Z:.0f}, dritto all'attacco sul retro")
print(f"   ingresso: attacco sul retro -> flussimetro (lungo Y, culla C04) -> due gomiti -> valvola dell'ingresso (lungo X, culla C07) -> gomito a x = {IN_X:.0f} -> "
      f"verso il retro (tra il tubo della pompa e la valvola WB) -> gomito su' nel raccordo del passaparete dell'ingresso a (x {IN_X:.0f}, y {IN_Y:.0f}), a destra della tank "
      f"(tubo verticale fino a {IN_Z - FLOOR:.0f} mm dal fondo: salto d'aria sopra l'acqua)")
print("   collettore -> Kamoer: gomito a codolo sull'uscita, adattatore a portagomma, silicone 6x10 fino alla bocca A; bocca B -> silicone 6x10 -> adattatore sotto il "
      "passaparete della tank; sopra il fondo del bagno il passaparete ha il portagomma per il silicone 8x12 NERO fino al portagomma della tank")
print("   troppopieno della tank: silicone 8x12 infilato nella colonna da sotto, attraverso il gommino nel fondo e nel piano, fino all'attacco sul retro")
print("   RECUPERO: adattatore a portagomma sulla bocca anteriore della valvola, silicone 8x12 fino al passaparete portagomma sul fronte")
print("== controllo delle spezzate: ogni tratto deve ospitare i bracci dei gomiti (28 mm dall'angolo) e almeno 2 mm di tubo in vista ==")
nbad = 0
for n, f in R.LINES.items():
    pts = f(); nn = len(pts) - 1
    for i, (a, b) in enumerate(zip(pts[:-1], pts[1:])):
        L = math.dist(a, b); need = (ELB_L if i > 0 else 0.0) + (ELB_L if i < nn - 1 else 0.0) + 2.0
        flag = "" if L >= need - 1e-6 else "  <-- TROPPO CORTO"
        if flag: nbad += 1
        print(f"   linea {n:20s} tratto {i + 1}: {L:6.1f} mm (minimo {need:4.0f}){flag}")
print(f"   tratti troppo corti: {nbad}")
assert nbad == 0, "spezzate con tratti troppo corti"
print("== tagli dei tubi 3/8\" (mm; nei raccordi a innesto il tubo entra per 15 mm, gia' compresi) ==")
for k, c in enumerate(("C1", "C2", "C3")):
    rows.append((f"spezzone 3/8\" gomito della valvola {c} -> camera {c}", (CH_Z0 + 14.0) - (VALVE_Z + ELB_L - INS)))
    rows.append((f"spezzone 3/8\" camera {c} -> raccordo del passaparete {c}", (BULK_Z0 + INS) - (CH_Z1 - 14.0)))
for n, f in R.LINES.items():
    for i, L in enumerate(tube_lens(f())):
        rows.append((f"tubo 3/8\" linea {n}, tratto {i + 1}", L))
rows.append(("spezzone 3/8\" collettore -> valvola (x6)", (MAN_Y0 + 14.0) - (VALVE_Y1 - INS)))
rows.append(("tubo 3/8\" verticale del troppopieno del bagno (dal raccordo del passaparete a filo dell'acqua)", OVF_Z - FLOOR - 24.0 + INS))
rows.append(("tubo 3/8\" verticale del beccuccio dell'ingresso", IN_Z - FLOOR - 24.0 + INS))
print("== flessibili (mm) ==")
for k in range(3):
    rows.append((f"silicone 8x12 camera C{k + 1} -> rubinetto C{k + 1}", route_len(R.chem_hose_pts(k)) + 20.0))
rows.append(("silicone 8x12 valvola RECUPERO -> uscita sul fronte", route_len(R.recovery_hose_pts()) + 20.0))
rows.append(("silicone 8x12 NERO: portagomma della tank -> portagomma del passaparete", route_len([(TANK_BARB[0] - 12.0, TANK_BARB[1], TANK_BARB[2]), (TANK_BH[0], TANK_BARB[1], TANK_BARB[2]), (TANK_BH[0], TANK_BH[1], TANK_BARB[2]), (TANK_BH[0], TANK_BH[1], FLOOR + 16.0)]) + 20.0))
rows.append(("silicone 8x12 NERO: troppopieno della tank, dalla colonna a 30 mm fuori dal pannello posteriore", route_len(R.overflow_hose_pts()) + 30.0 + 20.0))
rows.append(("silicone 6x10: adattatore del collettore -> bocca A della Kamoer", route_len(R.pump_hose_pts()) + 20.0))
rows.append(("silicone 6x10: bocca B della Kamoer -> adattatore del passaparete della tank", route_len(R.tank_hose_low_pts()) + 20.0))
for n, L in rows: print(f"   {n:100s} {L:6.0f}")
mult = lambda n: 6 if "x6" in n else 1
tot = sum(L * mult(n) for n, L in rows if "3/8" in n)
print(f"   tubo 3/8\" in tutto: {tot / 1000:.2f} m | silicone 8x12: {sum(L for n, L in rows if '8x12' in n) / 1000:.2f} m | silicone 6x10: {sum(L for n, L in rows if '6x10' in n) / 1000:.2f} m")
print("== raccordi ==")
print(f"   gomiti a innesto 3/8\": {R.elbow_count() - 1} (5 davanti alle valvole + {sum(max(len(f()) - 2, 0) for f in R.LINES.values())} sulle linee); gomito a codolo: 1; adattatori a portagomma 10: 4; a portagomma 6,4: 2")
print("== fori (mm) ==")
print(f"   fondo del bagno (PMMA 4) e piano (multistrato): " + "; ".join(f"{n} Ø{2 * r:g} in ({x:.1f}; {y:.1f})" for n, (x, y, r) in DECK_CUTS.items() if n in BATH_CUTS))
print(f"   piano, solo: " + "; ".join(f"{n} Ø{2 * r:g} in ({x:.1f}; {y:.1f})" for n, (x, y, r) in DECK_CUTS.items() if n not in BATH_CUTS))
print(f"   contenitori: un foro Ø{2 * BULK_R:g} al centro del fondo (passaparete)")
print(f"   parete destra del bagno: due fori Ø16,6 (riscaldatori) a y = {HEAT_Y[0]:.0f} e {HEAT_Y[1]:.0f}, z = {HEAT_Z - FLOOR:.0f} dal fondo interno (adattare al filetto delle cartucce)")
print(f"   parete sinistra del bagno: finestra {R.WIN[1] - R.WIN[0]:.0f} x {R.WIN[3] - R.WIN[2]:.0f} e 8 fori Ø3,4 (viti della cornice); bordo superiore a {WIN_TOP_Z - FLOOR:.0f} dal fondo")
print(f"   pannello frontale: display {DISP_WIN[0]:g} x {DISP_WIN[1]:g} centrato in ({DISP_X:.0f}; {DISP_Z:.0f}), rubinetti Ø21 in x = {[round(x) for x in TAP_X]} z = {TAP_Z:.0f}, RECUPERO Ø13 in ({REC_X:.0f}; {REC_Z:.0f})")
print(f"   pannello posteriore: IEC 27,5 x 47,5 in x {IEC_X[0]:.0f}..{IEC_X[1]:.0f} z {IEC_Z[0]:.0f}..{IEC_Z[1]:.0f}; attacchi Ø16,2: " + "; ".join(f"{n} ({x:.0f}; {z:.0f})" for n, (x, z) in CONN.items()))
print(f"== acqua nel bagno: {R.water_volume_l():.2f} l con il livello a {WATER_Z - FLOOR:.0f} mm dal fondo ==")
