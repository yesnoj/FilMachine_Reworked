"""Come si monta la fascetta nel tubo A03 (v6.2, come nella Lab-Box): sezione a meta' finestra e sezione lungo l'asse,
dalla geometria vera del tubo, con il perno del capo e il percorso per infilarla."""
import os, math
os.chdir(os.path.dirname(os.path.abspath(__file__)))
import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Rectangle, FancyArrowPatch
import cadquery as cq
from common import *
import reel

TUBE_C, STRAP_C, PIN_C, PATH_C, PIN_T = "#c9ccd1", "#3d7ab8", "#d9822b", "#c0392b", "#7a4a10"
mm = lambda v: f"{v:.2f}".rstrip("0").rstrip(".").replace(".", ",")
t = reel.tube().rotate((0, 0, 0), (0, 0, 1), -reel.STRAP_SLOT_ANG)       # finestra stretta su +X, larga su -X

def faces_xy(wp, proj):
    out = []
    for f in wp.faces().vals():
        w = f.outerWire(); n = max(60, int(w.Length() / 0.2))
        out.append([proj(w.positionAt(k / n)) for k in range(n + 1)])
    return out

fig, (ax, bx) = plt.subplots(1, 2, figsize=(15, 7.4), gridspec_kw={"width_ratios": [1.25, 1]})

# ---- (a) sezione a meta' altezza della finestra (z = 0), vista lungo l'asse
for poly in faces_xy(cq.Workplane("XY").add(t).section(0.0), lambda p: (p.x, p.y)):
    ax.fill([p[0] for p in poly], [p[1] for p in poly], color=TUBE_C, ec="#333", lw=1.0, zorder=2)
pr = reel.STRAP_PIN_T / 2; pin_x = TUBE_BORE - pr - 0.05
ax.plot([-34, 34], [0, 0], color=PATH_C, lw=1.5, ls=(0, (6, 4)), zorder=3)                # percorso per infilarla
ax.add_patch(FancyArrowPatch((30, 0), (35, 0), arrowstyle="-|>", mutation_scale=18, color=PATH_C, lw=1.5, zorder=3))
ax.add_patch(Circle((pin_x, 0), pr, color=PIN_C, zorder=5))                             # piolino sul capo della fascetta
ax.plot([pin_x + pr, TUBE_R + 0.3], [0.12, 0.12], color=STRAP_C, lw=2.4, zorder=4, solid_capstyle="butt")
rs, a1 = TUBE_R + 0.35, 235.0                                                           # fuori: si avvolge sul tubo
arc = [math.radians(2 + k * (a1 - 2) / 90) for k in range(91)]
ax.plot([rs * math.cos(a) for a in arc], [rs * math.sin(a) for a in arc], color=STRAP_C, lw=2.4, zorder=4)
a = math.radians(a1); ex, ey, tx, ty = rs * math.cos(a), rs * math.sin(a), -math.sin(a), math.cos(a)
ax.add_patch(FancyArrowPatch((ex, ey), (ex + 13 * tx, ey + 13 * ty), arrowstyle="-|>", mutation_scale=16, color=STRAP_C, lw=2.4, zorder=4))
ax.text(ex + 13 * tx + 1.2, ey + 13 * ty, "verso la clip", color=STRAP_C, fontsize=10, va="center")
ax.text(-36, 19.5, "1  la fascetta si infila dalla finestra larga,\n    attraversa il tubo ed esce dalla finestra\n    stretta, dal lato opposto (tratteggio)",
        color=PATH_C, fontsize=9.8, va="bottom")
ax.annotate(f"2  il perno sul capo della fascetta passa\n    dalla finestra larga ma non dalla stretta:\n    resta dentro e la fascetta non esce",
            (pin_x - 0.4, 0.9), (8.5, 19.5), fontsize=9.8, color=PIN_T, va="bottom", arrowprops=dict(arrowstyle="->", color=PIN_T))
ax.annotate(f"finestra larga {mm(reel.STRAP_WIN_W)} mm", (-TUBE_R - 0.2, -1.5), (-36, -10), fontsize=9.8,
            arrowprops=dict(arrowstyle="->", color="#333"))
ax.annotate(f"finestra stretta {mm(reel.STRAP_SLOT_W)} mm", (TUBE_R + 0.2, -0.6), (18.5, -10), fontsize=9.8,
            arrowprops=dict(arrowstyle="->", color="#333"))
ax.text(-36, -24.5, f"fascetta della Lab-Box, comprata gia' fatta: larga {mm(STRAP_W)} mm, spessa {mm(reel.STRAP_T)}; perno sul capo spesso\n"
        f"{mm(reel.STRAP_PIN_T)} mm e lungo {mm(reel.STRAP_PIN_L)}. All'altro capo la clip: girando la spirale, la fascetta si avvolge sul tubo",
        fontsize=9.8, va="top")
ax.add_patch(Circle((-35.0, -23.0), 0.9, color=PIN_C, zorder=5))
ax.set_aspect("equal"); ax.set_xlim(-37, 37); ax.set_ylim(-33, 30); ax.axis("off")
ax.set_title("Sezione a meta' finestra, vista lungo l'asse", fontsize=11.5)

# ---- (b) sezione lungo l'asse, nel piano delle due finestre
for poly in faces_xy(cq.Workplane("XZ").add(t).section(0.0), lambda p: (p.x, p.z)):
    bx.fill([p[0] for p in poly], [p[1] for p in poly], color=TUBE_C, ec="#333", lw=1.0, zorder=2)
L = STRAP_W
bx.add_patch(Rectangle((pin_x + pr, -L / 2), TUBE_R + 0.6 - pin_x - pr, L, color=STRAP_C, alpha=0.35, zorder=3))
bx.add_patch(Rectangle((pin_x - pr, -reel.STRAP_PIN_L / 2), 2 * pr, reel.STRAP_PIN_L, color=PIN_C, zorder=5))
hz = reel.STRAP_WIN_H / 2
for x0, lab, ha in ((TUBE_R + 2.2, "finestra\nstretta", "left"), (-TUBE_R - 2.2, "finestra\nlarga", "right")):
    bx.annotate("", (x0, -hz), (x0, hz), arrowprops=dict(arrowstyle="<->", color="#333"))
    bx.text(x0 + (0.7 if ha == "left" else -0.7), 0, f"{lab}\nalte {mm(reel.STRAP_WIN_H)}", fontsize=9.8, va="center", ha=ha)
for s in (1, -1):
    bx.annotate("", (s * (TUBE_R + 0.6), reel.COLLAR_Z + reel.COLLAR_T / 2), (s * 5.0, 13.6), arrowprops=dict(arrowstyle="->", color="#333"))
bx.text(0, 13.8, "collarini", fontsize=9.8, ha="center", va="bottom")
bx.text(0, 11.6, f"(la fascetta e' larga {mm(STRAP_W)} mm)", fontsize=9, ha="center", va="bottom", color="#555")
bx.annotate("perno", (pin_x - 0.3, -5.0), (2.0, -12.8), fontsize=9.8, color=PIN_T, ha="center",
            arrowprops=dict(arrowstyle="->", color=PIN_T))
bx.set_aspect("equal"); bx.set_xlim(-29, 29); bx.set_ylim(-18, 18); bx.axis("off")
bx.set_title("Sezione lungo l'asse, nel piano delle due finestre", fontsize=11.5)
fig.suptitle("Tubo A03: passaggio della fascetta da parte a parte, come nella Lab-Box", fontsize=13)
fig.savefig(os.path.join(OUT, "png", "fascetta_tubo.png"), dpi=110, bbox_inches="tight")
