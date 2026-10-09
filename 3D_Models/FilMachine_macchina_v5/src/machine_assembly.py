"""Assieme unico della macchina completa: parti stampate (serie A, B, C, D) + sagome dei componenti acquistati (REF_)."""
import itertools, sys, time
import cadquery as cq
from machine import *
import mrefs as R, hydro as H

def hydro_bodies():
    """Bagno, vaschette e idraulica."""
    P = {"REF_base_multistrato_420x505x12": R.base_board(), "REF_contenitore_Euro_400x300x170": R.tub(),
         "REF_longherone_anteriore_15x15": R.rail(True), "REF_longherone_posteriore_15x15": R.rail(False),
         "C05_testata_telaio_SX": H.frame_end(-1), "C05_testata_telaio_DX": H.frame_end(1), "C06_portasonde": H.probe_holder(),
         "C07_collettore": H.manifold(), "C08_pettine_valvole": H.valve_comb(), "C09_staffa_scarichi": H.drain_bracket(),
         "C10_carter_riscaldatori": H.heater_cover(), "REF_pompa_Marco_UP3-R": R.pump(),
         "REF_tubo_WB": R.wb_tube(), "REF_tubo_scarico": R.waste_tube(), "REF_passaparete_scarico": R.bulkhead(),
         "REF_tubo_collettore_pompa": R.manifold_tube(), "REF_tubo_pompa_tank": R.tank_tube(),
         "REF_gomito_scarico": R.waste_elbow(), "REF_gomito_a_codolo_collettore": R.manifold_elbow(),
         "REF_gomito_a_codolo_pompa": R.pump_elbow(), "REF_gomito_salita_tank": R.rise_elbow(),
         "REF_silicone_8x12_tank": R.tank_hose(), "REF_silicone_8x12_troppopieno": R.overflow_hose(),
         "REF_riscaldatore_1": R.heater(0), "REF_riscaldatore_2": R.heater(1),
         "REF_sensore_livello_MIN": R.level_sensor(LVL_ZMIN), "REF_sensore_livello_MAX": R.level_sensor(LVL_ZMAX),
         "REF_sonda_DS18B20_bagno": R.probe_bath(), "REF_sonda_DS18B20_chimica": R.probe_chem()}
    for k, c in enumerate(("C1", "C2", "C3")):
        P.update({f"REF_vaschetta_GN1-9_{c}": R.vaschetta(k), f"C01_sella_testina_{c}": H.saddle(k), f"C02_culla_sifone_{c}": H.siphon_holder(k), f"REF_sifone_silicone_{c}": R.siphon(k),
                  f"C03_staffa_ganci_{c}": H.latch(k), f"C04_fermo_posteriore_{c}": H.keeper(k),
                  f"REF_testina_gomiti_{c}": R.head_elbows(k), f"REF_testina_tubi_{c}": R.head_tubes(k), f"REF_tubo_{c}": R.feed_tube(k)})
    for k, c in enumerate(("C1", "C2", "C3", "WB", "scarico")):
        P[f"REF_elettrovalvola_{c}"] = R.valve(k); P[f"REF_spezzone_valvola_{c}"] = R.valve_stub(k)
    return P

def tank_bodies(fmt="135"):
    """Modulo tank + caricatore (serie A e B) portato nel riferimento macchina. Il rullo 120 e' mostrato come alternativa
    al caricatore 135 (stessa culla): nel PDF 3D si accende l'uno o l'altro."""
    import tank_assembly, loader
    P = {n: to_machine(p) for n, p in tank_assembly.build(fmt).items()}
    P["REF_rullo_120_alternativa"] = to_machine(loader.roll120())
    return P

def stand_bodies():
    import frame as F
    return {"D01_piano_cavalletto": F.stand_deck(), "D02_fianco_cavalletto_SX": F.stand_side(-1), "D03_fianco_cavalletto_DX": F.stand_side(1),
            "D04_frontale_cavalletto": F.stand_front(), "D05_carter_trasmissione": F.drive_cover()}

