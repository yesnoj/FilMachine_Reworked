"""Clip della fascetta (ganascia + cursore). La ganascia si stampa di fianco: il profilo e' nel piano XY."""
import cadquery as cq
from common import *

def jaw():
    """Profilo laterale (u = lunghezza, v = spessore) estruso per CLIP_W. Aperta a riposo.
    u 0-4.2 testa con ancoraggio fascetta, 4.2-8.4 collo (parcheggio cursore), 8.4-18.4 ganasce a cuneo."""
    R0, R1 = 8.4, CLIP_L
    inner = lambda u: 0.25 + 0.4 * (u - R0) / (R1 - R0)
    outer = lambda u: inner(u) + 1.0 + 0.45 * (u - R0) / (R1 - R0)
    t0, t1 = R1 - 3.0, R1 - 2.0
    up = [(0, 0.4), (0, 2.2), (4.2, 2.2), (4.2, 1.25), (R0, 1.25), (R1, outer(R1)), (R1, inner(R1)),
          (t1, inner(t1)), ((t0 + t1) / 2, inner((t0 + t1) / 2) + 0.4), (t0, inner(t0)),          # gola
          (R0, 0.25), (R0, 0.0), (3.6, 0.0), (3.6, 0.4)]
    lo = [(0, -0.4), (3.6, -0.4), (3.6, 0.0), (R0, 0.0), (R0, -0.25),
          (t0, -inner(t0)), ((t0 + t1) / 2, -inner((t0 + t1) / 2) + 0.4), (t1, -inner(t1)),       # dente
          (R1, -inner(R1)), (R1, -outer(R1)), (R0, -1.25), (4.2, -1.25), (4.2, -2.2), (0, -2.2)]
    j = (cq.Workplane("XY").polyline(up).close().extrude(CLIP_W)
         .union(cq.Workplane("XY").polyline(lo).close().extrude(CLIP_W)))
    return j.cut(cq.Workplane("XY").center(2.1, 0).circle(1.3).extrude(CLIP_W))                    # perno fascetta (filamento 1.75)

def slider():
    s = cq.Workplane("XY").rect(CLIP_W + 2.4, 4.6).rect(CLIP_W + 0.4, 2.6).extrude(4.0)
    return s

if __name__ == "__main__":
    export(jaw(), "17_clip_ganascia"); export(slider(), "18_clip_cursore")
    section_plot(jaw(), "sez_clip", [9.0], axis="Z", size=(9, 3))
