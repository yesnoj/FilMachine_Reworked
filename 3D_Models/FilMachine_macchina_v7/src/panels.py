"""Disegni dei pannelli da tagliare (Dibond 3 mm, PMMA 4 e 3 mm, piano in multistrato): un DXF per pannello (per il taglio CNC o laser, in mm,
vista dall'esterno) e un PDF riassuntivo con quote e fori, in docs/pannelli/."""
import os, math
import cadquery as cq
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from common import OUT
import shell as S

D = os.path.join(OUT, "docs", "pannelli"); os.makedirs(D, exist_ok=True)
# nome, funzione, materiale, (asse orizzontale, asse verticale) della vista dalla faccia ESTERNA (normale uscente = u x v)
import mrefs as R
PANELS = [("pannello_frontale", S.panel_front, "Dibond 3 mm, grigio chiaro o alluminio spazzolato", ((1, 0, 0), (0, 0, 1))),
          ("pannello_posteriore", S.panel_rear, "Dibond 3 mm, come il frontale", ((-1, 0, 0), (0, 0, 1))),
          ("pannello_sinistro", S.panel_left, "Dibond 3 mm, come il frontale", ((0, -1, 0), (0, 0, 1))),
          ("pannello_destro", S.panel_right, "Dibond 3 mm, come il frontale", ((0, 1, 0), (0, 0, 1))),
          ("pannello_copertura", S.panel_top, "Dibond 3 mm antracite", ((1, 0, 0), (0, 1, 0))),
          ("bagno_fondo", R.bath_floor, "PMMA trasparente 4 mm", ((1, 0, 0), (0, 1, 0))),
          ("bagno_fronte", R.bath_front, "PMMA trasparente 4 mm, bordi lucidati (e' a vista)", ((1, 0, 0), (0, 0, 1))),
          ("bagno_retro", R.bath_back, "PMMA trasparente 4 mm", ((-1, 0, 0), (0, 0, 1))),
          ("bagno_destra", R.bath_right, "PMMA trasparente 4 mm", ((0, 1, 0), (0, 0, 1))),
          ("bagno_sinistra", R.bath_left, "PMMA trasparente 4 mm (finestra della testata e fori delle viti della cornice)", ((0, -1, 0), (0, 0, 1))),
          ("bagno_coperchio", R.bath_cover, "PMMA trasparente 3 mm, bordi smussati", ((1, 0, 0), (0, 1, 0))),
          ("piano_multistrato", R.deck, "Multistrato 12 mm (fori dei passaparete e dei cavi, angoli tagliati)", ((1, 0, 0), (0, 1, 0)))]
AX = {"X": (1, 0, 0), "Y": (0, 1, 0), "Z": (0, 0, 1)}

def flat(f, uv):
    """Porta il pannello nel piano XY con la faccia esterna verso +Z: X = u, Y = v (nessuna specchiatura)."""
    u, v = uv
    n = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
    from OCP.gp import gp_Trsf
    from OCP.BRepBuilderAPI import BRepBuilderAPI_Transform
    t = gp_Trsf(); t.SetValues(*map(float, (u[0], u[1], u[2], 0, v[0], v[1], v[2], 0, n[0], n[1], n[2], 0)))
    w = cq.Workplane("XY").add(cq.Shape.cast(BRepBuilderAPI_Transform(f().val().wrapped, t, True).Shape()))
    bb = w.val().BoundingBox()
    return w.translate((-bb.xmin, -bb.ymin, -bb.zmin))

