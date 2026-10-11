"""Verifiche del bagno (rilascio 6): inserimento ed estrazione verticale delle vaschette con l'innesto dal basso, fasi
dell'innesto (tenuta prima dell'apertura), livelli e volumi, galleggiamento e fermi girevoli, sensori di livello."""
import math
import cadquery as cq
from machine import *
import mrefs as R, hydro as H, machine_assembly as M

P = M.hydro_bodies(open_poppets=True)
vol = lambda a, b: a.val().intersect(b.val()).Volume()
def bbint(a, b):
    return not (a.xmin > b.xmax or b.xmin > a.xmax or a.ymin > b.ymax or b.ymin > a.ymax or a.zmin > b.zmax or b.zmin > a.zmax)

print("== inserimento ed estrazione delle vaschette (in verticale, fermi girevoli aperti) ==")
for k, c in enumerate(("C1", "C2", "C3")):
    mov_names = [f"REF_vaschetta_GN1-9_{c}", f"C11_corpo_innesto_{c}", f"C12_ghiera_codolo_{c}", f"REF_guarnizione_innesto_{c}"]
    if c == "C1": mov_names += ["REF_sonda_DS18B20_chimica", "C19_gancio_portasonda"]   # il gancio si toglie con la sonda
    fixed = {n: p for n, p in P.items() if n not in mov_names and not n.startswith(("C03_", "C13_", "REF_oring_otturatore"))}
    res = []
    for dz in (0.5, 2.0, 4.0, 8.0, 12.0, 16.0, 25.0, 60.0, 120.0, 200.0):
        pop = H.place_pan(H.poppet_local(max(0.0, 3.5 - dz)), k)                # finche' il perno la tocca resta aperto, poi si chiude
        tot, worst = 0.0, None
        for m in mov_names + ["pop"]:
            pm = (pop if m == "pop" else P[m]).translate((0, 0, dz)); bm = pm.val().BoundingBox()
            for f, pf in fixed.items():
                if not bbint(bm, pf.val().BoundingBox()): continue
                v = vol(pm, pf)
                if v > 1e-3:
                    tot += v
                    if worst is None or v > worst[1]: worst = (m + " / " + f, round(v, 2))
        res.append(f"{dz:.0f}:{tot:.2f}" + (f"({worst[0]})" if worst else ""))
    print(f"{c} sollevata di dz mm -> interferenze mm3: " + "  ".join(res))
    v = 0.0
    for rear in (False, True):
        tb = H.turn_button_local().rotate((0, 0, 0), (0, 0, 1), 0.0 if not rear else 180.0).translate((VAS_X[k], H.TB_YR if rear else H.TB_YF, RAIL_Z1))
        v += vol(tb, P[f"REF_vaschetta_GN1-9_{c}"].translate((0, 0, 1.0)))
    print(f"   fermi girevoli aperti (leva parallela al longherone) contro la vaschetta che sale: {v:.3f} mm3")

print("== fasi dell'innesto (quote dalla posizione in sede, vaschetta che scende) ==")
bore_top = H.SK_TOP - 5.0                                                        # fine dell'invito, inizio della sede diritta
oring_low = VAS_ZB + H.GR_Z[0]                                                    # fondo della gola dell'O-ring in sede
print(f"   la punta del codolo entra nell'invito {H.SK_TOP - (VAS_ZB + H.SP_Z0):.1f} mm prima della sede: l'invito centra la vaschetta entro ±5 mm")
print(f"   l'O-ring 13 x 2 entra nella sede diritta {bore_top - oring_low:.1f} mm prima della fine della corsa; "
      f"il perno tocca l'otturatore {H.PIN_TOP - (VAS_ZB + H.SP_Z0):.1f} mm prima: la tenuta c'e' gia' quando si apre")
print(f"   in sede l'O-ring sta {bore_top - (VAS_ZB + H.GR_Z[1]):.1f} mm sotto l'invito: la vaschetta puo' stare fino a "
      f"{bore_top - (VAS_ZB + H.GR_Z[1]):.1f} mm piu' in alto senza perdere la tenuta")
