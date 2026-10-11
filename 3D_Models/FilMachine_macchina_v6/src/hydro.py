"""Parti stampate del bagno e dell'idraulica (serie C), nel riferimento macchina - rilascio 6.
C03 fermo girevole delle vaschette, C04 portasensore MAX, C05 testata del telaio, C06 portasonde, C07 collettore,
C08 pettine valvole, C09 piastra degli scarichi, C10 carter dei riscaldatori, C11 corpo dell'innesto (nella vaschetta),
C12 ghiera con codolo, C13 otturatore, C14 presa sul fondo del bagno, C15 camera di livello, C16 piastra dell'ingresso,
C17 staffa del beccuccio, C18 staffa a fascetta (flussimetro e valvola dell'ingresso)."""
import math
import cadquery as cq
from machine import *
import threads as TH

GROOVE_R = PIPE_R + 0.95 * ORING_CS          # fondo della gola dell'O-ring 9x2 (schiacciamento ~5 %)
GROOVE_H = 2.6
BORE_R = 4.95                                # sede del tubo da 9.52
Y_PL0 = RAIL_F_Y0 - 4.0                      # faccia anteriore delle staffe avvitate al longherone anteriore: 285
Z_RS = RAIL_Z1 - RAIL / 2                    # quota delle viti nella faccia del longherone: 233.5

def oring_bore(x, y, z_top, depth, z_groove):
    """Sede verticale per un tubo da 9.52 con gola per O-ring 9x2 e invito in alto."""
    b = zcyl(x, y, BORE_R, z_top - depth, z_top + 0.01)
    b = b.union(zcyl(x, y, GROOVE_R, z_groove, z_groove + GROOVE_H))
    cone = cq.Workplane("XY").add(cq.Solid.makeCone(BORE_R, BORE_R + 1.2, 1.2)).translate((x, y, z_top - 1.19))
    return b.union(cone)

def oring_bore_dir(p, d, depth, g0):
    """Sede per un tubo da 9.52 con gola per O-ring 9x2 lungo l'asse d (verso l'interno del pezzo) a partire dalla faccia p."""
    P = cq.Vector(*p); D = cq.Vector(*d)
    b = cq.Workplane("XY").add(cq.Solid.makeCylinder(BORE_R, depth + 0.01, P - D * 0.01, D))
    b = b.union(cq.Workplane("XY").add(cq.Solid.makeCylinder(GROOVE_R, GROOVE_H, P + D * g0, D)))
    return b.union(cq.Workplane("XY").add(cq.Solid.makeCone(BORE_R + 1.2, BORE_R, 1.2, P - D * 0.01, D)))

# ------------------------------------------------------------------ C03 fermo girevole
TB_L, TB_W, TB_T = 13.0, 8.0, 2.7            # leva: lunghezza dall'asse, larghezza, spessore
TB_YF, TB_YR = RAIL_F_Y0 + 4.5, RAIL_R_Y0 + RAIL - 2.5     # assi dei fermi sui longheroni: davanti al bordo anteriore, dietro al posteriore
def turn_button_local():
    """Fermo girevole (nel suo riferimento: asse Z, faccia inferiore del mozzo sul longherone a z = 0, leva verso +X).
    Mozzo Ø7 alto quanto il bordo della vaschetta + 0.3, leva sopra il bordo; vite autofilettante 2.9 a testa svasata."""
    h = GN9_RIM_T + 0.3
    b = zcyl(0, 0, 3.5, 0, h + TB_T).union(box(0, TB_L, -TB_W / 2, TB_W / 2, h, h + TB_T))
    b = b.union(zcyl(TB_L, 0, TB_W / 2, h, h + TB_T))
    b = b.union(box(-6.0, 0, -2.5, 2.5, h, h + TB_T))                               # codino per girarlo col dito
    b = b.cut(zcyl(0, 0, 1.6, -0.01, h + TB_T + 0.01))
    return b.cut(cq.Workplane("XY").add(cq.Solid.makeCone(1.6, 3.0, 1.4)).translate((0, 0, h + TB_T - 1.39)))

def turn_button(k, rear=False):
    """Fermo girevole montato (chiuso): davanti e dietro ogni vaschetta, leva sopra il bordo."""
    y = TB_YR if rear else TB_YF
    return turn_button_local().rotate((0, 0, 0), (0, 0, 1), 90.0 if not rear else -90.0).translate((VAS_X[k], y, RAIL_Z1))

