"""Parti stampate della struttura (serie D), nel riferimento macchina.
Cavalletto della tank: D01 piano, D02/D03 fianchi, D04 frontale, D05 carter della trasmissione.
Vano elettronica: D06/D07 fianchi, D08 frontale, D09 plancia del display, D10 tetto, D11 paratia posteriore,
D12 ripiano a griglia, D13/D14 colonnine, D15 morsetto per schede, D16 zoccolo dell'alimentatore,
D17 linguetta girevole che trattiene la scheda display."""
import math
import cadquery as cq
from machine import *
import tank as T
from mrefs import X_RISE, Y_RISE, OVF_XM, OVF_YM, Z_OVF_HOSE, SOCK_Z, OVF_ANG, PSU_X0, PSU_Y0, PSU_Z0, DISP_L, DISP_W

Z_DK0 = Z_STAND - DECK_T                       # faccia inferiore del piano del cavalletto
FEET_M = [tank_xy(x, y) for (x, y) in T.FEET]  # viti dei piedini della tank
SP = 4.0                                       # spessore dei fianchi del cavalletto
FL = 14.0                                      # larghezza delle flange
ST_SCREWS_Y = (30.0, 100.0, 175.0)             # viti piano-fianco sinistro; sul destro le stesse ordinate + 12 per le viti nella base
ST_SCREWS_YR = (20.0, 60.0, 175.0)             # viti piano-fianco destro (lontane dai passaggi del tubo e del cavo del motore)
CABLE_MOT = (X_RISE, 36.0)                     # passacavo del motore nel piano (accanto alla bocca della galleria)
CABLE_HALL = (226.0, 100.0)                    # passacavo del sensore Hall nel piano (sotto il carter, oltre la flangia del fianco)

SOCK_R, SOCK_L = 5.85, 12.0                    # innesto del troppopieno: sede Ø11.7 per il tubo in silicone da 12 mm (0.3 di interferenza)
CV_X0, CV_X1, CV_Y0, CV_Y1, CV_Z1, CV_T = ST_X0 + 0.5, X_T - T.PAD_Y1 - 0.4, 10.0, 158.0, Z_T0 + 82.0, 2.5     # carter della trasmissione
COVER_TAB_X, COVER_TAB_Y = CV_X0 + CV_T + 7.0, (CV_Y0 + CV_T + 6.0, CV_Y1 - CV_T - 6.0)

