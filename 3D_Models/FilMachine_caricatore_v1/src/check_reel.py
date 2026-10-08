"""Verifica cinematica della baionetta: infilaggio, rotazione di blocco, tenuta nelle due stazioni."""
from common import *
import reel
t = reel.tube().val()
fa0 = reel.flange()
def A(z_lug, ang):      # flangia A col centro dente a quota z_lug e ruotata di ang gradi
    return fa0.translate((0, 0, z_lug - LUG_AX / 2)).rotate((0, 0, 0), (0, 0, 1), ang).val()
def iv(z, a): return A(z, a).intersect(t).Volume()
z35, z120 = lug_z("135"), lug_z("120")
ins = [iv(z, -LOCK_DEG) for z in (-TUBE_HALF - 2, -TUBE_HALF + 1, z120, (z120 + z35) / 2, z35)]
print("infilaggio lungo il canale (a -30 gradi), interferenza mm3:", [round(v, 4) for v in ins])
for name, zs in (("120", z120), ("135", z35)):
    rot = [iv(zs, a) for a in (-30, -22.5, -15, -7.5, 0)]
    print(f"stazione {name}: rotazione di blocco -30..0 gradi:", [round(v, 4) for v in rot],
          "| oltre la battuta (+3 gradi):", round(iv(zs, 3), 3),
          "| sfilamento assiale a blocco inserito (+/-1 mm):", round(iv(zs - 1, 0), 3), round(iv(zs + 1, 0), 3))
print("rotazione a meta' canale (fuori stazione, -15 gradi):", round(iv((z120 + z35) / 2, -15), 3), "(deve interferire)")
