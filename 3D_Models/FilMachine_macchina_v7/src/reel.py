"""Spirale 35/120: due flange a spirale che si bloccano a baionetta su un tubo centrale; la fascetta della clip passa in
una finestra del tubo (come nella Lab-Box).
v6.2: come nella Lab-Box, una coppia di flange per ogni formato. Tubo, anelli alette e trascinamento sono comuni:
le flange 135 si bloccano nella stazione stretta, le 120 (giri piu' distanziati) in quella larga.
v6.2: niente tamburo, come nella Lab-Box: tra due collarini il tubo ha un passaggio da parte a parte, finestra larga da un
lato e stretta dall'altro; la fascetta esce dalla stretta e si avvolge direttamente sul tubo, il perno del suo capo resta
dentro. La costola d'attacco della spirale arriva fino al mozzo della flangia (al centro). La gola dello scatto c'e' solo
dove arriva il dentino delle flange e degli anelli alette.
v6.1: costole alte 2,5 mm (erano 1,2) e centro come la Lab-Box: dal giro piu' interno la costola arriva direttamente al
mozzo con una curva di mezzo giro, lasciando libero il centro (8 giri regolari: con il diametro di 93,5 ne servono 8
per un 36 pose; la Lab-Box ne ha 6 perche' la sua spirale e' piu' grande).
Carico dal centro verso l'esterno (Rondinax): la pellicola appoggia sul dorso delle costole.

v5: tre denti (uno piu' largo: la flangia entra in una sola posizione), piste speculari sui due lati del tubo
(le due flange si bloccano ruotandole in versi opposti), linguetta a scatto, tre tacche di trascinamento."""
import math
import cadquery as cq
from common import *

D = PITCH / 2.0            # incremento di raggio per mezzo giro (spirale 135)
HUB_R = SLEEVE_R + 0.3     # v6.2: mozzo della flangia appena oltre il manicotto (era RIB_R0 - RIB_W = 22.2, il raggio del tamburo)
COLLAR_R, COLLAR_T = TUBE_R + 1.0, 1.2              # collarini del tubo ai lati della finestra della fascetta
STRAP_WIN_H = 18.0                                  # altezza delle due finestre: il perno della fascetta e' lungo 15,8
                                                    # (nella Lab-Box 19; qui 18 per lasciare gioco ai manicotti delle flange 135)
COLLAR_Z = STRAP_WIN_H / 2 + 0.1                    # faccia interna dei collarini, appena oltre le finestre
STRAP_SLOT_W, STRAP_SLOT_ANG = 2.5, 30.0            # finestra stretta (ne esce la fascetta; 2,5 come nella Lab-Box) e posizione angolare
STRAP_WIN_W = 3.6                                   # finestra larga, dal lato opposto (+180 gradi): ci passa il perno del capo
                                                    # (nella Lab-Box 3,43; un po' di piu' perche' le fessure stampate vengono strette)
STRAP_T = 0.15                                      # spessore della fascetta Lab-Box (misurato)
STRAP_PIN_T, STRAP_PIN_L = 3.25, 15.8               # piolino sul capo della fascetta Lab-Box: spessore e lunghezza (misurati)
BUMP_TIP, BUMP_LEN = 0.35, 1.9                      # dentino della linguetta: distanza dall'estremita' libera e lunghezza
RING_FING = (1.0, 8.8)                              # linguetta dell'anello alette: radice ed estremita' (dalla faccia piana)
DET_MARGIN = 0.6                                    # gola dello scatto: margine assiale oltre il dentino

def det_spans():
    """Tratti della gola dello scatto sul lato A (z < 0), solo dove arriva il dentino a blocco inserito: flangia (e provino)
    in ognuna delle tre stazioni, anello alette nella stazione 120 (lo usa a 35 mm) e nella terza. Tratti sovrapposti uniti.
    v6.2: prima la gola correva lungo tutto il lato, fin quasi al centro del tubo."""
    raw = []
    fl = DISC_T + SLEEVE_L - LUG_AX / 2                 # flangia: dall'asse del dente all'estremita' libera della linguetta (verso il centro)
    for zs in (-abs(lug_z("135")), -abs(lug_z("120")), -ring_station("120")):
        raw.append((zs + fl - BUMP_TIP - BUMP_LEN, zs + fl - BUMP_TIP))
    e = RING_LUG - RING_FING[1]                         # anello (montato girato): la linguetta va verso l'estremita' del tubo
    for fmt in ("135", "120"):
        zs = -ring_station(fmt)
        raw.append((zs + e + BUMP_TIP, zs + e + BUMP_TIP + BUMP_LEN))
    out = []
    for a, b in sorted((a - DET_MARGIN, b + DET_MARGIN) for a, b in raw):
        if out and a <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], b))
        else:
            out.append((a, b))
    return out

