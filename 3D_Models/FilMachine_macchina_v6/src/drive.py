"""Trasmissione della spirale: trascinatore a tre denti su albero inox Ø8, cartuccia con paraolio e cuscinetti,
pulegge per cinghia tonda (fa anche da limitatore di coppia), ruota magneti per il sensore Hall, piastra del
motoriduttore JGB37-520 (regolazione a eccentrico), carter; perno folle a molla sul lato opposto."""
import math
import cadquery as cq
from common import *
from tank import box, xz_prism, yz_prism, xy_prism, ycyl, zcyl, xcyl, PAD_Y1, PIL_Y0, PIN_R, CART_HOLE_R, CART_N

# ---- quote lungo l'asse (Y) lato trasmissione ----
DOG_Y0 = TUBE_HALF - 10.0            # fondo del mozzo del trascinatore (dentro il foro del tubo)
MAG_R, MAG_D, MAG_H = 13.0, 6.0, 3.0 # magneti Ø6x3 sulla ruota
CART_IN = Y_DRV + 0.5                # la cartuccia arriva a 0.5 mm dalla faccia interna della parete
WASHER_Y0 = CART_IN - 1.6            # rondella inox M8 (8.4x16x1.6) tra trascinatore e cartuccia: regge la spinta della molla
CART_FL0, CART_FL1 = PAD_Y1, PAD_Y1 + 4.0
CART_OUT = PAD_Y1 + 18.0             # estremita' esterna della cartuccia
PUL_Y0 = CART_OUT + 1.0              # pulegge: gola tra PUL_Y0+0.7 e PUL_Y0+4.3
PUL_GC = PUL_Y0 + 2.5                # piano della cinghia
SHAFT_Y1 = PUL_Y0 + 20.0
SEAL_Y = (CART_IN + 0.8, CART_IN + 5.8)
BRG_Y = ((CART_OUT - 15.0, CART_OUT - 10.0), (CART_OUT - 5.0, CART_OUT))
MOT_ECC = 7.0                        # eccentricita' dell'albero d'uscita del JGB37-520
BELT_ANG = math.degrees(math.atan2(HC - MOT_Z, -MOT_X))    # direzione motore -> spirale
ECC_ANG = BELT_ANG + 90.0            # posizione nominale dell'albero motore: perpendicolare alla cinghia
MOT_SH = (MOT_X + MOT_ECC * math.cos(math.radians(ECC_ANG)), MOT_Z + MOT_ECC * math.sin(math.radians(ECC_ANG)))
MOT_FACE = YO1                       # faccia del riduttore a filo della parete esterna
PLATE_T = 4.0

def dog():
    """Trascinatore: piattello contro la testa del tubo, codolo nel foro, tre denti nelle tacche, mozzo con grano M3.
    Si stampa col piattello sul piatto, senza supporti."""
    d = ycyl(0, HC, TUBE_R - 0.3, TUBE_HALF, WASHER_Y0)                          # piattello: da un lato la testa del tubo, dall'altro la rondella reggispinta
    d = d.union(ycyl(0, HC, TUBE_BORE - 0.2, TUBE_HALF - 4.0, TUBE_HALF))
    d = d.union(ycyl(0, HC, 7.5, DOG_Y0, TUBE_HALF - 4.0))
    for k in range(3):
        a = NOTCH_ANG + 120.0 * k
        t = (cq.Workplane("XY").box(2.9, 5.4, 3.6, centered=(False, True, False))
             .translate((TUBE_BORE - 0.4, 0, 0)))                    # nel riferimento della spirale: asse Z
        t = t.rotate((0, 0, 0), (0, 0, 1), a).translate((0, 0, -TUBE_HALF))          # estremita' lato A (z = -TUBE_HALF)
        d = d.union(t.rotate((0, 0, 0), (1, 0, 0), 90).translate((0, 0, HC)))
    d = d.cut(ycyl(0, HC, SHAFT_D / 2 + 0.05, DOG_Y0 - 0.01, WASHER_Y0 + 0.01))
    yg = DOG_Y0 + 3.0                                                # grano M3 radiale con sede dado
    d = d.cut(zcyl(0, yg, M3_CLR, HC, HC + 8.0))
    d = d.cut(box(-2.9, 2.9, yg - 1.35, yg + 1.35, HC + 4.6, HC + 6.6)).cut(box(-2.9, 2.9, DOG_Y0 - 0.01, yg, HC + 4.6, HC + 6.6))
    return d

