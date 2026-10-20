"""Assieme unico della macchina completa (rilascio 6): parti stampate (serie A, B, C, D) + sagome dei componenti acquistati (REF_)."""
import itertools, sys, time
import cadquery as cq
from machine import *
import mrefs as R, hydro as H

def hydro_bodies():
    """Base, piano, bagno in PMMA, contenitori, passaparete, riscaldatori, sensori, sonda, pompetta, valvole e collettore,
    camere di livello, linee, pompa, ingresso dell'acqua, rubinetti, alimentatore, mainboard, display, presa."""
    P = R.all_refs()
    P.update({"C01_collettore": H.manifold(), "C02_culla_valvole": H.valve_cradle(), "C05_portasonda": H.probe_holder(),
              "C04_culla_flussimetro": H.flow_cradle(), "C07_culla_valvola_ingresso": H.inlet_valve_cradle(), "C08_culla_pompa": H.pump_cradle(),
              "C09_fermo_alimentatore_1": H.psu_stop(0), "C14_staffa_beccuccio": H.riser_clip(IN_X, IN_Y), "C14_staffa_troppopieno": H.riser_clip(OVF_X),
              "C09_fermo_alimentatore_2": H.psu_stop(1), "C10_montante_mainboard_1": H.board_post(0), "C10_montante_mainboard_2": H.board_post(1),
              "C12_cornice_display": H.display_bezel()})
    for k in range(3):
        P[f"C03_camera_livello_C{k + 1}"] = H.chamber(k)
    for i in range(2): P[f"C06_sella_riscaldatore_{i + 1}"] = H.heater_saddle(i)
    for j, (x, y) in enumerate(H.COLS): P[f"C13_colonna_piano_{j + 1}"] = H.deck_column(x, y)
    for j, t in enumerate(H.display_tabs()): P[f"D10_linguetta_display_{j + 1}"] = t
    return P

def tank_bodies(fmt="135"):
    """Modulo tank + caricatore (serie A e B, trasmissione magnetica) portato nel riferimento macchina. Il rullo 120 e' mostrato
    come alternativa al caricatore 135 (stessa culla): nel PDF 3D si accende l'uno o l'altro."""
    import tank_assembly, loader
    P = {n: to_machine(p) for n, p in tank_assembly.build(fmt).items()}
    P["REF_rullo_120_alternativa"] = to_machine(loader.roll120())
    P["REF_acqua_del_bagno"] = R.water()
    return P

def shell_bodies():
    """Scocca: pannelli in Dibond e montanti."""
    import shell as S
    P = {n: f() for n, f in S.PANELS}
    for i in range(4):
        P[f"D06_montante_inferiore_{i + 1}"] = S.post(i, True); P[f"D07_montante_superiore_{i + 1}"] = S.post(i, False)
    return P

GROUPS = (hydro_bodies, tank_bodies, shell_bodies)

# Coppie escluse dal controllo: SOLO quelle in cui un corpo entra nell'altro per costruzione (magneti nelle tasche, perno nel
# boss e nel mozzo, tubi in silicone infilati sui portagomma, spezzoni e codoli infilati nei raccordi, liquidi). Tutto il resto
# deve dare zero: anche gomiti contro valvole/passaparete, tubi contro gomiti, passaparete contro le pareti del bagno.
SKIP = (("REF_caricatore_135", "REF_rullo_120"), ("B14_slitta_DX", "REF_viti_zigrinate"), ("B15_slitta_SX", "REF_viti_zigrinate"),
        ("REF_acqua", ""), ("REF_chimica", ""), ("REF_magnete", "B16"), ("REF_magnete", "B18"), ("REF_perno_inox", "B01"), ("REF_perno_inox", "B16"),
        ("REF_silicone", "REF_passaparete"), ("REF_silicone", "REF_adattatore"), ("REF_silicone", "REF_rubinetto"), ("REF_silicone", "REF_uscita"),
        ("REF_silicone", "REF_pompa"), ("REF_silicone", "REF_gommino"), ("REF_silicone_8x12_tank", "B01"), ("REF_silicone_8x12_troppopieno", "B01"),
        ("REF_spezzone", "REF_gomit"), ("REF_spezzone", "C03"), ("REF_spezzone", "C01"), ("REF_spezzone", "REF_passaparete"), ("REF_spezzone", "REF_valvola"),
        ("REF_adattatore", "C03"), ("REF_adattatore", "REF_valvola"), ("REF_adattatore", "REF_passaparete"), ("REF_adattatore", "REF_gomito"),
        ("REF_gomito_codolo", "C01"), ("REF_tappo", "REF_contenitore"))