def electronics_bodies():
    """Vano elettronica: pannelli, ripiani, schede (REF) con i loro morsetti, alimentatore, display."""
    import frame as F
    P = {"D06_fianco_elettronica_SX": F.el_side(-1), "D07_fianco_elettronica_DX": F.el_side(1), "D08_frontale_elettronica": F.el_front(),
         "D09_plancia_display": F.bezel(), "D10_tetto_elettronica": F.el_top(), "D11_paratia_posteriore": F.el_rear(),
         "D16_zoccolo_alimentatore": F.psu_socket(), "REF_alimentatore_12V_30A": R.psu(),
         "REF_display_JC4880P443C": R.display(F.DISP_CENTER, SLOPE_ANG), "REF_altoparlante_8ohm_2W": R.speaker(), "REF_presa_IEC_interruttore_fusibile": R.iec_inlet()}
    for j, t in enumerate(F.disp_tabs()):
        P[f"D17_linguetta_display_{j + 1}"] = t
    for i in range(3):
        P[f"D12_ripiano_griglia_{i + 1}"] = F.shelf(i)
    for j, (x, y) in enumerate(F.COL_LOW):
        P[f"D13_colonnina_bassa_{j + 1}"] = F.column(x, y, 0, F.SH_Z[0])
    for i in range(2):
        for j, (x, y) in enumerate(F.COL_TALL):
            P[f"D14_colonnina_alta_{4 * i + j + 1}"] = F.column(x, y, F.SH_Z[i] + F.SH_T, F.SH_Z[i + 1])
    n = 0
    for (name, i, x0, y0, lx, ly, h) in F.BOARDS:
        z = F.SH_Z[i] + F.SH_T
        P[name] = R.board(x0, y0, z + F.PCB_Z, lx, ly, h)
        yc = y0 + ly / 2
        for (x, y, sd) in ((x0, yc - 8.0, -1), (x0 + lx, yc + 8.0, 1)):
            n += 1; P[f"D15_morsetto_scheda_{n}"] = F.clamp(x, y, z, sd)
    return P

SKIP = (("REF_caricatore_135", "REF_rullo_120"),)

def interferences(P, tol=1e-3, skip=()):
    bad = []; bbs = {n: p.val().BoundingBox() for n, p in P.items()}
    for a, b in itertools.combinations(list(P), 2):
        if any((s1 in a and s2 in b) or (s1 in b and s2 in a) for s1, s2 in skip): continue
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
           "A": "#2f2f2f", "A04": "#454545", "A05": "#5c5c5c", "A07": "#c94a4a", "A08": "#c94a4a",
           "B": "#5a6068", "B02": "#3f444a", "B04": "#2f9e8f", "B05": "#2f9e8f", "B06": "#2a8a7d", "B07": "#e07a3f", "B08": "#e07a3f", "B09": "#e07a3f",
           "B10": "#e07a3f", "B11": "#e07a3f", "B12": "#c9a227", "B13": "#4f8fd9", "B14": "#4f8fd9", "B15": "#4f8fd9",
           "B16": "#8a6fbf", "B17": "#8a6fbf", "B18": "#8a6fbf", "B19": "#8a6fbf", "B20": "#8a6fbf", "B21": "#8a6fbf", "B22": "#8a6fbf",
           "B23": "#7560a8", "B24": "#8a6fbf", "B25": "#8a6fbf",
           "C": "#e8922d", "D": "#5b7fbf"}

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
    for f in (hydro_bodies, tank_bodies, stand_bodies, electronics_bodies):
        P.update(f())
    inv = [n for n, p in P.items() if not p.val().isValid()]
    print(f"assieme: {len(P)} corpi, non validi {inv if inv else 'nessuno'} ({time.time() - t0:.0f} s)")
    bad = interferences(P, skip=SKIP)
    for b in bad: print("   INTERFERENZA", b)
    print(f"interferenze: {len(bad)} ({time.time() - t0:.0f} s)")