def cartridge():
    """Cartuccia: paraolio 8x16x5 verso la vasca, due cuscinetti 688 (8x16x5), flangia con tre viti e sede O-ring."""
    c = ycyl(0, HC, 11.3, CART_IN, CART_FL0).union(ycyl(0, HC, 21.0, CART_FL0, CART_FL1)).union(ycyl(0, HC, 11.0, CART_FL1, CART_OUT))
    c = c.cut(ycyl(0, HC, 4.6, CART_IN - 0.01, CART_IN + 0.8))                        # battuta del paraolio
    c = c.cut(ycyl(0, HC, 8.03, CART_IN + 0.8, CART_OUT + 0.01))                      # sede Ø16 passante
    for k in range(CART_N):
        a = math.radians(90 + 120 * k)
        c = c.cut(ycyl(CART_HOLE_R * math.cos(a), HC + CART_HOLE_R * math.sin(a), M3_CLR, CART_FL0 - 0.01, CART_FL1 + 0.01))
    c = c.cut(ycyl(0, HC, 14.2, CART_FL0 - 0.01, CART_FL0 + 1.4).cut(ycyl(0, HC, 12.2, CART_FL0 - 0.02, CART_FL0 + 1.5)))   # O-ring 24x2
    return c

def spacer(y0, y1):
    return ycyl(0, HC, 7.9, y0, y1).cut(ycyl(0, HC, 5.0, y0 - 0.01, y1 + 0.01))

RIM_W, WHEEL_T, HUB_L = 8.0, 4.0, 7.0   # larghezza della corona (gola + fianco a 45 gradi), ruota magneti, mozzo
WHEEL_Y1 = PUL_Y0 + RIM_W + WHEEL_T     # faccia esterna della ruota magneti

def _pulley(xc, zc, bore_r, hub, magnets=False, dflat=False, flank=1):
    """Puleggia per cinghia tonda Ø3: gola larga 3 mm con un fianco diritto e l'altro a 45 gradi (flank = +1: fianco
    inclinato verso +Y; -1: verso -Y), cosi' si stampa senza supporti col fianco diritto sul piatto.
    hub = (y0, y1): tratto del mozzo, con grano M3 e sede del dado."""
    rg, r2 = PUL_R - 1.5, PUL_R + 2.0
    y0r, y1r = (PUL_Y0, PUL_Y0 + RIM_W) if flank > 0 else (PUL_Y0 + 5.0 - RIM_W, PUL_Y0 + 5.0)
    p = ycyl(xc, zc, PUL_R + 1.8, y0r, y1r)
    prof = ([(rg, 1.0), (rg, 4.0), (r2, 4.0 + r2 - rg), (r2, 1.0)] if flank > 0 else [(rg, 1.0), (rg, 4.0), (r2, 4.0), (r2, 1.0 - (r2 - rg))])
    vee = cq.Workplane("XY").polyline(prof).close().revolve(360, (0, 0, 0), (0, 1, 0))
    p = p.cut(vee.translate((xc, PUL_Y0, zc)))
    y_end = y1r
    if magnets:                                                                       # ruota magneti, con smusso a 45 gradi sotto
        rw = MAG_R + 4.0
        p = p.union(cq.Workplane("XY").add(cq.Solid.makeCone(PUL_R + 1.8, rw, rw - PUL_R - 1.8, cq.Vector(xc, y1r, zc), cq.Vector(0, 1, 0))))
        p = p.union(ycyl(xc, zc, rw, y1r + rw - PUL_R - 1.8, WHEEL_Y1))
        for k in range(4):
            a = math.radians(45 + 90 * k)
            p = p.cut(ycyl(xc + MAG_R * math.cos(a), zc + MAG_R * math.sin(a), MAG_D / 2 + 0.1, WHEEL_Y1 - MAG_H - 0.2, WHEEL_Y1 + 0.01))
        y_end = WHEEL_Y1
    p = p.union(ycyl(xc, zc, 8.0, hub[0], hub[1]))                                    # mozzo
    y0, y1 = min(hub[0], y0r), max(hub[1], y_end)
    p = p.cut(ycyl(xc, zc, bore_r, y0 - 0.01, y1 + 0.01))
    if dflat:
        p = p.union(box(xc - bore_r, xc + bore_r, y0, y1, zc + bore_r - 0.55, zc + bore_r + 0.3).intersect(ycyl(xc, zc, bore_r + 0.2, y0, y1)))
    yg = (hub[0] + hub[1]) / 2
    p = p.cut(zcyl(xc, yg, M3_CLR, zc, zc + 8.5))
    slot_y = (hub[0] - 0.01, yg) if hub[0] < y0r else (yg, hub[1] + 0.01)               # la sede del dado si apre sul lato libero del mozzo
    p = p.cut(box(xc - 2.9, xc + 2.9, yg - 1.35, yg + 1.35, zc + 4.9, zc + 6.9)).cut(box(xc - 2.9, xc + 2.9, slot_y[0], slot_y[1], zc + 4.9, zc + 6.9))
    return p

