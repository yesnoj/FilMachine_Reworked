"""Parti stampate dell'idraulica e del bagno (serie C), nel riferimento macchina - rilascio 7.
C01 collettore a sei bocche orizzontali, C02 culla delle valvole, C03 camera di livello con bocca laterale, C04 culla
del flussimetro, C05 portasonda sulla parete posteriore, C06 sella dei riscaldatori, C07 culla della valvola dell'ingresso,
C08 culla della Kamoer, C09 fermo dell'alimentatore in piedi, C10 piede della mainboard, C11 guida superiore della
mainboard, C12 cornice del display, C13 colonna del piano, C14 staffa del beccuccio e del troppopieno."""
import math
import cadquery as cq
from machine import *
import mrefs as R

GROOVE_R = PIPE_R + 0.95 * 2.0               # fondo della gola dell'O-ring 9x2 (schiacciamento ~5 %)
GROOVE_H = 2.6
BORE_R = 4.95                                # sede del tubo da 9.52

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

# ------------------------------------------------------------------ C01 collettore
MAN_FEET = (MAN_X0 + 20.0, (MAN_X0 + MAN_X1) / 2, MAN_X1 - 20.0)
def manifold():
    """Collettore: barra lungo X sulla base; sei sedi orizzontali con O-ring sulla faccia anteriore (gli spezzoni delle
    valvole), una sull'estremita' sinistra verso la pompa; canale interno a rombo lungo X. Tre orecchie per le viti nella
    base. Si stampa di piatto (il rombo e' un tetto a 45 gradi)."""
    m = box(MAN_X0, MAN_X1, MAN_Y0, MAN_Y1, MAN_Z0, MAN_Z1)
    d = 3.5
    m = m.cut(yz_prism([(MAN_OUT_Y - d, VALVE_Z), (MAN_OUT_Y, VALVE_Z + d), (MAN_OUT_Y + d, VALVE_Z), (MAN_OUT_Y, VALVE_Z - d)], MAN_X0 + 14.0, VALVE_X[2]))
    for x in VALVE_X:
        m = m.cut(oring_bore_dir((x, MAN_Y0, VALVE_Z), (0, 1, 0), 14.0, 8.0)).cut(ycyl(x, VALVE_Z, 3.2, MAN_Y0 + 13.5, MAN_OUT_Y))
    m = m.cut(oring_bore_dir((MAN_X0, MAN_OUT_Y, VALVE_Z), (1, 0, 0), 14.0, 8.0))
    for x in MAN_FEET:
        m = m.union(box(x - 6.0, x + 6.0, MAN_Y0, MAN_Y1 + 12.0, 0, MAN_Z0 + 0.01))                 # gambe fino alla base, con l'orecchia dietro
        m = m.cut(zcyl(x, MAN_Y1 + 6.0, 2.1, -0.01, MAN_Z0 + 0.02))
    return m

# ------------------------------------------------------------------ C02 culla delle valvole
VC_Y0, VC_Y1 = VALVE_Y0 + 12.0, VALVE_Y1 - 12.0
def valve_cradle():
    """Culla delle sei valvole sdraiate: piastra sulla base con due traversine di appoggio sotto i corpi e, per ogni valvola,
    due coppie di asole per le fascette. Quattro viti nella base. Si stampa di piatto."""
    zt = VALVE_Z - VALVE_BODY / 2 - 0.3
    x0, x1 = VALVE_X[3] - 20.0, VALVE_X[2] + 20.0
    c = box(x0, x1, VC_Y0, VC_Y1, 0, 3.0)
    for y in (VC_Y0 + 6.0, VC_Y1 - 6.0):
        c = c.union(box(x0, x1, y - 4.0, y + 4.0, 0, zt))
    for x in VALVE_X:
        for y in (VC_Y0 + 6.0, VC_Y1 - 6.0):
            for sg in (1, -1):
                c = c.cut(box(x + sg * 18.5 - 1.0, x + sg * 18.5 + 1.0, y - 4.01, y + 4.01, 3.0, zt + 0.01))      # asole delle fascette
    for (x, y) in ((x0 + 8.0, (VC_Y0 + VC_Y1) / 2), (x1 - 8.0, (VC_Y0 + VC_Y1) / 2), ((x0 + x1) / 2, VC_Y0 + 20.0), ((x0 + x1) / 2, VC_Y1 - 20.0)):
        c = c.cut(zcyl(x, y, 2.1, -0.01, 3.01))
    return c