sqz = (2 * H.GR_R + 2 * 1.97 - 2 * H.SK_BR) / 2
print(f"   schiacciamento radiale dell'O-ring 13 x 2 (gola Ø{2 * H.GR_R:.1f}, sede Ø{2 * H.SK_BR:.1f}): {sqz:.2f} mm = {sqz / 1.97 * 100:.0f} %")
print(f"   otturatore aperto di 3.5 mm: passaggio anulare bocca Ø{2 * H.MOUTH_R:.0f} - naso Ø3.8 = {math.pi * (H.MOUTH_R ** 2 - 1.9 ** 2):.0f} mm2 "
      f"(un tubo da 3/8\" ne ha {math.pi * 3.175 ** 2:.0f})")
spring_closed = (VAS_ZB + GN9_WALL) - (VAS_ZB + H.POP_Z + 6.0)
print(f"   molla tra la crociera di C11 e il disco dell'otturatore: {spring_closed:.1f} mm a otturatore chiuso, {spring_closed - 3.5:.1f} aperto "
      f"(molla inox Ø7 x 32, filo 0,4: precarico circa 0,5 N)")

# ------------------------------------------------------------------ volumi, livelli
print("== volumi e livelli ==")
def pan_area(z_from_bottom, outer=True):
    t = z_from_bottom / (GN9_H - GN9_RIM_T)
    off = GN9_RIM + GN9_DRAFT * (1 - t) + (0 if outer else GN9_WALL)
    return (GN9_L - 2 * off) * (GN9_W - 2 * off)
def integ(f, a, b, n=400):
    h = (b - a) / n
    return sum(f(a + (i + 0.5) * h) for i in range(n)) * h
def pan_vol(z):                                                                  # litri nella vaschetta fino alla quota z (macchina)
    return integ(lambda q: pan_area(q, False), GN9_WALL, z - VAS_ZB) / 1e6
cap = pan_vol(VAS_ZR - GN9_RIM_T)
z1 = VAS_ZB + GN9_WALL
while pan_vol(z1) < 1.0: z1 += 0.5
resid = (pan_area(GN9_WALL, False) - math.pi * H.FL_R ** 2) * H.FL_T / 1e6
print(f"vaschetta GN 1/9 h150: capacita' al bordo {cap:.2f} l; 1 litro arriva a Z = {z1:.0f} ({z1 - VAS_ZB:.0f} mm dal fondo); "
      f"col pescaggio dal basso resta sul fondo solo lo strato alto quanto la flangia di C11 ({H.FL_T} mm): circa {resid * 1000:.0f} ml")
print(f"   sensore MAX della vaschetta a Z = {R.PMS_Z:.0f} (si bagna a circa {R.PMS_Z - 6:.0f}): {pan_vol(R.PMS_Z - 6):.2f} l; "
      f"sta {R.PMS_Z - 8 - (OVF_BATH_Z - 8):.0f} mm sopra il troppopieno del bagno: fuori dalla vaschetta li' l'acqua non arriva mai")
A_TUB = (IX1 - IX0) * (IY1 - IY0)
sock_v = P["C14_presa_C1"].val().Volume()
def bath_volume(z):
    v = A_TUB * (z - TUB_FLOOR)
    if z > VAS_ZB: v -= 3 * integ(lambda q: pan_area(q, True), 0.0, z - VAS_ZB)
    v -= 3 * sock_v * min(1.0, (z - TUB_FLOOR) / (H.SK_TOP - TUB_FLOOR))
    return v / 1e6
imm = WATER_Z - VAS_ZB
print(f"bagno: livello di lavoro Z = {WATER_Z:.0f} (vaschette immerse per {imm:.0f} mm) -> {bath_volume(WATER_Z):.1f} l d'acqua")
free = A_TUB - 3 * pan_area(imm, True)
print(f"   ogni litro prelevato per i risciacqui abbassa il livello di {1e6 / free:.1f} mm")
res = bath_volume(WATER_Z) - bath_volume(LVL_ZMIN)
print(f"   riserva tra livello di lavoro e sensore MIN (Z = {LVL_ZMIN:.0f}): {res:.1f} l = {res / 0.35:.0f} riempimenti da 350 ml")
print(f"   sensore MAX del bagno a Z = {LVL_ZMAX:.0f}: {bath_volume(LVL_ZMAX):.1f} l; il troppopieno comincia a Z = {OVF_BATH_Z - 8:.0f} "
      f"({bath_volume(OVF_BATH_Z - 8):.1f} l); bordo del contenitore a Z = {RIM_Z:.0f}")
