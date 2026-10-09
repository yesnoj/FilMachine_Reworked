"""Tank a tenuta di luce con caricatore integrato: corpo monoblocco (vasca spirale + ripiano asciutto + galleria
motore + vano servo), coperchio. Si stampa in piedi cosi' com'e', senza supporti: tutti gli sbalzi sono a 45 gradi."""
import math
import cadquery as cq
from common import *

def box(x0, x1, y0, y1, z0, z1):
    return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))

def xz_prism(pts, y0, y1):
    """Poligono nel piano XZ estruso lungo Y da y0 a y1."""
    return cq.Workplane("XZ").polyline(pts).close().extrude(-(y1 - y0)).translate((0, y0, 0))

def yz_prism(pts, x0, x1):
    """Poligono nel piano YZ estruso lungo X da x0 a x1."""
    return cq.Workplane("YZ").polyline(pts).close().extrude(x1 - x0).translate((x0, 0, 0))

def xy_prism(pts, z0, z1):
    return cq.Workplane("XY").polyline(pts).close().extrude(z1 - z0).translate((0, 0, z0))

def ycyl(x, z, r, y0, y1):
    return cq.Workplane("XZ").center(x, z).circle(r).extrude(-(y1 - y0)).translate((0, y0, 0))

def zcyl(x, y, r, z0, z1):
    return cq.Workplane("XY").center(x, y).circle(r).extrude(z1 - z0).translate((0, 0, z0))

def xcyl(y, z, r, x0, x1):
    return cq.Workplane("YZ").center(y, z).circle(r).extrude(x1 - x0).translate((x0, 0, 0))

# ---- quote derivate ----
CH_L = FLOOR_L[1] + (RC + FLOOR_L[0])          # quota a cui lo smusso sinistro incontra la parete
CH_R = FLOOR_R[1] + (RC - FLOOR_R[0])
PIL_Y0 = YO0 - 14.0                            # pilastro esterno lato perno folle (boccola perno + montante pescante)
RIS_X, RIS_Y, RIS_R, RIS_ZT = 17.0, YO0 - 9.5, 2.75, 70.0   # montante del pescante
PAD_Y1 = YO1 + 4.0                             # ringrosso esterno lato trasmissione (sede cartuccia cuscinetti)
CART_R, CART_HOLE_R, CART_N = 11.5, 17.0, 3    # foro per la cartuccia e viti della flangia
OVF_X, OVF_Y, OVF_R = XO1 + 6.5, 27.0, 4.35    # canale verticale del troppopieno (tubo 8x12 infilato da sotto)
SRV_WIN = (SRV_Z - 17.1, SRV_Z + 6.1)          # finestra per il corpo del servo (asse a 5.9 mm da un'estremita')
SRV_SCREW_Z = (SRV_Z - 5.5 - 13.9, SRV_Z - 5.5 + 13.9)
ROD_SLOT = (SRV_X - 3.9, SRV_X + 7.6, -3.6, 0.2)
PIN_R = 5.0                                    # gambo del perno folle a molla
MOTOR_PLATE_SCREWS = ((-110.0, 41.0), (-78.0, 41.0), (-69.0, 8.0))
BAY_COVER_SCREWS = ((SRV_X + 1.8, SBAY_Y0 - 4.5), (SRV_X + 1.8, SRV_YTAB + 2.5))
PAPER_HALF = 33.5                              # semi-larghezza della feritoia per la carta del 120 (carta 63 mm)
SHELF_SCREWS = ([(x, s * y) for (x, y) in ((BR_X0 - 3.5, 41.5), (BR_X1 + 3.5, 41.5)) for s in (1, -1)]
                + [(x, s * y) for (x, y) in ((-110.0, 30.0), (-85.0, 27.9)) for s in (1, -1)])
FEET = [(-108.0, YO0 - 7.0), (-108.0, YO1 + 7.0), (40.0, YO0 - 7.0), (40.0, YO1 + 7.0)]   # asole di fissaggio alla macchina

def trough_cavity():
    """Volume interno della vasca spirale (fondo a ottagono + canaletta)."""
    tr = xz_prism([(-RC, Z_RIM + 1), (-RC, CH_L), FLOOR_L, FLOOR_R, (RC, CH_R), (RC, Z_RIM + 1)], Y_IDL, Y_DRV)
    return tr.union(box(GUT_X0, GUT_X1, Y_IDL, Y_DRV, GUT_Z, 0.5))