# ------------------------------------------------------------------ C03 camera di livello
CH_RI = 10.0
CHS_Y1 = CY - CH_R + 2.0                     # faccia piana verso il fronte (sensore XKC-Y21)
def chamber(k):
    """C03: camera di livello sopra il gomito della valvola Ck, in PETG trasparente: sede a O-ring sotto (spezzone dal gomito),
    sopra (spezzone verso il raccordo del passaparete) e di lato verso +X (adattatore per il silicone del rubinetto); faccia
    piana verso il fronte con il sensore XKC-Y21 (MIN: si svuota quando il contenitore e' vuoto e la pompa aspira aria).
    Due asole per la fascetta del sensore. Si stampa in piedi."""
    x, y = VALVE_X[k], CY
    c = zcyl(x, y, CH_R, CH_Z0, CH_Z1).cut(box(x - CH_R, x + CH_R, y - CH_R - 0.01, CHS_Y1, CH_Z0 - 0.01, CH_Z1 + 0.01))
    c = c.union(box(x, x + CH_R, y - 9.0, y + 9.0, CH_SIDE_Z - 9.0, CH_SIDE_Z + 9.0))               # ringrosso della bocca laterale
    zc0, zc1 = CH_Z0 + 18.0, CH_Z1 - 18.0
    cav = zcyl(x, y, CH_RI, zc0, zc1)
    cav = cav.union(cq.Workplane("XY").add(cq.Solid.makeCone(3.2, CH_RI, CH_RI - 3.2)).translate((x, y, zc0 - (CH_RI - 3.2))))
    cav = cav.union(cq.Workplane("XY").add(cq.Solid.makeCone(CH_RI, 3.2, CH_RI - 3.2)).translate((x, y, zc1)))
    cav = cav.translate((0, 1.0, 0))
    c = c.cut(cav)
    c = c.cut(zcyl(x, y + 1.0, 3.2, CH_Z0 + 13.0, zc0)).cut(zcyl(x, y + 1.0, 3.2, zc1, CH_Z1 - 13.0))
    c = c.cut(oring_bore(x, y, CH_Z1, 14.0, CH_Z1 - 8.0))
    c = c.cut(oring_bore(x, y, CH_Z0 + 14.0, 14.0, CH_Z0 + 8.0 - GROOVE_H).mirror("XY", (0, 0, CH_Z0 + 7.0)))
    c = c.cut(oring_bore_dir((x + CH_R, y, CH_SIDE_Z), (-1, 0, 0), 14.0, 8.0)).cut(xcyl(y, CH_SIDE_Z, 3.2, x + 1.0, x + CH_R - 13.5))
    for sg in (1, -1):
        c = c.cut(box(x + sg * 9.0 - 1.0, x + sg * 9.0 + 1.0, y + 4.0, y + 8.0, CH_Z0 + 22.0, CH_Z0 + 26.0))   # asole della fascetta
    return c

# ------------------------------------------------------------------ C04 culla del flussimetro
def flow_cradle():
    """C04: culla del flussimetro YF-S201 sdraiato lungo Y: due selle a V con le asole per le fascette, due viti nella base."""
    x, z = FS_X, FS_Z
    c = box(x - 22.0, x + 22.0, FS_Y0 + 4.0, FS_Y1 - 4.0, 0, 3.0)
    for y in (FS_Y0 + 10.0, FS_Y1 - 10.0):
        s_ = box(x - 22.0, x + 22.0, y - 4.0, y + 4.0, 0, z - 17.5 + 4.0)
        s_ = s_.cut(box(x - 17.8, x + 17.8, y - 4.01, y + 4.01, z - 17.5 - 0.3, z + 1.0))
        for sg in (1, -1):
            s_ = s_.cut(box(x + sg * 20.0 - 1.0, x + sg * 20.0 + 1.0, y - 4.01, y + 4.01, 3.0, z - 17.5 + 4.01))
        c = c.union(s_)
    for y in (FS_Y0 + 10.0, FS_Y1 - 10.0):
        c = c.cut(zcyl(x, y, 2.1, -0.01, 3.01))
    return c

