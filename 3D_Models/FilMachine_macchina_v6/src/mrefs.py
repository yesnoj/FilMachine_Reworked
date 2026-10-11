"""Sagome dei componenti acquistati della macchina (REF_), nel riferimento macchina. Dimensioni indicative dove
il componente non e' normalizzato: servono per gli ingombri, per l'assieme e per la nuova distinta.
v6: bagno Euro h 220, vaschette a filo del coperchio con innesto dal basso, linee a ponte sopra il bordo, camere di
livello sopra le valvole, ingresso dell'acqua di rete con valvola e flussimetro, troppopieno del bagno, tank a filo."""
import math
import cadquery as cq
from common import loft_planar
from machine import *
import tank as T

def base_board():
    """Tavola di base in multistrato 420 x 505 x 12."""
    return box(0, BASE_X, 0, BASE_Y, -BASE_T, 0)

def tub():
    """Contenitore Euro 400 x 300 x 220 a pareti e fondo chiusi (bagno termostatico): due fori Ø16.5 per i
    riscaldatori, un foro Ø22 per il passaparete del troppopieno e un foro Ø21.5 per il rubinetto di svuotamento,
    tutti nella parete corta destra."""
    x0, x1, y0, y1 = TUB_X0, TUB_X0 + TUB_L, TUB_Y0, TUB_Y0 + TUB_W
    t = box(IX0 - TUB_WALL, IX1 + TUB_WALL, IY0 - TUB_WALL, IY1 + TUB_WALL, 0, TUB_H)
    t = t.union(box(x0, x1, y0, y1, TUB_H - TUB_BAND_T, TUB_H)).union(box(x0, x1, y0, y1, 0, TUB_BAND_B))
    t = t.cut(box(IX0, IX1, IY0, IY1, TUB_FLOOR, TUB_H + 1))
    for y in HEAT_Y:
        t = t.cut(xcyl(y, HEAT_Z, 8.25, IX1 - 1, IX1 + TUB_WALL + 1))
    t = t.cut(xcyl(BD_Y, BD_Z, 10.75, IX1 - 1, IX1 + TUB_WALL + 1))
    return t.cut(xcyl(OVF_BATH_Y, OVF_BATH_Z, 11.0, IX1 - 1, IX1 + TUB_WALL + 1))

def rail(front=True):
    """Longherone: tubo quadro di alluminio 15 x 15 x 1.5, rialzato sopra il bordo del contenitore dalle testate C05."""
    y0 = RAIL_F_Y0 if front else RAIL_R_Y0
    r = box(RAIL_X0, RAIL_X1, y0, y0 + RAIL, RAIL_Z1 - RAIL, RAIL_Z1)
    return r.cut(box(RAIL_X0 - 1, RAIL_X1 + 1, y0 + RAIL_T, y0 + RAIL - RAIL_T, RAIL_Z1 - RAIL + RAIL_T, RAIL_Z1 - RAIL_T))

def _pan(x0, y0, L, W, z0, z1, draft, wall, rim_w, rim_t):
    def ring(z, off):
        return [(x0 + off, y0 + off, z), (x0 + L - off, y0 + off, z), (x0 + L - off, y0 + W - off, z), (x0 + off, y0 + W - off, z)]
    outer = cq.Workplane("XY").add(loft_planar([ring(z0, rim_w + draft), ring(z1 - rim_t, rim_w)]))
    inner = cq.Workplane("XY").add(loft_planar([ring(z0 + wall, rim_w + draft + wall), ring(z1 + 0.01, rim_w + wall)]))
    return outer.union(box(x0, x0 + L, y0, y0 + W, z1 - rim_t, z1)).cut(inner)

PAN_HOLE_R = 8.2                     # foro Ø16.4 al centro del fondo (filetto M16 dell'innesto)
def vaschetta(k):
    """Vaschetta Gastronorm GN 1/9 h 150 in polipropilene (176 x 108, 1.5 litri) con il foro dell'innesto nel fondo."""
    v = _pan(VAS_X[k] - GN9_L / 2, VAS_Y0, GN9_L, GN9_W, VAS_ZB, VAS_ZR, GN9_DRAFT, GN9_WALL, GN9_RIM, GN9_RIM_T)
    return v.cut(zcyl(VAS_X[k], Y_SOCK, PAN_HOLE_R, VAS_ZB - 1, VAS_ZB + GN9_WALL + 1))

