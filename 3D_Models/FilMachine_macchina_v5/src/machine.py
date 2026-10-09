"""Macchina completa: quote della disposizione generale (riferimento macchina) e utilita'.
X = larghezza (0 = fianco sinistro, visto dal fronte), Y = profondita' (0 = fronte), Z = alto (0 = piano della base)."""
import math
import cadquery as cq
from common import *
from tank import box, xz_prism, yz_prism, xy_prism, ycyl, zcyl, xcyl

# ---- base ----
BASE_X, BASE_Y, BASE_T = 420.0, 505.0, 12.0
# ---- tank su cavalletto (fronte destra) ----
X_T, Y_T = 302.0, 130.0             # origine della tank in pianta (asse verticale per il centro della spirale)
Z_STAND = 118.0                     # piano d'appoggio della tank
Z_T0 = Z_STAND - Z_BOT              # quota macchina dello z = 0 della tank
DECK_T = 5.0                        # spessore del piano del cavalletto
ST_X0, ST_X1, ST_Y0, ST_Y1 = 198.0, 402.0, 8.0, 197.0
ST_PANEL = 5.0
# ---- bagno: contenitore Euro 400 x 300 x 170 ----
TUB_X0, TUB_Y0, TUB_L, TUB_W, TUB_H = 4.0, 200.0, 400.0, 300.0, 170.0
TUB_IN, TUB_WALL, TUB_FLOOR = 18.0, 3.0, 5.0      # rientro della parete rispetto al contorno, spessori
TUB_BAND_T, TUB_BAND_B = 20.0, 12.0               # fascia di rinforzo in alto e zoccolo in basso
IX0, IX1 = TUB_X0 + TUB_IN, TUB_X0 + TUB_L - TUB_IN      # interno: 22 .. 386
IY0, IY1 = TUB_Y0 + TUB_IN, TUB_Y0 + TUB_W - TUB_IN      # interno: 218 .. 482
RIM_Z = TUB_H                                            # bordo del contenitore
# ---- longheroni (tubo quadro di alluminio 15 x 15 x 1.5) e vaschette GN 1/9 h 150 ----
RAIL, RAIL_T = 15.0, 1.5
RAIL_R_Y0 = IY1 - 3.0 - RAIL        # longherone posteriore: 464 .. 479
GN9_L, GN9_W, GN9_H, GN9_RIM, GN9_RIM_T, GN9_WALL, GN9_DRAFT = 108.0, 176.0, 150.0, 11.0, 3.0, 1.5, 7.0
VAS_Y1 = RAIL_R_Y0 + RAIL - 5.0     # bordo posteriore delle vaschette: 474
VAS_Y0 = VAS_Y1 - GN9_W             # bordo anteriore: 298
RAIL_F_Y0 = VAS_Y0 - 9.0            # longherone anteriore: 289 .. 304 (5 mm liberi davanti al corpo della vaschetta, per sfilarla)
RAIL_X0, RAIL_X1 = IX0 + 4.0, IX1 - 4.0
VAS_X = [(IX0 + IX1) / 2 + GN9_L * k for k in (-1, 0, 1)]     # assi delle vaschette C1, C2, C3: 96, 204, 312
VAS_ZR = RIM_Z + GN9_RIM_T          # sommita' del bordo delle vaschette
VAS_ZB = VAS_ZR - GN9_H             # fondo esterno delle vaschette
WATER_Z = VAS_ZB + 90.0             # livello di lavoro del bagno (90 mm di vaschetta immersi)
# ---- testina (pescante) e innesto ----
PIPE_R = 4.76                       # tubo 3/8" (9.52 mm)
ELB_R, ELB_L = 8.5, 28.0            # gomito a innesto rapido 3/8": raggio del corpo, lunghezza di ogni braccio dall'angolo
INS = 15.0                          # inserimento del tubo nei raccordi a innesto
Y_DIP = VAS_Y0 + GN9_RIM + GN9_WALL + GN9_DRAFT + PIPE_R + 1.74   # asse del pescante: libero dalla parete inclinata fino in fondo (324)
Y_STUB = Y_DIP - 2 * ELB_L - 2.0    # asse del codolo che scende nell'innesto: 262
Z_CROSS = VAS_ZR + 3.0 + ELB_R + 2.0                  # asse del tratto orizzontale della testina
U_R = 25.0                          # raggio della curva del sifone in silicone 8x12 (nel piano XZ, verso sinistra)
Z_MOUTH = 150.0                     # bocca del sifone: il codolo della testina ci entra di 5 mm
STUB_Z0 = Z_MOUTH                   # estremita' del codolo come disegnata (in realta' 5 mm piu' in basso, dentro il silicone)
LP_Z0 = 124.0                       # bordo inferiore delle staffe sul longherone anteriore
HOSE_R = 6.0                        # tubo in silicone 8 x 12
ORING_ID, ORING_CS = 9.0, 2.0       # O-ring 9 x 2 (codoli da 9.52)
# ---- gruppo valvole e collettore (a sinistra del cavalletto, davanti al bagno) ----
VALVE_X = [22.0, 92.0, 127.0, 57.0, 162.0]            # C1, C2, C3, WB, scarico; da sinistra stanno C1, WB, C2, C3, scarico: cosi' i tubi verso il bagno non si incrociano
VALVE_Y = 177.5
MAN_Z1 = 22.0                       # sommita' del collettore
VALVE_Z0 = MAN_Z1 + 4.0             # estremita' dell'attacco inferiore delle valvole
VALVE_LEN, VALVE_BODY, VALVE_COIL = 82.0, 33.0, 33.5
VALVE_ZC, VALVE_Z1 = VALVE_Z0 + VALVE_LEN / 2, VALVE_Z0 + VALVE_LEN
MAN_OUT = (188.0, 163.0)            # uscita del collettore verso la pompa
# ---- pompa (sotto il cavalletto, testa verso il retro) ----
PUMP_XC, PUMP_Y0, PUMP_L, PUMP_W, PUMP_H, PUMP_FOOT = 300.0, 13.0, 177.0, 114.0, 88.0, 4.0
PUMP_PORT_Y, PUMP_PORT_Z = PUMP_Y0 + 150.0, PUMP_FOOT + 41.0
# ---- tubi ----
BEND_R = 45.0                       # raggio delle curve del tubo 3/8" posato libero (non scendere sotto 40 mm)
Z_BEND = 165.0                      # quota a cui i tubi che salgono dalle valvole cominciano a curvare verso il bagno
Z_LANE_WASTE = 136.0                # quota del tubo dello scarico (gomito sopra la valvola, poi nella rientranza del contenitore)
Y_RECESS = TUB_Y0 + 7.5             # corsia nella rientranza del contenitore (tubo dello scarico)
WB_X = 127.0                        # pescante dell'acqua del bagno, tra le culle dei sifoni di C1 e C2
WB_ZIN = 42.0                       # bocca del pescante WB (piu' alta dei riscaldatori)
# ---- riscaldatori (parete destra del bagno) e sensori di livello (parete anteriore) ----
HEAT_Y, HEAT_Z = (256.0, 282.0), 25.0
LVL_X, LVL_ZMIN, LVL_ZMAX = 330.0, 62.0, 120.0
# ---- elettronica (fronte sinistra) ----
EL_X1, EL_Y1, EL_Z1 = 196.0, 124.0, 226.0
EL_T = 3.0                          # spessore dei pannelli
SLOPE_Y, SLOPE_Z0 = 60.0, 166.0     # plancia inclinata a 45 gradi: da (Y = 0, Z = 166) a (Y = 60, Z = 226)
SLOPE_ANG = math.degrees(math.atan2(EL_Z1 - SLOPE_Z0, SLOPE_Y))
PSU_L, PSU_W, PSU_H = 215.0, 115.0, 50.0

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
    """Tubo lungo una spezzata a tratti paralleli agli assi (percorso indicativo). A ogni gomito i tratti proseguono di
    un raggio oltre l'angolo; tratti alterni hanno raggio diverso di 0.05 mm, cosi' le unioni sono sempre regolari."""
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

