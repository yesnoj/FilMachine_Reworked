"""Sagome dei componenti acquistati (REF_) nel riferimento macchina, rilascio 7: base e piano, pannelli del bagno in PMMA,
contenitori, passaparete, riscaldatori, sensori, sonde, pompetta di ricircolo, Kamoer, valvole, alimentatore, mainboard,
display, presa IEC, rubinetti, tubi e raccordi delle linee. Servono per gli assiemi e per la distinta."""
import math
import cadquery as cq
from machine import *
import tank as T, drive as D

# ---- base e piano (multistrato 12 mm) ----
def base_board():
    return box(0, W, 0, D_, -BASE_T, 0)

NOTCH = 16.0                         # angoli del piano smussati: ci passano i montanti
def deck():
    """Piano del piedistallo: multistrato 12 mm con i fori dei passaparete e dei cavi; angoli tagliati per i montanti."""
    p = box(0, W, 0, D_, DECK_Z0, PZ)
    for (x, y) in ((0, 0), (W, 0), (W, D_), (0, D_)):
        p = p.cut(box(x - NOTCH, x + NOTCH, y - NOTCH, y + NOTCH, DECK_Z0 - 0.01, PZ + 0.01))
    for n, (x, y, r) in DECK_CUTS.items():
        p = p.cut(zcyl(x, y, DECK_GROMMET_R if n == "troppopieno_tank" else r, DECK_Z0 - 0.01, PZ + 0.01))
    return p

# ---- bagno: pannelli in PMMA 4 mm (fondo, fronte, retro, destra, sinistra con la finestra) e coperchio 3 mm ----
WIN = (Y_T + XO0 + D.FRAME_W, Y_T + XO1 - D.FRAME_W, Z_T0 + Z_BOT - 4.0 + D.FRAME_W, Z_T0 + T.WIN_TOP - D.FRAME_W)   # apertura nella parete sinistra (y0, y1, z0, z1)
def bath_floor():
    """Fondo del bagno: copre anche il vano asciutto (ci appoggiano la staffa del motore e la parete sinistra)."""
    p = box(NOTCH, BX1 + BATH_T, BY0 - BATH_T, BY1 + BATH_T, PZ, FLOOR)
    for n in BATH_CUTS:
        x, y, r = DECK_CUTS[n]
        p = p.cut(zcyl(x, y, r, PZ - 0.01, FLOOR + 0.01))
    for (x, y) in [tank_xy(*f) for f in T.FEET] + [tank_xy(*f) for f in T.FEET_X]:                 # viti dei piedini della tank (M3, dado sotto)
        p = p.cut(zcyl(x, y, M3_CLR, PZ - 0.01, FLOOR + 0.01))
    return p

def bath_front():
    return box(BX0 - BATH_T, BX1 + BATH_T, BY0 - BATH_T, BY0, FLOOR, BZ1)

def bath_back():
    """Parete posteriore, liscia (i cavi di pompetta e sonda scavalcano il bordo, nella tacca del coperchio)."""
    return box(BX0 - BATH_T, BX1 + BATH_T, BY1, BY1 + BATH_T, FLOOR, BZ1)

def bath_right():
    """Parete destra: i due riscaldatori a cartuccia (filetto con dado e guarnizione) la attraversano; fuori i due XKC-Y21 del bagno."""
    p = box(BX1, BX1 + BATH_T, BY0, BY1, FLOOR, BZ1)
    for y in HEAT_Y:
        p = p.cut(xcyl(y, HEAT_Z, 8.3, BX1 - 0.01, BX1 + BATH_T + 0.01))
    return p

def bath_left():
    """Parete sinistra, piu' bassa (sotto la gonna del coperchio della tank), con la finestra della testata e i fori delle viti della cornice."""
    p = box(BX0 - BATH_T, BX0, BY0, BY1, FLOOR, WIN_TOP_Z)
    p = p.cut(box(BX0 - BATH_T - 0.01, BX0 + 0.01, WIN[0], WIN[1], WIN[2], WIN[3]))
    for (xt, zt) in T.HEAD_SCREWS:
        x, y = tank_xy(xt, YO1)
        p = p.cut(xcyl(y, Z_T0 + zt, M3_CLR, BX0 - BATH_T - 0.01, BX0 + 0.01))
    return p

