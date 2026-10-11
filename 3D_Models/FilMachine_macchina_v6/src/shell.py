"""Scocca della macchina (rilascio 6): pannelli in composito di alluminio da 3 mm (Dibond) tagliati a misura, coperchio del
bagno in plexiglass trasparente, montanti d'angolo e appoggi stampati, cruscotto inclinato del display.
D06 montante d'angolo inferiore, D07 montante d'angolo superiore, D08 appoggio del piano, D09 cruscotto del display,
D10 linguetta del display. Pannelli e coperchio sono sagome REF_ (si comprano tagliati, o si tagliano a mano)."""
import math
import cadquery as cq
from machine import *
import mrefs as R
import tank as T

LIP = 2.3                                   # risalto dei montanti oltre la faccia dei pannelli
SLOT = WT + 0.3                             # fessura per il pannello
LEG = 10.0                                  # lunghezza delle fessure lungo il pannello
Z_SPLIT = 124.5                             # i montanti (261 mm) sono in due pezzi: il piatto e' da 256
EDGE = WT + LIP                             # sporgenza del piano e del coperchio oltre la base: coprono i montanti
PE = 2.15                                   # i pannelli finiscono 2.15 mm prima dello spigolo della base (li' il montante unisce le due fessure)

# ------------------------------------------------------------------ pannelli (REF)
def _holes_grid(x0, x1, z0, z1, pitch, r, plane_y=None, plane_x=None, t0=0.0, t1=0.0):
    h = None
    nx = int((x1 - x0) // pitch) + 1; nz = int((z1 - z0) // pitch) + 1
    for i in range(nx):
        for j in range(nz):
            a, b = x0 + i * pitch, z0 + j * pitch
            c = ycyl(a, b, r, t0, t1) if plane_y is not None else xcyl(a, b, r, t0, t1)
            h = c if h is None else h.union(c)
    return h

BASE_SCREWS_Z = -6.0                        # viti dei pannelli nel bordo del multistrato
def panel_front():
    """Pannello frontale 420 x 261 (Dibond 3 mm), liscio. Tre viti svasate nere nel bordo della base."""
    p = box(PE, BASE_X - PE, -WT, 0, -BASE_T, Z_TOPW)
    for x in (60.0, BASE_X / 2, BASE_X - 60.0):
        p = p.cut(ycyl(x, BASE_SCREWS_Z, 1.75, -WT - 0.01, 0.01))
    return p

def panel_rear():
    """Pannello posteriore 420 x 261, liscio."""
    p = box(PE, BASE_X - PE, BASE_Y, BASE_Y + WT, -BASE_T, Z_TOPW)
    for x in (60.0, BASE_X / 2, BASE_X - 60.0):
        p = p.cut(ycyl(x, BASE_SCREWS_Z, 1.75, BASE_Y - 0.01, BASE_Y + WT + 0.01))
    return p

VENT_LO = (14.0, 80.0, 6.0, 40.0)           # griglia d'aria bassa del vano elettronica (Y0, Y1, Z0, Z1)
VENT_HI = (70.0, 118.0, 200.0, 240.0)       # griglia d'aria alta (sopra l'alimentatore)
def panel_left():
    """Pannello sinistro 505 x 261: presa IEC, due griglie di fori Ø5 (aria del vano elettronica), foratura dell'altoparlante."""
    p = box(-WT, 0, PE, BASE_Y - PE, -BASE_T, Z_TOPW)
    p = p.cut(box(-WT - 0.01, 0.01, R.IEC_Y[0], R.IEC_Y[1], R.IEC_Z[0], R.IEC_Z[1]))
    for (y0, y1, z0, z1) in (VENT_LO, VENT_HI):
        p = p.cut(_holes_grid(y0, y1, z0, z1, 8.0, 2.5, plane_x=True, t0=-WT - 0.01, t1=0.01))
    x, y, z = R.SPK
    for k in range(13):
        if k == 0: a, r = 0.0, 0.0
        elif k < 7: a, r = math.radians(60.0 * (k - 1)), 7.0
        else: a, r = math.radians(30.0 + 60.0 * (k - 7)), 13.0
        p = p.cut(xcyl(y + r * math.cos(a), z + r * math.sin(a), 1.75, -WT - 0.01, 0.01))
    for yy in (60.0, BASE_Y / 2, BASE_Y - 60.0):
        p = p.cut(xcyl(yy, BASE_SCREWS_Z, 1.75, -WT - 0.01, 0.01))
    return p

def panel_right():
    """Pannello destro 505 x 261: fori degli attacchi (INGRESSO, SCARICO, TROPPOPIENO tank e bagno), del rubinetto del
    bagno e viti delle due piastre."""
    p = box(BASE_X, BASE_X + WT, PE, BASE_Y - PE, -BASE_T, Z_TOPW)
    p = p.cut(xcyl(BD_Y, BD_Z, 11.5, BASE_X - 0.01, BASE_X + WT + 0.01))
    rr = {"INGRESSO": 8.1, "SCARICO": 8.1, "TROPPOPIENO_TANK": 8.5, "TROPPOPIENO_BAGNO": 8.2}
    for n, (y, z) in R.CONN.items():
        p = p.cut(xcyl(y, z, rr[n], BASE_X - 0.01, BASE_X + WT + 0.01))
    for (y0, y1, z0, z1) in ((180.0, 252.0, 52.0, 162.0), (38.0, 82.0, 91.0, 135.0)):
        for y in (y0 + 6.0, y1 - 6.0):
            for z in (z0 + 6.0, z1 - 6.0):
                p = p.cut(xcyl(y, z, M3_CLR, BASE_X - 0.01, BASE_X + WT + 0.01))
    for yy in (60.0, BASE_Y / 2, BASE_Y - 60.0):
        p = p.cut(xcyl(yy, BASE_SCREWS_Z, 1.75, BASE_X - 0.01, BASE_X + WT + 0.01))
    return p

# apertura del piano per la tank: sagoma del coperchio della tank + 1.5 mm, aperta in avanti (esce la carta del 120)
_lid = [T.XO0 - T.LID_GAP - 2.5, T.XO1 + T.LID_GAP + 2.5, T.YO0 - T.LID_GAP - 2.5, T.YO1 + T.LID_GAP + 2.5]
TW_X0, TW_X1 = X_T - _lid[3] - 1.5, X_T - _lid[2] + 1.5
TW_Y1 = Y_T + _lid[1] + 1.5
FINGER_R = 12.0
DISP_SLOT = (60.0, 136.0, 32.0, 46.0)       # passaggio del cavo del display sotto il cruscotto
LID_NOTCH = (120.0, 15.0)                   # tacca nel bordo posteriore del piano per sollevare il coperchio
def deck():
    """Piano anteriore fisso (Dibond 3 mm, 430.6 x 210.3): apertura della tank a filo (con le due tacche per le dita),
    passaggio del cavo del display, fori del cruscotto, tacca per sollevare il coperchio del bagno."""
    p = box(-EDGE, BASE_X + EDGE, -EDGE, Y_DECK1, Z_TOPW, Z_DECK)
    p = p.cut(box(TW_X0, TW_X1, -EDGE - 0.01, TW_Y1, Z_TOPW - 0.01, Z_DECK + 0.01))
    yc = (TW_Y1 + 0.0) / 2
    for x in (TW_X0, TW_X1):
        p = p.cut(zcyl(x, yc, FINGER_R, Z_TOPW - 0.01, Z_DECK + 0.01))
    p = p.cut(box(DISP_SLOT[0], DISP_SLOT[1], DISP_SLOT[2], DISP_SLOT[3], Z_TOPW - 0.01, Z_DECK + 0.01))
    for (x, y) in WEDGE_SCREWS:
        p = p.cut(zcyl(x, y, M3_CLR, Z_TOPW - 0.01, Z_DECK + 0.01))
    p = p.cut(zcyl(LID_NOTCH[0], Y_DECK1, LID_NOTCH[1], Z_TOPW - 0.01, Z_DECK + 0.01))
    return p

def lid():
    """Coperchio del bagno in plexiglass trasparente da 3 mm (430.6 x 310.3), appoggiato: si solleva dalla tacca del piano.
    Una fila di fori Ø6 vicino al bordo posteriore fa uscire l'aria umida (meno condensa)."""
    p = box(-EDGE, BASE_X + EDGE, Y_DECK1 + 0.5, BASE_Y + EDGE, Z_TOPW, Z_DECK)
    for i in range(9):
        p = p.cut(zcyl(50.0 + 40.0 * i, BASE_Y - 8.0, 3.0, Z_TOPW - 0.01, Z_DECK + 0.01))
    return p

def bulkhead_panel():
    """Paratia in Dibond tra elettronica e valvole (193 x 249), con due fori Ø22 per i passacavi."""
    from frame import PAR_Y0, PAR_Y1, DIV_X0
    p = box(0.3, DIV_X0 - 0.2, PAR_Y0, PAR_Y1, 2.0, Z_TOPW)
    for (x, z) in ((40.0, 170.0), (110.0, 170.0)):
        p = p.cut(ycyl(x, z, 11.0, PAR_Y0 - 0.01, PAR_Y1 + 0.01))
    return p

def divider_panel():
    """Divisorio in Dibond tra elettronica e cavalletto (122 x 249), con un foro Ø22 per i cavi del cavalletto."""
    from frame import PAR_Y0, DIV_X0, DIV_X1
    p = box(DIV_X0, DIV_X1, 0.3, PAR_Y0 - 0.2, 2.0, Z_TOPW)
    return p.cut(xcyl(60.0, 150.0, 11.0, DIV_X0 - 0.01, DIV_X1 + 0.01))

# ------------------------------------------------------------------ D06 / D07 montanti d'angolo
def post_local(lower=True):
    """Montante d'angolo nel riferimento dell'angolo anteriore sinistro (esterno verso -X e -Y). Due fessure tengono le
    teste dei pannelli; sotto la quota 0 restano solo il blocco d'angolo e le labbra esterne (dentro c'e' la base).
    Inferiore e superiore si incastrano con un tenone 3 x 3 x 8 nel blocco d'angolo."""
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

CORNERS = [((0.0, 0.0), 0.0), ((BASE_X, 0.0), 90.0), ((BASE_X, BASE_Y), 180.0), ((0.0, BASE_Y), 270.0)]
def post(i, lower=True):
    (x, y), a = CORNERS[i]
    return post_local(lower).rotate((0, 0, 0), (0, 0, 1), a).translate((x, y, 0))

# ------------------------------------------------------------------ D08 appoggio del piano
SUP_X = (22.0, 270.0, 340.0, 397.0)  # nei varchi tra le linee che scavalcano il bordo del bagno
def deck_support(x):
    """Appoggio del piano e del coperchio sulla fascia anteriore del contenitore: regge il bordo posteriore del piano fisso
    e quello anteriore del coperchio trasparente; il dente esterno lo centra sulla fascia."""
    s = box(x - 6.0, x + 6.0, TUB_Y0 - 4.0, TUB_Y0 + 14.0, RIM_Z, Z_TOPW)
    s = s.union(box(x - 6.0, x + 6.0, TUB_Y0 - 4.0, TUB_Y0 - 0.3, RIM_Z - 10.0, RIM_Z))
    s = s.cut(box(x - 3.0, x + 3.0, TUB_Y0 + 2.0, TUB_Y0 + 10.0, RIM_Z + 6.0, Z_TOPW - 6.0))                # alleggerimento
    return s

# ------------------------------------------------------------------ D09 cruscotto, D10 linguette del display
W_ANG = 34.0                                # inclinazione della faccia del display (dal piano)
W_P0 = (4.0, Z_DECK + 5.0)                  # spigolo anteriore della faccia (Y, Z)
W_S = 88.0                                  # lunghezza della faccia lungo la pendenza
W_X0, W_X1 = 14.0, 182.0
W_T = 3.0
_U = (math.cos(math.radians(W_ANG)), math.sin(math.radians(W_ANG)))
def w_pt(s, w):
    """(Y, Z) del punto a distanza s lungo la faccia e w perpendicolare (w > 0 verso l'esterno)."""
    return (W_P0[0] + _U[0] * s - _U[1] * w, W_P0[1] + _U[1] * s + _U[0] * w)

def to_face(p):
    """Dal riferimento della faccia (x = X, y = s lungo la pendenza, z = w verso l'esterno) a quello della macchina."""
    return p.rotate((0, 0, 0), (1, 0, 0), W_ANG).translate((0, W_P0[0], W_P0[1]))

_top = w_pt(W_S, 0.0)
W_CAP = 12.0                                # la fascia alta scende di 12 gradi verso il retro: stampata con la faccia del display
_tc = math.tan(math.radians(W_CAP))         # sul piatto pende a 45 gradi invece di 34 (niente supporti)
W_PROFILE = [(W_P0[0], Z_DECK), W_P0, _top, (_top[0] + 14.0, _top[1] - 14.0 * _tc), (_top[0] + 26.0, Z_DECK)]
WEDGE_SCREWS = [(W_X0 + W_T + 3.5, W_P0[0] + 16.0), (W_X1 - W_T - 3.5, W_P0[0] + 16.0), (W_X0 + W_T + 3.5, _top[0] + 10.0), (W_X1 - W_T - 3.5, _top[0] + 10.0)]
UC, VC = (W_X0 + W_X1) / 2, 44.0            # centro della scheda sulla faccia
def wedge():
    """D09: cruscotto inclinato a 34 gradi sul piano anteriore, sopra il vano elettronica: guscio aperto sotto, finestra
    96 x 58 per il display da 4.3", cornice di centraggio e quattro colonnette per le linguette D10 che trattengono la
    scheda. Quattro viti M3 da sotto il piano. Si stampa con la faccia del display sul piatto (la fascia alta, inclinata
    di 12 gradi verso il retro, sul piatto pende a 45 gradi)."""
    outer = yz_prism(W_PROFILE, W_X0, W_X1).edges("|X").fillet(3.0)
    inner = yz_prism([(W_P0[0] + W_T, Z_DECK - 0.01), w_pt(W_T * 0.6, -W_T), w_pt(W_S - 1.0, -W_T),
                      (_top[0] + 12.0, _top[1] - W_T - 12.0 * _tc), (_top[0] + 26.0 - W_T * 1.5, Z_DECK - 0.01)], W_X0 + W_T, W_X1 - W_T)
    w = outer.cut(inner)
    hl, hw = R.DISP_L / 2 + 0.3, R.DISP_W / 2 + 0.3
    f = box(UC - 48.0, UC + 48.0, VC - 29.0, VC + 29.0, -W_T - 0.01, 0.01)                         # finestra
    w = w.cut(to_face(f))
    fr = None
    for (u0, u1, v0, v1) in ((UC - hl - 2, UC + hl + 2, VC - hw - 2, VC - hw), (UC - hl - 2, UC + hl + 2, VC + hw, VC + hw + 2),
                             (UC - hl - 2, UC - hl, VC - hw, VC + hw), (UC + hl, UC + hl + 2, VC - hw, VC + hw)):
        b = box(u0, u1, v0, v1, -W_T - 2.0, -W_T + 0.01)
        fr = b if fr is None else fr.union(b)
    for su in (1, -1):
        for sv in (1, -1):
            u, v = UC + su * (hl + 6.5), VC + sv * 18.0
            fr = fr.union(zcyl(u, v, 4.0, -W_T - R.DISP_T, -W_T + 0.01))
    w = w.union(to_face(fr))
    for su in (1, -1):
        for sv in (1, -1):
            w = w.cut(to_face(zcyl(UC + su * (hl + 6.5), VC + sv * 18.0, M3_TAP, -W_T - R.DISP_T - 0.01, -0.6)))
    for (x, y) in WEDGE_SCREWS:                                                                          # torrette delle viti
        w = w.union(zcyl(x, y, 4.0, Z_DECK, Z_DECK + 10.0)).cut(zcyl(x, y, M3_TAP, Z_DECK - 0.01, Z_DECK + 9.0))
    return w

TAB_L0, TAB_L1, TAB_HW, TAB_T = 4.0, 13.0, 4.0, 3.0
def disp_tab_local():
    """D10: linguetta girevole: un'estremita' sulla colonnetta del cruscotto (vite 3 x 8), l'altra sul bordo della scheda display."""
    return box(-TAB_L0, TAB_L1, -TAB_HW, TAB_HW, 0, TAB_T).cut(zcyl(0, 0, M3_CLR, -0.01, TAB_T + 0.01))

def disp_tabs():
    out = []; hl = R.DISP_L / 2 + 0.3
    for su in (1, -1):
        for sv in (1, -1):
            t = disp_tab_local()
            if su > 0: t = t.rotate((0, 0, 0), (0, 0, 1), 180)
            out.append(to_face(t.translate((UC + su * (hl + 6.5), VC + sv * 18.0, -W_T - R.DISP_T - TAB_T))))
    return out

def display():
    """La scheda display JC4880P443C dietro la finestra, vetro contro la faccia interna del cruscotto."""
    return to_face(R.display_local().translate((UC, VC, -W_T)))

PARTS = [("D06_montante_inferiore", lambda: post_local(True)), ("D07_montante_superiore", lambda: post_local(False)),
         ("D08_appoggio_piano", lambda: deck_support(0.0)), ("D09_cruscotto_display", wedge), ("D10_linguetta_display", disp_tab_local)]

if __name__ == "__main__":
    print("apertura tank X", round(TW_X0, 1), round(TW_X1, 1), "Y fino a", round(TW_Y1, 1), "| cruscotto: sommita' Z", round(_top[1], 1), "profondita' Y", round(_top[0] + 26.0, 1))
    for n, f in PARTS + [("pannello frontale", panel_front), ("pannello sinistro", panel_left), ("pannello destro", panel_right),
                         ("piano", deck), ("coperchio", lid), ("paratia", bulkhead_panel), ("divisorio", divider_panel)]:
        s = f().val(); bb = s.BoundingBox()
        print(f"{n:26s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:6.1f} cm3  {bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f}")