# ------------------------------------------------------------------ D01 piano del cavalletto
def stand_deck():
    """Piano del cavalletto: la tank ci appoggia e si fissa con quattro viti M3 nei piedini. Finestra per il coperchietto
    del vano servo, imbutino del troppopieno con sotto l'innesto per il tubo in silicone (orizzontale, verso destra, in
    leggera discesa: il tubo ci entra a pressione e non deve curvare), passaggi per tubo e cavi. Si stampa capovolto."""
    d = box(ST_X0, ST_X1, ST_Y0, ST_Y1, Z_DK0, Z_STAND)
    for (x, y) in FEET_M:                                                             # borchie e fori per le viti dei piedini
        d = d.union(zcyl(x, y, 5.0, Z_DK0 - 7.0, Z_DK0)).cut(zcyl(x, y, M3_TAP, Z_DK0 - 7.01, Z_STAND + 0.01))
    # finestra per il coperchietto del vano servo (sporge 2 mm sotto la tank), le teste delle sue viti e il cavo del servo
    cx0, cy0 = tank_xy(SBAY_X0 - 3.0, SBAY_Y1 + 3.0); cx1, cy1 = tank_xy(SBAY_X1 + 3.0, SBAY_Y0 - 8.5)
    d = d.cut(box(min(cx0, cx1) - 1.0, max(cx0, cx1) + 1.0, min(cy0, cy1) - 1.0, max(cy0, cy1) + 1.0, Z_DK0 - 0.01, Z_STAND + 0.01))
    # imbutino del troppopieno + blocco con innesto orizzontale per il tubo 8x12: sede tonda piu' stretta del tubo (entra a pressione e fa tenuta)
    d = d.union(box(OVF_XM - 9.0, OVF_XM + SOCK_L + 2.0, OVF_YM - 9.0, ST_Y1, Z_DK0 - 18.5, Z_DK0))
    d = d.cut(zcyl(OVF_XM, OVF_YM, 4.0, SOCK_Z, Z_STAND + 0.01))
    d = d.cut(cq.Workplane("XY").add(cq.Solid.makeCone(4.0, 7.5, 4.0)).translate((OVF_XM, OVF_YM, Z_STAND - 3.99)))
    d = d.cut(cq.Workplane("XY").add(cq.Solid.makeSphere(4.0)).translate((OVF_XM, OVF_YM, SOCK_Z)))
    sock = xcyl(OVF_YM, SOCK_Z, 4.0, OVF_XM, OVF_XM + 2.5).union(xcyl(OVF_YM, SOCK_Z, SOCK_R, OVF_XM + 2.0, OVF_XM + SOCK_L + 4.0))
    d = d.cut(sock.rotate((OVF_XM, OVF_YM, SOCK_Z), (OVF_XM, OVF_YM + 1.0, SOCK_Z), OVF_ANG))
    d = d.cut(zcyl(X_RISE, Y_RISE, 7.5, Z_DK0 - 0.01, Z_STAND + 0.01))                     # salita del tubo verso la tank
    for (x, y) in (CABLE_MOT, CABLE_HALL):                                                     # passacavi: motore, sensore Hall
        d = d.cut(zcyl(x, y, 6.0, Z_DK0 - 0.01, Z_STAND + 0.01))
    for x, ys in ((ST_X0 + 2.0 + SP + FL / 2, ST_SCREWS_Y), (ST_X1 - SP - FL / 2, ST_SCREWS_YR)):  # viti nei fianchi
        for y in ys:
            d = d.cut(zcyl(x, y, M3_CLR, Z_DK0 - 0.01, Z_STAND + 0.01))
    for y in COVER_TAB_Y:                                                                      # viti del carter della trasmissione
        d = d.cut(zcyl(COVER_TAB_X, y, M3_TAP, Z_DK0 - 0.01, Z_STAND + 0.01))
    return d

# ------------------------------------------------------------------ D02 / D03 fianchi del cavalletto
def stand_side(side=-1):
    """Fianco del cavalletto (side = -1 sinistro, +1 destro): piastra con flangia in alto (viti del piano) e in basso
    (viti nella base). Si stampa di piatto, flange in su."""
    if side < 0:
        x0 = ST_X0 + 2.0; xa, xb = x0 + SP, x0 + SP + FL
    else:
        x0 = ST_X1 - SP; xa, xb = x0 - FL, x0
    p = box(x0, x0 + SP, ST_Y0 + 4.0, ST_Y1, 0, Z_DK0)
    p = p.union(box(xa, xb, ST_Y0 + 4.0, ST_Y1, Z_DK0 - 4.0, Z_DK0)).union(box(xa, xb, ST_Y0 + 4.0, ST_Y1, 0, 4.0))
    p = p.union(box(xa, xb, ST_Y0 + 4.0, ST_Y0 + 8.0, 60.0, Z_DK0))                             # flangia per il frontale
    for y, yt in zip(ST_SCREWS_Y, ST_SCREWS_Y if side < 0 else ST_SCREWS_YR):
        p = p.cut(zcyl((xa + xb) / 2, yt, M3_TAP, Z_DK0 - 4.01, Z_DK0 + 0.01)).cut(zcyl((xa + xb) / 2, y + 12.0, 2.1, -0.01, 4.01))
    for z in (72.0, 100.0):
        p = p.cut(ycyl((xa + xb) / 2, z, M3_TAP, ST_Y0 + 3.99, ST_Y0 + 8.01))
    if side < 0:
        for y in COVER_TAB_Y:                                                                                  # le viti del carter proseguono nella flangia
            p = p.cut(zcyl(COVER_TAB_X, y, M3_TAP, Z_DK0 - 4.01, Z_DK0 + 0.01))
        p = p.cut(xcyl(40.0, 95.0, 11.0, x0 - 0.01, x0 + SP + 0.01))                                           # passacavi verso l'elettronica
        p = p.cut(box(x0 - 0.01, xb + 0.01, PUMP_PORT_Y - 12.0, PUMP_PORT_Y + 12.0, -0.01, PUMP_PORT_Z + 12.0)) # gomito e tubo collettore -> pompa
    else:
        p = p.cut(box(xa - 0.01, x0 + SP + 0.01, OVF_YM - 9.0, ST_Y1 + 0.01, Z_OVF_HOSE - 9.0, Z_OVF_HOSE + 9.0))   # tubo del troppopieno
        p = p.cut(box(xa - 0.01, xb - 3.0, Y_RISE - 12.0, Y_RISE + 12.0, Z_DK0 - 4.01, Z_DK0 + 0.01))           # tubo pompa -> tank
        p = p.cut(box(xa - 0.01, xb - 3.0, CABLE_MOT[1] - 8.0, CABLE_MOT[1] + 8.0, Z_DK0 - 4.01, Z_DK0 + 0.01))   # cavo del motore
    return p

