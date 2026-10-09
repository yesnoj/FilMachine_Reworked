"""FilMachine - macchina completa: parametri e utilita' comuni (mm)."""
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

def flange_face_y(fmt):                   # |y| della faccia esterna delle flange
    return reel_width(fmt) / 2 + DISC_T

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

# ---- riferimento della tank (X = verso di marcia pellicola, caricatore a -X; Y = asse spirale; Z = alto) ----
HC = 50.0                  # quota dell'asse spirale (z=0: fondo interno della vasca, sotto la spirale)
H_EDGE = 2.4               # quota del bordo pellicola sopra la suola della guida
WALL_TIP = 0.6             # parete della guida nel tratto che entra tra le flange
TIP_CLEAR = 0.2            # gioco per lato tra guida e punte delle costole
M3_TAP, M3_CLR = 1.3, 1.7  # raggi foro: autofilettante M3 / passante
M2_TAP = 0.85              # autofilettante M2 (viti servo)
BLADE_W, BLADE_T = 9.0, 0.4   # lama da cutter 9 mm

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

AX = {"X": (1, 0, 0), "Y": (0, 1, 0), "Z": (0, 0, 1)}
def to_print(shape, rots=()):
    """Ruota il pezzo nell'orientamento di stampa e lo appoggia su z=0, centrato in XY."""
    w = shape if isinstance(shape, cq.Workplane) else cq.Workplane("XY").add(shape)
    for ax, ang in rots:
        w = w.rotate((0, 0, 0), AX[ax], ang)
    bb = w.val().BoundingBox()
    return w.translate((-(bb.xmin + bb.xmax) / 2, -(bb.ymin + bb.ymax) / 2, -bb.zmin))

# ---- clip della fascetta e relativa tasca nel nucleo ----
CLIP_W, CLIP_L = 18.0, 18.4               # larghezza ganasce, lunghezza clip
CLIP_FLOOR, CLIP_POCKET_T, CLIP_POCKET_A = CORE_R - 4.7, 19.5, 21.0   # pavimento tasca (dall'asse), lato tangenziale, lato assiale
STRAP_W = 16.0                            # larghezza fascetta

# ---- spirale regolabile 35/120 (sistema a baionetta tipo OpenReel/Paterson, carico dal centro) ----
TUBE_R, TUBE_BORE = 16.0, 13.15                    # tubo centrale: foro Ø26.3 come la OpenReel
SLEEVE_R, SLEEVE_L = 19.5, 6.0                     # manicotto di guida della flangia (lato interno)
DRUM_HALF = 11.5                                   # semi-lunghezza del tamburo centrale
LUG_DEG, CHAN_DEG, LOCK_DEG = 24.0, 28.0, 30.0     # dente, canale assiale, rotazione di blocco
LUG_ANG = (0.0, 120.0, 240.0)                      # v5: tre denti; quello a 0 gradi e' la chiave (piu' largo)
LUG_KEY_DEG, CHAN_KEY_DEG = 32.0, 36.0             # dente chiave e relativo canale: la flangia entra in una sola posizione
NOTCH_ANG = 48.0                                   # tacche di trascinamento (tre per estremita'), lato A; lato B speculare
DET_ANG = 57.0                                     # gola dello scatto sul tubo (lato A: +57, lato B: -57)
DET_DEPTH, DET_HALF_TOP, DET_HALF_BOT = 0.45, 0.62, 0.12   # gola dello scatto: profondita' e semi-larghezze
FING_T, FING_HALF, FING_SLIT, FING_GAP = 1.0, 2.0, 0.6, 0.8  # linguetta a scatto: spessore radiale, semi-larghezza, fessure, gioco dietro
BUMP_H = 0.40                                      # sporgenza del dentino della linguetta
LUG_H, LUG_AX = 1.3, 3.0                           # sporgenza radiale e lunghezza assiale del dente (a V)
REEL_FIT = 0.06                                    # gioco flangia/tubo: radiale sul foro e assiale sui fianchi del dente (era 0.15)
FIN_GAP, FIN_R, FIN_T, FIN_N = 0.4, 45.9, 1.6, 8            # anello alette: gioco dalla flangia, raggio esterno, spessore, numero
FIN_PITCH = 50.0                                   # inclinazione delle alette rispetto al piano della flangia (>=45 per stampare senza supporti)
SPOKE_N, SPOKE_W = 16, 2.4                         # razze della flangia aperta: tra una razza e l'altra il disco e' vuoto
RIM_W = 2.5                                        # corona esterna della flangia
SOLE_STOP_R = CORE_R - 0.5                         # raggio a cui il fermo tiene la suola del braccio a spirale vuota
def lug_z(fmt):                                    # quota assiale (dal centro) del centro dente, lato A (negativo)
    return -(reel_width(fmt) / 2 + DISC_T) + LUG_AX / 2
