"""Parti stampate della struttura (serie D), nel riferimento macchina - rilascio 6.
Cavalletto della tank: D01 piano, D02/D03 fianchi, D04 frontale, D05 carter della trasmissione.
Vano elettronica: D11 squadretta, D12 ripiano a griglia, D13/D14 colonnine, D15 morsetto per schede, D16 zoccolo
dell'alimentatore, D17 staffa alta dell'alimentatore, D19 supporto dell'altoparlante.
(La scocca - montanti, appoggi del piano, cruscotto del display - sta in shell.py.)"""
import math
import cadquery as cq
from machine import *
import tank as T
from mrefs import X_RISE, Y_RISE, OVF_XM, OVF_YM, Z_OVF_HOSE, SOCK_Z, OVF_ANG, OVF_YAW, PSU_X0, PSU_Y0, PSU_Z0, PSU_W, PSU_H, PSU_L, DISP_L, DISP_W
import mrefs as R

Z_DK0 = Z_STAND - DECK_T                       # faccia inferiore del piano del cavalletto
FEET_M = [tank_xy(x, y) for (x, y) in T.FEET]  # viti dei piedini della tank
SP = 4.0                                       # spessore dei fianchi del cavalletto
FL = 14.0                                      # larghezza delle flange
ST_SCREWS_Y = (30.0, 100.0, 175.0)             # viti piano-fianco sinistro; sul destro le stesse ordinate + 12 per le viti nella base
ST_SCREWS_YR = (20.0, 60.0, 175.0)             # viti piano-fianco destro (lontane dai passaggi del tubo e del cavo del motore)
CABLE_MOT = (388.0, 36.0)                      # passacavo del motore nel piano (accanto alla bocca della galleria)
CABLE_HALL = (226.0, 100.0)                    # passacavo del sensore Hall nel piano (sotto il carter, oltre la flangia del fianco)

SOCK_R, SOCK_L = 5.85, 12.0                    # innesto del troppopieno: sede Ø11.7 per il tubo in silicone da 12 mm (0.3 di interferenza)
TIE_SCREWS = [(x + sg * 14.0, y) for (x, y) in ((R.FS_X1 - 11.0, R.IN_Y0), (R.IV_X0 + VALVE_LEN / 2, R.IN_Y1)) for sg in (1, -1)]
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
    sock = sock.rotate((OVF_XM, OVF_YM, SOCK_Z), (OVF_XM, OVF_YM + 1.0, SOCK_Z), OVF_ANG).rotate((OVF_XM, OVF_YM, SOCK_Z), (OVF_XM, OVF_YM, SOCK_Z + 1.0), OVF_YAW)
    d = d.cut(sock)
    d = d.cut(zcyl(R.E3[0], R.E3[1], 9.5, Z_DK0 - 0.01, Z_STAND + 0.01))                    # salita dell'acqua di rete verso il bagno
    for (x, y) in TIE_SCREWS:                                                                 # staffe a fascetta C18 (sotto)
        d = d.cut(zcyl(x, y, M3_TAP, Z_DK0 - 0.01, Z_DK0 + 4.6))
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
        zh = SOCK_Z - (x0 + SP / 2 - OVF_XM) * math.tan(math.radians(OVF_ANG))
        p = p.cut(box(xa - 0.01, x0 + SP + 0.01, OVF_YM - 2.0, ST_Y1 + 0.01, zh - 9.0, zh + 9.0))            # tubo del troppopieno della tank
        p = p.cut(xcyl(PUMP_PORT_Y, PUMP_PORT_Z, 9.5, xa - 0.01, x0 + SP + 0.01))                              # gomito pompa -> tank
        p = p.cut(xcyl(R.CONN["INGRESSO"][0], R.CONN["INGRESSO"][1], 9.0, xa - 0.01, x0 + SP + 0.01))         # passaparete dell'ingresso
        p = p.cut(box(xa - 0.01, xb + 0.01, R.E3[1] - 10.0, R.E3[1] + 10.0, Z_DK0 - 4.01, Z_DK0 + 0.01))       # salita dell'acqua di rete
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

# ================================================================== vano elettronica (fronte sinistra) - v6
# pannelli in Dibond: paratia Y 122..125 (tra elettronica e valvole) e divisorio X 193..196 (verso il cavalletto)
PAR_Y0, PAR_Y1 = 122.0, 125.0
DIV_X0, DIV_X1 = 193.0, 196.0

