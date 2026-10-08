"""FilMachine - caricatore automatico pellicola: parametri e utilita' comuni (mm)."""
import math, os
import cadquery as cq

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")

# ---- pellicola ----
FILM_W = {"135": 35.0, "120": 61.5}
SIDE_PLAY = 0.6            # gioco laterale totale pellicola tra le flange

# ---- spirale (carico dal centro verso l'esterno, stile Rondinax) ----
CORE_R = 22.0              # raggio tamburo (fascetta da 115 mm: la clip arriva al caricatore / al rullo 120)
RIB_R0 = 23.0              # raggio esterno della costola al primo mezzo giro
PITCH = 2.2                # passo radiale spirale
RIB_W = 0.8                # larghezza costola (2 perimetri ugello 0.4)
RIB_H = 1.2                # sporgenza assiale costola (appoggio bordo pellicola)
HALF_TURNS = 20            # mezzi giri di costola
DISC_T = 2.0               # spessore disco flangia
FLANGE_R = 46.75          # Ø93.5 come la OpenReel (sistema Paterson)
# innesto flangia/nucleo (D) e trascinamento (esagono)
D_R, D_FLAT, D_H = 9.5, 7.5, 6.0
HEX_AF = 10.0              # chiave esagono albero di trascinamento
FIT = 0.15                 # gioco di accoppiamento stampato

def reel_width(fmt):
    return FILM_W[fmt] + SIDE_PLAY

def export(shape, name):
    """Scrive STEP + STL e restituisce volume e ingombro."""
    s = shape.val() if isinstance(shape, cq.Workplane) else shape
    cq.exporters.export(shape, os.path.join(OUT, "step", name + ".step"))
    cq.exporters.export(shape, os.path.join(OUT, "stl", name + ".stl"),
                        tolerance=0.02, angularTolerance=0.15)
    bb = s.BoundingBox()
    print(f"{name}: valid={s.isValid()} solids={len(s.Solids())} "
          f"vol={s.Volume()/1000:.2f}cm3 bbox={bb.xlen:.1f}x{bb.ylen:.1f}x{bb.zlen:.1f}")
    return s

def render(shapes, name, elev=28, azim=-55, colors=None, size=(9, 7)):
    """Anteprima PNG ombreggiata (matplotlib) di una o piu' forme."""
    import numpy as np, matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection
    if not isinstance(shapes, (list, tuple)):
        shapes = [shapes]
    colors = colors or ["#c9ccd1", "#e8a33d", "#5b8def", "#6fbf73", "#d9655b", "#9b7fd1", "#444444"]
    fig = plt.figure(figsize=size); ax = fig.add_subplot(111, projection="3d")
    light = np.array([0.35, -0.5, 0.8]); light = light / np.linalg.norm(light)
    lo = np.array([1e9] * 3); hi = -lo
    tris, cols = [], []
    for i, sh in enumerate(shapes):
        s = sh.val() if isinstance(sh, cq.Workplane) else sh
        v, t = s.tessellate(0.15, 0.3)
        V = np.array([[p.x, p.y, p.z] for p in v]); T = np.array(t)
        tri = V[T]
        n = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
        ln = np.linalg.norm(n, axis=1); ln[ln == 0] = 1; n = n / ln[:, None]
        sh_ = 0.45 + 0.55 * np.abs(n @ light)
        base = np.array(matplotlib.colors.to_rgb(colors[i % len(colors)]))
        tris.append(tri); cols.append(np.clip(sh_[:, None] * base, 0, 1))
        lo = np.minimum(lo, V.min(0)); hi = np.maximum(hi, V.max(0))
    # un'unica collezione: l'ordinamento in profondita' avviene per triangolo
    ax.add_collection3d(Poly3DCollection(np.concatenate(tris), facecolors=np.concatenate(cols),
                                         edgecolors="none"))
    c = (lo + hi) / 2; r = (hi - lo).max() / 2
    ax.set_xlim(c[0] - r, c[0] + r); ax.set_ylim(c[1] - r, c[1] + r); ax.set_zlim(c[2] - r, c[2] + r)
    ax.set_box_aspect((1, 1, 1)); ax.view_init(elev=elev, azim=azim); ax.set_axis_off()
    p = os.path.join(OUT, "png", name + ".png")
    fig.savefig(p, dpi=110, bbox_inches="tight"); plt.close(fig)
    return p

# ---- layout banco (X = verso di marcia pellicola, Y = asse spirale, Z = alto; z=0 piano base) ----
HC = 50.0                  # altezza asse spirale
PIV_X, PIV_DZ = -58.0, 34.0   # perno braccio guida rispetto all'asse spirale
KNIFE_X = -70.0            # piano della lama
YOKE_HALF = 35.2           # semi-larghezza esterna forcella braccio (uguale per 135 e 120)

