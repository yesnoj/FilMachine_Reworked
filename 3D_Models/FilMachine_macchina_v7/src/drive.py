"""Trasmissione della spirale, rilascio 7: giunto magnetico attraverso la testata della tank.
Dentro la tank il mozzo (nose a tre denti nel tubo della spirale + flangia Ø56 con sei magneti di coppia Ø8x3 a r = 14 e
quattro magneti indicatori Ø5x2 a r = 23,5, tutti sotto il tappo incollato) gira su un perno inox Ø6 incollato nel boss
della testata, con una rondella PTFE di spinta. Fuori, nel vano asciutto, il disco Ø40 con sei magneti uguali sull'albero
a D del motoriduttore JGB37-520, coassiale con la spirale, dentro l'incavo Ø60 x 1,5 della testata; la staffa del motore
scende sul fondo del vano; la staffa del Hall porta il KY-003 di fianco al disco (a sinistra, sull'asse), col sensore
piegato di piatto che guarda i magneti indicatori. Perno folle a molla sul lato opposto, come prima."""
import math
import cadquery as cq
from common import *
from tank import box, xz_prism, yz_prism, xy_prism, ycyl, zcyl, xcyl, PIL_Y0, PIN_R, WIN_R, WIN_T, PIN_BOSS_R, PIN_BOSS_L, HEAD_SCREWS, WIN_TOP

# ---- quote lungo l'asse (Y) lato trasmissione ----
DOG_Y0 = TUBE_HALF - 10.0            # fondo del nose del mozzo (dentro il foro del tubo)
HUB_Y0 = TUBE_HALF + 4.0             # flangia del mozzo: da HUB_Y0 a HUB_Y1 (6 mm)
WASHER_T = 1.0                       # rondella PTFE Ø24 x Ø13 x 1 tra flangia e boss
HUB_Y1 = Y_DRV - WASHER_T
HUB_R = 28.0                         # flangia Ø56
MAG_R, MAG_D, MAG_H, MAG_N = 14.0, 8.0, 3.0, 6     # magneti di coppia Ø8x3 N52, poli alternati (tasche fino a r = 18,1: sotto il tappo)
IND_R, IND_D, IND_H, IND_N, IND_A0 = 23.5, 5.0, 2.0, 4, 45.0   # magneti indicatori Ø5x2 per il Hall (tasche r 20,9..26,1, sfalsati di 45 gradi
                                                               # dai magneti di coppia), polo sud verso la testata
CAP_T = 1.0                          # tappo incollato che chiude le tasche (incapsulamento verso la chimica): disco da 1 mm incollato di
                                     # piatto sulla faccia delle tasche (nessuna sede incassata: la faccia resta piana e si stampa sul piatto)
