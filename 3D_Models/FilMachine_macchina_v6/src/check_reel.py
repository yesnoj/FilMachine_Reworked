"""Verifica cinematica della baionetta v5: tre denti con chiave, versi di blocco opposti sui due lati, scatto."""
import math
from common import *
import reel

t = reel.tube().val()
FA = reel.flange(bump=False)                       # per la cinematica la linguetta e' senza dentino
FB = reel.flange(mirror=True, bump=False)

def A(z_lug, ang):
    """Flangia A (lato z<0) col centro dente a quota z_lug (negativa), ruotata di ang gradi."""
    return FA.translate((0, 0, z_lug - LUG_AX / 2)).rotate((0, 0, 0), (0, 0, 1), ang).val()

def B(z_lug, ang):
    """Flangia B (lato z>0): la stessa trasformazione dell'assieme (ribaltata di 180 gradi attorno a X)."""
    return (FB.rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, z_lug + LUG_AX / 2))
            .rotate((0, 0, 0), (0, 0, 1), ang).val())

iv = lambda s: round(s.intersect(t).Volume(), 4)
z35, z120 = abs(lug_z("135")), abs(lug_z("120"))

print(f"denti: {len(LUG_ANG)} a {LUG_ANG} gradi, chiave {LUG_KEY_DEG:.0f} gradi (canale {CHAN_KEY_DEG:.0f}), gli altri {LUG_DEG:.0f} (canale {CHAN_DEG:.0f})")
for name, P, sg, tw in (("A", A, -1, -LOCK_DEG), ("B", B, +1, +LOCK_DEG)):
    ins = [iv(P(sg * z, tw)) for z in (TUBE_HALF + 2, TUBE_HALF - 1, z120, (z120 + z35) / 2, z35)]
    print(f"flangia {name}: infilaggio lungo il canale (a {tw:+.0f} gradi), interferenza mm3: {ins}")
    for st, zs in (("120", z120), ("135", z35)):
        rot = [iv(P(sg * zs, tw * k)) for k in (1, 0.75, 0.5, 0.25, 0)]
        print(f"   stazione {st}: rotazione di blocco {tw:+.0f}..0: {rot} | oltre la battuta ({-tw/10:+.0f} gradi): {iv(P(sg * zs, -tw / 10))}"
              f" | sfilamento assiale a blocco inserito (+/-1 mm): {iv(P(sg * (zs - 1), 0))} {iv(P(sg * (zs + 1), 0))}")
    print(f"   rotazione a meta' canale (fuori stazione): {iv(P(sg * (z120 + z35) / 2, tw / 2))} (deve interferire)")
    wrong = [iv(P(sg * (TUBE_HALF - 1), tw + d)) for d in (120.0, 240.0)]
    print(f"   anti-errore: imbocco con la flangia sfasata di 120 e 240 gradi: {wrong} (deve interferire)")
    print(f"   verso sbagliato: dal canale ruotando al contrario ({2*tw:+.0f} gradi) in stazione 135: {iv(P(sg * z35, 2 * tw))} (deve interferire)")
print(f"versi di blocco: A ruota di {LOCK_DEG:+.0f} gradi, B di {-LOCK_DEG:+.0f} gradi (visti dallo stesso lato) -> opposti")

# ---- spirali in fase (le due flange di ogni formato devono avere la costola nello stesso punto) ----
for fmt in ("135", "120"):
    ra = reel.assembled(fmt)
    ribA = ra[0].val().intersect(cq.Workplane("XY").circle(FLANGE_R - RIM_W - 0.2).circle(RIB_R0 + 0.6).extrude(RIB_H - 0.1)
                                  .translate((0, 0, -(reel_width(fmt) / 2) + 0.0)).val())
    ribB = ra[2].val().intersect(cq.Workplane("XY").circle(FLANGE_R - RIM_W - 0.2).circle(RIB_R0 + 0.6).extrude(-(RIB_H - 0.1))
                                  .translate((0, 0, (reel_width(fmt) / 2) - 0.0)).val())
    ribB_m = cq.Workplane("XY").add(ribB).mirror("XY").val()
    sym = ribA.cut(ribB_m).Volume() + ribB_m.cut(ribA).Volume()
    arcs = reel.spiral_arcs(reel.SPIRALS[fmt])
    print(f"spirali {fmt} A e B in fase: volume costola A {ribA.Volume():.1f} mm3, differenza con B specchiata {sym:.3f} mm3; "
          f"{len(arcs) / 2:.1f} giri a passo {2 * (arcs[1][1] - arcs[0][1]):.2f} mm, pellicola contenibile {reel.spiral_length(fmt):.0f} mm")

