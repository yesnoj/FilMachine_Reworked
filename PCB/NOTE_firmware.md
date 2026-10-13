# Firmware e FilMachine Mainboard v1.1

Il firmware è **uno solo** e funziona uguale con i moduli separati (Adafruit #6318, DBH-12V, modulo IRF540)
e con la Mainboard: stessi GPIO su JP1, stessa mappa dell'MCP23017, stessa logica ENA/IN1/IN2 del DBH-12V.
Non c'è niente da cambiare, ricompilare o impostare quando si passa da una configurazione all'altra.

## Modifiche fatte (`PCB/firmware_mainboard.patch`, già applicata alla cartella, non committata)

| Cosa | Perché | Moduli separati | Mainboard |
|---|---|---|---|
| `INLET_RLY 5`: A5 configurato come uscita e chiuso all'avvio, chiuso anche da `closeAllValves()` | prima A5 restava un ingresso e la valvola dell'ingresso acqua non si poteva aprire | A5 = canale 6 del #6318 | A5 = J20 `VALVE INLET` |
| Riempimento del bagno dalla rete: apre `INLET_RLY` invece di `WB_RLY` | nella v6 WB collega il bagno alla pompa, l'acqua di rete ha la sua valvola (note v6, 1.1 A) | uguale | uguale |
| Controllo del sensore MIN a **12 l entrati** invece che a 20 s; timeout 20 min invece di 10 | il bagno v6 (12,8 l) bagna il MIN dopo ~9,3 l (note v6, 1.1 B); 12 l lascia margine a un flussimetro che legge fino al 25 % in più | uguale | uguale |
| Due sicurezze nuove nel riempimento: stop se entrano **16 l** (`FILL_TIMEOUT`) e se il flusso si ferma per **10 s** a valvola aperta (`FILL_NOFLOW`) | se il sensore MAX si guastasse, con 20 minuti di timeout potrebbero passare 60–100 l dal troppopieno; se si chiude il rubinetto la valvola non resta aperta | uguale | uguale |
| Driver **ADS1115** (`drivers/ads1115.c`) + pannello *Power rails* nella diagnostica: 12 V, 5 V, corrente pompa | sulla Mainboard non restano GPIO per gli ADC del P4 | nessuno risponde a 0x48: il pannello mostra «n/a», nient'altro cambia | valori dal vivo |

Il vecchio codice `HAS_RAIL_MONITOR` con gli ADC del P4 è stato tolto: gli ADC1 del P4 sono sui GPIO 16–23,
che sulla JC4880P433 sono l'SDIO verso l'ESP32-C6 (Wi-Fi); attivarlo avrebbe rotto il Wi-Fi.

L'ADS1115 (U4, indirizzo 0x48, ADDR a massa) viene cercato **una volta** all'avvio, sul bus I2C, dopo l'MCP23017 e prima
del reset del bus che precede il touch; oltre al probe, il driver verifica che il registro di configurazione risponda
come un ADS1115, altrimenti il monitor resta spento. Viene letto solo mentre la pagina di diagnostica è aperta: un
canale ogni 350 ms, in single-shot a 128 SPS (±2,048 V, 62,5 µV/LSB), senza mai aspettare la conversione. Nel resto
del funzionamento non c'è nessun traffico I2C in più.

Nella versione precedente della scheda l'ADC era un MCP3424 a 0x68: il driver `mcp3424.c/.h` è stato tolto e
sostituito; indirizzo, registri e costanti sono in `boards/board_jc4880p433.h` (`RAIL_*`).

### A6 e A7 (pin liberi dell'MCP23017)

Il firmware non li tocca: all'avvio restano **ingressi** (stato di reset dell'MCP23017), il self-test delle valvole
cicla solo A0–A4, `HEATER_RLY` (A7) è usato solo se `HAS_DUAL_HEATER` è 0 (non è il nostro caso).
Sulla Mainboard sono disponibili in due forme, entrambe sullo stesso pin:
- **J21 / J22 `SPARE 1/2`**: uscite 12 V commutate (MOSFET low-side come le valvole, con diodo di ricircolo e LED);
  spente finché il pin è ingresso o basso (pull-down da 1 MΩ sul gate);