# ------------------------------------------------------------------ D04 frontale del cavalletto
def stand_front():
    """Frontale del cavalletto: traversa che irrigidisce i due fianchi e nasconde la pompa. Quattro viti M3."""
    f = box(ST_X0 + 2.0, ST_X1, ST_Y0, ST_Y0 + 4.0, 60.0, Z_DK0)
    for x in (ST_X0 + 2.0 + SP + FL / 2, ST_X1 - SP - FL / 2):
        for z in (72.0, 100.0):
            f = f.cut(ycyl(x, z, M3_CLR, ST_Y0 - 0.01, ST_Y0 + 4.01))
    for k in range(9):                                                                         # feritoie di aerazione
        x = 232.0 + 17.0 * k
        f = f.cut(box(x, x + 6.0, ST_Y0 - 0.01, ST_Y0 + 4.01, 68.0, 104.0))
    return f

# ------------------------------------------------------------------ D05 carter della trasmissione
def drive_cover():
    """Carter della trasmissione: copre pulegge, cinghia e sensore Hall sul fianco sinistro della tank. Due viti nel piano."""
    c = box(CV_X0, CV_X1, CV_Y0, CV_Y1, Z_STAND, CV_Z1 + CV_T)
    c = c.cut(box(CV_X0 + CV_T, CV_X1 + 0.01, CV_Y0 + CV_T, CV_Y1 - CV_T, Z_STAND - 0.01, CV_Z1))
    for y in COVER_TAB_Y:                                                                      # alette con fazzoletto a 45 gradi (il carter si stampa capovolto)
        c = c.union(box(CV_X0 + CV_T, CV_X0 + CV_T + 14.0, y - 6.0, y + 6.0, Z_STAND, Z_STAND + 3.0))
        c = c.union(xz_prism([(CV_X0 + CV_T - 0.01, Z_STAND + 2.99), (CV_X0 + CV_T + 14.0, Z_STAND + 2.99), (CV_X0 + CV_T - 0.01, Z_STAND + 17.0)], y - 6.0, y + 6.0))
        c = c.cut(zcyl(COVER_TAB_X, y, M3_CLR, Z_STAND - 0.01, Z_STAND + 3.01)).cut(zcyl(COVER_TAB_X, y, 3.4, Z_STAND + 3.0, Z_STAND + 18.0))
    return c

