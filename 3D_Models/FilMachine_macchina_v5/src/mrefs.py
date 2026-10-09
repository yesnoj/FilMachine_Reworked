"""Sagome dei componenti acquistati della macchina (REF_), nel riferimento macchina. Dimensioni indicative dove
il componente non e' normalizzato: servono per gli ingombri, per l'assieme e per la nuova distinta."""
import math
import cadquery as cq
from common import loft_planar
from machine import *

def base_board():
    """Tavola di base in multistrato 420 x 505 x 12."""
    return box(0, BASE_X, 0, BASE_Y, -BASE_T, 0)

def tub():
    """Contenitore Euro 400 x 300 x 170 a pareti e fondo chiusi (bagno termostatico), con i due fori Ø16.5 dei riscaldatori."""
    x0, x1, y0, y1 = TUB_X0, TUB_X0 + TUB_L, TUB_Y0, TUB_Y0 + TUB_W
    t = box(IX0 - TUB_WALL, IX1 + TUB_WALL, IY0 - TUB_WALL, IY1 + TUB_WALL, 0, TUB_H)
    t = t.union(box(x0, x1, y0, y1, TUB_H - TUB_BAND_T, TUB_H)).union(box(x0, x1, y0, y1, 0, TUB_BAND_B))
    t = t.cut(box(IX0, IX1, IY0, IY1, TUB_FLOOR, TUB_H + 1))
    for y in HEAT_Y:
        t = t.cut(xcyl(y, HEAT_Z, 8.25, IX1 - 1, IX1 + TUB_WALL + 1))
    return t

def rail(front=True):
    """Longherone: tubo quadro di alluminio 15 x 15 x 1.5."""
    y0 = RAIL_F_Y0 if front else RAIL_R_Y0
    r = box(RAIL_X0, RAIL_X1, y0, y0 + RAIL, RIM_Z - RAIL, RIM_Z)
    return r.cut(box(RAIL_X0 - 1, RAIL_X1 + 1, y0 + RAIL_T, y0 + RAIL - RAIL_T, RIM_Z - RAIL + RAIL_T, RIM_Z - RAIL_T))

def _pan(x0, y0, L, W, z0, z1, draft, wall, rim_w, rim_t):
    def ring(z, off):
        return [(x0 + off, y0 + off, z), (x0 + L - off, y0 + off, z), (x0 + L - off, y0 + W - off, z), (x0 + off, y0 + W - off, z)]
    outer = cq.Workplane("XY").add(loft_planar([ring(z0, rim_w + draft), ring(z1 - rim_t, rim_w)]))
    inner = cq.Workplane("XY").add(loft_planar([ring(z0 + wall, rim_w + draft + wall), ring(z1 + 0.01, rim_w + wall)]))
    return outer.union(box(x0, x0 + L, y0, y0 + W, z1 - rim_t, z1)).cut(inner)

def vaschetta(k):
    """Vaschetta Gastronorm GN 1/9 h 150 (176 x 108, 1.5 litri)."""
    return _pan(VAS_X[k] - GN9_L / 2, VAS_Y0, GN9_L, GN9_W, VAS_ZB, VAS_ZR, GN9_DRAFT, GN9_WALL, GN9_RIM, GN9_RIM_T)

# ---- testina: due gomiti a innesto + tre spezzoni di tubo 3/8" ----
def head_elbows(k):
    x = VAS_X[k]
    return elbow((x, Y_DIP, Z_CROSS), (0, 0, -1), (0, -1, 0)).union(elbow((x, Y_STUB, Z_CROSS), (0, 0, -1), (0, 1, 0)))

def head_tubes(k):
    """Pescante (taglio a 45 gradi in fondo), ponticello e codolo: disegnati fino alla bocca dei raccordi."""
    x = VAS_X[k]
    zb = VAS_ZB + GN9_WALL + 3.0
    dip = seg((x, Y_DIP, zb), (x, Y_DIP, Z_CROSS - ELB_L), PIPE_R)
    dip = dip.cut(yz_prism([(Y_DIP - 6, zb - 0.1), (Y_DIP + 6, zb - 0.1), (Y_DIP + 6, zb + 9.0)], x - 6, x + 6))
    t = dip.union(seg((x, Y_STUB + ELB_L, Z_CROSS), (x, Y_DIP - ELB_L, Z_CROSS), PIPE_R))
    return t.union(seg((x, Y_STUB, STUB_Z0), (x, Y_STUB, Z_CROSS - ELB_L), PIPE_R))

