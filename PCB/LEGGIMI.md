# FilMachine Mainboard v1.2 — ordine JLCPCB (PCB + assemblaggio)

Scheda madre per la FilMachine (macchine v6 e v7). Il display JC4880P433 (ESP32-P4) resta separato e si collega con il flat 2×13
su **J5**, che porta anche la sua **alimentazione** (5 V su JP1 pin 2/4, tramite PTC F7 e diodo D8) e da cui la scheda
prende i **3,3 V** per la sua logica (MCP23017, ADS1115, porte AND, pull-up): così l'I2C del display non resta mai
bloccato da un chip spento, nemmeno con la scheda senza 12 V e il display alimentato da USB.
La scheda arriva **completa e pronta**: tutti i componenti montati (SMD e THT), fusibili SMD già saldati.

**Tutti i componenti sono presi dal catalogo JLCPCB/LCSC** (codice C… in ogni riga della distinta, verificato in
magazzino il 22/10/2026): 29 codici *Basic* e 24 *Extended*; costo componenti ≈ 20 $ a scheda più i costi fissi degli
Extended. I connettori sono i più economici del catalogo: morsetti a vite WJ500V (5,08 mm) e DB301V (3,5 mm), JST-XH,
header 2,54.

## File del pacchetto

| File | A cosa serve |
|---|---|
| `FilMachine_Mainboard_Gerber.zip` | Gerber (estensioni Protel) + forature Excellon, da caricare in *Order now* |
| `FilMachine_Mainboard_BOM.csv` | Distinta nel formato JLCPCB: `Comment, Designator, Footprint, LCSC Part #` (una riga per codice; i DNP non ci sono) |
| `FilMachine_Mainboard_CPL.csv` | Piazzamento nel formato JLCPCB: `Designator, Mid X, Mid Y, Layer, Rotation` (solo lato Top). Posizione e rotazione di ogni componente sono riferite al **footprint della libreria JLCPCB/EasyEDA di quel codice LCSC** (origine e orientamento a 0°), ricavate confrontando pezzo per pezzo i due footprint (`sorgenti/jlcpos.py`): nell'anteprima di JLCPCB i componenti devono risultare già sui loro pad |
| `FilMachine_Mainboard_BOM_leggibile.xlsx` | La stessa distinta, leggibile (quantità, produttore, codice costruttore, descrizione; i non montati nel foglio «Non montati»). Solo da consultare: **su JLCPCB va caricato il CSV** |
| `FilMachine_Mainboard_Schema.pdf` | Schema elettrico completo (4 fogli) |
| `FilMachine_Mainboard_Assembly_Top.pdf` | Disegno di montaggio lato componenti |
| `FilMachine_Mainboard.step` | Modello 3D della scheda montata, per il cabinet |
| `FilMachine_Mainboard_top.png`, `_bottom.png`, `_3D.png` | Render della scheda |
| `../kicad/` | Progetto KiCad 9 (schema su 4 fogli, PCB, libreria `FM.pretty` con i footprint dei connettori LCSC) |
| `../sorgenti/` | Script Python che generano schema e PCB da `design.py` (`make.sh` rifà tutto, `export.py` produce questi file) |

## Ordine PCB (jlcpcb.com → Order now → carica il Gerber zip)

| Voce | Valore |
|---|---|
| Layers | **4** |
| Dimensions | 110 × 90 mm (le legge dal Gerber) |
| PCB Thickness | 1,6 mm |
| **Outer Copper Weight** | **2 oz** (correnti fino a 10 A sui riscaldatori) |
| Inner Copper Weight | **1 oz** (consigliato: il piano di massa interno fa da ritorno per riscaldatori e pompa insieme al rame esterno; con 0,5 oz la scheda funziona comunque, perché i ritorni principali hanno rame dedicato sugli strati esterni) |
| Impedance Control | No |
| Surface Finish | LeadFree HASL (va bene anche ENIG) |
| Via Covering | Tented |
| Min via hole size / diameter | 0,3 mm / (0,6 mm) — è il default |
| Mark on PCB | **Specify a location**: sul lato bottom c'è il testo `JLCJLCJLCJLC`, che JLCPCB sostituisce con il numero d'ordine |
| Stackup | il file `FilMachine_Mainboard-job.gbrjob` nello zip dichiara L1 segnali+potenza · L2 GND · L3 +12 V · L4 segnali+potenza |

## Assemblaggio (PCB Assembly → Standard; solo lato **Top**)