def interferences(P, tol=1e-3, skip=()):
    bad = []; bbs = {n: p.val().BoundingBox() for n, p in P.items()}
    for a, b in itertools.combinations(list(P), 2):
        if any((a.startswith(s1) and b.startswith(s2)) or (b.startswith(s1) and a.startswith(s2)) for s1, s2 in skip): continue
        ba, bb = bbs[a], bbs[b]
        if (ba.xmin > bb.xmax or bb.xmin > ba.xmax or ba.ymin > bb.ymax or bb.ymin > ba.ymax or ba.zmin > bb.zmax or bb.zmin > ba.zmax):
            continue
        try:
            v = P[a].val().intersect(P[b].val()).Volume()
        except Exception as e:
            v = -1.0
        if v > tol or v < 0: bad.append((a, b, round(v, 3)))
    return bad

# ---------------------------------------------------------------- anteprime
PALETTE = {"REF_base": "#b08a5a", "REF_contenitore": "#8fb7d9", "REF_vaschetta": "#d6dde3", "REF_longherone": "#9aa0a6",
           "REF_tubo": "#f2f2f2", "REF_spezzone": "#f2f2f2", "REF_testina_tubi": "#f2f2f2", "REF_testina_gomiti": "#e9e4d4", "REF_gomito": "#e9e4d4",
           "REF_silicone": "#444444", "REF_sifone": "#e6b8b8", "REF_elettrovalvola": "#3d6fb3", "REF_pompa": "#7a6a4f", "REF_riscaldatore": "#b8b8b8",
           "REF_sensore": "#2f8f5b", "REF_sonda": "#b8b8b8", "REF_passaparete": "#e9e4d4", "REF_": "#8a8f96",
           "REF_caricatore": "#dfe3e8", "REF_rullo": "#c9503a", "REF_motoriduttore": "#a9aeb4", "REF_servo": "#3b6fd1", "REF_cinghia": "#1c1c1c",
           "REF_lama": "#d5d9dd", "REF_albero": "#c5c9cd", "REF_cuscinetto": "#c5c9cd", "REF_paraolio": "#6b4a3a", "REF_rondella": "#c5c9cd",
           "REF_molla": "#c5c9cd", "REF_magneti": "#e0e0e0", "REF_alimentatore": "#a8adb3", "REF_display": "#20262e", "REF_altoparlante": "#444444",
           "REF_presa": "#2c2c2c", "REF_ponte_H": "#2f7d4f", "REF_modulo_MOSFET": "#2f7d4f", "REF_driver": "#2f7d4f", "REF_breakout": "#2f7d4f",
           "REF_convertitore": "#2f7d4f",
           "A": "#2f2f2f", "A05": "#5c5c5c",
           "B": "#5a6068", "B02": "#3f444a", "B04": "#2f9e8f", "B05": "#2f9e8f", "B06": "#2a8a7d", "B07": "#e07a3f", "B08": "#e07a3f", "B09": "#e07a3f",
           "B10": "#e07a3f", "B11": "#e07a3f", "B12": "#c9a227", "B13": "#4f8fd9", "B14": "#4f8fd9", "B15": "#4f8fd9",
           "B16": "#8a6fbf", "B17": "#8a6fbf", "B18": "#8a6fbf", "B19": "#8a6fbf", "B20": "#8a6fbf", "B21": "#8a6fbf", "B22": "#8a6fbf",
           "B23": "#7560a8", "B24": "#8a6fbf", "B25": "#8a6fbf",
           "C": "#e8922d", "D": "#5b7fbf",
           "REF_linea": "#f2f2f2", "REF_gomiti_ingresso": "#e9e4d4", "REF_tubi_ingresso": "#f2f2f2", "REF_tubo_PVC": "#cfe8f0",
           "REF_flussimetro": "#2a2a2a", "REF_guarnizione": "#222222", "REF_bagno": "#cfe6f2", "REF_acqua": "#9cc9e6", "REF_contenitore": "#e9eef2", "REF_tappo": "#1e1f21", "REF_chimica_C1": "#d9a520", "REF_chimica_C2": "#d8dde3", "REF_chimica_C3": "#3b8fd9", "REF_rubinetto": "#c0392b", "REF_uscita": "#2e8b57", "REF_mainboard": "#2f7d4f", "REF_pompetta": "#d07b2a", "REF_adattatore": "#e9e4d4", "REF_attacco": "#e9e4d4", "REF_perno": "#c5c9cd", "REF_magnete": "#b0b0b0", "REF_magnete_5x2": "#d9a520", "C03": "#cfe9f2", "C01": "#b9bec4", "C02": "#8a8f96", "C12": "#3b4047", "C13": "#2b2f35", "REF_oring": "#222222", "REF_barra": "#aaaaaa",
           "REF_pannello": "#e4e6e8", "REF_piano": "#3b4047", "REF_coperchio_plexiglass": "#bfe3f2", "REF_paratia": "#c9ccd0",
           "REF_divisorio": "#c9ccd0", "C15": "#cfe9f2", "C11": "#d9d9d9", "C12": "#d9d9d9", "C13": "#d97a2b", "C14": "#e8922d",
           "D06": "#2b2f35", "D07": "#2b2f35", "D08": "#2b2f35", "D09": "#3b4047", "D10": "#3b4047"}

