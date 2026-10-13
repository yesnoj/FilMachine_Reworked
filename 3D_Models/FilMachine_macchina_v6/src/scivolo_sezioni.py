"""Sezioni dello scivolo (slitta destra e, specchiata, sinistra) in alcune stazioni: invito a C all'imbocco, spalla, canale tra le flange, punta."""
import os, math
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from common import OUT, N_F, arch
import loader as L
pick = [(-5.0, "imbocco (s = -5): il cappuccio con il bordo a C"), (0.0, "s = 0: la spalla stringe la gola"), (6.0, "s = 6"),
        (13.0, "s = 13: canale, tra le flange"), (30.0, "s = 30: linguetta"), (L.S_TIP, "punta")]
chord, sag, R = arch("135")
fig, axs = plt.subplots(2, 3, figsize=(15, 7.4), sharey=True)
for ax, (s, tit) in zip(axs.flat, pick):
    poly = L.slide_section(s)
    ys = [p[0] for p in poly] + [poly[0][0]]; ns = [p[1] for p in poly] + [poly[0][1]]
    for sg in (1, -1):
        ax.fill([sg * y for y in ys], ns, color="#4a78b5", alpha=0.85)
    ax.axhline(N_F, color="#c77a12", lw=0.8, ls="--")
    ax.text(0, N_F - 1.2, "piano pellicola", fontsize=7, ha="center", color="#c77a12")
    if s < 0:
        ax.plot([-17.5, 17.5], [N_F, N_F], color="#c77a12", lw=2.2, alpha=0.7)          # pellicola 135 piatta all'imbocco
    elif s >= 13.0:
        th = math.asin(chord / 2 / R)                                                    # pellicola incurvata tra le flange
        a = [(-th + 2 * th * k / 60) for k in range(61)]
        ax.plot([R * math.sin(t) for t in a], [N_F + R * math.cos(t) - R * math.cos(th) for t in a], color="#c77a12", lw=2.2, alpha=0.7)
    ax.set_title(tit, fontsize=9); ax.set_aspect("equal"); ax.set_xlim(-27, 27); ax.set_ylim(3, 25); ax.grid(alpha=0.2)
fig.suptitle("Scivolo tipo Lab-Box (135): suola sottile, parete, labbro a C; all'imbocco l'invito e' aperto, tra le flange la gola "
             "tiene i bordi della pellicola incurvata (arancio: pellicola 135)", fontsize=11, y=0.98)
fig.tight_layout()
fig.savefig(os.path.join(OUT, "png", "scivolo_sezioni.png"), dpi=110, bbox_inches="tight")
