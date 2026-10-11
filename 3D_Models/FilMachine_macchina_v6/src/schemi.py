"""Schemi: idraulico ed elettrico (PNG + PDF in docs/)."""
import os
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, FancyBboxPatch, Circle, Polygon
from common import OUT

DOCS = os.path.join(OUT, "docs")
W, CH, GR, RD, BL = "#1f6fb5", "#c77a12", "#555555", "#b3261e", "#2b6cb0"

def valve(ax, x, y, label, col, left=False):
    ax.add_patch(Polygon([(x - 0.22, y + 0.28), (x + 0.22, y + 0.28), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
    ax.add_patch(Polygon([(x - 0.22, y - 0.28), (x + 0.22, y - 0.28), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
    sg = -1 if left else 1
    ax.add_patch(Rectangle((x + 0.22 if not left else x - 0.52, y - 0.13), 0.30, 0.26, fc="#dddddd", ec=GR, lw=1))
    ax.text(x + sg * 0.60, y, label, fontsize=9, va="center", ha="right" if left else "left")

def tap(ax, x, y, col, vertical=True):
    """Rubinetto manuale a sfera: farfalla con il pallino e la leva a T."""
    if vertical:
        ax.add_patch(Polygon([(x - 0.2, y + 0.25), (x + 0.2, y + 0.25), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
        ax.add_patch(Polygon([(x - 0.2, y - 0.25), (x + 0.2, y - 0.25), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
        ax.add_patch(Circle((x, y), 0.07, fc=col, ec=col)); ax.plot([x, x - 0.36], [y, y], color=col, lw=1.6); ax.plot([x - 0.36, x - 0.36], [y - 0.12, y + 0.12], color=col, lw=1.6)
    else:
        ax.add_patch(Polygon([(x - 0.25, y + 0.2), (x - 0.25, y - 0.2), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
        ax.add_patch(Polygon([(x + 0.25, y + 0.2), (x + 0.25, y - 0.2), (x, y)], closed=True, fc="white", ec=col, lw=1.6))
        ax.add_patch(Circle((x, y), 0.07, fc=col, ec=col)); ax.plot([x, x], [y, y + 0.36], color=col, lw=1.6); ax.plot([x - 0.12, x + 0.12], [y + 0.36, y + 0.36], color=col, lw=1.6)

def hydraulic():
    fig, ax = plt.subplots(figsize=(14.5, 10.6))
    ax.set_xlim(0, 17.6); ax.set_ylim(-1.4, 11.6); ax.axis("off")
    ax.text(0.2, 11.25, "FilMachine v6 - schema idraulico", fontsize=15, weight="bold")
    ax.text(0.2, 10.88, "Una valvola per recipiente su un collettore unico; pompa reversibile tra collettore e tank. Le vaschette pescano dal fondo "
                        "(innesto a pressione con otturatore); nessun foro sotto il livello del bagno. Tubo 3/8\" (9,52 mm), raccordi a innesto rapido.", fontsize=8.6, color=GR)
    # bagno
    ax.add_patch(Rectangle((0.5, 5.6), 9.6, 3.6, fc="#e3f0fb", ec=W, lw=2))
    ax.plot([0.5, 10.1], [8.3, 8.3], color=W, lw=0.8, ls=":"); ax.text(10.0, 8.36, "livello di lavoro", fontsize=7, color=W, ha="right")
    ax.text(10.0, 8.75, "BAGNO 13 l", fontsize=8, color=W, ha="right", weight="bold")
    xs = [1.6, 3.8, 6.0]
    for x, n, c in zip(xs, ("C1", "C2", "C3"), ("#f3d9a8", "#f0c9c9", "#d9e8c4")):
        ax.add_patch(Rectangle((x - 0.6, 6.55), 1.2, 2.95, fc=c, ec=CH, lw=1.6))
        ax.text(x, 7.9, n, fontsize=13, ha="center", color=CH, weight="bold")
        ax.add_patch(Rectangle((x - 0.62, 9.0), 0.22, 0.32, fc="#2f8f5b", ec="none")); ax.text(x - 0.36, 9.16, "MAX", fontsize=6, ha="left", va="center", color="#2f8f5b")
        ax.add_patch(Rectangle((x - 0.14, 6.15), 0.28, 0.4, fc="#d9d9d9", ec=GR, lw=1))                        # innesto
        ax.add_patch(Rectangle((x - 0.22, 5.75), 0.44, 0.4, fc="#e8922d", ec=GR, lw=1))                        # presa
        ax.plot([x, x, x + 0.9 + 0.0, x + 0.9, x + 1.0, x + 1.0], [5.75, 5.68, 5.68, 9.75, 9.75, 4.75], color=CH, lw=2.2)
        ax.add_patch(Rectangle((x + 0.82, 4.15), 0.36, 0.6, fc="#cfe9f2", ec=CH, lw=1.3))
        ax.add_patch(Rectangle((x + 1.2, 4.3), 0.16, 0.3, fc="#2f8f5b", ec="none")); ax.text(x + 1.4, 4.45, "MIN", fontsize=6, color="#2f8f5b", va="center")
        ax.plot([x + 1.0, x + 1.0], [4.15, 3.92], color=CH, lw=2.2)
        valve(ax, x + 1.0, 3.64, f"EV {n}", CH)
        ax.plot([x + 1.0, x + 1.0], [3.36, 2.6], color=CH, lw=2.2)
    ax.text(0.5, 10.45, "3 vaschette GN 1/9 a filo del coperchio: innesto dal basso con otturatore (si sollevano piene senza gocciolare); la linea esce dal bagno "
                        "scavalcando il bordo", fontsize=8.0, ha="left", color=CH)
    ax.text(0.55, 5.28, "presa + innesto", fontsize=7, color=GR)
    ax.text(0.55, 4.45, "camere di livello C15\n(sensore MIN: vuota =\nvaschetta vuota)", fontsize=7, color=GR, va="center")
    xw = 8.6
    ax.plot([xw, xw, 10.6, 10.6], [6.2, 9.75, 9.75, 3.92], color=W, lw=2.2)
    ax.text(xw - 0.12, 7.0, "pescante\nacqua (WB)", fontsize=8, ha="right", color=W)
    valve(ax, 10.6, 3.64, "EV WB", W)
    ax.plot([10.6, 10.6], [3.36, 2.6], color=W, lw=2.2)
    # collettore
    ax.plot([2.6, 13.6], [2.6, 2.6], color="black", lw=4)
    ax.text(2.65, 2.25, "COLLETTORE (C07)", fontsize=9, weight="bold")
    ax.text(4.65, 2.25, "sulla macchina le valvole stanno, da sinistra: C1, WB, C2, C3, scarico", fontsize=7.5, color=GR, va="baseline")
    # scarico
    ax.plot([12.1, 12.1], [2.6, 2.03], color=GR, lw=2.2)
    valve(ax, 12.1, 1.75, "EV SCARICO", GR, left=True)
    ax.plot([12.1, 12.1], [1.47, 0.75], color=GR, lw=2.2)
    ax.annotate("", xy=(12.1, 0.45), xytext=(12.1, 0.8), arrowprops=dict(arrowstyle="-|>", color=GR, lw=2))
    ax.text(11.9, 0.6, "SCARICO (pompato)", fontsize=9, ha="right", weight="bold", color=GR)
    # pompa
    ax.add_patch(Circle((14.02, 2.6), 0.42, fc="#fff2cc", ec="black", lw=2)); ax.text(14.02, 2.6, "P", fontsize=14, ha="center", va="center", weight="bold")
    ax.text(14.02, 1.95, "pompa a ingranaggi reversibile\nMarco UP3-R (PWM basso)", fontsize=8.2, ha="center", va="top")
    ax.annotate("", xy=(15.35, 3.75), xytext=(14.85, 3.75), arrowprops=dict(arrowstyle="-|>", color=RD, lw=1.6)); ax.text(14.78, 3.75, "IN = riempie la tank", fontsize=7.5, va="center", ha="right", color=RD)
    ax.annotate("", xy=(14.85, 3.42), xytext=(15.35, 3.42), arrowprops=dict(arrowstyle="-|>", color=BL, lw=1.6)); ax.text(14.78, 3.42, "OUT = svuota la tank", fontsize=7.5, va="center", ha="right", color=BL)
    # tank
    ax.add_patch(Rectangle((13.4, 5.6), 2.5, 2.2, fc="#eeeeee", ec="black", lw=2))
    ax.add_patch(Rectangle((13.42, 5.62), 2.46, 0.95, fc="#f6e7c8", ec="none"))
    ax.add_patch(Circle((14.4, 6.6), 0.62, fc="none", ec=GR, lw=1.2, ls="--"))
    ax.plot([14.44, 16.4, 16.4, 15.55, 15.55], [2.6, 2.6, 7.4, 7.4, 5.75], color="black", lw=3)
    ax.text(14.65, 8.05, "TANK a filo del piano", fontsize=9, ha="center", weight="bold")
    ax.text(13.5, 5.72, "250 / 350 ml", fontsize=8)
    ax.text(16.5, 6.6, "pescante\ndall'alto,\nportagomma\nlaterale", fontsize=7, va="center", color=GR)
    ax.plot([13.4, 12.4, 12.4], [7.1, 7.1, 4.95], color=GR, lw=2.0)
    ax.annotate("", xy=(12.4, 4.65), xytext=(12.4, 5.0), arrowprops=dict(arrowstyle="-|>", color=GR, lw=2))
    ax.text(12.52, 4.75, "TROPPOPIENO TANK", fontsize=8.5, ha="left", weight="bold", color=GR)
    # ingresso acqua di rete
    yi = 9.95
    ax.annotate("", xy=(16.2, yi), xytext=(17.4, yi), arrowprops=dict(arrowstyle="-|>", color=W, lw=2))
    ax.text(17.45, yi + 0.18, "INGRESSO\nrete", fontsize=8, ha="right", va="bottom", weight="bold", color=W)
    ax.add_patch(Circle((15.6, yi), 0.3, fc="white", ec=W, lw=1.6)); ax.text(15.6, yi, "F", fontsize=10, ha="center", va="center", color=W, weight="bold")
    ax.text(15.6, yi - 0.55, "flussimetro\nYF-S201", fontsize=7, ha="center", va="top", color=W)
    ax.plot([16.2, 15.9], [yi, yi], color=W, lw=2.2); ax.plot([15.3, 14.6], [yi, yi], color=W, lw=2.2)
    valve(ax, 14.3, yi, "", W)
    ax.text(14.3, yi + 0.42, "EV INGRESSO (6a valvola)", fontsize=7.5, ha="center", color=W)
    ax.plot([14.0, 9.3, 9.3], [yi, yi, 9.45], color=W, lw=2.2)
    ax.text(9.45, 10.1, "beccuccio sopra il troppopieno (salto d'aria)", fontsize=6.8, color=W, va="bottom")
    # troppopieno del bagno
    ax.plot([10.1, 11.1, 11.1], [8.75, 8.75, 8.3], color=W, lw=2.4)
    ax.annotate("", xy=(11.1, 8.0), xytext=(11.1, 8.35), arrowprops=dict(arrowstyle="-|>", color=W, lw=2))
    ax.text(11.25, 8.15, "TROPPOPIENO BAGNO\n(a gravita', tubo 12x16)", fontsize=7.5, ha="left", va="center", weight="bold", color=W)
    # riscaldatori e sonde
    for xh in (8.0, 9.6):
        ax.add_patch(Rectangle((xh - 0.35, 5.68), 0.7, 0.14, fc="#f77f00", ec="#9a4d00", lw=1))
    ax.text(8.8, 5.95, "2 riscaldatori, distanziati", fontsize=6.8, ha="center", color="#9a4d00")
    ax.add_patch(Rectangle((9.3, 6.4), 0.1, 0.45, fc=RD, ec=RD)); ax.text(9.47, 6.62, "sonda\nbagno", fontsize=6.5, ha="left", va="center", color=RD)
    ax.add_patch(Rectangle((1.95, 7.0), 0.1, 0.45, fc=RD, ec=RD)); ax.text(1.88, 7.22, "sonda\nchimica", fontsize=6.5, ha="right", va="center", color=RD)
    # rubinetto del bagno (in basso sulla parete destra, pescante quasi al fondo)
    ax.plot([9.0, 9.0], [5.62, 5.5], color=W, lw=2.2)
    tap(ax, 9.0, 5.25, W)
    ax.annotate("", xy=(9.0, 4.7), xytext=(9.0, 5.02), arrowprops=dict(arrowstyle="-|>", color=W, lw=2))
    ax.text(9.0, 4.45, "RUBINETTO BAGNO\n(manuale, a sifone)", fontsize=6.6, ha="center", va="center", weight="bold", color=W)
    ax.text(0.55, 1.75, "Per riversare una chimica nella sua bottiglia: si appoggia la vaschetta\nsull'imbuto C20 infilato nella bottiglia (stesso perno della presa:\nl'otturatore si apre, e si richiude da solo quando la si alza).", fontsize=8, color=CH, va="top", weight="bold")
    # note
    ax.text(0.5, -0.15, "Riempimento della tank: EV della sorgente aperta + pompa IN.  Ritorno alla vaschetta: stessa EV + pompa OUT.  Scarto: EV SCARICO + pompa OUT.\n"
                       "I risciacqui prendono l'acqua dal bagno (WB). Il bagno si riempie dalla rete: EV INGRESSO aperta, flussimetro e sensori MIN / MAX del bagno.\n"
                       "Nel bagno: 2 riscaldatori 12 V 100 W (uno davanti, uno dietro), 2 sensori di livello, sonda DS18B20; seconda sonda nella vaschetta C1. Otto sensori XKC-Y21 in tutto.", fontsize=8.6, va="top",
            bbox=dict(boxstyle="round", fc="#fafafa", ec="#cccccc"))
    for e in ("png", "pdf"):
        fig.savefig(os.path.join(DOCS, "schema_idraulico." + e), dpi=140, bbox_inches="tight")
    plt.close(fig)

def block(ax, x, y, w, h, title, lines=(), fc="#f4f7fb", ec="#33475b"):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02,rounding_size=0.08", fc=fc, ec=ec, lw=1.4))
    ax.text(x + w / 2, y + h - 0.22, title, fontsize=8.8, ha="center", va="top", weight="bold")
    for i, l in enumerate(lines):
        ax.text(x + 0.12, y + h - 0.60 - 0.27 * i, l, fontsize=7.2, va="top")

def wire(ax, pts, col="#444444", lw=1.3, label=None, lpos=None):
    ax.plot([p[0] for p in pts], [p[1] for p in pts], color=col, lw=lw, solid_capstyle="round")
    if label:
        x, y = lpos or pts[len(pts) // 2]
        ax.text(x, y + 0.08, label, fontsize=6.6, color=col, ha="center", va="bottom")

def electrical():
    fig, ax = plt.subplots(figsize=(16.5, 10.6))
    ax.set_xlim(0, 20.2); ax.set_ylim(0, 12.6); ax.axis("off")
    ax.text(0.2, 12.2, "FilMachine v6 - schema elettrico (collegamenti come nel firmware: boards/board_jc4880p433.h)", fontsize=14, weight="bold")
    P12, P5, SIG, I2C = "#b3261e", "#c77a12", "#2b6cb0", "#2e7d32"
    # colonna 1: alimentazione
    block(ax, 0.3, 9.6, 2.7, 1.9, "Rete 230 V", ["presa IEC con interruttore", "e fusibile; terra collegata", "(presa con differenziale)"], fc="#fdecea", ec=P12)
    block(ax, 0.3, 6.6, 2.7, 2.2, "Mean Well LRS-350-12", ["12 V 29 A, ventola a temperatura", "V+ / V-  ->  barra 12 V", "PE alla presa IEC"], fc="#fdecea", ec=P12)
    wire(ax, [(1.65, 9.6), (1.65, 8.8)], P12, 2.2, "L N PE", (2.25, 9.05))
    block(ax, 0.3, 2.6, 2.7, 2.0, "Convertitore LM2596S", ["12 V -> 5 V (regolare a 5,0 V", "prima di collegare)", "alimenta: scheda display,", "sensori di livello, servo"], fc="#fff4e0", ec=P5)
    # colonna 2: sensori
    block(ax, 3.6, 10.0, 3.0, 1.5, "2 sonde DS18B20", ["bagno + chimica (vaschetta C1)", "dati su GPIO35, pull-up 4,7 k a 3,3 V"], fc="#ffffff")
    block(ax, 3.6, 8.4, 3.0, 1.2, "Sensore Hall KY-003", ["sulla ruota magneti  ->  GPIO31"], fc="#ffffff")
    block(ax, 3.6, 6.4, 3.0, 1.6, "8 sensori XKC-Y21 (5 V)", ["bagno MIN -> GPIO29, MAX -> GPIO30", "vaschette MAX e camere MIN", "-> MCP23017 B0..B5"], fc="#ffffff")
    block(ax, 3.6, 5.35, 3.0, 0.95, "Flussimetro YF-S201 (ingresso)", ["impulsi -> GPIO52 (5 V: partitore)"], fc="#ffffff")
    block(ax, 3.6, 2.6, 3.0, 1.6, "Servo taglierina MG90S", ["5 V dal convertitore", "segnale: GPIO28"], fc="#fff4e0", ec=P5)
    wire(ax, [(6.6, 5.75), (7.2, 5.75)], SIG)
    wire(ax, [(3.0, 3.6), (3.6, 3.6)], P5, 1.8, "5 V", (3.3, 3.6))
    # colonna 3: scheda, breakout, driver
    block(ax, 7.2, 7.1, 5.2, 4.4, "Scheda display Guition JC4880P443C (ESP32-P4)", [
        "JP1  9  GPIO51  PUMP_ENA (PWM)", "JP1 11  GPIO50  PUMP_IN2", "JP1 13  GPIO49  PUMP_IN1",
        "JP1 19  GPIO32  MOTOR_ENA (PWM)", "JP1  8  GPIO33  MOTOR_IN1", "JP1 17  GPIO34  MOTOR_IN2",
        "JP1 23 / 25  GPIO7 / GPIO8  I2C SDA / SCL", "JP1 15  GPIO35  OneWire DS18B20", "JP1 10  GPIO31  Hall KY-003",
        "JP1 14  GPIO29  livello MIN bagno", "JP1 12  GPIO30  livello MAX bagno", "JP1  7  GPIO52  flussimetro (ingresso acqua)",
        "JP1 21  GPIO28  servo taglierina"], fc="#eaf2fb", ec=SIG)
    block(ax, 7.2, 5.9, 5.2, 0.85, "Breakout DC3-26P + cavo flat 2x13 (JP1 ai morsetti)", [], fc="#eaf2fb", ec=SIG)
    wire(ax, [(9.8, 7.1), (9.8, 6.75)], SIG, 2.0)
    for y in (10.75, 9.0, 7.3):
        wire(ax, [(6.6, y), (7.2, y)], SIG)
    block(ax, 7.2, 0.9, 5.2, 3.5, "Driver Adafruit #6318 (MCP23017, I2C 0x20)", [
        "A0  EV C1      A1  EV C2      A2  EV C3", "A3  EV WB     A4  EV SCARICO", "A5  EV INGRESSO *   A6 libero;  A7 non usato",
        "B0..B5  livelli: C1 MIN/MAX, C2 MIN/MAX, C3 MIN/MAX", "B6  riscaldatore 1      B7  riscaldatore 2", "alimentazione: 3,3 V (logica) + 12 V (carichi)"], fc="#f1f8e9", ec=I2C)
    wire(ax, [(8.2, 5.9), (8.2, 4.4)], I2C, 1.8, "I2C + 3,3 V", (7.75, 4.55))
    # colonna 4 e 5: potenza
    block(ax, 13.2, 9.9, 2.6, 1.6, "Altoparlante 8 ohm 2 W", ["sul connettore SPK", "della scheda"], fc="#eaf2fb", ec=SIG)
    wire(ax, [(12.4, 10.7), (13.2, 10.7)], SIG)
    block(ax, 13.2, 6.5, 3.4, 2.7, "Ponte H doppio 30 A (DBH-12V)", ["canale A  <-  GPIO33 / 34 / 32", "canale B  <-  GPIO49 / 50 / 51", "(IN1 / IN2 / ENA)", "alimentazione 12 V dalla barra"], fc="#f1f8e9", ec=I2C)
    wire(ax, [(12.4, 8.0), (13.2, 8.0)], SIG, 1.6, "6 fili", (12.8, 8.0))
    block(ax, 17.3, 8.2, 2.7, 1.0, "Motoriduttore JGB37-520", ["12 V, 20 giri/min: agitazione"], fc="#ffffff")
    block(ax, 17.3, 6.5, 2.7, 1.0, "Pompa Marco UP3-R", ["12 V 6 A, reversibile"], fc="#ffffff")
    wire(ax, [(16.6, 8.7), (17.3, 8.7)], P12, 2.0, "A", (16.95, 8.7)); wire(ax, [(16.6, 7.0), (17.3, 7.0)], P12, 2.0, "B", (16.95, 7.0))
    block(ax, 13.2, 3.3, 3.4, 1.3, "6 elettrovalvole 12 V NC", ["C1, C2, C3, WB, SCARICO, INGRESSO", "comune al +12 V"], fc="#ffffff")
    wire(ax, [(12.4, 3.9), (13.2, 3.9)], P12, 1.8, "A0..A5", (12.8, 3.9))
    block(ax, 13.2, 0.9, 3.4, 1.9, "Modulo MOSFET IRF540 (2 canali)", ["ingressi da B6 e B7", "8,3 A per canale: controllare", "la temperatura dei MOSFET"], fc="#fff4e0", ec=P5)
    wire(ax, [(12.4, 1.8), (13.2, 1.8)], SIG, 1.6, "B6 B7", (12.8, 1.8))
    block(ax, 17.3, 0.9, 2.7, 1.9, "2 riscaldatori 12 V 100 W", ["ognuno con fusibile 15 A", "e termostato di sicurezza", "55-60 gradi in serie"], fc="#ffffff")
    wire(ax, [(16.6, 1.8), (17.3, 1.8)], P12, 2.2)
    # barra 12 V
    wire(ax, [(1.65, 6.6), (1.65, 5.2), (18.6, 5.2)], P12, 2.8)
    ax.text(12.6, 5.3, "barra 12 V  -  fusibili: riscaldatori 2 x 15 A (in distinta); consigliati anche pompa 10 A, valvole e logica 5 A", fontsize=7.5, color=P12, ha="center", va="bottom")
    for x, y0, y1 in ((1.65, 5.2, 4.6), (14.9, 5.2, 6.5), (14.9, 5.2, 4.6), (10.9, 5.2, 4.4), (18.6, 5.2, 2.8)):
        wire(ax, [(x, y0), (x, y1)], P12, 2.0)
    ax.text(0.3, 0.3, "* EV INGRESSO su A5: nel firmware il riempimento del bagno apre oggi la valvola WB; serve una modifica minima (docs/note_firmware.md).   Masse (GND) tutte in comune.   "
                      "Cavi di potenza a 12 V (riscaldatori, pompa): almeno 1,5 mm2.", fontsize=8, color="#444444")
    for e in ("png", "pdf"):
        fig.savefig(os.path.join(DOCS, "schema_elettrico." + e), dpi=140, bbox_inches="tight")
    plt.close(fig)

if __name__ == "__main__":
    hydraulic(); electrical(); print("schemi scritti in docs/")
