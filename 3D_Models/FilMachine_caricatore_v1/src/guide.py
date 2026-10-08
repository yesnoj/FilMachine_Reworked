"""Braccio guida oscillante: incurva la pellicola (arco) e la posa nella spirale."""
import math
import cadquery as cq
from common import *

def arm_dims(fmt):
    chord, sag, R = arch(fmt)
    ai = chord / 2                       # semi-larghezza interna in punta
    ei = reel_width(fmt) / 2             # semi-larghezza interna all'ingresso (pellicola piana)
    wr = H_EDGE + sag + 0.9              # quota colmo interno
    return dict(ai=ai, ao=ai + WALL_TIP, ei=ei, eo=ei + 2.0, wr=wr, wt=wr + 1.2,
                run=(wr - H_EDGE - 2.0) / math.tan(math.radians(40)), sag=sag, R=R, chord=chord)

def _cav(a, s, d):
    """Mezza sezione della cavita' (lato +Y) a semi-larghezza a, ascissa s."""
    return [(s, 0, -1), (s, a - 1.6, -1), (s, a - 1.6, H_EDGE + 1.6), (s, a, H_EDGE),
            (s, a, H_EDGE + 2.0), (s, a - d["run"], d["wr"]), (s, 0, d["wr"])]

def _box(o, s, wt):
    return [(s, -o, 0), (s, o, 0), (s, o, wt), (s, -o, wt)]

def arm(fmt):
    d = arm_dims(fmt)
    st = [(-5.0, d["ei"] + 1.0, d["eo"] + 1.0), (ARM_TAPER0, d["ei"], d["eo"]),      # invito laterale all'ingresso
          (ARM_TAPER1, d["ai"], d["ao"]), (ARM_L, d["ai"], d["ao"])]
    body = None; cav = None
    for (s0, i0, o0), (s1, i1, o1) in zip(st[:-1], st[1:]):
        b = ruled(_box(o0, s0, d["wt"]), _box(o1, s1, d["wt"]))
        c = ruled(_cav(i0, s0 - 0.01, d), _cav(i1, s1 + 0.01, d))
        body = b if body is None else body.fuse(b)
        cav = c if cav is None else cav.fuse(c)
    body = cq.Workplane("XY").add(body)
    cav = cq.Workplane("XY").add(cav)
    cav = cav.union(cav.mirror("XZ"))
    # forcella con le orecchie del perno (fuori dalla larghezza pellicola)
    yoke = cq.Workplane("XY").box(10, 2 * YOKE_HALF, d["wt"] - d["wr"], centered=(True, True, False)) \
        .translate((0, 0, d["wr"]))
    ear = cq.Workplane("XY").box(10, 2.0, d["wt"] - (H_EDGE - 5), centered=(True, False, False)) \
        .translate((0, YOKE_HALF - 2.0, H_EDGE - 5))
    a = body.union(yoke).union(ear).union(ear.mirror("XZ")).cut(cav)
    # fori perno solo nelle orecchie (non attraversano le pareti del tunnel)
    hole = cq.Workplane("XZ").circle(1.65).extrude(4.0).translate((0, YOKE_HALF + 1.0, H_EDGE))
    a = a.cut(hole).cut(hole.mirror("XZ"))
    # invito verticale: all'ingresso le mensole partono basse e salgono (la pellicola arriva da sotto)
    ramp = (cq.Workplane("XZ").polyline([(-5.01, 0.6), (ARM_TAPER0, 4.1), (-5.01, 4.1)]).close()
            .extrude(d["ei"] + 1.0, both=True))
    a = a.cut(ramp)
    # punta della suola smussata per scorrere sul giro precedente
    wedge = cq.Workplane("XZ").polyline([(ARM_L - 3, -0.01), (ARM_L + 0.01, -0.01), (ARM_L + 0.01, 1.2)]) \
        .close().extrude(60, both=True)
    return a.cut(wedge), d

if __name__ == "__main__":
    for fmt in ("135", "120"):
        a, d = arm(fmt)
        export(to_print(a, [("X", 180)]), f"04_braccio_guida_{fmt}")
        print({k: round(v, 2) for k, v in d.items()})
        render([a], f"04_braccio_guida_{fmt}", elev=-28, azim=-50)