def pulley_reel():
    """Puleggia condotta sull'albero Ø8, con la ruota dei quattro magneti Ø6x3 per il sensore Hall (mozzo all'esterno).
    Si stampa con la faccia verso il cuscinetto sul piatto."""
    return _pulley(0.0, HC, SHAFT_D / 2 + 0.05, (WHEEL_Y1, WHEEL_Y1 + HUB_L), magnets=True, flank=1)

def shim():
    """Rasamento tra il cuscinetto esterno e la puleggia: tocca solo l'anello interno del cuscinetto."""
    return ycyl(0, HC, 5.4, CART_OUT + 0.05, PUL_Y0).cut(ycyl(0, HC, SHAFT_D / 2 + 0.05, CART_OUT, PUL_Y0 + 0.01))

def pulley_motor(pos=None):
    """Puleggia motrice sull'albero Ø6 a D del motoriduttore (mozzo verso il motore). Si stampa con la faccia esterna sul piatto."""
    x, z = pos or MOT_SH
    return _pulley(x, z, 3.05, (PUL_Y0 - 14.0, PUL_Y0 + 5.0 - RIM_W), dflat=True, flank=-1)

def motor_plate():
    """Piastra del motoriduttore: chiude la bocca della galleria; apertura a mezzaluna per mozzetto e albero,
    due asole ad arco sulle viti M3 del riduttore (regolazione a eccentrico +/-22 gradi; i sei fori permettono passi di 60 gradi)."""
    p = box(XO0, -64.0, MOT_FACE, MOT_FACE + PLATE_T, 0.5, 47.5)
    ea = math.radians(ECC_ANG)
    op = ycyl(MOT_X, MOT_Z, 13.6, MOT_FACE - 0.01, MOT_FACE + PLATE_T + 0.01)
    half = (box(-40, 40, MOT_FACE - 0.02, MOT_FACE + PLATE_T + 0.02, 0, 40)
            .rotate((0, 0, 0), (0, 1, 0), -(ECC_ANG - 90.0)).translate((MOT_X, 0, MOT_Z)))     # semipiano dal lato dell'albero
    p = p.cut(op.intersect(half))
    for a0 in (ECC_ANG + 180 - 60, ECC_ANG + 180 + 60):                                         # asole ad arco
        slot = None
        for k in range(-22, 23, 2):
            a = math.radians(a0 + k)
            h = ycyl(MOT_X + 15.5 * math.cos(a), MOT_Z + 15.5 * math.sin(a), M3_CLR, MOT_FACE - 0.01, MOT_FACE + PLATE_T + 0.01)
            slot = h if slot is None else slot.union(h)
        p = p.cut(slot)
    for x, z in PLATE_SCREWS:
        p = p.cut(ycyl(x, z, M3_CLR, MOT_FACE - 0.01, MOT_FACE + PLATE_T + 0.01))
    return p

PLATE_SCREWS = ((-110.0, 41.0), (-78.0, 41.0), (-69.0, 8.0))       # nelle zone piene attorno alla bocca della galleria

