"""Nuova distinta (docs/NuovaDistinta.xlsx): cose da acquistare, componenti gia' acquistati e loro uso, pezzi stampati, viteria."""
import os, json
from openpyxl import Workbook
from openpyxl.styles import Font, Alignment, PatternFill, Border, Side
from openpyxl.utils import get_column_letter
from common import OUT

F = "Arial"
HDR = PatternFill("solid", fgColor="1F3A5F"); GRP = PatternFill("solid", fgColor="E8EEF5"); TOT = PatternFill("solid", fgColor="FFF2CC")
thin = Side(style="thin", color="BBBBBB"); BOX = Border(left=thin, right=thin, top=thin, bottom=thin)
EUR = '#,##0.00 "€"'

def header(ws, row, cols, widths):
    for j, (c, w) in enumerate(zip(cols, widths), 1):
        cell = ws.cell(row=row, column=j, value=c)
        cell.font = Font(name=F, bold=True, color="FFFFFF", size=10); cell.fill = HDR; cell.border = BOX
        cell.alignment = Alignment(horizontal="center", vertical="center", wrap_text=True)
        ws.column_dimensions[get_column_letter(j)].width = w
    ws.row_dimensions[row].height = 30

def put(ws, row, col, value, bold=False, color="000000", fmt=None, wrap=True, fill=None, align=None, size=10):
    c = ws.cell(row=row, column=col, value=value)
    c.font = Font(name=F, bold=bold, color=color, size=size); c.border = BOX
    c.alignment = Alignment(vertical="top", wrap_text=wrap, horizontal=align)
    if fmt: c.number_format = fmt
    if fill: c.fill = fill
    return c

