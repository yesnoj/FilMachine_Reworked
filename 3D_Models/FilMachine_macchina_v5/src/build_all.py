"""Rigenera tutto: STEP/STL dei pezzi in orientamento di stampa, assieme unico, verifiche, anteprime."""
import glob, subprocess, sys, io, time, json
import matplotlib.colors as mc
from common import *
import parts, machine_assembly as M, viz, stlcheck

t0 = time.time()
for d in ("step", "stl"):
    for f in glob.glob(os.path.join(OUT, d, "*")):
        os.remove(f)
log = io.StringIO()
def out(*a):
    print(*a); print(*a, file=log); sys.stdout.flush()

out("== PEZZI DA STAMPARE (orientamento di stampa; ingombro in mm) ==")
table = []
for name, shape, rots, q, note in parts.all_parts():
    s = to_print(shape, rots).val()
    ok = s.isValid() and len(s.Solids()) == 1; vol = s.Volume(); bb = s.BoundingBox()
    cq.exporters.export(cq.Workplane("XY").add(s), os.path.join(OUT, "step", name + ".step"))
    stl = os.path.join(OUT, "stl", name + ".stl")
    cq.exporters.export(cq.Workplane("XY").add(s), stl, tolerance=0.02, angularTolerance=0.15)
    ntri, open_e, multi, vstl = stlcheck.check(stl)
    closed = open_e == 0 and multi == 0 and abs(vstl - vol) / vol < 0.01
    out(f"{name:32s} x{q:<2d} solido {'ok' if ok else 'NON VALIDO'}  STL {'chiuso' if closed else 'DA CONTROLLARE'} ({ntri:6d} triangoli)  "
        f"{vol / 1000:6.1f} cm3  {bb.xlen:6.1f} x {bb.ylen:6.1f} x {bb.zlen:5.1f}")
    assert ok and closed, name
    table.append({"nome": name, "q": q, "volume_cm3": round(vol / 1000, 1), "ingombro": [round(bb.xlen, 1), round(bb.ylen, 1), round(bb.zlen, 1)], "note": note})
json.dump(table, open(os.path.join(OUT, "docs", "_pezzi.json"), "w"), indent=1, ensure_ascii=False)
out(f"totale: {len(table)} pezzi diversi, {sum(max(r['q'], 0) for r in table)} da stampare, {sum(r['volume_cm3'] * max(r['q'], 0) for r in table):.0f} cm3 di materiale pieno")

for script in ("overhang.py", "check_reel.py", "check_tank.py", "check_bath.py", "check_screws.py", "cutlist.py"):
    r = subprocess.run([sys.executable, script], capture_output=True, text=True)
    out(f"\n== {script} ==\n" + "\n".join(l for l in r.stdout.splitlines() if l.strip()))
    if r.returncode != 0: out("ERRORE:", r.stderr[-800:])

out("\n== ASSIEME UNICO ==")
P = {}
for f in (M.hydro_bodies, M.tank_bodies, M.stand_bodies, M.electronics_bodies): P.update(f())
inv = [n for n, p in P.items() if not p.val().isValid()]
bad = M.interferences(P, skip=M.SKIP)
bbs = [p.val().BoundingBox() for n, p in P.items()]
out(f"{len(P)} corpi ({sum(1 for n in P if not n.startswith('REF_'))} stampati, {sum(1 for n in P if n.startswith('REF_'))} sagome di componenti acquistati); "
    f"non validi: {inv if inv else 'nessuno'}; coppie in interferenza: {bad if bad else 'nessuna'}")
out(f"ingombro: X {min(b.xmin for b in bbs):.0f}..{max(b.xmax for b in bbs):.0f}, Y {min(b.ymin for b in bbs):.0f}..{max(b.ymax for b in bbs):.0f}, "
    f"Z {min(b.zmin for b in bbs):.0f}..{max(b.zmax for b in bbs):.0f} mm")
asm = cq.Assembly(name="FilMachine")
for n, p in P.items():
    asm.add(p.val(), name=n, color=cq.Color(*mc.to_rgb(M.color_of(n))))
path = os.path.join(OUT, "step", "ASSIEME_FilMachine.step")
asm.export(path)
out(f"ASSIEME_FilMachine.step: {os.path.getsize(path) / 1e6:.1f} MB")
open(os.path.join(OUT, "verifiche.txt"), "w").write(log.getvalue())
print(f"fatto in {time.time() - t0:.0f} s")
