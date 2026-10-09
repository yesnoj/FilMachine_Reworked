"""Parti stampate del bagno e dell'idraulica (serie C), nel riferimento macchina.
C01 sella della testina, C02 culla del sifone, C03 staffa con ganci a scatto, C04 fermo posteriore, C05 testata del telaio,
C06 portasonde, C07 collettore, C08 pettine valvole, C09 staffa scarichi, C10 carter dei riscaldatori."""
import math
import cadquery as cq
from machine import *

GROOVE_R = PIPE_R + 0.95 * ORING_CS          # fondo della gola dell'O-ring 9x2 (schiacciamento ~5 %)
GROOVE_H = 2.6
BORE_R = 4.95                                # sede del tubo da 9.52
Y_PL0 = RAIL_F_Y0 - 4.0                      # faccia anteriore delle staffe avvitate al longherone anteriore: 289
Z_RS = RIM_Z - RAIL / 2                      # quota delle viti nel longherone: 162.5

def oring_bore(x, y, z_top, depth, z_groove):
    """Sede verticale per un tubo da 9.52 con gola per O-ring 9x2 e invito in alto."""
    b = zcyl(x, y, BORE_R, z_top - depth, z_top + 0.01)
    b = b.union(zcyl(x, y, GROOVE_R, z_groove, z_groove + GROOVE_H))
    cone = cq.Workplane("XY").add(cq.Solid.makeCone(BORE_R, BORE_R + 1.2, 1.2)).translate((x, y, z_top - 1.19))
    return b.union(cone)

# ------------------------------------------------------------------ C01 sella della testina
SAD_HW = 20.0
PAD_Y1 = Y_PL0 + 5.5                           # faccia dei tasselli dei ganci: ferma la sella (e la vaschetta) in avanti
def saddle(k=0):
    """Sella della testina: appoggia sul bordo anteriore della vaschetta, tiene i due gomiti (fascetta attorno alla
    culla nel tratto a sbalzo) e offre ai ganci a scatto i due angoli anteriori. Due fori Ø6.4 per la sonda DS18B20.
    Si stampa in piedi sui due blocchetti posteriori."""
    x = VAS_X[k]; zt = VAS_ZR
    y_f = PAD_Y1 + 0.3; y_l0 = VAS_Y0 - 2.7; y_in = VAS_Y0 + GN9_RIM + GN9_WALL; y_p1 = y_in + 3.0
    s = box(x - SAD_HW, x + SAD_HW, y_f, y_p1, zt, zt + 3.0)
    s = s.union(yz_prism([(y_l0, zt + 0.01), (VAS_Y0 - 0.3, zt + 0.01), (y_l0, zt - 2.4)], x - SAD_HW, x + SAD_HW))   # labbro a cuneo
    y_c0 = Y_STUB + ELB_R + 1.5
    s = s.union(box(x - 11.0, x + 11.0, y_c0, y_p1, zt + 3.0, zt + 5.0))                                   # fondo della culla
    for sg in (1, -1):
        w = yz_prism([(y_c0, zt + 4.9), (y_c0, zt + 14.5), (y_p1 - 11.5, zt + 14.5), (y_p1, zt + 3.0)],
                     min(sg * 8.7, sg * 11.0) + x, max(sg * 8.7, sg * 11.0) + x)                             # fianchi con rampa a 45 gradi
        s = s.union(w)
        b = box(x + min(sg * 10.0, sg * SAD_HW), x + max(sg * 10.0, sg * SAD_HW), y_in + 0.5, y_in + 14.0, zt - 9.0, zt + 3.0)
        s = s.union(b).cut(zcyl(x + sg * 15.0, y_in + 9.5, 3.2, zt - 9.01, zt + 3.01))
    s = s.cut(box(x - 9.2, x + 9.2, y_in + 0.4, y_p1 + 0.01, zt - 0.01, zt + 5.01))                         # passaggio del gomito interno
    return s

# ------------------------------------------------------------------ C02 culla del sifone
SIPH_RO = U_R + HOSE_R + 2.8
GROOVE_W, GROOVE_W2 = HOSE_R + 0.3, HOSE_R + 1.3   # semi-larghezza della gola: tubo 8x12 libero / calzato sul tubo da 9.52 (Ø13.5)
ROOF_K = math.tan(math.radians(48.0))              # pendenza delle falde del cielo a capanna (48 gradi dal piatto: 42 di sbalzo)
ROOF_D = (PIPE_R + 2.0) * math.hypot(1.0, ROOF_K) + 0.04   # colmo dall'asse del tubo: il tratto calzato (raggio 6.76) tocca le due falde con l'asse su Y_STUB
CR_HOLES = (-7.0, 5.0)                             # viti staffa-culla rispetto all'asse della vaschetta (lontane dal bordo del distanziale)
def u_prism(xc, zc, r_out, r_in, zt, y0, y1):
    """Prisma con sezione a U nel piano XZ (due tratti diritti fino a zt raccordati da un mezzo anello di centro
    (xc, zc)), estruso da y0 a y1. Con r_in = 0 la U e' piena."""
    w = cq.Workplane("XZ").moveTo(xc - r_out, zt).lineTo(xc - r_out, zc).threePointArc((xc, zc - r_out), (xc + r_out, zc)).lineTo(xc + r_out, zt)
    if r_in > 0:
        w = w.lineTo(xc + r_in, zt).lineTo(xc + r_in, zc).threePointArc((xc, zc - r_in), (xc - r_in, zc)).lineTo(xc - r_in, zt)
    return w.close().extrude(-(y1 - y0)).translate((0, y0, 0))