LID_OUT = (Y_T + XO0 - T.LID_GAP - 2.5, Y_T + XO1 + T.LID_GAP + 2.5)   # ingombro Y del coperchio della tank
NOTCH_CAB = (PROBE_XY[0] - 6.0, CIRC_XY[0] + 6.0)                     # tacca nel bordo posteriore del coperchio per i cavi di sonda e pompetta
def bath_cover():
    """Coperchio del bagno in PMMA 3 mm: aperto sulla tank (fino al bordo sinistro) e sui tre tappi dei contenitori; tacca per i cavi."""
    p = box(BX0 - BATH_T, BX1 + BATH_T, BY0 - BATH_T, BY1 + BATH_T, BZ1, BZ1 + COVER_T)
    p = p.cut(box(BX0 - BATH_T - 0.01, X_T - YO0 + T.LID_GAP + 2.5 + 1.0, LID_OUT[0] - 1.0, LID_OUT[1] + 1.0, BZ1 - 0.01, BZ1 + COVER_T + 0.01))
    for cx in CX:
        p = p.cut(box(cx - CW / 2 - 2.0, cx + CW / 2 + 2.0, CY - CD / 2 - 2.0, CY + CD / 2 + 2.0, BZ1 - 0.01, BZ1 + COVER_T + 0.01))
    p = p.cut(box(NOTCH_CAB[0], NOTCH_CAB[1], BY1 - 6.0, BY1 + BATH_T + 0.01, BZ1 - 0.01, BZ1 + COVER_T + 0.01))
    for x in (PROBE_XY[0], IN_X, OVF_X):                                                                                  # portasonda e staffe dei tubi verticali
        p = p.cut(box(x - 9.0, x + 9.0, BY1 - 3.0, BY1 + BATH_T + 0.01, BZ1 - 0.01, BZ1 + COVER_T + 0.01))
    for i in range(5):
        p = p.cut(zcyl(CX[0] + 40.0 * i, BY1 - 14.0, 3.0, BZ1 - 0.01, BZ1 + COVER_T + 0.01))   # sfiato dell'aria umida
    return p

BATH_PANELS = [("REF_bagno_fondo_PMMA4", bath_floor), ("REF_bagno_fronte_PMMA4", bath_front), ("REF_bagno_retro_PMMA4", bath_back),
               ("REF_bagno_destra_PMMA4", bath_right), ("REF_bagno_sinistra_PMMA4", bath_left), ("REF_bagno_coperchio_PMMA3", bath_cover)]

def water():
    w = box(BX0, BX1, BY0, BY1, FLOOR, WATER_Z)
    w = w.cut(box(X_T - YO1 - 1, X_T - T.PIL_Y0 + 1, Y_T + XO0 - 1, Y_T + XO1 + 14, FLOOR - 1, WATER_Z + 1))
    for cx in CX:
        w = w.cut(box(cx - CW / 2, cx + CW / 2, CY - CD / 2, CY + CD / 2, FLOOR - 1, WATER_Z + 1))
    return w

def water_volume_l():
    return water().val().Volume() / 1e6

# ---- contenitori da 500 ml, tappi, passaparete, sensori MAX ----
def container(k):
    cx = CX[k]
    c = box(cx - CW / 2, cx + CW / 2, CY - CD / 2, CY + CD / 2, FLOOR, FLOOR + CH)
    c = c.cut(box(cx - CW / 2 + 1.5, cx + CW / 2 - 1.5, CY - CD / 2 + 1.5, CY + CD / 2 - 1.5, FLOOR + 1.5, FLOOR + CH + 1))
    return c.cut(zcyl(cx, CY, BULK_R, FLOOR - 0.01, FLOOR + 1.51))

def container_lid(k):
    cx = CX[k]
    return box(cx - CW / 2 - 1.5, cx + CW / 2 + 1.5, CY - CD / 2 - 1.5, CY + CD / 2 + 1.5, FLOOR + CH - 5.0, FLOOR + CH + 7.0) \
        .cut(box(cx - CW / 2 - 0.3, cx + CW / 2 + 0.3, CY - CD / 2 - 0.3, CY + CD / 2 + 0.3, FLOOR + CH - 5.01, FLOOR + CH + 0.01))

def chemical(k, ml=350.0):
    cx = CX[k]; h = ml * 1000.0 / ((CW - 3.0) * (CD - 3.0))
    return box(cx - CW / 2 + 1.5, cx + CW / 2 - 1.5, CY - CD / 2 + 1.5, CY + CD / 2 - 1.5, FLOOR + 1.5, FLOOR + 1.5 + h)

