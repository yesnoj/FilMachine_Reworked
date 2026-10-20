"""Macchina completa, rilascio 7: quote della disposizione generale (riferimento macchina) e utilita'.
X = larghezza (0 = fianco sinistro, visto dal fronte), Y = profondita' (0 = fronte), Z = alto (0 = faccia superiore della base).

Dal basso: base in multistrato, piedistallo (pannelli Dibond, alto PZ) con alimentatore, Kamoer, mainboard, display sul
fronte, ingresso dell'acqua, collettore con le sei valvole sdraiate, camere di livello, rubinetti; piano in multistrato;
sopra, il bagno in PMMA con la tank immersa a sinistra (testata sulla finestra della parete sinistra, motore nel vano
asciutto piu' a sinistra) e i tre contenitori da 500 ml in fila a destra; dietro i contenitori la striscia di servizio
(riscaldatori dalla parete destra, sonda, ricircolo); dietro la parete posteriore del bagno l'intercapedine con i
passaparete di servizio (WB, troppopieno, ingresso) e a destra quella dei riscaldatori."""
import math
import cadquery as cq
from common import *
from tank import box, xz_prism, yz_prism, xy_prism, ycyl, zcyl, xcyl
import tank as T, drive as D

# ---- scocca ----
WT = 3.0                            # pannelli in composito di alluminio (Dibond) da 3 mm
BASE_T = 12.0                       # base e piano in multistrato da 12 mm
PZ = 150.0                          # faccia superiore del piano del piedistallo (il fondo del bagno ci appoggia sopra)
DECK_Z0 = PZ - BASE_T               # faccia inferiore del piano
BATH_T = D.BATH_T                   # parete del bagno (PMMA 4 mm)
COVER_T = 3.0                       # coperchio del bagno (PMMA 3 mm)
GASKET_T = D.GASKET_T               # guarnizione della finestra della tank
# ---- tank (asse della spirale lungo X, testata a sinistra) ----
X_T, Y_T = 149.0, 132.5             # origine della tank in pianta
FLOOR = PZ + BATH_T                 # fondo interno del bagno (e pavimento del vano asciutto)
PAD_H = 4.0                         # la tank sta su piedini di 4 mm (sotto passano coperchietto e guarnizione del vano servo)
Z_T0 = FLOOR + PAD_H - Z_BOT        # quota macchina dello z = 0 della tank
AXIS_Z = Z_T0 + HC                  # asse della spirale
WATER_Z = Z_T0 + 82.0               # livello dell'acqua: 36 mm sopra la chimica a 350 ml, 12 sotto il bordo della finestra
# ---- bagno (interno) ----
BX0 = X_T - YO1 - GASKET_T          # faccia interna della parete sinistra (= faccia della guarnizione)
BY0 = WT + 2.0 + BATH_T             # interno davanti
BY1 = Y_T + XO1 + 13.0 + 7.25       # interno dietro (7 mm dietro la colonna del troppopieno della tank)
BZ1 = FLOOR + 119.0                 # bordo del bagno
WIN_TOP_Z = Z_T0 + T.WIN_TOP        # bordo superiore della parete sinistra (sotto la gonna del coperchio della tank)
# ---- contenitori da 500 ml (PP rettangolari 70 x 95 x 115, tappo a vite) ----
CW, CD, CH, CGAP = 70.0, 95.0, 115.0, 12.0
CX0 = X_T + 85.8 + CGAP + CW / 2    # il primo contenitore a 12 mm dalla punta del portagomma della tank
CX = [CX0 + k * (CW + CGAP) for k in range(3)]           # assi C1, C2, C3
CY = BY0 + 10.0 + CD / 2            # asse Y dei contenitori
BX1 = CX[-1] + CW / 2 + CGAP        # faccia interna della parete destra
RGAP = 26.0                         # intercapedine destra: teste e cavi dei riscaldatori
REAR_GAP = 20.0                     # intercapedine dietro il bagno: sensori di livello del bagno, cavi di sonda e pompetta (i passaparete di servizio sono nel fondo)
W = BX1 + BATH_T + RGAP + WT        # larghezza esterna
D_ = BY1 + BATH_T + REAR_GAP + WT   # profondita' esterna
H = BZ1 + COVER_T                   # altezza esterna sopra la base (senza base e piedini): coperchio del bagno a filo
# ---- striscia di servizio dietro i contenitori (nel bagno) ----
SY0, SY1 = CY + CD / 2 + 8.0, BY1
HEAT_Z = FLOOR + 18.0               # asse delle cartucce Ø16 x 100 (12 V 100 W), parallele a X, infilate dalla parete destra
HEAT_Y = (SY0 + 18.0, SY0 + 62.0)
HEAT_X = (BX1 - 100.0, BX1)         # tratto immerso
PROBE_XY = (CX[0] + 10.0, SY0 + 40.0)             # sonda DS18B20 del bagno (appesa alla parete posteriore)
CIRC_XY = (CX[0] + 60.0, SY0 + 30.0)              # pompetta di ricircolo 12 V (corpo 30 x 24 x 42, bocca verso +X: soffia lungo i riscaldatori)
SRV_Y = SY0 + 43.0                  # fila del troppopieno del bagno, tra i due riscaldatori
WB_X, WB_Y = CX[0] + 24.0, BY1 - 19.0    # passaparete WB nel fondo (pescante: la bocca e' il passaparete stesso, a filo del fondo), vicino alla parete posteriore
OVF_X = CX[1] + 16.0                # troppopieno del bagno: passaparete nel fondo con tubo verticale (bocca a filo del livello)
OVF_Z = WATER_Z + 2.0
IN_X, IN_Y = X_T + 75.0, BY1 - 15.0      # ingresso dell'acqua di rete: passaparete nel fondo a destra della tank, tubo verticale fino sopra l'acqua (salto d'aria)
IN_Z = BZ1 - 10.0                   # bocca del beccuccio
LVL_X = CX[2] - 30.0                # sensori XKC-Y21 MIN/MAX del bagno, incollati fuori dalla parete posteriore (intercapedine)
LVL_ZMIN, LVL_ZMAX = FLOOR + 28.0, WATER_Z - 10.0
CMAX_Z = WATER_Z + 10.0             # sensori MAX delle chimiche, incollati sulla parete posteriore dei contenitori, sopra l'acqua e sotto il tappo
# ---- tank: passaparete del pescante e del troppopieno nel fondo del bagno ----
TANK_BARB = (X_T + 85.8, Y_T + 23.8, Z_T0 + T.RIS_ZB)     # punta del portagomma orizzontale
TANK_BH = (X_T + 113.0, BY1 - 15.0)                       # passaparete della linea tank nel fondo del bagno (dietro C1, vicino alla parete posteriore)
TANK_OVF = (X_T - T.OVF_Y, Y_T + T.OVF_X)                  # asse della colonna del troppopieno (il silicone scende dritto)
# ---- piedistallo ----
PSU_L, PSU_W, PSU_H = 215.0, 115.0, 30.0                  # Mean Well LRS-350-12, in piedi sul fianco sinistro, lungo Y (morsetti verso il retro)
PSU_X0, PSU_Y0, PSU_Z0 = 16.0, 6.0, 6.0
MB_X0, MB_Y0, MB_Z0, MB_L, MB_H = 54.0, 20.0, 20.0, 110.0, 90.0   # mainboard in piedi (piano XZ) dietro il display, componenti verso +Y
DISP_X, DISP_Z = 124.0, 78.0        # centro del display 4.3" sul pannello frontale (a destra dell'alimentatore in piedi)
DISP_WIN = (98.0, 58.0)             # finestra nel pannello (area attiva 95 x 54 + 1,5 per lato)
PUMP_X0, PUMP_Y0, PUMP_Z0 = 110.0, 125.0, 6.0             # Kamoer KPHM600 (corpo 90 x 60 x 56) sulla base, testa verso il fronte
PUMP_L, PUMP_W, PUMP_HH = 90.0, 60.0, 56.0
PUMP_PORTS = ((PUMP_X0 + 20.0, PUMP_Y0 - 20.0, PUMP_Z0 + 34.0), (PUMP_X0 + 70.0, PUMP_Y0 - 20.0, PUMP_Z0 + 34.0))   # bocche (portagomma 6,4) verso -Y
# ---- collettore e valvole (sdraiate, asse Y, bobina in alto), davanti all'intercapedine ----
PIPE_R = 4.76                       # tubo 3/8" (9.52 mm)
ELB_R, ELB_L = 8.5, 28.0            # gomito a innesto rapido 3/8": raggio del corpo, lunghezza di ogni braccio dall'angolo
INS = 15.0                          # inserimento del tubo nei raccordi a innesto
VALVE_LEN, VALVE_BODY, VALVE_COIL = 82.0, 33.0, 33.5
VALVE_X = [CX[0], CX[1], CX[2], CX[0] - 36.0, CX[1] - 36.0, CX[2] - 36.0]     # C1, C2, C3, WB, SCARICO, RECUPERO (3 mm tra i corpi)
VALVE_NAMES = ("C1", "C2", "C3", "WB", "SCARICO", "RECUPERO")
VALVE_Z = 27.0                      # asse delle valvole (corpo 10,5..43,5; bobina sopra fino a 77)
MAN_Y0, MAN_Y1 = BY1 - 22.0, BY1 + 4.0                   # collettore: barra lungo X
MAN_X0, MAN_X1 = VALVE_X[3] - 22.0, VALVE_X[2] + 22.0
MAN_Z0, MAN_Z1 = VALVE_Z - 11.0, VALVE_Z + 11.0
VALVE_Y1 = MAN_Y0 - 4.0             # bocca posteriore delle valvole (spezzone di 14 + 4 mm nel collettore)
VALVE_Y0 = VALVE_Y1 - VALVE_LEN     # bocca anteriore
ELB_Y = VALVE_Y0 - ELB_L            # angolo del gomito davanti alla valvola (= CY per le chimiche)
MAN_OUT = (MAN_X0, VALVE_Z)         # uscita del collettore verso la pompa: sulla faccia -X (bocca orizzontale), gomito a codolo girato verso il fronte
MAN_OUT_Y = MAN_Y0 + 18.0           # il canale interno del collettore corre a y = MAN_Y0 + 18
CH_Z0, CH_Z1 = VALVE_Z + ELB_L + 1.0, VALVE_Z + ELB_L + 1.0 + 57.0   # camera di livello verticale sopra il gomito
CH_SIDE_Z = CH_Z0 + 28.5            # bocca laterale della camera (verso +X) per la linea del rubinetto
CH_R = 13.0
# ---- passaparete dei contenitori (attraverso fondo del contenitore + fondo del bagno + piano) e rubinetti ----
BULK_R = 8.5                        # passaparete 3/8" BSP (foro Ø17): dado sotto il piano, raccordo a innesto 3/8" in basso
BULK_Z0 = DECK_Z0 - 8.0 - 15.0      # estremita' inferiore del raccordo sotto il passaparete
TAP_Z = 45.0                        # rubinetti a margherita sul pannello frontale
TAP_X = [c + 25.0 for c in CX]      # a destra dell'asse di ogni contenitore (la camera ha la bocca laterale verso +X)
TAP_LANE_Y = 42.0                   # i tubi in silicone dei rubinetti corrono a y = 42
CH_SIDE_X = 28.0                    # estremita' dell'adattatore innesto 3/8" - portagomma 10 sulla bocca laterale (+X) della camera (dall'asse)
REC_X, REC_Z = W - 22.0, VALVE_Z    # uscita RECUPERO (portagomma 10 mm) sul pannello frontale, all'altezza delle valvole
REC_LANE_Y = 52.0                   # il silicone del recupero corre a y = 52 (davanti ai gomiti delle valvole)
# ---- ingresso dell'acqua di rete (nel piedistallo, a sinistra, dietro la mainboard) ----
FS_X, FS_Z, FS_Y0, FS_Y1 = 70.0, 52.0, 120.0, 185.0      # flussimetro YF-S201 lungo Y (corpo 65), raccordi 1/2" F - 3/8" a innesto
FS_AD = 20.0
IV_X0, IV_Y, IV_Z = 103.0, 70.0, 52.0                    # valvola dell'ingresso sdraiata lungo X, bobina in alto
IN_RISER_X = IN_X                                        # dalla valvola: dritto fino a x = IN_X, poi lungo Y (a sinistra della valvola WB) fino sotto il passaparete, poi su
WB_LANE_Z, LINE_Z = 85.0, 85.0                           # corsia delle linee WB, SCARICO e troppopieno del bagno (sopra le bobine delle valvole): 30 mm dalle bocche dei gomiti delle valvole (z = 55) e dei raccordi dei passaparete (z = 115)
DECK_GROMMET_R = 10.0                                    # nel piano il foro del gommino del troppopieno della tank e' piu' largo: il gommino fa tenuta solo sul fondo in PMMA
# ---- attacchi sul pannello posteriore (x, z) e presa ----
IEC_X, IEC_Z = (478.0, 505.5), (30.0, 77.5)              # presa IEC C14 con interruttore e fusibile nel pannello posteriore, a destra
CONN = {"SCARICO": (VALVE_X[4], LINE_Z), "TROPPOPIENO_TANK": (160.0, 55.0), "TROPPOPIENO_BAGNO": (OVF_X, LINE_Z), "INGRESSO": (FS_X, FS_Z)}
# ---- fori nel piano (e nel fondo del bagno, per i primi otto) ----
DECK_CUTS = {"C1": (CX[0], CY, BULK_R), "C2": (CX[1], CY, BULK_R), "C3": (CX[2], CY, BULK_R), "tank": (TANK_BH[0], TANK_BH[1], BULK_R),
             "troppopieno_tank": (TANK_OVF[0], TANK_OVF[1], 6.5), "WB": (WB_X, WB_Y, BULK_R), "troppopieno_bagno": (OVF_X, SRV_Y, BULK_R),
             "ingresso": (IN_X, IN_Y, BULK_R), "cavi_vano_motore": (40.0, 110.0, 11.0), "cavi_riscaldatori": (W - WT - RGAP / 2, 120.0, 8.0)}