# ================================================================== vano elettronica (fronte sinistra)
from mrefs import IEC_Y, IEC_Z, SPK, DISP_T
A = math.radians(SLOPE_ANG); SL = math.hypot(SLOPE_Y, EL_Z1 - SLOPE_Z0)       # lunghezza della plancia
EFL = 12.0                                    # larghezza delle flange dei fianchi
XF = (EL_T + EFL / 2, EL_X1 - EL_T - EFL / 2) # assi delle viti nelle flange dei due fianchi
EL_SCREW_Z, EL_SCREW_ZR, EL_SCREW_V = (30.0, 90.0, 150.0), (30.0, 110.0, 200.0), (18.0, SL - 18.0)
EL_SCREW_YT = (SLOPE_Y + 20.0, EL_Y1 - 20.0)
CABLE_YZ = (40.0, 95.0)                       # passacavi tra vano elettronica e cavalletto

def slope_pt(v, w):
    """(Y, Z) del punto a distanza v lungo la plancia e w in direzione perpendicolare (w < 0 = verso l'interno)."""
    return (v * math.cos(A) - w * math.sin(A), SLOPE_Z0 + v * math.sin(A) + w * math.cos(A))

def to_slope(p):
    """Dal riferimento della plancia (x = X, y = v lungo la pendenza, z = w perpendicolare) a quello della macchina."""
    return p.rotate((0, 0, 0), (1, 0, 0), SLOPE_ANG).translate((0, 0, SLOPE_Z0))

# ------------------------------------------------------------------ D06 / D07 fianchi del vano elettronica
def el_side(side=-1):
    """Fianco del vano elettronica (side = -1 sinistro: presa IEC e griglia dell'altoparlante; +1 destro: passacavi).
    Piastra con flange su tutti i bordi: frontale, plancia, tetto e paratia ci si avvitano sopra. Si stampa di piatto."""
    x0 = 0.0 if side < 0 else EL_X1 - EL_T
    xa, xb = (EL_T, EL_T + EFL) if side < 0 else (EL_X1 - EL_T - EFL, EL_X1 - EL_T)
    xm = (xa + xb) / 2
    p = yz_prism([(0, 0), (EL_Y1, 0), (EL_Y1, EL_Z1), (SLOPE_Y, EL_Z1), (0, SLOPE_Z0)], x0, x0 + EL_T)
    p = p.union(box(xa, xb, EL_T, EL_Y1 - EL_T, 0, 3.0))
    p = p.union(box(xa, xb, EL_T, EL_T + 3.0, 3.0, SLOPE_Z0 - 6.0))
    p = p.union(box(xa, xb, EL_Y1 - EL_T - 3.0, EL_Y1 - EL_T, 3.0, EL_Z1 - EL_T))
    p = p.union(box(xa, xb, SLOPE_Y + 6.0, EL_Y1 - EL_T - 3.0, EL_Z1 - EL_T - 3.0, EL_Z1 - EL_T))
    p = p.union(yz_prism([slope_pt(8.0, -3.0), slope_pt(SL - 8.0, -3.0), slope_pt(SL - 8.0, -6.0), slope_pt(8.0, -6.0)], xa, xb))
    for y in (20.0, 62.0, 104.0): p = p.cut(zcyl(xm, y, 2.1, -0.01, 3.01))
    for z in EL_SCREW_Z: p = p.cut(ycyl(xm, z, M3_TAP, EL_T - 0.01, EL_T + 3.01))
    for z in EL_SCREW_ZR: p = p.cut(ycyl(xm, z, M3_TAP, EL_Y1 - EL_T - 3.01, EL_Y1 - EL_T + 0.01))
    for y in EL_SCREW_YT: p = p.cut(zcyl(xm, y, M3_TAP, EL_Z1 - EL_T - 3.01, EL_Z1 - EL_T + 0.01))
    for v in EL_SCREW_V: p = p.cut(to_slope(zcyl(xm, v, M3_TAP, -6.01, -2.99)))
    if side < 0:
        p = p.cut(box(-0.01, EL_T + 0.01, IEC_Y[0], IEC_Y[1], IEC_Z[0], IEC_Z[1]))
        for k in range(7):
            a = math.radians(60.0 * k); r = 0.0 if k == 6 else 9.0
            p = p.cut(xcyl(SPK[1] + r * math.cos(a), SPK[2] + r * math.sin(a), 1.8, -0.01, EL_T + 0.01))
    else:
        p = p.cut(xcyl(CABLE_YZ[0], CABLE_YZ[1], 11.0, x0 - 0.01, x0 + EL_T + 0.01))
    return p