CAP_MARGIN = 1.15                    # il tappo resta 1,15 mm dentro il bordo del pezzo
PIN_LEN = 22.0                       # perno inox Ø6: 6,5 nel boss + parete, il resto nel mozzo
DISC_R, DISC_T = 20.0, 6.0           # disco motore Ø40 x 6, nell'incavo Ø60 x 1,5 della testata
DISC_Y0 = YO1 - WIN_T + 0.5          # faccia del disco: 0,5 mm dal fondo dell'incavo
DISC_HUB_R, DISC_HUB_L = 8.0, 7.0    # mozzo del disco sull'albero a D Ø6
MOT_FACE = DISC_Y0 + DISC_T + DISC_HUB_L + 6.5     # faccia del riduttore (contro la piastra): il mozzetto Ø12 x 6 passa nella piastra e finisce 0,5 mm dopo il mozzo del disco
MOT_ECC = 7.0                        # eccentricita' dell'albero d'uscita del JGB37-520 (il corpo sta 7 mm sopra l'asse)
MOT_CEN = (0.0, HC + MOT_ECC)        # centro del riduttore (x, z)
PLATE_T = 4.0
HALL_X, HALL_Z = -IND_R, HC          # il sensore guarda i magneti indicatori a sinistra dell'asse (sull'orizzontale), dentro l'incavo
HALL_YF = YO1 - WIN_T + 0.4          # faccia sensibile del Hall: 0,4 mm dal fondo dell'incavo
HALL_BODY = (3.0, 1.52, 4.06)        # A3144 SIP: lunghezza lungo X (verso il PCB), spessore lungo Y, larghezza lungo Z
HALL_PCB_L, HALL_PCB_W, HALL_PCB_T = 18.5, 15.0, 1.6                   # PCB del KY-003, di piatto (piano XZ), lungo X verso -X
HALL_PCB = (HALL_YF + HALL_BODY[1] + 0.3, HALL_YF + HALL_BODY[1] + 0.3 + HALL_PCB_T)   # (y0, y1) del PCB: sensore piegato di piatto davanti al PCB, 0,3 mm di piega
HALL_PCB_X1 = HALL_X - HALL_BODY[0] / 2 + 1.2 + 0.0      # bordo +X del PCB (i reofori stanno 1,2 mm dentro il bordo; il corpo sporge oltre)
HALL_PCB_X0 = HALL_PCB_X1 - HALL_PCB_L
HALL_SCREWS = ((-28.0, HC - 6.0), (-28.0, HC + 6.0))   # viti 3 x 12 dalla faccia posteriore della piastra del motore nella staffa (x, z)
BRACKET_FEET = ((-26.0, 0.0), (26.0, 0.0))

def _teeth(d):
    for k in range(3):
        a = NOTCH_ANG + 120.0 * k
        t = (cq.Workplane("XY").box(2.9, 5.4, 3.6, centered=(False, True, False)).translate((TUBE_BORE - 0.4, 0, 0)))
        t = t.rotate((0, 0, 0), (0, 0, 1), a).translate((0, 0, -TUBE_HALF))
        d = d.union(t.rotate((0, 0, 0), (1, 0, 0), 90).translate((0, 0, HC)))
    return d

def mag_hub():
    """Mozzo magnetico interno: nose nel tubo con i tre denti, piattello contro la testa del tubo, flangia con le tasche dei
    magneti aperte verso la testata (chiuse dal tappo B17 incollato), foro passante per il perno Ø6 (gioco 0,15), incavo
    Ø13 per il boss del perno. Si stampa con la faccia delle tasche sul piatto (le tasche sono fori ciechi dal piatto) e il
    nose in alto, senza supporti."""
    d = ycyl(0, HC, TUBE_BORE - 0.2, TUBE_HALF - 4.0, TUBE_HALF)
    d = d.union(ycyl(0, HC, 7.5, DOG_Y0, TUBE_HALF - 4.0))
    d = _teeth(d)
    d = d.union(ycyl(0, HC, TUBE_R - 0.3, TUBE_HALF, HUB_Y0))                       # piattello contro la testa del tubo
    d = d.union(ycyl(0, HC, HUB_R, HUB_Y0, HUB_Y1 - CAP_T))                         # flangia (il tappo B17 completa lo spessore fino a HUB_Y1)
    yf = HUB_Y1 - CAP_T                                                             # faccia delle tasche
    for k in range(MAG_N):
        a = math.radians(360.0 / MAG_N * k)
        d = d.cut(ycyl(MAG_R * math.cos(a), HC + MAG_R * math.sin(a), MAG_D / 2 + 0.1, yf - MAG_H, yf + 0.01))
    for k in range(IND_N):
        a = math.radians(IND_A0 + 360.0 / IND_N * k)
        d = d.cut(ycyl(IND_R * math.cos(a), HC + IND_R * math.sin(a), IND_D / 2 + 0.1, yf - IND_H, yf + 0.01))
    d = d.cut(ycyl(0, HC, 3.15, DOG_Y0 - 0.01, yf + 0.01))                           # perno Ø6
    d = d.cut(ycyl(0, HC, PIN_BOSS_R + 0.5, Y_DRV - PIN_BOSS_L - 0.3, yf + 0.01))        # incavo per il boss (il mozzo arriva a 1 mm dalla parete)
    return d