def pan_wall_y(z):
    """Ordinata della faccia esterna della parete anteriore della vaschetta alla quota z."""
    return VAS_Y0 + GN9_RIM + GN9_DRAFT * (VAS_ZR - GN9_RIM_T - z) / (GN9_H - GN9_RIM_T)

def gasket(k):
    """Guarnizione piana in EPDM 16.5 x 26 x 2 sotto il fondo della vaschetta (stretta dalla ghiera C12)."""
    return zcyl(VAS_X[k], Y_SOCK, 13.0, VAS_ZB - 2.0, VAS_ZB).cut(zcyl(VAS_X[k], Y_SOCK, 8.25, VAS_ZB - 2.1, VAS_ZB + 0.1))

# ---- valvole, pompa ----
def valve(k, x=None, y=None, z0=None, axis="Z", coil=(0, -1, 0)):
    """Elettrovalvola 12 V NC con innesti rapidi 3/8" (82 mm tra le bocche, corpo 33, bobina di lato).
    Di serie in verticale sul collettore, bobina verso il fronte; con axis='X' e' sdraiata (valvola dell'ingresso)."""
    h = VALVE_BODY / 2
    if axis == "Z":
        x = VALVE_X[k]
        v = zcyl(x, VALVE_Y, ELB_R, VALVE_Z0, VALVE_Z1)
        v = v.union(box(x - h, x + h, VALVE_Y - h, VALVE_Y + h, VALVE_ZC - 15.0, VALVE_ZC + 15.0))
        return v.union(box(x - 15.0, x + 15.0, VALVE_Y - h - VALVE_COIL, VALVE_Y - h, VALVE_ZC - 14.0, VALVE_ZC + 14.0))
    xc = x + VALVE_LEN / 2
    v = xcyl(y, z0, ELB_R, x, x + VALVE_LEN)
    v = v.union(box(xc - 15.0, xc + 15.0, y - h, y + h, z0 - h, z0 + h))
    yc0, yc1 = (y + h, y + h + VALVE_COIL) if coil[1] > 0 else (y - h - VALVE_COIL, y - h)
    return v.union(box(xc - 14.0, xc + 14.0, yc0, yc1, z0 - 14.0, z0 + 14.0))

def valve_stub(k):
    """Spezzone di tubo tra collettore e valvola (inserito 14 mm nel collettore)."""
    return seg((VALVE_X[k], VALVE_Y, MAN_Z1 - 14.0), (VALVE_X[k], VALVE_Y, VALVE_Z0), PIPE_R)

def chamber_stub(k):
    """Spezzone di tubo tra la bocca superiore della valvola Ck e la camera di livello (15 mm nella valvola, 14 nella camera)."""
    return seg((VALVE_X[k], VALVE_Y, VALVE_Z1), (VALVE_X[k], VALVE_Y, CH_Z0 + 14.0), PIPE_R)

def pump():
    """Pompa a ingranaggi reversibile Marco UP3-R 12 V (177 x 114 x 88), con i due raccordi 3/8" BSP maschio - tubo 3/8"."""
    y0 = PUMP_Y0; zf = PUMP_FOOT
    p = ycyl(PUMP_XC, zf + 46.0, 38.0, y0, y0 + 122.0)                                             # motore
    p = p.union(box(PUMP_XC - 40.0, PUMP_XC + 40.0, y0 + 120.0, y0 + PUMP_L, zf + 6.0, zf + 84.0))   # testa della pompa
    p = p.union(box(PUMP_XC - PUMP_W / 2, PUMP_XC + PUMP_W / 2, y0 + 30.0, y0 + 110.0, zf, zf + 10.0))  # piede
    return p.union(xcyl(PUMP_PORT_Y, PUMP_PORT_Z, 9.0, PUMP_XC - 74.0, PUMP_XC + 74.0))             # attacchi con raccordi