def bulkhead_floor(x, y, top="flat", riser_z=None):
    """Passaparete 3/8" BSP nel fondo del bagno (corpo Ø17, flangia Ø26 con guarnizione sopra, dado sotto il piano, raccordo a
    innesto 3/8" in basso). top: 'flat' (bocca a filo: contenitori, pescante WB), 'barb' (portagomma per il silicone 8 x 12:
    linea tank), 'riser' (tubo 3/8" verticale a innesto fino a riser_z: troppopieno del bagno e beccuccio dell'ingresso)."""
    zf = FLOOR + (1.5 if top == "flat" and y == CY else 0.0)                                  # sotto un contenitore attraversa anche il suo fondo
    b = zcyl(x, y, BULK_R - 0.1, DECK_Z0 - 8.0, zf + 2.0 + 10.0)
    b = b.union(zcyl(x, y, 13.0, zf + 2.0, zf + 6.0))                                           # flangia + guarnizione interna
    b = b.union(cq.Workplane("XY").polygon(6, 27.0).extrude(7.0).translate((x, y, DECK_Z0 - 7.5)))   # dado sotto il piano
    b = b.union(zcyl(x, y, 8.0, BULK_Z0, DECK_Z0 - 8.0))                                       # raccordo a innesto 3/8" sotto
    if top == "barb":
        b = b.union(zcyl(x, y, 4.2, zf + 12.0, zf + 30.0)).union(zcyl(x, y, 5.0, zf + 26.0, zf + 29.0))
    elif top == "riser":
        b = b.union(zcyl(x, y, 8.0, zf + 12.0, zf + 24.0)).union(zcyl(x, y, PIPE_R, zf + 24.0, riser_z))   # innesto in alto + tubo verticale
    return b

def container_bulkhead(k):
    return bulkhead_floor(CX[k], CY)

def chem_max_sensor(k):
    """XKC-Y21 MAX della chimica k, incollato (biadesivo) sulla parete posteriore del contenitore, sopra l'acqua del bagno."""
    y0 = CY + CD / 2
    return box(CX[k] - 14.0, CX[k] + 14.0, y0, y0 + 6.0, CMAX_Z - 8.0, CMAX_Z + 8.0)

# ---- bagno: riscaldatori, sensori, sonda, pompetta, passaparete di servizio nel fondo ----
def heater(i):
    """Cartuccia 12 V 100 W Ø16 x 100 con filetto e dado: entra dalla parete destra, testa e cavo nell'intercapedine destra."""
    y = HEAT_Y[i]; xo = BX1 + BATH_T
    h = xcyl(y, HEAT_Z, 8.0, HEAT_X[0], xo + 3.0)
    h = h.union(xcyl(y, HEAT_Z, 12.5, BX1 - 6.0, BX1 - 0.5))                                     # dado + guarnizione interni
    h = h.union(cq.Workplane("YZ").center(y, HEAT_Z).polygon(6, 25.4).extrude(9.0).translate((xo + 0.5, 0, 0)))
    return h.union(xcyl(y, HEAT_Z, 3.5, xo + 9.5, xo + 20.0))                                     # uscita dei cavi

def level_sensor(z):
    """XKC-Y21 MIN/MAX del bagno, incollato all'esterno della parete posteriore (nell'intercapedine)."""
    yo = BY1 + BATH_T
    return box(LVL_X - 14.0, LVL_X + 14.0, yo, yo + 6.0, z - 8.0, z + 8.0)

def probe_bath():
    """Sonda DS18B20 inox Ø6 x 30 nel bagno, appesa al suo cavo nel portasonda sulla parete posteriore."""
    x, y = PROBE_XY
    return zcyl(x, y, 3.0, FLOOR + 12.0, FLOOR + 42.0).union(zcyl(x, y, 1.5, FLOOR + 42.0, BZ1 - 8.0))

def circulator():
    """Pompetta di ricircolo sommersa 12 V (corpo 30 x 24 x 42 con ventose, bocca Ø8 verso +X, cavo in alto: sale, va alla
    parete posteriore ed esce dalla tacca del coperchio)."""
    x, y = CIRC_XY
    c = box(x - 15.0, x + 15.0, y - 12.0, y + 12.0, FLOOR + 3.0, FLOOR + 45.0)
    c = c.union(xcyl(y, FLOOR + 20.0, 4.0, x + 15.0, x + 27.0)).union(zcyl(x, y, 1.5, FLOOR + 45.0, BZ1 - 6.0))
    return c.union(ycyl(x, BZ1 - 6.0, 1.5, y, BY1 - 3.0)).union(zcyl(x, BY1 - 3.0, 1.5, BZ1 - 6.0, BZ1 + 2.0))

def wb_bulkhead():
    return bulkhead_floor(WB_X, WB_Y, "flat")

