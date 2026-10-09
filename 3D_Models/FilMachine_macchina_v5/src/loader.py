"""Caricatore integrato nella tank: ponte (due blocchi + tetto), taglierina a "Λ" dal basso (portalama, morsetti,
biella, manovella), culle 135/120, guida a due slitte su barra quadra. Tutto nel riferimento della tank."""
import math
import cadquery as cq
from common import *
from tank import box, xz_prism, yz_prism, xy_prism, ycyl, zcyl, xcyl

TT = math.tan(math.radians(BLADE_TILT))
EAR_Z = (51.5, 71.0)                      # orecchie di guida del portalama (a riposo)
CAR_ZB = 58.0                             # bordo inferiore del portalama (a riposo)
RAILS = ((13.5, 18.5), (27.5, 32.0))      # fasce su cui scorre il margine della pellicola: 135 e 120
BLOCK_SCREWS = ((BR_X0 - 3.5, 41.5), (BR_X1 + 3.5, 41.5))     # viti dei blocchi ponte sul ripiano
ROOF_SCREWS = ((BR_X0 + 2.5, 42.8), (BR_X1 - 2.5, 42.8))      # viti del tetto sulle colonne
CLAMP_Y = (8.0, 26.0)                     # viti dei morsetti lama
JOURNAL_Y = 45.0                          # da qui in fuori la barra guida e' tonda e appoggia nelle selle

def edge_z(y, dz=0.0):
    """Quota del filo della lama a distanza y dal centro."""
    return APEX_PARK + dz - abs(y) * TT

def back_z(y):
    """Quota del dorso della lama (9 mm sotto il filo, misurati perpendicolarmente al filo) a riposo."""
    return APEX_PARK - BLADE_W / math.cos(math.radians(BLADE_TILT)) - abs(y) * TT

def _recess(z0, z1, x0, x1):
    """Zone ribassate tra le fasce di scorrimento (meta' +Y)."""
    r = box(x0, x1, -0.01, RAILS[0][0], z0, z1)
    return r.union(box(x0, x1, RAILS[0][1], RAILS[1][0], z0, z1))

def bridge_block(side=1):
    """Blocco ponte (colonna + mezze soglie + sella della barra guida). side=+1: lato +Y; -1: speculare.
    Si stampa in piedi sulla faccia esterna della colonna: le soglie crescono verso l'alto, senza sbalzi."""
    c = box(BR_X0, BR_X1, COL_Y0, COL_Y1, Z_SH, SLOT_Z1)                                   # colonna
    c = c.union(box(BR_X0, CH_X0, 0.0, COL_Y0, 76.0, SLOT_Z0))                             # soglia lato caricatore
    c = c.union(box(CH_X1, BR_X1, 0.0, COL_Y0, 76.0, SLOT_Z0))                             # soglia lato spirale
    c = c.union(box(BR_X0, CH_X0, 32.0, COL_Y0, SLOT_Z0, SLOT_Z1))                         # fianco della fessura pellicola
    c = c.union(box(CH_X1, BR_X1, 32.0, COL_Y0, SLOT_Z0, SLOT_Z1))
    c = c.cut(_recess(SLOT_Z0 - 0.6, SLOT_Z0 + 0.01, BR_X0 - 0.01, BR_X1 + 0.01))          # la pellicola tocca solo sulle fasce
    c = c.cut(xz_prism([(BR_X0 - 0.01, SLOT_Z0 + 0.01), (BR_X0 + 2.5, SLOT_Z0 + 0.01), (BR_X0 - 0.01, SLOT_Z0 - 1.5)],
                       -0.01, 32.0))                                                       # invito lato caricatore
    c = c.cut(box(CAR_X0 - 0.4, BLADE_X1 + 0.4, COL_Y0 - 0.01, 40.4, Z_SH - 0.01, SLOT_Z1 + 0.01))   # guida del portalama
    for (x0, x1) in ((BR_X0 - 7.0, BR_X0), (BR_X1, BR_X1 + 7.0)):                          # piedini
        c = c.union(box(x0, x1, COL_Y0, COL_Y1, Z_SH, Z_SH + 4.0))
    for x, y in BLOCK_SCREWS:
        c = c.cut(zcyl(x, y, M3_CLR, Z_SH - 0.01, Z_SH + 4.01))
    # sella della barra guida (a scatto, aperta in alto)
    c = c.union(box(BR_X1, XB + 7.0, JOURNAL_Y, COL_Y1, ZB - 7.5, ZB + 8.0))
    c = c.cut(ycyl(XB, ZB, 4.15, JOURNAL_Y - 0.01, COL_Y1 + 0.01))
    c = c.cut(box(XB - 4.15, XB + 4.15, JOURNAL_Y - 0.01, COL_Y1 + 0.01, ZB, ZB + 4.6))
    c = c.cut(xz_prism([(XB - 4.15, ZB + 4.59), (XB + 4.15, ZB + 4.59), (XB + 3.75, ZB + 5.4), (XB + 3.75, ZB + 6.4),
                        (XB + 5.4, ZB + 8.01), (XB - 5.4, ZB + 8.01), (XB - 3.75, ZB + 6.4), (XB - 3.75, ZB + 5.4)],
                       JOURNAL_Y - 0.01, COL_Y1 + 0.01))                                   # strozzatura 7.5 + invito
    for x, y in ROOF_SCREWS:
        c = c.cut(zcyl(x, y, M3_TAP, SLOT_Z1 - 12.0, SLOT_Z1 + 0.01))
    return c if side > 0 else c.mirror("XZ")

