"""Controllo delle viti sull'assieme (le viti non sono modellate, quindi il controllo delle interferenze non le vede):
1) attorno a ogni foro per vite autofilettante deve esserci parete piena;
2) ogni foro M3, passante o filettato, deve avere sullo stesso asse il suo compagno in un altro pezzo stampato, oppure
   finire in un componente acquistato (base, longheroni, motore, servo...), oppure essere uno dei casi previsti qui sotto;
3) la lunghezza di ogni vite autofilettante tra due pezzi stampati (tabella SCREW) deve dare almeno 3.5 mm di presa senza
   toccare il fondo del foro cieco e, nei fori passanti, senza che la punta arrivi contro un altro corpo."""
import collections
import cadquery as cq
from OCP.BRepAdaptor import BRepAdaptor_Surface
from OCP.GeomAbs import GeomAbs_Cylinder
import machine_assembly as M

WALL = 1.2                                  # parete minima attorno a un foro filettato M3 (0.8 attorno a un M2)
# fori senza compagno previsti dal disegno: (codice del pezzo, tipo) -> motivo; tipo: "f" filettato, "p" passante, "*" tutti
EXPECTED = {("A04", "*"): "spina di filamento che ferma la fascetta", ("A07", "*"): "perno della clip",
            ("B10", "*"): "perno di manovella: vite M2,5 nella squadretta del servo", ("B25", "f"): "vite del modulo Hall",
            ("C05", "*"): "viti di pressione M4 sotto la fascia del contenitore", ("D01", "f"): "viti dei piedini della tank (asole nel corpo)",
            ("D09", "f"): "colonnette della scheda display (viti con rondella)", ("D12", "*"): "griglia dei ripiani",
            ("D15", "*"): "morsetti universali, in un punto qualsiasi della griglia"}
# pareti ridotte previste dal disegno
THIN_OK = {"A04": "il foro della spina taglia apposta la fessura della fascetta", "A07": "perno della clip (non e' una vite)",
           "B11": "sede del braccio della squadretta del servo"}

# lunghezza delle viti autofilettanti da 3 mm: (pezzo sotto la testa, primo pezzo in cui la vite fa presa) -> mm
SCREW = {("B03", "B01"): 8, ("B04", "B01"): 10, ("B05", "B01"): 10, ("B06", "B04"): 16, ("B06", "B05"): 16, ("B12", "B01"): 10,
         ("B17", "B01"): 8, ("B25", "B01"): 10, ("B23", "B01"): 10, ("C03", "C02"): 10, ("D01", "D02"): 10, ("D01", "D03"): 10,
         ("D04", "D02"): 10, ("D04", "D03"): 10, ("D05", "D01"): 10, ("D08", "D06"): 8, ("D08", "D07"): 8, ("D09", "D06"): 8,
         ("D09", "D07"): 8, ("D10", "D06"): 8, ("D10", "D07"): 8, ("D11", "D06"): 8, ("D11", "D07"): 8, ("D17", "D09"): 8}
GRIP_MIN, BOTTOM_GAP = 3.5, 0.3

def holes(solid, rmin=0.6, rmax=2.3):
    """Fori cilindrici piccoli di un solido: (piede dell'asse, direzione, inizio, fine, raggio)."""
    out = {}
    for f in solid.Faces():
        ad = BRepAdaptor_Surface(f.wrapped)
        if ad.GetType() != GeomAbs_Cylinder: continue
        cyl = ad.Cylinder(); r = cyl.Radius()
        if not rmin <= r <= rmax: continue
        ax = cyl.Axis(); o = cq.Vector(ax.Location().X(), ax.Location().Y(), ax.Location().Z())
        d = cq.Vector(ax.Direction().X(), ax.Direction().Y(), ax.Direction().Z())
        p = ad.Value((ad.FirstUParameter() + ad.LastUParameter()) / 2, (ad.FirstVParameter() + ad.LastVParameter()) / 2)
        pt = cq.Vector(p.X(), p.Y(), p.Z()); rad = (pt - o) - d * ((pt - o).dot(d))
        if rad.dot(f.normalAt(pt)) > 0: continue                                   # e' un perno, non un foro
        if d.x + d.y * 1.0001 + d.z * 1.0002 < 0: d = d * -1                       # verso dell'asse sempre lo stesso
        ts = [cq.Vector(*v.toTuple()).dot(d) for v in f.Vertices()]
        s0, s1 = min(ts), max(ts)
        if s1 - s0 < 1.5: continue
        o0 = o - d * o.dot(d)
        key = (round(r, 2), round(d.x, 2), round(d.y, 2), round(d.z, 2), round(o0.x, 1), round(o0.y, 1), round(o0.z, 1))
        for seg in out.setdefault(key, []):                                        # le due meta' di uno stesso foro
            if s0 <= seg[3] + 0.3 and s1 >= seg[2] - 0.3:
                seg[2], seg[3] = min(seg[2], s0), max(seg[3], s1); break
        else:
            out[key].append([o0, d, s0, s1, r])
    return [tuple(s) for lst in out.values() for s in lst]