BATH_CUTS = ("C1", "C2", "C3", "tank", "troppopieno_tank", "WB", "troppopieno_bagno", "ingresso")
# ---- tubi ----
BEND_R = 45.0                       # raggio delle curve del tubo 3/8" posato libero
HOSE_R = 6.0                        # tubo in silicone 8 x 12

def to_machine(p):
    """Dal riferimento della tank a quello della macchina: rotazione di 90 gradi attorno a Z + traslazione."""
    return p.rotate((0, 0, 0), (0, 0, 1), 90).translate((X_T, Y_T, Z_T0))

def tank_xy(xt, yt):
    return X_T - yt, Y_T + xt

def seg(p0, p1, r):
    """Cilindro tra due punti allineati a un asse."""
    (x0, y0, z0), (x1, y1, z1) = p0, p1
    if abs(x1 - x0) > 1e-6: return xcyl(y0, z0, r, min(x0, x1), max(x0, x1))
    if abs(y1 - y0) > 1e-6: return ycyl(x0, z0, r, min(y0, y1), max(y0, y1))
    return zcyl(x0, y0, r, min(z0, z1), max(z0, z1))

def route(pts, r, knee=None):
    """Tubo lungo una spezzata a tratti paralleli agli assi (percorso indicativo)."""
    t, n = None, len(pts) - 1
    for i, (a, b) in enumerate(zip(pts[:-1], pts[1:])):
        L = math.dist(a, b); d = [(q - p) / L for p, q in zip(a, b)]
        rr = r - 0.05 * (i % 2)
        a2 = a if i == 0 else tuple(p - rr * c for p, c in zip(a, d))
        b2 = b if i == n - 1 else tuple(p + rr * c for p, c in zip(b, d))
        s = seg(a2, b2, rr)
        t = s if t is None else t.union(s)
    assert len(t.val().Solids()) == 1 and t.val().isValid(), "route: unione non riuscita"
    return t