def overflow_bulkhead():
    return bulkhead_floor(OVF_X, SRV_Y, "riser", OVF_Z)

def inlet_bulkhead():
    """Ingresso dell'acqua: passaparete nel fondo con tubo verticale fino sopra l'acqua (salto d'aria)."""
    return bulkhead_floor(IN_X, IN_Y, "riser", IN_Z)

def tank_overflow_grommet():
    """Gommino passatubo (per lamiera da 4 mm, foro Ø13, tubo Ø12) nel fondo del bagno: il silicone 8 x 12 del troppopieno della tank
    ci passa a tenuta. Fa tenuta solo sul PMMA: nel piano sotto il foro e' piu' largo (Ø20) e ospita il labbro inferiore."""
    x, y = TANK_OVF
    g = zcyl(x, y, 9.0, PZ - 1.5, PZ).union(zcyl(x, y, 6.4, PZ - 0.01, FLOOR + 0.01)).union(zcyl(x, y, 9.0, FLOOR, FLOOR + 1.5))
    return g.cut(zcyl(x, y, 5.8, PZ - 1.51, FLOOR + 1.51))

# ---- Kamoer, valvole, collettore, alimentatore, mainboard, display, presa, rubinetti ----
def pump():
    """Pompa peristaltica Kamoer KPHM600 12 V (corpo 90 x 60 x 56, testa Ø52 verso il fronte, due portagomma 6,4 verso -Y)."""
    p = box(PUMP_X0, PUMP_X0 + PUMP_L, PUMP_Y0, PUMP_Y0 + PUMP_W, PUMP_Z0, PUMP_Z0 + PUMP_HH)
    p = p.union(ycyl(PUMP_X0 + 45.0, PUMP_Z0 + 34.0, 26.0, PUMP_Y0 - 14.0, PUMP_Y0))
    for (x, y, z) in PUMP_PORTS:
        p = p.union(ycyl(x, z, 4.5, y, PUMP_Y0 - 14.0))
    return p

def valve(k, x=None, y=None, z0=None, axis="Y", coil=(0, 0, 1)):
    """Elettrovalvola 12 V NC con innesti rapidi 3/8" (82 mm tra le bocche, corpo 33, bobina di lato).
    Di serie sdraiata lungo Y sul collettore (bocca posteriore nel collettore), bobina in alto."""
    h = VALVE_BODY / 2
    if axis == "Y":
        x = VALVE_X[k]
        v = ycyl(x, VALVE_Z, ELB_R, VALVE_Y0, VALVE_Y1)
        yc = (VALVE_Y0 + VALVE_Y1) / 2
        v = v.union(box(x - h, x + h, yc - 15.0, yc + 15.0, VALVE_Z - h, VALVE_Z + h))
        return v.union(box(x - 14.0, x + 14.0, yc - 15.0, yc + 15.0, VALVE_Z + h, VALVE_Z + h + VALVE_COIL))
    xc = x + VALVE_LEN / 2
    v = xcyl(y, z0, ELB_R, x, x + VALVE_LEN)
    v = v.union(box(xc - 15.0, xc + 15.0, y - h, y + h, z0 - h, z0 + h))
    return v.union(box(xc - 14.0, xc + 14.0, y - 14.0, y + 14.0, z0 + h, z0 + h + VALVE_COIL))

def valve_stub(k):
    """Spezzone di tubo tra collettore e valvola (14 mm nel collettore, 15 nella valvola)."""
    return seg((VALVE_X[k], VALVE_Y1 - INS, VALVE_Z), (VALVE_X[k], MAN_Y0 + 14.0, VALVE_Z), PIPE_R)

def psu():
    """Mean Well LRS-350-12 in piedi sul fianco (215 lungo Y, 115 alto, 30 di spessore), morsetti verso il retro."""
    return box(PSU_X0, PSU_X0 + PSU_H, PSU_Y0, PSU_Y0 + PSU_L, PSU_Z0, PSU_Z0 + PSU_W)

def mainboard():
    """FilMachine Mainboard v1.1 (110 x 90): scheda da 1.6 in piedi nel piano XZ, componenti (fino a 26 mm) verso +Y."""
    return box(MB_X0, MB_X0 + MB_L, MB_Y0, MB_Y0 + 1.6, MB_Z0, MB_Z0 + MB_H).union(box(MB_X0 + 3, MB_X0 + MB_L - 3, MB_Y0 + 1.6, MB_Y0 + 26.0, MB_Z0 + 3, MB_Z0 + MB_H - 3))