# ------------------------------------------------------------------ C04 portasensore MAX
def sensor_holder(k):
    """Portasensore MAX: piastra avvitata alla faccia anteriore del longherone anteriore, che passa sotto il longherone e
    tiene il sensore XKC-Y21 appoggiato alla parete anteriore della vaschetta, sopra il livello del bagno (il sensore si
    infila dall'alto nella tasca e si ferma con una fascetta). Su C1 la tacca in alto tiene il cavo della sonda della chimica."""
    import mrefs as R
    x = VAS_X[k]; s = R.pan_sensor(k).val().BoundingBox()
    zb = s.zmin - 3.0
    p = box(x - 18.0, x + 18.0, Y_PL0, RAIL_F_Y0, zb, RAIL_Z1 - 0.5)
    p = p.union(box(x - 18.0, x + 18.0, Y_PL0, s.ymax - 0.2, zb, zb + 3.0))                          # fondo della tasca
    p = p.union(box(x - 18.0, x - 14.6, RAIL_F_Y0, s.ymax - 0.2, zb, s.zmax - 2.0))                   # fianchi della tasca
    p = p.union(box(x + 14.6, x + 18.0, RAIL_F_Y0, s.ymax - 0.2, zb, s.zmax - 2.0))
    p = p.union(box(x - 18.0, x + 18.0, RAIL_F_Y0, s.ymin - 0.4, zb, s.zmax - 2.0))                   # schienale del sensore
    for sg in (1, -1):
        p = p.cut(ycyl(x + sg * 9.0, Z_RS, M3_CLR, Y_PL0 - 0.01, RAIL_F_Y0 + 0.01))
        p = p.cut(box(x + sg * 16.3 - 1.0, x + sg * 16.3 + 1.0, Y_PL0 - 0.01, s.ymax + 0.01, zb + 6.0, zb + 10.0))   # asole della fascetta
    p = p.cut(box(x - 2.5, x + 2.5, Y_PL0 - 0.01, Y_PL0 + 1.5, RAIL_Z1 - 12.0, RAIL_Z1))           # tacca del cavo (sensore o sonda)
    return p

# ------------------------------------------------------------------ C05 testata del telaio
def frame_end(side=-1):
    """Testata del telaio portavaschette (side = -1 sinistra, +1 destra): a cavallo della fascia del contenitore, con le
    due tasche che tengono i longheroni 6 mm sopra il bordo (le vaschette arrivano a filo del coperchio) e due viti M4
    che passano sotto la fascia. Si stampa capovolta."""
    xo = TUB_X0 - 3.5
    e = box(xo, IX0 + 18.0, RAIL_F_Y0 - 8.0, IY1 - 0.4, RIM_Z, RIM_Z + 4.0)                                   # piastra sulla fascia
    e = e.union(box(IX0 + 0.3, IX0 + 4.0, RAIL_F_Y0 - 8.0, IY1 - 0.4, RIM_Z - 20.0, RIM_Z))                  # labbro interno
    e = e.union(box(xo, TUB_X0 - 0.3, RAIL_F_Y0 + 7.0, RAIL_R_Y0 + 2.0, RIM_Z - 34.0, RIM_Z))                # labbro esterno
    for y0 in (RAIL_F_Y0, RAIL_R_Y0):                                                                       # tasche dei longheroni
        ya, yb = y0 - 3.3, min(y0 + RAIL + 3.3, IY1 - 0.4)
        e = e.union(box(IX0 + 4.0, IX0 + 18.0, ya, yb, RIM_Z, RAIL_Z1 + 1.0))
        e = e.cut(box(IX0 + 4.0 - 0.01, IX0 + 18.01, y0 - 0.25, y0 + RAIL + 0.25, RAIL_Z1 - RAIL - 0.3, RAIL_Z1 + 1.01))
    e = e.union(box(IX0 + 4.0, IX0 + 18.0, RAIL_F_Y0 + RAIL + 3.3, RAIL_R_Y0 - 3.3, RIM_Z + 4.0, RIM_Z + 8.0))   # costola
    for y in (RAIL_F_Y0 + 37.0, RAIL_R_Y0 - 28.0):
        e = e.cut(xcyl(y, RIM_Z - 26.0, 1.75, xo - 0.01, TUB_X0))
    return e if side < 0 else e.mirror("YZ", ((IX0 + IX1) / 2, 0, 0))

