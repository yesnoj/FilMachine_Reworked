"""Tagli di tubi, pannelli e longheroni, raccordi, fori nel contenitore e nelle vaschette, ricavati dalle quote del modello (rilascio 6)."""
import math
from machine import *
import mrefs as R, hydro as H, shell as S

rows = []
print("== come sono posati i tubi ==")
print("   valvole da sinistra: C1, WB, C2, C3, scarico. Sopra C1, C2 e C3 c'e' la camera di livello C15 (spezzone 3/8\" dalla valvola)")
print(f"   camera -> bagno: un pezzo solo per vaschetta; sale fino a Z = {Z_BEND:.0f}, scavalca il bordo del contenitore con curve di raggio "
      f"{BEND_R:.0f} mm (sommita' Z = {Z_BEND + BEND_R + PIPE_R:.0f}), scende nella fascia anteriore del bagno a Y = {Y_DESC:.0f}, curva sul fondo e "
      f"arriva alla presa C14 sotto la vaschetta (entra 14 mm nella sede con O-ring)")
print("   WB: un pezzo solo dalla valvola al pescante, a ponte come le linee; taglio a 45 gradi in fondo")
print("   scarico: gomito sopra la valvola, girato di %.0f gradi verso il retro; il tubo passa sopra il piano del cavalletto, entra nella "
      "rientranza del contenitore e la segue fino al passaparete SCARICO" % R.WASTE_ANG)
print("   collettore -> pompa: gomito a codolo nella sede con O-ring del collettore; pompa -> tank: raccordo diritto nella pompa, tubo "
      "attraverso il fianco destro del cavalletto, gomito, salita fuori dal cavalletto e silicone nero sul portagomma della tank")
print(f"   troppopieno della tank: silicone diritto, in discesa di {R.OVF_ANG:.1f} gradi, dall'innesto sotto il piano del cavalletto alla piastra degli scarichi")
print("   ingresso dell'acqua di rete: passaparete -> flussimetro -> due gomiti -> valvola dell'ingresso -> gomito -> salita -> tratto sopra il "
      "bordo del bagno -> gomito -> beccuccio")
print("== tagli dei tubi (mm); nei raccordi a innesto il tubo entra per 15 mm, gia' compresi; i tubi a ponte vanno tagliati un po' abbondanti e rifilati ==")
for k, c in enumerate(("C1", "C2", "C3")):
    rows.append((f"tubo 3/8\" camera {c} -> presa {c}, a ponte", R.line_len(k)))
    rows.append((f"spezzone 3/8\" valvola {c} -> camera {c}", (CH_Z0 + 14.0) - (VALVE_Z1 - INS)))
p0, p1 = R.WB_ENDS; D = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
rows.append((f"tubo 3/8\" valvola WB -> pescante nel bagno, a ponte", INS + (Z_BEND - p0[2]) + bent_len(D, BEND_R) + (Z_BEND - WB_ZIN)))
rows.append(("tubo 3/8\" valvola scarico -> gomito (nascosto nei due innesti)", (Z_LANE_WASTE - ELB_L - VALVE_Z1) + 2 * INS))
rows.append(("tubo 3/8\" gomito dello scarico -> passaparete SCARICO", math.hypot(R.WASTE_J[0] - VALVE_X[4], R.WASTE_J[1] - VALVE_Y) - ELB_L + (R.PL_X0 - 18.0 - R.WASTE_J[0]) + 2 * INS))
rows.append(("tubo 3/8\" gomito a codolo del collettore -> pompa", (R.PUMP_PL - (R.P_MAN[0] + ELB_L)) + 2 * INS))
rows.append(("tubo 3/8\" pompa -> gomito fuori dal cavalletto", (R.X_RISE - ELB_L - R.PUMP_PR) + 2 * INS))
rows.append(("tubo 3/8\" salita verso la tank", R.Z_HOSE0 - (PUMP_PORT_Z + ELB_L) + INS))
rows.append(("spezzone 3/8\" collettore -> valvola (x5)", 14.0 + (VALVE_Z0 - MAN_Z1) + INS))
for n, L in R.inlet_tube_lens():
    rows.append((f"ingresso: tubo 3/8\" {n}", L))
