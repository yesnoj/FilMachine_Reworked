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
 ("Bagno e vaschette", "Contenitore Euro 400 x 300 x 220 mm (RS PRO, codice RS 725-9151)", "polipropilene, pareti e fondo CHIUSI (non forato); interno circa 365 x 265 x 215", 1, "pz", 27.05, "bagno termostatico", "regge 70 °C; quattro fori nella parete corta destra: due Ø16,5 (riscaldatori), uno Ø22 (troppopieno), uno Ø21,5 (rubinetto); posizioni in verifiche.txt"),
 ("Bagno e vaschette", "Vaschetta Gastronorm GN 1/9, altezza 150 mm, in polipropilene", "176 x 108 mm, 1,5 l, con coperchio", 3, "pz", 8.0, "chimiche C1, C2, C3", "un foro Ø16,4 al centro del fondo (punta a gradino) per l'innesto dal basso C11/C12"),
 ("Bagno e vaschette", "Tubo quadro di alluminio 15 x 15 x 1,5 mm", "barra da 1 m (servono due spezzoni da 356 mm)", 1, "pz", 5.0, "longheroni che reggono le vaschette", ""),
 ("Bagno e vaschette", "O-ring 13 x 2 in EPDM", "", 5, "pz", 0.3, "codolo dell'innesto C12 (uno per vaschetta)", "2 di scorta"),
 ("Bagno e vaschette", "O-ring 6 x 1,5 in EPDM", "", 5, "pz", 0.2, "otturatore C13 (uno per vaschetta)", "2 di scorta"),
 ("Bagno e vaschette", "Guarnizione piana in EPDM 16,5 x 26 x 2 mm", "o ritagliata da un foglio di EPDM da 2 mm con due fustelle", 5, "pz", 0.3, "tra flangia di C11 e fondo della vaschetta", "2 di scorta"),
 ("Bagno e vaschette", "Molla a compressione inox Ø7 x 32 mm, filo 0,4", "", 5, "pz", 0.5, "richiamo dell'otturatore C13", "2 di scorta; precarico circa 0,5 N"),
 ("Bagno e vaschette", "Dado basso M16 inox + 2 guarnizioni piane in silicone 16 x 24 x 2", "una serie per riscaldatore", 2, "serie", 3.0, "passaggio dei riscaldatori nella parete del bagno", "controllare il passo del filetto sul riscaldatore"),
 ("Bagno e vaschette", "Termostato bimetallico KSD301 normalmente chiuso, 60 °C, 10 A", "", 2, "pz", 1.5, "sicurezza, in serie a ogni riscaldatore", "i riscaldatori non devono mai lavorare a secco"),
 ("Bagno e vaschette", "Passaparete 1/2\" con portagomma a gomito da 16 mm", "con dado e guarnizione", 1, "pz", 4.0, "troppopieno del bagno (foro Ø22 nella parete del contenitore)", ""),
 ("Bagno e vaschette", "Tubo in PVC cristal 12 x 16 mm", "3 m", 1, "pz", 4.0, "troppopieno del bagno e uscita del rubinetto del bagno, fino a un secchio o al lavandino", ""),
 ("Bagno e vaschette", "Passaparete in PP 1/2\" a filetto lungo (almeno 60 mm), con due dadi e guarnizioni", "foro Ø21,5 nel contenitore, Ø23 nel pannello", 1, "pz", 4.0, "rubinetto del bagno: attraversa la parete del contenitore e il pannello destro", "un dado dentro il bagno, flangia e guarnizione fuori, secondo dado sul pannello"),
 ("Bagno e vaschette", "Gomito 1/2\" F-F in PP + nipplo 1/2\" M-M corto (30 mm)", "", 1, "serie", 3.0, "pescante del rubinetto del bagno, dentro il contenitore", "il nipplo va accorciato perche' arrivi a 4 mm dal fondo"),
 ("Bagno e vaschette", "Rubinetto a sfera 1/2\" F x portagomma 16 mm, in PP o PVC", "", 1, "pz", 6.0, "rubinetto del bagno, fuori dal pannello destro", "con il tubo 12 x 16 verso un secchio piu' in basso della macchina: il bagno si svuota per sifone"),
 ("Bagno e vaschette", "Nastro biadesivo per esterni (tipo VHB) 19 mm", "rotolo corto", 1, "pz", 3.0, "sensori di livello del bagno sulla parete del contenitore, supporto dell'altoparlante", ""),
 ("Idraulica", "Tubo LLDPE 3/8\" (9,52 x 6,35 mm) per osmosi inversa", "rotolo da 10 m (ne servono circa 3,1 dentro la macchina, il resto per il collegamento al rubinetto)", 1, "pz", 8.0, "tutte le linee interne e l'ingresso dell'acqua", ""),
 ("Idraulica", "Elettrovalvola 12 V normalmente chiusa, 3/8\" a innesto (uguale alle altre cinque)", "", 1, "pz", 7.59, "valvola dell'ingresso dell'acqua di rete (sotto il piano del cavalletto)", "la sesta: le cinque già acquistate stanno sul collettore. Montarla con la freccia verso il bagno"),
 ("Idraulica", "Gomito a innesto rapido 3/8\" - 3/8\"", "per tubo da 9,52 mm", 9, "pz", 0.7, "1 sopra la valvola dello scarico, 1 ai piedi della salita verso la tank, 5 sulla linea dell'ingresso", "2 di scorta; meglio in polipropilene"),
 ("Idraulica", "Gomito a codolo 3/8\" (un lato a innesto, l'altro a codolo liscio da 9,52 mm)", "in inglese \"stem elbow\"", 2, "pz", 1.5, "uscita del collettore C07 verso la pompa", "uno di scorta; codolo lungo almeno 33 mm dall'angolo"),
 ("Idraulica", "Raccordo diritto: filetto maschio 3/8\" BSP - tubo 3/8\" a innesto", "", 2, "pz", 1.5, "attacchi della pompa", "con nastro in PTFE"),
 ("Idraulica", "Raccordo diritto: filetto FEMMINA 1/2\" - tubo 3/8\" a innesto", "", 2, "pz", 1.5, "attacchi del flussimetro YF-S201", "con nastro in PTFE"),
 ("Idraulica", "Passaparete a innesto rapido 3/8\" - 3/8\"", "foro Ø16", 2, "pz", 2.0, "attacchi SCARICO (piastra C09) e INGRESSO (piastra C16)", ""),
 ("Idraulica", "Adattatore per rubinetto con valvolina, uscita tubo 3/8\" (kit di alimentazione per osmosi inversa)", "filetto 1/2\" o 3/4\" secondo il rubinetto", 1, "pz", 5.0, "collegamento dell'ingresso INGRESSO al rubinetto dell'acqua", "pressione di rete massima 0,8 MPa (limite delle valvole)"),
 ("Idraulica", "Tubo in silicone 8 x 12 mm, NERO", "3 m", 1, "pz", 9.0, "tubo della tank, troppopieno della tank, tubi esterni verso le taniche", "nero: i tubi che arrivano alla tank non devono far passare la luce"),
 ("Idraulica", "O-ring 9 x 2 in EPDM", "", 20, "pz", 0.15, "6 sedi del collettore C07, 6 delle camere C15, 3 delle prese C14, 1 dell'imbuto C20 (di scorta per i codoli)", "4 di scorta"),
 ("Idraulica", "Fascette in nylon 2,5 x 100 mm", "confezione da 100", 1, "conf", 3.0, "valvole, sensori, cavi", ""),
 ("Idraulica", "Fascette in nylon 4,8 x 200 mm", "confezione da 50", 1, "conf", 3.0, "staffe C18 (flussimetro e valvola dell'ingresso), sensori sulle camere C15", ""),
 ("Idraulica", "Grasso al silicone per O-ring", "tubetto", 1, "pz", 5.0, "O-ring, codoli degli innesti, paraolio, baionetta della spirale", ""),
 ("Tank e caricatore", "Albero in acciaio inox Ø8 x 61 mm", "barra rettificata h6/h7, due spianature per i grani", 1, "pz", 4.0, "trasmissione della spirale", ""),
 ("Tank e caricatore", "Cuscinetto 688-2RS (8 x 16 x 5)", "meglio in acciaio inox", 2, "pz", 2.5, "cartuccia B17", ""),
 ("Tank e caricatore", "Paraolio 8 x 16 x 5", "NBR", 1, "pz", 2.0, "tenuta dell'albero verso la vasca", ""),
 ("Tank e caricatore", "Rondella inox M8 (8,4 x 16 x 1,6)", "", 1, "pz", 0.2, "reggispinta tra trascinatore e cartuccia", ""),
 ("Tank e caricatore", "O-ring 24 x 2", "", 1, "pz", 0.3, "tenuta della flangia della cartuccia", ""),
 ("Tank e caricatore", "O-ring 80 x 3 in EPDM", "", 2, "pz", 1.0, "cinghia tonda tra motore e spirale", "uno di scorta"),
 ("Tank e caricatore", "Magneti al neodimio Ø6 x 3", "confezione da 10 (ne servono 4)", 1, "conf", 3.0, "ruota magneti del sensore Hall", ""),
 ("Tank e caricatore", "Molla a compressione Ø6 x 22 mm, filo 0,5", "", 1, "pz", 1.0, "perno folle a molla", ""),
 ("Tank e caricatore", "Vite M3 x 10 a testa zigrinata (vite a mano)", "", 2, "pz", 0.5, "blocco delle due slitte della guida B14/B15 nelle fossette della barra B13", "si stringe a mano, senza attrezzi"),
 ("Tank e caricatore", "Fascetta della Lab-Box, già fatta", "fascetta trasparente 14,5 x 0,15 mm con il piolino sul capo (3,25 x 15,8 mm), ricambio Ars-Imago", 1, "pz", None, "passa nel tubo A03 da parte a parte, tra i due collarini: si infila dalla finestra larga e il perno resta dentro, contro la finestra stretta", "si compra già fatta; prezzo da verificare. Le finestre del tubo sono fatte sulle sue misure"),
 ("Tank e caricatore", "Micro-servo MG90S", "con squadretta a un braccio", 1, "pz", 4.0, "taglierina", ""),
 ("Tank e caricatore", "Lame di ricambio per cutter da 9 mm", "confezione da 10", 1, "conf", 2.0, "due spezzoni da 36 mm nella taglierina", ""),
 ("Tank e caricatore", "Velluto nero adesivo", "striscia da 1 m", 1, "pz", 4.0, "guarnizione di luce su coperchio e fessura", "facoltativo, da decidere dopo la prova di tenuta alla luce"),
 ("Elettrico", "Alimentatore Mean Well LRS-350-12 (12 V, 29 A)", "215 x 115 x 30 mm, ventola a controllo di temperatura", 1, "pz", 32.0, "in piedi nello zoccolo D16 contro la paratia", "sostituisce il 12 V 30 A già acquistato, che non entra nel vano"),
 ("Elettrico", "Presa IEC C14 da pannello con interruttore e portafusibile", "foro 27,5 x 47,5 mm", 1, "pz", 4.0, "pannello sinistro, vano elettronica", ""),
 ("Elettrico", "Cavo di alimentazione con spina e connettore IEC C13", "", 1, "pz", 4.0, "", ""),
 ("Elettrico", "Dissipatori adesivi per TO-220", "confezione", 1, "conf", 3.0, "i due MOSFET del modulo IRF540 dei riscaldatori", "a 8,3 A per canale l'IRF540 scalda: dissipatori e ripiano basso, vicino alle griglie d'aria"),
 ("Elettrico", "Portafusibile a lama in linea con fusibile", "uno da 10 A, uno da 5 A", 2, "pz", 1.5, "pompa; valvole e logica", "consigliati"),
 ("Elettrico", "Cavo unipolare 1,5 mm2 rosso e nero", "5 m + 5 m", 1, "serie", 8.0, "linee a 12 V di riscaldatori e pompa", ""),
 ("Elettrico", "Cavetti per segnali, puntalini, connettori", "assortimento", 1, "serie", 5.0, "sensori, valvole, servo", ""),
 ("Elettrico", "Passacavo in gomma Ø22", "", 4, "pz", 0.5, "paratia (2), divisorio (1), fianco del cavalletto (1)", ""),
 ("Elettrico", "Resistenza 4,7 kohm", "", 1, "pz", 0.1, "pull-up del bus DS18B20", "se non e' gia' sulla scheda"),
 ("Elettrico", "Tester per servo", "", 1, "pz", 3.0, "prova della taglierina senza firmware", "facoltativo"),
 ("Struttura e viteria", "Multistrato 420 x 505 x 12 mm", "tagliato a misura", 1, "pz", 8.0, "base della macchina", "verniciare o impregnare"),
 ("Struttura e viteria", "Pannelli in Dibond (composito di alluminio) 3 mm tagliati a misura", "7 pezzi, 0,64 m2 in tutto; misure e fori nel foglio Pannelli e nei DXF di docs/pannelli", 1, "serie", 40.0, "guscio, piano anteriore, paratia, divisorio", "colori: fianchi grigio chiaro o alluminio spazzolato, piano antracite"),
 ("Struttura e viteria", "Lastra di plexiglass (PMMA) trasparente 3 mm, 430,6 x 309,8 mm", "taglio a misura con 9 fori Ø6 (vedi DXF)", 1, "pz", 15.0, "coperchio del bagno", "bordi lucidati o smussati"),
 ("Struttura e viteria", "Piedini in gomma Ø20", "", 4, "pz", 0.5, "sotto la base", ""),
 ("Struttura e viteria", "Viti da legno 3,5 x 16 a testa svasata NERE", "confezione da 20 (ne servono 12)", 1, "conf", 3.0, "pannelli del guscio nel bordo della base", "svasare leggermente i fori del Dibond"),
 ("Struttura e viteria", "Viti da legno 3,5 x 12 a testa cilindrica", "confezione da 50 (ne servono 33)", 1, "conf", 3.0, "tutti i pezzi fissati alla base (dima)", "non piu' lunghe: la base e' spessa 12 mm"),
 ("Struttura e viteria", "Viti autofilettanti per plastica 3 x 8", "testa cilindrica, confezione da 50 (ne servono 16)", 1, "conf", 4.0, "vedi foglio Viteria", ""),
 ("Struttura e viteria", "Viti autofilettanti per plastica 3 x 10", "testa cilindrica, confezione da 50 (ne servono 24)", 1, "conf", 4.0, "vedi foglio Viteria", ""),
 ("Struttura e viteria", "Viti autofilettanti per plastica 3 x 16", "testa cilindrica, confezione da 20 (ne servono 8)", 1, "conf", 3.0, "vedi foglio Viteria", ""),
 ("Struttura e viteria", "Viti M3 x 10 a testa svasata con dado", "8 pezzi", 1, "conf", 2.0, "piastre C09 e C16 attraverso il pannello destro", ""),
 ("Struttura e viteria", "Viti M3 x 16 con dado (10) e M3 x 10 con dado (2)", "", 1, "conf", 3.0, "morsetti delle schede D15; staffa dell'alimentatore D17 sulla paratia", "M3 x 12 e' troppo corta per i morsetti: morsetto 8,7 mm + ripiano 3 mm + dado"),
 ("Struttura e viteria", "Minuteria della tank: 3 grani M3 x 6 con dado M3, 4 viti autofilettanti M2 x 6, 1 vite M2,5 x 8, 2 viti M3 x 8, 2 viti M3 x 10, 1 vite M3 x 5", "assortimento", 1, "serie", 4.0, "trascinatore, pulegge, morsetti lama, manovella, motoriduttore, perno folle, modulo Hall", "una delle M3 x 10 fa da spinotto tra biella e portalama"),
 ("Struttura e viteria", "Viti per lamiera: 2,9 x 13 a testa svasata (6) e 2,9 x 9,5 a testa cilindrica (8)", "una confezione piccola per misura", 1, "serie", 3.0, "fermi girevoli C03; portasensori C04 e portasonde C06 sui longheroni", "forare l'alluminio a 2,4 mm"),
 ("Struttura e viteria", "Viti M4 x 12", "4 pezzi", 1, "conf", 1.0, "testate del telaio C05 sotto la fascia del contenitore", ""),
 ("Struttura e viteria", "Barra filettata M3 da 131 mm con dadi e rondelle", "4 pezzi (tagliare da una barra da 1 m)", 1, "serie", 3.0, "colonna dei tre ripiani a griglia", ""),
 ("Struttura e viteria", "Filamento PETG, bobina da 1 kg", "colore a scelta (antracite consigliato per montanti e cruscotto)", 3, "pz", 20.0, "tutti i pezzi stampati", "stima: vedi foglio Pezzi stampati"),
 ("Struttura e viteria", "Filamento PETG TRASPARENTE, bobina da 1 kg", "", 1, "pz", 22.0, "camere di livello C15 (si vede il liquido; il sensore lavora attraverso la parete)", "ne servono circa 100 g: va bene anche una bobina piccola"),
]

