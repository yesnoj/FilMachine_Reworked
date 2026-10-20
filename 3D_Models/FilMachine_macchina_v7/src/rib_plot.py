"""Pianta delle due spirali (v6.2): flange 135 e flange 120, alla stessa scala, con quanta pellicola contengono."""
import os, math
os.chdir(os.path.dirname(os.path.abspath(__file__)))
import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
from common import *
import reel
fig, axs = plt.subplots(1, 2, figsize=(13, 6.8))
for ax, fmt, tit in ((axs[0], "135", "flange 135 (A01, A02)"), (axs[1], "120", "flange 120 (A10, A11)")):
    face = [fc for fc in reel.rib_2d(fmt).extrude(1.0).val().Faces() if abs(fc.normalAt().z) > 0.9][0]
    for e in face.Edges():
        pts = [e.positionAt(k / 40) for k in range(41)]
        ax.plot([p.x for p in pts], [p.y for p in pts], color="#222", lw=0.8)
    for r, c, l in ((SLEEVE_R, "#b85c3d", "manicotto della flangia"), (FLANGE_R, "#999", "bordo flangia (D 93,5)"), (TUBE_R, "#3d7ab8", "tubo")):
        t = [i / 200 * 2 * math.pi for i in range(201)]
        ax.plot([r * math.cos(a) for a in t], [r * math.sin(a) for a in t], color=c, lw=1, ls="--", label=l)
    arcs = reel.spiral_arcs(reel.SPIRALS[fmt]); p = 2 * (arcs[1][1] - arcs[0][1])
    ax.set_title(f"{tit}\n{len(arcs) / 2:.1f} giri a passo {p:.1f} mm, dal raggio {arcs[0][1]:.0f} al {arcs[-1][1]:.1f}: "
                 f"contengono {reel.spiral_length(fmt) / 10:.0f} cm di pellicola", fontsize=10.5)
    ax.set_aspect("equal"); ax.legend(fontsize=8, loc="lower right"); ax.set_xlim(-50, 50); ax.set_ylim(-50, 50)
fig.suptitle("Come nella Lab-Box: nel 120 meno giri e piu' distanziati (passo 1,38 volte quello del 135)", fontsize=12)
fig.savefig(os.path.join(OUT, "png", "spirale_135_120.png"), dpi=110, bbox_inches="tight")
