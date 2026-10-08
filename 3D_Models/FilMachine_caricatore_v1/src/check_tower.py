"""Verifiche della torretta: interferenze fisse, spazzata della lama, quote di taglio, bracci guida."""
import itertools
from common import *
import tower, layout

def vol(a, b): return a.val().intersect(b.val()).Volume()
fixed = {"guanciaSX": tower.cheek(1), "guanciaDX": tower.cheek(-1), "ponte": tower.bridge(),
         "servo": tower.servo_bracket(), "culla135": tower.cradle135()}
fixed120 = {**{n: p for n, p in fixed.items() if n != "culla135"}, "culla120": tower.cradle120()}
for tag, F in (("135", fixed), ("120", fixed120)):
    bad = [(a, b, round(vol(F[a], F[b]), 3)) for a, b in itertools.combinations(F, 2) if vol(F[a], F[b]) > 1e-4]
    print(f"parti fisse ({tag}): interferenze {bad if bad else 'nessuna'}")
tot = 0; worst = 0
for al in (-SWING_PARK, -20, -10, 0, 10, 20, SWING_END):
    for m in (tower.swing_arm(al), tower.blade_cap(al), tower.blade(al)):
        for f in list(fixed.values()) + [fixed120["culla120"]]:
            v = vol(m, f); tot += v; worst = max(worst, v)
print(f"spazzata lama da {-SWING_PARK:.1f} a {SWING_END:.1f} gradi (7 posizioni): interferenza totale {tot:.4f} mm3")
tip = lambda y: SERVO_Z + math.sqrt(SWING_R ** 2 - y ** 2)
print(f"quota punta lama: ai bordi 120 (y=31.05) {tip(31.05):.2f} | ai bordi 135 (y=17.5) {tip(17.5):.2f} | al centro {tip(0):.2f} "
      f"| cielo fessura pellicola {SLOT_Z1:.1f}, cielo fessura lama {BR_Z1 - 3.5:.1f}")
park = tower.blade(-SWING_PARK).val()
zone = tower.ybox(-80, -60, -31.05, 31.05, SLOT_Z0, SLOT_Z1).val()        # dove puo' trovarsi la pellicola 120
print(f"lama parcheggiata: distanza dalla zona pellicola 120 {park.distance(zone):.2f} mm, dalla 135 "
      f"{park.distance(tower.ybox(-80, -60, -17.8, 17.8, SLOT_Z0, SLOT_Z1).val()):.2f} mm; "
      f"gioco lama/ponte {park.distance(fixed['ponte'].val()):.2f} mm, lama/guancia DX {park.distance(fixed['guanciaDX'].val()):.2f} mm")
head = tower.swing_arm(0).val().BoundingBox().zmax
print(f"testa del braccio lama (verticale): z max {head:.1f}, sotto il ponte (z={BR_Z0}) di {BR_Z0 - head:.1f} mm; "
      f"gioco minimo braccio lama/guance a fine corsa {min(tower.swing_arm(s).val().distance(fixed[g].val()) for s in (-SWING_PARK, SWING_END) for g in ('guanciaSX', 'guanciaDX')):.2f} mm")
cas = cq.Workplane("XZ").center(CAS_X, CAS_Z).circle(12.5).extrude(22.25, both=True)
print("caricatore 135 nella culla: interferenza", round(vol(cas, fixed["culla135"]), 4), "mm3; gioco", round(cas.val().distance(fixed["culla135"].val()), 2))
spool = cq.Workplane("XZ").center(CAS_X, ZF - 7.5).circle(12.5).extrude(32.5, both=True)
print("rocchetto 120 nella culla: interferenza", round(vol(spool, fixed120["culla120"]), 4), "mm3; gioco", round(spool.val().distance(fixed120["culla120"].val()), 2))
for fmt, F in (("135", fixed), ("120", fixed120)):
    for r in (SOLE_STOP_R, 42.0):
        a, phi = layout.place_arm(fmt, r)
        print(f"braccio guida {fmt} r={r}: interferenza con torretta {sum(vol(a, f) for f in F.values()):.4f} mm3, "
              f"gioco minimo {min(a.val().distance(f.val()) for f in F.values()):.2f} mm")