# ------------------------------------------------------------------ C06 portasonde
def probe_holder():
    """Portasonde avvitato alla faccia anteriore del longherone anteriore: un braccio porta in avanti il pescante dell'acqua
    del bagno (a frizione: si spinge giu' per pescare piu' in basso) e la sonda DS18B20 del bagno. Si stampa col braccio sul piatto."""
    import mrefs as R
    x0, x1 = WB_X - 10.0, R.PROBE_X + 9.0; za, zb = 160.0, 168.0
    h = box(x0, x1, Y_PL0, RAIL_F_Y0, za, RAIL_Z1 - 0.5)
    h = h.union(box(x0, x1, Y_WB - 9.0, Y_PL0, za, zb))
    for x, r in ((WB_X, PIPE_R + 0.05), (R.PROBE_X, 3.15)):
        h = h.cut(zcyl(x, Y_WB, r, za - 0.01, zb + 0.01)).cut(box(x - 0.6, x + 0.6, Y_WB - 9.01, Y_WB, za - 0.01, zb + 0.01))
    for x in (x0 + 6.0, x1 - 6.0):
        h = h.cut(ycyl(x, Z_RS, M3_CLR, Y_PL0 - 0.01, RAIL_F_Y0 + 0.01))
    return h

# ------------------------------------------------------------------ C07 collettore
MAN_Y0, MAN_Y1, MAN_X0, MAN_X1 = VALVE_Y - 10.0, VALVE_Y + 10.0, 8.0, 198.0
MAN_ZC = 7.0
def manifold():
    """Collettore: cinque sedi verticali con O-ring per gli spezzoni di tubo delle valvole, un'uscita verso la pompa,
    canale interno a rombo (si stampa di piatto senza ponti). Due orecchie per le viti nella base."""
    xl = MAN_OUT[0] - 12.0
    m = box(MAN_X0, xl, MAN_Y0, MAN_Y1, 0, MAN_Z1).union(box(xl, MAN_X1, MAN_OUT[1] - 10.0, MAN_Y1, 0, MAN_Z1))
    d = 3.5
    m = m.cut(yz_prism([(VALVE_Y - d, MAN_ZC), (VALVE_Y, MAN_ZC + d), (VALVE_Y + d, MAN_ZC), (VALVE_Y, MAN_ZC - d)], VALVE_X[0], MAN_OUT[0]))
    m = m.cut(xz_prism([(MAN_OUT[0] - d, MAN_ZC), (MAN_OUT[0], MAN_ZC + d), (MAN_OUT[0] + d, MAN_ZC), (MAN_OUT[0], MAN_ZC - d)], MAN_OUT[1], VALVE_Y))
    for (x, y) in [(vx, VALVE_Y) for vx in VALVE_X] + [MAN_OUT]:
        m = m.cut(oring_bore(x, y, MAN_Z1, 14.0, MAN_Z1 - 8.0)).cut(zcyl(x, y, 3.2, MAN_ZC, MAN_Z1 - 13.99))
    for x in (40.0, 110.0):
        m = m.union(box(x - 6.0, x + 6.0, MAN_Y0 - 12.0, MAN_Y0, 0, 5.0)).cut(zcyl(x, MAN_Y0 - 6.0, 2.1, -0.01, 5.01))
    return m

# ------------------------------------------------------------------ C08 pettine valvole
def valve_comb():
    """Pettine valvole: schienale avvitato alla base dietro le cinque valvole; ogni valvola e' legata con una fascetta
    attorno all'attacco superiore (due asole). Si stampa di piatto sullo schienale."""
    y0, y1 = VALVE_Y + VALVE_BODY / 2 + 0.3, TUB_Y0 - 2.7
    x0, x1 = VALVE_X[0] - 19.0, VALVE_X[-1] + 19.0
    c = box(x0, x1, y0, y1, 0, 100.0)
    c = c.union(box(x0, x1, MAN_Y1 + 0.7, y0, 0, 4.0)).union(box(x0, x1, y0 - 4.3, y0, 96.0, 100.0))
    for vx in VALVE_X:
        for sg in (1, -1):
            c = c.cut(box(vx + sg * 12.0 - 1.0, vx + sg * 12.0 + 1.0, y0 - 0.01, y1 + 0.01, 86.0, 94.0))
    for x in [x0 + 4.5] + [vx + 17.5 for vx in VALVE_X[:-1]] + [x1 - 4.5]:
        c = c.cut(zcyl(x, (MAN_Y1 + 0.7 + y0) / 2, 1.9, -0.01, 4.01))
    return c