# ---- valvole, pompa, tubi fissi ----
def valve(k):
    """Elettrovalvola 12 V NC con innesti rapidi 3/8", in verticale, bobina verso il fronte."""
    x = VALVE_X[k]; h = VALVE_BODY / 2
    v = zcyl(x, VALVE_Y, ELB_R, VALVE_Z0, VALVE_Z1)
    v = v.union(box(x - h, x + h, VALVE_Y - h, VALVE_Y + h, VALVE_ZC - 15.0, VALVE_ZC + 15.0))
    return v.union(box(x - 15.0, x + 15.0, VALVE_Y - h - VALVE_COIL, VALVE_Y - h, VALVE_ZC - 14.0, VALVE_ZC + 14.0))

def valve_stub(k):
    """Spezzone di tubo tra collettore e valvola (inserito 14 mm nel collettore)."""
    return seg((VALVE_X[k], VALVE_Y, MAN_Z1 - 14.0), (VALVE_X[k], VALVE_Y, VALVE_Z0), PIPE_R)

def pump():
    """Pompa a ingranaggi reversibile Marco UP3-R 12 V (177 x 114 x 88), con i due raccordi 3/8" BSP maschio - tubo 3/8"."""
    y0 = PUMP_Y0; zf = PUMP_FOOT
    p = ycyl(PUMP_XC, zf + 46.0, 38.0, y0, y0 + 122.0)                                             # motore
    p = p.union(box(PUMP_XC - 40.0, PUMP_XC + 40.0, y0 + 120.0, y0 + PUMP_L, zf + 6.0, zf + 84.0))   # testa della pompa
    p = p.union(box(PUMP_XC - PUMP_W / 2, PUMP_XC + PUMP_W / 2, y0 + 30.0, y0 + 110.0, zf, zf + 10.0))  # piede
    return p.union(xcyl(PUMP_PORT_Y, PUMP_PORT_Z, 9.0, PUMP_XC - 74.0, PUMP_XC + 74.0))             # attacchi con raccordi

PUMP_PL, PUMP_PR = PUMP_XC - 74.0, PUMP_XC + 74.0        # bocche dei due raccordi della pompa
X_RISE, Y_RISE = 386.0, 88.0                             # salita del tubo verso la tank (lontana dal portagomma: curva larga del silicone)
BARB_X, BARB_Y = X_T + 68.75, Y_T + 17.0                 # portagomma del pescante della tank
OVF_XM, OVF_YM = X_T - 27.0, Y_T + 58.75                 # asse del troppopieno della tank
SOCK_Z = Z_STAND - DECK_T - 9.0                          # asse dell'innesto del troppopieno sotto il piano del cavalletto
Z_OVF_HOSE = 95.5                                        # quota del tubo del troppopieno sulla staffa degli scarichi
OVF_END = (BASE_X - 9.0, OVF_YM, Z_OVF_HOSE)
OVF_ANG = math.degrees(math.atan2(SOCK_Z - Z_OVF_HOSE, OVF_END[0] - OVF_XM))   # pendenza del tubo del troppopieno (sempre in discesa)
WASTE_J = (253.0, Y_RECESS)                              # punto in cui il tubo dello scarico entra nella rientranza del contenitore
WASTE_ANG = math.degrees(math.atan2(WASTE_J[1] - VALVE_Y, WASTE_J[0] - VALVE_X[4]))
P_MAN = (MAN_OUT[0], MAN_OUT[1], PUMP_PORT_Z)            # gomito a codolo sull'uscita del collettore
P_PUMP = (X_RISE, PUMP_PORT_Y, PUMP_PORT_Z)              # gomito a codolo nel raccordo destro della pompa
P_RISE = (X_RISE, Y_RISE, PUMP_PORT_Z)                   # gomito ai piedi della salita verso la tank
Z_HOSE0 = Z_T0 + 86.0                                    # da qui in su il silicone fa la curva a U