OWNED = [  # componente, specifiche, quantita', prezzo, uso in questa macchina
 ("Alimentatore 12V 30A", "", 1, 28.18, "Non usato in questa versione: al suo posto il Mean Well LRS-350-12, più sottile (30 mm), che sta in piedi contro la paratia. Tenerlo per il banco di prova"),
 ("Display", "JC4880P443C", 1, 25.59, "Usato: dietro la finestra del cruscotto inclinato D09, trattenuto dalle quattro linguette D10"),
 ("Pompa", "Marco UP3-R", 1, 184.46, "Usata: sotto il cavalletto della tank. Tenerla a PWM basso e sciacquarla con acqua a fine lavoro (corpo in ottone nichelato)"),
 ("Elettrovalvola", "DC 12V Normally Close", 5, 7.59, "Usate tutte, in fila sul collettore C07. Da sinistra: C1, WB, C2, C3, scarico. Freccia dalla vaschetta (o dal bagno) verso il collettore. La sesta, per l'ingresso dell'acqua, è nel foglio Da acquistare"),
 ("Mosfet driver", "IRF540 V4.0", 1, 5.9, "Usato per i due riscaldatori (uscite B6 e B7), come deciso: a 8,3 A per canale scalda, quindi dissipatori sui MOSFET e ripiano basso, vicino alle griglie d'aria"),
 ("Motor driver", "DC5-12V 0A-30A Dual channel H bridge", 1, 17.29, "Usato: ripiano centrale. Canale A motore della spirale, canale B pompa"),
 ("Solenoid Driver", "MCP23017 GPIO", 1, 13.0, "Usato: ripiano alto, accanto alla breakout. Valvole su A0-A4, valvola dell'ingresso su A5 (serve una piccola modifica al firmware), sensori di livello delle chimiche su B0-B5, riscaldatori su B6-B7"),
 ("Speaker", "Speaker 8ohm 2W", 1, 1.69, "Usato: supporto D19 dietro la foratura del pannello sinistro"),
 ("Motore agitatore", "JGB37-520 12V 20RPM", 1, 8.34, "Usato: nella galleria sotto il ripiano della tank, con cinghia tonda"),
 ("Portafusibile 15A", "Fusibile per riscaldatore", 2, 5.5, "Usati: uno per riscaldatore"),
 ("Riscaldatore", "100W 12V", 2, 10.85, "Usati: nella parete corta destra del bagno, vicino al fondo, uno verso il fronte e uno verso il retro (calore distribuito), ognuno sotto il suo carter C10"),
 ("Convertitore 12V / 5V", "LM2596S", 1, 4.29, "Usato: ripiano basso. Scheda display, sensori di livello, servo"),
 ("Cavo Flat", "Cavo flat grigio 2x13 20cm", 1, 6.75, "Usato: dal display alla breakout sul ripiano alto (con 20 cm ci arriva solo da lì), attraverso la fessura del piano"),
 ("Breakout board", "DC3 26 p 2x13Pins", 1, 10.34, "Usata: sul ripiano alto"),
 ("Sensore hall", "KY-003 A3144", 1, 4.83, "Usato: sulla staffa B25, davanti alla ruota magneti"),
 ("Sensore flusso", "YF-S201 12V", 1, 5.94, "Usato: sulla linea dell'acqua di rete, sotto il piano del cavalletto (staffa C18), subito dopo il passaparete INGRESSO: dice se l'acqua arriva davvero"),
 ("Sensore livello", "XKC-Y21 5V", 8, 5.53, "Usati tutti: 2 sulla parete del bagno (MIN e MAX), 3 sulle camere di livello C15 (MIN delle chimiche: la linea si svuota), 3 sui portasensori C04 (MAX delle vaschette)"),
 ("Sensore temperatura", "DS18B20 30mm 1m", 2, 3.31, "Usati: uno nel bagno (portasonde C06, sul longherone anteriore, lontano dai riscaldatori), uno nella vaschetta C1 (gancio C19 sul longherone, si toglie a mano)"),
]