def u_tent(xc, zc, r_mid, w, z0, z1, y0, yr, arc=True):
    """Gola a U con cielo a capanna: sezione a pentagono (larga 2w, pareti diritte da y0, due falde di pendenza ROOF_K
    fino al colmo yr) che segue due tratti diritti (da z0 a z1, a distanza r_mid dal centro) e, se arc, il mezzo anello
    che li raccorda. Nessun ponte: stampata col davanti sul piatto la gola si chiude da sola."""
    pen = lambda xm: [(xm - w, y0), (xm + w, y0), (xm + w, yr - ROOF_K * w), (xm, yr), (xm - w, yr - ROOF_K * w)]
    t = None
    for sx in (1, -1):
        leg = cq.Workplane("XY").polyline(pen(xc + sx * r_mid)).close().extrude(z1 - z0).translate((0, 0, z0))
        t = leg if t is None else t.union(leg)
    if arc:
        a = cq.Workplane("XY").polyline(pen(r_mid)).close().revolve(180, (0, 0, 0), (0, 1, 0))      # mezzo giro attorno a Y
        if a.val().BoundingBox().zmax > 1.0: a = a.mirror("XY")
        t = t.union(a.translate((xc, 0, zc)))
    return t

def siphon_holder(k=0):
    """Culla del sifone: tiene piegato a U lo spezzone di silicone 8x12 (premuto nella gola dal davanti, due fascette)
    e ne presenta la bocca al codolo della testina. Nessuna parte stampata tocca la chimica. La gola ha il cielo a
    capanna: si stampa col davanti sul piatto, senza ponti, e le fascette centrano il tubo nella V."""
    x = VAS_X[k]; xc = x - U_R; zc = Z_MOUTH - 10.0; zt = Z_MOUTH - 2.0
    yf, yr = Y_STUB - 3.0, Y_STUB + ROOF_D; yb1 = yr + 2.0
    c = u_prism(xc, zc, SIPH_RO, 0.0, zt, yf, yb1)
    g = u_tent(xc, zc, U_R, GROOVE_W, zc, zt + 1.0, yf - 1.0, yr)                                           # gola del tubo
    g = g.union(u_tent(xc, zc, U_R, GROOVE_W2, zt - 10.0, zt + 1.0, yf - 1.0, yr, arc=False))               # in cima e' piu' larga: li' il silicone e' calzato sul tubo da 9.52
    assert len(g.val().Solids()) == 1 and g.val().isValid(), "culla del sifone: gola non valida"
    c = c.cut(g)
    c = c.union(box(x - TRAP_HW, x + SIPH_RO - U_R, yb1, Y_PL0, LP_Z0, zt).intersect(u_prism(xc, zc, SIPH_RO, 0.0, zt, yb1, Y_PL0)))   # distanziale verso la staffa, dentro il contorno della U
    for xl in (x - 10.3, x - 2 * U_R + 10.3):                                                               # asole per le fascette
        c = c.cut(box(xl - 0.9, xl + 0.9, yf - 0.01, yb1 + 0.01, zt - 7.0, zt - 3.0))
    for dx in CR_HOLES:
        c = c.cut(ycyl(x + dx, LP_Z0 + 15.0, M3_TAP, Y_PL0 - 9.0, Y_PL0 + 0.01))
    assert len(c.val().Solids()) == 1 and c.val().isValid(), "culla del sifone non valida"
    return c