def outline(w):
    """Contorno esterno e fori della faccia superiore (esterna) del pannello appiattito."""
    top = w.faces(">Z").val()
    def pts(wire):                                                             # un segmento per lato (l'ordine dei lati non conta)
        p = []
        for e in wire.Edges():
            n = 24 if e.geomType() != "LINE" else 1
            p.append([e.positionAt(i / n).toTuple()[:2] for i in range(n + 1)])
        return p
    holes = []
    for iw in top.innerWires():
        es = iw.Edges()
        if len(es) == 1 and es[0].geomType() == "CIRCLE":
            c = es[0].Center(); holes.append(("cerchio", round(c.x, 1), round(c.y, 1), round(2 * es[0].radius(), 1), pts(iw)))
        else:
            bb = iw.BoundingBox(); holes.append(("apertura", round(bb.xmin, 1), round(bb.ymin, 1), (round(bb.xlen, 1), round(bb.ylen, 1)), pts(iw)))
    return top, pts(top.outerWire()), holes

if __name__ == "__main__":
    pdf = PdfPages(os.path.join(D, "pannelli.pdf"))
    summary = []
    for name, f, mat, rots in PANELS:
        w = flat(f, rots)
        top, out, holes = outline(w)
        cq.exporters.export(cq.Workplane("XY").add(top), os.path.join(D, name + ".dxf"))
        bb = w.val().BoundingBox()
        fig, ax = plt.subplots(figsize=(11.7, 8.3))
        ax.add_patch(plt.Rectangle((0, 0), bb.xlen, bb.ylen, fc="#e4e6e8" if "PMMA" not in mat else "#d8eef5", ec="none"))
        for seg in out: ax.plot([p[0] for p in seg], [p[1] for p in seg], color="black", lw=1)
        circ = {}
        for h in holes:
            if h[0] == "cerchio":
                ax.add_patch(plt.Circle((h[1], h[2]), h[3] / 2, fc="white", ec="black", lw=0.6))
            else:
                for seg in h[4]: ax.plot([p[0] for p in seg], [p[1] for p in seg], color="black", lw=0.6)
            if h[0] == "cerchio": circ.setdefault(h[3], []).append((h[1], h[2]))
            else: ax.text(h[1] + h[3][0] / 2, h[2] + h[3][1] / 2, f"{h[3][0]:g} x {h[3][1]:g}\nangolo {h[1]:g}; {h[2]:g}", fontsize=6, ha="center", va="center", color="#b3261e")
        for dia, cs in circ.items():
            if len(cs) <= 6:
                for (x, y) in cs: ax.text(x + dia / 2 + 1, y + dia / 2 + 1, f"Ø{dia:g} ({x:g}; {y:g})", fontsize=5.5, color="#1f6fb5")
        ax.set_aspect("equal"); ax.grid(alpha=0.25)
        ax.set_title(f"{name.replace('_', ' ')} - {bb.xlen:.1f} x {bb.ylen:.1f} mm - {mat}\nvista dalla faccia esterna (alto = alto della macchina, o retro per i piani), coordinate in mm dall'angolo in basso a sinistra; "
                     + ", ".join(f"{len(v)} fori Ø{k:g}" for k, v in sorted(circ.items())), fontsize=9)
        pdf.savefig(fig); plt.close(fig)
        summary.append((name, round(bb.xlen, 1), round(bb.ylen, 1), mat, sum(len(v) for v in circ.values()), sum(1 for h in holes if h[0] != "cerchio")))
        print(f"{name:22s} {bb.xlen:6.1f} x {bb.ylen:6.1f}  fori tondi {sum(len(v) for v in circ.values()):3d}  aperture {sum(1 for h in holes if h[0] != 'cerchio')}")
    pdf.close()
    import json
    json.dump([{"nome": n, "x": a, "y": b, "materiale": m, "fori": f, "aperture": o} for n, a, b, m, f, o in summary],
              open(os.path.join(OUT, "docs", "_pannelli.json"), "w"), indent=1, ensure_ascii=False)
    area = sum(a * b for _, a, b, m, *_ in summary if "Dibond" in m) / 1e6
    print(f"Dibond: {area:.2f} m2 in tutto; PMMA: {sum(a * b for _, a, b, m, *_ in summary if "PMMA" in m) / 1e6:.2f} m2")