# ------------------------------------------------------------------ C09 piastra degli scarichi, C16 piastra dell'ingresso
def _plate(holes, y0, y1, z0, z1):
    from mrefs import PL_X0
    p = box(PL_X0, BASE_X, y0, y1, z0, z1)
    for (y, z, r) in holes:
        p = p.cut(xcyl(y, z, r, PL_X0 - 0.01, BASE_X + 0.01))
    for y in (y0 + 6.0, y1 - 6.0):
        for z in (z0 + 6.0, z1 - 6.0):
            p = p.cut(xcyl(y, z, M3_CLR, PL_X0 - 0.01, BASE_X + 0.01))
    return p

def drain_plate():
    """Piastra degli scarichi, dietro il pannello destro: passaparete SCARICO (foro Ø16.2), tubo in silicone del
    TROPPOPIENO della tank (Ø12.4) e tubo in PVC del TROPPOPIENO del bagno (Ø16.4). Quattro viti M3 col pannello."""
    from mrefs import CONN
    sy, sz = CONN["SCARICO"]; ty, tz = CONN["TROPPOPIENO_TANK"]; by, bz = CONN["TROPPOPIENO_BAGNO"]
    return _plate([(sy, sz, 8.1), (ty, tz, 8.5), (by, bz, 8.2)], 180.0, 252.0, 52.0, 162.0)

def inlet_plate():
    """Piastra dell'ingresso dell'acqua di rete, dietro il pannello destro: passaparete 3/8" (foro Ø16.2)."""
    from mrefs import CONN
    y, z = CONN["INGRESSO"]
    return _plate([(y, z, 8.1)], y - 22.0, y + 22.0, z - 22.0, z + 22.0)

# ------------------------------------------------------------------ C10 carter dei riscaldatori
def heater_cover(i=0):
    """Carter di un riscaldatore (uno per riscaldatore, uguali): copre testa e morsetti sul fianco destro del bagno; i cavi escono in basso."""
    xw = TUB_X0 + TUB_L + 0.6; y0, y1 = HEAT_Y[i] - 20.0, HEAT_Y[i] + 20.0; zt = HEAT_Z + 26.0
    c = box(BASE_X - 3.5, BASE_X - 0.5, y0, y1, 0, zt)
    c = c.union(box(xw, BASE_X - 0.5, y0, y0 + 3.0, 0, zt)).union(box(xw, BASE_X - 0.5, y1 - 3.0, y1, 0, zt))
    c = c.union(box(IX1 + TUB_WALL + 1.0, BASE_X - 0.5, y0, y1, zt - 3.0, zt))
    for ya, yb in ((y0 - 12.0, y0), (y1, y1 + 12.0)):
        c = c.union(box(xw, BASE_X - 0.5, ya, yb, 0, 3.0)).cut(zcyl((xw + BASE_X) / 2, (ya + yb) / 2, 2.1, -0.01, 3.01))
    return c.cut(box(xw - 0.01, xw + 8.0, y0 - 0.01, y0 + 3.01, 3.0, 12.0))

# ------------------------------------------------------------------ C19 gancio portasonda della chimica
PH_X, PH_W = 126.0, 14.0                     # asse e larghezza del gancio (tra il portasensore C04 e il portasonde C06)
def probe_hook():
    """C19: gancio che si appoggia a cavallo del longherone anteriore e tiene la sonda DS18B20 della chimica dentro la
    vaschetta C1: un labbro davanti al longherone, una lingua sottile sopra il bordo della vaschetta (sotto il coperchio)
    e una manica verticale con fessura in cui scorre il cavo. Si toglie a mano, con la sonda, prima di sollevare C1.
    Si stampa capovolto, con la lingua sul piatto."""
    x0, x1 = PH_X - PH_W / 2, PH_X + PH_W / 2
    yf, yr = RAIL_F_Y0, RAIL_F_Y0 + RAIL
    import mrefs as R
    xs, ys = R.probe_chem_xy()
    zt = Z_TOPW - 0.6                                   # 0.6 sotto il coperchio
    h = box(x0, x1, yf - 3.0, yf, RAIL_Z1 - 12.0, zt)                         # labbro davanti al longherone
    h = h.union(box(x0, x1, yf - 3.0, VAS_Y0 - 0.5, RAIL_Z1 + 0.2, zt))       # sopra il longherone, fino al bordo della vaschetta
    h = h.union(box(x0, x1, yf - 3.0, ys + 6.0, VAS_ZR + 0.3, zt))            # lingua sopra il bordo della vaschetta
    h = h.union(zcyl(xs, ys, 6.0, VAS_ZR - 40.0, zt))                         # manica
    h = h.cut(zcyl(xs, ys, 3.4, VAS_ZR - 40.01, zt + 0.01))                    # passa la sonda Ø6
    return h.cut(box(xs - 0.9, xs + 0.9, ys - 7.0, ys, VAS_ZR - 40.01, zt + 0.01))   # fessura del cavo