DISP_L, DISP_W, DISP_T = 114.4, 66.8, 3.2
def display():
    """Scheda Guition JC4880P433 (114.4 x 66.8) in verticale dietro il pannello frontale, vetro contro il pannello."""
    x, z = DISP_X, DISP_Z
    return box(x - DISP_L / 2, x + DISP_L / 2, 0.0, DISP_T, z - DISP_W / 2, z + DISP_W / 2).union(box(x - 50.0, x + 50.0, DISP_T, 11.0, z - 29.0, z + 29.0))

def iec_inlet():
    """Presa IEC C14 a pannello con interruttore e fusibile (foro 27.5 x 47.5), nel pannello posteriore a destra."""
    b = box(IEC_X[0] - 4.0, IEC_X[1] + 4.0, D_ + WT, D_ + WT + 2.0, IEC_Z[0] - 4.0, IEC_Z[1] + 4.0)
    return b.union(box(IEC_X[0] + 0.25, IEC_X[1] - 0.25, D_ - 27.0, D_ + WT, IEC_Z[0] + 0.25, IEC_Z[1] - 0.25))

def tap(k):
    """Rubinetto a margherita in PP con due portagomma 10 mm, sul pannello frontale sotto il contenitore k: corpo Ø20 lungo Y
    (portagomma interno verso +Y per il silicone dalla camera), volantino davanti, portagomma esterno in giu' (bottiglia)."""
    x = TAP_X[k]
    t = ycyl(x, TAP_Z, 10.0, -WT - 16.0, 12.0).union(ycyl(x, TAP_Z, 15.0, -WT - 22.0, -WT - 16.0)).union(ycyl(x, TAP_Z, 4.5, 12.0, 27.0))
    return t.union(zcyl(x, -WT - 9.0, 4.5, TAP_Z - 22.0, TAP_Z))

def recovery_outlet():
    """Uscita RECUPERO: passaparete portagomma 10 - portagomma 10 nel pannello frontale, con gomito esterno verso il basso."""
    b = ycyl(REC_X, REC_Z, 4.5, -WT - 8.0, 20.0).union(ycyl(REC_X, REC_Z, 11.0, -WT - 3.0, -WT)).union(ycyl(REC_X, REC_Z, 11.0, 0.0, 5.0))
    b = b.union(ycyl(REC_X, REC_Z, 6.0, -WT - 14.0, -WT - 8.0))
    return b.union(zcyl(REC_X, -WT - 11.0, 4.5, REC_Z - 22.0, REC_Z - 3.0)).union(cq.Workplane("XY").add(cq.Solid.makeSphere(6.0)).translate((REC_X, -WT - 11.0, REC_Z)))

# ---- linee: spezzate a tratti paralleli agli assi; gomiti a innesto agli angoli ----
def chem_hose_pts(k):
    """Silicone 8 x 12 dall'adattatore sulla bocca laterale (+X) della camera al portagomma interno del rubinetto."""
    x, xt = CX[k] + CH_SIDE_X, TAP_X[k]
    return [(x, CY, CH_SIDE_Z), (x, CY, TAP_Z), (x, TAP_LANE_Y, TAP_Z), (xt, TAP_LANE_Y, TAP_Z), (xt, 20.0, TAP_Z)]

def chem_adapter(k):
    """Adattatore innesto 3/8" - portagomma 10 sulla bocca laterale della camera (codolo da 9,52 nella sede con O-ring)."""
    x = CX[k]
    return seg((x + CH_R - 14.0, CY, CH_SIDE_Z), (x + CH_R + 6.0, CY, CH_SIDE_Z), PIPE_R).union(seg((x + CH_R + 6.0, CY, CH_SIDE_Z), (x + CH_SIDE_X, CY, CH_SIDE_Z), 4.5))

def valve_elbow(k):
    """Gomito davanti alla valvola k: dalla bocca anteriore (asse Y) in su'."""
    return elbow((VALVE_X[k], ELB_Y, VALVE_Z), (0, 1, 0), (0, 0, 1))

def chem_stub_low(k):
    return seg((VALVE_X[k], ELB_Y, VALVE_Z + ELB_L - INS), (VALVE_X[k], ELB_Y, CH_Z0 + 14.0), PIPE_R)

def chem_stub_high(k):
    return seg((VALVE_X[k], ELB_Y, CH_Z1 - 14.0), (VALVE_X[k], ELB_Y, BULK_Z0 + INS), PIPE_R)

