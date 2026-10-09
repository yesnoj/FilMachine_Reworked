"""Spirale regolabile 35/120 (v5): due flange a spirale che si bloccano a baionetta, in due posizioni,
su un tubo centrale; tamburo centrale con ancoraggio della fascetta e tasca della clip.
Carico dal centro verso l'esterno (Rondinax): la pellicola appoggia sul dorso delle costole.

v5: tre denti (uno piu' largo: la flangia entra in una sola posizione), piste speculari sui due lati del tubo
(le due flange si bloccano ruotandole in versi opposti), linguetta a scatto, tre tacche di trascinamento."""
import math
import cadquery as cq
from common import *

D = PITCH / 2.0            # incremento di raggio per mezzo giro

def _arc_pts(k, off):
    """Mezzo giro k della spirale a due centri: (inizio, meta', fine) a offset radiale off."""
    cx = 0.0 if k % 2 == 0 else D
    rho = (RIB_R0 - RIB_W / 2) + D * k + off
    s = 1 if k % 2 == 0 else -1
    return (cx + rho * s, 0.0), (cx, rho * s), (cx - rho * s, 0.0)

def rib_2d():
    o, i = RIB_W / 2, -RIB_W / 2
    w = cq.Workplane("XY").moveTo(*_arc_pts(0, o)[0])
    for k in range(HALF_TURNS):
        _, m, e = _arc_pts(k, o); w = w.threePointArc(m, e)
    w = w.lineTo(*_arc_pts(HALF_TURNS - 1, i)[2])
    for k in reversed(range(HALF_TURNS)):
        s, m, _ = _arc_pts(k, i); w = w.threePointArc(m, s)
    return w.close()

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

def _finger(rb, z0, z1, ang, bump=True):
    """Linguetta a scatto assiale (radice a z0, estremita' libera a z1) ricavata nel manicotto, con dentino verso il tubo.
    Restituisce (tasca da togliere, linguetta da aggiungere). Il dentino ha rampe a 45 gradi nei due sensi."""
    rm = rb + FING_T / 2
    dp = math.degrees((FING_HALF + FING_SLIT) / rm)
    df = math.degrees(FING_HALF / rm)
    pocket = sector([(rb - 0.6, z0), (rb + FING_T + FING_GAP, z0), (rb + FING_T + FING_GAP, z1 + 0.02), (rb - 0.6, z1 + 0.02)],
                    -dp, dp)
    beam = sector([(rb, z0), (rb + FING_T, z0), (rb + FING_T, z1), (rb, z1)], -df, df)
    zb1 = z1 - 0.35; zb0 = zb1 - 1.9                       # dentino vicino all'estremita' libera
    tang = (cq.Workplane("XY").polyline([(rb + 0.15, -0.60), (rb + 0.15, 0.60), (rb - BUMP_H, 0.10), (rb - BUMP_H, -0.10)])
            .close().extrude(zb1 - zb0).translate((0, 0, zb0)))
    axial = (cq.Workplane("XZ").polyline([(rb + 0.15, zb0), (rb - BUMP_H, zb0 + BUMP_H + 0.15), (rb - BUMP_H, zb1 - BUMP_H - 0.15),
                                          (rb + 0.15, zb1)]).close().extrude(2.0, both=True))
    bmp = tang.intersect(axial)
    rot = lambda w: w.rotate((0, 0, 0), (0, 0, 1), ang)
    return rot(pocket), rot(beam.union(bmp) if bump else beam), rot(bmp)

def _marks(n):
    """Segni incisi sulla faccia esterna (sul piatto): triangolo che punta al dente chiave + n puntini (1 = A, 2 = B)."""
    r0 = TUBE_R + 1.6
    m = cq.Workplane("XY").polyline([(r0, 0), (r0 + 3.2, 1.7), (r0 + 3.2, -1.7)]).close().extrude(0.4)
    for k in range(n):
        m = m.union(cq.Workplane("XY").center(r0 + 1.6, 0).rect(1.4, 1.4).extrude(0.4)
                    .rotate((0, 0, 0), (0, 0, 1), 14.0 + 7.0 * k))
    return m