SCREWS = [  # tipo, quantita', dove. Le lunghezze delle autofilettanti da 3 mm sono quelle verificate da check_screws.py
 ("Vite autofilettante 3 x 10", 4, "blocchi ponte sul ripiano della tank (B04, B05)"), ("Vite autofilettante 3 x 16", 4, "tetto del ponte (B06)"),
 ("Vite autofilettante 3 x 10", 4, "culla universale (B12)"), ("Vite M2 x 6 autofilettante", 4, "morsetti delle lame"),
 ("Vite M2 x 8 (del servo)", 2, "alette del servo"), ("Vite M2,5 x 8", 1, "perno di manovella"), ("Vite M3 x 10", 1, "spinotto biella-portalama"),
 ("Vite autofilettante 3 x 8", 2, "coperchietto del vano servo (B03)"), ("Vite autofilettante 3 x 8", 2, "flangia della cartuccia (B17)"),
 ("Vite autofilettante 3 x 10", 1, "staffa Hall B25"), ("Vite autofilettante 3 x 10", 3, "piastra del motore (B23)"),
 ("Vite M3 x 8", 2, "motoriduttore sulla piastra: nel riduttore entrano 4 mm; se i suoi fori sono meno profondi usare M3 x 6"),
 ("Grano M3 x 6 + dado M3", 3, "trascinatore e due pulegge"), ("Vite M3 x 10", 1, "ritegno del perno folle"),
 ("Vite M3 x 5 (o una fascetta)", 1, "modulo Hall sulla staffa B25"),
 ("Vite M3 x 10 a testa zigrinata", 2, "blocco delle slitte B14 e B15 (nelle fossette della barra B13: posizione 135 e 120)"),
 ("Vite autofilettante 3 x 16", 4, "piedini della tank sul piano del cavalletto D01"),
 ("Vite per lamiera 2,9 x 13 svasata", 6, "fermi girevoli C03 sui longheroni (davanti e dietro ogni vaschetta)"),
 ("Vite per lamiera 2,9 x 9,5", 6, "portasensori MAX C04 sul longherone anteriore"), ("Vite per lamiera 2,9 x 9,5", 2, "portasonde C06"),
 ("Vite M4 x 12", 4, "testate del telaio C05"),
 ("Vite da legno 3,5 x 12", 2, "collettore C07"), ("Vite da legno 3,5 x 12", 6, "pettine valvole C08"), ("Vite da legno 3,5 x 12", 4, "carter dei due riscaldatori C10 (due ciascuno)"),
 ("Vite M3 x 10 svasata + dado", 4, "piastra degli scarichi C09 attraverso il pannello destro"), ("Vite M3 x 10 svasata + dado", 4, "piastra dell'ingresso C16 attraverso il pannello destro"),
 ("Vite autofilettante 3 x 8", 4, "staffe a fascetta C18 sotto il piano del cavalletto"),
 ("Vite da legno 3,5 x 12", 6, "fianchi del cavalletto D02 e D03"),
 ("Vite da legno 3,5 x 12", 4, "piede della pompa: forare secondo le asole della pompa"),
 ("Vite autofilettante 3 x 10", 6, "piano del cavalletto D01 sui fianchi"), ("Vite autofilettante 3 x 10", 4, "frontale del cavalletto D04"), ("Vite autofilettante 3 x 10", 2, "carter della trasmissione D05"),
 ("Vite da legno 3,5 x 12", 5, "guide a U D11 di paratia (3) e divisorio (2)"), ("Vite da legno 3,5 x 12", 4, "piedi della colonna D13"),
 ("Vite da legno 3,5 x 12", 2, "zoccolo dell'alimentatore D16"), ("Vite M3 x 10 + dado", 2, "staffa dell'alimentatore D17 sulla paratia"),
 ("Barra filettata M3 x 131 + dadi", 4, "colonna dei ripiani (dado incassato nel piede D13)"), ("Vite M3 x 16 + dado", 10, "morsetti delle schede D15"),
 ("Vite da legno 3,5 x 16 svasata nera", 12, "pannelli del guscio nel bordo della base (3 per pannello)"),
 ("Vite autofilettante 3 x 8", 4, "cruscotto D09, da sotto il piano anteriore"), ("Vite autofilettante 3 x 8", 4, "linguette D10 del display"),
]

