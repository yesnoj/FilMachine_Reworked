"""Assiemi completi 135 / 120: tutti i pezzi in posizione di montaggio + sagome dei componenti commerciali."""
import itertools, sys
import cadquery as cq
from common import *
import reel, guide, tower, drive, accessories, layout

def dummies(fmt):
    yb, yh = tower.ybox, tower.yhole
    d = {}
    d["REF_motore_NEMA17"] = yb(-21.15, 21.15, UP_Y + UP_T, UP_Y + UP_T + 40, HC - 21.15, HC + 21.15) \
        .union(yh(0, HC, 2.5, UP_Y + UP_T - 24.0, UP_Y + UP_T))
    sx = SB_X1                                            # faccia d'appoggio delle alette del servo
    d["REF_servo_SG90"] = yb(sx - 18.4, sx + 4.3, -6.0, 16.8, SERVO_Z - 6.1, SERVO_Z + 6.1) \
        .union(yb(sx, sx + 2.5, 5.4 - 16.15, 5.4 + 16.15, SERVO_Z - 6.1, SERVO_Z + 6.1)) \
        .union(cq.Workplane("YZ").center(0, SERVO_Z).circle(2.4).extrude(11.0).translate((sx + 4.3, 0, 0)))
    d["REF_lama_cutter_9mm"] = tower.blade(-SWING_PARK)
    if fmt == "135":
        d["REF_caricatore_135"] = cq.Workplane("XZ").center(CAS_X, CAS_Z).circle(12.5).extrude(22.25, both=True) \
            .union(yb(CAS_X, CAS_X + 14.5, -19, 19, ZF - 1.5, ZF + 1.0))
    else:
        rz = ZF - 7.5
        d["REF_rocchetto_120"] = cq.Workplane("XZ").center(CAS_X, rz).circle(6.0).extrude(32.5, both=True) \
            .union(cq.Workplane("XZ").center(CAS_X, rz).circle(12.5).extrude(1.2).translate((0, 32.5, 0))) \
            .union(cq.Workplane("XZ").center(CAS_X, rz).circle(12.5).extrude(-1.2).translate((0, -32.5, 0)))
    return d

def clip_in_pocket():
    """Clip appoggiata nella tasca del tamburo (ganasce disegnate aperte, cursore parcheggiato sul collo)."""
    j = accessories.jaw().rotate((0, 0, 0), (0, 0, 1), 90).translate((CLIP_FLOOR + 2.3, -CLIP_L / 2, -CLIP_W / 2))
    s = accessories.slider().rotate((0, 0, 0), (1, 1, 1), -120).translate((CLIP_FLOOR + 2.3, -CLIP_L / 2 + 4.3, 0))
    return [p.rotate((0, 0, 0), (0, 0, 1), -30).rotate((0, 0, 0), (1, 0, 0), 90).translate((0, 0, HC)) for p in (j, s)]

def build(fmt):
    fa, tb, dr, fb = layout.place_reel(fmt)
    arm, phi = layout.place_arm(fmt, SOLE_STOP_R)
    cj, cs = clip_in_pocket()
    P = {"01_flangia_A": fa, "02_flangia_B": fb, "03_tubo_baionetta": tb, "04_tamburo": dr,
         ("05_braccio_guida_135" if fmt == "135" else "06_braccio_guida_120"): arm,
         "07_guancia_SX": tower.cheek(1), "08_guancia_DX": tower.cheek(-1), "09_ponte_lama": tower.bridge(),
         "10_braccio_lama": tower.swing_arm(-SWING_PARK), "11_morsetto_lama": tower.blade_cap(-SWING_PARK),
         "12_supporto_servo": tower.servo_bracket(),
         ("13_culla_135" if fmt == "135" else "14_culla_120"): (tower.cradle135() if fmt == "135" else tower.cradle120()),
         "15_montante_motore": drive.motor_upright(), "16_montante_folle": drive.idle_upright(),
         "17_trascinatore": drive.drive_dog()[0], "18_perno_folle": drive.idle_plug()[0],
         "19_basamento": drive.base(), "20_clip_ganascia": cj, "21_clip_cursore": cs}
    P.update(dummies(fmt))
    return P

COL = {"01": (0.80, 0.82, 0.85), "02": (0.80, 0.82, 0.85), "03": (0.95, 0.65, 0.20), "04": (0.85, 0.50, 0.15),
       "05": (0.36, 0.55, 0.94), "06": (0.36, 0.55, 0.94), "07": (0.62, 0.66, 0.72), "08": (0.62, 0.66, 0.72),
       "09": (0.90, 0.45, 0.35), "10": (0.30, 0.70, 0.45), "11": (0.20, 0.55, 0.35), "12": (0.60, 0.50, 0.85),
       "13": (0.85, 0.75, 0.30), "14": (0.85, 0.75, 0.30), "15": (0.45, 0.50, 0.58), "16": (0.45, 0.50, 0.58),
       "17": (0.95, 0.35, 0.55), "18": (0.95, 0.35, 0.55), "19": (0.30, 0.33, 0.38), "20": (0.95, 0.85, 0.20),
       "21": (0.95, 0.85, 0.20), "RE": (0.15, 0.15, 0.17)}

def interferences(P):
    """Controllo incrociato di tutte le coppie di corpi dell'assieme."""
    bad = []
    for a, b in itertools.combinations(list(P), 2):
        ba, bb = P[a].val().BoundingBox(), P[b].val().BoundingBox()
        if (ba.xmin > bb.xmax or bb.xmin > ba.xmax or ba.ymin > bb.ymax or bb.ymin > ba.ymax
                or ba.zmin > bb.zmax or bb.zmin > ba.zmax):
            continue
        v = P[a].val().intersect(P[b].val()).Volume()
        if v > 1e-3: bad.append((a, b, round(v, 3)))
    return bad

def save(fmt, P):
    asm = cq.Assembly(name=f"FilMachine_caricatore_{fmt}")
    for n, p in P.items():
        asm.add(p.val(), name=n, color=cq.Color(*COL[n[:2]]))
    path = os.path.join(OUT, "step", f"ASSIEME_{fmt}.step")
    asm.export(path)
    return path

if __name__ == "__main__":
    for fmt in (sys.argv[1:] or ["135", "120"]):
        P = build(fmt)
        path = save(fmt, P)
        bad = interferences(P)
        print(f"ASSIEME_{fmt}: {len(P)} corpi, {os.path.getsize(path)/1e6:.1f} MB, coppie in interferenza: {bad if bad else 'nessuna'}")