def hub_cap():
    """Tappo anulare del mozzo (Ø53,7 / Ø13,2 x 1): chiude le tasche dei magneti; si incolla di piatto con epossidica sulla faccia
    della flangia, poi si sigilla il bordo. Il foro centrale lascia passare il boss del perno; la rondella PTFE appoggia sul tappo."""
    return ycyl(0, HC, HUB_R - CAP_MARGIN, HUB_Y1 - CAP_T, HUB_Y1).cut(ycyl(0, HC, PIN_BOSS_R + 0.6, HUB_Y1 - CAP_T - 0.01, HUB_Y1 + 0.01))

def mag_disc():
    """Disco motore Ø40 x 6 con le sei tasche verso la testata (chiuse dal tappo B19), mozzo sull'albero a D con grano M3.
    Si stampa con la faccia delle tasche sul piatto (fori ciechi dal piatto) e il mozzo in alto, senza supporti."""
    d = ycyl(0, HC, DISC_R, DISC_Y0 + CAP_T, DISC_Y0 + DISC_T).union(ycyl(0, HC, DISC_HUB_R, DISC_Y0 + DISC_T, DISC_Y0 + DISC_T + DISC_HUB_L))   # il tappo B19 completa lo spessore fino a DISC_Y0
    for k in range(MAG_N):
        a = math.radians(360.0 / MAG_N * k)
        d = d.cut(ycyl(MAG_R * math.cos(a), HC + MAG_R * math.sin(a), MAG_D / 2 + 0.1, DISC_Y0 + CAP_T - 0.01, DISC_Y0 + CAP_T + MAG_H))
    bore = ycyl(0, HC, 3.05, DISC_Y0 + 2.0, DISC_Y0 + DISC_T + DISC_HUB_L + 0.01)
    bore = bore.cut(box(-3.1, 3.1, DISC_Y0 + 1.99, MOT_FACE - 9.0, HC + 2.5, HC + 3.2))   # spianatura a D (solo dove l'albero e' spianato: 6 mm dalla punta)
    d = d.cut(bore)
    yg = DISC_Y0 + DISC_T + DISC_HUB_L / 2
    d = d.cut(zcyl(0, yg, M3_TAP, HC, HC + DISC_HUB_R + 0.01))                        # grano M3 sulla spianatura
    return d

def disc_cap():
    """Tappo del disco (Ø37,7 x 1, pieno): incollato di piatto sulla faccia delle tasche, bordo sigillato."""
    return ycyl(0, HC, DISC_R - CAP_MARGIN, DISC_Y0, DISC_Y0 + CAP_T)

def motor_bracket():
    """Staffa del motoriduttore: piastra verticale con il foro dell'albero sull'asse della spirale e le sei asole M3 del
    riduttore (centro 7 mm sopra l'asse), due gambe fino al fondo del vano con asole per l'allineamento. Si stampa di piatto."""
    y0, y1 = MOT_FACE - PLATE_T, MOT_FACE
    p = box(-34.0, 34.0, y0, y1, HC - 32.0, HC + 42.0)
    zf = Z_BOT - 4.0                                                                 # pavimento del vano asciutto (la tank sta su piedini di 4 mm)
    p = p.union(box(-34.0, -26.0, y0, y1, zf, HC - 32.0)).union(box(26.0, 34.0, y0, y1, zf, HC - 32.0))
    for x, z in BRACKET_FEET:                                                        # piedi con asola sul fondo del vano, dietro la piastra
        p = p.union(box(x - 8.0, x + 8.0, y0, y1 + 12.0, zf, zf + PLATE_T))
        p = p.cut(box(x - 1.7, x + 1.7, y1 + 3.0, y1 + 10.0, zf - 0.01, zf + PLATE_T + 0.01))
    p = p.cut(ycyl(0, HC, 6.5, y0 - 0.01, y1 + 0.01))                                 # mozzetto Ø12 del riduttore
    for k in range(6):
        a = math.radians(90 + 60 * k)
        p = p.cut(ycyl(MOT_CEN[0] + 15.5 * math.cos(a), MOT_CEN[1] + 15.5 * math.sin(a), M3_CLR, y0 - 0.01, y1 + 0.01))
    for x, z in HALL_SCREWS:                                                         # viti della staffa Hall: passanti nella piastra, dal retro
        p = p.cut(ycyl(x, z, M3_CLR, y0 - 0.01, y1 + 0.01))
    return p

