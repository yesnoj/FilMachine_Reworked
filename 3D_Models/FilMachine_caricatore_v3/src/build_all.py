"""Rigenera tutto: STEP/STL dei pezzi in orientamento di stampa, assiemi 135/120, verifiche, anteprime."""
import glob, subprocess, sys, io, contextlib
from common import *
import reel, guide, tower, drive, accessories, layout, assembly

for d in ("step", "stl"):
    for f in glob.glob(os.path.join(OUT, d, "*")):
        os.remove(f)

PARTS = [   # nome, forma (nel riferimento in cui e' modellata), rotazioni per la stampa
    ("01_flangia_A", reel.flange(), []), ("02_flangia_B", reel.flange(mirror=True), []),
    ("03_tubo_baionetta", reel.tube(), []), ("04_tamburo", reel.drum(), []),
    ("05_braccio_guida_135", guide.arm("135")[0], [("X", 180)]), ("06_braccio_guida_120", guide.arm("120")[0], [("X", 180)]),
    ("07_guancia_SX", tower.cheek(1), [("X", 90)]), ("08_guancia_DX", tower.cheek(-1), [("X", -90)]),
    ("09_ponte_lama", tower.bridge(), [("X", 90)]), ("10_braccio_lama", tower.swing_arm(), [("Y", -90)]),
    ("11_morsetto_lama", tower.blade_cap(), [("Y", -90)]), ("12_supporto_servo", tower.servo_bracket(), [("Y", -90)]),
    ("13_culla_135", tower.cradle135(), []), ("14_culla_120", tower.cradle120(), []),
    ("15_montante_motore", drive.motor_upright(), [("X", 90)]), ("16_montante_folle", drive.idle_upright(), [("X", -90)]),
    ("17_trascinatore", drive.drive_dog()[1], []), ("18_perno_folle", drive.idle_plug()[1], []),
    ("19_basamento", drive.base(), []), ("20_clip_ganascia", accessories.jaw(), []), ("21_clip_cursore", accessories.slider(), []),
    ("22_anello_alette_A", reel.fin_ring("A"), []), ("22_anello_alette_B", reel.fin_ring("B"), []),
    ("23_provino_accoppiamento", reel.fit_gauge(), []),
]
log = io.StringIO()
def out(*a):
    print(*a); print(*a, file=log)

out("== PEZZI (orientamento di stampa) ==")
for name, shape, rots in PARTS:
    s = to_print(shape, rots).val()
    cq.exporters.export(cq.Workplane("XY").add(s), os.path.join(OUT, "step", name + ".step"))
    cq.exporters.export(cq.Workplane("XY").add(s), os.path.join(OUT, "stl", name + ".stl"), tolerance=0.02, angularTolerance=0.15)
    bb = s.BoundingBox()
    out(f"{name:24s} valido={s.isValid()} solidi={len(s.Solids())} volume={s.Volume()/1000:6.1f} cm3  ingombro {bb.xlen:6.1f} x {bb.ylen:6.1f} x {bb.zlen:5.1f} mm")
    assert s.isValid() and len(s.Solids()) == 1, name

out("\n== SPIRALE ==")
D = PITCH / 2
out("capacita' pellicola:", round(sum(math.pi * (RIB_R0 + D * k) for k in range(HALF_TURNS - 2))), "mm | passo", PITCH,
    "| canale tra costole", round(PITCH - RIB_W, 2), "| fascetta (300 gradi sul tamburo)", round(math.radians(300) * CORE_R, 1), "mm")
for fmt in ("135", "120"):
    c, sag, R = arch(fmt)
    out(f"{fmt}: distanza tra le flange {reel_width(fmt):.1f} | luce tra le punte delle costole {reel_width(fmt)-2*RIB_H:.1f} | "
        f"arco pellicola nel braccio: corda {c:.1f}, freccia {sag:.2f}, raggio {R:.1f} mm")
for script in ("check_reel.py", "layout.py", "check_tower.py"):
    r = subprocess.run([sys.executable, script], capture_output=True, text=True)
    out(f"\n== {script} ==\n" + "\n".join(l for l in r.stdout.splitlines() if l.strip()))
    if r.returncode != 0: out("ERRORE:", r.stderr[-800:])

out("\n== ASSIEMI ==")
for fmt in ("135", "120"):
    P = assembly.build(fmt)
    path = assembly.save(fmt, P)
    bad = assembly.interferences(P)
    out(f"ASSIEME_{fmt}.step: {len(P)} corpi, {os.path.getsize(path)/1e6:.1f} MB, coppie in interferenza: {bad if bad else 'nessuna'}")
    names = list(P)
    cols = ["#%02x%02x%02x" % tuple(int(255 * c) for c in assembly.COL[n[:2]]) for n in names]
    render([P[n] for n in names], f"assieme_{fmt}_vista", elev=22, azim=-118, colors=cols, size=(12, 9))
    keep = [n for n in names if not n.startswith(("08", "16", "19"))]
    render([P[n] for n in keep], f"assieme_{fmt}_spaccato", elev=10, azim=-92,
           colors=["#%02x%02x%02x" % tuple(int(255 * c) for c in assembly.COL[n[:2]]) for n in keep], size=(12, 8))
open(os.path.join(OUT, "verifiche.txt"), "w").write(log.getvalue())