PUMP_PL, PUMP_PR = PUMP_XC - 74.0, PUMP_XC + 74.0        # bocche dei due raccordi della pompa
# ---- tank: salita esterna al cavalletto e silicone dal portagomma orizzontale ----
X_RISE, Y_RISE = tank_xy(*T.RISER_T)                     # 411, 163: tra il fianco destro del cavalletto e la scocca
BARB_Z = Z_T0 + T.RIS_ZB                                 # asse del portagomma della tank
BARB_TIP = tank_xy(*T.BARB_TIP)
Z_HOSE0 = BARB_Z - T.HOSE_RB                             # da qui in su il silicone curva verso il portagomma
OVF_XM, OVF_YM = X_T - 27.0, Y_T + 58.75                 # asse del troppopieno della tank
SOCK_Z = Z_STAND - DECK_T - 9.0                          # asse dell'innesto del troppopieno sotto il piano del cavalletto
Z_OVF_HOSE, Y_OVF_HOSE = 100.0, 200.0                    # uscita TROPPOPIENO TANK sulla piastra degli scarichi
OVF_END = (BASE_X + WT + 6.0, Y_OVF_HOSE, Z_OVF_HOSE)
OVF_YAW = math.degrees(math.atan2(Y_OVF_HOSE - OVF_YM, OVF_END[0] - OVF_XM))
OVF_ANG = math.degrees(math.atan2(SOCK_Z - Z_OVF_HOSE, math.hypot(OVF_END[0] - OVF_XM, Y_OVF_HOSE - OVF_YM)))  # pendenza (sempre in discesa)
WASTE_J = (253.0, Y_RECESS)                              # punto in cui il tubo dello scarico entra nella rientranza del contenitore
WASTE_ANG = math.degrees(math.atan2(WASTE_J[1] - VALVE_Y, WASTE_J[0] - VALVE_X[4]))
P_MAN = (MAN_OUT[0], MAN_OUT[1], PUMP_PORT_Z)            # gomito a codolo sull'uscita del collettore
P_RISE = (X_RISE, PUMP_PORT_Y, PUMP_PORT_Z)              # gomito ai piedi della salita verso la tank
# ---- piastre degli attacchi sul fianco destro ----
PL_X0 = BASE_X - 4.0                                     # faccia interna delle piastre (avvitate al pannello destro)
CONN = {"SCARICO": (Y_RECESS, Z_LANE_WASTE), "TROPPOPIENO_TANK": (Y_OVF_HOSE, Z_OVF_HOSE), "TROPPOPIENO_BAGNO": (232.0, 70.0),
        "INGRESSO": (60.0, 113.0)}

# ---- linee delle vaschette: camera -> ponte sopra il bordo -> discesa nella fascia anteriore -> fondo -> presa ----
def line_ends(k):
    return (VALVE_X[k], VALVE_Y, CH_Z1 - 14.0), (VAS_X[k], Y_DESC, LINE_Z + BEND_R)

def floor_bend(x, y0, r=None):
    """Curva di raggio BEND_R dal tratto verticale (in y0) al tratto sul fondo verso +Y, nel piano X = x."""
    r = PIPE_R if r is None else r
    c = math.sqrt(0.5); z0 = LINE_Z + BEND_R
    w = cq.Workplane("YZ", origin=(x, 0, 0)).moveTo(y0, z0 + 0.5).lineTo(y0, z0).threePointArc(
        (y0 + BEND_R - BEND_R * c, z0 - BEND_R * c), (y0 + BEND_R, LINE_Z))
    return cq.Workplane("XY").workplane(offset=z0 + 0.5).center(x, y0).circle(r).sweep(w)

def line(k):
    """Tubo 3/8" della vaschetta k, un pezzo solo: esce dalla camera di livello, passa a ponte sopra il bordo del bagno
    (curve di raggio BEND_R), scende nella fascia anteriore, curva sul fondo e corre fino alla presa sotto la vaschetta."""
    p0, p1 = line_ends(k)
    t = bent_tube(p0, p1, Z_BEND, BEND_R)
    t = t.union(floor_bend(VAS_X[k], Y_DESC, PIPE_R - 0.03))
    t = t.union(ycyl(VAS_X[k], LINE_Z, PIPE_R - 0.06, Y_DESC + BEND_R - 0.5, SK_Y0 + 14.0))
    assert len(t.val().Solids()) == 1 and t.val().isValid(), "linea: unione non riuscita"
    return t

def line_len(k):
    p0, p1 = line_ends(k); D = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
    return 14.0 + (Z_BEND - p0[2]) + bent_len(D, BEND_R) + (Z_BEND - p1[2]) + math.pi / 2 * BEND_R + (SK_Y0 + 14.0 - Y_DESC - BEND_R)

WB_ENDS = ((VALVE_X[3], VALVE_Y, VALVE_Z1), (WB_X, Y_WB, WB_ZIN))
def wb_tube():
    """Tubo 3/8" dalla valvola WB al pescante nel bagno: un pezzo solo, a ponte come le linee delle vaschette; taglio a 45 gradi in fondo."""
    t = bent_tube(WB_ENDS[0], WB_ENDS[1], Z_BEND, BEND_R)
    return t.cut(xz_prism([(WB_X - 6, WB_ZIN - 0.1), (WB_X + 6, WB_ZIN - 0.1), (WB_X + 6, WB_ZIN + 9.0)], Y_WB - 6, Y_WB + 6))