def build():
    wb = Workbook()
    # ------------------------------------------------------------ Da acquistare
    ws = wb.active; ws.title = "Da acquistare"
    ws["A1"] = "FilMachine - nuova distinta: componenti da acquistare"; ws["A1"].font = Font(name=F, bold=True, size=14)
    ws["A2"] = ("Prezzi unitari in blu: stime indicative (ottobre 2026), da verificare all'acquisto. Controllati sul web: contenitore Euro RS PRO 400 x 300 x 220 = 27,05 € IVA inclusa "
                "(it.rs-online.com, codice 725-9151); elettrovalvola: prezzo della distinta originale. Alimentatore LRS-350-12: prezzo indicativo dei distributori.")
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
    ws2["A1"] = "Componenti già acquistati (DistintaComponenti.xlsx) e loro uso in questa macchina"; ws2["A1"].font = Font(name=F, bold=True, size=14)
    ws2["A2"] = "Quantità e prezzi sono quelli della distinta originale del progetto."; ws2["A2"].font = Font(name=F, size=9, italic=True)
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
    ws5["A1"] = "Pannelli da tagliare (Dibond 3 mm e plexiglass 3 mm)"; ws5["A1"].font = Font(name=F, bold=True, size=14)
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
        put(ws5, r, 8, f'=G{r}*IF(ISNUMBER(SEARCH("PLEXI",D{r})),$D$3,$B$3)', fmt=EUR); put(ws5, r, 9, "docs/pannelli/" + p["nome"] + ".dxf")
    rt5 = 6 + len(pan)
    put(ws5, rt5, 1, "TOTALE", bold=True, fill=TOT); put(ws5, rt5, 7, f"=SUM(G6:G{rt5 - 1})", bold=True, fmt="0.000", fill=TOT); put(ws5, rt5, 8, f"=SUM(H6:H{rt5 - 1})", bold=True, fmt=EUR, fill=TOT)
    for j in (2, 3, 4, 5, 6, 9): put(ws5, rt5, j, None, fill=TOT)
    ws5.freeze_panes = "B6"
    path = os.path.join(OUT, "docs", "NuovaDistinta.xlsx"); wb.save(path)
    return path

if __name__ == "__main__":
    print(build())