def flange(mirror=False, fit=REEL_FIT, bump=True):
    """Flangia aperta. Orientamento di stampa: faccia esterna sul piatto (z=0), costole e manicotto in alto.
    Mozzo + corona + razze; la costola a spirale parte dal piatto (niente ponti) e sporge RIB_H oltre le razze.
    Tutto il resto del disco e' vuoto: il liquido entra tra i giri di pellicola dalle due facce.
    mirror=True: flangia B (spirale speculare). Denti, linguetta a scatto e segni NON sono specchiati: nelle
    coordinate di stampa sono uguali sulle due flange, e sul tubo i due lati sono ruotati di 180 gradi uno rispetto all'altro."""
    r_hub, r_rim = RIB_R0 - RIB_W, FLANGE_R - RIM_W
    f = cq.Workplane("XY").circle(r_hub).extrude(DISC_T)
    f = f.union(cq.Workplane("XY").circle(FLANGE_R).circle(r_rim).extrude(DISC_T))
    spoke = (cq.Workplane("XY").box(r_rim - r_hub + 1.0, SPOKE_W, DISC_T, centered=(False, True, False))
             .translate((r_hub - 0.5, 0, 0)))
    for k in range(SPOKE_N):
        f = f.union(spoke.rotate((0, 0, 0), (0, 0, 1), 360.0 / SPOKE_N * k))
    f = f.union(rib_2d().extrude(DISC_T + RIB_H))
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
    pocket, finger, _ = _finger(rb, 1.0, 8.8, -DET_ANG, bump)
    open_top = sector([(rb - 0.6, 8.8), (rb + FING_T + FING_GAP, 8.8), (rb + FING_T + FING_GAP, h + 0.02), (rb - 0.6, h + 0.02)],
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
    r0 = TUBE_R                                                  # gola dello scatto, lungo tutto il lato
    det = (cq.Workplane("XY").polyline([(r0 + 0.3, -DET_HALF_TOP - 0.33), (r0 + 0.3, DET_HALF_TOP + 0.33),
                                        (r0 - DET_DEPTH, DET_HALF_BOT), (r0 - DET_DEPTH, -DET_HALF_BOT)]).close()
           .extrude(TUBE_HALF - 2.0).translate((0, 0, zend + 0.5)))
    add(det.rotate((0, 0, 0), (0, 0, 1), DET_ANG))
    return cuts

def tube():
    """Tubo centrale (asse Z, centrato): piste a baionetta alle due estremita', tre stazioni per lato:
    flange a 35 mm, flange a 120 (= anelli alette a 35 mm), anelli alette a 120.
    Il tubo e' uguale girato da un capo all'altro (simmetria di rotazione di 180 gradi attorno a X)."""
    t = cq.Workplane("XY").circle(TUBE_R).circle(TUBE_BORE).extrude(2 * TUBE_HALF).translate((0, 0, -TUBE_HALF))
    cham = cq.Workplane("XZ").polyline([(TUBE_R - 0.5, -TUBE_HALF - 0.01), (TUBE_R + 0.2, -TUBE_HALF - 0.01),
                                        (TUBE_R + 0.2, -TUBE_HALF + 0.7)]).close().revolve(360, (0, 0, 0), (0, 1, 0))
    ca = _tube_cuts_A().union(cham)
    cb = ca.rotate((0, 0, 0), (1, 0, 0), 180)
    t = t.cut(ca).cut(cb)
    for a in (120.0, 240.0):                                     # sedi cieche delle spine del tamburo (come nella v4: il tamburo non cambia)
        h = cq.Workplane("YZ").circle(0.95).extrude(TUBE_R + 0.5).translate((TUBE_R - 2.0, 0, 0))
        t = t.cut(h.rotate((0, 0, 0), (0, 0, 1), a))
    return t

def drum():
    """Tamburo centrale: superficie di avvolgimento della fascetta, ancoraggio e tasca (piana) della clip."""
    d = (cq.Workplane("XY").circle(CORE_R).circle(TUBE_R + 0.1).extrude(2 * DRUM_HALF)
         .translate((0, 0, -DRUM_HALF)))
    pocket = (cq.Workplane("XY").box(CORE_R, CLIP_POCKET_T, 2 * DRUM_HALF, centered=(False, True, True))
              .translate((CLIP_FLOOR, 0, 0)).rotate((0, 0, 0), (0, 0, 1), -30))
    pin = cq.Workplane("XY").circle(1.3).extrude(2 * DRUM_HALF).translate((CORE_R - 2.4, 0, -DRUM_HALF))
    slit = cq.Workplane("XY").box(3.5, 0.8, STRAP_W + 1.5, centered=(False, True, True)).translate((CORE_R - 2.4, 0, 0))
    d = d.cut(pocket).cut(pin.union(slit).rotate((0, 0, 0), (0, 0, 1), 30))
    for a in (120.0, 240.0):                                     # spine (filamento 1.75) tamburo-tubo
        h = cq.Workplane("YZ").circle(0.95).extrude(CORE_R).translate((TUBE_R - 2.0, 0, 0))
        d = d.cut(h.rotate((0, 0, 0), (0, 0, 1), a))
    return d

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
    """Le quattro parti nel riferimento della spirale (asse Z, centro nell'origine): A, tubo, tamburo, B."""
    zo = reel_width(fmt) / 2 + DISC_T
    fa = flange().translate((0, 0, -zo))
    fb = flange(mirror=True).rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, zo))
    return [fa, tube(), drum(), fb]