BUY = [  # gruppo, componente, specifiche, quantita', unita', prezzo unitario stimato, dove si usa, note
 ("Bagno", "Lastre di PMMA (plexiglass) trasparente da 4 mm tagliate a misura", "5 pannelli (fondo, fronte, retro, destra, sinistra con la finestra): misure e fori nei DXF di docs/pannelli", 1, "serie", 45.0, "vasca del bagno, incollata con collante per acrilico (Acrifix o diclorometano)", "la parete sinistra e' piu' bassa (bordo sotto la gonna del coperchio della tank) e ha la finestra della testata"),
 ("Bagno", "Lastra di PMMA trasparente da 3 mm tagliata a misura", "coperchio con le aperture dei tappi e della tank (DXF)", 1, "pz", 15.0, "coperchio del bagno", "bordi smussati"),
 ("Bagno", "Contenitore in PP rettangolare da 500 ml con tappo a vite", "circa 70 x 95 x 115 mm (lato stretto sul fronte); un foro Ø17 al centro del fondo", 3, "pz", 3.0, "chimiche C1, C2, C3, immersi nel bagno", "misure di riferimento: se il contenitore scelto e' diverso cambiare CW, CD, CH in machine.py e rigenerare"),
 ("Bagno", "Passaparete 3/8\" BSP in PP con dado, guarnizione e raccordo a innesto 3/8\" sul filetto", "filetto lungo almeno 25 mm (fondo del contenitore 1,5 + fondo del bagno 4 + piano 12)", 7, "pz", 3.0, "3 sotto i contenitori, WB, troppopieno del bagno, ingresso, linea tank (quest'ultimo con portagomma 8 mm sopra)", "uno di scorta; guarnizione piana sopra e sotto il fondo del bagno"),
 ("Bagno", "Gommino passatubo in gomma per lamiera da 4 mm, foro Ø13, tubo Ø12", "", 2, "pz", 0.5, "troppopieno della tank nel fondo del bagno in PMMA (nel piano sotto il foro e' Ø20: il gommino fa tenuta solo sul PMMA)", "uno di scorta"),
 ("Bagno", "Guarnizione piana in EPDM 2 mm, telaio della finestra della testata", "ritagliata da un foglio 200 x 120 (DXF REF_guarnizione_finestra)", 1, "pz", 4.0, "tra la testata della tank e la parete sinistra del bagno", ""),
 ("Bagno", "Guarnizione piana in EPDM/silicone 1 mm, coperchietto del vano servo", "ritagliata da foglio", 1, "pz", 1.0, "sotto il coperchietto B03 (il vano servo e' sott'acqua)", ""),
 ("Bagno", "Dado basso inox + 2 guarnizioni piane in silicone, per il filetto delle cartucce", "una serie per riscaldatore; passo del filetto da leggere sulla cartuccia (foro Ø16,6 nella parete destra, da adattare)", 2, "serie", 3.0, "passaggio dei riscaldatori nella parete destra del bagno", "i riscaldatori non devono mai lavorare a secco: il firmware li accende solo con il MIN del bagno bagnato"),
 ("Bagno", "Termostato bimetallico KSD301 normalmente chiuso, 60 °C, 10 A", "", 2, "pz", 1.5, "sicurezza, in serie a ogni riscaldatore", ""),
 ("Bagno", "Pompetta di ricircolo sommergibile 12 V DC (2-3 W, 100-200 l/h) con ventose", "circa 30 x 24 x 42 mm", 1, "pz", 6.0, "ricircolo dell'acqua del bagno (uscita SPARE OUT 2 della Mainboard, A7)", "bocca verso i riscaldatori"),
 ("Bagno", "Nastro biadesivo per esterni (tipo VHB) 19 mm", "rotolo corto", 1, "pz", 3.0, "XKC-Y21 sui contenitori (MAX) e sulla parete posteriore del bagno (MIN/MAX)", ""),
 ("Idraulica", "Tubo LLDPE 3/8\" (9,52 x 6,35 mm) per osmosi inversa", "rotolo da 5 m (ne serve circa 1 m dentro la macchina)", 1, "pz", 6.0, "linee WB, scarico, troppopieno, ingresso, spezzoni delle valvole e delle camere", ""),
 ("Idraulica", "Elettrovalvola 12 V normalmente chiusa, 3/8\" a innesto", "uguale alle cinque gia' acquistate", 2, "pz", 7.59, "sesta valvola del collettore (RECUPERO, A6) e valvola dell'ingresso (A5)", "con la freccia verso il collettore (RECUPERO) e verso il bagno (ingresso)"),
 ("Idraulica", "Gomito a innesto rapido 3/8\" - 3/8\"", "per tubo da 9,52 mm", 14, "pz", 0.7, "5 davanti alle valvole, 1 per WB, 1 per lo scarico, 1 per il troppopieno, 4 sull'ingresso", "2 di scorta"),
 ("Idraulica", "Gomito a codolo 3/8\" (un lato a innesto, l'altro a codolo liscio da 9,52 mm)", "\"stem elbow\"", 2, "pz", 1.5, "uscita del collettore C01 verso la pompa", "uno di scorta; codolo lungo almeno 22 mm dall'angolo"),
 ("Idraulica", "Adattatore innesto 3/8\" (codolo liscio da 9,52) - portagomma 10 mm", "", 5, "pz", 1.5, "bocca laterale delle tre camere di livello, bocca della valvola RECUPERO", "uno di scorta"),
 ("Idraulica", "Adattatore innesto 3/8\" - portagomma 6,4 mm", "", 3, "pz", 1.5, "gomito del collettore e raccordo sotto il passaparete della tank: da qui parte il silicone della Kamoer", "uno di scorta"),
 ("Idraulica", "Raccordo diritto: filetto FEMMINA 1/2\" - tubo 3/8\" a innesto", "", 2, "pz", 1.5, "attacchi del flussimetro YF-S201", "con nastro in PTFE"),
 ("Idraulica", "Passaparete a innesto rapido 3/8\" - 3/8\"", "foro Ø16", 4, "pz", 2.0, "attacchi sul pannello posteriore: SCARICO, TROPPOPIENO tank, TROPPOPIENO bagno, INGRESSO", ""),
 ("Idraulica", "Passaparete portagomma 10 - portagomma 10", "foro Ø13", 1, "pz", 2.0, "uscita RECUPERO sul pannello frontale", "con un gomito o un tubo verso il basso: la bottiglia sta sotto"),
 ("Idraulica", "Rubinetto a margherita in PP con portagomma 10 mm da entrambi i lati", "da pannello, foro Ø21", 3, "pz", 3.0, "svuotamento manuale di C1, C2, C3 (sotto ogni contenitore)", "normalmente chiusi"),
 ("Idraulica", "Adattatore per rubinetto con valvolina, uscita tubo 3/8\" (kit per osmosi inversa)", "filetto 1/2\" o 3/4\" secondo il rubinetto", 1, "pz", 5.0, "collegamento dell'INGRESSO al rubinetto dell'acqua", "pressione di rete massima 0,8 MPa"),
 ("Idraulica", "Tubo in silicone 8 x 12 mm, NERO", "2 m", 1, "pz", 7.0, "linea della tank, troppopieno della tank, rubinetti, recupero", "nero: il tubo della tank non deve far passare la luce"),
 ("Idraulica", "Tubo in silicone 6 x 10 mm (o PharMed 6,4 x 9,6)", "1 m", 1, "pz", 5.0, "Kamoer: dal collettore alla bocca A, dalla bocca B al passaparete della tank", ""),
 ("Idraulica", "O-ring 9 x 2 in EPDM", "", 20, "pz", 0.15, "7 sedi del collettore C01, 9 delle camere C03", "4 di scorta"),
 ("Idraulica", "Fascette in nylon 2,5 x 100 mm", "confezione da 100", 1, "conf", 3.0, "valvole nelle culle, sensori, cavi", ""),
 ("Idraulica", "Fascette in nylon 4,8 x 200 mm", "confezione da 50", 1, "conf", 3.0, "flussimetro, valvola dell'ingresso, Kamoer nelle culle", ""),
 ("Idraulica", "Grasso al silicone per O-ring", "tubetto", 1, "pz", 5.0, "O-ring, codoli degli innesti, baionetta della spirale", ""),
 ("Tank e trasmissione", "Perno in acciaio inox Ø6 x 22 mm", "barra rettificata h6", 1, "pz", 2.0, "perno del mozzo magnetico, incollato (epossidica) nel boss della testata", ""),
 ("Tank e trasmissione", "Rondella di spinta in PTFE Ø24 x Ø13 x 1", "", 2, "pz", 0.5, "tra la flangia del mozzo e la testata", "una di scorta"),
 ("Tank e trasmissione", "Magneti al neodimio N52 Ø8 x 3", "confezione da 20 (ne servono 12)", 1, "conf", 6.0, "6 nel mozzo B16, 6 nel disco B18 (r = 14): poli ALTERNATI, il disco speculare al mozzo", "coppia stimata 0,12 N m; se slitta avvolgendo, Ø10 x 3"),
 ("Tank e trasmissione", "Magneti al neodimio Ø5 x 2", "confezione da 10 (ne servono 4)", 1, "conf", 3.0, "indicatori sul mozzo (r = 23,5) per il sensore Hall", "tutti col polo SUD verso la testata (l'A3144 commuta col sud)"),
 ("Tank e trasmissione", "Resina epossidica bicomponente", "tubetto", 1, "pz", 5.0, "incollaggio dei magneti e dei tappi B17/B19 (incapsulamento), del perno", "la chimica non deve raggiungere i magneti"),
 ("Tank e trasmissione", "Molla a compressione Ø6 x 22 mm, filo 0,5", "", 1, "pz", 1.0, "perno folle a molla", ""),
 ("Tank e trasmissione", "Vite M3 x 10 a testa zigrinata (vite a mano)", "", 2, "pz", 0.5, "blocco delle due slitte della guida B14/B15", ""),
 ("Tank e trasmissione", "Fascetta della Lab-Box, gia' fatta", "ricambio Ars-Imago", 1, "pz", None, "passa nel tubo A03 tra i due collarini", "prezzo da verificare"),
 ("Tank e trasmissione", "Micro-servo MG90S", "con squadretta a un braccio", 1, "pz", 4.0, "taglierina", ""),
 ("Tank e trasmissione", "Lame di ricambio per cutter da 9 mm", "confezione da 10", 1, "conf", 2.0, "due spezzoni da 36 mm nella taglierina", ""),
 ("Tank e trasmissione", "Passacavo in gomma per foro Ø6,4, cavo Ø3", "", 2, "pz", 0.3, "cavo del servo attraverso la testata (nel vano asciutto)", "sigillare con silicone: tenuta alla luce"),
 ("Elettrico", "Presa IEC C14 da pannello con interruttore e portafusibile", "foro 27,5 x 47,5 mm", 1, "pz", 4.0, "pannello posteriore, a destra", ""),
 ("Elettrico", "Cavo di alimentazione con spina e connettore IEC C13", "", 1, "pz", 4.0, "", ""),
 ("Elettrico", "Cavo unipolare 1,5 mm2 rosso e nero", "5 m + 5 m", 1, "serie", 8.0, "12 V dall'alimentatore ai tre ingressi della Mainboard, riscaldatori", ""),
 ("Elettrico", "Cavetti per segnali, puntalini, connettori JST-XH", "assortimento", 1, "serie", 5.0, "sensori, valvole, servo, Hall, pompetta", ""),
 ("Elettrico", "Passacavo in gomma Ø22", "", 3, "pz", 0.5, "piano: vano motore, intercapedine destra (riscaldatori), cavi del bagno", ""),
 ("Struttura e viteria", "Multistrato 526 x 232 x 12 mm", "2 pezzi (base e piano); il piano con i fori della dima e gli angoli tagliati", 2, "pz", 8.0, "base della macchina e piano del piedistallo (il bagno ci appoggia sopra)", "impregnare o verniciare: il piano sta sotto il bagno"),
 ("Struttura e viteria", "Pannelli in Dibond (composito di alluminio) 3 mm tagliati a misura", "5 pezzi (frontale a U, posteriore, sinistro, destro, copertura): misure e fori nel foglio Pannelli e nei DXF", 1, "serie", 45.0, "guscio", ""),
 ("Struttura e viteria", "Piedini in gomma Ø20", "", 4, "pz", 0.5, "sotto la base", ""),
 ("Struttura e viteria", "Viti da legno 3,5 x 16 a testa svasata NERE", "confezione da 20 (ne servono 12)", 1, "conf", 3.0, "pannelli del guscio nel bordo della base", ""),
 ("Struttura e viteria", "Viti da legno 3,5 x 12 a testa cilindrica", "confezione da 50 (ne servono circa 30)", 1, "conf", 3.0, "tutti i pezzi fissati alla base (dima) e al piano", ""),
 ("Struttura e viteria", "Viti autofilettanti per plastica 3 x 8, 3 x 10, 3 x 12 e 3 x 16", "una confezione piccola per misura", 4, "conf", 3.0, "vedi foglio Viteria", ""),
 ("Struttura e viteria", "Viti M3 x 12 a testa cilindrica", "8 pezzi + scorta", 1, "conf", 2.0, "cornice B22: attraverso parete del bagno e guarnizione, nella testata della tank", ""),
 ("Struttura e viteria", "Viti M3 x 16 con dado e rondella larga", "6 pezzi", 1, "conf", 2.0, "piedini della tank attraverso il fondo del bagno e il piano (rondella in gomma sopra il fondo)", ""),
 ("Struttura e viteria", "Minuteria della tank: 1 grano M3 x 6, 4 viti autofilettanti M2 x 6, 1 vite M2,5 x 8, 2 viti M3 x 8, 2 viti M3 x 10, 6 viti M3 x 8 (riduttore)", "assortimento", 1, "serie", 4.0, "disco motore, morsetti lama, manovella, motoriduttore, perno folle, staffa Hall", ""),
 ("Struttura e viteria", "Filamento PETG, bobina da 1 kg", "colore a scelta (nero per la tank)", 2, "pz", 20.0, "tutti i pezzi stampati", "stima: vedi foglio Pezzi stampati"),
 ("Struttura e viteria", "Filamento PETG TRASPARENTE, bobina piccola", "", 1, "pz", 12.0, "camere di livello C03", "circa 60 g"),
 ("Struttura e viteria", "Collante per PMMA (Acrifix 192 o diclorometano) + silicone neutro", "", 1, "serie", 12.0, "incollaggio della vasca, sigillatura dei passaparete e delle selle dei riscaldatori", ""),
]