def body():
    b = box(XO0, XO1, YO0, YO1, Z_BOT, Z_RIM)
    # --- aggiunte esterne (tutte partono dal piatto di stampa: nessuno sbalzo) ---
    b = b.union(box(-12.0, 12.0, PIL_Y0, YO0, Z_BOT, HC)).union(ycyl(0, HC, 12.0, PIL_Y0, YO0))      # boccola perno folle
    b = b.union(box(12.0, 21.5, PIL_Y0, YO0, Z_BOT, RIS_ZT))                                        # montante pescante
    b = b.union(zcyl(RIS_X, RIS_Y, 4.1, RIS_ZT, RIS_ZT + 14.0))                                     # portagomma (tubo silicone 8x12)
    for k in range(3):
        z0 = RIS_ZT + 3.0 + 3.6 * k
        b = b.union(cq.Workplane("XY").add(cq.Solid.makeCone(4.85, 4.1, 3.4)).translate((RIS_X, RIS_Y, z0)))
    b = b.union(box(-24.0, 24.0, YO1, PAD_Y1, Z_BOT, HC)).union(ycyl(0, HC, 24.0, YO1, PAD_Y1))       # ringrosso cartuccia
    b = b.union(box(XO1, XO1 + 13.0, OVF_Y - 9.0, OVF_Y + 9.0, Z_BOT, Z_OVF + 8.0))                  # colonna troppopieno
    for x, y in FEET:                                                                               # piedini con asola
        y0, y1 = (y - 7.0, YO0) if y < 0 else (YO1, y + 7.0)
        b = b.union(box(x - 9.0, x + 9.0, y0, y1, Z_BOT, Z_BOT + 5.0))
    # --- cavita' ---
    b = b.cut(trough_cavity())
    b = b.cut(box(X_DRY0, X_PART, Y_IDL, Y_DRV, Z_SH, Z_RIM + 1))                                   # ripiano asciutto
    b = b.cut(box(X_PART - 0.01, -RC + 0.01, Y_IDL, Y_DRV, Z_PART, Z_RIM + 1))                      # sopra il divisorio
    b = b.cut(xz_prism([(TUN_X0, TUN_ZF), (TUN_X1, TUN_ZF), (TUN_X1, TUN_ZW),
                        (MOT_X, TUN_ZW + (TUN_X1 - TUN_X0) / 2), (TUN_X0, TUN_ZW)], YO0 - 1, YO1 + 1))  # galleria motore
    # vano servo (aperto sotto) con le due traverse per le alette
    b = b.cut(box(SBAY_X0, SBAY_X1, SBAY_Y0, SBAY_Y1, Z_BOT - 0.01, SBAY_ZT))
    b = b.union(box(SBAY_X0, SBAY_X1, SRV_YTAB, SRV_YTAB + 5.0, SRV_WIN[1], SBAY_ZT))
    b = b.union(box(SBAY_X0, SBAY_X1, SRV_YTAB, SRV_YTAB + 5.0, Z_BOT, SRV_WIN[0]))
    for z in SRV_SCREW_Z:
        b = b.cut(ycyl(SRV_X, z, M2_TAP, SRV_YTAB - 0.01, SRV_YTAB + 5.01))
    b = b.cut(box(ROD_SLOT[0], ROD_SLOT[1], ROD_SLOT[2], ROD_SLOT[3], SBAY_ZT - 0.01, Z_SH + 0.01))  # passaggio biella
    # perno folle a molla: foro cieco dall'interno, scarico in basso, vite di ritegno dall'alto
    b = b.cut(ycyl(0, HC, PIN_R + 0.2, PIL_Y0 + 1.5, Y_IDL + 0.01))
    b = b.cut(box(-0.8, 0.8, PIL_Y0 + 1.5, Y_IDL + 0.01, HC - PIN_R - 1.2, HC - PIN_R + 0.3))
    b = b.cut(zcyl(0, YO0 - 5.5, M3_TAP, HC + PIN_R - 0.5, HC + 12.01))
    # pescante: galleria dal fondo della canaletta + montante verticale fino al portagomma
    b = b.cut(ycyl(RIS_X, GUT_Z + 2.5, 2.5, RIS_Y, Y_IDL + 0.01))
    b = b.cut(zcyl(RIS_X, RIS_Y, RIS_R, GUT_Z, RIS_ZT + 14.01))
    # sede della cartuccia cuscinetti/paraolio
    b = b.cut(ycyl(0, HC, CART_R, Y_DRV - 0.01, PAD_Y1 + 0.01))
    for k in range(CART_N):
        a = math.radians(90 + 120 * k)
        b = b.cut(ycyl(CART_HOLE_R * math.cos(a), HC + CART_HOLE_R * math.sin(a), M3_TAP, PAD_Y1 - 6.0, PAD_Y1 + 0.01))
    # troppopieno: soglia nella parete + canale verticale aperto sotto
    b = b.cut(box(RC - 0.01, OVF_X, OVF_Y - 7.0, OVF_Y + 7.0, Z_OVF, Z_OVF + 4.0))
    b = b.cut(zcyl(OVF_X, OVF_Y, OVF_R, Z_BOT - 0.01, Z_OVF + 4.0))
    for x, y in FEET:
        b = b.cut(box(x - 4.0, x + 4.0, y - 2.2, y + 2.2, Z_BOT - 0.01, Z_BOT + 5.01))
    for x, y in BAY_COVER_SCREWS:                                                                   # viti del coperchietto vano servo
        b = b.cut(zcyl(x, y, M3_TAP, Z_BOT - 0.01, Z_BOT + 8.0))
    for (x, y) in SHELF_SCREWS:                                                                     # ponte e culle: fori ciechi nel ripiano
        b = b.cut(zcyl(x, y, M3_TAP, Z_SH - 9.0, Z_SH + 0.01))
    b = b.cut(box(XO0 - 0.01, X_DRY0 + 0.01, -PAPER_HALF, PAPER_HALF, Z_RIM - 1.0, Z_RIM + 0.01))      # feritoia carta del 120
    for x, z in MOTOR_PLATE_SCREWS:                                                                 # viti della piastra motore
        b = b.cut(ycyl(x, z, M3_TAP, YO1 - 9.0, YO1 + 0.01))
    return b