def idle_pin(dy=0.0):
    """Perno folle a molla: tampone nel foro del tubo, battuta contro la testa del tubo, gambo cavo per la molla
    con cava per la vite di ritegno. dy = arretramento (0 = spirale montata)."""
    y_sh = -TUBE_HALF                                         # faccia della battuta contro il tubo
    p = ycyl(0, HC, TUBE_BORE - 0.15, y_sh, y_sh + 6.0)        # tampone
    p = p.cut(ycyl(0, HC, TUBE_BORE + 1, y_sh + 5.0, y_sh + 6.01).cut(
        cq.Workplane("XZ").center(0, HC).circle(TUBE_BORE - 0.15).workplane(offset=-1.0).circle(TUBE_BORE - 1.2).loft(combine=True)
        .translate((0, y_sh + 5.0, 0))))                       # smusso d'invito
    p = p.union(cq.Workplane("XY").add(cq.Solid.makeCone(15.0, TUBE_BORE - 0.15, 2.0, cq.Vector(0, y_sh - 2.0, HC), cq.Vector(0, 1, 0))))   # battuta conica: centra il tubo e si stampa senza supporti
    p = p.union(ycyl(0, HC, PIN_R, y_sh - 16.0, y_sh - 2.0))   # gambo
    p = p.cut(ycyl(0, HC, 3.25, y_sh - 16.01, y_sh - 6.0))     # sede molla
    p = p.cut(box(-1.6, 1.6, y_sh - 15.2, y_sh - 5.8, HC + PIN_R - 1.2, HC + PIN_R + 0.1))   # cava vite di ritegno
    return p.translate((0, -dy, 0))

if __name__ == "__main__":
    for n, f in (("trascinatore", dog), ("cartuccia", cartridge), ("puleggia_spirale", pulley_reel), ("puleggia_motore", pulley_motor),
                 ("piastra_motore", motor_plate), ("perno_folle", idle_pin)):
        s = f().val(); bb = s.BoundingBox()
        print(f"{n:18s} valid={s.isValid()} solids={len(s.Solids())} vol={s.Volume()/1000:6.2f} x[{bb.xmin:.1f},{bb.xmax:.1f}] y[{bb.ymin:.1f},{bb.ymax:.1f}] z[{bb.zmin:.1f},{bb.zmax:.1f}]")
    print("albero motore nominale:", [round(v, 1) for v in MOT_SH], "| interasse:", round(math.hypot(MOT_SH[0], HC - MOT_SH[1]), 1),
          "| piano cinghia y =", PUL_GC, "| albero Ø8 da", DOG_Y0, "a", SHAFT_Y1, "=", SHAFT_Y1 - DOG_Y0, "mm")

HALL_Z = HC + MAG_R                     # il sensore guarda i magneti nel punto piu' alto della ruota
HALL_YF = WHEEL_Y1 + 5.5                # faccia interna della testa: 5.5 mm dalla ruota magneti (scheda + sensore)
def hall_bracket():
    """Staffa del sensore Hall KY-003: profilo a C (si stampa di fianco) fissato con la vite superiore della cartuccia;
    il modulo si avvita (M3) o si lega con una fascetta sulla testa, col sensore sulla pista dei magneti (raggio 14)."""
    y0 = CART_FL1
    c = yz_prism([(y0, HC + 12.0), (y0, HC + 27.5), (HALL_YF + 2.5, HC + 27.5), (HALL_YF + 2.5, HC + 9.0), (HALL_YF, HC + 9.0),
                  (HALL_YF, HC + 23.6), (y0 + 2.5, HC + 23.6), (y0 + 2.5, HC + 12.0)], -9.5, 9.5)
    c = c.cut(ycyl(0, HC + CART_HOLE_R, M3_CLR, y0 - 0.01, y0 + 2.51))
    c = c.cut(ycyl(0, HC + 19.0, M3_TAP, HALL_YF - 0.01, HALL_YF + 2.51))                  # vite del modulo
    for z in (HC + 11.0, HC + 20.5):                                                     # asole per fascetta
        c = c.cut(box(-9.51, -6.5, HALL_YF - 0.01, HALL_YF + 2.51, z, z + 1.8)).cut(box(6.5, 9.51, HALL_YF - 0.01, HALL_YF + 2.51, z, z + 1.8))
    return c