# ---- scatto ----
rb = TUBE_R + REEL_FIT
cyl = cq.Workplane("XY").circle(TUBE_R).extrude(2 * TUBE_HALF).translate((0, 0, -TUBE_HALF)).val()
def bump_at(part, z_lug, ang, side):
    """Dentino della linguetta nella posizione indicata (flangia o anello, lato A o B)."""
    if part == "flange":
        _, _, b = reel._finger(rb, DISC_T, DISC_T + SLEEVE_L, DET_ANG)
        if side == "A":
            b = b.translate((0, 0, -z_lug - LUG_AX / 2))
        else:
            b = b.rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, z_lug + LUG_AX / 2))
    else:
        _, _, b = reel._finger(rb, 1.0, 8.8, -DET_ANG, steep=+1)
        if side == "B":
            b = b.translate((0, 0, z_lug - RING_LUG))
        else:
            b = b.rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, -z_lug + RING_LUG))
    return b.rotate((0, 0, 0), (0, 0, 1), ang).val()

full = bump_at("flange", z35, -LOCK_DEG, "A").intersect(cyl).Volume()
print(f"scatto: il dentino sporge {BUMP_H:.2f} mm nel tubo (linguetta flessa di {BUMP_H - REEL_FIT:.2f} mm fuori dalla gola); volume di riferimento {full:.3f} mm3")
for part, side, tw, stations in (("flange", "A", -LOCK_DEG, (z35, z120)), ("flange", "B", LOCK_DEG, (z35, z120)),
                                 ("ring", "A", -LOCK_DEG, (ring_station("135"), ring_station("120"))),
                                 ("ring", "B", LOCK_DEG, (ring_station("135"), ring_station("120")))):
    zs_path = [TUBE_HALF + 6, TUBE_HALF + 3, TUBE_HALF, TUBE_HALF - 3] + list(stations)
    onpath = []
    for z in zs_path:
        b = bump_at(part, z, tw, side)
        onpath.append(round(b.intersect(cyl).Volume() - b.intersect(t).Volume(), 4))
    lock = [round(bump_at(part, z, 0, side).intersect(t).Volume(), 4) for z in stations]
    half = [round(bump_at(part, z, tw / 2, side).intersect(t).Volume(), 3) for z in stations]
    print(f"   {part:6s} lato {side}: lungo l'infilaggio il dentino trova sempre tubo pieno (mancante mm3): {onpath} | "
          f"a meta' rotazione preme {half} | a blocco inserito interferenza {lock} (0 = nella gola)")

# ---- v6.2: scalino contro lo svitamento (fianco ripido della gola e del dentino) ----
def unlock_sweep(part, side, zs, sign):
    """Volume del dentino dentro il tubo ruotando dal blocco verso lo sblocco (sign = verso dello sblocco)."""
    out = []
    for a in (0.0, 0.5, 1.0, 1.5, 2.0, 3.0, 5.0):
        out.append(round(bump_at(part, zs, sign * a, side).intersect(t).Volume(), 3))
    return out
print(f"scalino contro lo svitamento: dentino alto {BUMP_H:.2f} mm, fianco ripido a {DET_STEEP:.0f} gradi dal lato dello sblocco (prima 45)")
for part, side, sign, zs in (("flange", "A", -1, z35), ("flange", "B", +1, z35), ("ring", "A", -1, ring_station("135")), ("ring", "B", +1, ring_station("135"))):
    print(f"   {part:6s} lato {side}: ruotando verso lo sblocco di 0 / 0,5 / 1 / 1,5 / 2 / 3 / 5 gradi il dentino entra nel tubo per {unlock_sweep(part, side, zs, sign)} mm3 "
          f"(0 = libero nella gola; il massimo = linguetta flessa del tutto)")