# ------------------------------------------------------------------ C05 portasonda
def probe_holder():
    """C05: a cavallo della parete posteriore del bagno (4 mm), tiene il cavo della sonda del bagno in una manica fessurata."""
    x, y = PROBE_XY
    zt = BZ1 + COVER_T
    h = box(x - 8.0, x + 8.0, BY1 - 2.5, BY1 + BATH_T + 2.5, BZ1 - 14.0, zt)
    h = h.cut(box(x - 8.01, x + 8.01, BY1 - 0.2, BY1 + BATH_T + 0.2, BZ1 - 14.01, BZ1 + 0.01))
    h = h.union(box(x - 8.0, x + 8.0, y - 4.0, BY1 - 0.2, BZ1 - 12.0, BZ1 - 1.0))
    return h.cut(zcyl(x, y, 1.9, BZ1 - 12.01, BZ1 - 0.99)).cut(box(x - 1.0, x + 1.0, y - 4.01, y, BZ1 - 12.01, BZ1 - 0.99))

# ------------------------------------------------------------------ C06 sella dei riscaldatori
def heater_saddle(i):
    """C06: sella che regge l'estremita' libera della cartuccia Ø16 a 18 mm dal fondo, incollata al fondo del bagno (PMMA);
    la cartuccia ci si incastra dall'alto (apertura 15 mm)."""
    y = HEAT_Y[i]; x = HEAT_X[0] + 12.0
    s = box(x - 6.0, x + 6.0, y - 12.0, y + 12.0, FLOOR, HEAT_Z + 6.0)
    s = s.cut(xcyl(y, HEAT_Z, 8.2, x - 6.01, x + 6.01)).cut(box(x - 6.01, x + 6.01, y - 7.5, y + 7.5, HEAT_Z, HEAT_Z + 6.01))
    return s.union(box(x - 10.0, x + 10.0, y - 14.0, y + 14.0, FLOOR, FLOOR + 2.0))

# ------------------------------------------------------------------ C07 culla della valvola dell'ingresso
def inlet_valve_cradle():
    """C07: culla della valvola dell'ingresso sdraiata lungo X: due traversine sotto il corpo con le asole per le fascette, due viti nella base."""
    zt = IV_Z - VALVE_BODY / 2 - 0.3
    x0, x1 = IV_X0 + 8.0, IV_X0 + VALVE_LEN - 8.0
    c = box(x0, x1, IV_Y - 22.0, IV_Y + 22.0, 0, 3.0)
    for x in (x0 + 6.0, x1 - 6.0):
        c = c.union(box(x - 4.0, x + 4.0, IV_Y - 22.0, IV_Y + 22.0, 0, zt))
        for sg in (1, -1):
            c = c.cut(box(x - 4.01, x + 4.01, IV_Y + sg * 18.5 - 1.0, IV_Y + sg * 18.5 + 1.0, 3.0, zt + 0.01))
    for x in (x0 + 6.0, x1 - 6.0):
        c = c.cut(zcyl(x, IV_Y, 2.1, -0.01, 3.01))
    return c

# ------------------------------------------------------------------ C08 culla della Kamoer, C09 fermi dell'alimentatore
def pump_cradle():
    """C08: culla della Kamoer: piastra con bordo basso attorno al corpo e due coppie di asole per le fascette; tre viti nella base."""
    x0, x1, y0, y1 = PUMP_X0 - 3.0, PUMP_X0 + PUMP_L + 3.0, PUMP_Y0 - 3.0, PUMP_Y0 + PUMP_W + 3.0
    c = box(x0, x1, y0, y1, 0, PUMP_Z0 + 8.0).cut(box(PUMP_X0 - 0.3, PUMP_X0 + PUMP_L + 0.3, PUMP_Y0 - 0.3, PUMP_Y0 + PUMP_W + 0.3, PUMP_Z0, PUMP_Z0 + 8.01))
    c = c.cut(box(PUMP_X0 + 12.0, PUMP_X0 + PUMP_L - 12.0, PUMP_Y0 + 12.0, PUMP_Y0 + PUMP_W - 12.0, -0.01, PUMP_Z0 + 0.01))
    for x in (PUMP_X0 + 18.0, PUMP_X0 + PUMP_L - 18.0):
        for y in (y0, y1 - 4.0):
            c = c.cut(box(x - 1.5, x + 1.5, y - 0.01, y + 4.01, 2.0, PUMP_Z0 + 8.01))
    for (x, y) in ((x0 + 6.0, y0 + 6.0), (x1 - 6.0, y0 + 6.0), ((x0 + x1) / 2, y1 - 6.0)):
        c = c.cut(zcyl(x, y, 2.1, -0.01, PUMP_Z0 + 0.01))
    return c

