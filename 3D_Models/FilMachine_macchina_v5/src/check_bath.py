"""Verifiche del bagno: inserimento / estrazione delle vaschette, tenuta al galleggiamento, quote di livello, volumi."""
import math
import cadquery as cq
from machine import *
import mrefs as R, hydro as H, machine_assembly as M

P = M.hydro_bodies()
vol = lambda a, b: a.val().intersect(b.val()).Volume()
PIV = (0.0, VAS_Y1, RIM_Z)                                   # spigolo posteriore del bordo, appoggiato sul longherone
def tilt(p, deg, dy=0.0, dz=0.0):
    return p.rotate(PIV, (1.0, PIV[1], PIV[2]), -deg).translate((0, dy, dz))

print("== inserimento della vaschetta: rotazione attorno al bordo posteriore (davanti in alto) ==")
for k, c in enumerate(("C1", "C2", "C3")):
    mov = [n for n in P if n.endswith("_" + c) and (n.startswith("REF_vaschetta") or n.startswith("C01") or n.startswith("REF_testina"))]
    if c == "C1": mov.append("REF_sonda_DS18B20_chimica")
    fixed = [n for n in P if n not in mov]
    for deg in (0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0):
        tot, worst, lat = 0.0, None, 0.0
        for m in mov:
            pm = tilt(P[m], deg); bm = pm.val().BoundingBox()
            for f in fixed:
                bf = P[f].val().BoundingBox()
                if bm.xmin > bf.xmax or bf.xmin > bm.xmax or bm.ymin > bf.ymax or bf.ymin > bm.ymax or bm.zmin > bf.zmax or bf.zmin > bm.zmax: continue
                v = vol(pm, P[f])
                if v > 1e-3:
                    if f.startswith("C03"): lat += v
                    else:
                        tot += v
                        if worst is None or v > worst[1]: worst = (m + " / " + f, round(v, 2))
        front = (VAS_Y1 - VAS_Y0) * math.sin(math.radians(deg))
        print(f"{c} inclinata {deg:3.1f} gradi (bordo anteriore +{front:4.1f} mm): interferenze {tot:7.2f} mm3 {worst if worst else ''}"
              f" | con i ganci a scatto {lat:6.1f} mm3")
    # estrazione: davanti su di 4.5 gradi (ganci liberi), avanti di 3.5 mm (il bordo posteriore esce dal labbro), poi su raddrizzando
    for deg, dy, dz in ((4.5, -1.5, 0.0), (4.5, -3.5, 0.0), (3.0, -3.5, 12.0), (0.0, -3.5, 30.0), (0.0, -3.5, 100.0), (0.0, -3.5, 200.0)):
        tot, worst = 0.0, None
        for m in mov:
            pm = tilt(P[m], deg, dy, dz); bm = pm.val().BoundingBox()
            for f in fixed:
                bf = P[f].val().BoundingBox()
                if bm.xmin > bf.xmax or bf.xmin > bm.xmax or bm.ymin > bf.ymax or bf.ymin > bm.ymax or bm.zmin > bf.zmax or bf.zmin > bm.zmax: continue
                v = vol(pm, P[f])
                if v > 1e-3:
                    tot += v
                    if worst is None or v > worst[1]: worst = (m + " / " + f, round(v, 2))
        print(f"   {c} estratta: {deg:.1f} gradi, avanti {-dy:.1f} mm, su {dz:.0f} mm -> interferenze {tot:.2f} mm3 {worst if worst else ''}")

# ------------------------------------------------------------------ volumi, livelli, galleggiamento
print("== volumi e livelli ==")
def pan_area(z_from_bottom, outer=True):
    """Sezione orizzontale della vaschetta GN 1/9 a una data quota sopra il fondo esterno (mm2)."""
    t = z_from_bottom / (GN9_H - GN9_RIM_T)
    off = GN9_RIM + GN9_DRAFT * (1 - t) + (0 if outer else GN9_WALL)
    return (GN9_L - 2 * off) * (GN9_W - 2 * off)
def integ(f, a, b, n=400):
    h = (b - a) / n
    return sum(f(a + (i + 0.5) * h) for i in range(n)) * h
