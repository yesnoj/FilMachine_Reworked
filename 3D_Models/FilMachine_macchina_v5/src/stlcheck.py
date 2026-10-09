"""Controllo degli STL: rete chiusa (ogni spigolo condiviso da due triangoli) e volume confrontato col solido."""
import struct, sys, collections
import numpy as np

def read_stl(path):
    d = open(path, "rb").read()
    n = struct.unpack("<I", d[80:84])[0]
    if len(d) == 84 + 50 * n:
        a = np.frombuffer(d, dtype=np.dtype([("n", "<f4", 3), ("v", "<f4", (3, 3)), ("a", "<u2")]), count=n, offset=84)
        return a["v"].astype(np.float64)
    tri = []; cur = []
    for line in d.decode("ascii", "ignore").splitlines():
        p = line.split()
        if p[:1] == ["vertex"]:
            cur.append([float(x) for x in p[1:4]])
            if len(cur) == 3: tri.append(cur); cur = []
    return np.array(tri)

def check(path):
    T = read_stl(path)
    V = np.round(T.reshape(-1, 3), 4)
    _, idx = np.unique(V, axis=0, return_inverse=True)
    F = idx.reshape(-1, 3)
    E = collections.Counter()
    for a, b in ((0, 1), (1, 2), (2, 0)):
        for u, v in zip(F[:, a], F[:, b]):
            E[(min(u, v), max(u, v))] += 1
    open_e = sum(1 for c in E.values() if c == 1); multi = sum(1 for c in E.values() if c > 2)
    vol = abs(np.einsum("ij,ij->i", T[:, 0], np.cross(T[:, 1], T[:, 2])).sum() / 6.0)
    return len(T), open_e, multi, vol

if __name__ == "__main__":
    for p in sys.argv[1:]:
        n, o, m, v = check(p)
        print(f"{p.split('/')[-1]:40s} triangoli {n:7d}  spigoli aperti {o}  spigoli multipli {m}  volume {v / 1000:.2f} cm3")