def waste_elbow():
    """Gomito a innesto sopra la valvola dello scarico, ruotato verso la rientranza del contenitore."""
    p = (VALVE_X[4], VALVE_Y, Z_LANE_WASTE)
    return elbow(p, (0, 0, -1), (1, 0, 0)).rotate(p, (p[0], p[1], p[2] + 1.0), WASTE_ANG)

def waste_tube():
    """Tubo 3/8" dal gomito dello scarico all'attacco SCARICO: diritto (sopra il piano del cavalletto) fino alla
    rientranza del contenitore, poi lungo la rientranza (la piega tra i due tratti la fa il tubo da solo)."""
    vx = VALVE_X[4]; a = math.radians(WASTE_ANG); d = cq.Vector(math.cos(a), math.sin(a), 0)
    m = cq.Vector(vx, VALVE_Y, Z_LANE_WASTE) + d * ELB_L
    L = math.hypot(WASTE_J[0] - vx, WASTE_J[1] - VALVE_Y) - ELB_L
    t = cq.Workplane("XY").add(cq.Solid.makeCylinder(PIPE_R, L + 1.5, m, d))
    t = t.union(xcyl(Y_RECESS, Z_LANE_WASTE, PIPE_R - 0.05, WASTE_J[0] - 1.5, PL_X0 - 18.0))
    assert len(t.val().Solids()) == 1 and t.val().isValid(), "tubo dello scarico: unione non riuscita"
    return t

def bulkhead(name="SCARICO"):
    """Passaparete a innesto rapido 3/8" - 3/8" (foro Ø16), sulla piastra degli attacchi e sul pannello destro."""
    y, z = CONN[name]
    b = xcyl(y, z, 8.0, PL_X0 - 18.0, BASE_X + WT + 8.0)
    return b.union(xcyl(y, z, 11.0, PL_X0 - 10.0, PL_X0 - 4.0)).union(xcyl(y, z, 11.0, BASE_X + WT, BASE_X + WT + 5.0))

def manifold_elbow():
    """Gomito a codolo sull'uscita del collettore: il codolo scende nella sede con O-ring, il braccio a innesto guarda la pompa."""
    return stem_elbow(P_MAN, (0, 0, -1), (1, 0, 0), P_MAN[2] - (MAN_Z1 - 14.0))

def manifold_tube():
    """Spezzone di tubo 3/8" dal gomito del collettore al raccordo sinistro della pompa (disegnato tra le due bocche)."""
    return seg((P_MAN[0] + ELB_L, P_MAN[1], P_MAN[2]), (PUMP_PL, PUMP_PORT_Y, PUMP_PORT_Z), PIPE_R)

def rise_elbow():
    """Gomito a innesto ai piedi della salita verso la tank, fuori dal fianco destro del cavalletto."""
    return elbow(P_RISE, (-1, 0, 0), (0, 0, 1))

def tank_tube():
    """Tubo 3/8" dal raccordo destro della pompa al gomito (attraversa il fianco del cavalletto) e salita verso la tank."""
    t = seg((PUMP_PR, PUMP_PORT_Y, PUMP_PORT_Z), (X_RISE - ELB_L, PUMP_PORT_Y, PUMP_PORT_Z), PIPE_R)
    return t.union(seg((X_RISE, Y_RISE, PUMP_PORT_Z + ELB_L), (X_RISE, Y_RISE, Z_HOSE0), PIPE_R - 0.05))

def tank_hose():
    """Tubo in silicone 8 x 12 nero dal portagomma orizzontale della tank alla salita: quarto di curva di raggio 25 nel
    piano verticale del portagomma, calzato sul portagomma per 13 mm e sul tubo da 3/8" per 15 mm (fascette)."""
    d = cq.Vector(X_RISE - BARB_TIP[0], Y_RISE - BARB_TIP[1], 0).normalized()
    R = T.HOSE_RB; c = math.sqrt(0.5)
    w = cq.Workplane("XZ").moveTo(-13.0, 0).lineTo(0, 0).threePointArc((R * c, -(R - R * c)), (R, -R)).lineTo(R, -R - 15.0)
    h = cq.Workplane("YZ").workplane(offset=-13.0).circle(HOSE_R).sweep(w)
    h = h.rotate((0, 0, 0), (0, 0, 1), math.degrees(math.atan2(d.y, d.x))).translate((BARB_TIP[0], BARB_TIP[1], BARB_Z))
    assert len(h.val().Solids()) == 1 and h.val().isValid(), "silicone della tank"
    return h

