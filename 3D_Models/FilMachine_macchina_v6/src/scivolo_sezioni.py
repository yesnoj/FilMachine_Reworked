"""Sezioni dello scivolo (slitta destra e, specchiata, sinistra) in alcune stazioni: invito a C all'imbocco, spalla, canale tra le flange, punta."""
import os
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from common import OUT, N_F
import loader as L
st = {round(s[0], 1): s for s in L._ST}
pick = [(-7.0, "imbocco (s = -7): invito a C"), (-2.0, "s = -2"), (6.0, "s = 6: la gola si stringe"), (13.0, "s = 13: canale"),
        (17.0, "s = 17: tra le flange"), (L.S_TIP, "punta")]
fig, axs = plt.subplots(2, 3, figsize=(15, 7.4), sharey=True)
for ax, (s, tit) in zip(axs.flat, pick):
    poly = L._slide_section(st[round(s, 1)])
    ys = [p[1] for p in poly] + [poly[0][1]]; ns = [p[2] for p in poly] + [poly[0][2]]
    for sg in (1, -1):
        ax.fill([sg * y for y in ys], ns, color="#4a78b5", alpha=0.85)
    ax.axhline(N_F, color="#c77a12", lw=0.8, ls="--")
    ax.text(0, N_F + 0.3, "piano pellicola", fontsize=7, ha="center", color="#c77a12")
    ax.plot([-17.5, 17.5], [N_F, N_F], color="#c77a12", lw=2.2, alpha=0.6)          # pellicola 135 piatta, per confronto
    ax.set_title(tit, fontsize=9); ax.set_aspect("equal"); ax.set_xlim(-27, 27); ax.set_ylim(4, 25); ax.grid(alpha=0.2)
fig.suptitle("Scivolo tipo Lab-Box (135): l'invito a C e' all'imbocco, poi la spalla stringe la gola fino al canale tra le flange "
             "(arancio: pellicola 135 ancora piatta)", fontsize=11, y=0.98)
fig.tight_layout()
fig.savefig(os.path.join(OUT, "png", "scivolo_sezioni.png"), dpi=110, bbox_inches="tight")