print(f"   sommita' dei riscaldatori Z = {HEAT_Z + 6:.0f}; bocca del pescante WB Z = {WB_ZIN:.0f} (restano {bath_volume(WB_ZIN):.1f} l); "
      f"sensore MIN {LVL_ZMIN - HEAT_Z - 6:.0f} mm sopra i riscaldatori")
print(f"   beccuccio dell'acqua di rete a Z = {R.SPOUT_Z:.0f}: {R.SPOUT_Z - (OVF_BATH_Z - 8):.0f} mm d'aria sopra il troppopieno (non puo' risucchiare acqua del bagno)")
print(f"   rubinetto del bagno: foro a Z = {BD_Z:.0f}, pescante interno fino a Z = {TUB_FLOOR + 4.0:.0f}: aperto il rubinetto (tubo d'uscita piu' in basso "
      f"della macchina) il bagno si svuota per sifone; restano sul fondo circa {A_TUB * 4.0 / 1e6:.2f} l (senza pescante, col solo foro, ne resterebbero "
      f"{A_TUB * (BD_Z - 10.4 - TUB_FLOOR) / 1e6:.1f})")
print(f"   svuotamento di una vaschetta nella sua bottiglia: appoggiata sull'imbuto C20 (stesso perno della presa) si svuota "
      f"dall'innesto fino a circa {resid * 1000:.0f} ml")
print(f"   riscaldatori: uno a Y = {HEAT_Y[0]:.0f} e uno a Y = {HEAT_Y[1]:.0f} (distanza {HEAT_Y[1] - HEAT_Y[0]:.0f} mm), sotto le vaschette davanti e dietro; "
      f"sonda del bagno a X = {R.PROBE_X:.0f}, {IX1 - 100.0 - R.PROBE_X:.0f} mm dalla punta dei riscaldatori")
for dT, lab in ((18.0, "da 20 a 38 gradi"), (5.0, "da 33 a 38 gradi")):
    m = bath_volume(WATER_Z) + 3.0
    print(f"   riscaldamento {lab} di {m:.1f} kg (acqua + 3 l di chimica) con 200 W, senza perdite: {m * 4186 * dT / 200 / 60:.0f} minuti")
print("   con l'ingresso dell'acqua di rete conviene riempire con acqua gia' tiepida: i riscaldatori poi la mantengono")
print("== galleggiamento e fermi girevoli ==")
disp = integ(lambda q: pan_area(q, True), 0.0, imm) / 1e6
up = disp * 9.81
w = P["REF_vaschetta_GN1-9_C1"].val().Volume() / 1000 * 0.91 / 1000 * 9.81
print(f"vaschetta vuota in polipropilene al livello di lavoro: spinta {up:.1f} N, peso {w:.1f} N -> {up - w:.1f} N verso l'alto")
F = (up - w) / 2; L = H.TB_L - 3.5; b, t = H.TB_W, H.TB_T
sig = F * L / (b * t ** 2 / 6)
print(f"   due fermi girevoli (davanti e dietro): {F:.1f} N ciascuno, leva {L:.1f} x {b:.0f} x {t:.1f} mm -> {sig:.1f} MPa (PETG: circa 50), "
      f"freccia {F * L ** 3 / (3 * 2000 * b * t ** 3 / 12):.2f} mm")
print("== linee e camere di livello ==")
for k, c in enumerate(("C1", "C2", "C3")):
    L = R.line_len(k)
    print(f"   linea {c}: {L:.0f} mm di tubo 3/8\" ({math.pi * 3.175 ** 2 * L / 1000:.0f} ml); camera di livello {math.pi * H.CH_RI ** 2 * (CH_Z1 - CH_Z0 - 36.0) / 1000:.0f} ml")
print(f"   sommita' delle linee a ponte Z = {Z_BEND + BEND_R + PIPE_R:.1f} (piano e coperchio a Z = {Z_TOPW:.0f})")