GUIDE_W = 1.5                               # pareti del binario
def panel_guide(paratia=True):
    """D11: binario a U avvitato al multistrato (viti da legno 3,5 x 12): ci si infila il bordo inferiore della paratia
    (paratia=True) o del divisorio. In alto i pannelli sono tenuti dal piano, all'angolo si appoggiano l'uno all'altro."""
    if paratia:
        x0, x1 = 3.0, DIV_X0 - 0.15 - GUIDE_W - 0.5
        g = box(x0, x1, PAR_Y0 - 0.15 - GUIDE_W, PAR_Y1 + 0.15 + GUIDE_W, 0, 10.0).cut(box(x0 - 0.01, x1 + 0.01, PAR_Y0 - 0.15, PAR_Y1 + 0.15, 2.0, 10.01))
        for x in (x0 + 12.0, (x0 + x1) / 2, x1 - 12.0):
            g = g.union(box(x - 5.0, x + 5.0, PAR_Y1 + 0.15 + GUIDE_W - 0.01, PAR_Y1 + 0.15 + GUIDE_W + 9.0, 0, 3.0))
            g = g.cut(zcyl(x, PAR_Y1 + 0.15 + GUIDE_W + 4.5, 1.9, -0.01, 3.01))
        return g
    y0, y1 = 3.0, PAR_Y0 - 0.15 - GUIDE_W - 0.5
    g = box(DIV_X0 - 0.15 - GUIDE_W, DIV_X1 + 0.15 + GUIDE_W, y0, y1, 0, 10.0).cut(box(DIV_X0 - 0.15, DIV_X1 + 0.15, y0 - 0.01, y1 + 0.01, 2.0, 10.01))
    for y in (35.0, 81.0):
        xe = DIV_X0 - 0.15 - GUIDE_W
        g = g.union(box(xe - 9.0, xe + 0.01, y - 4.0, y + 4.0, 0, 3.0))
        g = g.cut(zcyl(xe - 4.5, y, 1.9, -0.01, 3.01))
    return g

# ------------------------------------------------------------------ D12 ripiano a griglia, D13 / D14 colonnine, D15 morsetto
SH_X0, SH_X1, SH_Y0, SH_Y1, SH_T = 20.0, 190.0, 4.0, 80.0, 3.0
SH_Z = (8.0, 66.0, 124.0)
GRID_X = [25.0 + 10.0 * i for i in range(17)]; GRID_Y = [9.0 + 10.0 * j for j in range(7)]
COL = [(25.0, 9.0), (185.0, 9.0), (25.0, 69.0), (185.0, 69.0)]
def shelf(i=0):
    """Ripiano a griglia 170 x 76 (fori Ø3.4 a passo 10): le schede si fissano con i morsetti D15, i cavi con fascette.
    I tre ripiani sono uguali e stanno su quattro barre filettate M3 con colonnine distanziali."""
    s = box(SH_X0, SH_X1, SH_Y0, SH_Y1, SH_Z[i], SH_Z[i] + SH_T)
    holes = None
    for x in GRID_X:
        for y in GRID_Y:
            h = zcyl(x, y, M3_CLR, SH_Z[i] - 0.01, SH_Z[i] + SH_T + 0.01)
            holes = h if holes is None else holes.union(h)
    return s.cut(holes)

def column(x, y, z0, z1):
    """Colonnina distanziale sulla barra filettata M3."""
    return zcyl(x, y, 4.5, z0, z1).cut(zcyl(x, y, 1.7, z0 - 0.01, z1 + 0.01))

def col_foot(x, y):
    """D13: piede della colonna: si avvita al multistrato (vite da legno 3,5 x 12: aletta da 3 mm, base da 12) e tiene la barra filettata (dado incassato)."""
    f = zcyl(x, y, 6.0, 0, SH_Z[0]).union(box(x - 6.0, x + 6.0, y - 6.0, y + 6.0, 0, 3.0))
    f = f.cut(zcyl(x, y, 1.7, 3.0, SH_Z[0] + 0.01)).cut(cq.Workplane("XY").polygon(6, 6.4).extrude(3.0).translate((x, y, SH_Z[0] - 3.0)))
    return f.cut(zcyl(x, y, 1.9, -0.01, 3.01))

CL_L, CL_H, PCB_Z = 13.0, 8.7, 5.0
def clamp(x, y, z, side=1):
    """Morsetto per schede: appoggia il circuito a 5 mm dal ripiano e lo trattiene sul bordo; asola per la vite M3.
    (x, y, z) = punto del bordo della scheda sul piano del ripiano; side = +1 se il morsetto sta a destra della scheda."""
    c = box(0, CL_L, -6.0, 6.0, 0, CL_H).union(box(-2.5, 0, -6.0, 6.0, 0, PCB_Z)).union(box(-2.5, 0, -6.0, 6.0, PCB_Z + 1.7, CL_H))
    c = c.cut(box(3.0, 10.0, -1.7, 1.7, -0.01, CL_H + 0.01)).cut(zcyl(3.0, 0, 1.7, -0.01, CL_H + 0.01)).cut(zcyl(10.0, 0, 1.7, -0.01, CL_H + 0.01))
    if side < 0: c = c.mirror("YZ")
    return c.translate((x, y, z))

# schede (misure del prototipo): (nome, ripiano, x0, y0, lunghezza X, larghezza Y, altezza dei componenti)
BOARDS = [("REF_modulo_MOSFET_IRF540", 0, 36.0, 6.0, 70.0, 70.0, 20.0), ("REF_convertitore_LM2596S", 0, 133.0, 6.0, 35.0, 70.0, 20.0),
          ("REF_ponte_H_doppio_30A", 1, 36.0, 10.0, 70.0, 60.0, 30.0),
          ("REF_breakout_DC3-26P", 2, 36.0, 6.0, 85.0, 70.0, 30.0), ("REF_driver_MCP23017", 2, 146.0, 16.0, 30.0, 50.0, 20.0)]   # breakout in alto: il cavo flat da 20 cm arriva al display

