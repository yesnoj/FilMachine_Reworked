"""Torretta: guance, ponte (fessura pellicola + fessura lama), taglierina a pendolo, supporto servo, culle 135/120."""
import math
import cadquery as cq
from common import *

def ybox(x0, x1, y0, y1, z0, z1):
    return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))

def yhole(x, z, r, y0, y1):
    """Foro cilindrico parallelo a Y."""
    return cq.Workplane("XZ").center(x, z).circle(r).extrude(-(y1 - y0)).translate((0, y0, 0))

def arm_stop_xz():
    """Centro della vite di fermo: tiene la suola del braccio guida a SOLE_STOP_R quando la spirale e' vuota."""
    import layout
    phi = math.radians(layout.arm_angle(SOLE_STOP_R))
    u = (math.cos(phi), math.sin(phi)); n = (-math.sin(phi), math.cos(phi))
    return (PIV_X + 3.0 * u[0] - 6.5 * n[0], ZF + 3.0 * u[1] - 6.5 * n[1])

CHEEK_HOLES_TAP = [(PIV_X, ZF), arm_stop_xz()]                    # perno braccio guida e fermo (viti M3x8)
CHEEK_HOLES_CLR = [(-74.5, 88.0), (-74.5, 94.0),                  # ponte
                   (-107.5, 62.0), (-80.5, 62.0),                 # culla 135 / culla 120
                   (SB_X0 + 3.0, SERVO_Z - 7.0), (SB_X0 + 3.0, SERVO_Z + 7.0)]   # supporto servo

def cheek(side=1):
    c = ybox(CHEEK_X0, CHEEK_X1, CHEEK_IN, CHEEK_IN + CHEEK_T, 0, CHEEK_ZTOP)
    c = c.union(ybox(CHEEK_X0, CHEEK_X1, CHEEK_IN + CHEEK_T, CHEEK_IN + CHEEK_T + 12, 0, 4))   # piede
    c = c.cut(ybox(-104, -82, CHEEK_IN, CHEEK_IN + CHEEK_T, 32, 52))                           # alleggerimento
    c = c.cut(ybox(KNIFE_X - 0.6, KNIFE_X + 0.6, CHEEK_IN, CHEEK_IN + 4.2, 64, 86))            # tasca cieca: dorso lama in parcheggio
    for x, z in CHEEK_HOLES_TAP:
        c = c.cut(yhole(x, z, M3_TAP, CHEEK_IN, CHEEK_IN + CHEEK_T))
    for x, z in CHEEK_HOLES_CLR:
        c = c.cut(yhole(x, z, M3_CLR, CHEEK_IN, CHEEK_IN + CHEEK_T))
    for x in (-104.0, -60.0):                                                                  # fori piede
        c = c.cut(cq.Workplane("XY").center(x, CHEEK_IN + CHEEK_T + 6).circle(M3_CLR).extrude(4))
    return c if side > 0 else c.mirror("XZ")

def bridge():
    b = ybox(BR_X0, BR_X1, -CHEEK_IN, CHEEK_IN, BR_Z0, BR_Z1)
    b = b.cut(ybox(BR_X0, BR_X1, -32, 32, SLOT_Z0, SLOT_Z1))                      # fessura pellicola
    b = b.cut(ybox(KNIFE_X - 0.5, KNIFE_X + 0.5, -CHEEK_IN, 34.1, BR_Z0, BR_Z1 - 3.5))  # fessura lama: cieca in alto, aperta sul lato parcheggio
    b = b.cut(ybox(BR_X0, BR_X1, -12, 12, SLOT_Z0 - 0.5, ZF + 2.8))               # passaggio clip + fascetta
    lead = cq.Workplane("XZ").polyline([(BR_X0, SLOT_Z0 - 1.5), (BR_X0 + 2.5, SLOT_Z0),
                                        (BR_X0 + 2.5, SLOT_Z1), (BR_X0, SLOT_Z1 + 1.5)]).close().extrude(32, both=True)
    b = b.cut(lead)                                                               # invito lato caricatore
    for s in (1, -1):
        for z in (88.0, 94.0):
            y0, y1 = (CHEEK_IN - 10, CHEEK_IN) if s > 0 else (-CHEEK_IN, -CHEEK_IN + 10)
            b = b.cut(yhole(-74.5, z, M3_TAP, y0, y1))
    return b

def _swing(shape, alpha):
    """Ruota attorno all'asse del servo (parallelo a X): alpha>0 porta la punta verso +Y."""
    return shape.rotate((0, 0, SERVO_Z), (1, 0, SERVO_Z), -alpha)

def _yz(pts, x0, x1):
    return (cq.Workplane("YZ").polyline([(y, SERVO_Z + r) for y, r in pts]).close()
            .extrude(x1 - x0).translate((x0, 0, 0)))

CLAMP_R, CLAMP_Y = 41.0, (-11.3, 2.3)       # raggio e posizioni delle due viti del morsetto lama