def wall_fraction(solid, o, d, s0, s1, r, t):
    """Quota di materiale pieno in un anello di spessore t attorno al foro (1 = parete completa)."""
    L = s1 - s0 - 0.4
    if L <= 0.5: return 1.0
    base = o + d * (s0 + 0.2)
    ring = cq.Solid.makeCylinder(r + t, L, base, d).cut(cq.Solid.makeCylinder(r + 0.02, L, base, d))
    return solid.intersect(ring).Volume() / ring.Volume()

def kind(r):
    return "f" if r <= 1.36 else "p"

def inside_ref(P, bbs, pt, only_ref=True, skip=()):
    for n, bb in bbs.items():
        if (only_ref and not n.startswith("REF_")) or n in skip: continue
        if not (bb.xmin - 0.1 <= pt.x <= bb.xmax + 0.1 and bb.ymin - 0.1 <= pt.y <= bb.ymax + 0.1 and bb.zmin - 0.1 <= pt.z <= bb.zmax + 0.1): continue
        for s in P[n].val().Solids():
            if s.isInside(pt, 0.05): return n
    return None

if __name__ == "__main__":
    P = {}
    for f in (M.hydro_bodies, M.tank_bodies, M.stand_bodies, M.electronics_bodies): P.update(f())
    bbs = {n: b.val().BoundingBox() for n, b in P.items()}
    H = [(n,) + h for n, b in P.items() if not n.startswith("REF_") for h in holes(b.val())]
    print(f"fori per viti nei pezzi stampati dell'assieme: {len(H)} ({sum(1 for h in H if kind(h[5]) == 'f')} filettati, {sum(1 for h in H if kind(h[5]) == 'p')} passanti)")
    # ---- 1) pareti attorno ai fori filettati (una volta per pezzo diverso)
    seen, thin, thin_ok = set(), [], collections.Counter()
    for n, o, d, s0, s1, r in H:
        code = n[:3]
        if kind(r) != "f" or (code, round(r, 2), round(s1 - s0, 1), round(o.x, 0), round(o.y, 0), round(o.z, 0)) in seen: continue
        if n[-1].isdigit() and not n.endswith(("_1", "_C1")) and n[-2] in "_C": continue          # gli altri esemplari dello stesso pezzo
        seen.add((code, round(r, 2), round(s1 - s0, 1), round(o.x, 0), round(o.y, 0), round(o.z, 0)))
        fr = wall_fraction(P[n].val(), o, d, s0, s1, r, WALL if r > 1.0 else 0.8)
        if fr < 0.965:
            if code in THIN_OK: thin_ok[code] += 1
            else:
                c = o + d * ((s0 + s1) / 2); thin.append(f"{n}: foro Ø{2 * r:.1f} in ({c.x:.1f}, {c.y:.1f}, {c.z:.1f}), parete piena solo per il {100 * fr:.0f} %")
    print(f"pareti attorno ai fori filettati (anello di {WALL} mm): fori controllati {len(seen)}, pareti sottili {len(thin)}")
    for t in thin: print("   ANOMALIA", t)
    for c, k in sorted(thin_ok.items()): print(f"   previsto: {c} x{k}, {THIN_OK[c]}")
    # ---- 2) compagni coassiali
    pairs, in_ref, expected, odd = 0, collections.Counter(), collections.Counter(), []
    for i, (n, o, d, s0, s1, r) in enumerate(H):
        if not (1.2 <= r <= 1.36 or 1.6 <= r <= 1.76 or n[:3] in ("A04", "A07", "B10")): continue
        found = False
        for j, (n2, o2, d2, a0, a1, r2) in enumerate(H):
            if j == i or n2 == n or abs(d.dot(d2)) < 0.9995: continue
            if (o2 - o).Length > 0.3: continue                                    # piedi degli assi (stessa direzione): rette coincidenti
            if max(a0 - s1, s0 - a1) < 4.0: found = True; break
        if found:
            pairs += 1; continue
        ref = None
        for e in (s0 - 0.7, s1 + 0.7, s0 - 2.0, s1 + 2.0):
            ref = inside_ref(P, bbs, o + d * e)
            if ref: break
        code = n[:3]
        if ref: in_ref[ref.split("_")[1]] += 1
        elif (code, kind(r)) in EXPECTED or (code, "*") in EXPECTED: expected[code] += 1
        else:
            c = o + d * ((s0 + s1) / 2); odd.append(f"{n}: foro Ø{2 * r:.1f} {'filettato' if kind(r) == 'f' else 'passante'} in ({c.x:.1f}, {c.y:.1f}, {c.z:.1f}) senza compagno")
    print(f"fori M3 con il compagno sullo stesso asse in un altro pezzo stampato: {pairs}")
    print("fori che finiscono in un componente acquistato: " + ", ".join(f"{k} {v}" for k, v in sorted(in_ref.items())))
    print("fori senza compagno previsti dal disegno:")
    for c, k in sorted(expected.items()):
        why = EXPECTED.get((c, "*")) or EXPECTED.get((c, "f")) or EXPECTED.get((c, "p"))
        print(f"   {c} x{k}: {why}")
    print("fori senza compagno NON previsti:", "nessuno" if not odd else len(odd))
    for t in odd: print("   ANOMALIA", t)
    # ---- 3) lunghezza delle viti autofilettanti tra pezzi stampati
    lines = []                                                                    # rette: [direzione, piede, [(nome, s0, s1, r)]]
    for n, o, d, s0, s1, r in H:
        if not (1.2 <= r <= 1.36 or 1.6 <= r <= 1.76) or n[:3] in ("D12", "D13", "D14", "D15"): continue
        for ln in lines:
            if abs(d.dot(ln[0])) > 0.9995 and (o - ln[1]).Length < 0.3:
                ln[2].append((n, s0, s1, r)); break
        else:
            lines.append([d, o, [(n, s0, s1, r)]])
    rows, bad3, per_len = collections.OrderedDict(), [], collections.Counter()
    for d, o, segs in lines:
        segs.sort(key=lambda t: t[1]); cl = [[segs[0]]]
        for sg in segs[1:]:
            if sg[1] - max(t[2] for t in cl[-1]) > 4.0: cl.append([sg])
            else: cl[-1].append(sg)
        for c in cl:
            taps = [t for t in c if t[3] <= 1.36]; clrs = [t for t in c if t[3] >= 1.6]
            if not taps or not clrs: continue
            t0, t1 = min(t[1] for t in taps), max(t[2] for t in taps)
            if all(t[2] <= t0 + 0.3 for t in clrs):   sgn, head, first, far, last = 1, min(t[1] for t in clrs), min(clrs, key=lambda t: t[1]), t1, max(taps, key=lambda t: t[2])
            elif all(t[1] >= t1 - 0.3 for t in clrs): sgn, head, first, far, last = -1, max(t[2] for t in clrs), max(clrs, key=lambda t: t[2]), t0, min(taps, key=lambda t: t[1])
            else:
                bad3.append(f"{c[0][0]}: fori passanti dai due lati di un foro filettato"); continue
            stack = (t0 - head) if sgn > 0 else (head - t1); depth = t1 - t0
            tap1 = min(taps, key=lambda t: t[1]) if sgn > 0 else max(taps, key=lambda t: t[2])
            key = (first[0][:3], tap1[0][:3]); L = SCREW.get(key)
            pm = o + d * ((t0 + t1) / 2); where = f"{first[0]} -> {tap1[0]} in ({pm.x:.0f}, {pm.y:.0f}, {pm.z:.0f})"
            if L is None:
                bad3.append(f"{where}: giunzione senza lunghezza in tabella"); continue
            grip = L - stack; blind = any(sol.isInside(o + d * (far + sgn * 0.3), 0.05) for sol in P[last[0]].val().Solids())
            per_len[L] += 1
            note = ""
            if grip < GRIP_MIN: note = "PRESA SCARSA"
            if blind and grip > depth - BOTTOM_GAP: note = "TOCCA IL FONDO"
            if not blind and grip > depth:
                k = 0.5
                while k <= grip - depth + 0.01:
                    hit = inside_ref(P, bbs, o + d * (far + sgn * k), only_ref=False, skip=(last[0],))
                    if hit: note = f"LA PUNTA TOCCA {hit}"; break
                    k += 0.5
            if note: bad3.append(f"{where}: vite 3 x {L}, sotto la testa {stack:.1f}, presa {grip:.1f}, foro {'cieco' if blind else 'passante'} da {depth:.1f}: {note}")
            r = rows.setdefault(key, [0, L, stack, grip, depth, blind])
            r[0] += 1; r[2] = max(r[2], stack); r[3] = min(r[3], grip)
    print("viti autofilettanti da 3 mm tra pezzi stampati (lunghezza dalla tabella SCREW):")
    for (a, b), (k, L, stack, grip, depth, blind) in sorted(rows.items()):
        tail = f"foro cieco da {depth:.1f} (restano {depth - grip:.1f})" if blind else (f"foro passante da {depth:.1f}" + (f", la punta esce di {grip - depth:.1f}" if grip > depth else ""))
        print(f"   {a} -> {b}  x{k:<2d} 3 x {L:<2d}  sotto la testa {stack:4.1f}  presa {grip:4.1f}  {tail}")
    print("   piedini della tank -> D01  x4  3 x 16  sotto la testa  5.0 (asola nel piedino)  presa 11.0  foro passante da 12.0")
    per_len[16] += 4
    print("   totale per lunghezza: " + ", ".join(f"3 x {L}: {k}" for L, k in sorted(per_len.items())))
    print("lunghezze da correggere:", "nessuna" if not bad3 else len(bad3))
    for t in bad3: print("   ANOMALIA", t)
