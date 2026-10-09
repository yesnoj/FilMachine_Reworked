"""Dima della base: pianta in scala 1:1 (PDF 440 x 525 mm) con le impronte dei pezzi che poggiano sulla tavola e
i centri dei fori per le viti; elenco dei fori in CSV."""
import os, csv
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
import cadquery as cq
from OCP.BRepTools import BRepTools_WireExplorer
from OCP.TopAbs import TopAbs_REVERSED
from machine import *
import machine_assembly as M, mrefs as R

def footprints(P, z=2.0):
    pl = cq.Face.makePlane(2000, 2000, basePnt=(0, 0, z), dir=(0, 0, 1))
    out, holes = [], []
    for n, p in P.items():
        bb = p.val().BoundingBox()
        if n.startswith("REF_base") or bb.zmin > z or bb.zmax < z: continue
        try: sec = p.val().intersect(pl)
        except Exception: continue
        for f in sec.Faces():
            polys = []
            for i, w in enumerate([f.outerWire()] + f.innerWires()):
                es = w.Edges()
                if i and len(es) == 1 and es[0].geomType() == "CIRCLE":
                    c = es[0].Center(); holes.append((n, round(c.x, 1), round(c.y, 1), round(2 * es[0].radius(), 1)))
                pts = []; ex = BRepTools_WireExplorer(w.wrapped)
                while ex.More():
                    e = cq.Edge(ex.Current()); m = 24 if e.geomType() != "LINE" else 1
                    sg = [e.positionAt(j / m).toTuple() for j in range(m + 1)]
                    if ex.Orientation() == TopAbs_REVERSED: sg = sg[::-1]
                    pts += sg; ex.Next()
                polys.append(pts)
            out.append((n, polys))
    return out, holes

def draw(P, path_base):
    fp, holes = footprints(P)
    W_, H_ = 440.0, 525.0
    fig = plt.figure(figsize=(W_ / 25.4, H_ / 25.4))
    ax = fig.add_axes([0, 0, 1, 1]); ax.set_xlim(-10, W_ - 10); ax.set_ylim(-10, H_ - 10); ax.set_aspect("equal"); ax.axis("off")
    ax.add_patch(Rectangle((0, 0), BASE_X, BASE_Y, fc="none", ec="black", lw=1.2))
    for v in range(0, int(BASE_X) + 1, 10):
        ax.plot([v, v], [0, BASE_Y], color="#dddddd" if v % 50 else "#aaaaaa", lw=0.3 if v % 50 else 0.6, zorder=0)
        if v % 50 == 0: ax.text(v, -4, str(v), fontsize=7, ha="center", va="top")
    for v in range(0, int(BASE_Y) + 1, 10):
        ax.plot([0, BASE_X], [v, v], color="#dddddd" if v % 50 else "#aaaaaa", lw=0.3 if v % 50 else 0.6, zorder=0)
        if v % 50 == 0: ax.text(-3, v, str(v), fontsize=7, ha="right", va="center")
    seen = set()
    for n, polys in fp:
        col = M.color_of(n)
        for i, pts in enumerate(polys):
            ax.fill([q[0] for q in pts], [q[1] for q in pts], facecolor="white" if i else col, edgecolor="black", lw=0.5, alpha=1.0 if i else 0.35)
        key = n.rsplit("_", 1)[0] if n[-1].isdigit() else n
        if key not in seen:
            seen.add(key); xs = [q[0] for q in polys[0]]; ys = [q[1] for q in polys[0]]
            ax.text((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, key.replace("REF_", ""), fontsize=6, ha="center", va="center", rotation=90 if max(ys) - min(ys) > 2 * (max(xs) - min(xs)) else 0)
    # impronta della pompa (appoggia sui gommini) e dei suoi quattro fori indicativi
    ax.add_patch(Rectangle((PUMP_XC - PUMP_W / 2, PUMP_Y0 + 30), PUMP_W, 80, fc="none", ec="#7a6a4f", lw=1.0, ls="--"))
    ax.text(PUMP_XC, PUMP_Y0 + 70, "piede della pompa UP3-R\n(forare secondo le asole della pompa)", fontsize=6.5, ha="center", va="center", color="#7a6a4f")
    for n, x, y, d in holes:
        ax.plot([x - 4, x + 4], [y, y], color="red", lw=0.5); ax.plot([x, x], [y - 4, y + 4], color="red", lw=0.5)
        ax.text(x + 3, y + 3, f"{x:g}; {y:g}", fontsize=5, color="red")
    ax.text(5, BASE_Y + 6, "FilMachine - dima della base 420 x 505 mm, scala 1:1 (stampare senza adattare alla pagina; controllare il riquadro da 100 mm). "
            "Croci rosse = viti da legno 3,5 (preforo Ø2,5 non passante); coordinate X; Y in mm dall'angolo anteriore sinistro.", fontsize=7.5)
    ax.plot([300, 400], [BASE_Y - 12, BASE_Y - 12], color="black", lw=1.5); ax.text(350, BASE_Y - 10, "100 mm", fontsize=7, ha="center", va="bottom")
    ax.text(BASE_X / 2, 3, "FRONTE", fontsize=9, ha="center", weight="bold"); ax.text(BASE_X / 2, BASE_Y - 26, "RETRO", fontsize=9, ha="center", weight="bold")
    fig.savefig(path_base + ".pdf"); fig.savefig(path_base + ".png", dpi=60); plt.close(fig)
    with open(path_base + "_fori.csv", "w", newline="") as f:
        w = csv.writer(f, delimiter=";"); w.writerow(["pezzo", "X_mm", "Y_mm", "foro_nel_pezzo_mm", "preforo_nel_legno_mm"])
        for h in sorted(holes, key=lambda h: (h[0], h[1], h[2])): w.writerow(h + (2.5,))
    return len(holes)

if __name__ == "__main__":
    P = {}
    for f in (M.hydro_bodies, M.tank_bodies, M.stand_bodies, M.electronics_bodies): P.update(f())
    n = draw(P, os.path.join(OUT, "docs", "dima_base"))
    print("dima della base:", n, "fori")