def route_len(pts):
    return sum(math.dist(a, b) for a, b in zip(pts[:-1], pts[1:]))

def stem_elbow(p, d_stem, d_arm, stem_len):
    """Gomito a codolo 3/8": un braccio a innesto (lungo ELB_L dall'angolo p, direzione d_arm) e un codolo liscio da
    9.52 mm lungo stem_len dall'angolo (direzione d_stem)."""
    a = tuple(q - ELB_R * c for q, c in zip(p, d_arm)); e = tuple(q + ELB_L * c for q, c in zip(p, d_arm))
    s = tuple(q + stem_len * c for q, c in zip(p, d_stem))
    b = seg(a, e, ELB_R).union(seg(p, s, PIPE_R))
    assert len(b.val().Solids()) == 1 and b.val().isValid(), "stem_elbow: unione non riuscita"
    return b

def elbow(p, d1, d2):
    """Gomito a innesto rapido 3/8": due bracci di lunghezza ELB_L dall'angolo p lungo le direzioni d1 e d2."""
    a = tuple(q - ELB_R * c for q, c in zip(p, d1))
    e1 = tuple(q + ELB_L * c for q, c in zip(p, d1))
    e2 = tuple(q + ELB_L * c for q, c in zip(p, d2))
    e = seg(a, e1, ELB_R).union(seg(p, e2, ELB_R - 0.05))
    assert len(e.val().Solids()) == 1 and e.val().isValid(), "elbow: unione non riuscita"
    return e