def overflow_hose():
    """Tubo in silicone 8 x 12 nero del troppopieno della tank: dall'innesto sotto il piano del cavalletto alla piastra degli
    scarichi, diritto e sempre in discesa (nessuna curva: non puo' strozzarsi)."""
    a0 = cq.Vector(OVF_XM, OVF_YM, SOCK_Z); e = cq.Vector(*OVF_END)
    d = (e - a0).normalized(); a = a0 + d * 14.5
    return cq.Workplane("XY").add(cq.Solid.makeCylinder(HOSE_R, (e - a).Length, a, d))

# ---- ingresso dell'acqua di rete: passaparete -> flussimetro -> gomito -> gomito -> valvola -> gomito -> salita -> beccuccio ----
IN_Y0, IN_Y1, IN_Z = 60.0, 125.0, 113.0                  # i due tratti sotto il piano del cavalletto
FS_X0, FS_X1 = 296.0, 361.0                              # corpo del flussimetro YF-S201
FS_AD = 20.0                                             # nippli 1/2" + raccordi 1/2" femmina - 3/8" a innesto (per lato)
IV_X0 = 255.0                                            # bocca d'ingresso della valvola dell'ingresso (sdraiata, asse X)
E1 = (215.0, IN_Y0, IN_Z); E2 = (215.0, IN_Y1, IN_Z); E3 = (384.0, IN_Y1, IN_Z)
Z_IN_TOP = 238.0                                         # tratto orizzontale sopra il bordo del bagno
SPOUT = (374.0, 245.0); SPOUT_Z = 200.0                  # beccuccio: 26 mm d'aria sopra il troppopieno
E4 = (E3[0], E3[1], Z_IN_TOP); E5 = (SPOUT[0], SPOUT[1], Z_IN_TOP)

def flow_sensor():
    """Flussimetro YF-S201 (corpo 65 x 35 x 35, nippli G 1/2") con i due raccordi 1/2" F - 3/8" a innesto, asse X."""
    f = box(FS_X0, FS_X1, IN_Y0 - 17.5, IN_Y0 + 17.5, IN_Z - 17.5, IN_Z + 17.5)
    f = f.union(xcyl(IN_Y0, IN_Z, 10.0, FS_X0 - FS_AD, FS_X1 + FS_AD))
    return f.union(box((FS_X0 + FS_X1) / 2 - 9, (FS_X0 + FS_X1) / 2 + 9, IN_Y0 - 27.0, IN_Y0 - 17.5, IN_Z - 8.0, IN_Z + 8.0))   # sensore Hall

def inlet_valve():
    return valve(0, x=IV_X0, y=IN_Y1, z0=IN_Z, axis="X", coil=(0, 1, 0))

def _spout_dir():
    a = math.atan2(SPOUT[1] - E3[1], SPOUT[0] - E3[0]); return (math.cos(a), math.sin(a), 0)

def inlet_fittings():
    """Gomiti a innesto 3/8" della linea d'ingresso (cinque)."""
    d = _spout_dir()
    e = elbow(E1, (1, 0, 0), (0, 1, 0)).union(elbow(E2, (0, -1, 0), (1, 0, 0))).union(elbow(E3, (-1, 0, 0), (0, 0, 1)))
    return e.union(elbow(E4, (0, 0, -1), d)).union(elbow(E5, (-d[0], -d[1], 0), (0, 0, -1)))

def inlet_tubes():
    """Spezzoni di tubo 3/8" della linea d'ingresso (disegnati tra le bocche dei raccordi)."""
    t = seg((FS_X1 + FS_AD, IN_Y0, IN_Z), (PL_X0 - 18.0, IN_Y0, IN_Z), PIPE_R)
    for s in (seg((E1[0] + ELB_L, IN_Y0, IN_Z), (FS_X0 - FS_AD, IN_Y0, IN_Z), PIPE_R),
              seg((E1[0], IN_Y0 + ELB_L, IN_Z), (E2[0], IN_Y1 - ELB_L, IN_Z), PIPE_R),
              seg((E2[0] + ELB_L, IN_Y1, IN_Z), (IV_X0, IN_Y1, IN_Z), PIPE_R),
              seg((IV_X0 + VALVE_LEN, IN_Y1, IN_Z), (E3[0] - ELB_L, IN_Y1, IN_Z), PIPE_R),
              seg((E3[0], E3[1], IN_Z + ELB_L), (E4[0], E4[1], Z_IN_TOP - ELB_L), PIPE_R),
              seg((SPOUT[0], SPOUT[1], SPOUT_Z), (SPOUT[0], SPOUT[1], Z_IN_TOP - ELB_L), PIPE_R)):
        t = t.add(s)
    d = cq.Vector(*_spout_dir()); L = math.hypot(SPOUT[0] - E3[0], SPOUT[1] - E3[1])
    t = t.add(cq.Workplane("XY").add(cq.Solid.makeCylinder(PIPE_R, L - 2 * ELB_L, cq.Vector(E4[0], E4[1], Z_IN_TOP) + d * ELB_L, d)))
    return cq.Workplane("XY").add(cq.Compound.makeCompound([v for v in t.vals()]))

