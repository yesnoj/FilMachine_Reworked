"""Verifiche del modulo tank: taglierina, guida (anche col coperchio), montaggio della spirale, volumi di liquido."""
import math
import cadquery as cq
from common import *
import reel, layout, tank, loader, drive, refs, tank_assembly

body, lid = tank.body(), tank.lid()
fixed = {"corpo": body, "coperchio": lid, "bloccoDX": loader.bridge_block(1), "bloccoSX": loader.bridge_block(-1), "tetto": loader.roof()}
vol = lambda a, b: a.val().intersect(b.val()).Volume()
dist = lambda a, b: a.val().distance(b.val())

# ---- taglierina ----
print("== taglierina a Λ ==")
for th in (0, 45, 90, 135, 180):
    dz = loader.lift(th)
    mov = [loader.carrier(dz), loader.clamp(1, dz), loader.clamp(-1, dz), loader.blade(1, dz), loader.blade(-1, dz), loader.rod(th), loader.crank(th)]
    tot = sum(vol(m, f) for m in mov for f in fixed.values())
    srv = sum(vol(m, refs.servo()) for m in mov[5:])
    print(f"manovella {th:3d} gradi: alzata {dz:5.2f} mm, interferenze con parti fisse {tot:.4f} mm3, biella+manovella/servo {srv:.4f} mm3")
dz = STROKE
print(f"a riposo la punta sta a z={APEX_PARK:.1f}: {SLOT_Z0 - APEX_PARK:.1f} mm sotto il piano della fessura (z={SLOT_Z0}); a fine corsa punta a {APEX_PARK + dz:.1f}")
for w, lab in ((35.0, "135"), (61.5, "120")):
    print(f"   {lab}: ai bordi della pellicola il filo arriva a z={loader.edge_z(w / 2, dz):.2f}, {loader.edge_z(w / 2, dz) - SLOT_Z1:.2f} mm sopra il cielo della fessura (z={SLOT_Z1})")
print(f"   forza disponibile sul portalama a meta' corsa con un MG90S (1.8 kg*cm): circa {0.18 / (CRANK_R / 1000):.0f} N")

# ---- guida ----
print("== guida a due slitte ==")
names = ["flangia A", "tubo", "tamburo", "flangia B", "anello A", "anello B"]
for fmt in ("135", "120"):
    R = layout.place_reel(fmt)
    for r in (CORE_R, 30.0, 42.0):
        G, phi = loader.place_guide(fmt, r)
        sp = sum(vol(g, p) for g in G for p in R)
        fx = sum(vol(g, f) for g in G for f in fixed.values())
        car = sum(vol(g, m) for g in G for m in (loader.carrier(STROKE), loader.clamp(1, STROKE), loader.clamp(-1, STROKE)))
        print(f"{fmt} raggio {r:4.1f} (inclinazione {phi:+5.1f} gradi): interferenze spirale {sp:.4f}, parti fisse+coperchio {fx:.4f}, portalama alzato {car:.4f} mm3; "
              f"gioco slitte/flange {min(dist(g, R[i]) for g in G[1:] for i in (0, 3)):.2f}, slitte/coperchio {min(dist(g, lid) for g in G):.2f} mm")
print(f"punto di tangenza della suola sulla spirale: s = {math.sqrt(math.hypot(XB, HC - ZB) ** 2 - (CORE_R - loader.N_SOLE) ** 2):.1f} mm (vuota), "
      f"{math.sqrt(math.hypot(XB, HC - ZB) ** 2 - (42 - loader.N_SOLE) ** 2):.1f} mm (piena); la slitta e' lunga {S_TIP:.0f} mm")
for fmt in ("135", "120"):
    best = None
    for phi in range(95, 141, 5):                 # guida ribaltata all'indietro (coperchio aperto)
        G, _ = loader.place_guide(fmt, phi=float(phi))
        v = sum(vol(g, f) for g in G for n, f in fixed.items() if n != "coperchio")
        if v > 1e-3:
            best = phi; break
    print(f"{fmt}: guida ribaltata all'indietro: libera fino a {best - 5 if best else 140} gradi (poi appoggia sul ponte)")

# ---- montaggio della spirale ----
print("== montaggio della spirale ==")
for fmt in ("135", "120"):
    R0 = layout.place_reel(fmt)
    seat = sum(vol(p, f) for p in R0 for f in (body, drive.dog(), drive.idle_pin(), drive.cartridge()))
    back = 5.5
    R1 = [p.translate((0, -back, 0)) for p in R0]
    pin = drive.idle_pin(back)
    v1 = sum(vol(p, f) for p in R1 for f in (body, drive.dog(), pin))
    v2 = vol(pin, body)
    lift_ok = sum(vol(p.translate((0, 0, 30.0)), f) for p in R1 for f in (body, loader.bridge_block(1), loader.bridge_block(-1), loader.roof()))
    print(f"{fmt}: in sede interferenze {seat:.4f} mm3; arretrata di {back} mm contro la molla: spirale/corpo+trascinatore+perno {v1:.4f}, perno/corpo {v2:.4f}; "
          f"sollevata di 30 mm: {lift_ok:.4f} mm3")
print(f"   gioco minimo spirale/vasca: {min(dist(p, body) for p in layout.place_reel('120')):.2f} mm (120), {min(dist(p, body) for p in layout.place_reel('135')):.2f} mm (135)")

# ---- volumi di liquido ----
print("== liquido nella vasca ==")
cav = tank.trough_cavity()
def level_volume(z, solids):
    wet = cav.val().intersect(tank.box(-200, 200, -200, 200, -10, z).val())       # liquido lordo fino alla quota z
    disp = 0.0
    for s in solids:
        bb = s.val().BoundingBox()
        if bb.zmin >= z: continue
        try:
            disp += s.val().intersect(wet).Volume()
        except Exception:
            pass
    return (wet.Volume() - disp) / 1000.0
FILM = {"135": 35.0 * 1650 * 0.14, "120": 61.5 * 820 * 0.11}
for fmt in ("135", "120"):
    solids = layout.place_reel(fmt) + [drive.dog(), drive.idle_pin(), refs.shaft()]
    tab = {z: level_volume(z, solids) for z in range(20, 67, 2)}
    def z_for(ml):
        for z in sorted(tab):
            if tab[z] >= ml: return z
        return None
    film = FILM[fmt] / 1000.0
    print(f"{fmt}: livello all'asse (z=50) = {tab[50]:.0f} ml; alla soglia del troppopieno (z={Z_OVF:.0f}) = {tab[int(Z_OVF)]:.0f} ml; "
          f"per bagnare il giro piu' interno (z=28) = {tab[28]:.0f} ml  (pellicola, da togliere: fino a {film:.0f} ml)")
    print("   livello raggiunto da 250 / 350 / 500 / 550 ml:", [z_for(v) for v in (250, 350, 500, 550)], "mm dal fondo;",
          f"asse albero a {HC:.0f}, sommita' del divisorio a {Z_PART:.0f}")
print(f"volume della canaletta e del pescante (resta bagnato): {(GUT_X1 - GUT_X0) * (0 - GUT_Z) * (Y_DRV - Y_IDL) / 1000 + math.pi * tank.RIS_R ** 2 * (tank.RIS_ZT + 14 - GUT_Z) / 1000:.1f} ml")
