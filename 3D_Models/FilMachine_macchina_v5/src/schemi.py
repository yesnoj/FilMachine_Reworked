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

def hydraulic():
    fig, ax = plt.subplots(figsize=(13.5, 8.6))
    ax.set_xlim(0, 16.4); ax.set_ylim(0, 10.4); ax.axis("off")
    ax.text(0.2, 10.05, "FilMachine - schema idraulico", fontsize=15, weight="bold")
    ax.text(0.2, 9.68, "Una valvola per recipiente su un collettore unico; pompa reversibile tra collettore e tank. Tubo 3/8\" (9,52 mm), raccordi a innesto rapido.", fontsize=9, color=GR)
    # bagno
    ax.add_patch(Rectangle((0.5, 5.4), 9.1, 2.9, fc="#e3f0fb", ec=W, lw=2))
    ax.text(9.5, 5.52, "BAGNO termostatico\ncirca 7,4 l d'acqua", fontsize=8.5, color=W, ha="right", weight="bold")
    xs = [1.5, 3.7, 5.9]
    for x, n, c in zip(xs, ("C1", "C2", "C3"), ("#f3d9a8", "#f0c9c9", "#d9e8c4")):
        ax.add_patch(Rectangle((x - 0.6, 5.95), 1.2, 2.65, fc=c, ec=CH, lw=1.6))
        ax.text(x - 0.25, 6.25, n, fontsize=13, ha="center", color=CH, weight="bold")
        ax.plot([x + 0.25, x + 0.25, x + 1.0, x + 1.0], [6.15, 9.0, 9.0, 5.05], color=CH, lw=2.2)        # testina (U rovesciata)
        ax.add_patch(Circle((x + 1.0, 4.95), 0.10, fc="white", ec=CH, lw=1.6))
        ax.plot([x + 1.0, x + 1.0], [4.85, 4.18], color=CH, lw=2.2)
        valve(ax, x + 1.0, 3.9, f"EV {n}", CH)
        ax.plot([x + 1.0, x + 1.0], [3.62, 2.6], color=CH, lw=2.2)
    ax.text(0.5, 9.22, "3 vaschette GN 1/9 (1 litro di chimica ciascuna) con testina pescante: si tolgono sollevandole", fontsize=8.5, ha="left", color=CH)
    ax.text(2.28, 4.95, "innesti a sifone\n(silicone 8x12)", fontsize=7.5, va="center", ha="right", color=GR)
    xw = 8.4
    ax.plot([xw, xw, 10.4, 10.4], [6.1, 9.0, 9.0, 4.18], color=W, lw=2.2)
    ax.text(xw - 0.12, 7.2, "pescante\nacqua (WB)", fontsize=8, ha="right", color=W)
    valve(ax, 10.4, 3.9, "EV WB", W)
    ax.plot([10.4, 10.4], [3.62, 2.6], color=W, lw=2.2)
    # collettore
    ax.plot([2.5, 13.2], [2.6, 2.6], color="black", lw=4)
    ax.text(2.55, 2.25, "COLLETTORE (C07)", fontsize=9, weight="bold")
    ax.text(4.55, 2.25, "sulla macchina le valvole stanno, da sinistra: C1, WB, C2, C3, scarico", fontsize=7.5, color=GR, va="baseline")
    # scarico
    ax.plot([11.9, 11.9], [2.6, 2.03], color=GR, lw=2.2)
    valve(ax, 11.9, 1.75, "EV SCARICO", GR, left=True)
    ax.plot([11.9, 11.9], [1.47, 0.75], color=GR, lw=2.2)
    ax.annotate("", xy=(11.9, 0.45), xytext=(11.9, 0.8), arrowprops=dict(arrowstyle="-|>", color=GR, lw=2))
    ax.text(11.7, 0.6, "SCARICO (pompato)", fontsize=9, ha="right", weight="bold", color=GR)
    # pompa
    ax.add_patch(Circle((13.62, 2.6), 0.42, fc="#fff2cc", ec="black", lw=2)); ax.text(13.62, 2.6, "P", fontsize=14, ha="center", va="center", weight="bold")
    ax.text(13.62, 1.95, "pompa a ingranaggi reversibile\nMarco UP3-R (PWM basso)", fontsize=8.2, ha="center", va="top")
    ax.annotate("", xy=(14.95, 3.75), xytext=(14.45, 3.75), arrowprops=dict(arrowstyle="-|>", color=RD, lw=1.6)); ax.text(14.38, 3.75, "IN = riempie la tank", fontsize=7.5, va="center", ha="right", color=RD)
    ax.annotate("", xy=(14.45, 3.42), xytext=(14.95, 3.42), arrowprops=dict(arrowstyle="-|>", color=BL, lw=1.6)); ax.text(14.38, 3.42, "OUT = svuota la tank", fontsize=7.5, va="center", ha="right", color=BL)
    # tank
    ax.add_patch(Rectangle((12.3, 5.6), 2.5, 2.2, fc="#eeeeee", ec="black", lw=2))
    ax.add_patch(Rectangle((12.32, 5.62), 2.46, 0.95, fc="#f6e7c8", ec="none"))
    ax.add_patch(Circle((13.3, 6.6), 0.62, fc="none", ec=GR, lw=1.2, ls="--"))
    ax.plot([14.04, 15.3, 15.3, 14.45, 14.45], [2.6, 2.6, 8.2, 8.2, 5.75], color="black", lw=3)
    ax.text(13.3, 8.5, "TANK con spirale e caricatore", fontsize=9, ha="center", weight="bold")
    ax.text(12.4, 5.72, "250 / 350 ml", fontsize=8)
    ax.text(15.42, 7.2, "pescante\ndall'alto:\nnessun foro\nsotto il livello", fontsize=7, va="center", color=GR)
    ax.plot([12.3, 11.3, 11.3], [7.1, 7.1, 4.75], color=GR, lw=2.0)
    ax.annotate("", xy=(11.3, 4.45), xytext=(11.3, 4.8), arrowprops=dict(arrowstyle="-|>", color=GR, lw=2))
    ax.text(11.42, 4.55, "TROPPOPIENO (a gravita')", fontsize=9, ha="left", weight="bold", color=GR)
    ax.text(12.4, 7.42, "soglia troppopieno: 500 ml\n(smaltisce circa 0,4 l/min)", fontsize=7, ha="left", va="bottom", color=GR)
    # rubinetto manuale di scarico del bagno (facoltativo)
    ax.plot([10.4, 10.4 - 0.0, 9.3], [4.55, 4.55, 4.55], color=W, lw=1.4, ls="--")
    ax.add_patch(Circle((9.15, 4.55), 0.14, fc="white", ec=W, lw=1.4)); ax.text(8.95, 4.55, "scarico manuale del bagno\n(rubinetto, facoltativo)", fontsize=7, va="center", ha="right", color=W)
    # note
    ax.text(0.5, 1.95, "Riempimento della tank: EV della sorgente aperta + pompa IN.\nRitorno al recipiente: stessa EV + pompa OUT.   Scarto: EV SCARICO + pompa OUT.\n"
                       "I risciacqui prendono l'acqua dal bagno (sorgente WB) e la mandano allo scarico.\n"
                       "Nel bagno (contenitore Euro 400x300x170): 2 riscaldatori 12 V 100 W, 2 sensori di livello, sonda DS18B20.", fontsize=9, va="top",
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
    ax.text(0.2, 12.2, "FilMachine - schema elettrico (collegamenti come nel firmware: boards/board_jc4880p433.h)", fontsize=14, weight="bold")
    P12, P5, SIG, I2C = "#b3261e", "#c77a12", "#2b6cb0", "#2e7d32"
    # colonna 1: alimentazione
    block(ax, 0.3, 9.6, 2.7, 1.9, "Rete 230 V", ["presa IEC con interruttore", "e fusibile; terra collegata", "(presa con differenziale)"], fc="#fdecea", ec=P12)
    block(ax, 0.3, 6.6, 2.7, 2.2, "Alimentatore 12 V 30 A", ["V+ / V-  ->  barra 12 V", "PE alla presa IEC"], fc="#fdecea", ec=P12)
    wire(ax, [(1.65, 9.6), (1.65, 8.8)], P12, 2.2, "L N PE", (2.25, 9.05))
    block(ax, 0.3, 2.6, 2.7, 2.0, "Convertitore LM2596S", ["12 V -> 5 V (regolare a 5,0 V", "prima di collegare)", "alimenta: scheda display,", "sensori di livello, servo"], fc="#fff4e0", ec=P5)
    # colonna 2: sensori
    block(ax, 3.6, 10.0, 3.0, 1.5, "2 sonde DS18B20", ["bagno + chimica (vaschetta C1)", "dati su GPIO35, pull-up 4,7 k a 3,3 V"], fc="#ffffff")
    block(ax, 3.6, 8.4, 3.0, 1.2, "Sensore Hall KY-003", ["sulla ruota magneti  ->  GPIO31"], fc="#ffffff")
    block(ax, 3.6, 6.6, 3.0, 1.4, "2 sensori XKC-Y21 (5 V)", ["livello MIN -> GPIO29", "livello MAX -> GPIO30"], fc="#ffffff")
    block(ax, 3.6, 2.6, 3.0, 2.0, "Servo taglierina MG90S *", ["5 V dal convertitore", "segnale: GPIO28 (da aggiungere", "nel firmware) oppure un", "tester per servo"], fc="#fff4e0", ec=P5)
    wire(ax, [(3.0, 3.6), (3.6, 3.6)], P5, 1.8, "5 V", (3.3, 3.6))
    # colonna 3: scheda, breakout, driver
    block(ax, 7.2, 7.1, 5.2, 4.4, "Scheda display Guition JC4880P443C (ESP32-P4)", [
        "JP1  9  GPIO51  PUMP_ENA (PWM)", "JP1 11  GPIO50  PUMP_IN2", "JP1 13  GPIO49  PUMP_IN1",
        "JP1 19  GPIO32  MOTOR_ENA (PWM)", "JP1  8  GPIO33  MOTOR_IN1", "JP1 17  GPIO34  MOTOR_IN2",
        "JP1 23 / 25  GPIO7 / GPIO8  I2C SDA / SCL", "JP1 15  GPIO35  OneWire DS18B20", "JP1 10  GPIO31  Hall KY-003",
        "JP1 14  GPIO29  livello MIN bagno", "JP1 12  GPIO30  livello MAX bagno", "JP1  7  GPIO52  flussimetro (non usato)",
        "JP1 21  GPIO28  libero  ->  servo taglierina *"], fc="#eaf2fb", ec=SIG)
    block(ax, 7.2, 5.9, 5.2, 0.85, "Breakout DC3-26P + cavo flat 2x13 (JP1 ai morsetti)", [], fc="#eaf2fb", ec=SIG)
    wire(ax, [(9.8, 7.1), (9.8, 6.75)], SIG, 2.0)
    for y in (10.75, 9.0, 7.3):
        wire(ax, [(6.6, y), (7.2, y)], SIG)
    block(ax, 7.2, 0.9, 5.2, 3.5, "Driver Adafruit #6318 (MCP23017, I2C 0x20)", [
        "A0  EV C1      A1  EV C2      A2  EV C3", "A3  EV WB     A4  EV SCARICO", "A5, A6 liberi;  A7 non usato",
        "B0..B5  livelli delle vaschette: NON collegati", "B6  riscaldatore 1      B7  riscaldatore 2", "alimentazione: 3,3 V (logica) + 12 V (carichi)"], fc="#f1f8e9", ec=I2C)
    wire(ax, [(8.2, 5.9), (8.2, 4.4)], I2C, 1.8, "I2C + 3,3 V", (7.75, 4.55))
    # colonna 4 e 5: potenza
    block(ax, 13.2, 9.9, 2.6, 1.6, "Altoparlante 8 ohm 2 W", ["sul connettore SPK", "della scheda"], fc="#eaf2fb", ec=SIG)
    wire(ax, [(12.4, 10.7), (13.2, 10.7)], SIG)
    block(ax, 13.2, 6.5, 3.4, 2.7, "Ponte H doppio 30 A (DBH-12V)", ["canale A  <-  GPIO33 / 34 / 32", "canale B  <-  GPIO49 / 50 / 51", "(IN1 / IN2 / ENA)", "alimentazione 12 V dalla barra"], fc="#f1f8e9", ec=I2C)
    wire(ax, [(12.4, 8.0), (13.2, 8.0)], SIG, 1.6, "6 fili", (12.8, 8.0))
    block(ax, 17.3, 8.2, 2.7, 1.0, "Motoriduttore JGB37-520", ["12 V, 20 giri/min: agitazione"], fc="#ffffff")
    block(ax, 17.3, 6.5, 2.7, 1.0, "Pompa Marco UP3-R", ["12 V 6 A, reversibile"], fc="#ffffff")
    wire(ax, [(16.6, 8.7), (17.3, 8.7)], P12, 2.0, "A", (16.95, 8.7)); wire(ax, [(16.6, 7.0), (17.3, 7.0)], P12, 2.0, "B", (16.95, 7.0))
    block(ax, 13.2, 3.3, 3.4, 1.3, "5 elettrovalvole 12 V NC", ["C1, C2, C3, WB, SCARICO", "comune al +12 V"], fc="#ffffff")
    wire(ax, [(12.4, 3.9), (13.2, 3.9)], P12, 1.8, "A0..A4", (12.8, 3.9))
    block(ax, 13.2, 0.9, 3.4, 1.9, "2 moduli MOSFET 15 A", ["ingressi da B6 e B7", "(il modulo IRF540 in distinta", "scalda troppo a 8,3 A)"], fc="#fff4e0", ec=P5)
    wire(ax, [(12.4, 1.8), (13.2, 1.8)], SIG, 1.6, "B6 B7", (12.8, 1.8))
    block(ax, 17.3, 0.9, 2.7, 1.9, "2 riscaldatori 12 V 100 W", ["ognuno con fusibile 15 A", "e termostato di sicurezza", "55-60 gradi in serie"], fc="#ffffff")
    wire(ax, [(16.6, 1.8), (17.3, 1.8)], P12, 2.2)
    # barra 12 V
    wire(ax, [(1.65, 6.6), (1.65, 5.2), (18.6, 5.2)], P12, 2.8)
    ax.text(10.0, 5.3, "barra 12 V  -  fusibili: riscaldatori 2 x 15 A (in distinta); consigliati anche pompa 10 A, valvole e logica 5 A", fontsize=7.5, color=P12, ha="center", va="bottom")
    for x, y0, y1 in ((1.65, 5.2, 4.6), (14.9, 5.2, 6.5), (14.9, 5.2, 4.6), (10.9, 5.2, 4.4), (18.6, 5.2, 2.8)):
        wire(ax, [(x, y0), (x, y1)], P12, 2.0)
    ax.text(0.3, 0.3, "* Taglierina e ciclo di caricamento non esistono ancora nel firmware: vedi docs/note_firmware.md.   Masse (GND) tutte in comune.   "
                      "Cavi di potenza a 12 V (riscaldatori, pompa): almeno 1,5 mm2.", fontsize=8, color="#444444")
    for e in ("png", "pdf"):
        fig.savefig(os.path.join(DOCS, "schema_elettrico." + e), dpi=140, bbox_inches="tight")
    plt.close(fig)

if __name__ == "__main__":
    hydraulic(); electrical(); print("schemi scritti in docs/")