# ------------------------------------------------------------------ innesto dal basso: C11 corpo, C12 ghiera con codolo, C13 otturatore
# riferimento locale: asse Z dell'innesto, z = 0 sulla faccia esterna del fondo della vaschetta
FL_R, FL_T = 13.0, 1.8                      # flangia interna: Ø26, sporge 1.8 mm sul fondo (residuo di circa 15 ml)
TH_D, TH_P = 16.0, 2.0                      # filetto M16 x 2 stampato
SH_Z0 = -10.0                               # estremita' inferiore del gambo filettato
NUT_Z = (-10.0, -2.0)                       # dado esagonale da 24 sopra il codolo (stringe la guarnizione da 2 mm)
SP_R = 8.0                                  # codolo Ø16
SP_Z0 = -SPIG_L                             # punta del codolo: -34
BORE_P = 5.2                                # camera dell'otturatore Ø10.4
SEAT_Z = SP_Z0 + 4.7                        # inizio della sede conica a 45 gradi (da Ø10.4 a Ø7)
MOUTH_R = 3.5
GR_Z = (SP_Z0 + 6.0, SP_Z0 + 8.6)           # gola dell'O-ring 13 x 2 sul codolo (fondo Ø13.2)
GR_R = 6.6

def inlet_body_local():
    """C11: corpo dell'innesto, dentro la vaschetta: flangia sottile sul fondo con la crociera (appoggio della molla),
    gambo filettato M16 x 2 che attraversa il fondo. Si stampa capovolto, flangia sul piatto."""
    zf = GN9_WALL
    b = zcyl(0, 0, FL_R - 1.2, zf, zf + FL_T).union(cq.Workplane("XY").add(cq.Solid.makeCone(FL_R, FL_R - 1.2, 1.2)).translate((0, 0, zf)))
    b = b.union(zcyl(0, 0, FL_R, zf, zf + FL_T - 1.2))
    b = b.union(TH.ext_thread(TH_D, TH_P, zf - SH_Z0, SH_Z0))
    b = b.cut(zcyl(0, 0, 5.0, SH_Z0 - 0.01, zf + FL_T + 0.01))
    for a in (90.0, 210.0, 330.0):                                                          # crociera a tre bracci
        b = b.union(box(0, 5.2, -1.0, 1.0, zf, zf + FL_T).rotate((0, 0, 0), (0, 0, 1), a))
    b = b.union(zcyl(0, 0, 1.6, zf, zf + FL_T))
    return b

def nut_spigot_local():
    """C12: ghiera esagonale da 24 con filetto interno M16 x 2 e codolo Ø16 con la gola dell'O-ring 13 x 2; dentro, la camera
    dell'otturatore con la sede conica a 45 gradi e la bocca Ø7. Si stampa col dado sul piatto (codolo in su)."""
    n = cq.Workplane("XY").polygon(6, 24.0 / math.cos(math.radians(30))).extrude(NUT_Z[1] - NUT_Z[0]).translate((0, 0, NUT_Z[0]))
    n = n.edges("|Z").chamfer(0.8)
    n = n.union(zcyl(0, 0, SP_R, SP_Z0 + 1.0, NUT_Z[0] + 0.01))
    n = n.union(cq.Workplane("XY").add(cq.Solid.makeCone(SP_R - 1.0, SP_R, 1.0)).translate((0, 0, SP_Z0)))
    n = n.cut(TH.int_thread_cut(TH_D, TH_P, NUT_Z[1] - NUT_Z[0] + 0.02, NUT_Z[0] - 0.01))
    n = n.cut(zcyl(0, 0, BORE_P, SEAT_Z, NUT_Z[0] + 0.5))
    n = n.cut(cq.Workplane("XY").add(cq.Solid.makeCone(MOUTH_R, BORE_P, BORE_P - MOUTH_R)).translate((0, 0, SEAT_Z - (BORE_P - MOUTH_R))))
    n = n.cut(zcyl(0, 0, MOUTH_R, SP_Z0 - 0.01, SEAT_Z))
    g = zcyl(0, 0, SP_R + 0.1, GR_Z[0], GR_Z[1]).cut(zcyl(0, 0, GR_R, GR_Z[0] - 0.1, GR_Z[1] + 0.1))
    return n.cut(g)

