"""Tagli di tubi e longheroni, raccordi e posizioni dei pezzi lungo i longheroni, ricavati dalle quote del modello."""
import math
from machine import *
import mrefs as R

rows = []
print("== come sono posati i tubi ==")
print("   valvole da sinistra: C1, WB, C2, C3, scarico (cosi' i quattro tubi verso il bagno non si incrociano)")
print(f"   valvole -> bagno: un pezzo solo ciascuno, a ponte sopra il bordo del contenitore; salgono in verticale fino a Z = {Z_BEND:.0f}, "
      f"poi curve di raggio {BEND_R:.0f} mm (sommita' a Z = {Z_BEND + BEND_R + PIPE_R:.0f})")
print("   scarico: gomito sopra la valvola, girato di %.0f gradi verso il retro; il tubo entra nella rientranza del contenitore e la segue fino al passaparete" % R.WASTE_ANG)
print("   collettore -> pompa: gomito a codolo, col codolo nella sede con O-ring del collettore; pompa -> tank: gomito a codolo nel raccordo destro, gomito, salita")
print(f"   troppopieno: silicone diritto, in discesa di {R.OVF_ANG:.1f} gradi dall'innesto sotto il piano del cavalletto alla staffa degli scarichi")
print("== tagli (mm); nei raccordi a innesto il tubo entra per 15 mm, gia' compresi; i tubi a ponte vanno tagliati un po' abbondanti e rifilati ==")
for k, c in enumerate(("C1", "C2", "C3")):
    p0, p1 = R.feed_ends(k); D = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
    L = INS + (Z_BEND - p0[2]) + bent_len(D, BEND_R) + (Z_BEND - (p1[2] - INS))
    rows.append((f"tubo 3/8\" valvola {c} -> sifone {c}, a ponte (curve di raggio {min(BEND_R, D / 2):.0f})", L))
p0, p1 = R.WB_ENDS; D = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
rows.append((f"tubo 3/8\" valvola WB -> pescante nel bagno, a ponte (curve di raggio {min(BEND_R, D / 2):.0f})", INS + (Z_BEND - p0[2]) + bent_len(D, BEND_R) + (Z_BEND - (TUB_FLOOR + 8.0))))
rows.append(("tubo 3/8\" valvola scarico -> gomito (nascosto nei due innesti)", (Z_LANE_WASTE - ELB_L - VALVE_Z1) + 2 * INS))
rows.append(("tubo 3/8\" gomito dello scarico -> passaparete SCARICO", math.hypot(R.WASTE_J[0] - VALVE_X[4], R.WASTE_J[1] - VALVE_Y) - ELB_L + (BASE_X - 26.0 - R.WASTE_J[0]) + 2 * INS))
rows.append(("tubo 3/8\" gomito a codolo del collettore -> pompa", (R.PUMP_PL - (R.P_MAN[0] + ELB_L)) + 2 * INS))
rows.append(("tubo 3/8\" tra i due gomiti sotto il cavalletto", (PUMP_PORT_Y - ELB_L) - (R.Y_RISE + ELB_L) + 2 * INS))
rows.append(("tubo 3/8\" salita verso la tank", R.Z_HOSE0 - (PUMP_PORT_Z + ELB_L) + INS))
rows.append(("spezzone 3/8\" collettore -> valvola (x5)", 14.0 + (VALVE_Z0 - MAN_Z1) + INS))
dip = (Z_CROSS - ELB_L) - (VAS_ZB + GN9_WALL + 3.0) + INS
rows.append(("testina: pescante, taglio a 45 gradi in fondo (x3)", dip))
rows.append(("testina: ponticello tra i due gomiti (x3)", (Y_DIP - Y_STUB - 2 * ELB_L) + 2 * INS))
rows.append(("testina: codolo, punta conica Ø7,5 lunga 8 (x3)", (Z_CROSS - ELB_L) - (Z_MOUTH - 5.0) + INS))
rows.append(("silicone 8x12: sifone (x3)", 10.0 + math.pi * U_R + 15.0))
Rb = math.hypot(R.X_RISE - R.BARB_X, R.Y_RISE - R.BARB_Y) / 2
rows.append((f"silicone 8x12 nero: pescante tank -> tubo 3/8\" (curva libera di raggio {Rb:.0f})", 14.0 + (R.Z_HOSE0 - Z_T0 - 84.0) + math.pi * Rb + 15.0))
rows.append(("silicone 8x12 nero: troppopieno, dall'innesto al bordo della base", (BASE_X - R.OVF_XM - 2.0) / math.cos(math.radians(R.OVF_ANG))))
rows.append(("longherone di alluminio 15x15 (x2)", RAIL_X1 - RAIL_X0))
for n, L in rows: print(f"   {n:74s} {L:6.0f}")
mult = lambda n: 5 if "x5" in n else 3 if "x3" in n else 2 if "x2" in n else 1
tot = sum(L * mult(n) for n, L in rows if n.startswith(("tubo", "spezzone", "testina")))
print(f"   totale tubo 3/8\": {tot / 1000:.1f} m;  silicone 8x12: {sum(L * mult(n) for n, L in rows if n.startswith('silicone')) / 1000:.2f} m + i tubi esterni verso le taniche")
print("== raccordi ==")
print("   gomiti a innesto 3/8\": 6 nelle tre testine + 1 sopra la valvola dello scarico + 1 ai piedi della salita verso la tank = 8")
print(f"   gomiti a codolo 3/8\": 2. Sul collettore il codolo e' lungo {R.P_MAN[2] - (MAN_Z1 - 14.0):.0f} mm dall'angolo (deve superare l'O-ring: almeno {R.P_MAN[2] - (MAN_Z1 - 8.0 - 2.0):.0f}); "
      f"sulla pompa va accorciato a {R.P_PUMP[0] - R.PUMP_PR + INS:.0f} mm dall'angolo")
print("   raccordi diritti filetto maschio 3/8\" BSP - tubo 3/8\": 2 (pompa);  passaparete 3/8\": 1 (SCARICO)")
print("== posizioni lungo i longheroni (mm dall'estremita' sinistra del longherone al centro del pezzo) ==")
print("   staffe con ganci C03 e fermi posteriori C04:", [round(x - RAIL_X0) for x in VAS_X], "| portasonde C06:", round(WB_X - RAIL_X0))
print("== fori nel contenitore del bagno (parete corta destra, dall'interno) ==")
print(f"   2 fori Ø16,5 per i riscaldatori: a {HEAT_Z - TUB_FLOOR:.0f} mm dal fondo interno, a {HEAT_Y[0] - IY0:.0f} e {HEAT_Y[1] - IY0:.0f} mm dalla parete lunga anteriore")
print(f"   sensori di livello sulla parete lunga anteriore, all'esterno: centro a {LVL_X - TUB_X0:.0f} mm dallo spigolo sinistro, a {LVL_ZMIN:.0f} mm (MIN) e {LVL_ZMAX:.0f} mm (MAX) dal piano d'appoggio")
