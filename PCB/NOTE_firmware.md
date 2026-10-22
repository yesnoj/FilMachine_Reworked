# Firmware e FilMachine Mainboard v1.2

Il firmware è **uno solo** e funziona uguale con i moduli separati (Adafruit #6318, DBH-12V, modulo IRF540)
e con la Mainboard (v1.1 o v1.2): stessi GPIO su JP1, stessa mappa dell'MCP23017, stessa logica ENA/IN1/IN2 del
DBH-12V. Non c'è niente da cambiare, ricompilare o impostare quando si passa da una configurazione all'altra.

## Cosa cambia con la v1.2 (firmware e app aggiornati nelle cartelle, da committare)

La v1.2 toglie i **sensori di livello dei contenitori delle chimiche** (i livelli si controllano a vista; con la
peristaltica ogni strumento sposta un volume noto) e aggiunge il **blocco dei riscaldatori** sul sensore MIN della vasca.

| Cosa | Prima | Ora |
|---|---|---|
| MCP23017 B0–B5 | MIN/MAX dei 3 contenitori (`HAS_CHEM_LEVEL_SENSORS`, `CHEM*_PIN`) | GPIO liberi (`GPIO_SPARE_B_FIRST/COUNT`): ingressi con pull-up interno; sulla Mainboard v1.2 sono l'header **J30 `GPIO SPARE`** |
| «Riempi chimica» (Strumenti → Liquidi) | WB → C1 finché il MAX del C1 si bagnava | versa **`CHEM_CONTAINER_ML`** d'acqua della vasca nel C1 attraverso la tank (motore di trasferimento a volume, Stop compreso); il popup dice quanti ml e che **il C1 dev'essere vuoto** |
| «Svuota chimiche» (travaso nella bottiglia) | si fermava quando il MIN del contenitore sentiva aria | travasa `CHEM_CONTAINER_ML`; a contenitore vuoto la pompa sposta solo aria (innocuo) |
| Svuota macchina / Pulisci | sensori disattivati di default (`CLEAN_USE_LEVEL_SENSORS` 0) | a tempo per i contenitori; il WB può ancora fermarsi sul MIN della vasca |
| `CHEM_CONTAINER_ML` | 1000 | **500** (contenitori della macchina v7, meccanica del 20/10/2026) |
| Riscaldatori | accesi dal termostato senza guardare il livello | **blocco**: si accendono solo con il MIN della vasca bagnato (vedi sotto) |
| Diagnostica | pallini MIN/MAX C1–C3 | pallini **B0–B5** (stato di J30) e «Heat OK» (vasca sopra il MIN); il test manuale dei riscaldatori non li accende con la vasca sotto il MIN |
| WebSocket / app | — | campo `heaterBlocked` nello stato; l'app mostra «Riscaldamento in pausa: vasca sotto il MIN» e i nuovi testi di «Riempi chimica» |

### Blocco dei riscaldatori

`sim_setHeater(on)` (chiamato dal termostato della pagina di processo, da Stop e dalla WebSocket) ora è una **richiesta**:
le uscite B6/B7 la seguono solo se il sensore **MIN della vasca** (XKC-Y21 su JP1 pin 14 / GPIO29, LOW = acqua) legge
acqua. Il livello è campionato una volta al secondo:
- **2 campioni** asciutti di fila (1–2 s) → riscaldatori spenti subito, anche quelli accesi a mano in diagnostica;
- la richiesta del termostato resta, quindi ripartono da soli dopo **5 campioni** bagnati (4–5 s);
- ogni 5 s il blocco riscrive comunque lo stato delle uscite, così una scrittura I2C persa non lascia un riscaldatore
  acceso.

Nella pagina di processo, al posto di «Riscaldatore: ON/OFF» compare «Riscaldamento in pausa: vasca sotto il MIN»; se la
vasca non torna sopra il MIN, la fase «raggiungi temperatura» finisce con il suo timeout come prima. Il test manuale dei
riscaldatori in diagnostica passa dallo stesso blocco (`heaterDebugSet()`): rifiutato con la vasca sotto il MIN, spento
se la vasca scende.

Corretto anche un difetto che c'era già: a fine processo (ultimo scarico finito, Stop subito, Stop dopo lo step, Stop
dall'app) il termostato si fermava ma il riscaldatore poteva restare acceso fino a «Chiudi». Ora ogni uscita dal
processo spegne i riscaldatori.

Il sensore MIN va montato **sopra i riscaldatori**. Se è scollegato o rotto legge «asciutto» (pull-up): la macchina non
scalda. È voluto: un riscaldatore da 100 W fuori dall'acqua si brucia in pochi secondi.

### A6 e A7 (J21 / J22)

Con il firmware v7 sono **uscite**: A6 = valvola di recupero (J21 `SPARE OUT 1`), A7 = pompetta di ricircolo della vasca
(J22 `SPARE OUT 2`). Sulla v1.2 hanno il pull-down del gate da 100 kΩ come le valvole (nella v1.1 era 1 MΩ perché A6/A7
uscivano anche su J30). Sul #6318 sono i canali MOSFET 7 e 8.

### J30 `GPIO SPARE` (B0–B5)

Header 2,54 a 9 poli: **B0, B1, B2, B3, B4, B5, 3V3, 5V, GND** (pin 1 = B0, a sinistra). Ogni pin passa da 1 kΩ in serie
con un diodo di clamp ai 3,3 V: un segnale a 5 V non fa danni, ma niente 12 V. Il firmware li lascia **ingressi con
pull-up interno** (100 kΩ): un pulsante o un contatto verso GND si legge subito con `mcp23017_digitalRead(&mcp, 8 + n)`;
per usarli come uscite basta `mcp23017_pinMode(..., MCP23017_OUTPUT)`. Il 3V3 viene dal display (pochi mA), il 5 V è
quello di servizio (PTC F5, condiviso con J29). Con i moduli separati sono i B0–B5 del #6318.

### ADS1115 (monitor tensioni e corrente pompa)

L'ADS1115 (U4, indirizzo 0x48, ADDR a massa) viene cercato **una volta** all'avvio, sul bus I2C, dopo l'MCP23017 e prima
del reset del bus che precede il touch; oltre al probe, il driver verifica che il registro di configurazione risponda
come un ADS1115, altrimenti il monitor resta spento. Viene letto solo mentre la pagina di diagnostica è aperta: un
canale ogni 350 ms, in single-shot a 128 SPS (±2,048 V, 62,5 µV/LSB), senza mai aspettare la conversione. Nel resto
del funzionamento non c'è nessun traffico I2C in più. Con i moduli separati nessuno risponde a 0x48 e il pannello
*Power rails* mostra «n/a».

**Verifiche (v1.2):** simulatore compilato, **208 test su 208** passati (3 nuovi: blocco dei riscaldatori, «Riempi
chimica» a volume, porta B; suite eseguita 3 volte di fila senza errori); build ESP-IDF 5.5.1 per esp32p4 riuscita.

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