POP_Z = SEAT_Z - (BORE_P - MOUTH_R) + 0.2   # fondo della guarnizione dell'otturatore chiuso (sul cono)
def poppet_local(open_mm=0.0):
    """C13: otturatore. Disco Ø10 con tre spianature (passaggio del liquido), O-ring 6 x 1.5 sotto il disco che chiude sulla
    sede conica, naso Ø3.8 che sporge dalla bocca e viene spinto dal perno della presa, codolo per la molla."""
    z0 = POP_Z + open_mm
    p = zcyl(0, 0, 3.0, z0, z0 + 1.5)                                                  # gambo dell'O-ring
    p = p.union(cq.Workplane("XY").add(cq.Solid.makeCone(3.0, 5.0, 2.0)).translate((0, 0, z0 + 1.5)))
    p = p.union(zcyl(0, 0, 5.0, z0 + 3.5, z0 + 6.0))
    for a in (0.0, 120.0, 240.0):
        p = p.cut(box(3.9, 6.0, -6.0, 6.0, z0 + 1.0, z0 + 6.1).rotate((0, 0, 0), (0, 0, 1), a))
    p = p.union(zcyl(0, 0, 2.8, z0 + 6.0, z0 + 10.0))                                   # codolo della molla
    return p.union(zcyl(0, 0, 1.9, SP_Z0 + open_mm, z0 + 0.01))                         # naso

def place_pan(p, k):
    return p.translate((VAS_X[k], Y_SOCK, VAS_ZB))

def oring_pop_local(open_mm=0.0):
    """O-ring 6 x 1.5 dell'otturatore."""
    z0 = POP_Z + open_mm
    return cq.Workplane("XY").add(cq.Solid.makeTorus(3.75, 0.75)).translate((0, 0, z0 + 0.75))

# ------------------------------------------------------------------ C14 presa sul fondo del bagno
SK_TOP = VAS_ZB + NUT_Z[0] - 2.0            # sommita' della presa: 2 mm sotto la ghiera
SK_BR = SP_R + 0.3                          # sede Ø16.6: l'O-ring 13 x 2 sul codolo Ø13.2 si schiaccia del 14 %
PIN_TOP = VAS_ZB + SP_Z0 + 3.5              # il perno spinge in su il naso di 3.5 mm
def socket(k):
    """C14: presa appoggiata sul fondo del bagno sotto la vaschetta k. Sede del codolo con invito largo (centra la vaschetta
    entro ±5 mm), perno che apre l'otturatore, canale verso il basso e sede a O-ring orizzontale per la linea (3/8").
    Pesante e larga: sta ferma da sola; la posizione la danno la vaschetta e il tubo. Si stampa in piedi."""
    x, y = VAS_X[k], Y_SOCK
    s = box(x - 20.0, x + 20.0, SK_Y0, y + 20.0, TUB_FLOOR, 26.0)
    s = s.edges("|Z").fillet(4.0)
    s = s.union(zcyl(x, y, 15.0, 25.0, SK_TOP))
    s = s.cut(zcyl(x, y, SK_BR, SK_Z0, SK_TOP + 0.01))
    lead = 5.0
    s = s.cut(cq.Workplane("XY").add(cq.Solid.makeCone(SK_BR, SK_BR + lead, lead)).translate((x, y, SK_TOP - lead + 0.01)))
    s = s.cut(zcyl(x, y, 3.5, LINE_Z - 1.3, SK_Z0 + 0.01))                              # canale verticale
    s = s.cut(ycyl(x, LINE_Z, 3.2, SK_Y0 + 13.0, y + 1.1))                              # canale orizzontale
    s = s.cut(cq.Workplane("XY").add(cq.Solid.makeSphere(3.5)).translate((x, y, LINE_Z)))
    s = s.cut(oring_bore_dir((x, SK_Y0, LINE_Z), (0, 1, 0), 14.0, 8.0))
    for a in (90.0, 210.0, 330.0):                                                      # crociera del perno
        s = s.union(box(0, 3.6, -0.8, 0.8, SK_Z0 - 2.0, SK_Z0).rotate((0, 0, 0), (0, 0, 1), a).translate((x, y, 0)))
    return s.union(zcyl(x, y, 1.8, SK_Z0 - 2.0, PIN_TOP))