# ------------------------------------------------------------------ C03 staffa con ganci a scatto
TRAP_HW = 12.0
BARB_Z = VAS_ZR + 3.3                         # piano di presa dei ganci (0.3 mm sopra la sella)
def latch(k=0):
    """Staffa avvitata sulla faccia anteriore del longherone: porta la culla del sifone (due viti da dietro) e i due
    ganci a scatto che trattengono la sella, cioe' la vaschetta, contro la spinta di galleggiamento; i tasselli sotto
    i denti la fermano anche in avanti. Si stampa di piatto sulla faccia anteriore."""
    x = VAS_X[k]
    p = box(x - TRAP_HW, x + TRAP_HW, Y_PL0, RAIL_F_Y0, LP_Z0, RIM_Z)
    p = p.union(box(x - SAD_HW, x + SAD_HW, Y_PL0, RAIL_F_Y0, LP_Z0, LP_Z0 + 16.0))
    for sg in (1, -1):
        x0, x1 = x + min(sg * 13.0, sg * SAD_HW), x + max(sg * 13.0, sg * SAD_HW)
        p = p.union(box(x0, x1, Y_PL0, Y_PL0 + 2.5, LP_Z0 + 16.0, BARB_Z + 15.7))                           # braccio elastico
        p = p.union(yz_prism([(Y_PL0 + 2.5, RIM_Z + 1.0), (PAD_Y1, RIM_Z + 1.0), (PAD_Y1, BARB_Z), (PAD_Y1 + 4.5, BARB_Z),
                              (PAD_Y1 + 4.5, BARB_Z + 0.8), (Y_PL0 + 2.5, BARB_Z + 8.3)], x0, x1))           # tassello + dente con invito a 45 gradi
        p = p.cut(ycyl(x + sg * 6.0, Z_RS, M3_CLR, Y_PL0 - 0.01, RAIL_F_Y0 + 0.01))                          # viti nel longherone
    for dx in CR_HOLES:                                                                                     # viti della culla del sifone, da dietro
        p = p.cut(ycyl(x + dx, LP_Z0 + 15.0, M3_CLR, Y_PL0 - 0.01, RAIL_F_Y0 + 0.01))
        p = p.cut(ycyl(x + dx, LP_Z0 + 15.0, 3.1, RAIL_F_Y0 - 1.8, RAIL_F_Y0 + 0.01))
    return p

# ------------------------------------------------------------------ C04 fermo posteriore
LIP = 3.0                                     # sovrapposizione del labbro sul bordo posteriore della vaschetta
def keeper(k=0):
    """Fermo posteriore: labbro fisso sotto cui si infila il bordo posteriore della vaschetta. Due viti nel longherone."""
    x = VAS_X[k]; y0, y1 = VAS_Y1 + 0.4, IY1 - 0.4
    f = box(x - SAD_HW, x + SAD_HW, y0, y1, RIM_Z, VAS_ZR + 4.0)
    f = f.union(yz_prism([(VAS_Y1 - LIP, VAS_ZR + 1.2), (y0, VAS_ZR + 0.5), (y0, VAS_ZR + 4.0), (VAS_Y1 - LIP, VAS_ZR + 4.0)], x - SAD_HW, x + SAD_HW))
    f = f.union(box(x - SAD_HW, x + SAD_HW, RAIL_R_Y0 + RAIL + 0.3, y1, RIM_Z - 14.0, RIM_Z))
    for sg in (1, -1):
        f = f.cut(zcyl(x + sg * 12.0, y0 + 2.5, M3_CLR, RIM_Z - 0.01, VAS_ZR + 4.01))
    return f

# ------------------------------------------------------------------ C05 testata del telaio
def frame_end(side=-1):
    """Testata del telaio portavaschette (side = -1 sinistra, +1 destra): a cavallo della fascia del contenitore, con
    le due tasche in cui si infilano i longheroni e due viti M4 che passano sotto la fascia. Si stampa capovolta."""
    xo = TUB_X0 - 3.5
    e = box(xo, IX0 + 18.0, RAIL_F_Y0 - 8.0, IY1 - 0.4, RIM_Z, RIM_Z + 4.0)                                   # piastra sulla fascia
    e = e.union(box(IX0 + 0.3, IX0 + 4.0, RAIL_F_Y0 - 8.0, IY1 - 0.4, RIM_Z - 20.0, RIM_Z))                  # labbro interno
    e = e.union(box(xo, TUB_X0 - 0.3, RAIL_F_Y0 + 7.0, RAIL_R_Y0 + 2.0, RIM_Z - 34.0, RIM_Z))                # labbro esterno
    for y0 in (RAIL_F_Y0, RAIL_R_Y0):                                                                       # tasche dei longheroni
        ya, yb = y0 - 3.3, min(y0 + RAIL + 3.3, IY1 - 0.4)
        e = e.union(box(IX0 + 4.0, IX0 + 18.0, ya, yb, RIM_Z - 18.3, RIM_Z))
        e = e.cut(box(IX0 + 4.0 - 0.01, IX0 + 18.01, y0 - 0.25, y0 + RAIL + 0.25, RIM_Z - RAIL - 0.3, RIM_Z + 0.01))
    for y in (RAIL_F_Y0 + 37.0, RAIL_R_Y0 - 28.0):
        e = e.cut(xcyl(y, RIM_Z - 26.0, 1.75, xo - 0.01, TUB_X0))
    return e if side < 0 else e.mirror("YZ", ((IX0 + IX1) / 2, 0, 0))