cap = integ(lambda z: pan_area(z, False), GN9_WALL, GN9_H - GN9_RIM_T) / 1e6
z1 = GN9_WALL
while integ(lambda z: pan_area(z, False), GN9_WALL, z1) < 1.0e6: z1 += 0.5
print(f"vaschetta GN 1/9 h150: capacita' al bordo {cap:.2f} l; 1 litro arriva a {z1:.0f} mm dal fondo (quota Z = {VAS_ZB + z1:.0f}, {VAS_ZR - VAS_ZB - z1:.0f} mm sotto il bordo)")
A_TUB = (IX1 - IX0) * (IY1 - IY0)
def bath_volume(z):
    """Litri d'acqua nel bagno fino alla quota z, con le tre vaschette in sede."""
    v = A_TUB * (z - TUB_FLOOR)
    if z > VAS_ZB: v -= 3 * integ(lambda q: pan_area(q, True), 0.0, z - VAS_ZB)
    return v / 1e6
imm = WATER_Z - VAS_ZB
disp = integ(lambda q: pan_area(q, True), 0.0, imm) / 1e6
print(f"bagno: livello di lavoro Z = {WATER_Z:.0f} (vaschette immerse per {imm:.0f} mm) -> {bath_volume(WATER_Z):.1f} l d'acqua; "
      f"senza vaschette la stessa acqua arriva a Z = {TUB_FLOOR + bath_volume(WATER_Z) * 1e6 / A_TUB:.0f}")
free = A_TUB - 3 * pan_area(imm, True)
print(f"   ogni litro prelevato per i risciacqui abbassa il livello di {1e6 / free:.1f} mm")
res = bath_volume(WATER_Z) - bath_volume(LVL_ZMIN)
print(f"   riserva per i risciacqui tra livello di lavoro e sensore MIN (Z = {LVL_ZMIN:.0f}): {res:.1f} l = {res / 0.35:.0f} riempimenti da 350 ml "
      f"({res / 0.25:.0f} da 250 ml)")
print(f"   sensore MAX a Z = {LVL_ZMAX:.0f}: {bath_volume(LVL_ZMAX):.1f} l; bordo del contenitore a Z = {RIM_Z:.0f}")
print(f"   sommita' dei riscaldatori Z = {HEAT_Z + 6:.0f}; bocca del pescante WB Z = {WB_ZIN:.0f}: la pompa non puo' scoprire i riscaldatori "
      f"(restano {bath_volume(WB_ZIN):.1f} l); sensore MIN {LVL_ZMIN - HEAT_Z - 6:.0f} mm sopra i riscaldatori")
for dT, lab in ((18.0, "da 20 a 38 gradi"), (5.0, "da 33 a 38 gradi")):
    m = bath_volume(WATER_Z) + 3.0
    print(f"   riscaldamento {lab} di {m:.1f} kg (acqua + 3 l di chimica) con 200 W, senza perdite: {m * 4186 * dT / 200 / 60:.0f} minuti")
print("== galleggiamento ==")
up = disp * 9.81
for mat, rho in (("polipropilene", 0.91), ("policarbonato", 1.20)):
    w = P["REF_vaschetta_GN1-9_C1"].val().Volume() / 1000 * rho / 1000 * 9.81
    print(f"vaschetta vuota in {mat}: peso {w:.1f} N, spinta {up:.1f} N -> {up - w:.1f} N verso l'alto, divisi tra labbro posteriore e due ganci a scatto")
from hydro import BARB_Z, PAD_Y1, Y_PL0
E, t, b = 2000.0, 2.5, 7.0                     # PETG: modulo elastico (MPa); braccio del gancio: spessore, larghezza
L = BARB_Z - (LP_Z0 + 16.0)                    # lunghezza libera del braccio
I = b * t ** 3 / 12; ov = 4.5 - 0.3            # sovrapposizione del dente sulla sella
k = 3 * E * I / L ** 3
print(f"ganci a scatto: braccio {L:.0f} x {b:.0f} x {t:.1f} mm, rigidezza {k:.2f} N/mm; per sganciare servono {ov:.1f} mm = {k * ov:.1f} N per braccio, "
      f"deformazione {1.5 * t * ov / L ** 2 * 100:.1f} % (limite prudente per il PETG: 2 %)")
F = (up - 1.2) / 2 / 2                         # forza verso l'alto su ogni dente (meta' della spinta va al labbro posteriore)
M = F * (ov / 2 + t / 2)
print(f"   con la vaschetta vuota ogni dente porta circa {F:.1f} N: il braccio si apre di {M * L ** 2 / (2 * E * I):.2f} mm su {ov:.1f} di presa")