# ------------------------------------------------------------------ C20 imbuto di svuotamento delle vaschette
def drain_funnel_local():
    """C20: imbuto di svuotamento, nel riferimento della vaschetta (z = 0 sulla faccia esterna del fondo). Si infila nel
    collo della bottiglia della chimica; ci si appoggia sopra la vaschetta tolta dal bagno: il codolo entra nella sede
    (stesso invito e stesso O-ring della presa C14), la ghiera appoggia sulla faccia superiore e il perno apre
    l'otturatore. Si alza la vaschetta e l'otturatore si richiude. Due scanalature sul cono fanno uscire l'aria dalla
    bottiglia. Uno solo serve per tutte e tre le vaschette. Si stampa cosi', beccuccio sul piatto."""
    zt, zb = NUT_Z[0], SP_Z0 - 3.0                       # faccia d'appoggio (sotto la ghiera) e fondo della sede
    zc = zb - 4.0                                        # inizio del cono che entra nel collo della bottiglia
    f = zcyl(0, 0, 15.0, zc, zt)
    f = f.union(cq.Workplane("XY").add(cq.Solid.makeCone(6.6, 15.0, 22.0)).translate((0, 0, zc - 22.0)))   # cono continuo col beccuccio: niente sbalzi
    f = f.union(zcyl(0, 0, 6.5, zc - 40.0, zc - 21.99))                                # beccuccio
    f = f.cut(zcyl(0, 0, SK_BR, zb, zt + 0.01))
    lead = 5.0
    f = f.cut(cq.Workplane("XY").add(cq.Solid.makeCone(SK_BR, SK_BR + lead, lead)).translate((0, 0, zt - lead + 0.01)))
    f = f.cut(zcyl(0, 0, 4.2, zc - 40.01, zb + 0.01))                                  # canale di uscita
    for a in (90.0, 210.0, 330.0):                                                      # crociera del perno
        f = f.union(box(0, 4.3, -0.8, 0.8, zb - 2.0, zb).rotate((0, 0, 0), (0, 0, 1), a))
    f = f.union(zcyl(0, 0, 1.8, zb - 2.0, SP_Z0 + 3.5))                                 # perno: apre l'otturatore di 3.5 mm
    for a in (0.0, 180.0):                                                              # scanalature per l'aria
        f = f.cut(box(5.6, 16.0, -1.2, 1.2, zc - 40.01, zc - 2.0).rotate((0, 0, 0), (0, 0, 1), a))
    return f

# ------------------------------------------------------------------ C15 camera di livello
CH_RI = 10.0
def chamber(k):
    """C15: camera di livello sopra la valvola Ck, in PETG trasparente: sede a O-ring sotto (spezzone dalla valvola) e sopra
    (linea della vaschetta), faccia piana verso il fronte con il sensore XKC-Y21 (MIN: si svuota quando la vaschetta e'
    vuota e la pompa aspira aria). Due asole per la fascetta del sensore. Si stampa in piedi."""
    import mrefs as R
    x, y = VALVE_X[k], VALVE_Y
    c = zcyl(x, y, CH_R, CH_Z0, CH_Z1).cut(box(x - CH_R, x + CH_R, y - CH_R - 0.01, R.CHS_Y1, CH_Z0 - 0.01, CH_Z1 + 0.01))
    zc0, zc1 = CH_Z0 + 18.0, CH_Z1 - 18.0
    cav = zcyl(x, y, CH_RI, zc0, zc1)
    cav = cav.union(cq.Workplane("XY").add(cq.Solid.makeCone(3.2, CH_RI, CH_RI - 3.2)).translate((x, y, zc0 - (CH_RI - 3.2))))
    cav = cav.union(cq.Workplane("XY").add(cq.Solid.makeCone(CH_RI, 3.2, CH_RI - 3.2)).translate((x, y, zc1)))
    cav = cav.translate((0, 1.0, 0))
    c = c.cut(cav)
    c = c.cut(zcyl(x, y + 1.0, 3.2, CH_Z0 + 13.0, zc0)).cut(zcyl(x, y + 1.0, 3.2, zc1, CH_Z1 - 13.0))
    c = c.cut(oring_bore(x, y, CH_Z1, 14.0, CH_Z1 - 8.0))
    c = c.cut(oring_bore(x, y, CH_Z0 + 14.0, 14.0, CH_Z0 + 8.0 - GROOVE_H).mirror("XY", (0, 0, CH_Z0 + 7.0)))
    for sg in (1, -1):
        c = c.cut(box(x + sg * 9.0 - 1.0, x + sg * 9.0 + 1.0, y + 4.0, y + 8.0, CH_Z0 + 22.0, CH_Z1 - 22.0))   # asole della fascetta
    return c