rows.append((f"silicone 8x12 nero: portagomma della tank -> salita (curva di raggio {R.T.HOSE_RB:.0f})", 13.0 + math.pi / 2 * R.T.HOSE_RB + 15.0))
rows.append(("silicone 8x12 nero: troppopieno della tank, dall'innesto a 30 mm fuori dal pannello", math.dist((R.OVF_XM, R.OVF_YM, R.SOCK_Z), R.OVF_END) - 14.5 + 30.0))
rows.append(("PVC cristal 12x16: troppopieno del bagno, dal portagomma a 30 mm fuori dal pannello", R.bath_overflow_len() + 30.0))
rows.append(("longherone di alluminio 15x15 (x2)", RAIL_X1 - RAIL_X0))
for n, L in rows: print(f"   {n:86s} {L:6.0f}")
mult = lambda n: 5 if "x5" in n else 3 if "x3" in n else 2 if "x2" in n else 1
tot = sum(L * mult(n) for n, L in rows if "3/8" in n)
print(f"   totale tubo 3/8\": {tot / 1000:.1f} m;  silicone 8x12: {sum(L for n, L in rows if n.startswith('silicone')) / 1000:.2f} m + i tubi esterni verso le taniche")
print("== raccordi ==")
print("   gomiti a innesto 3/8\": 1 sopra la valvola dello scarico, 1 ai piedi della salita verso la tank, 5 sulla linea dell'ingresso = 7")
print(f"   gomito a codolo 3/8\": 1 sul collettore (codolo lungo {R.P_MAN[2] - (MAN_Z1 - 14.0):.0f} mm dall'angolo; deve superare l'O-ring: almeno {R.P_MAN[2] - (MAN_Z1 - 8.0 - 2.0):.0f})")
print("   raccordi diritti filetto maschio 3/8\" BSP - tubo 3/8\": 2 (pompa);  raccordi diritti filetto FEMMINA 1/2\" - tubo 3/8\": 2 (flussimetro)")
print("   passaparete a innesto 3/8\": 2 (SCARICO e INGRESSO);  passaparete 1/2\" con portagomma a gomito da 16 mm: 1 (troppopieno del bagno)")
print("   rubinetto del bagno: passaparete PP 1/2\" a filetto lungo (60 mm) + gomito 1/2\" F-F + nipplo 1/2\" corto (pescante) + rubinetto a sfera 1/2\" F x portagomma 16 mm")
print("== pannelli (taglio a misura; Dibond = composito di alluminio 3 mm) ==")
for n, f in (("pannello frontale", S.panel_front), ("pannello posteriore", S.panel_rear), ("pannello sinistro", S.panel_left), ("pannello destro", S.panel_right),
             ("piano anteriore (antracite)", S.deck), ("paratia", S.bulkhead_panel), ("divisorio", S.divider_panel), ("coperchio del bagno, PLEXIGLASS 3 mm trasparente", S.lid)):
    bb = f().val().BoundingBox(); d = sorted((bb.xlen, bb.ylen, bb.zlen), reverse=True)
    print(f"   {n:52s} {d[0]:6.1f} x {d[1]:6.1f} mm")
print(f"   fori e asole: vedi i disegni dei pannelli (docs/pannelli/pannelli.pdf e i DXF nella stessa cartella); apertura della tank nel piano: X {S.TW_X0:.1f}..{S.TW_X1:.1f}, "
      f"profonda {S.TW_Y1 + S.EDGE:.1f} dal bordo anteriore del piano")
print("== posizioni lungo i longheroni (mm dall'estremita' sinistra del longherone) ==")
print("   fermi girevoli C03 (davanti e dietro) e portasensori C04 (davanti):", [round(x - RAIL_X0) for x in VAS_X], "| portasonde C06:", round((WB_X - 10.0 + R.PROBE_X + 9.0) / 2 - RAIL_X0))
print(f"   fermi davanti: asse a {H.TB_YF - RAIL_F_Y0:.1f} mm dalla faccia anteriore del longherone anteriore; dietro: a {RAIL_R_Y0 + RAIL - H.TB_YR:.1f} mm dalla faccia posteriore del longherone posteriore")
print("== fori nel contenitore del bagno (parete corta destra, dall'interno) ==")
print(f"   2 fori Ø16,5 per i riscaldatori: a {HEAT_Z - TUB_FLOOR:.0f} mm dal fondo interno, a {HEAT_Y[0] - IY0:.0f} e {HEAT_Y[1] - IY0:.0f} mm dalla parete lunga anteriore")
print(f"   1 foro Ø21,5 per il rubinetto di svuotamento: centro a {BD_Z - TUB_FLOOR:.0f} mm dal fondo interno, a {BD_Y - IY0:.0f} mm dalla parete lunga anteriore "
      f"(tra i due riscaldatori); il pescante interno arriva a 4 mm dal fondo")
print(f"   1 foro Ø22 per il passaparete del troppopieno: centro a {OVF_BATH_Z - TUB_FLOOR:.0f} mm dal fondo interno, a {OVF_BATH_Y - IY0:.0f} mm dalla parete lunga anteriore "
      f"(il troppopieno comincia a {OVF_BATH_Z - 8 - TUB_FLOOR:.0f} mm dal fondo)")
print(f"   pannello destro: foro Ø23 per il rubinetto del bagno (Y {BD_Y:.0f}, Z {BD_Z:.0f}) - nei DXF")
print(f"   sensori di livello sulla parete lunga anteriore, all'esterno: centro a {LVL_X - TUB_X0:.0f} mm dallo spigolo sinistro, a {LVL_ZMIN:.0f} mm (MIN) e {LVL_ZMAX:.0f} mm (MAX) dal piano d'appoggio")
print("== fori nelle vaschette ==")
print(f"   un foro Ø{2 * R.PAN_HOLE_R:.1f} al centro del fondo di ogni vaschetta (punta a gradino, poi sbavare): ci passa il gambo filettato M16 di C11")