- **J30 `GPIO SPARE`** (header 2,54: A6, A7, 3V3, 5V, GND): gli stessi pin come GPIO a 3,3 V, attraverso 1 kΩ in serie
  e un diodo di clamp a 3,3 V (un segnale a 5 V non li danneggia). Come uscite, J30 segue il pin e la rispettiva SPARE
  si accende. Come ingressi: **un livello alto sul pin accende anche la SPARE corrispondente** (è lo stesso nodo, con
  pull-down da 1 MΩ sul gate), compreso il pull-up interno da 100 kΩ dell'MCP23017 — quindi leggerli senza pull-up
  interno, da una sorgente push-pull a 3,3 V (o accettare che la SPARE segua l'ingresso, se J21/J22 non è usata).
  Il 5 V di J30 è quello di servizio (PTC F5, condiviso con J29), non quello dei sensori.
Sul #6318 restano i canali MOSFET 7 e 8, come oggi.

Gli stati del riempimento sono quelli di prima: display e app non cambiano.

**Verifiche:** build ESP-IDF 5.5.1 per esp32p4 riuscita, nessun avviso nei file toccati; simulatore compilato;
tutti i test del simulatore passati (vedi il riepilogo nel report).

**Non toccato:** il trasferimento a due tempi di `cleanRelayManager()` (note v6, punto 4). Riguarda l'impianto
idraulico, non la scheda.

## Corrente pompa (AIN2/AIN3 dell'ADS1115)

I ≈ (V / 1,5 kΩ − 0,2 mA) × 10 000 (BTN8962TA: kILIS = 10 000, offset tipico 0,2 mA), il maggiore dei due lati
(conduce solo il lato alto acceso). Il valore è mediato sul PWM (10 k + 100 nF), quindi è la corrente presa dal 12 V,
non quella nel motore. È da tarare a banco se serve precisione: le costanti sono `RAIL_PUMP_*` in
`boards/board_jc4880p433.h`.

## Comportamenti diversi dal DBH-12V (nessuna modifica necessaria)

- **Pompa ferma = frenata**, non ruota libera (BTN8962: con IN basso è acceso il lato basso). Per una pompa a
  ingranaggi non cambia nulla.
- PWM pompa a 16 kHz: va bene per il BTN8962 (slew rate al massimo, SR a massa).
- Motore spirale su DRV8871 con limite di corrente a 2,1 A: se il motore si blocca la corrente viene limitata;
  il rilevamento di stallo via sensore Hall continua a funzionare uguale.
- Il limite "duty max 98 %" del DBH-12V non serve più, ma può restare.

## Alimentazione del display e 3,3 V della scheda

Il display si alimenta **dal flat 2×13**, pin 2 e 4 di JP1 (`VCC5V` del JC4880P433), attraverso un PTC da 1,1 A (F7)
e un diodo Schottky (D8). Il diodo fa due cose:
- il display non può mai alimentare all'indietro il 5 V della scheda;
- se si collega la USB-C del display al PC per programmarlo con la macchina accesa, il PC (5 V) sta circa 0,4 V
  sopra la scheda (≈ 4,7 V) e prevale: le due alimentazioni non si contrastano e nessuna corrente va verso il PC.

La logica della scheda (MCP23017, ADS1115, porte AND, pull-up, DS18B20) prende i **3,3 V dal display** (JP1 pin 1/3/18,
circa 15 mA). Vantaggi: con la scheda senza 12 V e il display su USB l'MCP23017 è comunque alimentato, quindi non
blocca mai l'I2C del touch; senza display collegato l'INH del ponte pompa resta basso (ponte inibito).
Il LED verde «3V3» si accende quando il display è collegato e acceso.

Il 5 V della scheda è a **5,17 V** (partitore 100 k / 24 k), per lasciare margine alla caduta su PTC e diodo verso il
display (≈ 4,7 V su JP1).
Il pull-up dell'INH del ponte pompa (R84) è da 1 kΩ: con la corrente d'ingresso massima dei due BTN8962 (2 × 150 µA a
5,3 V, cioè ~2 × 93 µA a 3,3 V) l'INH resta a 3,1 V, sopra la soglia massima di 2,15 V.
Il pin 18 di JP1 (`ESP_3V3`) non è collegato sulla scheda: i 3,3 V arrivano dai pin 1 e 3 (`VCC3.3`).
Le uscite di servizio J28 (12 V, PTC 2 A) e J29 (5 V, PTC 1,1 A) sono semplici morsetti a valle di un PTC: nessuna
logica, nessun firmware.
