"""Assieme unico della macchina completa (rilascio 6): parti stampate (serie A, B, C, D) + sagome dei componenti acquistati (REF_)."""
import itertools, sys, time
import cadquery as cq
from machine import *
import mrefs as R, hydro as H

def hydro_bodies(open_poppets=True):
    """Bagno, vaschette con innesti, linee, valvole con camere di livello, ingresso dell'acqua di rete, troppopieni."""
    P = {"REF_base_multistrato_420x505x12": R.base_board(), "REF_contenitore_Euro_400x300x220": R.tub(),
         "REF_longherone_anteriore_15x15": R.rail(True), "REF_longherone_posteriore_15x15": R.rail(False),
         "C05_testata_telaio_SX": H.frame_end(-1), "C05_testata_telaio_DX": H.frame_end(1), "C06_portasonde": H.probe_holder(),
         "C07_collettore": H.manifold(), "C08_pettine_valvole": H.valve_comb(), "C09_piastra_scarichi": H.drain_plate(),
         "C16_piastra_ingresso": H.inlet_plate(), "C10_carter_riscaldatore_1": H.heater_cover(0), "C10_carter_riscaldatore_2": H.heater_cover(1),
         "C17_staffa_beccuccio": H.spout_clip(), "C19_gancio_portasonda": H.probe_hook(),
         "REF_passaparete_svuotamento_bagno": R.bath_drain(), "REF_rubinetto_bagno": R.bath_tap(),

         "REF_pompa_Marco_UP3-R": R.pump(),
         "REF_tubo_WB": R.wb_tube(), "REF_tubo_scarico": R.waste_tube(), "REF_passaparete_scarico": R.bulkhead("SCARICO"),
         "REF_passaparete_ingresso": R.bulkhead("INGRESSO"),
         "REF_tubo_collettore_pompa": R.manifold_tube(), "REF_tubo_pompa_tank": R.tank_tube(),
         "REF_gomito_scarico": R.waste_elbow(), "REF_gomito_a_codolo_collettore": R.manifold_elbow(), "REF_gomito_salita_tank": R.rise_elbow(),
         "REF_silicone_8x12_tank": R.tank_hose(), "REF_silicone_8x12_troppopieno": R.overflow_hose(),
         "REF_flussimetro_YF-S201": R.flow_sensor(), "REF_elettrovalvola_ingresso": R.inlet_valve(),
         "REF_gomiti_ingresso": R.inlet_fittings(), "REF_tubi_ingresso": R.inlet_tubes(),
         "REF_passaparete_troppopieno_bagno": R.bath_overflow_fitting(), "REF_tubo_PVC_troppopieno_bagno": R.bath_overflow_hose(),
         "REF_riscaldatore_1": R.heater(0), "REF_riscaldatore_2": R.heater(1),
         "REF_sensore_livello_bagno_MIN": R.level_sensor(LVL_ZMIN), "REF_sensore_livello_bagno_MAX": R.level_sensor(LVL_ZMAX),
         "REF_sonda_DS18B20_bagno": R.probe_bath(), "REF_sonda_DS18B20_chimica": R.probe_chem()}
    for i, b in enumerate(H.tie_brackets()):
        P[f"C18_staffa_fascetta_{i + 1}"] = b
    op = 3.5 if open_poppets else 0.0
    for k, c in enumerate(("C1", "C2", "C3")):
        P.update({f"REF_vaschetta_GN1-9_{c}": R.vaschetta(k), f"REF_guarnizione_innesto_{c}": R.gasket(k),
                  f"C11_corpo_innesto_{c}": H.place_pan(H.inlet_body_local(), k), f"C12_ghiera_codolo_{c}": H.place_pan(H.nut_spigot_local(), k),
                  f"C13_otturatore_{c}": H.place_pan(H.poppet_local(op), k), f"REF_oring_otturatore_{c}": H.place_pan(H.oring_pop_local(op), k),
                  f"C14_presa_{c}": H.socket(k), f"REF_linea_{c}": R.line(k),
                  f"C15_camera_livello_{c}": H.chamber(k), f"REF_spezzone_camera_{c}": R.chamber_stub(k),
                  f"REF_sensore_livello_MIN_{c}": R.chamber_sensor(k), f"REF_sensore_livello_MAX_{c}": R.pan_sensor(k),
                  f"C04_portasensore_MAX_{c}": H.sensor_holder(k),
                  f"C03_fermo_girevole_ant_{c}": H.turn_button(k), f"C03_fermo_girevole_post_{c}": H.turn_button(k, rear=True)})
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
    """Vano elettronica: squadrette, ripiani, colonne, schede (REF) con i loro morsetti, alimentatore, presa, altoparlante."""
    import frame as F
    P = {"D16_zoccolo_alimentatore": F.psu_socket(), "D17_staffa_alimentatore": F.psu_strap(), "REF_alimentatore_LRS-350-12": R.psu(),
         "REF_altoparlante_8ohm_2W": R.speaker(), "D19_supporto_altoparlante": F.speaker_holder(),
         "REF_presa_IEC_interruttore_fusibile": R.iec_inlet()}
    P["D11_guida_paratia"] = F.panel_guide(True); P["D11_guida_divisorio"] = F.panel_guide(False)
    for i in range(3):
        P[f"D12_ripiano_griglia_{i + 1}"] = F.shelf(i)
    for j, (x, y) in enumerate(F.COL):
        P[f"D13_piede_colonna_{j + 1}"] = F.col_foot(x, y)
        for i in range(2):
            P[f"D14_colonnina_{4 * i + j + 1}"] = F.column(x, y, F.SH_Z[i] + F.SH_T, F.SH_Z[i + 1])
        P[f"REF_barra_filettata_M3_{j + 1}"] = zcyl(x, y, 1.45, 3.0, F.SH_Z[2] + F.SH_T + 4.0)
    n = 0
    for (name, i, x0, y0, lx, ly, h) in F.BOARDS:
        z = F.SH_Z[i] + F.SH_T
        P[name] = R.board(x0, y0, z + F.PCB_Z, lx, ly, h)
        yc = y0 + ly / 2
        for (x, y, sd) in ((x0, yc - 8.0, -1), (x0 + lx, yc + 8.0, 1)):
            n += 1; P[f"D15_morsetto_scheda_{n}"] = F.clamp(x, y, z, sd)
    return P