# ------------------------------------------------------------------ D08 frontale, D10 tetto, D11 paratia
def el_front():
    """Frontale del vano elettronica, con feritoie d'aria in basso."""
    p = box(EL_T, EL_X1 - EL_T, 0, EL_T, 0, SLOPE_Z0 - 3.0)
    for x in XF:
        for z in EL_SCREW_Z: p = p.cut(ycyl(x, z, M3_CLR, -0.01, EL_T + 0.01))
    for k in range(12):
        p = p.cut(box(28.0 + 12.0 * k, 32.0 + 12.0 * k, -0.01, EL_T + 0.01, 12.0, 48.0))
    return p

def el_top():
    """Tetto del vano elettronica, con feritoie sopra l'alimentatore."""
    p = box(EL_T, EL_X1 - EL_T, SLOPE_Y + 3.0, EL_Y1 - EL_T, EL_Z1 - EL_T, EL_Z1)
    for x in XF:
        for y in EL_SCREW_YT: p = p.cut(zcyl(x, y, M3_CLR, EL_Z1 - EL_T - 0.01, EL_Z1 + 0.01))
    for k in range(9):
        p = p.cut(box(70.0 + 12.0 * k, 74.0 + 12.0 * k, 76.0, 112.0, EL_Z1 - EL_T - 0.01, EL_Z1 + 0.01))
    return p

def el_rear():
    """Paratia posteriore: separa l'elettronica dal canale delle valvole; due passacavi Ø22 (con gommino)."""
    p = box(EL_T, EL_X1 - EL_T, EL_Y1 - EL_T, EL_Y1, 0, EL_Z1)
    for x in XF:
        for z in EL_SCREW_ZR: p = p.cut(ycyl(x, z, M3_CLR, EL_Y1 - EL_T - 0.01, EL_Y1 + 0.01))
    for z in (125.0, 160.0):
        p = p.cut(ycyl(34.0, z, 11.0, EL_Y1 - EL_T - 0.01, EL_Y1 + 0.01))
    return p

# ------------------------------------------------------------------ D09 plancia del display
UC, VC = EL_X1 / 2, SL / 2
def bezel():
    return to_slope(bezel_local())

def bezel_local():
    """Plancia inclinata a 45 gradi (nel suo riferimento: x = X, y lungo la pendenza, z perpendicolare, faccia esterna
    a z = 0): finestra 96 x 58 per il display da 4.3" (area attiva circa 93.6 x 56.2), cornice di centraggio della scheda sul retro e quattro colonnette
    alte quanto la scheda: su ognuna si avvita una linguetta girevole D17 che la trattiene. Si stampa con la faccia esterna sul piatto."""
    from mrefs import DISP_L, DISP_W
    b = box(EL_T + 0.3, EL_X1 - EL_T - 0.3, 0, SL, -3.0, 0)
    b = b.cut(box(UC - 48.0, UC + 48.0, VC - 29.0, VC + 29.0, -3.01, 0.01))
    hl, hw = DISP_L / 2 + 0.3, DISP_W / 2 + 0.3
    for (u0, u1, v0, v1) in ((UC - hl - 2, UC + hl + 2, VC - hw - 2, VC - hw), (UC - hl - 2, UC + hl + 2, VC + hw, VC + hw + 2),
                             (UC - hl - 2, UC - hl, VC - hw, VC + hw), (UC + hl, UC + hl + 2, VC - hw, VC + hw)):
        b = b.union(box(u0, u1, v0, v1, -5.0, -3.0))
    for su in (1, -1):
        for sv in (1, -1):
            u, v = UC + su * (hl + 6.5), VC + sv * 18.0
            b = b.union(zcyl(u, v, 4.0, -3.0 - DISP_T, -3.0)).cut(zcyl(u, v, M3_TAP, -3.01 - DISP_T, -0.8))
    for u in XF:
        for v in EL_SCREW_V: b = b.cut(zcyl(u, v, M3_CLR, -3.01, 0.01))
    return b