# ---- anello alette: infilaggio/blocco sui due lati ----
RA = reel.fin_ring("A", bump=False)
def R(z_lug, ang, side):
    if side == "B":
        r = RA.translate((0, 0, z_lug - RING_LUG))
    else:
        r = RA.rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, -z_lug + RING_LUG))
    return r.rotate((0, 0, 0), (0, 0, 1), ang).val()
for side, tw in (("A", -LOCK_DEG), ("B", LOCK_DEG)):
    for fmt in ("135", "120"):
        zs = ring_station(fmt)
        print(f"anello alette lato {side}, stazione per il {fmt} (|z|={zs:.2f}): infilaggio {[iv(R(z, tw, side)) for z in (TUBE_HALF + 2, TUBE_HALF - 1, zs)]}"
              f" | rotazione {[iv(R(zs, tw * k, side)) for k in (1, 0.5, 0)]} | oltre battuta {iv(R(zs, -tw / 10, side))}"
              f" | sfilamento {iv(R(zs + 1, 0, side))} {iv(R(zs - 1, 0, side))}")

# ---- assiemi della spirale ----
def push(ring, toward, cw=True):
    """+1 se la faccia d'attacco delle alette guarda verso la spirale (spinge dentro), -1 se guarda fuori."""
    out = []
    for f in ring.val().Faces():
        if f.geomType() != "PLANE" or f.Area() < 150: continue
        n = f.normalAt(); c = f.Center()
        if abs(n.z) > 0.95: continue
        v = (c.y, -c.x) if cw else (-c.y, c.x)
        if n.x * v[0] + n.y * v[1] > 0: out.append(n.z * toward)
    assert len(out) == FIN_N and (min(out) > 0 or max(out) < 0)
    return "DENTRO" if min(out) > 0 else "FUORI"
for fmt in ("135", "120"):
    P = reel.assembled(fmt)
    names = ["flangia A", "tubo", "flangia B"]
    bad = []
    for i in range(3):
        for j in range(i + 1, 3):
            v = P[i].val().intersect(P[j].val()).Volume()
            if v > 1e-4: bad.append((names[i], names[j], round(v, 4)))
    print(f"spirale {fmt} montata: interferenze {bad if bad else 'nessuna'}; gioco flangia/tubo {P[0].val().distance(P[1].val()):.3f} mm")
    for mode, label in (("alternata", "stesso anello A sui due lati"), ("unico", "anelli speculari A + B")):
        F = reel.fin_rings_assembled(fmt, mode)
        badr = [(i, names[j], round(a.val().intersect(b.val()).Volume(), 4)) for i, a in enumerate(F) for j, b in enumerate(P)
                if a.val().intersect(b.val()).Volume() > 1e-4]
        gap = round(min(F[0].val().distance(P[0].val()), F[1].val().distance(P[2].val())), 2)
        print(f"   {fmt} alette, {label}: interferenze con la spirale {badr if badr else 'nessuna'}, gioco dalla flangia {gap} mm")
        if fmt == "135":
            for cw, verso in ((True, "orario (avvolgimento)"), (False, "antiorario")):
                print(f"      rotazione {verso:22s}: lato A spinge {push(F[0], +1, cw)}, lato B spinge {push(F[1], -1, cw)}")

# ---- passaggio della fascetta nel tubo (v6.2, come la Lab-Box) e gola dello scatto solo nelle stazioni ----
import math as _m
T = reel.tube().val()
def _open_arcs(r, z):
    free = [i / 10 for i in range(3600) if not T.isInside(cq.Vector(r * _m.cos(_m.radians(i / 10)), r * _m.sin(_m.radians(i / 10)), z), 1e-3)]
    iv = []
    for a in free:
        if iv and abs(a - iv[-1][1]) < 0.15: iv[-1][1] = a
        else: iv.append([a, a])
    return [((a + b) / 2, _m.radians(b - a + 0.1) * r) for a, b in iv]