def swing_arm(alpha=0.0):
    """Braccio lama: mozzo sulla squadretta del servo, sede radiale per lama 9 mm (filo sull'asse radiale, verso +Y)."""
    a = _yz([(-6, 0), (-6, 34), (-13.8, 36.5), (-13.8, 46), (-10.5, 48.5), (-10.5, 56), (1.5, 56), (1.5, 48.5),
             (4.8, 46), (4.8, 36.5), (6, 34), (6, 0)], SWA_X0, SWA_X1)
    a = a.union(cq.Workplane("YZ").center(0, SERVO_Z).circle(8.0).extrude(SWA_T).translate((SWA_X0, 0, 0)))
    a = a.cut(_yz([(-BLADE_W - 0.15, 10), (0.15, 10), (0.15, 57), (-BLADE_W - 0.15, 57)], SWA_X1 - 0.3, SWA_X1))  # sede lama
    a = a.cut(cq.Workplane("YZ").center(0, SERVO_Z).circle(3.7).extrude(SWA_T).translate((SWA_X0, 0, 0)))        # accesso vite squadretta
    a = a.cut(_yz([(-3.2, 0), (3.2, 0), (3.2, 22), (-3.2, 22)], SWA_X0, SWA_X0 + 1.8))                           # sede squadretta
    for y in CLAMP_Y:
        a = a.cut(cq.Workplane("YZ").center(y, SERVO_Z + CLAMP_R).circle(M3_TAP).extrude(SWA_T).translate((SWA_X0, 0, 0)))
    return _swing(a, alpha)

def blade_cap(alpha=0.0):
    k = _yz([(-13.8, 36.5), (4.8, 36.5), (4.8, 46), (-13.8, 46)], SWA_X1 + 0.1, SWA_X1 + 2.6)
    for y in CLAMP_Y:
        k = k.cut(cq.Workplane("YZ").center(y, SERVO_Z + CLAMP_R).circle(M3_CLR).extrude(3).translate((SWA_X1, 0, 0)))
    return _swing(k, alpha)

def blade(alpha=0.0):
    """Sagoma della lama da cutter 9 mm: filo sul raggio (y=0), punta a SWING_R dall'asse."""
    b = _yz([(-BLADE_W, 30), (0, 30), (0, SWING_R), (-BLADE_W, SWING_R - BLADE_W * math.tan(math.radians(30)))],
            KNIFE_X - BLADE_T / 2, KNIFE_X + BLADE_T / 2)
    return _swing(b, alpha)

def servo_bracket():
    s = ybox(SB_X0, SB_X1, -CHEEK_IN, CHEEK_IN, SERVO_Z - 10.5, SERVO_Z + 9.5)
    s = s.cut(ybox(SB_X0, SB_X1, -6.2, 17.0, SERVO_Z - 6.3, SERVO_Z + 6.3))       # finestra corpo SG90/MG90S
    for y in (5.4 - 13.9, 5.4 + 13.9):
        s = s.cut(cq.Workplane("YZ").center(y, SERVO_Z).circle(0.85).extrude(6).translate((SB_X0, 0, 0)))
    for sg in (1, -1):
        for z in (SERVO_Z - 7.0, SERVO_Z + 7.0):
            y0, y1 = (CHEEK_IN - 10, CHEEK_IN) if sg > 0 else (-CHEEK_IN, -CHEEK_IN + 10)
            s = s.cut(yhole(SB_X0 + 3.0, z, M3_TAP, y0, y1))
    return s

def cradle135():
    k = ybox(-110, -77.5, -CHEEK_IN, CHEEK_IN, 56.5, 80.5)
    seat = (cq.Workplane("XZ").center(CAS_X - 2, CAS_Z).slot2D(4 + 2 * CAS_R, 2 * CAS_R, 0)
            .extrude(22.7, both=True))
    top = ybox(-110, CAS_X, -22.7, 22.7, CAS_Z, 81)                               # apertura alto/retro
    hub = (cq.Workplane("XZ").center(CAS_X - 2, CAS_Z).slot2D(4 + 14, 14, 0).extrude(27.5, both=True)
           .union(ybox(CAS_X - 11, CAS_X + 7, -27.5, 27.5, CAS_Z, 81)))           # scarico codoli rocchetto
    lip = ybox(CAS_X, -77.5, -20, 20, ZF - 4.0, 81)                               # uscita labbra feltrate
    k = k.cut(seat).cut(top).cut(hub).cut(lip)
    for sg in (1, -1):
        for x in (-107.5, -80.5):
            y0, y1 = (CHEEK_IN - 10, CHEEK_IN) if sg > 0 else (-CHEEK_IN, -CHEEK_IN + 10)
            k = k.cut(yhole(x, 62.0, M3_TAP, y0, y1))
    return k

def cradle120():
    """Culla per il rullo 120 (stile Rondinax 60): il rocchetto appoggia nel semicilindro, la pellicola
    esce dall'alto verso il ponte, la carta si sfila verso l'alto/indietro."""
    RX, RZ, RR = CAS_X, ZF - 7.5, 12.8
    k = ybox(-110, -77.5, -CHEEK_IN, CHEEK_IN, 56.5, 90.0)
    seat = cq.Workplane("XZ").center(RX - 1.5, RZ).slot2D(3 + 2 * RR, 2 * RR, 0).extrude(32.8, both=True)
    top = ybox(-110 + 3.2, RX, -32.8, 32.8, RZ, 90.0)                              # apertura per inserire il rullo
    out = ybox(RX, -77.5, -32.8, 32.8, ZF - 2.0, 90.0)                             # uscita pellicola verso il ponte
    k = k.cut(seat).cut(top).cut(out)
    for sg in (1, -1):
        for x in (-107.5, -80.5):
            y0, y1 = (CHEEK_IN - 10, CHEEK_IN) if sg > 0 else (-CHEEK_IN, -CHEEK_IN + 10)
            k = k.cut(yhole(x, 62.0, M3_TAP, y0, y1))
    return k