def hall_bracket():
    """Staffa del KY-003, a sinistra del disco: cornice che tiene il PCB di piatto (piano XZ, parallelo alla testata) in una
    fessura aperta verso la testata e verso +X (il PCB si infila da destra, col sensore - piegato di piatto sul bordo +X -
    che resta fuori dalla fessura e sporge nell'incavo della testata); dietro il PCB c'e' lo spazio dei componenti e dei
    tre pin; il blocco posteriore sta contro la piastra del motore con due viti 3 x 12 dal retro della piastra (fori M3
    filettati). Il PCB si ferma con una goccia di colla a caldo. Si stampa di fianco (faccia -X sul piatto)."""
    y0 = MOT_FACE - PLATE_T
    x0, x1 = HALL_PCB_X0 - 1.6 - 2.0, HALL_PCB_X1 + 0.8
    zb, zt = HC - HALL_PCB_W / 2 - 2.4, HC + HALL_PCB_W / 2 + 2.4
    b = box(x0, x1, HALL_PCB[0], y0, zb, zt)
    b = b.cut(box(HALL_PCB_X0 - 0.2, x1 + 0.01, HALL_PCB[0] - 0.01, HALL_PCB[1] + 0.1, HC - HALL_PCB_W / 2 - 0.1, HC + HALL_PCB_W / 2 + 0.1))   # fessura del PCB
    b = b.cut(box(HALL_PCB_X0 - 0.2, x1 + 0.01, HALL_PCB[1] + 0.09, HALL_PCB[1] + 3.0, HC - HALL_PCB_W / 2 - 0.1, HC + HALL_PCB_W / 2 + 0.1))  # componenti (LED, resistenza)
    b = b.cut(box(HALL_PCB_X0 - 0.2, HALL_PCB_X0 + 6.5, HALL_PCB[1] + 0.09, HALL_PCB[1] + 9.5, HC - 5.0, HC + 5.0))                            # i tre pin del connettore
    for x, z in HALL_SCREWS:
        b = b.cut(ycyl(x, z, M3_TAP, y0 - 8.5, y0 + 0.02))
    return b

FRAME_W = 12.0                       # larghezza della cornice e della guarnizione della finestra (apertura z 3..82: l'incavo Ø60 arriva a z = 80)
GASKET_T = 2.0
BATH_T = 4.0                         # parete del bagno (PMMA)
def window_frame():
    """Cornice che stringe la testata della tank sulla guarnizione, dall'esterno del bagno (vano asciutto): telaio largo 12 mm
    sul perimetro della testata, otto fori M3 passanti. Si stampa di piatto."""
    yf = YO1 + GASKET_T + BATH_T
    f = box(XO0, XO1, yf, yf + PLATE_T, Z_BOT - 4.0, WIN_TOP)
    f = f.cut(box(XO0 + FRAME_W, XO1 - FRAME_W, yf - 0.01, yf + PLATE_T + 0.01, Z_BOT - 4.0 + FRAME_W, WIN_TOP - FRAME_W))
    for x, z in HEAD_SCREWS:
        f = f.cut(ycyl(x, z, M3_CLR, yf - 0.01, yf + PLATE_T + 0.01))
    return f