DISP_CENTER = (UC,) + slope_pt(VC, -3.0)

# ------------------------------------------------------------------ D17 linguetta della scheda display
TAB_L0, TAB_L1, TAB_HW, TAB_T = 4.0, 13.0, 4.0, 3.0
def disp_tab_local():
    """Linguetta girevole: un'estremita' sulla colonnetta della plancia (vite 3 x 8), l'altra sul bordo della scheda display.
    Si puo' ruotare per evitare connettori e componenti. Si stampa di piatto."""
    return box(-TAB_L0, TAB_L1, -TAB_HW, TAB_HW, 0, TAB_T).cut(zcyl(0, 0, M3_CLR, -0.01, TAB_T + 0.01))

def disp_tabs():
    """Le quattro linguette montate (riferimento macchina), rivolte verso il centro della scheda."""
    out = []
    hl = DISP_L / 2 + 0.3
    for su in (1, -1):
        for sv in (1, -1):
            t = disp_tab_local()
            if su > 0: t = t.rotate((0, 0, 0), (0, 0, 1), 180)
            out.append(to_slope(t.translate((UC + su * (hl + 6.5), VC + sv * 18.0, -3.0 - DISP_T - TAB_T))))
    return out

# ------------------------------------------------------------------ D12 ripiano a griglia, D13 / D14 colonnine, D15 morsetto
SH_X0, SH_X1, SH_Y0, SH_Y1, SH_T = 8.0, 188.0, 7.0, 65.0, 3.0
SH_Z = (10.0, 65.0, 120.0)
GRID_X = [13.0 + 10.0 * i for i in range(18)]; GRID_Y = [12.0 + 10.0 * j for j in range(6)]
COL_LOW = [(33.0, 12.0), (163.0, 12.0), (33.0, 62.0), (163.0, 62.0)]
COL_TALL = [(13.0, 12.0), (183.0, 12.0), (13.0, 62.0), (183.0, 62.0)]
def shelf(i=0):
    """Ripiano a griglia (fori Ø3.4 a passo 10; Ø3.8 nei quattro punti delle colonnine basse, per le viti da legno):
    le schede si fissano con i morsetti D15, i cavi con fascette. I tre ripiani sono uguali."""
    s = box(SH_X0, SH_X1, SH_Y0, SH_Y1, SH_Z[i], SH_Z[i] + SH_T)
    holes = None
    for x in GRID_X:
        for y in GRID_Y:
            h = zcyl(x, y, 1.9 if (x, y) in COL_LOW else M3_CLR, SH_Z[i] - 0.01, SH_Z[i] + SH_T + 0.01)
            holes = h if holes is None else holes.union(h)
    return s.cut(holes)

def column(x, y, z0, z1):
    """Colonnina distanziale: bassa (vite da legno nella base) o alta (barra filettata M3 passante)."""
    return zcyl(x, y, 4.5, z0, z1).cut(zcyl(x, y, 1.7 if z1 - z0 > 20 else 1.9, z0 - 0.01, z1 + 0.01))

CL_L, CL_H, PCB_Z = 13.0, 8.7, 5.0
def clamp(x, y, z, side=1):
    """Morsetto per schede: appoggia il circuito a 5 mm dal ripiano e lo trattiene sul bordo; asola per la vite M3.
    (x, y, z) = punto del bordo della scheda sul piano del ripiano; side = +1 se il morsetto sta a destra della scheda."""
    c = box(0, CL_L, -6.0, 6.0, 0, CL_H).union(box(-2.5, 0, -6.0, 6.0, 0, PCB_Z)).union(box(-2.5, 0, -6.0, 6.0, PCB_Z + 1.7, CL_H))
    c = c.cut(box(3.0, 10.0, -1.7, 1.7, -0.01, CL_H + 0.01)).cut(zcyl(3.0, 0, 1.7, -0.01, CL_H + 0.01)).cut(zcyl(10.0, 0, 1.7, -0.01, CL_H + 0.01))
    if side < 0: c = c.mirror("YZ")
    return c.translate((x, y, z))

