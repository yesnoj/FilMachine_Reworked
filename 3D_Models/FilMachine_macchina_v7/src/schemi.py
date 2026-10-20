"""Schemi: idraulico ed elettrico del rilascio 7 (PNG + PDF in docs/)."""
import os
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, FancyBboxPatch, Circle, Polygon
from common import OUT

DOCS = os.path.join(OUT, "docs")
W, CH, GR, RD, BL, GN = "#1f6fb5", "#c77a12", "#555555", "#b3261e", "#2b6cb0", "#2e8b57"

def valve(ax, x, y, label, col, horizontal=False, lab_dy=0.0):
    if horizontal:
        ax.add_patch(Polygon([(x - 0.28, y + 0.22), (x - 0.28, y - 0.22), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
        ax.add_patch(Polygon([(x + 0.28, y + 0.22), (x + 0.28, y - 0.22), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
        ax.add_patch(Rectangle((x - 0.13, y + 0.22), 0.26, 0.30, fc="#dddddd", ec=GR, lw=1))
        ax.text(x, y + 0.62 + lab_dy, label, fontsize=8.5, ha="center", va="bottom")
    else:
        ax.add_patch(Polygon([(x - 0.22, y + 0.28), (x + 0.22, y + 0.28), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
        ax.add_patch(Polygon([(x - 0.22, y - 0.28), (x + 0.22, y - 0.28), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
        ax.add_patch(Rectangle((x + 0.22, y - 0.13), 0.30, 0.26, fc="#dddddd", ec=GR, lw=1))
        ax.text(x + 0.60, y + lab_dy, label, fontsize=8.5, va="center", ha="left")

def tap(ax, x, y, col):
    ax.add_patch(Polygon([(x - 0.2, y + 0.25), (x + 0.2, y + 0.25), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
    ax.add_patch(Polygon([(x - 0.2, y - 0.25), (x + 0.2, y - 0.25), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
    ax.add_patch(Circle((x, y), 0.07, fc=col, ec=col)); ax.plot([x, x - 0.36], [y, y], color=col, lw=1.6); ax.plot([x - 0.36, x - 0.36], [y - 0.12, y + 0.12], color=col, lw=1.6)

def hydraulic():
    fig, ax = plt.subplots(figsize=(14.5, 10.6))
    ax.set_xlim(0, 17.6); ax.set_ylim(-1.6, 11.6); ax.axis("off")
    ax.text(0.2, 11.25, "FilMachine v7 - schema idraulico", fontsize=15, weight="bold")
    ax.text(0.2, 10.88, "Un collettore unico con sei valvole sul lato di aspirazione della pompa peristaltica (Kamoer, reversibile); dall'altra parte solo la tank. "
                        "Ogni spostamento tra recipienti passa dalla tank a cicli da 300 ml. Tank e contenitori immersi nel bagno.", fontsize=8.6, color=GR)
    # bagno con tank e contenitori
    ax.add_patch(Rectangle((0.5, 5.6), 11.0, 3.6, fc="#e3f0fb", ec=W, lw=2))
    ax.plot([0.5, 11.5], [8.4, 8.4], color=W, lw=0.8, ls=":"); ax.text(11.4, 8.46, "livello dell'acqua (3,1 l)", fontsize=7, color=W, ha="right")
    ax.text(11.4, 8.85, "BAGNO", fontsize=8, color=W, ha="right", weight="bold")
    ax.add_patch(Rectangle((0.9, 5.9), 3.0, 3.0, fc="#3a3c3e", ec="k", lw=1.2)); ax.text(2.4, 7.4, "TANK\n(spirale, caricatore)", fontsize=8.5, color="white", ha="center", va="center")
    ax.plot([0.9, 0.9], [5.9, 8.9], color="#8a6fbf", lw=4); ax.text(0.75, 9.3, "giunto\nmagnetico", fontsize=7, ha="center", color="#8a6fbf")
    xs = [5.6, 7.6, 9.6]
    for x, n, c in zip(xs, ("C1", "C2", "C3"), ("#f3d9a8", "#f0c9c9", "#d9e8c4")):
        ax.add_patch(Rectangle((x - 0.6, 5.9), 1.2, 2.6, fc=c, ec=CH, lw=1.4)); ax.text(x, 8.2, n, fontsize=9, ha="center", weight="bold")
        ax.text(x, 7.0, "500 ml", fontsize=7, ha="center", color=GR)
        ax.plot([x, x], [5.9, 4.7], color=CH, lw=1.8)                                  # passaparete nel fondo
        ax.add_patch(Circle((x, 4.9), 0.16, fc="white", ec=GR, lw=1)); ax.text(x + 0.22, 4.9, "MIN", fontsize=6, va="center")
        ax.add_patch(Rectangle((x - 0.18, 4.1), 0.36, 0.6, fc="#cfe9f2", ec=GR, lw=1)); ax.text(x + 0.25, 4.3, "camera", fontsize=6, va="center")
        ax.plot([x, x], [4.1, 3.0], color=CH, lw=1.8)
        valve(ax, x, 2.5, n, CH)
        ax.plot([x, x], [2.2, 1.6], color=CH, lw=1.8)
        ax.plot([x + 0.18, x + 1.0], [4.4, 4.4], color=RD, lw=1.4); ax.plot([x + 1.0, x + 1.0], [4.4, -0.3], color=RD, lw=1.4); tap(ax, x + 1.0, 0.2, RD)
        ax.text(x + 1.0, -0.85, "rubinetto", fontsize=6.5, ha="center", color=RD)
        ax.text(x + 0.62, 8.65, "MAX", fontsize=6, color=GR)
    # riscaldatori, sonda, ricircolo, sensori del bagno
    ax.plot([9.0, 11.2], [6.1, 6.1], color="#9c4d2f", lw=4); ax.plot([9.0, 11.2], [6.4, 6.4], color="#9c4d2f", lw=4); ax.text(10.1, 6.6, "2 cartucce 100 W", fontsize=6.5, ha="center", color="#9c4d2f")
    ax.add_patch(Rectangle((10.6, 7.0), 0.5, 0.6, fc="#d07b2a", ec="k", lw=0.8)); ax.text(10.85, 7.75, "ricircolo A7", fontsize=6.5, ha="center")
    ax.plot([11.3, 11.3], [6.0, 7.0], color="k", lw=1.2); ax.text(11.35, 7.1, "sonda", fontsize=6.5)
    ax.text(0.6, 5.3, "sensori del bagno MIN / MAX sulla parete posteriore (fuori)", fontsize=6.5, color=GR)
    # passaparete di servizio nel fondo: WB, troppopieno, ingresso
    ax.plot([4.6, 4.6], [5.9, 1.6], color=W, lw=1.8); ax.text(4.5, 5.0, "WB", fontsize=8, ha="right", color=W, weight="bold")
    ax.plot([8.6, 8.6], [8.5, 5.9], color=W, lw=1.8); ax.plot([8.6, 8.6], [5.9, 4.0], color=W, lw=1.8); ax.text(8.75, 5.2, "troppopieno\n(a filo)", fontsize=6, color=W)
    ax.plot([8.6, 12.5], [4.0, 4.0], color=W, lw=1.8); ax.text(12.6, 4.0, "TROPPOPIENO bagno", fontsize=7, va="center", color=W)
    ax.plot([10.9, 10.9], [9.0, 5.9], color=BL, lw=1.8); ax.plot([10.9, 10.9], [5.9, 3.4], color=BL, lw=1.8); ax.text(11.0, 9.1, "beccuccio sopra l'acqua (salto d'aria)", fontsize=6, color=BL)
    valve(ax, 10.9, 2.9, "INGRESSO (A5)", BL); ax.plot([10.9, 10.9], [2.6, 1.9], color=BL, lw=1.8)
    ax.add_patch(Rectangle((10.6, 1.3), 0.6, 0.6, fc="white", ec=BL, lw=1.4)); ax.text(11.3, 1.6, "flussimetro", fontsize=7, va="center", color=BL)
    ax.plot([10.9, 12.5], [1.0, 1.0], color=BL, lw=1.8); ax.plot([10.9, 10.9], [1.3, 1.0], color=BL, lw=1.8); ax.text(12.6, 1.0, "INGRESSO acqua di rete", fontsize=7, va="center", color=BL)
    # collettore
    ax.add_patch(Rectangle((1.4, 1.1), 10.0, 0.5, fc="#dde3ea", ec=GR, lw=1.4)); ax.text(6.4, 0.78, "COLLETTORE C01 (sei valvole sdraiate, lato aspirazione)", fontsize=8, ha="center", color=GR)
    valve(ax, 4.6, 2.5, "WB", W); valve(ax, 2.4, 2.5, "SCARICO", GR); valve(ax, 3.5, 2.5, "RECUPERO (A6)", GN, lab_dy=-0.45)
    ax.plot([2.4, 2.4], [1.6, 2.2], color=GR, lw=1.8); ax.plot([2.4, 2.4], [2.8, 4.2], color=GR, lw=1.8); ax.plot([2.4, 0.3], [4.2, 4.2], color=GR, lw=1.8); ax.text(0.5, 4.45, "SCARICO (retro)", fontsize=7, color=GR)
    ax.plot([3.5, 3.5], [1.6, 2.2], color=GN, lw=1.8); ax.plot([3.5, 3.5], [2.8, 3.6], color=GN, lw=1.8); ax.plot([3.5, 0.3], [3.6, 3.6], color=GN, lw=1.8); ax.text(0.5, 3.3, "RECUPERO (fronte): bottiglia", fontsize=7, color=GN)
    ax.plot([4.6, 4.6], [2.2, 1.6], color=W, lw=1.8)
    # pompa e tank
    ax.plot([1.4, 0.8], [1.35, 1.35], color="k", lw=1.8); ax.plot([0.8, 0.8], [1.35, -0.2], color="k", lw=1.8)
    ax.add_patch(Circle((0.8, -0.7), 0.42, fc="white", ec="k", lw=1.8)); ax.text(0.8, -0.7, "P", fontsize=10, ha="center", va="center", weight="bold")
    ax.text(1.35, -0.6, "Kamoer KPHM600, 600 ml/min, reversibile", fontsize=7.5, va="center")
    ax.plot([0.8, 0.8], [-1.12, -1.4], color="k", lw=1.8); ax.plot([0.8, 0.3], [-1.4, -1.4], color="k", lw=1.8); ax.plot([0.3, 0.3], [-1.4, 5.0], color="k", lw=1.8)
    ax.plot([0.3, 1.2], [5.0, 5.0], color="k", lw=1.8); ax.plot([1.2, 1.2], [5.0, 5.9], color="k", lw=1.8); ax.text(0.35, 5.1, "linea tank (silicone nero)", fontsize=6.5)
    ax.plot([3.2, 3.2], [5.9, 4.4], color="k", lw=1.2, ls="--"); ax.plot([3.2, 0.3], [4.4, 4.4], color="k", lw=1.2, ls="--"); ax.text(1.5, 4.55, "troppopieno tank (retro)", fontsize=6.5)
    # legenda dei cicli
    ax.text(12.6, 9.2, "Processo:", fontsize=9, weight="bold")
    ax.text(12.6, 8.85, "riempi tank: valvola Cx aperta, pompa in aspirazione\nsvuota tank: valvola Cx (riuso) o SCARICO, pompa in mandata + 4 s d'aria\nrisciacqui: WB -> tank -> SCARICO", fontsize=7.5, va="top")
    ax.text(12.6, 7.4, "Strumenti -> Liquidi:", fontsize=9, weight="bold")
    ax.text(12.6, 7.05, "Riempi bagno: INGRESSO fino al MAX del bagno\nRiempi chimica: tank -> Cx\nSvuota chimiche: WB->tank->SCARICO, Cx->SCARICO (30 ml),\n   Cx->tank->RECUPERO a cicli finche' MIN(Cx) vede aria, WB->SCARICO\nSvuota macchina: tutto -> SCARICO\nPulisci macchina: WB -> tank -> ogni bocca", fontsize=7.5, va="top")
    ax.text(12.6, 3.3, "Misure:", fontsize=9, weight="bold")
    ax.text(12.6, 2.95, "tubo 3/8\" a innesto rapido (LLDPE 9,52 mm)\nsilicone 8x12 (rubinetti, recupero, tank)\nsilicone 6x10 sulla Kamoer\ncamera di livello: XKC-Y21 (MIN = la linea ha liquido)\nsensori MAX incollati sui contenitori sopra l'acqua", fontsize=7.5, va="top")
    for ext in ("png", "pdf"):
        fig.savefig(os.path.join(DOCS if ext == "pdf" else os.path.join(OUT, "png"), "schema_idraulico." + ext), dpi=150, bbox_inches="tight")
    plt.close(fig)

def electrical():
    fig, ax = plt.subplots(figsize=(14.5, 10.0))
    ax.set_xlim(0, 17.6); ax.set_ylim(-0.6, 11.2); ax.axis("off")
    ax.text(0.2, 10.85, "FilMachine v7 - schema elettrico (collegamenti alla Mainboard v1.1)", fontsize=15, weight="bold")
    ax.text(0.2, 10.5, "Rete 230 V -> presa IEC con interruttore e fusibile -> LRS-350-12. Dall'alimentatore tre coppie +V/-V ai tre ingressi 12 V della scheda. "
                       "Tutto il resto parte dalla scheda.", fontsize=8.6, color=GR)
    ax.add_patch(FancyBboxPatch((0.4, 7.4), 3.2, 2.4, boxstyle="round,pad=0.05", fc="#f2f2f2", ec="k", lw=1.4)); ax.text(2.0, 9.45, "IEC C14 + interruttore\n+ fusibile 3,15 A", fontsize=8.5, ha="center", va="center")
    ax.add_patch(FancyBboxPatch((4.4, 7.4), 3.6, 2.4, boxstyle="round,pad=0.05", fc="#e4e6e8", ec="k", lw=1.4)); ax.text(6.2, 8.6, "Mean Well LRS-350-12\n12 V 29 A\n(in piedi, morsetti verso il retro)", fontsize=8.5, ha="center", va="center")
    ax.annotate("", (4.4, 8.6), (3.6, 8.6), arrowprops=dict(arrowstyle="->", lw=1.6)); ax.text(4.0, 8.75, "230 V", fontsize=7, ha="center")
    ax.add_patch(FancyBboxPatch((9.0, 2.0), 4.4, 7.8, boxstyle="round,pad=0.05", fc="#e8f3ea", ec="#2f7d4f", lw=2)); ax.text(11.2, 9.45, "FilMachine Mainboard v1.1", fontsize=10, ha="center", weight="bold", color="#2f7d4f")
    for i, (j, lab) in enumerate((("J1", "12V IN HEATER 1"), ("J2", "12V IN HEATER 2"), ("J3", "12V IN MAIN"))):
        y = 8.9 - i * 0.5
        ax.annotate("", (9.0, y), (8.0, y), arrowprops=dict(arrowstyle="->", lw=1.4, color=RD)); ax.text(9.1, y, f"{j} {lab}", fontsize=7.5, va="center")
    rows = [("J23 HEATER 1", "cartuccia 100 W n.1 (+ KSD301 in serie)", "#9c4d2f"), ("J24 HEATER 2", "cartuccia 100 W n.2 (+ KSD301 in serie)", "#9c4d2f"),
            ("J25 PUMP", "Kamoer KPHM600 (ponte H canale B, verso invertibile)", "#d07b2a"), ("J26 MOTOR", "JGB37-520 della spirale (nel vano asciutto)", "#8a6fbf"),
            ("J15..J20 VALVE", "C1, C2, C3, WB, WASTE, INLET (A0..A5)", CH), ("J21 SPARE OUT 1 (A6)", "valvola RECUPERO", GN),
            ("J22 SPARE OUT 2 (A7)", "pompetta di ricircolo del bagno", "#d07b2a"), ("J7 BATH LEVEL", "XKC-Y21 MIN e MAX del bagno (parete posteriore)", BL),
            ("J8..J10 CHEM 1/2/3", "XKC-Y21 MIN (camera C03) e MAX (contenitore) di ogni chimica", BL), ("J11 FLOW", "YF-S201", BL),
            ("J12 HALL", "KY-003 sulla staffa B21 (magneti indicatori del mozzo)", "#8a6fbf"), ("J13 TEMP 1", "DS18B20 del bagno", "#777"),
            ("J27 SERVO", "MG90S della taglierina (cavo attraverso la testata, passacavo)", "#3b6fd1"), ("J5 DISPLAY JP1", "flat 2x13 verso il JC4880P433 (subito davanti)", "#20262e")]
    for i, (j, what, col) in enumerate(rows):
        y = 7.6 - i * 0.42
        ax.text(9.15, y, j, fontsize=7.5, va="center", weight="bold"); ax.annotate("", (14.4, y), (13.4, y), arrowprops=dict(arrowstyle="->", lw=1.2, color=col))
        ax.text(14.5, y, what, fontsize=7.5, va="center", color=col)
    ax.text(0.3, 6.6, "Cavi che attraversano il piano (fori Ø22 e Ø16): vano motore (motore, Hall, servo), intercapedine destra (riscaldatori, con i termostati),\n"
                      "intercapedine posteriore (sensori del bagno, sonda, pompetta: i cavi di sonda e pompetta scavalcano la parete nella tacca del coperchio).\n"
                      "Sensori MAX dei contenitori e della camera: cavi dentro il bagno? NO: i sensori MAX stanno fuori dai contenitori, sopra l'acqua; i loro cavi\n"
                      "salgono sul bordo posteriore e scendono nell'intercapedine. Tenere i cavi lontani dall'acqua: nessun giunto sotto il livello.", fontsize=7.5, va="top", color=GR)
    ax.text(0.3, 4.3, "Note:", fontsize=9, weight="bold")
    ax.text(0.3, 3.95, "- A6 e A7 sono le uscite SPARE 1/2 della scheda (MOSFET 2 A): il firmware v7 le usa per RECUPERO e ricircolo.\n"
                       "- Il motore e' coassiale con la spirale: nessuna cinghia, nessun paraolio; il Hall legge la spirale (mozzo), non il motore.\n"
                       "- I riscaldatori restano a 100 W: con 3 litri d'acqua la regolazione va fatta nel firmware (un solo riscaldatore vicino al set-point).\n"
                       "- Il secondo DS18B20 (chimica) non e' montato: i contenitori sono chiusi; la chimica e' alla temperatura del bagno.", fontsize=7.5, va="top")
    for ext in ("png", "pdf"):
        fig.savefig(os.path.join(DOCS if ext == "pdf" else os.path.join(OUT, "png"), "schema_elettrico." + ext), dpi=150, bbox_inches="tight")
    plt.close(fig)

if __name__ == "__main__":
    hydraulic(); electrical(); print("schemi: ok")