if __name__ == "__main__":
    import time
    t0 = time.time()
    s = body().val(); bb = s.BoundingBox()
    print(f"corpo tank: valid={s.isValid()} solids={len(s.Solids())} vol={s.Volume()/1000:.0f} cm3 "
          f"bbox={bb.xlen:.1f}x{bb.ylen:.1f}x{bb.zlen:.1f}  ({time.time()-t0:.0f}s)")


# ---------------------------------------------------------------- coperchio a labirinto
LID_GAP, LID_SKIRT, LID_LIP, LID_T = 0.4, 8.0, 6.0, 3.0

def lid():
    """Coperchio: piano + bordo esterno che scende fuori dalle pareti + labbro interno che scende dentro (labirinto a U).
    Sul lato caricatore bordo e labbro sono piu' larghi per lasciar passare la carta del 120. Si stampa capovolto."""
    g = LID_GAP
    top = box(XO0 - g - 2.5, XO1 + g + 2.5, YO0 - g - 2.5, YO1 + g + 2.5, Z_RIM, Z_RIM + LID_T)
    skirt = box(XO0 - g - 2.5, XO1 + g + 2.5, YO0 - g - 2.5, YO1 + g + 2.5, Z_RIM - LID_SKIRT, Z_RIM).cut(
            box(XO0 - g, XO1 + g, YO0 - g, YO1 + g, Z_RIM - LID_SKIRT - 0.01, Z_RIM + 0.01))
    lip = box(X_DRY0 + g, RC - g, Y_IDL + g, Y_DRV - g, Z_RIM - LID_LIP, Z_RIM).cut(
          box(X_DRY0 + g + 2.0, RC - g - 2.0, Y_IDL + g + 2.0, Y_DRV - g - 2.0, Z_RIM - LID_LIP - 0.01, Z_RIM + 0.01))
    l = top.union(skirt).union(lip)
    # passaggio carta 120: sul lato caricatore il labirinto si allarga a 1.5 mm per la larghezza della carta
    l = l.cut(box(XO0 - 1.5, XO0 - g + 0.01, -PAPER_HALF, PAPER_HALF, Z_RIM - LID_SKIRT - 0.01, Z_RIM))
    l = l.cut(box(X_DRY0 + g - 0.01, X_DRY0 + 1.5, -PAPER_HALF, PAPER_HALF, Z_RIM - LID_LIP - 0.01, Z_RIM))
    return l

def bay_cover():
    """Coperchietto del vano servo (da sotto): chiude alla luce il vano, con una tacca per il cavo."""
    c = box(SBAY_X0 - 3.0, SBAY_X1 + 3.0, SBAY_Y0 - 8.5, SBAY_Y1 + 3.0, Z_BOT - 2.0, Z_BOT)
    for x, y in BAY_COVER_SCREWS:
        c = c.cut(zcyl(x, y, M3_CLR, Z_BOT - 2.01, Z_BOT + 0.01))
    return c.cut(box(SBAY_X1 - 6.0, SBAY_X1 - 2.0, SBAY_Y1 - 4.0, SBAY_Y1 + 3.01, Z_BOT - 2.01, Z_BOT + 0.01))
