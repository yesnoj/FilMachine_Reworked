"""Controllo degli sbalzi sugli STL (gia' nell'orientamento di stampa): area delle facce rivolte in basso con piu' di
45 gradi dalla verticale e non appoggiate sul piatto. Distingue i ponti (appoggiati ai due lati) non lo sa fare:
elenca l'area e l'intervallo di quota, per decidere a occhio."""
import glob, os, sys
import numpy as np
from stlcheck import read_stl
from common import OUT

def overhang(path, ang=45.0, zmin=0.25):
    T = read_stl(path)
    n = np.cross(T[:, 1] - T[:, 0], T[:, 2] - T[:, 0]); a = np.linalg.norm(n, axis=1) / 2
    nz = np.divide(n[:, 2], 2 * a, out=np.zeros(len(a)), where=a > 0)
    zc = T[:, :, 2].min(axis=1)
    m = (nz < -np.cos(np.radians(90 - ang - 0.5))) & (zc > zmin)           # piu' piatte di 45 gradi (mezzo grado di tolleranza: le falde a 45 esatti vanno bene)
    flat = (nz < -0.999) & (zc > zmin)
    if not m.any(): return 0.0, 0.0, None
    return a[m].sum(), a[flat].sum(), (zc[m].min(), T[m][:, :, 2].max())

if __name__ == "__main__":
    rows = []
    for p in sorted(glob.glob(os.path.join(OUT, "stl", "*.stl"))):
        tot, flat, zr = overhang(p)
        rows.append((os.path.basename(p)[:-4], tot, flat, zr))
    print("== sbalzi oltre 45 gradi (mm2): totale | di cui orizzontali (ponti o tetti) | quote ==")
    for n, tot, flat, zr in sorted(rows, key=lambda r: -r[1]):
        if tot > 1.0:
            print(f"   {n:32s} {tot:8.0f} | {flat:8.0f} | z {zr[0]:.1f}..{zr[1]:.1f}")
    print("   pezzi senza sbalzi:", sum(1 for r in rows if r[1] <= 1.0), "su", len(rows))