def color_of(name):
    for k in sorted(PALETTE, key=len, reverse=True):
        if name.startswith(k): return PALETTE[k]
    return "#999999"

def preview(P, name, elev=28, azim=-55, hide=(), tol=0.4, size=(11, 8.5), clip=None):
    """Anteprima PNG ombreggiata dell'assieme (matplotlib). hide: prefissi dei corpi da non disegnare."""
    import numpy as np, matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection
    light = np.array([0.35, -0.5, 0.8]); light /= np.linalg.norm(light)
    tris, cols = [], []
    for n, p in P.items():
        if any(n.startswith(h) for h in hide): continue
        v, t = p.val().tessellate(tol, 0.4)
        V = np.array([[q.x, q.y, q.z] for q in v]); tri = V[np.array(t)]
        if clip is not None:
            keep = clip(tri.mean(axis=1)); tri = tri[keep]
        nn = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0]); ln = np.linalg.norm(nn, axis=1); ln[ln == 0] = 1
        sh = 0.45 + 0.55 * np.abs((nn / ln[:, None]) @ light)
        tris.append(tri); cols.append(np.clip(sh[:, None] * np.array(matplotlib.colors.to_rgb(color_of(n))), 0, 1))
    T = np.concatenate(tris); C = np.concatenate(cols)
    fig = plt.figure(figsize=size); ax = fig.add_subplot(111, projection="3d")
    ax.add_collection3d(Poly3DCollection(T, facecolors=C, edgecolors="none"))
    lo, hi = T.reshape(-1, 3).min(0), T.reshape(-1, 3).max(0); c = (lo + hi) / 2; r = (hi - lo).max() / 2
    ax.set_xlim(c[0] - r, c[0] + r); ax.set_ylim(c[1] - r, c[1] + r); ax.set_zlim(c[2] - r, c[2] + r)
    ax.set_box_aspect((1, 1, 1)); ax.view_init(elev=elev, azim=azim); ax.set_axis_off()
    import os
    p = os.path.join(OUT, "png", name + ".png"); fig.savefig(p, dpi=105, bbox_inches="tight"); plt.close(fig)
    return p

def section(P, axis, c, name, window=None, size=(11, 7), hide=(), title=None):
    """Sezione piana dell'assieme (perpendicolare ad axis, alla quota c), un colore per corpo."""
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt, os
    from OCP.BRepTools import BRepTools_WireExplorer
    from OCP.TopAbs import TopAbs_REVERSED
    if axis == "X": pl = cq.Face.makePlane(2000, 2000, basePnt=(c, 0, 0), dir=(1, 0, 0)); ia, ib = 1, 2
    elif axis == "Y": pl = cq.Face.makePlane(2000, 2000, basePnt=(0, c, 0), dir=(0, 1, 0)); ia, ib = 0, 2
    else: pl = cq.Face.makePlane(2000, 2000, basePnt=(0, 0, c), dir=(0, 0, 1)); ia, ib = 0, 1
    fig, ax = plt.subplots(figsize=size)
    for n, p in P.items():
        if any(n.startswith(h) for h in hide): continue
        bb = p.val().BoundingBox(); lo, hi = ((bb.xmin, bb.xmax), (bb.ymin, bb.ymax), (bb.zmin, bb.zmax))["XYZ".index(axis)]
        if c < lo or c > hi: continue
        try: sec = p.val().intersect(pl)
        except Exception: continue
        for f in sec.Faces():
            for i, w in enumerate([f.outerWire()] + f.innerWires()):
                pts = []; ex = BRepTools_WireExplorer(w.wrapped)
                while ex.More():
                    e = cq.Edge(ex.Current()); m = 16 if e.geomType() != "LINE" else 1
                    sg = [e.positionAt(j / m).toTuple() for j in range(m + 1)]
                    if ex.Orientation() == TopAbs_REVERSED: sg = sg[::-1]
                    pts += sg; ex.Next()
                ax.fill([q[ia] for q in pts], [q[ib] for q in pts], facecolor=("white" if i else color_of(n)), edgecolor="k", lw=0.5, alpha=1.0 if i else 0.85)
    ax.set_aspect("equal"); ax.grid(alpha=0.25)
    if window: ax.set_xlim(window[0], window[1]); ax.set_ylim(window[2], window[3])
    ax.set_title(title or f"sezione {axis} = {c}", fontsize=10)
    p = os.path.join(OUT, "png", name + ".png"); fig.savefig(p, dpi=110, bbox_inches="tight"); plt.close(fig)
    return p

if __name__ == "__main__":
    t0 = time.time()
    P = {}
    for f in GROUPS:
        P.update(f())
    inv = [n for n, p in P.items() if not p.val().isValid()]
    print(f"assieme: {len(P)} corpi, non validi {inv if inv else 'nessuno'} ({time.time() - t0:.0f} s)")
    bad = interferences(P, skip=SKIP)
    for b in bad: print("   INTERFERENZA", b)
    print(f"interferenze: {len(bad)} ({time.time() - t0:.0f} s)")