# ------------------------------------------------------------------ C06 portasonde
def probe_holder():
    """Portasonde sul longherone anteriore: tiene il pescante dell'acqua del bagno (a frizione: si spinge giu' per
    svuotare del tutto) e la sonda DS18B20 del bagno. Si stampa col braccio sul piatto."""
    x = WB_X
    h = box(x - 9.5, x + 9.5, Y_PL0, RAIL_F_Y0, RIM_Z - 30.0, RIM_Z)
    h = h.union(box(x - 8.0, x + 8.0, Y_STUB - 26.0, Y_PL0, RIM_Z - 30.0, RIM_Z - 22.0))
    for y, r in ((Y_STUB, PIPE_R + 0.05), (Y_STUB - 16.0, 3.15)):
        h = h.cut(zcyl(x, y, r, RIM_Z - 30.01, RIM_Z - 21.99)).cut(box(x, x + 8.01, y - 0.6, y + 0.6, RIM_Z - 30.01, RIM_Z - 21.99))
    for sg in (1, -1):
        h = h.cut(ycyl(x + sg * 5.5, Z_RS, M3_CLR, Y_PL0 - 0.01, RAIL_F_Y0 + 0.01))
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

# ------------------------------------------------------------------ C09 staffa scarichi
DR_X0, DR_Y0, DR_Y1 = BASE_X - 14.0, 176.0, 221.5
def drain_bracket():
    """Staffa scarichi sul fianco destro: foro Ø16.4 per il passaparete 3/8" dello SCARICO (pompa) e foro Ø13.6 per il
    tubo del TROPPOPIENO (gravita'), che arriva in leggera discesa dall'innesto sotto il piano del cavalletto."""
    from mrefs import OVF_YM, Z_OVF_HOSE
    d = box(DR_X0, DR_X0 + 4.0, DR_Y0, DR_Y1, 0, Z_LANE_WASTE + 14.0).union(box(DR_X0, BASE_X, DR_Y0, DR_Y1, 0, 4.0))
    for y0 in (DR_Y0, DR_Y1 - 3.0):
        d = d.union(xz_prism([(DR_X0 + 3.99, 3.99), (BASE_X, 3.99), (DR_X0 + 3.99, 3.99 + 10.0)], y0, y0 + 3.0))
    d = d.cut(xcyl(Y_RECESS, Z_LANE_WASTE, 8.2, DR_X0 - 0.01, DR_X0 + 4.01)).cut(xcyl(OVF_YM, Z_OVF_HOSE, 6.8, DR_X0 - 0.01, DR_X0 + 4.01))
    for y in (DR_Y0 + 12.0, DR_Y1 - 12.0):
        d = d.cut(zcyl(BASE_X - 5.0, y, 2.1, -0.01, 4.01))
    return d

# ------------------------------------------------------------------ C10 carter dei riscaldatori
def heater_cover():
    """Carter dei riscaldatori: copre le teste e i morsetti sul fianco destro del bagno; i cavi escono in basso."""
    xw = TUB_X0 + TUB_L + 0.6; y0, y1 = HEAT_Y[0] - 20.0, HEAT_Y[1] + 20.0
    c = box(BASE_X - 3.5, BASE_X - 0.5, y0, y1, 0, 52.0)
    c = c.union(box(xw, BASE_X - 0.5, y0, y0 + 3.0, 0, 52.0)).union(box(xw, BASE_X - 0.5, y1 - 3.0, y1, 0, 52.0))
    c = c.union(box(IX1 + TUB_WALL + 1.0, BASE_X - 0.5, y0, y1, 49.0, 52.0))
    for ya, yb in ((y0 - 12.0, y0), (y1, y1 + 12.0)):
        c = c.union(box(xw, BASE_X - 0.5, ya, yb, 0, 3.0)).cut(zcyl((xw + BASE_X) / 2, (ya + yb) / 2, 2.1, -0.01, 3.01))
    return c.cut(box(xw - 0.01, xw + 8.0, y0 - 0.01, y0 + 3.01, 3.0, 12.0))

PARTS = [("C01_sella_testina", saddle), ("C02_culla_sifone", siphon_holder), ("C03_staffa_ganci", latch), ("C04_fermo_posteriore", keeper),
         ("C05_testata_telaio_SX", lambda: frame_end(-1)), ("C05_testata_telaio_DX", lambda: frame_end(1)), ("C06_portasonde", probe_holder),
         ("C07_collettore", manifold), ("C08_pettine_valvole", valve_comb), ("C09_staffa_scarichi", drain_bracket), ("C10_carter_riscaldatori", heater_cover)]

if __name__ == "__main__":
    for n, f in PARTS:
        s = f().val(); bb = s.BoundingBox()
        print(f"{n:26s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:6.1f} cm3  {bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f}")
