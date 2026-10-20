# Note per il firmware (FilMachine_Reworked) – macchina v7

Modifiche fatte al firmware per la macchina v7 (pompa peristaltica Kamoer KPHM600, uscita di recupero, pompetta di
ricircolo del bagno). Lavoro fatto nella cartella locale del repo sul Mac (`~/Documents/GitHub/FilMachine_Reworked`),
**non committato**: si committa dopo la prova sulla scheda. Provato nel simulatore e con i test automatici (205 test,
tutti passati, 7 nuovi); la build ESP-IDF per la scheda e' descritta in fondo.

## 0. Cosa cambia, in breve

| Cosa | Prima | Ora |
|---|---|---|
| Pompa | 15 l/min (tempi fissi in tabella) | Kamoer 600 ml/min: i tempi di carico e scarico si calcolano dal volume e dalla velocita' (350 ml = 35 s al 100 %) |
| Velocita' pompa | default 30 %, duty 150–250 | default **100 %**, duty lineare 0–250 con minimo 64 (il motoriduttore parte) |
| Dopo ogni scarico | la linea restava piena di chimica | la pompa gira ancora `PUMP_PURGE_SECS` (4 s al 100 %) ad aria: la linea si svuota nel contenitore (o nello scarico) da cui/verso cui si stava scaricando |
| Lavaggio linea (opzione) | WB aperta + pompa OUT (sbagliato con un solo collettore) | acqua della vasca nella tank per `lineRinseTime` secondi, poi tank → scarico + aria |
| Strumenti che muovono liquido tra due porte (pulizia, svuota macchina, riempi chimica) | due valvole aperte insieme (non funziona: tutto finiva nella tank) | **trasferimento a cicli attraverso la tank**: riempi 300 ml dalla sorgente, svuota verso la destinazione + aria, ripeti |
| Uscita di recupero | — | valvola **A6** (Mainboard J21 SPARE OUT 1), sesta valvola del collettore |
| Pompetta di ricircolo del bagno | — | **A7** (Mainboard J22 SPARE OUT 2), accesa solo mentre un processo scalda o sviluppa |
| Tools | 6 righe in Manutenzione | Manutenzione = Liquidi (sottomenu: Riempi vasca, Riempi chimica, Svuota chimiche, Svuota macchina, Pulisci macchina), Carica pellicola, Autodiagnosi; in «Svuota chimiche» si sceglie C1, C2 o C3, poi l'animazione (sorgente → tank → bottiglia) |
| Impostazioni | titolo «Processo» ~10 px più in basso | allineato a Processi e Tools (`SETTINGS_SCROLL_TOP` −22) |
| Diagnostica | 5 valvole | 6 valvole (+ Rec) e il pulsante Circ |

## 1. Schema idraulico (quello della macchina v7)

Un solo collettore sul lato aspirazione della pompa, con **sei** valvole: C1, C2, C3, WB (pescante del bagno), SCARICO,
**RECUPERO** (A6, tubo fino al fronte dove si mette la bottiglia). INGRESSO (A5) sta sulla rete idrica e riempie il bagno
dal beccuccio, con il flussimetro. Dall'altra parte della pompa c'e' solo la tank. Quindi:

- il processo riempie e svuota solo la tank (`sendValueToRelay`, una valvola + verso della pompa): come prima;
- qualunque altro spostamento (C1 → bottiglia, WB → C1, C1 → scarico) **passa per la tank** a cicli da 300 ml
  (`XFER_CYCLE_ML`; la tank trabocca a ~500 ml). Ogni ciclo: riempi la tank dalla sorgente (30 s al 100 %), svuotala
  verso la destinazione (30 s) piu' 4 s d'aria che spingono fuori quello che resta nel tubo.

Sotto ogni contenitore c'e' il rubinetto manuale (dalla bocca laterale della camera di livello) per il travaso d'emergenza: non c'entra col firmware.

## 2. Collegamenti nuovi

| Funzione | Pin | Dove va |
|---|---|---|
| Valvola di recupero | MCP23017 **A6** | Mainboard J21 «SPARE OUT 1» (12 V, 2 A) |
| Pompetta di ricircolo 12 V | MCP23017 **A7** | Mainboard J22 «SPARE OUT 2» |
| Kamoer KPHM600 | ponte H canale B (J25 sulla Mainboard) | 12 V, 0,8 A; tubo PharMed 6,4 × 9,6 |

All'avvio A6 e A7 sono impostati come uscite basse. L'autotest delle valvole al boot fa scattare anche la valvola di
recupero dopo le cinque del collettore; l'ingresso dell'acqua (A5) no, apposta. La pompetta si accende con il
riscaldamento (fase «raggiungi temperatura») e durante lo sviluppo, si spegne a fine processo, a Stop, alla chiusura
della pagina e dalla diagnostica.