LEAD_HT = 4                # la spirale 135 regolare comincia dal mezzo giro 4 (raggio 27): come nella Lab-Box, dal giro piu'
                           # interno la costola arriva direttamente al mozzo con una sola curva di mezzo giro, e al centro
                           # resta uno spazio libero per fascetta e clip

# ---- spirali per formato (v7) ----
# r0: raggio della mezzeria della costola all'inizio della spirale regolare (mezzo giro 0, che parte sull'asse +X e passa
#     per il semipiano y > 0); steps: incremento di raggio a ogni mezzo giro successivo (passo / 2; una lista permette
#     anche un passo variabile). Prima della spirale regolare c'e' l'attacco: mezzo giro che parte dal mozzo.
# v6.2, come la Lab-Box: due coppie di flange, una per formato. La 135 e' identica alla v6.1 (8 giri a passo 2,2).
# La 120 ha giri piu' distanziati: nella Lab-Box il passo del 120 e' 1,38 volte quello del 135 (misurato: 3,08 / 2,23),
# quindi 3,1 mm; 5,5 giri dal raggio 27 contengono circa 1,2 m di pellicola (un rullo 120 e' circa 85 cm).
SPIRALS = {
    "135": dict(r0=(RIB_R0 - RIB_W / 2) + D * LEAD_HT, steps=[D] * (HALF_TURNS - LEAD_HT - 1)),
    "120": dict(r0=27.0, steps=[3.1 / 2] * 10),
}

def spiral_arcs(spec):
    """Mezzi giri regolari: lista di (cx, rho, s) con centro (cx, 0), raggio della mezzeria rho, s = +1 (y > 0) / -1.
    Spirale a due centri generalizzata: i centri si spostano in modo che gli archi si raccordino in tangenza sull'asse X."""
    rho = [spec["r0"]]
    for d in spec["steps"]:
        rho.append(rho[-1] + d)
    cx = [0.0]
    for k in range(1, len(rho)):
        cx.append(cx[-1] + (rho[k] - rho[k - 1]) * (1 if k % 2 == 1 else -1))
    return [(cx[k], rho[k], 1 if k % 2 == 0 else -1) for k in range(len(rho))]

def _arc3(arc, off):
    """(inizio, meta', fine) del mezzo giro a offset radiale off."""
    cx, rho, s = arc
    r = rho + off
    return (cx + r * s, 0.0), (cx, r * s), (cx - r * s, 0.0)

def _arc_pts(k, off):
    """Compatibilita' con la v6: mezzo giro k (assoluto) della spirale 135."""
    return _arc3(spiral_arcs(SPIRALS["135"])[k - LEAD_HT], off)

def _lead_r(t, r_b):
    """Raggio della mezzeria della curva d'attacco per t da 0 (sul mozzo) a 1 (inizio della spirale regolare, raggio r_b)."""
    r_a = HUB_R - 0.05 + RIB_W / 2                            # la costola nasce attaccata al mozzo della flangia
    return r_a + (r_b - r_a) * (3 * t * t - 2 * t ** 3)

def rib_2d(fmt="135"):
    """Pianta della costola: attacco di mezzo giro dal mozzo (da -X a +X passando per y < 0), poi la spirale regolare."""
    arcs = spiral_arcs(SPIRALS[fmt])
    o, i = RIB_W / 2, -RIB_W / 2
    n = 48
    r_b = arcs[0][1]
    lead = lambda off: [((_lead_r(k / n, r_b) + off) * math.cos(math.pi + math.pi * k / n),
                         (_lead_r(k / n, r_b) + off) * math.sin(math.pi + math.pi * k / n)) for k in range(n + 1)]
    lo, li = lead(o), lead(i)
    lo[-1] = _arc3(arcs[0], o)[0]; li[-1] = _arc3(arcs[0], i)[0]
    w = cq.Workplane("XY").moveTo(*lo[0]).spline(lo[1:], includeCurrent=True)
    for a in arcs:
        _, m, e = _arc3(a, o); w = w.threePointArc(m, e)
    w = w.lineTo(*_arc3(arcs[-1], i)[2])
    for a in reversed(arcs):
        s_, m, _ = _arc3(a, i); w = w.threePointArc(m, s_)
    w = w.spline(list(reversed(li[:-1])), includeCurrent=True)
    return w.close()