OWNED = [  # componente, specifiche, quantita', prezzo, uso in questa macchina
 ("Alimentatore", "Mean Well LRS-350-12", 1, 32.0, "Usato: in piedi sul fianco sinistro del piedistallo, tra i fermi C09, morsetti verso il retro. Tre coppie +V/-V verso J1, J2, J3 della Mainboard"),
 ("Display", "JC4880P433", 1, 25.59, "Usato: in verticale dietro la finestra del pannello frontale, nella cornice C12, trattenuto dalle quattro linguette D10"),
 ("Pompa", "Kamoer KPHM600 12 V", 1, 60.0, "Usata: nella culla C08 sulla base, testa verso il fronte. Bocca A dal collettore, bocca B al passaparete della tank"),
 ("Mainboard", "FilMachine Mainboard v1.1 (JLCPCB)", 1, 0.0, "Usata: in piedi dietro il display sui due montanti C10 (quattro viti M3 x 8 nei fori di fissaggio della scheda, a 4 mm dai bordi), componenti verso il retro"),
 ("Elettrovalvola", "DC 12V Normally Close", 5, 7.59, "Usate tutte, sdraiate sulla culla C02, bocca posteriore nel collettore C01. Da sinistra: WB, C1, SCARICO, C2, RECUPERO (nuova), C3. Freccia verso il collettore"),
 ("Motore agitatore", "JGB37-520 12V 20RPM", 1, 8.34, "Usato: nel vano asciutto, coassiale con la spirale, sulla staffa B20; trascina attraverso la testata col giunto magnetico"),
 ("Riscaldatore", "100W 12V Ø16 x 100", 2, 10.85, "Usati: infilati dalla parete destra del bagno (teste nell'intercapedine destra), paralleli al fondo dietro i contenitori, estremita' sulle selle C06"),
 ("Sensore hall", "KY-003 A3144", 1, 4.83, "Usato: sulla staffa B21 a sinistra del disco, col sensore piegato di piatto verso la testata (dentro l'incavo), legge i 4 magneti indicatori del mozzo"),
 ("Sensore flusso", "YF-S201 12V", 1, 5.94, "Usato: sulla linea dell'ingresso, nella culla C04 dietro la mainboard"),
 ("Sensore livello", "XKC-Y21 5V", 8, 5.53, "Usati tutti: 2 sulla parete posteriore del bagno (MIN e MAX, fuori), 3 sulle camere di livello C03 (MIN delle chimiche), 3 sulla parete posteriore dei contenitori sopra l'acqua (MAX)"),
 ("Sensore temperatura", "DS18B20 30mm 1m", 2, 3.31, "Usato uno nel bagno (portasonda C05 sulla parete posteriore). Il secondo (chimica) non ha piu' posto: i contenitori sono chiusi; tenerlo di scorta"),
 ("Cavo Flat", "Cavo flat grigio 2x13 20cm", 1, 6.75, "Usato: dal display alla Mainboard, che sta subito dietro"),
]

