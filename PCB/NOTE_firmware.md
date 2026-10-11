# Firmware e FilMachine Mainboard v1.0

Il firmware è **uno solo** e funziona uguale con i moduli separati (Adafruit #6318, DBH-12V, modulo IRF540)
e con la Mainboard: stessi GPIO su JP1, stessa mappa dell'MCP23017, stessa logica ENA/IN1/IN2 del DBH-12V.
Non c'è niente da cambiare, ricompilare o impostare quando si passa da una configurazione all'altra.

## Modifiche fatte (`PCB/firmware_mainboard.patch`, già applicata alla cartella, non committata)

| Cosa | Perché | Moduli separati | Mainboard |
|---|---|---|---|
| `INLET_RLY 5`: A5 configurato come uscita e chiuso all'avvio, chiuso anche da `closeAllValves()` | prima A5 restava un ingresso e la valvola dell'ingresso acqua non si poteva aprire | A5 = canale 6 del #6318 | A5 = J16 pin 3–4 |
| Riempimento del bagno dalla rete: apre `INLET_RLY` invece di `WB_RLY` | nella v6 WB collega il bagno alla pompa, l'acqua di rete ha la sua valvola (note v6, 1.1 A) | uguale | uguale |
| Controllo del sensore MIN a **12 l entrati** invece che a 20 s; timeout 20 min invece di 10 | il bagno v6 (12,8 l) bagna il MIN dopo ~9,3 l (note v6, 1.1 B); 12 l lascia margine a un flussimetro che legge fino al 25 % in più | uguale | uguale |
| Due sicurezze nuove nel riempimento: stop se entrano **16 l** (`FILL_TIMEOUT`) e se il flusso si ferma per **10 s** a valvola aperta (`FILL_NOFLOW`) | se il sensore MAX si guastasse, con 20 minuti di timeout potrebbero passare 60–100 l dal troppopieno; se si chiude il rubinetto la valvola non resta aperta | uguale | uguale |
| Driver MCP3424 + pannello *Power rails* nella diagnostica: 12 V, 5 V, corrente pompa | sulla Mainboard non restano GPIO per gli ADC del P4 | nessuno risponde a 0x68: il pannello mostra «n/a», nient'altro cambia | valori dal vivo |

Il vecchio codice `HAS_RAIL_MONITOR` con gli ADC del P4 è stato tolto: gli ADC1 del P4 sono sui GPIO 16–23,
che sulla JC4880P433 sono l'SDIO verso l'ESP32-C6 (Wi-Fi); attivarlo avrebbe rotto il Wi-Fi.

L'MCP3424 viene cercato **una volta** all'avvio, sul bus I2C, dopo l'MCP23017 e prima del reset del bus che precede il
touch. Viene letto solo mentre la pagina di diagnostica è aperta: un canale ogni 350 ms, senza mai aspettare la
conversione. Nel resto del funzionamento non c'è nessun traffico I2C in più.

A6 e A7 non vengono toccati: sulla Mainboard sono le uscite di riserva su J17 (spente dal pull-down sul gate),
sul #6318 restano come oggi. `HEATER_RLY` (A7) è usato solo se `HAS_DUAL_HEATER` è 0, e non è il nostro caso.

Gli stati del riempimento sono quelli di prima: display e app non cambiano.

**Verifiche:** build ESP-IDF 5.5.1 per esp32p4 riuscita, nessun avviso nei file toccati; simulatore compilato;
198 test su 198 passati.

**Non toccato:** il trasferimento a due tempi di `cleanRelayManager()` (note v6, punto 4). Riguarda l'impianto
idraulico, non la scheda.

## Corrente pompa (CH3/CH4 dell'MCP3424)

I ≈ (V / 1,5 kΩ − 0,17 mA) × 19 500, il maggiore dei due lati (conduce solo il lato alto acceso). Il valore è mediato
sul PWM, quindi è la corrente presa dal 12 V, non quella nel motore. È da tarare a banco: le costanti sono
`RAIL_PUMP_*` in `boards/board_jc4880p433.h`.

## Comportamenti diversi dal DBH-12V (nessuna modifica necessaria)

- **Pompa ferma = frenata**, non ruota libera (BTN8982: con IN basso è acceso il lato basso). Per una pompa a
  ingranaggi non cambia nulla.
- PWM pompa a 16 kHz: va bene per il BTN8982 (slew rate al massimo, SR a massa).
- Motore spirale su DRV8871 con limite di corrente a 2,1 A: se il motore si blocca la corrente viene limitata;
  il rilevamento di stallo via sensore Hall continua a funzionare uguale.
- Il limite "duty max 98 %" del DBH-12V non serve più, ma può restare.

## Alimentazione del display

Il display si alimenta **dal flat 2×13**, pin 2 e 4 di JP1 (`VCC5V` del JC4880P433), attraverso un PTC da 1,1 A (F7)
e un diodo Schottky (D8). Il diodo fa due cose:
- il display non può mai alimentare all'indietro il 5 V della scheda;
- se si collega la USB-C del display al PC per programmarlo con la macchina accesa, il PC (5 V) sta circa 0,4 V
  sopra la scheda (≈ 4,6 V) e prevale: le due alimentazioni non si contrastano e nessuna corrente va verso il PC.

La **USB-C J3** è solo un'**uscita** 5 V di servizio per un accessorio, senza dati, con corrente USB standard (Default USB
power, 500–900 mA; PTC F5). Il display non ne ha bisogno. Il 5 V si accende solo quando il dispositivo collegato si
presenta come utilizzatore (Rd sulla CC), come vuole la specifica USB-C. Un comparatore (U11) confronta la CC con una
soglia, e due P-MOS in antiserie (Q11/Q12) fanno da interruttore con avvio dolce di circa 1 ms. Con un cavo verso un altro
alimentatore o verso un PC, a scheda accesa o spenta, la porta resta bloccata nei due versi.

Il 5 V della scheda è a **5,12 V** (prima 5,02 V), per lasciare margine alla caduta su PTC e MOSFET della USB-C e al diodo
del display.