def wb_line_pts():
    """Valvola WB -> su -> sopra la valvola (oltre la bobina) -> a destra -> su nel raccordo del passaparete WB nel fondo del bagno."""
    x = VALVE_X[3]
    return [(x, ELB_Y, VALVE_Z + ELB_L), (x, ELB_Y, WB_LANE_Z), (x, WB_Y, WB_LANE_Z), (WB_X, WB_Y, WB_LANE_Z), (WB_X, WB_Y, BULK_Z0)]

def waste_line_pts():
    """Valvola SCARICO -> su -> dritto all'attacco SCARICO sul pannello posteriore."""
    x = VALVE_X[4]
    return [(x, ELB_Y, VALVE_Z + ELB_L), (x, ELB_Y, LINE_Z), (x, D_ - 18.0, LINE_Z)]

def bath_overflow_pts():
    """Troppopieno del bagno: dal raccordo sotto il passaparete giu' alla corsia, poi dritto all'attacco sul retro."""
    return [(OVF_X, SRV_Y, BULK_Z0), (OVF_X, SRV_Y, LINE_Z), (OVF_X, D_ - 18.0, LINE_Z)]

def inlet_pts():
    """Acqua di rete: attacco sul retro -> flussimetro (lungo Y)."""
    return [(FS_X, D_ - 18.0, FS_Z), (FS_X, FS_Y1 + FS_AD, FS_Z)]

def fs_valve_pts():
    """Flussimetro -> gomito -> valvola dell'ingresso (lungo X)."""
    return [(FS_X, FS_Y0 - FS_AD, FS_Z), (FS_X, IV_Y, IV_Z), (IV_X0, IV_Y, IV_Z)]

def inlet_pts2():
    """Dalla valvola dell'ingresso: dritto fino a x = IN_X, poi lungo Y (tra il tubo della pompa e la valvola WB, sopra il corpo
    della valvola), poi su nel raccordo del passaparete dell'ingresso."""
    x0 = IV_X0 + VALVE_LEN
    return [(x0, IV_Y, IV_Z), (IN_RISER_X, IV_Y, IV_Z), (IN_RISER_X, IN_Y, IV_Z), (IN_RISER_X, IN_Y, BULK_Z0)]

def flow_sensor():
    """Flussimetro YF-S201 (corpo 65 x 35 x 35, nippli G 1/2") con i due raccordi 1/2" F - 3/8" a innesto, asse Y."""
    f = box(FS_X - 17.5, FS_X + 17.5, FS_Y0, FS_Y1, FS_Z - 17.5, FS_Z + 17.5)
    f = f.union(ycyl(FS_X, FS_Z, 10.0, FS_Y0 - FS_AD, FS_Y1 + FS_AD))
    return f.union(box(FS_X + 17.5, FS_X + 27.0, (FS_Y0 + FS_Y1) / 2 - 9, (FS_Y0 + FS_Y1) / 2 + 9, FS_Z - 8.0, FS_Z + 8.0))   # sensore Hall

def inlet_valve():
    return valve(0, x=IV_X0, y=IV_Y, z0=IV_Z, axis="X")

def panel_bulkhead(name):
    """Passaparete a innesto 3/8" - 3/8" (foro Ø16) nel pannello posteriore, con i dadi."""
    x, z = CONN[name]
    b = ycyl(x, z, 8.0, D_ - 18.0, D_ + WT + 8.0)
    return b.union(ycyl(x, z, 11.0, D_ - 6.0, D_)).union(ycyl(x, z, 11.0, D_ + WT, D_ + WT + 5.0))

def manifold_outlet():
    """Gomito a codolo sull'uscita del collettore (faccia -X): codolo nella sede con O-ring, braccio a innesto verso il fronte."""
    p = (MAN_OUT[0] - ELB_R, MAN_OUT_Y, VALVE_Z)
    return stem_elbow(p, (1, 0, 0), (0, -1, 0), 14.0 + ELB_R)

def out_adapter():
    """Adattatore innesto 3/8" - portagomma 6,4 sul gomito del collettore."""
    p = (MAN_OUT[0] - ELB_R, MAN_OUT_Y - ELB_L + INS, VALVE_Z)
    return seg(p, (p[0], p[1] - 22.0, VALVE_Z), PIPE_R).union(seg((p[0], p[1] - 22.0, VALVE_Z), (p[0], p[1] - 34.0, VALVE_Z), 3.2))

def pump_hose_pts():
    """Dall'adattatore del collettore alla bocca A della Kamoer (silicone 6 x 10)."""
    (xa, ya, za), _ = PUMP_PORTS
    x = MAN_OUT[0] - ELB_R; y = MAN_OUT_Y - ELB_L + INS - 34.0
    return [(x, y, VALVE_Z), (x, ya - 5.0, VALVE_Z), (xa, ya - 5.0, VALVE_Z), (xa, ya - 5.0, za), (xa, ya, za)]