def roof():
    """Tetto del ponte: cielo della fessura pellicola, gola della lama e tasca centrale per il portalama.
    Si stampa capovolto (faccia superiore sul piatto)."""
    r = box(BR_X0, BR_X1, -COL_Y1, COL_Y1, SLOT_Z1, BR_ZTOP)
    r = r.cut(box(CAR_X1 - 0.4, BLADE_X1 + 0.4, -37.5, 37.5, SLOT_Z1 - 0.01, SLOT_Z1 + 8.0))   # gola lama (1.2 mm)
    r = r.cut(box(CH_X0, CH_X1, -13.0, 13.0, SLOT_Z1 - 0.01, SLOT_Z1 + 3.0))                  # tasca centrale
    rec = _recess(SLOT_Z1 - 0.01, SLOT_Z1 + 0.6, BR_X0 - 0.01, BR_X1 + 0.01)
    r = r.cut(rec).cut(rec.mirror("XZ"))
    r = r.cut(xz_prism([(BR_X0 - 0.01, SLOT_Z1 - 0.01), (BR_X0 + 2.5, SLOT_Z1 - 0.01), (BR_X0 - 0.01, SLOT_Z1 + 1.5)],
                       -32.0, 32.0))
    for x, y in ROOF_SCREWS:
        for s in (1, -1):
            r = r.cut(zcyl(x, s * y, M3_CLR, SLOT_Z1 - 0.01, BR_ZTOP + 0.01))
            r = r.cut(zcyl(x, s * y, 3.1, BR_ZTOP - 3.2, BR_ZTOP + 0.01))                    # sede testa vite
    return r