def spiral_length(fmt):
    """Pellicola che la spirale regolare puo' contenere (mm): appoggiata sul dorso delle costole."""
    return sum(math.pi * (rho + RIB_W / 2 + 0.07) for _, rho, _ in spiral_arcs(SPIRALS[fmt]))

def _lug(rb, z0, z1):
    """Profilo (r, z) del dente a V con fianchi a 45 gradi paralleli a quelli della gola; z0..z1 = tratto sul foro."""
    zc = (z0 + z1) / 2
    return [(rb + 0.7, z0), (rb + 0.7, z1), (rb, z1), (rb - LUG_H, zc + 0.2), (rb - LUG_H, zc - 0.2), (rb, z0)]

def _bore(rb, h):
    """Foro passante con smussi d'invito alle due estremita' (compensano anche la 'zampa d'elefante' del primo strato)."""
    b = cq.Workplane("XY").circle(rb).extrude(h)
    b = b.union(cq.Workplane("XY").add(cq.Solid.makeCone(rb + 0.5, rb, 0.5)))
    return b.union(cq.Workplane("XY").add(cq.Solid.makeCone(rb, rb + 0.5, 0.5)).translate((0, 0, h - 0.5)))

def lug_width(i, ring=False):
    """Larghezza angolare del dente i: il dente 0 delle flange e' la chiave; gli anelli alette hanno tre denti stretti."""
    return LUG_KEY_DEG if (i == 0 and not ring) else LUG_DEG

def _lugs(rb, z0, z1, ring=False):
    out = None
    for i, a in enumerate(LUG_ANG):
        w = lug_width(i, ring)
        s = sector(_lug(rb, z0, z1), a - w / 2, a + w / 2)
        out = s if out is None else out.union(s)
    return out

def _finger(rb, z0, z1, ang, bump=True, steep=-1):
    """Linguetta a scatto assiale (radice a z0, estremita' libera a z1) ricavata nel manicotto, con dentino verso il tubo.
    Restituisce (tasca da togliere, linguetta da aggiungere, dentino). Rampe assiali a 45 gradi (infilaggio).
    v6.2: in senso tangenziale il dentino e' asimmetrico: dal lato dello sblocco (steep = -1: verso -Y locale, flange e
    provino; +1: anelli alette) il fianco e' ripido (DET_STEEP), dall'altro a 45 gradi. Bloccando si sente il click; per
    sbloccare il dentino deve risalire il fianco ripido della gola: il reel non si svita da solo."""
    rm = rb + FING_T / 2
    dp = math.degrees((FING_HALF + FING_SLIT) / rm)
    df = math.degrees(FING_HALF / rm)
    pocket = sector([(rb - 0.6, z0), (rb + FING_T + FING_GAP, z0), (rb + FING_T + FING_GAP, z1 + 0.02), (rb - 0.6, z1 + 0.02)],
                    -dp, dp)
    beam = sector([(rb, z0), (rb + FING_T, z0), (rb + FING_T, z1), (rb, z1)], -df, df)
    zb1 = z1 - BUMP_TIP; zb0 = zb1 - BUMP_LEN               # dentino vicino all'estremita' libera
    h = BUMP_H + 0.15
    ys = -0.12 - h / math.tan(math.radians(DET_STEEP))           # fianco ripido (lato sblocco)
    prof = [(rb + 0.15, ys), (rb + 0.15, 0.10 + h), (rb - BUMP_H, 0.10), (rb - BUMP_H, -0.12)]
    if steep > 0:
        prof = [(x, -y) for x, y in prof][::-1]
    tang = (cq.Workplane("XY").polyline(prof).close().extrude(zb1 - zb0).translate((0, 0, zb0)))
    axial = (cq.Workplane("XZ").polyline([(rb + 0.15, zb0), (rb - BUMP_H, zb0 + BUMP_H + 0.15), (rb - BUMP_H, zb1 - BUMP_H - 0.15),
                                          (rb + 0.15, zb1)]).close().extrude(2.0, both=True))
    bmp = tang.intersect(axial)
    rot = lambda w: w.rotate((0, 0, 0), (0, 0, 1), ang)
    return rot(pocket), rot(beam.union(bmp) if bump else beam), rot(bmp)