def inlet_tube_lens():
    a = math.hypot(SPOUT[0] - E3[0], SPOUT[1] - E3[1])
    return [("passaparete INGRESSO -> flussimetro", PL_X0 - 18.0 - FS_X1 - FS_AD + 2 * INS),
            ("flussimetro -> gomito", FS_X0 - FS_AD - E1[0] - ELB_L + 2 * INS),
            ("gomito -> gomito", IN_Y1 - IN_Y0 - 2 * ELB_L + 2 * INS),
            ("gomito -> valvola dell'ingresso", IV_X0 - E2[0] - ELB_L + 2 * INS),
            ("valvola dell'ingresso -> gomito", E3[0] - ELB_L - IV_X0 - VALVE_LEN + 2 * INS),
            ("salita sopra il cavalletto", Z_IN_TOP - IN_Z - 2 * ELB_L + 2 * INS),
            ("tratto sopra il bordo del bagno", a - 2 * ELB_L + 2 * INS),
            ("beccuccio", Z_IN_TOP - ELB_L - SPOUT_Z + INS)]

# ---- troppopieno del bagno ----
OB_HOSE_R = 8.0                                          # tubo in PVC cristal 12 x 16
OB_X = 404.0                                             # discesa nel vano tra contenitore e pannello destro
def bath_overflow_fitting():
    """Passaparete 1/2" con dado e guarnizioni (foro Ø22) e portagomma a gomito 1/2" - 16 mm rivolto in basso."""
    y, z = OVF_BATH_Y, OVF_BATH_Z
    b = xcyl(y, z, 10.0, IX1 - 8.0, OB_X).union(xcyl(y, z, 16.0, IX1 - 4.0, IX1)).union(xcyl(y, z, 16.0, IX1 + TUB_WALL, IX1 + TUB_WALL + 7.0))
    return b.union(zcyl(OB_X, y, 10.0, z - 26.0, z + 10.0)).union(zcyl(OB_X, y, 7.0, z - 44.0, z - 26.0))

def bath_overflow_hose():
    """Tubo in PVC cristal 12 x 16 del troppopieno del bagno: scende nel vano a destra del contenitore, passa sopra il carter
    dei riscaldatori ed esce dalla piastra degli scarichi (percorso indicativo: in realta' curve larghe)."""
    yo, zo = CONN["TROPPOPIENO_BAGNO"]
    return route([(OB_X, OVF_BATH_Y, OVF_BATH_Z - 30.0), (OB_X, OVF_BATH_Y, zo), (OB_X, yo, zo), (BASE_X + WT + 12.0, yo, zo)], OB_HOSE_R)

def bath_overflow_len():
    yo, zo = CONN["TROPPOPIENO_BAGNO"]
    return (OVF_BATH_Z - 30.0 - zo) + (OVF_BATH_Y - yo) + (BASE_X + WT + 12.0 - OB_X) + 20.0

# ---- rubinetto di svuotamento del bagno ----
BD_XD = IX1 - 20.0                                       # asse del pescante interno
def bath_drain():
    """Passaparete in PP 1/2" a filetto lungo (60 mm) nella parete destra del contenitore: flangia e guarnizione fuori,
    dado e guarnizione dentro. Dentro, un gomito 1/2" F-F rivolto in basso con un nipplo corto che pesca a 4 mm dal
    fondo: aperto il rubinetto, il bagno si svuota per sifone fin quasi al fondo (il tubo d'uscita deve scendere sotto la macchina).
    Il filetto attraversa il pannello destro (foro Ø23) e un secondo dado lo blocca da fuori."""
    y, z, xo = BD_Y, BD_Z, IX1 + TUB_WALL
    b = xcyl(y, z, 10.4, BD_XD + 9.0, BASE_X + WT + 12.0)                                      # filetto 1/2" (Ø20.9)
    b = b.union(xcyl(y, z, 16.0, xo, xo + 3.0)).union(xcyl(y, z, 13.5, IX1 - 9.0, IX1 - 0.5))   # flangia fuori, dado dentro
    b = b.union(xcyl(y, z, 13.5, BASE_X + WT, BASE_X + WT + 6.0))                              # dado sul pannello
    b = b.union(xcyl(y, z, 13.0, BD_XD - 13.0, BD_XD + 13.0).intersect(zcyl(BD_XD, y, 13.0, z - 13.0, z + 13.0)).union(zcyl(BD_XD, y, 13.0, z - 13.0, z)).union(xcyl(y, z, 13.0, BD_XD, BD_XD + 13.0)))
    return b.union(zcyl(BD_XD, y, 10.4, TUB_FLOOR + 4.0, z - 12.0))                            # nipplo pescante