def carrier(dz=0.0):
    """Portalama: piastra con due sedi inclinate (lame a Λ), orecchie di guida nelle colonne, forcella della biella.
    Si stampa di piatto sulla faccia lato caricatore."""
    zt0 = APEX_PARK - 5.0
    pts = [(-40.0, EAR_Z[0]), (-40.0, EAR_Z[1]), (-36.2, EAR_Z[1]), (-36.2, zt0 - 36.2 * TT), (0.0, zt0),
           (36.2, zt0 - 36.2 * TT), (36.2, EAR_Z[1]), (40.0, EAR_Z[1]), (40.0, EAR_Z[0]), (36.2, EAR_Z[0]),
           (36.2, CAR_ZB), (3.0, CAR_ZB), (3.0, WRIST_Z - 3.5), (-6.0, WRIST_Z - 3.5), (-6.0, CAR_ZB), (-36.2, CAR_ZB),
           (-36.2, EAR_Z[0])]
    c = yz_prism(pts, CAR_X0, CAR_X1)
    for y0, y1 in ((-5.6, -3.6), (0.2, 2.2)):                                             # forcella
        c = c.union(box(CAR_X1, SRV_X + 3.5, y0, y1, WRIST_Z - 3.5, WRIST_Z + 3.5))
    c = c.cut(ycyl(SRV_X, WRIST_Z, 1.55, -5.61, 2.21))
    for s in (1, -1):                                                                     # appoggio del dorso lama
        zl = lambda y: back_z(y)
        led = yz_prism([(s * 4.0, zl(4.0)), (s * 36.0, zl(36.0)), (s * 36.0, zl(36.0) - 1.0), (s * 4.0, zl(4.0) - 1.0)],
                       CAR_X1, CAR_X1 + 0.35)
        c = c.union(led)
        for y in CLAMP_Y:
            c = c.cut(xcyl(s * y, zl(y) - 3.0, M2_TAP, CAR_X0 - 0.01, CAR_X1 + 0.5))
    return c.translate((0, 0, dz))

def clamp(side=1, dz=0.0):
    """Morsetto lama (uno per lato): preme la lama sul portalama, due viti M2."""
    s = side
    zl = lambda y: back_z(y)
    top = lambda y: APEX_PARK - 5.0 - y * TT
    k = yz_prism([(s * 0.8, top(0.8)), (s * 36.0, top(36.0)), (s * 36.0, zl(36.0) - 6.0), (s * 0.8, zl(0.8) - 6.0)],
                 BLADE_X1, CLAMP_X1)
    k = k.union(yz_prism([(s * 4.0, zl(4.0) - 1.2), (s * 36.0, zl(36.0) - 1.2), (s * 36.0, zl(36.0) - 6.0),
                          (s * 4.0, zl(4.0) - 6.0)], BLADE_X1 - 0.38, BLADE_X1))             # tallone: tiene il morsetto parallelo
    for y in CLAMP_Y:
        k = k.cut(xcyl(s * y, zl(y) - 3.0, 1.1, BLADE_X1 - 0.5, CLAMP_X1 + 0.01))
        k = k.cut(xcyl(s * y, zl(y) - 3.0, 2.0, CLAMP_X1 - 1.2, CLAMP_X1 + 0.01))
    return k.translate((0, 0, dz))

def blade(side=1, dz=0.0):
    """Sagoma di uno spezzone di lama da cutter 9 mm (36 mm): filo inclinato, punta al centro."""
    s = side
    t = math.radians(BLADE_TILT)
    e = (-math.cos(t), -math.sin(t)); n = (math.sin(t), -math.cos(t))
    d = (0.5 * e[0] + math.sin(math.radians(60)) * n[0], 0.5 * e[1] + math.sin(math.radians(60)) * n[1])
    k = BLADE_W / math.sin(math.radians(60))
    T = (0.0, APEX_PARK); E = (T[0] + 36.0 * e[0], T[1] + 36.0 * e[1])
    pts = [T, E, (E[0] + k * d[0], E[1] + k * d[1]), (T[0] + k * d[0], T[1] + k * d[1])]
    pts = [(-s * y, z) for y, z in pts]                    # side=+1: lama sul lato +Y
    return yz_prism(pts, BLADE_X0, BLADE_X1).translate((0, 0, dz))

def crank_pin(theta):
    """Posizione (x, z) del perno di manovella; theta = 0 a riposo (in basso), 180 a fine taglio (in alto)."""
    a = math.radians(theta - 90.0)                       # da -90 (basso) a +90 (alto) passando per +X
    return SRV_X + CRANK_R * math.cos(a), SRV_Z + CRANK_R * math.sin(a)