def window_gasket():
    """Guarnizione piana EPDM 2 mm tra la testata e la parete del bagno (REF, tagliata da foglio)."""
    g = box(XO0, XO1, YO1, YO1 + GASKET_T, Z_BOT - 4.0, WIN_TOP)
    g = g.cut(box(XO0 + FRAME_W, XO1 - FRAME_W, YO1 - 0.01, YO1 + GASKET_T + 0.01, Z_BOT - 4.0 + FRAME_W, WIN_TOP - FRAME_W))
    for x, z in HEAD_SCREWS:
        g = g.cut(ycyl(x, z, M3_CLR, YO1 - 0.01, YO1 + GASKET_T + 0.01))
    return g

def idle_pin(dy=0.0):
    """Perno folle a molla: tampone nel foro del tubo, battuta contro la testa del tubo, gambo cavo per la molla
    con cava per la vite di ritegno. dy = arretramento (0 = spirale montata)."""
    y_sh = -TUBE_HALF
    p = ycyl(0, HC, TUBE_BORE - 0.15, y_sh, y_sh + 6.0)
    p = p.cut(ycyl(0, HC, TUBE_BORE + 1, y_sh + 5.0, y_sh + 6.01).cut(
        cq.Workplane("XZ").center(0, HC).circle(TUBE_BORE - 0.15).workplane(offset=-1.0).circle(TUBE_BORE - 1.2).loft(combine=True)
        .translate((0, y_sh + 5.0, 0))))
    p = p.union(cq.Workplane("XY").add(cq.Solid.makeCone(15.0, TUBE_BORE - 0.15, 2.0, cq.Vector(0, y_sh - 2.0, HC), cq.Vector(0, 1, 0))))
    p = p.union(ycyl(0, HC, PIN_R, y_sh - 16.0, y_sh - 2.0))
    p = p.cut(ycyl(0, HC, 3.25, y_sh - 16.01, y_sh - 6.0))
    p = p.cut(box(-1.6, 1.6, y_sh - 15.2, y_sh - 5.8, HC + PIN_R - 1.2, HC + PIN_R + 0.1))
    return p.translate((0, -dy, 0))

# ---- stima della coppia del giunto ----
def coupling_torque(gap_mm=None):
    """Coppia di stallo stimata del giunto (N m): forza di taglio per coppia di poli ~ 0,55 x forza assiale a quel traferro,
    forza assiale di un Ø8x3 N52 contro un uguale a distanza g: ~ 12 N x exp(-g / 3,2 mm) (curva del costruttore, approssimata)."""
    g = gap_mm if gap_mm is not None else (Y_DRV - HUB_Y1) + CAP_T + WIN_T + 0.5 + CAP_T       # da faccia a faccia dei magneti
    f_ax = 12.0 * math.exp(-g / 3.2)
    return MAG_N * 0.55 * f_ax * MAG_R / 1000.0, g

if __name__ == "__main__":
    for n, f in (("mozzo", mag_hub), ("tappo_mozzo", hub_cap), ("disco_motore", mag_disc), ("tappo_disco", disc_cap),
                 ("staffa_motore", motor_bracket), ("staffa_hall", hall_bracket), ("cornice", window_frame), ("guarnizione", window_gasket),
                 ("perno_folle", idle_pin)):
        s = f().val(); bb = s.BoundingBox()
        print(f"{n:14s} valid={s.isValid()} solids={len(s.Solids())} vol={s.Volume()/1000:6.2f} x[{bb.xmin:.1f},{bb.xmax:.1f}] y[{bb.ymin:.1f},{bb.ymax:.1f}] z[{bb.zmin:.1f},{bb.zmax:.1f}]")
    t, g = coupling_torque()
    print(f"traferro magnete-magnete {g:.1f} mm -> coppia di stallo stimata {t:.2f} N m | faccia motore y = {MOT_FACE:.1f}, Y_DRV = {Y_DRV:.1f}, YO1 = {YO1:.1f}")