## 3. Svuota chimica nella bottiglia (Tools)

Una riga nella sezione Liquidi: «Svuota chimiche». Nel popup si sceglie il contenitore (C1, C2 o C3), con l'avviso di
mettere la bottiglia sotto l'uscita RECUPERO; poi l'animazione. Le quattro fasi (`element_recoverPopup.c`), tutte
trasferimenti attraverso la tank:

1. **Lavaggio della linea**: 100 ml d'acqua dal bagno (WB) → tank → scarico + aria.
2. **Scarto di testa**: i primi 30 ml della chimica → tank → scarico.
3. **Travaso**: Cx → tank → RECUPERO, a cicli, finche' il sensore MIN della linea di Cx sente aria (la camera di
   livello si svuota) oppure fino a `CHEM_CONTAINER_ML`.
4. **Lavaggio finale**: come la fase 1.

Dall'app: `{"cmd":"recover_start","container":0..2}`, `recover_stop`, `recover_close`; stato in `recoverToolState`,
`recoverToolPhase`, `recoverToolPct`, `recoverToolRemaining`, `recoverToolMovedMl`; piu' `xferPhase` e `bathCirculator`.
App (FilMachine_App): Strumenti → Liquidi come sul display, schermata «Svuota chimiche» (`recover_screen.dart`).

## 4. Tempi della Kamoer

- `PUMP_FLOW_ML_MIN` (600) nel file di board; `calculateFillTime(ml, %)` = ml / (600/60 × %/100) secondi.
- Processo: `tankFillSeconds(volume, tank)` dal volume in `tankSizesMl`: al 100 % 25/35 s per 250/350 ml.
- **Attenzione alla config salvata sulla SD**: `pumpSpeed` resta quello di prima (30 %) finche' non lo si porta a 100
  in Impostazioni → Velocita' pompa (o Ripristina predefiniti).

## 5. Ordine di prova a banco

1. Flash, poi diagnostica: Rec apre la valvola A6, Circ accende la pompetta.
2. Impostazioni → Velocita' pompa = 100. Tune: con il tubo in un misurino, 60 s devono dare circa 600 ml.
3. Un processo di prova con acqua: dopo ogni scarico la pompa deve girare 4 s a vuoto e il tubo verso il collettore restare vuoto.
4. Tools → Liquidi → Svuota chimiche, C1, con acqua colorata nel C1 e la bottiglia sotto RECUPERO.
5. Pulisci macchina e Svuota macchina muovono il liquido a cicli: la tank non deve mai traboccare (300 ml per ciclo).

## 6. Cosa chiede in piu' la meccanica del rilascio 7 (da fare dopo la prova a banco)

- **Contenitori da 500 ml**: `CHEM_CONTAINER_ML` da 1000 a **500** (limite del travaso in «Svuota chimiche») e volumi di
  processo limitati a **250 o 350 ml** (`tankSizesMl`: togliere o bloccare 550 e la tank «grande»): con 350 ml nel
  contenitore ne restano 150 di margine, con 550 si andrebbe a secco.
- **Sensore Hall**: nessuna modifica. Legge i 4 magneti indicatori del mozzo magnetico (4 impulsi/giro come prima) e
  quindi vede la spirale, non il motore: a fine rullino la pellicola trattenuta dal caricatore ferma la spirale, il giunto
  slitta sui poli e gli impulsi cessano → stallo (3 × intervallo medio, min 2,5 s) → taglio, come oggi. Il giunto fa anche
  da limitatore di coppia (≈0,12 N m stimati): se a banco slitta avvolgendo, magneti Ø10 x 3.
- **Riscaldatori 100 W in 3 litri**: la temperatura sale di ~0,8 °C/min a tutti e due accesi. Prevedere: tutti e due fino a
  2 °C dal set-point, poi uno solo (B6 e B7 sono indipendenti), isteresi stretta. Da tarare con la sonda nel bagno.
- **Sensori di livello del bagno** (MIN/MAX): ora sulla parete posteriore, fuori; `Riempi bagno` si ferma al MAX come prima.
  Il MIN e' a 28 mm dal fondo: i riscaldatori (asse a 18 mm) sono gia' coperti quando il MIN e' bagnato.
- **Secondo DS18B20** (chimica): non montato nella v7 (contenitori chiusi); il firmware deve tollerare la sonda assente.
- **Tempera della tank**: non serve piu' (la tank sta nel bagno): niente fase di pre-riscaldamento.
