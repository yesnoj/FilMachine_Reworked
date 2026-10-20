"""Scocca della macchina (rilascio 7): base e piano in multistrato, pannelli in composito di alluminio da 3 mm (Dibond)
tagliati a misura nelle fessure dei montanti d'angolo stampati; il bagno in PMMA (mrefs) e' a vista davanti.
D06 montante d'angolo inferiore, D07 montante d'angolo superiore. Pannelli: sagome REF_ (si comprano tagliati)."""
import math
import cadquery as cq
from machine import *
import mrefs as R
import tank as T

LIP = 2.3                                   # risalto dei montanti oltre la faccia dei pannelli
SLOT = WT + 0.3                             # fessura per il pannello
LEG = 10.0                                  # lunghezza delle fessure lungo il pannello
Z_SPLIT = DECK_Z0 - 2.0                     # i montanti sono in due pezzi: sotto e sopra il piano
Z_TOPW = H - WT                             # sommita' dei pannelli verticali = faccia inferiore del pannello di copertura
PE = 2.15                                   # i pannelli finiscono 2.15 mm prima dello spigolo della base
BASE_SCREWS_Z = -6.0                        # viti dei pannelli nel bordo del multistrato
EDGE = WT + LIP

def _holes_grid(a0, a1, z0, z1, pitch, r, plane, t0, t1):
    h = None
    na = int((a1 - a0) // pitch) + 1; nz = int((z1 - z0) // pitch) + 1
    for i in range(na):
        for j in range(nz):
            a, b = a0 + i * pitch, z0 + j * pitch
            c = ycyl(a, b, r, t0, t1) if plane == "Y" else xcyl(a, b, r, t0, t1)
            h = c if h is None else h.union(c)
    return h

# ------------------------------------------------------------------ pannelli (REF)
BATH_OPEN = (BX0 - BATH_T - 0.5, BX1 + BATH_T + 0.5, PZ + 6.0)    # apertura del frontale sul bagno: x0, x1, z0 (il fondo del bagno resta nascosto)
LOGO = (DISP_X + 80.0, DISP_X + 170.0, 24.0, 30.0)
def panel_front():
    """Pannello frontale (Dibond 3 mm): a U attorno al bagno (si vede la parete anteriore in PMMA), finestra del display 4.3",
    fori dei tre rubinetti (Ø21) e dell'uscita RECUPERO (Ø13), fessura del logo, viti nel bordo della base."""
    p = box(PE, W - PE, -WT, 0, -BASE_T, Z_TOPW)
    p = p.cut(box(BATH_OPEN[0], BATH_OPEN[1], -WT - 0.01, 0.01, BATH_OPEN[2], Z_TOPW + 1.0))
    p = p.cut(box(DISP_X - DISP_WIN[0] / 2, DISP_X + DISP_WIN[0] / 2, -WT - 0.01, 0.01, DISP_Z - DISP_WIN[1] / 2, DISP_Z + DISP_WIN[1] / 2))
    for x in TAP_X:
        p = p.cut(ycyl(x, TAP_Z, 10.5, -WT - 0.01, 0.01))
    p = p.cut(ycyl(REC_X, REC_Z, 6.5, -WT - 0.01, 0.01))
    for x in (60.0, W / 2, W - 60.0):
        p = p.cut(ycyl(x, BASE_SCREWS_Z, 1.75, -WT - 0.01, 0.01))
    return p

VENT_REAR = (240.0, 400.0, 95.0, 135.0)     # griglia d'aria sopra le valvole (X0, X1, Z0, Z1)
def panel_rear():
    """Pannello posteriore: presa IEC, quattro attacchi (SCARICO, troppopieni, INGRESSO), griglia d'aria, viti nella base."""
    p = box(PE, W - PE, D_, D_ + WT, -BASE_T, Z_TOPW)
    p = p.cut(box(IEC_X[0], IEC_X[1], D_ - 0.01, D_ + WT + 0.01, IEC_Z[0], IEC_Z[1]))
    for n, (x, z) in CONN.items():
        p = p.cut(ycyl(x, z, 8.1, D_ - 0.01, D_ + WT + 0.01))
    p = p.cut(_holes_grid(VENT_REAR[0], VENT_REAR[1], VENT_REAR[2], VENT_REAR[3], 8.0, 2.5, "Y", D_ - 0.01, D_ + WT + 0.01))
    for x in (60.0, W / 2, W - 60.0):
        p = p.cut(ycyl(x, BASE_SCREWS_Z, 1.75, D_ - 0.01, D_ + WT + 0.01))
    return p

VENT_LEFT_LO = (30.0, 200.0, 30.0, 110.0)    # aria dell'alimentatore, in piedi dietro il pannello (Y0, Y1, Z0, Z1)
VENT_LEFT_HI = (30.0, 120.0, PZ + 20.0, PZ + 60.0)   # aria del vano motore
def panel_left():
    """Pannello sinistro: due griglie di fori Ø5 (alimentatore in basso, vano motore in alto), viti nella base."""
    p = box(-WT, 0, PE, D_ - PE, -BASE_T, Z_TOPW)
    for (y0, y1, z0, z1) in (VENT_LEFT_LO, VENT_LEFT_HI):
        p = p.cut(_holes_grid(y0, y1, z0, z1, 8.0, 2.5, "X", -WT - 0.01, 0.01))
    for y in (60.0, D_ / 2, D_ - 60.0):
        p = p.cut(xcyl(y, BASE_SCREWS_Z, 1.75, -WT - 0.01, 0.01))
    return p

VENT_RIGHT = (30.0, 200.0, 20.0, 60.0)
def panel_right():
    """Pannello destro: griglia d'aria del piedistallo (valvole e Kamoer), viti nella base."""
    p = box(W, W + WT, PE, D_ - PE, -BASE_T, Z_TOPW)
    p = p.cut(_holes_grid(VENT_RIGHT[0], VENT_RIGHT[1], VENT_RIGHT[2], VENT_RIGHT[3], 8.0, 2.5, "X", W - 0.01, W + WT + 0.01))
    for y in (60.0, D_ / 2, D_ - 60.0):
        p = p.cut(xcyl(y, BASE_SCREWS_Z, 1.75, W - 0.01, W + WT + 0.01))
    return p

def panel_top():
    """Pannello di copertura (Dibond 3 mm): copre il vano motore, l'intercapedine posteriore e quella destra; il bagno resta
    scoperto (il coperchio in PMMA e' a filo). Appoggia sui montanti e sulle pareti del bagno."""
    p = box(-EDGE, W + EDGE, -EDGE, D_ + EDGE, Z_TOPW, H)
    return p.cut(box(BX0 - BATH_T - 0.5, BX1 + BATH_T + 0.5, BY0 - BATH_T - 0.5, BY1 + BATH_T + 3.5, Z_TOPW - 0.01, H + 0.01))

# ------------------------------------------------------------------ D06 / D07 montanti d'angolo
def post_local(lower=True):
    """Montante d'angolo nel riferimento dell'angolo anteriore sinistro (esterno verso -X e -Y). Due fessure tengono le
    teste dei pannelli; sotto la quota 0 restano solo il blocco d'angolo e le labbra esterne (dentro c'e' la base).
    Inferiore e superiore si incastrano con un tenone 3 x 3 x 8 nel blocco d'angolo; il piano (multistrato) ha l'angolo
    smussato di 16 mm e passa accanto al montante."""
    z0, z1 = (-BASE_T, Z_SPLIT) if lower else (Z_SPLIT, Z_TOPW)
    o = -(SLOT + 2.0)
    s = box(o, 2.0, o, -SLOT, z0, z1).union(box(o, -SLOT, o, 2.0, z0, z1))                   # labbra esterne
    s = s.union(box(o, 2.0, o, 0.0, z0, z1)).union(box(o, 0.0, o, 2.0, z0, z1))               # blocco d'angolo + ponticelli
    s = s.union(box(2.0, LEG, o, -SLOT, z0, z1)).union(box(o, -SLOT, 2.0, LEG, z0, z1))
    zi = max(z0, 0.0)
    s = s.union(box(0, LEG, 0, 2.0, zi, z1)).union(box(0, 2.0, 0, LEG, zi, z1))               # labbra interne
    s = s.edges("|Z").edges("<X and <Y").fillet(3.5)
    if lower:
        s = s.union(box(o + 1.35, o + 4.05, o + 1.35, o + 4.05, Z_SPLIT, Z_SPLIT + 7.7))
    else:
        s = s.cut(box(o + 1.2, o + 4.2, o + 1.2, o + 4.2, Z_SPLIT - 0.01, Z_SPLIT + 8.0))
    return s

CORNERS = [((0.0, 0.0), 0.0), ((W, 0.0), 90.0), ((W, D_), 180.0), ((0.0, D_), 270.0)]
def post(i, lower=True):
    (x, y), a = CORNERS[i]
    return post_local(lower).rotate((0, 0, 0), (0, 0, 1), a).translate((x, y, 0))

PANELS = [("REF_pannello_frontale", panel_front), ("REF_pannello_posteriore", panel_rear), ("REF_pannello_sinistro", panel_left),
          ("REF_pannello_destro", panel_right), ("REF_pannello_copertura", panel_top)]
PARTS = [("D06_montante_inferiore", lambda: post_local(True)), ("D07_montante_superiore", lambda: post_local(False))]

if __name__ == "__main__":
    for n, f in PARTS + PANELS:
        s = f().val(); bb = s.BoundingBox()
        print(f"{n:26s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:6.1f} cm3  {bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f}")