def _marks(n):
    """Segni incisi sulla faccia esterna (sul piatto): triangolo che punta al dente chiave + n puntini (1 = A, 2 = B)."""
    r0 = TUBE_R + 0.9                                            # v6.2: mozzo piu' piccolo, segni piu' vicini al foro
    m = cq.Workplane("XY").polyline([(r0, 0), (r0 + 2.5, 1.5), (r0 + 2.5, -1.5)]).close().extrude(0.4)
    for k in range(n):
        m = m.union(cq.Workplane("XY").center(r0 + 1.3, 0).rect(1.2, 1.2).extrude(0.4)
                    .rotate((0, 0, 0), (0, 0, 1), 16.0 + 8.0 * k))
    return m

def flange(mirror=False, fit=REEL_FIT, bump=True, fmt="135"):
    """Flangia aperta. Orientamento di stampa: faccia esterna sul piatto (z=0), costole e manicotto in alto.
    Mozzo + corona + razze; la costola a spirale parte dal piatto (niente ponti) e sporge RIB_H oltre le razze.
    Tutto il resto del disco e' vuoto: il liquido entra tra i giri di pellicola dalle due facce.
    mirror=True: flangia B (spirale speculare). Denti, linguetta a scatto e segni NON sono specchiati: nelle
    coordinate di stampa sono uguali sulle due flange, e sul tubo i due lati sono ruotati di 180 gradi uno rispetto all'altro."""
    r_hub, r_rim = HUB_R, FLANGE_R - RIM_W
    f = cq.Workplane("XY").circle(r_hub).extrude(DISC_T)
    f = f.union(cq.Workplane("XY").circle(FLANGE_R).circle(r_rim).extrude(DISC_T))
    spoke = (cq.Workplane("XY").box(r_rim - r_hub + 1.0, SPOKE_W, DISC_T, centered=(False, True, False))
             .translate((r_hub - 0.5, 0, 0)))
    for k in range(SPOKE_N):
        f = f.union(spoke.rotate((0, 0, 0), (0, 0, 1), 360.0 / SPOKE_N * k))
    f = f.union(rib_2d(fmt).extrude(DISC_T + RIB_H))
    f = f.union(cq.Workplane("XY").circle(SLEEVE_R).extrude(SLEEVE_L).translate((0, 0, DISC_T)))
    if mirror:
        f = f.mirror("XZ")
    rb = TUBE_R + fit
    f = f.cut(_bore(rb, DISC_T + SLEEVE_L))
    f = f.union(_lugs(rb, 0.0, LUG_AX))                          # denti della baionetta (a V, nascono dal piatto)
    pocket, finger, _ = _finger(rb, DISC_T, DISC_T + SLEEVE_L, DET_ANG, bump)
    f = f.cut(pocket).union(finger)
    return f.cut(_marks(2 if mirror else 1))

def fin_ring(side="A", fit=REEL_FIT, bump=True):
    """Anello con alette inclinate: si infila sul tubo fuori dalla flangia e si blocca a baionetta nella stazione
    subito all'esterno (a 35 mm la stazione 120, a 120 la terza stazione).
    Orientamento di stampa: lato flangia sul piatto (z=0). A e B hanno le alette speculari: con la spirale che gira
    nel verso dell'avvolgimento, A sul lato A e B sul lato B spingono il liquido verso l'interno.
    Tre denti stretti (entra in qualunque delle tre posizioni) e linguetta a scatto."""
    h = FIN_H
    rb = TUBE_R + fit
    tau = math.radians(90.0 - FIN_PITCH)                         # inclinazione dalla verticale
    th, sh = FIN_T / math.cos(tau), h * math.tan(tau)            # spessore orizzontale, spostamento in cima
    g = cq.Workplane("XY").circle(SLEEVE_R).extrude(h)
    vane = (cq.Workplane("YZ").polyline([(-th / 2, 0), (th / 2, 0), (th / 2 + sh, h), (-th / 2 + sh, h)]).close()
            .extrude(FIN_R - 0.8 - 10.0).translate((10.0, 0, 0)))
    trim = (cq.Workplane("XZ").polyline([(30.0, h + 0.01), (FIN_R + 1, 7.0), (FIN_R + 1, h + 1), (30.0, h + 1)]).close()
            .extrude(40, both=True))
    vane = vane.cut(trim)                                        # bordo superiore che scende verso l'esterno
    step = 360.0 / FIN_N
    for k in range(FIN_N):                                       # piede dell'aletta a meta' di un vano tra due razze
        g = g.union(vane.rotate((0, 0, 0), (0, 0, 1), 180.0 / SPOKE_N + step * k))
    g = g.union(cq.Workplane("XY").circle(FIN_R).circle(FIN_R - 1.6).extrude(5.0))           # cerchio esterno, sopra la corona
    if side != "A":
        g = g.mirror("XZ")
    g = g.cut(_bore(rb, h))
    zc = RING_LUG
    g = g.union(_lugs(rb, zc - LUG_AX / 2, zc + LUG_AX / 2, ring=True))
    pocket, finger, _ = _finger(rb, RING_FING[0], RING_FING[1], -DET_ANG, bump, steep=+1)
    z1 = RING_FING[1]
    open_top = sector([(rb - 0.6, z1), (rb + FING_T + FING_GAP, z1), (rb + FING_T + FING_GAP, h + 0.02), (rb - 0.6, h + 0.02)],
                      -DET_ANG - math.degrees((FING_HALF + FING_SLIT) / (rb + FING_T / 2)),
                      -DET_ANG + math.degrees((FING_HALF + FING_SLIT) / (rb + FING_T / 2)))
    return g.cut(pocket).cut(open_top).union(finger)