def lift(theta):
    """Alzata del portalama per l'angolo di manovella theta (biella-manovella)."""
    px, pz = crank_pin(theta)
    return pz + math.sqrt(ROD_L ** 2 - (px - SRV_X) ** 2) - WRIST_Z

def rod(theta=0.0):
    """Biella: occhio da 7 mm per lo spinotto (Ø3.2) e gambo da 5 mm con foro Ø2.7 per il perno di manovella (M2.5)."""
    r = box(-2.5, 2.5, -3.2, -0.2, -2.6, ROD_L - 3.0).union(box(-3.5, 3.5, -3.2, -0.2, ROD_L - 3.5, ROD_L + 3.5))
    r = r.cut(ycyl(0, 0, 1.35, -3.21, -0.19)).cut(ycyl(0, ROD_L, 1.6, -3.21, -0.19))
    px, pz = crank_pin(theta)
    ang = math.degrees(math.asin((SRV_X - px) / ROD_L))
    return r.rotate((0, 0, 0), (0, 1, 0), ang).translate((px, 0, pz))

def crank(theta=0.0):
    """Manovella a braccio (spazza solo mezzo giro, verso +X): mozzo sulla squadretta del servo, perno = vite M2.5."""
    c = ycyl(0, 0, 3.6, 0.3, 3.3).union(ycyl(0, -CRANK_R, 2.6, 0.3, 3.3)).union(box(-2.6, 2.6, 0.3, 3.3, -CRANK_R, 0.0))
    c = c.cut(ycyl(0, 0, 1.25, 0.29, 3.31))                                                # vite della squadretta
    c = c.cut(box(-2.0, 2.0, 1.7, 3.31, -CRANK_R - 2.61, 0.0))                             # sede del braccio della squadretta
    c = c.cut(ycyl(0, -CRANK_R, 1.05, 0.29, 3.31))                                         # perno M2.5 autofilettante
    return c.rotate((0, 0, 0), (0, 1, 0), -theta).translate((SRV_X, 0, SRV_Z))

# ---------------------------------------------------------------- culla universale: caricatore 135 oppure rullo 120
CR_X0, CR_X1, CR_Y = X_DRY0 + 1.0, BR_X0 - 0.5, 36.0
CR_SCREWS = ((-110.0, 30.0), (-85.0, 27.9))
SILL_Z = CAS_Z + 8.3                       # soglia verso il ponte (sotto il becco feltrato del caricatore 135)
R120_Z, R120_R, R120_Y = ZF - 7.5, 12.8, 32.8   # sede del rullo 120: quota dell'asse, raggio, semi-larghezza
R135_Y = 23.9                              # semi-larghezza della sede del caricatore 135
CR_TOP = 84.0                              # sommita' di guance e parete di fondo
NOTCH_X, NOTCH_Z = (-105.0, -87.0), 66.0   # scassi per le dita nelle guance

def cradle():
    """Culla unica per i due formati. Sede larga (rullo 120, che appoggia sulle due flange del rocchetto) e, al centro,
    sede piu' profonda per il caricatore 135, tenuto di lato dai due gradini a mezzaluna tra le due sedi. Soglia verso
    il ponte comune ai due formati; scassi nelle guance per prendere caricatore o rullo con due dita.
    Si stampa cosi' com'e', appoggiata sul fondo."""
    k = box(CR_X0, CR_X1, -CR_Y, CR_Y, Z_SH, CR_TOP)
    seat120 = cq.Workplane("XZ").center(CAS_X - 1.5, R120_Z).slot2D(3 + 2 * R120_R, 2 * R120_R, 0).extrude(R120_Y, both=True)
    k = k.cut(seat120)
    k = k.cut(box(CR_X0 + 3.2, CAS_X, -R120_Y, R120_Y, R120_Z, CR_TOP + 0.01))                    # apertura per inserire il rullo
    k = k.cut(box(CAS_X, CR_X1 + 0.01, -R120_Y, R120_Y, SILL_Z, CR_TOP + 0.01))                   # uscita pellicola sopra la soglia
    seat135 = ycyl(CAS_X, CAS_Z, CAS_R, -R135_Y, R135_Y).union(
              box(CAS_X - CAS_R, CAS_X + CAS_R, -R135_Y, R135_Y, CAS_Z, CR_TOP + 0.01))
    k = k.cut(seat135)
    for s in (1, -1):                                                                             # scassi per le dita
        y0, y1 = (R120_Y - 0.01, CR_Y + 0.01) if s > 0 else (-CR_Y - 0.01, -R120_Y + 0.01)
        k = k.cut(box(NOTCH_X[0], NOTCH_X[1], y0, y1, NOTCH_Z, CR_TOP + 0.01))
    for x, y in CR_SCREWS:
        for s in (1, -1):
            k = k.cut(zcyl(x, s * y, M3_CLR, Z_SH - 0.01, Z_SH + 6.0))
            k = k.cut(zcyl(x, s * y, 3.1, Z_SH + 5.0, CR_TOP + 0.01))                              # pozzetto per la testa della vite
    return k