def feed_ends(k):
    """Estremi del tubo della vaschetta k: bocca superiore della valvola e sommita' del tratto sinistro del sifone."""
    return (VALVE_X[k], VALVE_Y, VALVE_Z1), (VAS_X[k] - 2 * U_R, Y_STUB, Z_MOUTH + 5.0)

def feed_tube(k):
    """Tubo 3/8" dalla valvola k al sifone della vaschetta k: un pezzo solo posato a ponte sopra il bordo del bagno
    (curve di raggio BEND_R); entra per 15 mm nel silicone."""
    p0, p1 = feed_ends(k)
    return bent_tube(p0, p1, Z_BEND, BEND_R)

def siphon(k):
    """Sifone: spezzone di tubo in silicone 8 x 12 piegato a U nella sua culla; a sinistra e' calzato sul tubo che
    viene dalla valvola (fascetta), a destra presenta la bocca verso l'alto al codolo della testina."""
    x = VAS_X[k]; zc = Z_MOUTH - 10.0
    u = cq.Workplane("XY").circle(HOSE_R).extrude(Z_MOUTH - zc).translate((x, Y_STUB, zc))
    u = u.union(cq.Workplane("XY").circle(HOSE_R).extrude(Z_MOUTH + 5.0 - zc).translate((x - 2 * U_R, Y_STUB, zc)))
    bend = cq.Workplane("XY").moveTo(U_R, 0).circle(HOSE_R).revolve(180, (0, 0, 0), (0, 1, 0))      # mezza ciambella attorno a Y
    bb = bend.val().BoundingBox()
    if bb.zmax > 1.0: bend = bend.mirror("XY")
    u = u.union(bend.translate((x - U_R, Y_STUB, zc)))
    assert len(u.val().Solids()) == 1 and u.val().isValid(), "sifone: unione non riuscita"
    return u

WB_ENDS = ((VALVE_X[3], VALVE_Y, VALVE_Z1), (WB_X, Y_STUB, WB_ZIN))
def wb_tube():
    """Tubo 3/8" dalla valvola WB al pescante nel bagno: un pezzo solo, a ponte come quelli delle vaschette."""
    return bent_tube(WB_ENDS[0], WB_ENDS[1], Z_BEND, BEND_R)

def waste_elbow():
    """Gomito a innesto sopra la valvola dello scarico, ruotato verso la rientranza del contenitore."""
    p = (VALVE_X[4], VALVE_Y, Z_LANE_WASTE)
    return elbow(p, (0, 0, -1), (1, 0, 0)).rotate(p, (p[0], p[1], p[2] + 1.0), WASTE_ANG)

def waste_tube():
    """Tubo 3/8" dal gomito dello scarico all'attacco SCARICO: diritto fino alla rientranza del contenitore, poi lungo
    la rientranza (la piega tra i due tratti e' di 18 gradi: il tubo la fa da solo)."""
    vx = VALVE_X[4]; a = math.radians(WASTE_ANG); d = cq.Vector(math.cos(a), math.sin(a), 0)
    m = cq.Vector(vx, VALVE_Y, Z_LANE_WASTE) + d * ELB_L
    L = math.hypot(WASTE_J[0] - vx, WASTE_J[1] - VALVE_Y) - ELB_L
    t = cq.Workplane("XY").add(cq.Solid.makeCylinder(PIPE_R, L + 1.5, m, d))
    t = t.union(xcyl(Y_RECESS, Z_LANE_WASTE, PIPE_R - 0.05, WASTE_J[0] - 1.5, BASE_X - 26.0))
    assert len(t.val().Solids()) == 1 and t.val().isValid(), "tubo dello scarico: unione non riuscita"
    return t