# schede: (nome, ripiano, x0, y0, lunghezza X, larghezza Y, altezza dei componenti)
BOARDS = [("REF_ponte_H_doppio_30A", 0, 30.0, 8.0, 69.0, 55.0, 44.0), ("REF_modulo_MOSFET_IRF540", 0, 113.0, 8.0, 60.0, 48.0, 22.0),
          ("REF_driver_elettrovalvole_MCP23017", 1, 21.0, 9.0, 72.0, 52.0, 16.0), ("REF_breakout_DC3-26P", 1, 107.0, 9.0, 68.0, 45.0, 20.0),
          ("REF_convertitore_LM2596S", 2, 60.0, 20.0, 43.0, 21.0, 14.0)]

# ------------------------------------------------------------------ D16 zoccolo dell'alimentatore
def psu_socket():
    """Zoccolo dell'alimentatore: l'alimentatore ci entra in piedi, contro la paratia; fondo aperto per l'aria."""
    x0, x1, y0, y1 = PSU_X0 - 2.5, PSU_X0 + PSU_W + 2.5, PSU_Y0 - 2.5, EL_Y1 - EL_T - 0.3
    s = box(x0, x1, y0, y1, 0, 34.0)
    s = s.cut(box(PSU_X0 - 0.3, PSU_X0 + PSU_W + 0.3, PSU_Y0 - 0.3, y1 + 0.01, PSU_Z0, 34.01))
    s = s.cut(box(PSU_X0 + 10.0, PSU_X0 + PSU_W - 10.0, PSU_Y0 + 8.0, y1 - 8.0, -0.01, PSU_Z0 + 0.01))
    for (xa, xb, ya, yb) in ((x0 - 12.0, x0, 80.0, 92.0), (x0 - 12.0, x0, 100.0, 112.0), (70.0, 82.0, y0 - 10.0, y0), (120.0, 132.0, y0 - 10.0, y0)):
        s = s.union(box(xa, xb, ya, yb, 0, 4.0)).cut(zcyl((xa + xb) / 2, (ya + yb) / 2, 2.1, -0.01, 4.01))
    return s

PARTS = [("D01_piano_cavalletto", stand_deck), ("D02_fianco_cavalletto_SX", lambda: stand_side(-1)), ("D03_fianco_cavalletto_DX", lambda: stand_side(1)),
         ("D04_frontale_cavalletto", stand_front), ("D05_carter_trasmissione", drive_cover),
         ("D06_fianco_elettronica_SX", lambda: el_side(-1)), ("D07_fianco_elettronica_DX", lambda: el_side(1)), ("D08_frontale_elettronica", el_front),
         ("D09_plancia_display", bezel_local), ("D10_tetto_elettronica", el_top), ("D11_paratia_posteriore", el_rear), ("D12_ripiano_griglia", shelf),
         ("D13_colonnina_bassa", lambda: column(0, 0, 0, SH_Z[0])), ("D14_colonnina_alta", lambda: column(0, 0, 0, SH_Z[1] - SH_Z[0] - SH_T)),
         ("D15_morsetto_scheda", lambda: clamp(0, 0, 0)), ("D16_zoccolo_alimentatore", psu_socket), ("D17_linguetta_display", disp_tab_local)]

if __name__ == "__main__":
    print("viti piedini tank:", [(round(x, 1), round(y, 1)) for x, y in FEET_M], "| carter X", CV_X0, CV_X1, "Z fino a", CV_Z1 + CV_T)
    for n, f in PARTS:
        s = f().val(); bb = s.BoundingBox()
        print(f"{n:26s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:6.1f} cm3  {bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f}")