def psu_stop(end=0):
    """C09: fermo dell'alimentatore in piedi (uno per testa): staffa a U alta 40 mm che abbraccia lo spessore (30 mm) della
    testa, due viti nella base; la testa posteriore lascia liberi i morsetti."""
    y = PSU_Y0 if end == 0 else PSU_Y0 + PSU_L
    y0, y1 = (y - 4.0, y + 12.0) if end == 0 else (y - 12.0, y + 4.0)
    s_ = box(PSU_X0 - 4.0, PSU_X0 + PSU_H + 4.0, y0, y1, 0, PSU_Z0 + 40.0)
    s_ = s_.cut(box(PSU_X0 - 0.3, PSU_X0 + PSU_H + 0.3, PSU_Y0 - 0.3, PSU_Y0 + PSU_L + 0.3, PSU_Z0, PSU_Z0 + 40.01))
    for x in (PSU_X0 + 6.0, PSU_X0 + PSU_H - 6.0):
        s_ = s_.cut(zcyl(x, (y0 + y1) / 2, 2.1, -0.01, PSU_Z0 + 0.01))
    return s_

# ------------------------------------------------------------------ C10 piede e C11 guida della mainboard
MB_HOLES_X = (MB_X0 + 4.0, MB_X0 + MB_L - 4.0)       # i quattro fori M3 della Mainboard v1.1 stanno a 4 mm dai bordi
MB_HOLES_Z = (MB_Z0 + 4.0, MB_Z0 + MB_H - 4.0)
MB_STANDOFF = 3.0                                    # distanza tra la faccia posteriore della scheda (saldature) e il montante
def board_post(i):
    """C10: montante della mainboard (uno per lato): barra verticale dalla base al piano, dietro la scheda (lato saldature),
    con due boss Ø8 x 3 forati M3 (autofilettante) sui fori di fissaggio della scheda, un piede avvitato nella base e una
    linguetta avvitata nel piano da sotto (viti da legno). La scheda, in piedi, si fissa con quattro viti M3 x 8 dal lato
    componenti; la Mainboard v1.1 ha connettori su tutti e quattro i bordi, quindi niente guide a fessura."""
    x = MB_HOLES_X[i]
    y1 = MB_Y0 - MB_STANDOFF; y0 = y1 - 5.0
    p = box(x - 6.0, x + 6.0, y0, y1, 0.0, DECK_Z0)
    for z in MB_HOLES_Z:
        p = p.union(ycyl(x, z, 4.0, y1 - 0.01, MB_Y0 - 0.1)).cut(ycyl(x, z, M3_TAP, y0 + 1.0, MB_Y0 + 0.01))
    p = p.union(box(x - 6.0, x + 6.0, y1 - 0.01, y1 + 16.0, 0.0, 4.0)).cut(zcyl(x, y1 + 10.0, 2.1, -0.01, 4.01))             # piede nella base
    p = p.union(box(x - 6.0, x + 6.0, y1 - 0.01, y1 + 16.0, DECK_Z0 - 4.0, DECK_Z0)).cut(zcyl(x, y1 + 10.0, 2.1, DECK_Z0 - 4.01, DECK_Z0 + 0.01))   # linguetta nel piano
    return p

# ------------------------------------------------------------------ C12 cornice del display
DB_SCREWS = [(DISP_X + sx * (R.DISP_L / 2 + 6.5), DISP_Z + sz * 18.0) for sx in (1, -1) for sz in (1, -1)]
def display_bezel():
    """C12: cornice dietro il pannello frontale: cornice di centraggio della scheda display (vetro contro il pannello) con
    quattro colonnette per le linguette D10; si incolla al pannello (biadesivo) o si avvita con le viti delle linguette."""
    hl, hw = R.DISP_L / 2 + 0.3, R.DISP_W / 2 + 0.3
    y0, y1 = 0.0, R.DISP_T
    fr = box(DISP_X - hl - 8.0, DISP_X + hl + 8.0, y0, y1 + 2.0, DISP_Z - hw - 8.0, DISP_Z + hw + 8.0)
    fr = fr.cut(box(DISP_X - hl, DISP_X + hl, y0 - 0.01, y1 + 2.01, DISP_Z - hw, DISP_Z + hw))
    for (x, z) in DB_SCREWS:
        fr = fr.union(ycyl(x, z, 4.0, y0, y1 + 4.0)).cut(ycyl(x, z, M3_TAP, y0 + 0.6, y1 + 4.01))      # colonnette alte 7,2: vite 3 x 8 con 5 mm di presa
    return fr