1. Carica `FilMachine_Mainboard_BOM.csv` (non l'xlsx) e `FilMachine_Mainboard_CPL.csv`.
2. Nella pagina dei componenti tutte le righe devono risultare abbinate per codice LCSC (nessuna riga «no part
   selected»). Se un codice risultasse momentaneamente esaurito, in fondo c'è la tabella delle alternative.
3. **Anteprima di piazzamento**: JLCPCB mostra ogni componente sulle piazzole con il proprio modello. Con questo CPL
   devono risultare tutti al posto giusto (anche i THT: morsetti, box header, header, JST, elettrolitici). I modelli dei
   morsetti alti (14 mm) nella vista 3D in prospettiva sembrano «sporgere» dalle piazzole: è solo la prospettiva — con
   la vista dall'alto (2D) i pin coincidono con i fori. Da controllare comunque, confrontando con `Assembly_Top.pdf`:
   - integrati U1–U10 (pin 1 = puntino/smusso sul lato del segno in serigrafia);
   - diodi D1, D2 (TVS, banda = catodo verso il +12 V), D6, D7, D8, D10–D13, D21–D28, D41, D42, D47–D52 (banda sul
     lato della riga in serigrafia); LED D3, D4, D5, D31–D38, D43, D44;
   - transistor Q1–Q8 (SOT-23), Q9/Q10 (TO-252: linguetta in alto), U8/U9 (TO-263: linguetta a sinistra, pin a destra);
   - elettrolitici C1, C55, C56: piazzola **quadrata = +**;
   - morsetti a vite J1–J3, J15–J26, J28, J29: l'apertura per i fili guarda **verso il bordo** della scheda;
   - box header J5: il flat esce verso il bordo alto.
   Se qualcosa non torna, ruotarlo nell'anteprima: JLCPCB verifica comunque le polarità col controllo DFM prima di produrre.
4. **Non montati (DNP)**: J6 (header ESP32-C6), R6, R7 (pull-up I2C: ci sono già sul display). Non compaiono né nella
   BOM né nel CPL, quindi JLCPCB non li chiede. Se compare l'avviso «R6,R7,J6 designators don't exist in the CPL file»
   vuol dire che come BOM è stato caricato un file che li contiene (p. es. l'xlsx): si può premere *Continue* (quei tre
   non vanno montati) oppure, meglio, ricaricare `FilMachine_Mainboard_BOM.csv`.

## Cosa collegare

| Connettore | Cosa | Note |
|---|---|---|
| J1 `12V IN HEATER 1` | LRS-350-12, coppia +V/−V n. 1 | **− sopra, + sotto** (serigrafia accanto al morsetto); solo riscaldatore 1 (fusibile F1 15 A) |
| J2 `12V IN HEATER 2` | LRS-350-12, coppia +V/−V n. 2 | **− sopra, + sotto**; solo riscaldatore 2 (F2 15 A) |
| J3 `12V IN MAIN` | LRS-350-12, coppia +V/−V n. 3 | **− sopra, + sotto**; pompa (F3 15 A), valvole/motore/logica/uscite (F4 8 A) |
| J23 / J24 `HEATER 1/2` | riscaldatori 100 W | + a sinistra, − a destra (indifferente per una resistenza) |
| J25 `PUMP` | pompa peristaltica Kamoer KPHM600 (12 V, 0,8 A, motore DC a spazzole) | A sopra, B sotto; il verso si inverte da firmware (*Inverti pompa*). Va bene qualsiasi motore DC 12 V reversibile |
| J26 `MOTOR` | motore spirale JGB37-520 | |
| J15 … J20 | elettrovalvole 12 V: C1, C2, C3, WB, WASTE, INLET | un morsetto a 2 poli per valvola, + a sinistra (nome in serigrafia) |
| J21 / J22 `SPARE 1/2` (`SPARE OUT 1/2` nello schema) | macchina v7: J21 = valvola di **recupero** (A6), J22 = **pompetta di ricircolo** della vasca (A7) | come le valvole: carico fino a 2 A, con diodo di ricircolo e LED |
| J30 `GPIO SPARE` | header 2,54 a 9 poli: B0, B1, B2, B3, B4, B5, 3V3, 5V, GND (pin 1 = B0, a sinistra) | i 6 pin liberi della porta B dell'MCP23017 (fino alla v1.1 erano i sensori di livello delle chimiche) come GPIO **a 3,3 V**: 1 kΩ in serie e diodo di clamp a 3,3 V (un segnale a 5 V non fa danni, ma niente 12 V); il firmware li lascia ingressi con pull-up. 3,3 V dal display (pochi mA), 5 V di servizio (PTC F5, condiviso con J29), GND |
| J28 `12V OUT` · J29 `5V OUT` | uscite di servizio per accessori (ventola, striscia LED, modulo relè…) | + a sinistra; PTC autoripristinanti: 12 V max 2 A (F8), 5 V max 1 A (F5) |
| J7 `BATH LEVEL` | XKC-Y21 MIN e MAX della vasca | JST-XH 4 poli: 5 V, MIN, MAX, GND. Il **MIN** va montato **sopra i riscaldatori**: senza MIN bagnato il firmware non li accende. Il MAX serve al riempimento automatico dalla rete |
| J11 `FLOW` · J12 `HALL` | YF-S201 · KY-003 | JST-XH 3 poli: 5 V, segnale, GND |
| J13 / J14 `TEMP 1/2` | DS18B20 bagno / chimica | JST-XH 3 poli: 3,3 V, DQ, GND |

I 5 connettori dei sensori stanno in **una sola fila** sul lato corto destro (dall'alto: BATH LEVEL, FLOW, HALL, TEMP 1,
TEMP 2). Dalla v1.2 i contenitori delle chimiche non hanno sensori di livello: i livelli si controllano a vista.
| J27 `SERVO` | MG90S della taglierina | header 2,54: segnale, +5 V, GND |
| J5 `DISPLAY JP1` | flat 2×13 verso JP1 del JC4880P433 | pin 1 = pin 1 (piazzola quadrata); porta segnali, alimentazione del display (pin 2/4) e i 3,3 V verso la scheda (pin 1/3, `VCC3.3`); il pin 18 (`ESP_3V3`) è lasciato scollegato sulla scheda |

Le tre coppie +V/−V dell'LRS-350-12 vanno ciascuna sul proprio morsetto: così i 10 A di ogni riscaldatore e quelli
della pompa non passano mai dallo stesso morsetto (ogni morsetto resta sotto 8–10 A, metà della portata del WJ500V).

## Verifiche fatte

- Schema: ERC 0 violazioni; netlist dello schema identica pin per pin a `design.py`; PCB identico pin per pin.
- PCB: DRC con le regole JLCPCB 4 strati (0,2/0,2 mm, foro 0,3): 0 errori, 0 collegamenti mancanti; restano solo
  avvisi di serigrafia (testi che si toccano), che non riguardano la produzione.
- Distinta: ogni riga ha un codice LCSC esistente, in magazzino e abbinabile (controllo fatto sul catalogo live di
  JLCPCB); BOM e CPL con gli stessi designator (nessun DNP in nessuno dei due).
- Revisione dei datasheet LCSC per i footprint dei connettori e dei fusibili (misure prese dai disegni dei costruttori).
- Revisione indipendente del progetto (pinout, stati all'accensione, correnti, alimentazione del display).
- Firmware: uno solo per moduli separati e Mainboard (vedi `NOTE_firmware.md`).
- **Non è stata provata dal vero.** Ordinare il lotto minimo (5 pezzi, 2 assemblati bastano) e alla prima accensione
  seguire l'ordine: solo J3 senza carichi (LED verdi 12 V e 5 V) → flat del display (LED 3,3 V) → valvole → motore →
  pompa → J1/J2 e riscaldatori. Sulla scheda non c'è niente da saldare, ponticellare o impostare.
- **Prima di infilare il flat**, col display acceso da USB, misurare sul suo JP1 (pin 1 = quello segnato): pin 1 e 3
  = 3,3 V, pin 2 e 4 = 5 V, pin 5, 6 e 16 = 0 V (GND). È la mappa usata dalla scheda (quella del README del firmware);
  un tester toglie ogni dubbio su un eventuale cambio di revisione del display.
- Senza display collegato il LED verde «3V3» può accendersi debolmente (perdita attraverso i partitori dell'ADC):
  non è un guasto.
- Da misurare alla prima accensione, col display collegato e il suo cavo USB-C staccato: la tensione sul VBUS della
  USB-C del display (dice se l'IP5306 lascia passare il 5 V all'indietro; è innocuo, ma è bene saperlo).

## Alternative se un codice risultasse esaurito

Tutte verificate sul catalogo live JLCPCB (13/10/2026). Stesso passo e stesse forature; per i morsetti l'ingombro del
corpo può differire di qualche decimo di millimetro, senza conseguenze sul piazzamento.

| Designator | Codice usato | Alternativa |
|---|---|---|
| J5 box header 2×13 angolato | C7430326 (CONNFLY DS1013-26RSiB-B-0) | C21167 (BOOMELE 2×13 right angle) |
| J1–J3, J23–J25 WJ500V-5.08-2P | C8465 (Kangnex, 18 A) | C474906 (Cixi Kefa KF127-5.08-2P, 15 A) |
| J15–J22, J26, J28, J29 DB301V-3.5-2P | C695629 (DORABO, 7 A) | C474892 (Cixi Kefa KF350-3.5-2P, 10 A) |
| Q9/Q10 AOD4184A | C99124 (Alpha & Omega) | C5370959 (TECH PUBLIC AOD4184A, stesso package TO-252) |
| F1–F3 / F4 fusibili 2410 | C136393 / C136390 (JDT JFC2410) | qualsiasi fusibile SMD 2410 da 15 A / 8 A con lo stesso footprint |
