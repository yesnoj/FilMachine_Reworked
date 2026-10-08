"""Trascinamento e banco: montante motore, montante folle, trascinatore, perno folle, basamento."""
import math
import cadquery as cq
from common import *
from tower import ybox, yhole

def _upright():
    u = ybox(-UP_HALF, UP_HALF, UP_Y, UP_Y + UP_T, 0, HC + 26)
    u = u.union(ybox(-UP_HALF, UP_HALF, UP_Y + UP_T, UP_Y + UP_T + 14, 0, 4))                 # piede
    for sx in (1, -1):                                                                        # fazzoletti
        g = (cq.Workplane("YZ").polyline([(UP_Y + UP_T, 4), (UP_Y + UP_T + 14, 4), (UP_Y + UP_T, 30)]).close()
             .extrude(4).translate((22 if sx > 0 else -26, 0, 0)))
        u = u.union(g)
        u = u.cut(cq.Workplane("XY").center(15 * sx, UP_Y + UP_T + 8).circle(M3_CLR).extrude(4))
    return u

def motor_upright():
    u = _upright().cut(yhole(0, HC, NEMA_BOSS_R, UP_Y, UP_Y + UP_T))
    for sx in (1, -1):
        for sz in (1, -1):
            u = u.cut(yhole(NEMA_HOLES * sx, HC + NEMA_HOLES * sz, M3_CLR, UP_Y, UP_Y + UP_T))
    return u

def idle_upright():
    u = _upright().union(yhole(0, HC, 10.0, UP_Y + UP_T, UP_Y + UP_T + 12))                   # mozzo perno
    u = u.cut(yhole(0, HC, 5.0, UP_Y, UP_Y + UP_T + 12))
    u = u.cut(cq.Workplane("XY").center(0, UP_Y + UP_T + 6).circle(M3_TAP).extrude(12).translate((0, 0, HC)))
    return u.mirror("XZ")

def drive_dog():
    """Trascinatore unico: corpo sull'albero NEMA17 (grano M3 + dado), codolo nel foro del tubo, due denti nelle tacche."""
    d = cq.Workplane("XY").circle(TUBE_R).extrude(14.0)
    d = d.union(cq.Workplane("XY").circle(TUBE_BORE - 0.2).extrude(7.0).translate((0, 0, 14.0)))
    for sg in (1, -1):
        d = d.union(cq.Workplane("XY").box(5.4, 2.75, 3.6, centered=(True, False, False))
                    .translate((0, (TUBE_BORE - 0.4) if sg > 0 else -(TUBE_BORE - 0.4) - 2.75, 14.0)))
    d = d.cut(cq.Workplane("XY").circle(2.6).extrude(SHAFT_IN - 0.7))                         # foro albero Ø5.2
    d = d.cut(cq.Workplane("YZ").center(0, 5.0).circle(M3_CLR).extrude(TUBE_R).translate((2.0, 0, 0)))   # grano M3
    d = d.cut(cq.Workplane("XY").box(2.7, 5.8, 8.0, centered=(False, True, False)).translate((4.0, 0, 0)))  # sede dado
    return d.rotate((0, 0, 0), (1, 0, 0), 90).translate((0, UP_Y - 1.2, HC)), d

def idle_plug():
    """Perno folle: tampone che entra nel foro del tubo (la spirale ci gira sopra) + gambo scorrevole nel montante."""
    p = cq.Workplane("XY").circle(TUBE_BORE - 0.15).extrude(8.0).faces("<Z").chamfer(0.8)
    p = p.union(cq.Workplane("XY").circle(4.85).extrude(50.0))
    p = p.cut(cq.Workplane("YZ").center(0, 45.0).circle(1.0).extrude(12, both=True))          # foro per maniglia (filamento)
    return p.rotate((0, 0, 0), (1, 0, 0), 90).translate((0, -(TUBE_HALF - 7.5), HC)), p

def base():
    yb = UP_Y + UP_T + 20
    b = ybox(-116, 52, -yb, yb, -4, 0)
    b = b.cut(ybox(-40, 40, -44, 44, -4, 0)).cut(ybox(-100, -64, -30, 30, -4, 0))             # alleggerimenti
    pts = [(-104, 46.5), (-60, 46.5), (-104, -46.5), (-60, -46.5),
           (15, UP_Y + UP_T + 8), (-15, UP_Y + UP_T + 8), (15, -(UP_Y + UP_T + 8)), (-15, -(UP_Y + UP_T + 8))]
    for x, y in pts:
        b = b.cut(cq.Workplane("XY").center(x, y).circle(M3_TAP).extrude(-4))
    return b

if __name__ == "__main__":
    pass