def bulkhead():
    """Passaparete a innesto rapido 3/8" - 3/8" (attacco SCARICO sulla staffa di destra)."""
    b = xcyl(Y_RECESS, Z_LANE_WASTE, 8.0, BASE_X - 26.0, BASE_X - 0.5)
    return b.union(xcyl(Y_RECESS, Z_LANE_WASTE, 11.0, BASE_X - 18.0, BASE_X - 14.0)).union(xcyl(Y_RECESS, Z_LANE_WASTE, 11.0, BASE_X - 10.0, BASE_X - 6.0))

def manifold_elbow():
    """Gomito a codolo sull'uscita del collettore: il codolo scende nella sede con O-ring, il braccio a innesto guarda la pompa."""
    return stem_elbow(P_MAN, (0, 0, -1), (1, 0, 0), P_MAN[2] - (MAN_Z1 - 14.0))

def manifold_tube():
    """Spezzone di tubo 3/8" dal gomito del collettore al raccordo sinistro della pompa (disegnato tra le due bocche)."""
    return seg((P_MAN[0] + ELB_L, P_MAN[1], P_MAN[2]), (PUMP_PL, PUMP_PORT_Y, PUMP_PORT_Z), PIPE_R)

def pump_elbow():
    """Gomito a codolo nel raccordo destro della pompa (codolo accorciato: 15 mm nel raccordo + 12 fino all'angolo)."""
    return stem_elbow(P_PUMP, (-1, 0, 0), (0, -1, 0), P_PUMP[0] - PUMP_PR)

def rise_elbow():
    """Gomito a innesto ai piedi della salita verso la tank."""
    return elbow(P_RISE, (0, 1, 0), (0, 0, 1))

def tank_tube():
    """Tubo 3/8" tra i due gomiti sotto il cavalletto e salita verso la tank (disegnati tra le bocche dei raccordi)."""
    t = seg((X_RISE, Y_RISE + ELB_L, PUMP_PORT_Z), (X_RISE, PUMP_PORT_Y - ELB_L, PUMP_PORT_Z), PIPE_R)
    return t.union(seg((X_RISE, Y_RISE, PUMP_PORT_Z + ELB_L), (X_RISE, Y_RISE, Z_HOSE0), PIPE_R))

def tank_hose():
    """Tubo in silicone 8 x 12 dal portagomma del pescante della tank al tubo 3/8" (calzato sopra, con fascetta):
    mezza curva a U nel piano verticale che passa per i due punti."""
    dx, dy = X_RISE - BARB_X, Y_RISE - BARB_Y
    R = math.hypot(dx, dy) / 2; ang = math.degrees(math.atan2(dy, dx))
    h = cq.Workplane("XY").moveTo(R, 0).circle(HOSE_R).revolve(180, (0, 0, 0), (0, 1, 0))
    if h.val().BoundingBox().zmin < -1.0: h = h.mirror("XY")
    h = h.translate((R, 0, 0)).rotate((0, 0, 0), (0, 0, 1), ang).translate((BARB_X, BARB_Y, Z_HOSE0))
    h = h.union(zcyl(BARB_X, BARB_Y, HOSE_R, Z_T0 + 84.5, Z_HOSE0))
    assert len(h.val().Solids()) == 1 and h.val().isValid()
    return h

def overflow_hose():
    """Tubo in silicone 8 x 12 del troppopieno: dall'innesto sotto il piano del cavalletto alla staffa degli scarichi,
    diritto e sempre in discesa (nessuna curva: non puo' strozzarsi)."""
    x0 = 14.5                                                # disegnato dalla bocca dell'innesto (dentro ci sta a pressione per altri 12 mm)
    a = cq.Vector(OVF_XM + x0, OVF_YM, SOCK_Z - x0 * math.tan(math.radians(OVF_ANG)))
    d = (cq.Vector(*OVF_END) - cq.Vector(OVF_XM, OVF_YM, SOCK_Z)).normalized()
    L = (cq.Vector(*OVF_END) - a).Length
    return cq.Workplane("XY").add(cq.Solid.makeCylinder(HOSE_R, L, a, d))

# ---- bagno: riscaldatori, sensori ----
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