SCREWS = [  # tipo, quantita', dove
 ("Vite autofilettante 3 x 10", 4, "blocchi ponte sul ripiano della tank (B04, B05)"), ("Vite autofilettante 3 x 16", 4, "tetto del ponte (B06)"),
 ("Vite autofilettante 3 x 10", 4, "culla universale (B12)"), ("Vite M2 x 6 autofilettante", 4, "morsetti delle lame"),
 ("Vite M2 x 8 (del servo)", 2, "alette del servo"), ("Vite M2,5 x 8", 1, "perno di manovella"), ("Vite M3 x 10", 1, "spinotto biella-portalama"),
 ("Vite autofilettante 3 x 8", 4, "coperchietto del vano servo (B03) con la guarnizione"),
 ("Vite M3 x 12", 8, "cornice B22 attraverso la parete del bagno e la guarnizione, nella testata della tank"),
 ("Vite M3 x 16 + dado + rondella", 5, "piedini della tank attraverso il fondo del bagno e il piano"),
 ("Vite M3 x 8", 6, "motoriduttore sulla staffa B20 (sei fori del riduttore)"), ("Vite M3 x 8 + dado", 2, "piedi della staffa B20 sul fondo del vano (asole)"),
 ("Grano M3 x 6", 1, "disco motore B18 sulla spianatura dell'albero"), ("Vite autofilettante 3 x 12", 2, "staffa Hall B21, dal retro della piastra del motore B20"),
 ("Vite M3 x 10", 1, "ritegno del perno folle"),
 ("Vite M3 x 10 a testa zigrinata", 2, "blocco delle slitte B14 e B15"),
 ("Vite da legno 3,5 x 12", 3, "collettore C01"), ("Vite da legno 3,5 x 12", 4, "culla delle valvole C02"), ("Vite da legno 3,5 x 12", 2, "culla del flussimetro C04"),
 ("Vite da legno 3,5 x 12", 2, "culla della valvola dell'ingresso C07"), ("Vite da legno 3,5 x 12", 3, "culla della Kamoer C08"), ("Vite da legno 3,5 x 12", 4, "fermi dell'alimentatore C09"),
 ("Vite da legno 3,5 x 12", 2, "piedi dei montanti della mainboard C10 (nella base)"), ("Vite da legno 3,5 x 12", 2, "linguette dei montanti C10 (nel piano, da sotto)"), ("Vite M3 x 8", 4, "mainboard sui boss dei montanti C10"),
 ("Vite da legno 3,5 x 12", 4, "colonne del piano C13 (nella base)"), ("Vite da legno 3,5 x 12", 2, "colonne del piano C13 (nel piano, da sopra)"),
 ("Vite da legno 3,5 x 16 svasata nera", 12, "pannelli del guscio nel bordo della base (3 per pannello)"),
 ("Vite autofilettante 3 x 8", 4, "linguette D10 del display sulla cornice C12"),
 ("Vite da legno 3,5 x 12", 8, "piano del piedistallo sui montanti e sulle colonne (dall'alto)"),
]