# ------------------------------------------------------------------ D16 zoccolo e D17 staffa dell'alimentatore, D19 altoparlante
def psu_socket():
    """D16: zoccolo dell'alimentatore LRS-350-12: lo tiene in piedi (morsetti in basso, verso il fronte), sollevato di 10 mm
    perche' l'aria entri da sotto; fondo aperto. Due alette per le viti da legno 3,5 x 12."""
    x0, x1, y0, y1 = PSU_X0 - 3.0, PSU_X0 + PSU_W + 3.0, PSU_Y0 - 3.0, PAR_Y0 - 0.15 - GUIDE_W - 0.3
    s = box(x0, x1, y0, y1, 0, PSU_Z0 + 14.0)
    s = s.cut(box(PSU_X0 - 0.3, PSU_X0 + PSU_W + 0.3, PSU_Y0 - 0.3, PSU_Y0 + PSU_H + 0.3, PSU_Z0, PSU_Z0 + 14.01))
    s = s.cut(box(PSU_X0 + 8.0, PSU_X0 + PSU_W - 8.0, PSU_Y0 + 5.0, PSU_Y0 + PSU_H - 5.0, -0.01, PSU_Z0 + 0.01))
    s = s.cut(box(PSU_X0 + 15.0, PSU_X0 + PSU_W - 15.0, y0 - 0.01, PSU_Y0 + 0.01, 3.0, PSU_Z0 + 14.01))     # morsetti e cavi
    for (xa, xb) in ((x0 - 12.0, x0), (x1, x1 + 12.0)):
        s = s.union(box(xa, xb, y0, y1, 0, 4.0)).cut(zcyl((xa + xb) / 2, (y0 + y1) / 2, 1.9, -0.01, 4.01))
    return s

def psu_strap():
    """D17: staffa alta dell'alimentatore: abbraccia la testa dell'alimentatore e si avvita alla paratia (due viti M3 x 10 con dado)."""
    z1 = PSU_Z0 + PSU_L; z0 = z1 - 20.0
    x0, x1 = PSU_X0 - 3.0, PSU_X0 + PSU_W + 3.0
    s = box(x0, x1, PSU_Y0 - 3.0, PAR_Y0 - 0.3, z0, z1 + 3.0)
    s = s.cut(box(PSU_X0 - 0.3, PSU_X0 + PSU_W + 0.3, PSU_Y0 - 0.3, PSU_Y0 + PSU_H + 0.3, z0 - 0.01, z1 + 0.01))
    s = s.cut(box(PSU_X0 + 10.0, PSU_X0 + PSU_W - 10.0, PSU_Y0 - 3.01, PSU_Y0 + PSU_H - 3.0, z1 - 0.01, z1 + 3.01))   # aria
    for (xa, xb) in ((x0 - 12.0, x0), (x1, x1 + 12.0)):
        s = s.union(box(xa, xb, PAR_Y0 - 4.0, PAR_Y0 - 0.3, z0, z1 + 3.0)).cut(ycyl((xa + xb) / 2, z0 + 11.0, M3_CLR, PAR_Y0 - 4.01, PAR_Y0))
    return s

def speaker_holder():
    """D19: supporto dell'altoparlante Ø40 dietro la foratura del pannello sinistro (incollato al pannello con biadesivo)."""
    x, y, z = R.SPK
    h = xcyl(y, z, 24.0, 0.0, 8.0).cut(xcyl(y, z, 20.3, 0.0 - 0.01, 5.3)).cut(xcyl(y, z, 15.0, 5.0, 8.01))
    return h

PARTS = [("D01_piano_cavalletto", stand_deck), ("D02_fianco_cavalletto_SX", lambda: stand_side(-1)), ("D03_fianco_cavalletto_DX", lambda: stand_side(1)),
         ("D04_frontale_cavalletto", stand_front), ("D05_carter_trasmissione", drive_cover),
         ("D11_guida_paratia", lambda: panel_guide(True)), ("D11_guida_divisorio", lambda: panel_guide(False)), ("D12_ripiano_griglia", shelf), ("D13_piede_colonna", lambda: col_foot(0, 0)),
         ("D14_colonnina", lambda: column(0, 0, 0, SH_Z[1] - SH_Z[0] - SH_T)), ("D15_morsetto_scheda", lambda: clamp(0, 0, 0)),
         ("D16_zoccolo_alimentatore", psu_socket), ("D17_staffa_alimentatore", psu_strap), ("D19_supporto_altoparlante", speaker_holder)]

if __name__ == "__main__":
    for n, f in PARTS:
        s = f().val(); bb = s.BoundingBox()
        print(f"{n:26s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:6.1f} cm3  {bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f}")