def bent_len(D, R):
    """Lunghezza della parte curva + orizzontale di un tubo a ponte tra due tratti verticali distanti D in pianta."""
    R = min(R, D / 2)
    return math.pi * R + (D - 2 * R)

def bent_tube(p0, p1, zb, R, r=None):
    """Tubo posato a ponte: sale in verticale da p0 fino a zb, curva di 90 gradi (raggio R), tratto orizzontale verso p1,
    seconda curva di 90 gradi e discesa in verticale fino a p1. Se i due punti distano meno di 2R in pianta le due
    curve diventano un mezzo cerchio di raggio D/2."""
    r = PIPE_R if r is None else r
    (x0, y0, z0), (x1, y1, z1) = p0, p1
    D = math.hypot(x1 - x0, y1 - y0); R = min(R, D / 2); c = math.sqrt(0.5)
    w = cq.Workplane("XZ").moveTo(0, z0).lineTo(0, zb).threePointArc((R - R * c, zb + R * c), (R, zb + R))
    if D - 2 * R > 1e-6: w = w.lineTo(D - R, zb + R)
    w = w.threePointArc((D - R + R * c, zb + R * c), (D, zb)).lineTo(D, z1)
    t = cq.Workplane("XY").workplane(offset=z0).circle(r).sweep(w)
    t = t.rotate((0, 0, 0), (0, 0, 1), math.degrees(math.atan2(y1 - y0, x1 - x0))).translate((x0, y0, 0))
    assert len(t.val().Solids()) == 1 and t.val().isValid(), "bent_tube: solido non valido"
    return t