def build():
    wb = Workbook()
    # ------------------------------------------------------------ Da acquistare
    ws = wb.active; ws.title = "Da acquistare"
    ws["A1"] = "FilMachine rilascio 7 - distinta: componenti da acquistare"; ws["A1"].font = Font(name=F, bold=True, size=14)
    ws["A2"] = ("Prezzi unitari in blu: stime indicative (ottobre 2026), da verificare all'acquisto. Elettrovalvola: prezzo della distinta originale. "
                "I contenitori da 500 ml e i passaparete 3/8\" vanno scelti prima di tagliare il piano e il fondo del bagno (fori Ø17).")
    ws["A2"].font = Font(name=F, size=9, italic=True); ws["A2"].alignment = Alignment(wrap_text=True, vertical="top"); ws.merge_cells("A2:J2"); ws.row_dimensions[2].height = 40
    cols = ["N.", "Gruppo", "Componente", "Specifiche", "Q.tà", "Unità", "Prezzo unitario indicativo (€)", "Totale (€)", "Dove si usa", "Note"]
    header(ws, 4, cols, [5, 18, 40, 38, 7, 7, 13, 12, 38, 40])
    r0 = 5
    for i, (g, c, s, q, u, p, w, n) in enumerate(BUY):
        r = r0 + i
        put(ws, r, 1, i + 1, align="center"); put(ws, r, 2, g); put(ws, r, 3, c, bold=True); put(ws, r, 4, s)
        put(ws, r, 5, q, color="0000FF", align="center"); put(ws, r, 6, u, align="center"); put(ws, r, 7, p, color="0000FF", fmt=EUR)
        put(ws, r, 8, f"=E{r}*G{r}", fmt=EUR); put(ws, r, 9, w); put(ws, r, 10, n)
    r1 = r0 + len(BUY) - 1; rt = r1 + 1
    put(ws, rt, 3, "TOTALE da acquistare", bold=True, fill=TOT); put(ws, rt, 8, f"=SUM(H{r0}:H{r1})", bold=True, fmt=EUR, fill=TOT)
    for j in (1, 2, 4, 5, 6, 7, 9, 10): put(ws, rt, j, None, fill=TOT)
    rs = rt + 2
    put(ws, rs, 3, "Totale per gruppo", bold=True, fill=GRP); put(ws, rs, 8, None, fill=GRP)
    groups = []
    for g, *_ in BUY:
        if g not in groups: groups.append(g)
    for k, g in enumerate(groups, 1):
        put(ws, rs + k, 3, g); put(ws, rs + k, 8, f'=SUMIF($B${r0}:$B${r1},C{rs + k},$H${r0}:$H${r1})', fmt=EUR)
    rf = rs + len(groups) + 1
    put(ws, rf, 3, "di cui facoltativi (velluto, tester per servo)", bold=False)
    opt = [r0 + i for i, b in enumerate(BUY) if "facoltativo" in b[7]]
    put(ws, rf, 8, "=" + "+".join(f"H{r}" for r in opt), fmt=EUR)
    ws.freeze_panes = "D5"
    # ------------------------------------------------------------ Gia' acquistati
    ws2 = wb.create_sheet("Già acquistati")
    ws2["A1"] = "Componenti già acquistati e loro uso nel rilascio 7"; ws2["A1"].font = Font(name=F, bold=True, size=14)
    ws2["A2"] = "Quantità e prezzi: distinta originale del progetto dove nota; alimentatore, Kamoer e riscaldatori ai prezzi indicativi."; ws2["A2"].font = Font(name=F, size=9, italic=True)
    header(ws2, 4, ["Componente", "Specifiche", "Q.tà", "Prezzo unitario (€)", "Totale (€)", "Uso in questa macchina"], [26, 36, 7, 13, 12, 90])
    for i, (c, s, q, p, u) in enumerate(OWNED):
        r = 5 + i
        put(ws2, r, 1, c, bold=True); put(ws2, r, 2, s); put(ws2, r, 3, q, align="center"); put(ws2, r, 4, p, fmt=EUR); put(ws2, r, 5, f"=C{r}*D{r}", fmt=EUR)
        put(ws2, r, 6, u, color="B3261E" if u.startswith(("NON", "Non")) else "000000")
    rt2 = 5 + len(OWNED)
    put(ws2, rt2, 1, "TOTALE già speso", bold=True, fill=TOT); put(ws2, rt2, 5, f"=SUM(E5:E{rt2 - 1})", bold=True, fmt=EUR, fill=TOT)
    for j in (2, 3, 4, 6): put(ws2, rt2, j, None, fill=TOT)
    ws2.freeze_panes = "A5"
    # ------------------------------------------------------------ Pezzi stampati
    ws3 = wb.create_sheet("Pezzi stampati")
    parts = json.load(open(os.path.join(OUT, "docs", "_pezzi.json")))
    ws3["A1"] = "Pezzi da stampare (file in stl/ e step/, già nell'orientamento di stampa)"; ws3["A1"].font = Font(name=F, bold=True, size=14)
    ws3["A2"] = "Ipotesi per la stima del filamento (celle in blu, modificabili):"; ws3["A2"].font = Font(name=F, size=10, italic=True)
    ws3["A3"] = "Densità PETG (g/cm3)"; ws3["B3"] = 1.27; ws3["C3"] = "Quota di materiale rispetto al pieno (pareti + riempimento 25 %)"; ws3["D3"] = 0.5
    for a in ("A3", "C3"): ws3[a].font = Font(name=F, size=10)
    for a in ("B3", "D3"): ws3[a].font = Font(name=F, size=10, color="0000FF")
    header(ws3, 5, ["Pezzo", "Q.tà", "Volume pieno (cm3)", "Volume totale (cm3)", "Ingombro X (mm)", "Ingombro Y (mm)", "Altezza di stampa Z (mm)", "Note di stampa"], [36, 7, 12, 12, 12, 12, 12, 60])
    for i, p in enumerate(parts):
        r = 6 + i
        put(ws3, r, 1, p["nome"], bold=True); put(ws3, r, 2, p["q"], align="center"); put(ws3, r, 3, p["volume_cm3"], fmt="0.0")
        put(ws3, r, 4, f"=B{r}*C{r}", fmt="0.0")
        for j, v in enumerate(p["ingombro"]): put(ws3, r, 5 + j, v, fmt="0.0")
        put(ws3, r, 8, p["note"])
    rt3 = 6 + len(parts)
    put(ws3, rt3, 1, "TOTALE", bold=True, fill=TOT); put(ws3, rt3, 2, f"=SUM(B6:B{rt3 - 1})", bold=True, fill=TOT, align="center")
    put(ws3, rt3, 4, f"=SUM(D6:D{rt3 - 1})", bold=True, fmt="0", fill=TOT)
    for j in (3, 5, 6, 7, 8): put(ws3, rt3, j, None, fill=TOT)
    put(ws3, rt3 + 1, 1, "Filamento stimato (kg)", bold=True); put(ws3, rt3 + 1, 4, f"=D{rt3}*B3*D3/1000", bold=True, fmt="0.0")
    put(ws3, rt3 + 2, 1, "Ingombro massimo sul piatto (mm)"); put(ws3, rt3 + 2, 5, f"=MAX(E6:E{rt3 - 1})", fmt="0.0"); put(ws3, rt3 + 2, 6, f"=MAX(F6:F{rt3 - 1})", fmt="0.0"); put(ws3, rt3 + 2, 7, f"=MAX(G6:G{rt3 - 1})", fmt="0.0")
    ws3.freeze_panes = "B6"
    # ------------------------------------------------------------ Viteria
    ws4 = wb.create_sheet("Viteria")
    ws4["A1"] = "Viteria: dove va ogni vite"; ws4["A1"].font = Font(name=F, bold=True, size=14)
    ws4["A2"] = "Le viti autofilettanti da 3 mm entrano nei fori Ø2,6 dei pezzi stampati; le lunghezze sono verificate sul modello (verifiche.txt, check_screws.py). Le quantità per tipo sono sommate a destra."; ws4["A2"].font = Font(name=F, size=9, italic=True)
    header(ws4, 4, ["Tipo", "Q.tà", "Dove"], [38, 7, 60])
    for i, (t, q, w) in enumerate(SCREWS):
        r = 5 + i; put(ws4, r, 1, t, bold=True); put(ws4, r, 2, q, align="center"); put(ws4, r, 3, w)
    re_ = 5 + len(SCREWS) - 1
    types = []
    for t, *_ in SCREWS:
        if t not in types: types.append(t)
    ws4.column_dimensions["E"].width = 38; ws4.column_dimensions["F"].width = 10
    for j, c in ((5, "Tipo"), (6, "Totale")):
        cell = ws4.cell(row=4, column=j, value=c); cell.font = Font(name=F, bold=True, color="FFFFFF", size=10); cell.fill = HDR; cell.border = BOX
        cell.alignment = Alignment(horizontal="center", vertical="center")
    for k, t in enumerate(types):
        put(ws4, 5 + k, 5, t); put(ws4, 5 + k, 6, f"=SUMIF($A$5:$A${re_},E{5 + k},$B$5:$B${re_})", align="center")
    put(ws4, 5 + len(types), 5, "Totale viti", bold=True, fill=TOT); put(ws4, 5 + len(types), 6, f"=SUM(F5:F{4 + len(types)})", bold=True, fill=TOT, align="center")
    ws4.freeze_panes = "A5"
    # ------------------------------------------------------------ Pannelli
    ws5 = wb.create_sheet("Pannelli")
    pan = json.load(open(os.path.join(OUT, "docs", "_pannelli.json")))
    ws5["A1"] = "Pannelli da tagliare (Dibond 3 mm, PMMA 4 mm e 3 mm)"; ws5["A1"].font = Font(name=F, bold=True, size=14)
    ws5["A2"] = ("Un file DXF per pannello in docs/pannelli/ (mm, vista dalla faccia esterna) e il PDF pannelli.pdf con quote e fori. "
                 "Si possono mandare cosi' come sono a un servizio di taglio CNC; il prezzo al m2 (in blu) e' una stima.")
    ws5["A2"].font = Font(name=F, size=9, italic=True); ws5["A2"].alignment = Alignment(wrap_text=True, vertical="top"); ws5.merge_cells("A2:H2"); ws5.row_dimensions[2].height = 30
    ws5["A3"] = "Prezzo Dibond tagliato (€/m2)"; ws5["B3"] = 60.0; ws5["C3"] = "Prezzo plexiglass tagliato (€/m2)"; ws5["D3"] = 110.0
    for a in ("A3", "C3"): ws5[a].font = Font(name=F, size=10)
    for a in ("B3", "D3"): ws5[a].font = Font(name=F, size=10, color="0000FF"); ws5[a].number_format = EUR
    header(ws5, 5, ["Pannello", "Larghezza (mm)", "Altezza (mm)", "Materiale", "Fori tondi", "Aperture", "Area (m2)", "Costo stimato (€)", "File DXF"], [24, 11, 11, 46, 9, 9, 10, 12, 26])
    for i, p in enumerate(pan):
        r = 6 + i
        put(ws5, r, 1, p["nome"].replace("_", " "), bold=True); put(ws5, r, 2, p["x"], fmt="0.0"); put(ws5, r, 3, p["y"], fmt="0.0"); put(ws5, r, 4, p["materiale"])
        put(ws5, r, 5, p["fori"], align="center"); put(ws5, r, 6, p["aperture"], align="center"); put(ws5, r, 7, f"=B{r}*C{r}/1000000", fmt="0.000")
        put(ws5, r, 8, f'=G{r}*IF(ISNUMBER(SEARCH("PMMA",D{r})),$D$3,$B$3)', fmt=EUR); put(ws5, r, 9, "docs/pannelli/" + p["nome"] + ".dxf")
    rt5 = 6 + len(pan)
    put(ws5, rt5, 1, "TOTALE", bold=True, fill=TOT); put(ws5, rt5, 7, f"=SUM(G6:G{rt5 - 1})", bold=True, fmt="0.000", fill=TOT); put(ws5, rt5, 8, f"=SUM(H6:H{rt5 - 1})", bold=True, fmt=EUR, fill=TOT)
    for j in (2, 3, 4, 5, 6, 9): put(ws5, rt5, j, None, fill=TOT)
    ws5.freeze_panes = "B6"
    path = os.path.join(OUT, "docs", "NuovaDistinta.xlsx"); wb.save(path)
    return path

if __name__ == "__main__":
    print(build())