BV_X0 = BASE_X + WT + 12.0                               # rubinetto a sfera fuori dal pannello destro
def bath_tap():
    """Rubinetto a sfera 1/2" F x portagomma 16 mm (PP o PVC) avvitato sul passaparete, fuori dal pannello destro: leva in
    alto, portagomma rivolto in basso (ci si infila un tubo da 12 x 16 verso un secchio sotto il piano del tavolo)."""
    y, z = BD_Y, BD_Z
    v = xcyl(y, z, 14.0, BV_X0 - 2.0, BV_X0 + 34.0).union(box(BV_X0 + 12.0, BV_X0 + 18.0, y - 4.0, y + 4.0, z + 12.0, z + 22.0))
    v = v.union(box(BV_X0 + 10.0, BV_X0 + 20.0, y - 4.0, y + 40.0, z + 20.0, z + 24.0))        # leva (chiuso: di traverso)
    return v.union(zcyl(BV_X0 + 26.0, y, 9.0, z - 22.0, z)).union(zcyl(BV_X0 + 26.0, y, 8.0, z - 40.0, z - 22.0))

# ---- bagno: riscaldatori, sensori, sonde ----
def heater(i):
    """Riscaldatore a cartuccia filettata 12 V 100 W: tubo Ø12 x 100, filetto M16, esagono 22, dado e guarnizioni."""
    y = HEAT_Y[i]; xo = IX1 + TUB_WALL
    h = xcyl(y, HEAT_Z, 6.0, IX1 - 100.0, IX1).union(xcyl(y, HEAT_Z, 8.0, IX1 - 7.0, xo + 3.0))
    h = h.union(xcyl(y, HEAT_Z, 12.5, IX1 - 6.0, IX1 - 0.5))                                          # dado + guarnizione interni
    h = h.union(cq.Workplane("YZ").center(y, HEAT_Z).polygon(6, 25.4).extrude(9.0).translate((xo + 0.5, 0, 0)))
    return h.union(xcyl(y, HEAT_Z, 3.5, xo + 9.5, xo + 22.0))                                          # uscita dei cavi

def level_sensor(z):
    """Sensore di livello capacitivo XKC-Y21, incollato all'esterno della parete anteriore del bagno."""
    yo = IY0 - TUB_WALL
    return box(LVL_X - 14.0, LVL_X + 14.0, yo - 6.0, yo, z - 8.0, z + 8.0)

CHS_Y1 = VALVE_Y - CH_R + 2.0                          # faccia piana della camera di livello (verso il fronte)
def chamber_sensor(k):
    """XKC-Y21 della camera di livello sopra la valvola Ck (MIN: la camera si svuota quando la vaschetta e' vuota)."""
    zc = (CH_Z0 + CH_Z1) / 2
    return box(VALVE_X[k] - 14.0, VALVE_X[k] + 14.0, CHS_Y1 - 6.0, CHS_Y1, zc - 8.0, zc + 8.0)

PMS_Z = 200.0                                          # quota del sensore MAX sulla parete anteriore della vaschetta (sopra l'acqua del bagno)
def pan_sensor(k):
    """XKC-Y21 MAX della vaschetta k, tenuto dalla staffa C03 contro la parete anteriore, sopra il livello del bagno."""
    y1 = pan_wall_y(PMS_Z + 8.0) - 0.6
    return box(VAS_X[k] - 14.0, VAS_X[k] + 14.0, y1 - 6.0, y1, PMS_Z - 8.0, PMS_Z + 8.0)

PROBE_X = WB_X + 18.0
def probe_bath():
    """Sonda DS18B20 inox Ø6 x 30 nel bagno, accanto al pescante WB."""
    return zcyl(PROBE_X, Y_WB, 3.0, 62.0, 92.0).union(zcyl(PROBE_X, Y_WB, 1.5, 92.0, 215.0))