def shell_bodies():
    """Scocca: pannelli in Dibond, piano, coperchio trasparente, paratia, divisorio, montanti, appoggi, cruscotto con il display."""
    import shell as S
    P = {"REF_pannello_frontale_Dibond": S.panel_front(), "REF_pannello_posteriore_Dibond": S.panel_rear(),
         "REF_pannello_sinistro_Dibond": S.panel_left(), "REF_pannello_destro_Dibond": S.panel_right(),
         "REF_piano_anteriore_Dibond": S.deck(), "REF_coperchio_plexiglass": S.lid(),
         "REF_paratia_Dibond": S.bulkhead_panel(), "REF_divisorio_Dibond": S.divider_panel(),
         "D09_cruscotto_display": S.wedge(), "REF_display_JC4880P443C": S.display()}
    for i in range(4):
        P[f"D06_montante_inferiore_{i + 1}"] = S.post(i, True); P[f"D07_montante_superiore_{i + 1}"] = S.post(i, False)
    for j, x in enumerate(S.SUP_X):
        P[f"D08_appoggio_piano_{j + 1}"] = S.deck_support(x)
    for j, t in enumerate(S.disp_tabs()):
        P[f"D10_linguetta_display_{j + 1}"] = t
    return P

GROUPS = (hydro_bodies, tank_bodies, stand_bodies, electronics_bodies, shell_bodies)

SKIP = (("REF_caricatore_135", "REF_rullo_120"), ("C11_corpo_innesto", "C12_ghiera_codolo"), ("REF_tubo_pompa_tank", "REF_silicone_8x12_tank"),
        ("REF_silicone_8x12_tank", "B01_corpo_tank"), ("REF_passaparete_troppopieno_bagno", "REF_tubo_PVC_troppopieno_bagno"),
        ("REF_silicone_8x12_troppopieno", "D01_piano_cavalletto"),
        ("B14_slitta_DX", "REF_viti_zigrinate"), ("B15_slitta_SX", "REF_viti_zigrinate"),
        ("REF_passaparete_svuotamento_bagno", "REF_rubinetto_bagno"))      # vite di blocco nel suo foro filettato

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
           "A": "#2f2f2f", "A05": "#5c5c5c",
           "B": "#5a6068", "B02": "#3f444a", "B04": "#2f9e8f", "B05": "#2f9e8f", "B06": "#2a8a7d", "B07": "#e07a3f", "B08": "#e07a3f", "B09": "#e07a3f",
           "B10": "#e07a3f", "B11": "#e07a3f", "B12": "#c9a227", "B13": "#4f8fd9", "B14": "#4f8fd9", "B15": "#4f8fd9",
           "B16": "#8a6fbf", "B17": "#8a6fbf", "B18": "#8a6fbf", "B19": "#8a6fbf", "B20": "#8a6fbf", "B21": "#8a6fbf", "B22": "#8a6fbf",
           "B23": "#7560a8", "B24": "#8a6fbf", "B25": "#8a6fbf",
           "C": "#e8922d", "D": "#5b7fbf",
           "REF_linea": "#f2f2f2", "REF_gomiti_ingresso": "#e9e4d4", "REF_tubi_ingresso": "#f2f2f2", "REF_tubo_PVC": "#cfe8f0",
           "REF_flussimetro": "#2a2a2a", "REF_guarnizione": "#222222", "REF_oring": "#222222", "REF_barra": "#aaaaaa",
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