def display_tab_local():
    return box(-4.0, 13.0, -4.0, 4.0, 0, 3.0).cut(zcyl(0, 0, M3_CLR, -0.01, 3.01))

def display_tabs():
    out = []
    for i, (x, z) in enumerate(DB_SCREWS):
        t = display_tab_local().rotate((0, 0, 0), (0, 0, 1), 180 if x > DISP_X else 0)
        out.append(t.rotate((0, 0, 0), (1, 0, 0), -90).translate((x, R.DISP_T + 4.0, z)))
    return out

# ------------------------------------------------------------------ C13 colonne del piano
COLS = ((215.0, 12.0), (440.0, 12.0))
def deck_column(x, y):
    """C13: colonna di appoggio del piano a meta' luce: tubo Ø20 con piede largo (due viti nella base) e testa (vite nel piano)."""
    c = zcyl(x, y, 10.0, 0, DECK_Z0).union(box(x - 14.0, x + 14.0, y - 10.0, y + 10.0, 0, 4.0))
    c = c.cut(zcyl(x, y, 6.5, 6.0, DECK_Z0 - 8.0))
    c = c.cut(zcyl(x, y, 1.9, DECK_Z0 - 8.01, DECK_Z0 + 0.01))
    for sg in (1, -1):
        c = c.cut(zcyl(x + sg * 11.0, y, 2.1, -0.01, 4.01))
    return c

# ------------------------------------------------------------------ C14 staffa dei tubi verticali (beccuccio e troppopieno)
def riser_clip(x, y=SRV_Y):
    """C14: staffetta a cavallo della parete posteriore del bagno che tiene in verticale il tubo 3/8" del beccuccio (o del
    troppopieno): fessura da 10 mm, si infila dall'alto; il ponte sta nella tacca del coperchio. y = asse del tubo."""
    c = box(x - 8.0, x + 8.0, BY1 - 2.5, BY1 + BATH_T + 2.5, BZ1 - 14.0, BZ1 + COVER_T)
    c = c.cut(box(x - 8.01, x + 8.01, BY1 - 0.2, BY1 + BATH_T + 0.2, BZ1 - 14.01, BZ1 + 0.01))
    c = c.union(box(x - 8.0, x + 8.0, y - 8.0, BY1 - 0.2, BZ1 - 12.0, BZ1 - 1.0))
    return c.cut(zcyl(x, y, 5.0, BZ1 - 12.01, BZ1 - 0.99)).cut(box(x - 4.0, x + 4.0, y - 8.01, y, BZ1 - 12.01, BZ1 - 0.99))

PARTS = [("C01_collettore", manifold), ("C02_culla_valvole", valve_cradle), ("C03_camera_livello", lambda: chamber(0)),
         ("C04_culla_flussimetro", flow_cradle), ("C05_portasonda", probe_holder), ("C06_sella_riscaldatore", lambda: heater_saddle(0)),
         ("C07_culla_valvola_ingresso", inlet_valve_cradle), ("C08_culla_pompa", pump_cradle), ("C09_fermo_alimentatore", lambda: psu_stop(0)),
         ("C10_montante_mainboard", lambda: board_post(0)), ("C12_cornice_display", display_bezel),
         ("C13_colonna_piano", lambda: deck_column(*COLS[0])), ("C14_staffa_tubo_verticale", lambda: riser_clip(OVF_X)), ("D10_linguetta_display", display_tab_local)]

if __name__ == "__main__":
    import time
    for n, f in PARTS:
        t = time.time(); s = f().val(); bb = s.BoundingBox()
        print(f"{n:26s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:6.2f} cm3  {bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f}  ({time.time() - t:.0f} s)")