def probe_chem_xy():
    return VAS_X[0] + 24.0, pan_wall_y(150.0) + GN9_WALL + 5.0

def probe_chem():
    """Sonda DS18B20 inox Ø6 x 30 nella vaschetta C1, appesa al suo cavo che passa nella manica del gancio C19."""
    x, y = probe_chem_xy()
    return zcyl(x, y, 3.0, 112.0, 142.0).union(zcyl(x, y, 1.5, 142.0, VAS_ZR - 2.0))

# ---- elettronica ----
PSU_L, PSU_W, PSU_H = 215.0, 115.0, 30.0              # Mean Well LRS-350-12 (12 V 29 A), ventola a temperatura
PSU_X0, PSU_Y0, PSU_Z0 = 60.0, 88.0, 10.0
def psu():
    """Alimentatore Mean Well LRS-350-12 (215 x 115 x 30) in piedi contro la paratia, morsetti in basso."""
    return box(PSU_X0, PSU_X0 + PSU_W, PSU_Y0, PSU_Y0 + PSU_H, PSU_Z0, PSU_Z0 + PSU_L)

def board(x0, y0, z0, lx, ly, h):
    """Scheda elettronica: circuito stampato da 1.6 + ingombro dei componenti."""
    return box(x0, x0 + lx, y0, y0 + ly, z0, z0 + 1.6).union(box(x0 + 3, x0 + lx - 3, y0 + 3, y0 + ly - 3, z0 + 1.6, z0 + h))

DISP_L, DISP_W, DISP_T = 114.4, 66.8, 3.2
def display_local():
    """Scheda Guition JC4880P443C (114.4 x 66.8): vetro a z = 0 verso +z, elettronica sotto."""
    return box(-DISP_L / 2, DISP_L / 2, -DISP_W / 2, DISP_W / 2, -DISP_T, 0.0).union(box(-50.0, 50.0, -29.0, 29.0, -11.0, -DISP_T))

SPK = (0.0, 40.0, 222.0)
def speaker():
    """Altoparlante 8 ohm 2 W Ø40 dietro la foratura del pannello sinistro."""
    x, y, z = SPK
    return xcyl(y, z, 20.0, 0.0, 5.0).union(xcyl(y, z, 11.0, 5.0, 18.0))

IEC_Y, IEC_Z = (92.0, 119.5), (24.0, 71.5)
def iec_inlet():
    """Presa IEC C14 a pannello con interruttore e fusibile (foro 27.5 x 47.5), nel pannello sinistro."""
    b = box(-WT - 2.0, -WT, IEC_Y[0] - 4.0, IEC_Y[1] + 4.0, IEC_Z[0] - 4.0, IEC_Z[1] + 4.0)
    return b.union(box(-WT, 27.0, IEC_Y[0] + 0.25, IEC_Y[1] - 0.25, IEC_Z[0] + 0.25, IEC_Z[1] - 0.25))

if __name__ == "__main__":
    print("salita tank", round(X_RISE, 1), round(Y_RISE, 1), "portagomma", [round(v, 1) for v in BARB_TIP], round(BARB_Z, 1), "| troppopieno tank: pendenza", round(OVF_ANG, 1), "imbardata", round(OVF_YAW, 1))
    for n, f in (("contenitore", tub), ("vaschetta", lambda: vaschetta(0)), ("linea C1", lambda: line(0)), ("linea C2", lambda: line(1)),
                 ("linea C3", lambda: line(2)), ("tubo WB", wb_tube), ("pompa", pump), ("riscaldatore", lambda: heater(0)),
                 ("tubo scarico", waste_tube), ("tubo tank", tank_tube), ("silicone tank", tank_hose), ("troppopieno tank", overflow_hose),
                 ("ingresso raccordi", inlet_fittings), ("ingresso tubi", inlet_tubes), ("flussimetro", flow_sensor), ("valvola ingresso", inlet_valve),
                 ("troppopieno bagno", bath_overflow_hose), ("passaparete troppopieno", bath_overflow_fitting)):
        s = f().val(); bb = s.BoundingBox()
        print(f"{n:24s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:8.1f} cm3  X[{bb.xmin:.0f},{bb.xmax:.0f}] Y[{bb.ymin:.0f},{bb.ymax:.0f}] Z[{bb.zmin:.0f},{bb.zmax:.0f}]")
    print("lunghezze linee:", [round(line_len(k)) for k in range(3)])
