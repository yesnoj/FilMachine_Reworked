"""Spirale regolabile 35/120: due flange a spirale che si bloccano a baionetta, in due posizioni,
su un tubo centrale; tamburo centrale con ancoraggio della fascetta e tasca della clip.
Carico dal centro verso l'esterno (Rondinax): la pellicola appoggia sul dorso delle costole."""
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

def flange(mirror=False):
    """Orientamento di stampa: faccia esterna sul piatto (z=0), costole e manicotto verso l'alto."""
    f = cq.Workplane("XY").circle(FLANGE_R).extrude(DISC_T)
    f = f.union(rib_2d().extrude(RIB_H).translate((0, 0, DISC_T)))
    f = f.union(cq.Workplane("XY").circle(SLEEVE_R).extrude(SLEEVE_L).translate((0, 0, DISC_T)))
    w0, w1 = CORE_R + 4.0, FLANGE_R - 2.5
    slot = cq.Workplane("XY").center((w0 + w1) / 2, 0).slot2D(w1 - w0, 6.0, 0).extrude(DISC_T)
    for a in range(6):                                           # finestre per i liquidi (le costole fanno ponte)
        f = f.cut(slot.rotate((0, 0, 0), (0, 0, 1), 30 + 60 * a))
    rb = TUBE_R + FIT
    f = f.cut(cq.Workplane("XY").circle(rb).extrude(DISC_T + SLEEVE_L))
    lug = [(rb + 0.2, 0.0), (rb + 0.2, LUG_AX), (rb - LUG_H, LUG_AX / 2 + 0.2), (rb - LUG_H, LUG_AX / 2 - 0.2)]
    for a in (0.0, 180.0):                                       # denti della baionetta (a V, nascono dal piatto)
        f = f.union(sector(lug, a - LUG_DEG / 2, a + LUG_DEG / 2))
    return f.mirror("XZ") if mirror else f

def tube():
    """Tubo centrale (asse Z, centrato): piste a baionetta alle due estremita', due stazioni per lato."""
    t = cq.Workplane("XY").circle(TUBE_R).circle(TUBE_BORE).extrude(2 * TUBE_HALF).translate((0, 0, -TUBE_HALF))
    rf, ro = TUBE_R - 1.4, TUBE_R + 0.3
    for sg in (-1, 1):                                           # lato A (z<0) e lato B (z>0), speculari
        z35, z120 = sg * abs(lug_z("135")), sg * abs(lug_z("120"))
        zin = z35 - sg * (LUG_AX / 2 + 0.2)                      # fondo del canale (oltre la stazione 35)
        zend = sg * TUBE_HALF
        for a in (0.0, 180.0):
            ac = a - LOCK_DEG                                    # centro del canale d'infilaggio
            chan = [(rf, min(zin, zend)), (ro, min(zin, zend)), (ro, max(zin, zend)), (rf, max(zin, zend))]
            t = t.cut(sector(chan, ac - CHAN_DEG / 2, ac + CHAN_DEG / 2))
            for zs in (z35, z120):                               # gole a V: il dente ruota fino alla battuta
                vee = [(ro, zs - 1.8), (ro, zs + 1.8), (rf, zs + 0.1), (rf, zs - 0.1)]
                t = t.cut(sector(vee, ac - CHAN_DEG / 2, a + LUG_DEG / 2 + 0.6))
        for a in (90.0, 270.0):                                  # tacche di trascinamento
            n = cq.Workplane("XY").box(6.0, 6.0, 4.0, centered=(False, True, False)) \
                .translate((TUBE_BORE - 1, 0, zend - 4.0 if sg > 0 else zend))
            t = t.cut(n.rotate((0, 0, 0), (0, 0, 1), a))
    for a in (120.0, 240.0):                                     # sedi cieche delle spine del tamburo
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

def assembled(fmt):
    """Le quattro parti nel riferimento della spirale (asse Z, centro nell'origine): A, tubo, tamburo, B."""
    zo = reel_width(fmt) / 2 + DISC_T
    fa = flange().translate((0, 0, -zo))
    fb = flange(mirror=True).rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, zo))
    return [fa, tube(), drum(), fb]

if __name__ == "__main__":
    import itertools
    cap = sum(math.pi * (RIB_R0 + D * k) for k in range(HALF_TURNS - 2))
    print("capacita' pellicola (mm):", round(cap), "| fascetta 300 gradi:", round(math.radians(300) * CORE_R, 1))
    for fmt in ("135", "120"):
        P = dict(zip(("flangiaA", "tubo", "tamburo", "flangiaB"), assembled(fmt)))
        for n, p in P.items():
            s = p.val(); print(f"  {fmt} {n}: valid={s.isValid()} solids={len(s.Solids())} vol={s.Volume()/1000:.1f}cm3")
        for (na, a), (nb, b) in itertools.combinations(P.items(), 2):
            v = a.val().intersect(b.val()).Volume()
            print(f"  {fmt} {na}/{nb}: interferenza {v:.4f} mm3, distanza minima {a.val().distance(b.val()):.2f} mm")
    P = assembled("135")
    render([P[0], P[1], P[2]], "spirale_v2_aperta", elev=25, azim=-40)
    render([tube()], "tubo_baionetta", elev=20, azim=-60)
    section_plot(cq.Workplane("XY").add(P[0].val()).union(P[1]).union(P[2]).union(P[3]), "sez_spirale_v2_135", [0.0], axis="Y", size=(11, 7))