cradle135 = cradle120 = cradle             # compatibilita' con le versioni a due culle

def cassette135():
    """Sagoma del caricatore 135 (corpo Ø25 x 47.5, codoli, becco feltrato)."""
    c = ycyl(CAS_X, CAS_Z, 12.5, -23.75, 23.75)
    c = c.union(ycyl(CAS_X, CAS_Z, 5.5, -27.5, 26.0))
    return c.union(box(CAS_X, CAS_X + 14.5, -19.0, 19.0, CAS_Z + 9.0, CAS_Z + 12.5))

def roll120():
    """Sagoma del rullo 120 (rocchetto 64.5 mm, flange Ø25, rullo pieno Ø24)."""
    rz = ZF - 7.5
    r = ycyl(CAS_X, rz, 12.0, -31.0, 31.0)
    return r.union(ycyl(CAS_X, rz, 12.6, 31.0, 32.25)).union(ycyl(CAS_X, rz, 12.6, -32.25, -31.0))

# ---------------------------------------------------------------- guida a due slitte su barra quadra (tipo Lab-Box)
N_SOLE = N_F - H_EDGE                      # quota della suola nel riferimento della slitta
N_TOP = 12.0                               # faccia superiore della slitta (piano di stampa)
HUB_Y0, HUB_Y1 = 18.0, 27.9                # mozzo, all'esterno della pellicola (coordinate della slitta a 135)
HUB_YC = (HUB_Y0 + HUB_Y1) / 2
FMT_SHIFT = {"135": 0.0, "120": (reel_width("120") - reel_width("135")) / 2}     # spostamento laterale di ogni slitta
BAR_HALF = COL_Y1                          # la barra arriva alle selle nelle colonne del ponte
# stazioni della slitta: s, y esterno, y vertice gola, y labbro superiore, y labbro inferiore, angolo inf., angolo sup., n suola
_ST = [(-7.0, 19.3, 18.7, 17.5, 16.4, -30.0, 50.0, N_SOLE),
       (-2.0, 18.5, 17.9, 17.0, 16.9,   0.0, 45.0, N_SOLE),
       ( 6.0, 17.6, 17.02, 16.1, 15.9, 25.0, 55.0, N_SOLE),
       (17.0, 16.4, 15.8, 14.9, 14.2,  45.0, 70.0, N_SOLE),
       (S_TIP - 3.0, 16.4, 15.8, 14.9, 14.2, 45.0, 70.0, N_SOLE),
       (S_TIP, 16.4, 15.8, 14.9, 14.2, 45.0, 70.0, N_SOLE + 1.4)]

def _slide_section(st):
    s, yo, yv, yiu, yil, al, au, nb = st
    mu = N_F + (yv - yiu) * math.tan(math.radians(au))
    ml = N_F + (yv - yil) * math.tan(math.radians(al))
    return [(s, yo, nb), (s, yo, N_TOP), (s, yiu, N_TOP), (s, yiu, mu), (s, yv, N_F), (s, yil, ml), (s, yil, nb)]