def tank_hose_low_pts():
    """Dalla bocca B della Kamoer all'adattatore sotto il passaparete della linea tank (silicone 6 x 10)."""
    _, (xb, yb, zb) = PUMP_PORTS
    x, y, _ = DECK_CUTS["tank"]
    zl = 122.0
    return [(xb, yb, zb), (xb, yb - 12.0, zb), (xb, yb - 12.0, zl), (x + 14.0, yb - 12.0, zl), (x + 14.0, y - 4.0, zl), (x, y - 4.0, zl), (x, y, zl), (x, y, BULK_Z0 - 20.0)]

def tank_adapter():
    x, y, _ = DECK_CUTS["tank"]
    return seg((x, y, BULK_Z0 + INS - 0.01), (x, y, BULK_Z0 - 8.0), PIPE_R).union(seg((x, y, BULK_Z0 - 8.0), (x, y, BULK_Z0 - 20.0), 3.2))

def tank_hose():
    """Silicone 8 x 12 dal portagomma della tank al portagomma del passaparete nel fondo: in discesa."""
    x0, y0, z0 = TANK_BARB; x1, y1 = TANK_BH
    pts = [(x0 - 12.0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x1, y1, FLOOR + 16.0)]
    return route(pts, HOSE_R)

def overflow_hose_pts():
    """Silicone 8 x 12 del troppopieno della tank: dalla colonna giu' attraverso fondo e piano (gommino), poi all'attacco sul retro."""
    x, y = TANK_OVF; xc, zc = CONN["TROPPOPIENO_TANK"]
    return [(x, y, Z_T0 + Z_BOT + 10.0), (x, y, zc), (xc, y, zc), (xc, D_ - 18.0, zc)]

def recovery_hose_pts():
    """Silicone 8 x 12 dall'adattatore sulla bocca anteriore della valvola RECUPERO al portagomma interno dell'uscita sul fronte."""
    x = VALVE_X[5]
    return [(x, VALVE_Y0 - 36.0, VALVE_Z), (x, REC_LANE_Y, VALVE_Z), (REC_X, REC_LANE_Y, VALVE_Z), (REC_X, 20.0, VALVE_Z)]

def recovery_adapter():
    x = VALVE_X[5]
    return seg((x, VALVE_Y0 - INS, VALVE_Z), (x, VALVE_Y0 - 22.0, VALVE_Z), PIPE_R).union(seg((x, VALVE_Y0 - 22.0, VALVE_Z), (x, VALVE_Y0 - 36.0, VALVE_Z), 4.5))

def hoses():
    out = {"REF_silicone_6x10_collettore_pompa": route(pump_hose_pts(), 5.0), "REF_silicone_6x10_pompa_tank": route(tank_hose_low_pts(), 5.0),
           "REF_silicone_8x12_tank": tank_hose(), "REF_silicone_8x12_troppopieno_tank": route(overflow_hose_pts(), HOSE_R),
           "REF_silicone_8x12_RECUPERO": route(recovery_hose_pts(), HOSE_R), "REF_adattatore_portagomma_RECUPERO": recovery_adapter(),
           "REF_adattatore_portagomma_collettore": out_adapter(), "REF_adattatore_portagomma_tank": tank_adapter()}
    for k in range(3):
        out[f"REF_silicone_8x12_rubinetto_C{k + 1}"] = route(chem_hose_pts(k), HOSE_R)
        out[f"REF_adattatore_portagomma_C{k + 1}"] = chem_adapter(k)
    return out

def hose_lengths():
    """Metri di silicone 8 x 12 e 6 x 10 per la distinta."""
    s8 = sum(route_len(f()) for f in (recovery_hose_pts, overflow_hose_pts)) + sum(route_len(chem_hose_pts(k)) for k in range(3)) + 140.0
    s6 = route_len(pump_hose_pts()) + route_len(tank_hose_low_pts())
    return s8 / 1000.0, s6 / 1000.0

LINES = {"WB": wb_line_pts, "SCARICO": waste_line_pts, "TROPPOPIENO_BAGNO": bath_overflow_pts, "INGRESSO_1": inlet_pts, "INGRESSO_2": inlet_pts2,
         "FLUSSIMETRO_VALVOLA": fs_valve_pts}