def fit_gauge(fits=(0.10, 0.06, 0.02)):
    """Provino: tre fori con denti e linguetta, a gioco decrescente (1, 2, 3 tacche), da provare sul tubo gia' stampato."""
    h, pitch = DISC_T + SLEEVE_L, 2 * SLEEVE_R + 3.0
    g = cq.Workplane("XY").box(pitch * (len(fits) - 1), 20.0, h, centered=(True, True, False))
    for i, ft in enumerate(fits):
        x = pitch * (i - (len(fits) - 1) / 2)
        g = g.union(cq.Workplane("XY").circle(SLEEVE_R).extrude(h).translate((x, 0, 0)))
    for i, ft in enumerate(fits):
        x = pitch * (i - (len(fits) - 1) / 2); rb = TUBE_R + ft
        g = g.cut(_bore(rb, h).translate((x, 0, 0)))
        g = g.union(_lugs(rb, 0.0, LUG_AX).translate((x, 0, 0)))
        pocket, finger, _ = _finger(rb, DISC_T, h, DET_ANG)
        g = g.cut(pocket.translate((x, 0, 0))).union(finger.translate((x, 0, 0)))
        for n in range(i + 1):                                   # tacche di riconoscimento sul bordo
            g = g.cut(cq.Workplane("XY").box(1.2, 1.6, h, centered=(True, False, False))
                      .translate((x + 3.0 * (n - i / 2), -SLEEVE_R, 0)))
    return g

def _tube_cuts_A():
    """Tutte le lavorazioni del lato A (z<0): canali, gole a V, tacche di trascinamento, gola dello scatto.
    Il lato B si ottiene ruotando queste di 180 gradi attorno all'asse X: le piste risultano speculari (blocco in verso opposto)."""
    rf, ro = TUBE_R - 1.4, TUBE_R + 0.3
    z35, z120, z3 = -abs(lug_z("135")), -abs(lug_z("120")), -ring_station("120")
    zin = z35 + (LUG_AX / 2 + 0.2)                               # fondo del canale (oltre la stazione 35)
    zend = -TUBE_HALF
    cuts = None
    def add(s):
        nonlocal cuts
        cuts = s if cuts is None else cuts.union(s)
    for i, a in enumerate(LUG_ANG):
        cw = CHAN_KEY_DEG if i == 0 else CHAN_DEG
        lw = LUG_KEY_DEG if i == 0 else LUG_DEG
        ac = a - LOCK_DEG                                        # centro del canale d'infilaggio
        add(sector([(rf, zend - 0.01), (ro, zend - 0.01), (ro, zin), (rf, zin)], ac - cw / 2, ac + cw / 2))
        for zs in (z35, z120, z3):                               # gole a V: il dente ruota fino alla battuta
            vee = [(ro, zs - 1.8), (ro, zs + 1.8), (rf, zs + 0.1), (rf, zs - 0.1)]
            add(sector(vee, ac - cw / 2, a + lw / 2 + 0.6))
    for k in range(3):                                           # tacche di trascinamento
        n = (cq.Workplane("XY").box(6.0, 6.0, 4.0, centered=(False, True, False))
             .translate((TUBE_BORE - 1, 0, zend - 0.01)))
        add(n.rotate((0, 0, 0), (0, 0, 1), NOTCH_ANG + 120.0 * k))
    r0 = TUBE_R                                                  # gola dello scatto: v6.2 solo nei tratti dove arriva il dentino
    hh = DET_DEPTH + 0.3                                         # v6.2: fianco ripido dal lato dello sblocco (-Y), 45 gradi dall'altro
    prof = [(r0 + 0.3, -0.22 - hh / math.tan(math.radians(DET_STEEP))), (r0 + 0.3, 0.20 + hh), (r0 - DET_DEPTH, 0.20), (r0 - DET_DEPTH, -0.22)]
    for z0, z1 in det_spans():
        det = cq.Workplane("XY").polyline(prof).close().extrude(z1 - z0).translate((0, 0, z0))
        add(det.rotate((0, 0, 0), (0, 0, 1), DET_ANG))
    return cuts