# ------------------------------------------------------------------ C17 staffa del beccuccio, C18 staffa a fascetta
def spout_clip():
    """C17: staffa a cavallo della fascia anteriore del bagno che tiene in verticale il beccuccio dell'acqua di rete, 30 mm
    sopra il troppopieno (salto d'aria: l'acqua del bagno non puo' tornare nella rete)."""
    import mrefs as R
    x, y = R.SPOUT
    c = box(x - 8.0, x + 8.0, TUB_Y0 - 3.0, IY0 + 1.0, RIM_Z, RIM_Z + 4.0)
    c = c.union(box(x - 8.0, x + 8.0, TUB_Y0 - 3.0, TUB_Y0 - 0.3, RIM_Z - 12.0, RIM_Z))
    c = c.union(box(x - 8.0, x + 8.0, IY0 + 0.3, IY0 + 3.0, RIM_Z - 12.0, RIM_Z + 4.0))
    c = c.union(box(x - 5.0, x + 5.0, IY0 + 0.3, y, RIM_Z - 4.0, RIM_Z + 4.0)).union(zcyl(x, y, ELB_R + 2.5, RIM_Z - 4.0, RIM_Z + 4.0))
    return c.cut(zcyl(x, y, ELB_R + 0.2, RIM_Z - 4.01, RIM_Z + 4.01))

def tie_bracket(x, y, w, z1):
    """C18: staffa a fascetta avvitata sotto il piano del cavalletto (due viti 3 x 8): una fascetta da 4,8 mm passa nelle
    due asole e stringe il corpo (flussimetro o valvola dell'ingresso) contro la staffa."""
    b = box(x - 20.0, x + 20.0, y - w / 2 - 6.0, y + w / 2 + 6.0, z1 - 4.0, z1)
    for sg in (1, -1):
        b = b.cut(box(x - 3.0, x + 3.0, y + sg * (w / 2 + 2.0) - 1.5, y + sg * (w / 2 + 2.0) + 1.5, z1 - 4.01, z1 + 0.01))
        b = b.cut(zcyl(x + sg * 14.0, y, M3_CLR, z1 - 4.01, z1 + 0.01))
    return b

def tie_brackets():
    import mrefs as R
    zb = Z_STAND - DECK_T
    return [tie_bracket(R.FS_X1 - 11.0, R.IN_Y0, 35.0, zb), tie_bracket(R.IV_X0 + VALVE_LEN / 2, R.IN_Y1, 33.0, zb)]

PARTS = [("C03_fermo_girevole", turn_button_local), ("C04_portasensore_MAX", lambda: sensor_holder(0)),
         ("C05_testata_telaio_SX", lambda: frame_end(-1)), ("C05_testata_telaio_DX", lambda: frame_end(1)), ("C06_portasonde", probe_holder),
         ("C07_collettore", manifold), ("C08_pettine_valvole", valve_comb), ("C09_piastra_scarichi", drain_plate), ("C10_carter_riscaldatori", heater_cover),
         ("C11_corpo_innesto", inlet_body_local), ("C12_ghiera_codolo", nut_spigot_local), ("C13_otturatore", poppet_local),
         ("C14_presa", lambda: socket(0)), ("C15_camera_livello", lambda: chamber(0)), ("C16_piastra_ingresso", inlet_plate),
         ("C17_staffa_beccuccio", spout_clip), ("C18_staffa_fascetta", lambda: tie_brackets()[0])]

if __name__ == "__main__":
    import time
    for n, f in PARTS:
        t = time.time(); s = f().val(); bb = s.BoundingBox()
        print(f"{n:26s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:6.2f} cm3  {bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f}  ({time.time() - t:.0f} s)")