FIN_H = 14.8                                       # altezza assiale dell'anello alette
def ring_face_y(fmt):                              # |y| della faccia piana dell'anello alette montato
    return flange_face_y(fmt) + FIN_GAP
RING_LUG = abs(lug_z("120")) - ring_face_y("135")   # quota del dente dell'anello dalla sua faccia piana (a 35 mm usa la stazione 120)
def ring_station(fmt):                             # |z| della stazione a baionetta usata dall'anello alette
    return ring_face_y(fmt) + RING_LUG
TUBE_HALF = ring_face_y("120") + FIN_H + 0.5       # il tubo arriva oltre gli anelli montati a 120 (tre stazioni per lato)
def sector(profile_rz, a0, a1):
    """Solido di rivoluzione attorno a Z del profilo (r, z) tra gli angoli a0 e a1 (gradi)."""
    w = cq.Workplane("XZ").polyline(profile_rz).close().revolve(a1 - a0, (0, 0, 0), (0, 1, 0))
    c = w.val().Center()
    ac = math.degrees(math.atan2(c.y, c.x))
    return w.rotate((0, 0, 0), (0, 0, 1), (a0 + a1) / 2 - ac)

# =====================================================================================================
# TANK con caricatore integrato (v5). Riferimento: X = marcia pellicola (caricatore a -X), Y = asse spirale
# (lato A / trasmissione a +Y, perno folle a -Y), Z = alto; z = 0 fondo interno vasca, asse spirale a z = HC.
# =====================================================================================================
T_WALL = 3.0
Z_BOT, Z_RIM = -5.0, 104.0                   # fondo esterno e bordo superiore della tank (la guida a spirale piena sale fino a z = 102)
RC = 49.25                                   # semi-larghezza interna della vasca spirale (flangia 46.75 + 2.5)
Y_DRV = TUBE_HALF + 5.0                      # faccia interna parete lato trasmissione
Y_IDL = -(TUBE_HALF + 7.5)                   # faccia interna parete lato perno folle (corsa del perno a molla)
X_PART = -RC - T_WALL                        # faccia del divisorio verso il ripiano asciutto
X_DRY0 = -114.5                              # faccia interna parete di fondo del ripiano
XO0, XO1 = -117.5, RC + T_WALL               # ingombro esterno in X
YO0, YO1 = Y_IDL - T_WALL, Y_DRV + T_WALL    # ingombro esterno in Y
Z_SH = 48.5                                  # piano del ripiano asciutto
Z_PART = 66.0                                # sommita' del divisorio vasca/ripiano
Z_OVF = 62.0                                 # soglia del troppopieno
FLOOR_L, FLOOR_R = (-20.4, 1.3), (20.4, 0.0) # piedi degli smussi a 45 gradi; il fondo pende verso la canaletta
GUT_X0, GUT_X1, GUT_Z = 14.4, 20.4, -2.5     # canaletta di raccolta (pescante)
TUN_X0, TUN_X1, TUN_ZF, TUN_ZW = -114.0, -74.0, -2.0, 25.0   # galleria del motore sotto il ripiano (tetto a 45 gradi)
MOT_X, MOT_Z = (TUN_X0 + TUN_X1) / 2, 17.5   # asse del riduttore JGB37-520
SBAY_X0, SBAY_X1, SBAY_Y0, SBAY_Y1, SBAY_ZT = -71.5, -55.2, -18.0, 34.0, 41.5   # vano del servo taglierina
SRV_X, SRV_Z, SRV_YTAB = -65.2, 30.0, 15.0   # asse del servo (parallelo a Y) e piano delle alette; la manovella spazza solo verso +X
CRANK_R = 6.4                                # manovella: corsa lama 12.8 mm
ROD_L = 28.9                                 # biella (interasse)
WRIST_Z = SRV_Z - CRANK_R + ROD_L            # quota dello spinotto a lama parcheggiata
# ponte e taglierina a "Λ" dal basso
ZF = 84.0                                    # quota nominale della linea pellicola
SLOT_Z0, SLOT_Z1 = 81.0, 84.6                # fessura pellicola nel ponte
BR_X0, BR_X1 = -81.0, -64.0                  # ponte
CH_X0, CH_X1 = -76.8, -69.1                  # canale del portalama
CAR_X0, CAR_X1 = -76.4, -72.4                # piastra portalama (4 mm)
BLADE_X0, BLADE_X1 = CAR_X1, CAR_X1 + BLADE_T
CLAMP_X1 = BLADE_X1 + 2.5
BLADE_TILT = 12.0                            # inclinazione dei due fili rispetto all'orizzontale
APEX_PARK = 79.0                             # quota della punta a riposo (2 mm sotto il piano della fessura)
STROKE = 2 * CRANK_R
BR_ZTOP = 95.6
COL_Y0, COL_Y1 = 37.0, 52.0                  # colonne del ponte (con le selle della barra guida all'esterno)
# guida a due slitte su barra quadra
XB, ZB = -53.0, 74.5                         # asse della barra
BAR = 8.0                                    # lato della barra
N_F = 8.0                                    # quota della linea pellicola sopra l'asse barra (riferimento slitta)
S_TIP = 66.0                                 # lunghezza della slitta dall'asse barra
HUB = 13.0                                   # lato esterno del mozzo
# caricatore 135 / rullo 120
CAS_X, CAS_Z, CAS_R = -96.0, ZF - 11.5, 13.0
# trasmissione
SHAFT_D = 8.0
PUL_R = 14.0                                 # raggio primitivo delle pulegge (cinghia tonda Ø3)

def loft_planar(sections):
    """Solido a facce piane tra sezioni poligonali 3D successive (stesso numero di vertici, stesso verso):
    testate poligonali, fianchi a quadrilateri se complanari, altrimenti divisi in due triangoli."""
    V = cq.Vector
    def face(pts):
        return cq.Face.makeFromWires(cq.Wire.makePolygon([V(*p) for p in pts], close=True))
    def planar(a, b, c, d):
        n = (V(*b) - V(*a)).cross(V(*c) - V(*a))
        if n.Length < 1e-12: return True
        return abs((V(*d) - V(*a)).dot(n.normalized())) < 1e-7
    faces = [face(sections[0]), face(sections[-1])]
    n = len(sections[0])
    for A, B in zip(sections[:-1], sections[1:]):
        for i in range(n):
            j = (i + 1) % n
            a, b, c, d = A[i], A[j], B[j], B[i]
            if planar(a, b, c, d):
                faces.append(face([a, b, c, d]))
            else:
                faces.append(face([a, b, c])); faces.append(face([a, c, d]))
    sh = cq.Shell.makeShell(faces)
    so = cq.Solid.makeSolid(sh)
    if so.Volume() < 0:
        so = cq.Solid(so.wrapped.Reversed())
    return so