PROBE_Y = Y_STUB - 16.0
def probe_bath():
    """Sonda DS18B20 inox Ø6 x 30 nel bagno."""
    return zcyl(WB_X, PROBE_Y, 3.0, 62.0, 92.0).union(zcyl(WB_X, PROBE_Y, 1.5, 92.0, 150.0))

def probe_chem():
    """Sonda DS18B20 inox Ø6 x 30 nella vaschetta C1 (infilata nella sella della testina)."""
    x, y = VAS_X[0] + 15.0, VAS_Y0 + GN9_RIM + GN9_WALL + 9.5
    return zcyl(x, y, 3.0, 60.0, 90.0).union(zcyl(x, y, 1.5, 90.0, VAS_ZR + 8.0))

# ---- elettronica ----
PSU_X0, PSU_Y0, PSU_Z0 = 62.0, 70.0, 4.0
def psu():
    """Alimentatore 12 V 30 A (215 x 115 x 50), in piedi nello zoccolo, contro la paratia posteriore."""
    return box(PSU_X0, PSU_X0 + PSU_W, PSU_Y0, PSU_Y0 + PSU_H, PSU_Z0, PSU_Z0 + PSU_L)

def board(x0, y0, z0, lx, ly, h):
    """Scheda elettronica: circuito stampato da 1.6 + ingombro dei componenti."""
    return box(x0, x0 + lx, y0, y0 + ly, z0, z0 + 1.6).union(box(x0 + 3, x0 + lx - 3, y0 + 3, y0 + ly - 3, z0 + 1.6, z0 + h))

DISP_L, DISP_W, DISP_T = 114.4, 66.8, 3.2
def display(center, ang):
    """Scheda Guition JC4880P443C (114.4 x 66.8): vetro verso l'alto, parallela alla plancia inclinata."""
    d = box(-DISP_L / 2, DISP_L / 2, -DISP_W / 2, DISP_W / 2, -DISP_T, 0.0).union(box(-50.0, 50.0, -29.0, 29.0, -11.0, -DISP_T))
    return d.rotate((0, 0, 0), (1, 0, 0), ang).translate(center)

SPK = (EL_T, 95.0, 110.0)
def speaker():
    """Altoparlante 8 ohm 2 W Ø40 dietro la griglia del fianco sinistro."""
    x, y, z = SPK
    return xcyl(y, z, 20.0, x, x + 5.0).union(xcyl(y, z, 11.0, x + 5.0, x + 18.0))

IEC_Y, IEC_Z = (78.0, 105.5), (20.0, 67.5)
def iec_inlet():
    """Presa IEC C14 a pannello con interruttore e fusibile (foro 27.5 x 47.5), sul fianco sinistro."""
    b = box(-2.0, 0.0, IEC_Y[0] - 4.0, IEC_Y[1] + 4.0, IEC_Z[0] - 4.0, IEC_Z[1] + 4.0)
    return b.union(box(0.0, 30.0, IEC_Y[0] + 0.25, IEC_Y[1] - 0.25, IEC_Z[0] + 0.25, IEC_Z[1] - 0.25))

if __name__ == "__main__":
    for n, f in (("contenitore", tub), ("vaschetta", lambda: vaschetta(0)), ("gomiti testina", lambda: head_elbows(0)),
                 ("tubi testina", lambda: head_tubes(0)), ("valvola", lambda: valve(0)), ("pompa", pump), ("riscaldatore", lambda: heater(0)),
                 ("tubo C2", lambda: feed_tube(1)), ("tubo WB", wb_tube), ("tubo scarico", waste_tube), ("tubo collettore", manifold_tube),
                 ("tubo tank", tank_tube), ("silicone tank", tank_hose), ("silicone troppopieno", overflow_hose)):
        s = f().val(); bb = s.BoundingBox()
        print(f"{n:22s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:8.1f} cm3  X[{bb.xmin:.0f},{bb.xmax:.0f}] Y[{bb.ymin:.0f},{bb.ymax:.0f}] Z[{bb.zmin:.0f},{bb.zmax:.0f}]")
