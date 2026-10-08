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

# ---- nuovo accoppiamento e anello alette ----
print(f"accoppiamento flangia/tubo: gioco radiale foro {REEL_FIT:.2f} | fianchi dente (assiale) {REEL_FIT:.2f} | punta dente/fondo gola {0.1 + REEL_FIT:.2f} mm")
print("distanza minima flangia/tubo a blocco inserito:", round(A(z35, 0).distance(t), 3), "mm")
ring0 = reel.fin_ring("B")
zc = abs(z120) - (flange_face_y("135") + FIN_GAP)
def R(z_lug, ang):      # anello alette lato B, col centro dente a quota z_lug
    return ring0.translate((0, 0, z_lug - zc)).rotate((0, 0, 0), (0, 0, 1), ang).val()
rv = lambda z, a: R(z, a).intersect(t).Volume()
print("anello alette: infilaggio", [round(rv(z, -LOCK_DEG), 4) for z in (TUBE_HALF + 2, TUBE_HALF - 1, abs(z120))],
      "| rotazione di blocco", [round(rv(abs(z120), a), 4) for a in (-30, -15, 0)],
      "| oltre battuta", round(rv(abs(z120), 3), 3), "| sfilamento", round(rv(abs(z120) + 1, 0), 3), round(rv(abs(z120) - 1, 0), 3))
P = reel.assembled("135")
def push(ring, toward, cw=True):
    """+1 se la faccia d'attacco delle alette guarda verso la spirale (spinge dentro), -1 se guarda fuori."""
    out = []
    for f in ring.val().Faces():
        if f.geomType() != "PLANE" or f.Area() < 150: continue
        n = f.normalAt(); c = f.Center()
        if abs(n.z) > 0.95: continue
        v = (c.y, -c.x) if cw else (-c.y, c.x)             # orario / antiorario visto da +z (lato perno folle)
        if n.x * v[0] + n.y * v[1] > 0: out.append(n.z * toward)
    assert len(out) == FIN_N and (min(out) > 0 or max(out) < 0)
    return "DENTRO" if min(out) > 0 else "FUORI"
for mode, label in (("alternata", "stesso anello A sui due lati"), ("unico", "anelli speculari A + B")):
    F = reel.fin_rings_assembled(mode)
    bad = [(i, j, round(a.val().intersect(b.val()).Volume(), 4)) for i, a in enumerate(F) for j, b in enumerate(P) if a.val().intersect(b.val()).Volume() > 1e-4]
    gap = round(min(F[0].val().distance(P[0].val()), F[1].val().distance(P[3].val())), 2)
    print(f"alette, {label}: interferenze con la spirale {bad if bad else 'nessuna'}, gioco dalla flangia {gap} mm")
    for cw, verso in ((True, "orario (avvolgimento)"), (False, "antiorario")):
        print(f"   rotazione {verso:22s}: lato motore spinge {push(F[0], +1, cw)}, lato perno folle spinge {push(F[1], -1, cw)}")