def slide_local():
    """Slitta destra (+Y) nel suo riferimento: x = s lungo la suola, y = laterale (posizione 135), z = n.
    Si stampa capovolta, con la faccia superiore sul piatto: la suola (che scorre sulla pellicola) viene liscia."""
    body = cq.Workplane("XY").add(loft_planar([_slide_section(st) for st in _ST]))
    hub = box(-HUB / 2, HUB / 2, HUB_Y0, HUB_Y1, -HUB / 2, N_TOP)
    g = body.union(hub)
    g = g.cut(box(-BAR / 2 - 0.15, BAR / 2 + 0.15, HUB_Y0 - 0.01, HUB_Y1 + 0.01, -BAR / 2 - 0.15, BAR / 2 + 0.15))   # foro quadro
    notch = xy_prism([(BAR / 2 + 0.14, HUB_YC - 1.45), (BAR / 2 + 0.75, HUB_YC - 0.3), (BAR / 2 + 0.75, HUB_YC + 0.3),
                      (BAR / 2 + 0.14, HUB_YC + 1.45)], -BAR / 2 - 0.15, BAR / 2 + 0.15)
    return g.cut(notch)

def bar_local():
    """Barra quadra 8x8 con perni tondi alle estremita' e quattro linguette a scatto (posizioni 135 e 120).
    Riferimento: asse lungo Y, faccia con i dentini verso +X (verso la spirale). Si stampa di piatto."""
    b = box(-BAR / 2, BAR / 2, -JOURNAL_Y + 0.4, JOURNAL_Y - 0.4, -BAR / 2, BAR / 2)
    for s in (1, -1):
        y0, y1 = (JOURNAL_Y - 0.4, BAR_HALF) if s > 0 else (-BAR_HALF, -JOURNAL_Y + 0.4)
        b = b.union(ycyl(0, 0, BAR / 2, y0, y1))
    fingers = ((11.4, 25.0, HUB_YC, 1.5), (26.6, 38.2, HUB_YC + FMT_SHIFT["120"], 1.3))    # radice, estremita', dentino, spessore
    for s in (1, -1):
        for ya, yb, yc, t in fingers:
            x1 = BAR / 2; x0 = x1 - t
            slit = box(x0 - 0.8, x0, ya, yb + 0.8, -BAR / 2 - 0.01, BAR / 2 + 0.01).union(
                   box(x0 - 0.8, x1 + 0.01, yb, yb + 0.8, -BAR / 2 - 0.01, BAR / 2 + 0.01))
            bump = xy_prism([(x1 - 0.01, yc - 1.3), (x1 + 0.5, yc - 0.3), (x1 + 0.5, yc + 0.3), (x1 - 0.01, yc + 1.3)],
                            -BAR / 2, BAR / 2)
            if s < 0:
                slit, bump = slit.mirror("XZ"), bump.mirror("XZ")
            b = b.cut(slit).union(bump)
    return b

def slide_angle(r):
    """Inclinazione della guida (gradi, + = punta in alto) con la suola appoggiata sul raggio r."""
    d = math.hypot(XB, HC - ZB)
    return math.degrees(math.asin((r - N_SOLE) / d) - math.atan2(ZB - HC, -XB))

def place_guide(fmt, r=None, phi=None):
    """Barra + due slitte nel riferimento della tank, per il formato fmt, appoggiate sul raggio r (o all'angolo phi)."""
    if phi is None:
        phi = slide_angle(r)
    sh = FMT_SHIFT[fmt]
    right = slide_local().translate((0, sh, 0))
    left = slide_local().mirror("XZ").translate((0, -sh, 0))
    out = []
    for p in (bar_local(), right, left):
        out.append(p.rotate((0, 0, 0), (0, 1, 0), -phi).translate((XB, 0, ZB)))
    return out, phi
