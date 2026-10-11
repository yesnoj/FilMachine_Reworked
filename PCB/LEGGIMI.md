# FilMachine Mainboard v1.0 — ordine PCBWay

Scheda madre per la FilMachine v6. Il display JC4880P433 (ESP32-P4) resta separato e si collega con il
flat 2×13 su **J4**, che porta anche la sua **alimentazione** (5 V su JP1 pin 2/4, tramite PTC F7 e diodo D8).
La scheda arriva **completa e pronta**: tutti i componenti montati e i fusibili già inseriti.

## File del pacchetto

| File | A cosa serve |
|---|---|
| `FilMachine_Mainboard_Gerber.zip` | Gerber RS-274X + forature Excellon (PTH e NPTH separate), da caricare nel preventivo PCB |
| `FilMachine_Mainboard_BOM.xlsx` | Distinta nel formato PCBWay (designator, quantità, produttore, codice, package, SMD/THT, note) |
| `FilMachine_Mainboard_CPL.csv` | File di piazzamento (centroidi), solo lato TOP |
| `FilMachine_Mainboard_Schema.pdf` | Schema elettrico completo (4 fogli) |
| `FilMachine_Mainboard_Assembly_Top.pdf` | Disegno di montaggio lato componenti |
| `FilMachine_Mainboard.step` | Modello 3D della scheda montata, per il cabinet |
| `FilMachine_Mainboard_3D.png`, `_top.png`, `_bottom.png` | Render della scheda montata |
| `../kicad/` | Progetto KiCad 9 (schema su 4 fogli, PCB, file di progetto) |
| `../sorgenti/` | Script Python che generano schema e PCB da `design.py` (`make.sh` rifà tutto) |

## Parametri da impostare nel preventivo PCB

| Voce | Valore |
|---|---|
| Strati | **4** |
| Dimensioni | **110 × 90 mm** |
| Spessore | 1,6 mm, FR-4 TG150 o superiore |
| Rame esterno / interno | **2 oz / 1 oz** (correnti fino a 16 A sui riscaldatori) |
| Pista / spazio minimi | 0,2 / 0,2 mm (8/8 mil) |
| Foro minimo | 0,3 mm (via 0,6/0,3) |
| Finitura | HASL senza piombo o ENIG |
| Solder mask | verde (o a piacere), serigrafia bianca |
| Stackup | L1 segnali + potenza (70 µm) · L2 GND (35 µm) · L3 +12 V (35 µm) · L4 segnali + potenza (70 µm) — già dichiarato nel file job dei Gerber |
| Via | tenting standard; nessuna via nei pad (non serve via-in-pad) |

## Assemblaggio (PCBWay Assembly, turnkey)

- **Solo lato TOP.** Tutti i componenti sono su un lato.
- Componenti SMD e THT (morsetti, portafusibili, elettrolitici, USB-C, box header, JST-XH): il file BOM indica il tipo.
- **Non montare (DNP):** J5 (header ESP32-C6), R6, R7 (pull-up I2C: ci sono già sul display). La BOM li riporta con quantità 0.
- **Fusibili:** sono le ultime due voci della BOM (Littelfuse 0297015.WXNV × 3, 0297007.WXNV × 1). PCBWay li fornisce
  e li **inserisce** nei portafusibili F1–F4 dopo la saldatura (tipo *Hand insert*, nessuna saldatura).
  Nel campo note dell'ordine scrivere anche: *"Please insert the 4 mini blade fuses into holders F1–F4 (F1–F3 15 A, F4 7.5 A)."*
- Passivi generici (R 1 %, C X7R/X5R): sono ammessi equivalenti con tensione ≥ di quella indicata.

## Cosa collegare ai morsetti

| Morsetto | Cosa | Note |
|---|---|---|
| J1 `12V HEAT IN` | LRS-350-12, coppia +V/−V n. 1 | **− sopra, + sotto**; solo riscaldatori, fino a 16,6 A |
| J2 `12V MAIN IN` | LRS-350-12, coppia +V/−V n. 2 | **− sopra, + sotto** (stesso ordine di J1); pompa, valvole, motore, logica |
| J18 / J19 `HEATER 1/2` | riscaldatori 100 W | non polarizzati |
| J20 `PUMP` | Marco UP3-R | il verso si inverte da firmware (*Inverti pompa*) |
| J21 `MOTOR` | motore spirale JGB37-520 | |
| J14…J17 | 8 elettrovalvole 12 V (2 poli ciascuna) | C1, C2, C3, WB, scarico, ingresso, riserva A6, A7 — non polarizzate |
| J6 `BATH LEVEL` | XKC-Y21 MIN, MAX del bagno | JST-XH 4 poli: 5 V, MIN, MAX, GND |
| J7…J9 `CHEM 1/2/3` | XKC-Y21 MIN, MAX di ogni chimica | JST-XH 4 poli |
| J10 `FLOW` · J11 `HALL` | YF-S201 · KY-003 | JST-XH 3 poli: 5 V, segnale, GND |
| J12 / J13 `TEMP 1/2` | DS18B20 bagno / chimica | JST-XH 3 poli: 3,3 V, DQ, GND |
| J22 `SERVO` | MG90S della taglierina | header 2,54: segnale, +5 V, GND |
| J4 `DISPLAY JP1` | flat 2×13 verso JP1 del JC4880P433 | pin 1 = pin 1; porta segnali **e** alimentazione del display |
| J3 `USB-C 5V OUT` | **uscita** 5 V per un eventuale accessorio (cavo USB-C) | solo alimentazione, corrente USB standard (500–900 mA); il 5 V si accende solo quando dall'altra parte c'è un dispositivo da alimentare; non serve al display |

## Verifiche fatte

- ERC dello schema: 0 violazioni; netlist dello schema identica pin per pin a `design.py` (223 componenti, 125 net); PCB identico pin per pin.
- DRC della scheda (regole 0,2/0,2 mm): 0 errori, 0 collegamenti mancanti; restano solo avvisi di serigrafia
  (testi che si toccano), che non riguardano la produzione.
- Revisione indipendente: mappa di JP1 e dell'MCP23017 confrontata con il firmware, pinout di ogni integrato
  e transistor confrontati con i datasheet, stati all'accensione, valori di buck, ADC e limiti di corrente.
  Seconda revisione indipendente sulle modifiche di questa versione (display alimentato da JP1, USB-C di uscita,
  fusibili inseriti, firmware): tutti i casi della USB-C ricontrollati, esito «pronta da ordinare».
- Da misurare alla prima accensione, col display collegato e il suo cavo USB-C staccato: la tensione sul VBUS della
  USB-C del display (dice se l'IP5306 lascia passare il 5 V all'indietro; è innocuo, ma è bene saperlo).
- Firmware: uno solo per moduli separati e Mainboard (vedi `NOTE_firmware.md`).
- **Non è stata provata dal vero.** Nessuno può garantire una scheda nuova senza averla accesa: ordinare il lotto
  minimo (5 pezzi) e alla prima accensione seguire l'ordine: solo J2 senza carichi (LED verdi 12 V / 5 V / 3,3 V)
  → flat del display → valvole → motore → pompa → J1 e riscaldatori. Sulla scheda non c'è niente da saldare o
  ponticellare.