# ---- braccio guida ----
H_EDGE = 2.4               # quota bordo pellicola sopra la suola del braccio
WALL_TIP = 0.6             # parete del tunnel nel tratto che entra tra le flange
TIP_CLEAR = 0.2            # gioco per lato tra tunnel e punte delle costole
ARM_L, ARM_TAPER0, ARM_TAPER1 = 70.0, 2.0, 22.0   # suola piana fino a ARM_L-3: il punto di tangenza cade tra s=54 e s=65

def arch(fmt):
    """Arco che la pellicola deve assumere per passare tra le costole: (corda, freccia, raggio)."""
    wf = FILM_W[fmt]
    chord = reel_width(fmt) - 2 * RIB_H - 2 * TIP_CLEAR - 2 * WALL_TIP
    lo, hi = 1e-6, math.pi / 2
    for _ in range(60):
        mid = (lo + hi) / 2
        if math.sin(mid) / mid > chord / wf: lo = mid
        else: hi = mid
    th = (lo + hi) / 2; R = wf / (2 * th)
    return chord, R * (1 - math.cos(th)), R

def ruled(pa, pb):
    """Solido a facce piane tra due poligoni 3D con lo stesso numero di vertici."""
    V = cq.Vector
    def face(pts):
        return cq.Face.makeFromWires(cq.Wire.makePolygon([V(*p) for p in pts], close=True))
    n = len(pa)
    faces = [face(pa), face(pb)]
    for i in range(n):
        j = (i + 1) % n
        faces.append(face([pa[i], pa[j], pb[j], pb[i]]))
    s = cq.Solid.makeSolid(cq.Shell.makeShell(faces))
    return s if s.Volume() > 0 else cq.Solid.makeSolid(cq.Shell.makeShell([f.flipped() if hasattr(f,'flipped') else f for f in faces]))

def section_plot(shape, name, cuts, axis="X", size=(11, 4)):
    """Sezioni piane (perpendicolari ad `axis`) disegnate in 2D: verifica dei profili."""
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    s = shape.val() if isinstance(shape, cq.Workplane) else shape
    fig, axs = plt.subplots(1, len(cuts), figsize=size, squeeze=False)
    for ax, c in zip(axs[0], cuts):
        if axis == "X":
            pl = cq.Face.makePlane(400, 400, basePnt=(c, 0, 0), dir=(1, 0, 0)); ia, ib = 1, 2
        elif axis == "Y":
            pl = cq.Face.makePlane(400, 400, basePnt=(0, c, 0), dir=(0, 1, 0)); ia, ib = 0, 2
        else:
            pl = cq.Face.makePlane(400, 400, basePnt=(0, 0, c), dir=(0, 0, 1)); ia, ib = 0, 1
        sec = s.intersect(pl)
        for f in sec.Faces():
            for w in [f.outerWire()] + f.innerWires():
                pts = []
                from OCP.BRepTools import BRepTools_WireExplorer
                from OCP.TopAbs import TopAbs_REVERSED
                ex = BRepTools_WireExplorer(w.wrapped)
                while ex.More():
                    e = cq.Edge(ex.Current())
                    n = 24 if e.geomType() != "LINE" else 1
                    seg = [e.positionAt(i / n).toTuple() for i in range(n + 1)]
                    if ex.Orientation() == TopAbs_REVERSED: seg = seg[::-1]
                    pts += seg; ex.Next()
                xs = [p[ia] for p in pts]; ys = [p[ib] for p in pts]
                ax.fill(xs, ys, facecolor="#9ab", edgecolor="k", lw=0.6, alpha=0.6)
        ax.set_aspect("equal"); ax.set_title(f"{axis}={c}", fontsize=9); ax.grid(alpha=0.3)
    p = os.path.join(OUT, "png", name + ".png")
    fig.savefig(p, dpi=110, bbox_inches="tight"); plt.close(fig)
    return p

# ---- torretta (guance, ponte lama, slitta, culla) ----
ZF = HC + PIV_DZ           # quota della linea pellicola (bordi) tra caricatore e perno braccio
CHEEK_IN, CHEEK_T = YOKE_HALF + 0.3, 5.0
CHEEK_X0, CHEEK_X1, CHEEK_ZTOP = -112.0, -52.0, 100.0
BR_X0, BR_X1, BR_Z0, BR_Z1 = -77.0, -66.5, 78.0, 100.0      # ponte lama
SLOT_Z0, SLOT_Z1 = ZF - 3.0, ZF + 0.6                      # fessura pellicola nel ponte
# taglierina a pendolo: lama da cutter 9 mm (0.4) che spazza la fessura, mossa da un micro-servo
BLADE_W, BLADE_T = 9.0, 0.4
SWING_R = 75.0                                      # distanza asse servo - punta lama
TIP_OVER = 1.07                                     # di quanto la punta supera il cielo fessura ai bordi del 120
SERVO_Z = SLOT_Z1 + TIP_OVER - math.sqrt(SWING_R ** 2 - 31.05 ** 2)   # quota asse servo
SWING_PARK = math.degrees(math.asin(33.8 / SWING_R))                # parcheggio (punta a y=-33.8, fuori dalla pellicola)
SWING_END = math.degrees(math.asin(33.5 / SWING_R))                 # fine corsa di taglio (punta a y=+33.5)
SWA_T = 5.0                                         # spessore braccio lama
SWA_X1 = KNIFE_X - BLADE_T / 2 + 0.3; SWA_X0 = SWA_X1 - SWA_T       # lama centrata sul piano di taglio (sede 0.3)
SB_X1 = SWA_X0 - 13.5; SB_X0 = SB_X1 - 6.0          # supporto servo (13.5 mm tra alette e faccia squadretta)
CAS_X, CAS_Z, CAS_R = -93.5, ZF - 11.5, 13.0               # asse caricatore 135 e raggio sede
M3_TAP, M3_CLR = 1.3, 1.7  # raggi foro: autofilettante M3 / passante