def stem_elbow(p, d_stem, d_arm, stem_len):
    """Gomito a codolo 3/8": un braccio a innesto (lungo ELB_L dall'angolo p, direzione d_arm) e un codolo liscio da
    9.52 mm lungo stem_len dall'angolo (direzione d_stem), che entra in un altro innesto o in una sede con O-ring."""
    a = tuple(q - ELB_R * c for q, c in zip(p, d_arm)); e = tuple(q + ELB_L * c for q, c in zip(p, d_arm))
    s = tuple(q + stem_len * c for q, c in zip(p, d_stem))
    b = seg(a, e, ELB_R).union(seg(p, s, PIPE_R))
    assert len(b.val().Solids()) == 1 and b.val().isValid(), "stem_elbow: unione non riuscita"
    return b

def elbow(p, d1, d2):
    """Gomito a innesto rapido 3/8": due bracci di lunghezza ELB_L dall'angolo p lungo le direzioni d1 e d2.
    Il primo braccio prosegue di un raggio oltre l'angolo e il secondo e' di un soffio piu' sottile (unione regolare)."""
    a = tuple(q - ELB_R * c for q, c in zip(p, d1))
    e1 = tuple(q + ELB_L * c for q, c in zip(p, d1))
    e2 = tuple(q + ELB_L * c for q, c in zip(p, d2))
    e = seg(a, e1, ELB_R).union(seg(p, e2, ELB_R - 0.05))
    assert len(e.val().Solids()) == 1 and e.val().isValid(), "elbow: unione non riuscita"
    return e

if __name__ == "__main__":
    print("tank: X", X_T - 96.75, X_T + 73.25, "| Y", Y_T - 120.4, Y_T + 65.25, "| asse spirale Z", Z_T0 + HC, "| coperchio Z", Z_T0 + Z_RIM + 3)
    print("bagno: interno X", IX0, IX1, "Y", IY0, IY1, "| longheroni Y", RAIL_F_Y0, RAIL_R_Y0, "| vaschette X", VAS_X, "Y", VAS_Y0, VAS_Y1,
          "bordo Z", VAS_ZR, "fondo Z", VAS_ZB, "| livello acqua Z", WATER_Z)
    print("testina: pescante Y", Y_DIP, "codolo Y", Y_STUB, "asse orizzontale Z", Z_CROSS, "| curve dei tubi da Z", Z_BEND, "raggio", BEND_R)
    print("valvole X", VALVE_X, "Z", VALVE_Z0, VALVE_Z1, "| pompa attacchi Y", PUMP_PORT_Y, "Z", PUMP_PORT_Z, "| plancia", round(SLOPE_ANG, 1), "gradi")