def elbows(pts):
    """Gomiti a innesto a tutti gli angoli interni di una spezzata a tratti paralleli agli assi."""
    e = None
    for a, p, b in zip(pts[:-2], pts[1:-1], pts[2:]):
        d1 = tuple((q - r) / math.dist(a, p) for q, r in zip(a, p)); d2 = tuple((q - r) / math.dist(b, p) for q, r in zip(b, p))
        s = elbow(p, d1, d2)
        e = s if e is None else e.union(s)
    return e

def tubes(pts, r=None):
    """Spezzoni di tubo tra i bracci dei gomiti di una spezzata (ogni tratto accorciato di ELB_L a ogni angolo)."""
    r = PIPE_R if r is None else r
    t = None; n = len(pts) - 1
    for i, (a, b) in enumerate(zip(pts[:-1], pts[1:])):
        L = math.dist(a, b); d = [(q - p) / L for p, q in zip(a, b)]
        a2 = a if i == 0 else tuple(p + ELB_L * c for p, c in zip(a, d))
        b2 = b if i == n - 1 else tuple(p - ELB_L * c for p, c in zip(b, d))
        if math.dist(a2, b2) < 1.0: continue
        s = seg(a2, b2, r - 0.05 * (i % 2))
        t = s if t is None else t.union(s)
    return t