AX = {"X": (1, 0, 0), "Y": (0, 1, 0), "Z": (0, 0, 1)}
def to_print(shape, rots=()):
    """Ruota il pezzo nell'orientamento di stampa e lo appoggia su z=0, centrato in XY."""
    w = shape if isinstance(shape, cq.Workplane) else cq.Workplane("XY").add(shape)
    for ax, ang in rots:
        w = w.rotate((0, 0, 0), AX[ax], ang)
    bb = w.val().BoundingBox()
    return w.translate((-(bb.xmin + bb.xmax) / 2, -(bb.ymin + bb.ymax) / 2, -bb.zmin))

# ---- trascinamento e banco ----
UP_Y, UP_T, UP_HALF = 51.0, 5.0, 26.0     # montanti: faccia interna |y|, spessore, semi-larghezza in X
NEMA_HOLES, NEMA_BOSS_R = 15.5, 11.5      # NEMA17: semi-interasse fori M3 (31 mm), foro centraggio Ø23
SHAFT_IN = 19.0                           # albero motore sporgente oltre il montante (24 - 5)
def flange_face_y(fmt):                   # |y| della faccia esterna delle flange
    return reel_width(fmt) / 2 + DISC_T

# ---- clip della fascetta e relativa tasca nel nucleo ----
CLIP_W, CLIP_L = 18.0, 18.4               # larghezza ganasce, lunghezza clip
CLIP_FLOOR, CLIP_POCKET_T, CLIP_POCKET_A = CORE_R - 4.7, 19.5, 21.0   # pavimento tasca (dall'asse), lato tangenziale, lato assiale
STRAP_W = 16.0                            # larghezza fascetta

# ---- spirale regolabile 35/120 (sistema a baionetta tipo OpenReel/Paterson, carico dal centro) ----
TUBE_R, TUBE_BORE, TUBE_HALF = 16.0, 13.15, 35.5   # tubo centrale: foro Ø26.3 come la OpenReel
SLEEVE_R, SLEEVE_L = 19.5, 6.0                     # manicotto di guida della flangia (lato interno)
DRUM_HALF = 11.5                                   # semi-lunghezza del tamburo centrale
LUG_DEG, CHAN_DEG, LOCK_DEG = 24.0, 28.0, 30.0     # dente, canale assiale, rotazione di blocco
LUG_H, LUG_AX = 1.3, 3.0                           # sporgenza radiale e lunghezza assiale del dente (a V)
REEL_FIT = 0.06                                    # gioco flangia/tubo: radiale sul foro e assiale sui fianchi del dente (era 0.15)
FIN_GAP, FIN_R, FIN_T, FIN_N = 0.4, 45.9, 1.6, 8            # anello alette: gioco dalla flangia, raggio esterno, spessore, numero
FIN_PITCH = 50.0                                   # inclinazione delle alette rispetto al piano della flangia (>=45 per stampare senza supporti)
SPOKE_N, SPOKE_W = 16, 2.4                         # razze della flangia aperta: tra una razza e l'altra il disco e' vuoto
RIM_W = 2.5                                        # corona esterna della flangia
SOLE_STOP_R = CORE_R - 0.5                         # raggio a cui il fermo tiene la suola del braccio a spirale vuota
def lug_z(fmt):                                    # quota assiale (dal centro) del centro dente, lato A (negativo)
    return -(reel_width(fmt) / 2 + DISC_T) + LUG_AX / 2

def sector(profile_rz, a0, a1):
    """Solido di rivoluzione attorno a Z del profilo (r, z) tra gli angoli a0 e a1 (gradi)."""
    w = cq.Workplane("XZ").polyline(profile_rz).close().revolve(a1 - a0, (0, 0, 0), (0, 1, 0))
    c = w.val().Center()
    ac = math.degrees(math.atan2(c.y, c.x))
    return w.rotate((0, 0, 0), (0, 0, 1), (a0 + a1) / 2 - ac)