def tube():
    """Tubo centrale (asse Z, centrato): piste a baionetta alle due estremita', tre stazioni per lato:
    flange a 35 mm, flange a 120 (= anelli alette a 35 mm), anelli alette a 120.
    v6.2, come nella Lab-Box: in mezzo, tra due collarini che tengono centrata la fascetta mentre si avvolge sul tubo, il
    passaggio della fascetta va da parte a parte: da un lato una finestra larga (ci passa il perno del capo: la fascetta
    ripiegata attorno a uno spezzone di filamento), dal lato opposto una finestra stretta da cui esce solo la fascetta; il
    perno resta dentro, contro la finestra stretta. Il tubo e' uguale girato da un capo all'altro (rotazione di 180 gradi
    attorno a X), salvo le finestre."""
    t = cq.Workplane("XY").circle(TUBE_R).circle(TUBE_BORE).extrude(2 * TUBE_HALF).translate((0, 0, -TUBE_HALF))
    cham = cq.Workplane("XZ").polyline([(TUBE_R - 0.5, -TUBE_HALF - 0.01), (TUBE_R + 0.2, -TUBE_HALF - 0.01),
                                        (TUBE_R + 0.2, -TUBE_HALF + 0.7)]).close().revolve(360, (0, 0, 0), (0, 1, 0))
    ca = _tube_cuts_A().union(cham)
    cb = ca.rotate((0, 0, 0), (1, 0, 0), 180)
    t = t.cut(ca).cut(cb)
    for z0, z1 in ((-COLLAR_Z - COLLAR_T, -COLLAR_Z), (COLLAR_Z, COLLAR_Z + COLLAR_T)):      # collarini, smussati a 45 gradi sotto
        prof = [(TUBE_R - 0.01, z0 - (COLLAR_R - TUBE_R)), (COLLAR_R, z0), (COLLAR_R, z1), (TUBE_R - 0.01, z1)]
        t = t.union(cq.Workplane("XZ").polyline(prof).close().revolve(360, (0, 0, 0), (0, 1, 0)))
    win = lambda w: (cq.Workplane("XY").box(TUBE_R - TUBE_BORE + 2.0, w, STRAP_WIN_H, centered=(False, True, True))
                     .translate((TUBE_BORE - 1.0, 0, 0)))
    slot = win(STRAP_SLOT_W)                                                                       # finestra stretta: esce la fascetta
    wide = win(STRAP_WIN_W).rotate((0, 0, 0), (0, 0, 1), 180.0)                                    # finestra larga, dal lato opposto
    return t.cut(slot.union(wide).rotate((0, 0, 0), (0, 0, 1), STRAP_SLOT_ANG))

def fin_rings_assembled(fmt="135", mode="alternata"):
    """I due anelli alette nel riferimento della spirale, per il formato indicato: [lato A, lato B].
    A 35 mm usano le stazioni 120 (libere); a 120 usano la terza stazione, la piu' esterna.
    mode="alternata": lo stesso anello (A) sui due lati -> un lato spinge dentro e l'altro tira fuori, e i ruoli
                      si scambiano a ogni inversione (flusso che attraversa la pellicola da un bordo all'altro).
    mode="unico":     anelli speculari (A e B) -> per il verso dell'avvolgimento spingono dentro tutti e due."""
    z = ring_face_y(fmt)
    a_side = fin_ring("A").rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, -z))
    b_side = fin_ring("A" if mode == "alternata" else "B").translate((0, 0, z))
    return [a_side, b_side]

def assembled(fmt):
    """Le tre parti nel riferimento della spirale (asse Z, centro nell'origine): flangia A, tubo, flangia B."""
    zo = reel_width(fmt) / 2 + DISC_T
    sp = fmt if fmt in SPIRALS else "135"
    fa = flange(fmt=sp).translate((0, 0, -zo))
    fb = flange(mirror=True, fmt=sp).rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, zo))
    return [fa, tube(), fb]