rw = (TUBE_R + TUBE_BORE) / 2
arcs = {round(c) % 360: w for c, w in _open_arcs(rw, 0.0)}
through = all(len(_open_arcs(r, z)) == 2 for r in (TUBE_BORE + 0.1, rw, TUBE_R - 0.1) for z in (-STRAP_W / 2 + 0.2, 0.0, STRAP_W / 2 - 0.2))
zs = [z / 20 for z in range(-300, 301) if not T.isInside(cq.Vector(rw * _m.cos(_m.radians(reel.STRAP_SLOT_ANG)), rw * _m.sin(_m.radians(reel.STRAP_SLOT_ANG)), z / 20), 1e-3)]
w_n, w_w = arcs.get(round(reel.STRAP_SLOT_ANG) % 360, 0.0), arcs.get(round(reel.STRAP_SLOT_ANG + 180) % 360, 0.0)
print(f"fascetta: finestra stretta {w_n:.2f} mm a {reel.STRAP_SLOT_ANG:.0f} gradi, finestra larga {w_w:.2f} mm a {reel.STRAP_SLOT_ANG + 180:.0f} gradi, "
      f"alte {zs[-1] - zs[0]:.1f} mm, passanti da parte a parte: {'si' if through and len(arcs) == 2 else 'NO'}")
col = None
for z0, z1 in ((-reel.COLLAR_Z - reel.COLLAR_T, -reel.COLLAR_Z), (reel.COLLAR_Z, reel.COLLAR_Z + reel.COLLAR_T)):   # i due collarini, come nel tubo
    prof = [(TUBE_R - 0.01, z0 - (reel.COLLAR_R - TUBE_R)), (reel.COLLAR_R, z0), (reel.COLLAR_R, z1), (TUBE_R - 0.01, z1)]
    c = cq.Workplane("XZ").polyline(prof).close().revolve(360, (0, 0, 0), (0, 1, 0))
    col = c if col is None else col.union(c)
fa135, _, fb135 = reel.assembled("135")
gap = min(col.val().distance(fa135.val()), col.val().distance(fb135.val()))
print(f"   fascetta larga {STRAP_W:.1f}, spessa {reel.STRAP_T:.2f}: tra i collarini {2 * reel.COLLAR_Z:.1f} mm (gioco {reel.COLLAR_Z - STRAP_W / 2:.2f} per lato); "
      f"nella stretta passa con {w_n - reel.STRAP_T:.2f} mm di gioco; collarini / manicotti delle flange 135 (le piu' vicine): gioco {gap:.2f} mm")
print(f"   perno del capo spesso {reel.STRAP_PIN_T:.2f}, lungo {reel.STRAP_PIN_L:.1f}: dalla larga passa ({'si' if reel.STRAP_PIN_T < w_w - 0.2 else 'NO'}, gioco {w_w - reel.STRAP_PIN_T:.2f}), "
      f"dalla stretta no ({'ok' if reel.STRAP_PIN_T > w_n + 0.4 else 'NO'}, {reel.STRAP_PIN_T - w_n:.2f} mm piu' spesso); lungo la finestra ci sta ({'si' if reel.STRAP_PIN_L < zs[-1] - zs[0] else 'NO'})")
r = TUBE_R - 0.3
spans = []
for side, ang in (("A", DET_ANG), ("B", -DET_ANG)):
    a = _m.radians(ang); seq = []
    for z in [k / 10 for k in range(int(-TUBE_HALF * 10) + 50, int(TUBE_HALF * 10) - 50)]:
        if not T.isInside(cq.Vector(r * _m.cos(a), r * _m.sin(a), z), 1e-3):
            if seq and z - seq[-1][1] < 0.15: seq[-1][1] = z
            else: seq.append([z, z])
    spans.append((side, [(round(p, 1), round(q, 1)) for p, q in seq]))
mid = any(abs(p) < reel.COLLAR_Z + reel.COLLAR_T or abs(q) < reel.COLLAR_Z + reel.COLLAR_T for _, sp in spans for p, q in sp)
print("gola dello scatto solo dove arriva il dentino (flange, anelli alette, provino): " + "; ".join(f"lato {s}: {sp}" for s, sp in spans)
      + f" | tra i collarini: {'GOLA PRESENTE' if mid else 'niente'}")