def tube_lens(pts):
    """Lunghezze dei tratti di tubo: i punti estremi sono le bocche dei raccordi (il tubo ci entra per 15 mm), i punti
    intermedi gli angoli dei gomiti (braccio di 28 mm, di cui 15 di innesto)."""
    out = []; n = len(pts) - 1
    for i, (a, b) in enumerate(zip(pts[:-1], pts[1:])):
        L = math.dist(a, b) + (INS if i == 0 else -(ELB_L - INS)) + (INS if i == n - 1 else -(ELB_L - INS))
        out.append(L)
    return out

if __name__ == "__main__":
    print(f"ingombro {W:.0f} x {D_:.0f} x {H:.0f} (+ base {BASE_T:.0f}) | bagno interno X {BX0:.1f}..{BX1:.1f} Y {BY0:.1f}..{BY1:.1f} fondo {FLOOR:.0f} bordo {BZ1:.0f} acqua {WATER_Z:.0f}")
    print(f"tank: testata x {X_T - YO1:.2f}, pilastro x {X_T - T.PIL_Y0:.2f}, portagomma {tuple(round(v, 1) for v in TANK_BARB)}, y {Y_T + XO0:.1f}..{Y_T + XO1 + 13:.1f}, asse z {AXIS_Z:.0f}, bordo {Z_T0 + Z_RIM:.0f}")
    print(f"motore: faccia x {X_T - D.MOT_FACE:.1f}, coda x {X_T - D.MOT_FACE - 58:.1f} | vano asciutto x {WT:.0f}..{BX0 - BATH_T:.2f}")
    print(f"contenitori x {[round(c) for c in CX]} y {CY - CD / 2:.1f}..{CY + CD / 2:.1f} | striscia di servizio y {SY0:.0f}..{SY1:.0f} | riscaldatori y {HEAT_Y} z {HEAT_Z:.0f} | passaparete: WB ({WB_X:.0f}, {WB_Y:.0f}) troppopieno ({OVF_X:.0f}, {SRV_Y:.0f}) ingresso ({IN_X:.0f}, {IN_Y:.0f}) linea tank {TANK_BH}")
    print(f"valvole x {[round(v) for v in VALVE_X]} y {VALVE_Y0:.0f}..{VALVE_Y1:.0f} z {VALVE_Z:.0f} | camere z {CH_Z0:.0f}..{CH_Z1:.0f} | raccordo passaparete giu' fino a z {BULK_Z0:.0f} | collettore x {MAN_X0:.0f}..{MAN_X1:.0f} y {MAN_Y0:.0f}..{MAN_Y1:.0f}")