def lines():
    """Tubi 3/8" e gomiti di tutte le linee."""
    out = {}
    for n, f in LINES.items():
        pts = f()
        t = tubes(pts)
        if t is not None: out[f"REF_tubo_{n}"] = t
        e = elbows(pts)
        if e is not None: out[f"REF_gomiti_{n}"] = e
    for k in range(3):
        out[f"REF_gomito_valvola_C{k + 1}"] = valve_elbow(k)
        out[f"REF_spezzone_C{k + 1}_basso"] = chem_stub_low(k)
        out[f"REF_spezzone_C{k + 1}_alto"] = chem_stub_high(k)
    for k in (3, 4):
        out[f"REF_gomito_valvola_{VALVE_NAMES[k]}"] = valve_elbow(k)
    for k in range(6):
        out[f"REF_spezzone_collettore_{VALVE_NAMES[k]}"] = valve_stub(k)
    out["REF_gomito_codolo_collettore"] = manifold_outlet()
    return out

def line_lengths():
    """Metri di tubo 3/8" per la distinta: tratti delle linee + spezzoni."""
    tot = 0.0
    for n, f in LINES.items():
        tot += sum(tube_lens(f()))
    tot += 6 * ((MAN_Y0 + 14.0) - (VALVE_Y1 - INS))
    tot += 3 * ((CH_Z0 + 14.0) - (VALVE_Z + ELB_L - INS)) + 3 * ((BULK_Z0 + INS) - (CH_Z1 - 14.0))
    tot += (OVF_Z - FLOOR - 24.0) + (IN_Z - FLOOR - 24.0)
    return tot / 1000.0

def elbow_count():
    return sum(max(len(f()) - 2, 0) for f in LINES.values()) + 5 + 1

def all_refs():
    P = {"REF_base_multistrato": base_board(), "REF_piano_multistrato": deck(), "REF_alimentatore_LRS-350-12": psu(), "REF_mainboard_v1-1": mainboard(),
         "REF_display_JC4880P433": display(), "REF_presa_IEC_interruttore_fusibile": iec_inlet(), "REF_pompa_Kamoer_KPHM600": pump(),
         "REF_flussimetro_YF-S201": flow_sensor(), "REF_valvola_INGRESSO": inlet_valve(), "REF_uscita_RECUPERO": recovery_outlet(),
         "REF_pompetta_ricircolo_12V": circulator(), "REF_sonda_DS18B20_bagno": probe_bath(), "REF_passaparete_WB": wb_bulkhead(),
         "REF_passaparete_TROPPOPIENO_BAGNO": overflow_bulkhead(), "REF_passaparete_INGRESSO": inlet_bulkhead(),
         "REF_passaparete_linea_tank": bulkhead_floor(*TANK_BH, top="barb"), "REF_gommino_troppopieno_tank": tank_overflow_grommet(),
         "REF_sensore_livello_bagno_MIN": level_sensor(LVL_ZMIN), "REF_sensore_livello_bagno_MAX": level_sensor(LVL_ZMAX)}
    for n, f in BATH_PANELS: P[n] = f()
    for k in range(3):
        P[f"REF_contenitore_500ml_C{k + 1}"] = container(k); P[f"REF_tappo_C{k + 1}"] = container_lid(k)
        P[f"REF_chimica_C{k + 1}"] = chemical(k); P[f"REF_passaparete_C{k + 1}"] = container_bulkhead(k)
        P[f"REF_sensore_MAX_C{k + 1}"] = chem_max_sensor(k); P[f"REF_rubinetto_C{k + 1}"] = tap(k)
    for i in range(2): P[f"REF_riscaldatore_100W_{i + 1}"] = heater(i)
    for k in range(6): P[f"REF_valvola_{VALVE_NAMES[k]}"] = valve(k)
    for n in CONN: P[f"REF_attacco_retro_{n}"] = panel_bulkhead(n)
    P.update(lines()); P.update(hoses())
    return P

if __name__ == "__main__":
    import time
    t = time.time(); P = all_refs()
    for n, p in P.items():
        s = p.val(); bb = s.BoundingBox()
        print(f"{n:42s} valid={s.isValid()} solidi={len(s.Solids())} vol={s.Volume() / 1000:8.1f} cm3  X[{bb.xmin:.0f},{bb.xmax:.0f}] Y[{bb.ymin:.0f},{bb.ymax:.0f}] Z[{bb.zmin:.0f},{bb.zmax:.0f}]")
    s8, s6 = hose_lengths()
    print(f"{len(P)} sagome in {time.time() - t:.0f} s | acqua nel bagno {water_volume_l():.2f} l | tubo 3/8 {line_lengths():.2f} m, gomiti {elbow_count()} | silicone 8x12 {s8:.2f} m, 6x10 {s6:.2f} m")
