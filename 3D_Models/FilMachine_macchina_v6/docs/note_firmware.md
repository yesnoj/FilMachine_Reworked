# Note per il firmware (FilMachine_Reworked) – macchina v6

Cosa funziona così com'è con la macchina v6, cosa no e quali sono le modifiche più piccole possibili.
Ho riletto il codice di `origin/main` (commit `2abba80`: `main/accessories.c`, `drivers/sensors.c`, i popup in
`c_elements/`, `boards/board_jc4880p433.h`); **non ho potuto provarlo su una macchina vera** e non ho modificato il firmware.

## 0. Cosa cambia con la v6, in breve

| Cosa | v5 | v6 | Firmware |
|---|---|---|---|
| Ingresso dell'acqua di rete | non c'era | sesta valvola (ingresso) + flussimetro YF-S201 + beccuccio sopra il bagno | **modifica piccola** (punto 1.1): oggi il riempimento apre WB_RLY |
| Flussimetro (GPIO52) | non usato | usato, sulla linea dell'ingresso | nessuna: c'è già (`HAS_FLOW_METER`, 450 impulsi/l) |
| Sensori delle chimiche B0–B5 | non montati | MIN nella camera di livello C15 di ogni linea, MAX sulla parete della vaschetta | nessuna per leggerli; semantica del MIN al punto 1.2 |
| Pescaggio delle vaschette | testina e sifone sul bordo | innesto dal basso a tenuta, linea a ponte sopra il bordo del bagno | nessuna: idraulicamente è la stessa linea |
| Bagno | 7,4 l | **12,8 l**, MIN a 110 e MAX a 168 mm dal piano d'appoggio | il riempimento va adattato (punto 1.1) |
| Svuotamento | — | rubinetto manuale del bagno; imbuto C20 per riversare una chimica nella bottiglia | nessuna |
| Riscaldatori | modulo da sostituire | resta il modulo IRF540, con dissipatori | nessuna |

## 1. Collegamenti

I pin sono quelli di `boards/board_jc4880p433.h`; GPIO28 resta il servo della taglierina. L'unica uscita nuova è
**MCP23017 A5** per la valvola dell'ingresso. Lo schema è in `schema_elettrico.pdf`.

| Funzione | Pin | Dove va |
|---|---|---|
| Motore spirale: ENA / IN1 / IN2 | GPIO32 / 33 / 34 (JP1 19 / 8 / 17) | ponte H, canale A |
| Pompa: ENA / IN1 / IN2 | GPIO51 / 49 / 50 (JP1 9 / 13 / 11) | ponte H, canale B |
| I2C SDA / SCL | GPIO7 / 8 (JP1 23 / 25) | driver Adafruit #6318 (MCP23017, 0x20) |
| Elettrovalvole C1, C2, C3, WB, scarico | MCP23017 A0…A4 | uscite MOSFET del driver (sul collettore le valvole stanno nell'ordine C1, WB, C2, C3, scarico: conta il collegamento, non la posizione) |
| **Valvola dell'ingresso dell'acqua** | **MCP23017 A5** (oggi libero) | sesta elettrovalvola, sotto il piano del cavalletto |
| Sensori delle chimiche MIN / MAX | MCP23017 B0/B1 (C1), B2/B3 (C2), B4/B5 (C3) | MIN: XKC-Y21 sulla camera di livello C15; MAX: XKC-Y21 nel portasensore C04 |
| Riscaldatori 1 e 2 | MCP23017 B6, B7 | modulo IRF540 (due canali) |
| Sonde DS18B20 (0 = bagno, 1 = chimica) | GPIO35 (JP1 15) | bus OneWire |
| Livello bagno MIN / MAX | GPIO29 / 30 (JP1 14 / 12) | due XKC-Y21 sulla parete anteriore del bagno |
| Flussimetro YF-S201 | GPIO52 (JP1 7) | linea dell'ingresso, subito dopo il passaparete |
| Sensore Hall | GPIO31 (JP1 10) | KY-003 sulla ruota magneti (4 magneti) |
| Servo della taglierina | GPIO28 (JP1 21, `CUTTER_SERVO_PIN`) | MG90S |

### 1.1 Riempimento del bagno dalla rete: due modifiche

Il riempimento (`machine_fill_task()` in `main/accessories.c`) è scritto per un ingresso in pressione collegato alla
valvola WB: apre `WB_RLY`, conta l'acqua col flussimetro e si ferma sul sensore MAX. Nella v6 la valvola WB collega il
pescante del bagno alla pompa (serve per i risciacqui), mentre l'acqua di rete passa da una valvola sua.

**Modifica A – valvola dedicata** (due righe e una definizione):

```c
/* boards/board_jc4880p433.h, sotto WASTE_RLY */
#define INLET_RLY                   5       /* Port A pin 5 - valvola dell'ingresso dell'acqua di rete */

/* main/accessories.c, machine_fill_task(): al posto di WB_RLY, in apertura e in chiusura */
mcp23017_digitalWrite(&mcp, INLET_RLY, 1);          /* apre l'ingresso dell'acqua */
...
mcp23017_digitalWrite(&mcp, INLET_RLY, 0);          /* chiude l'ingresso          */
```

`RELAY_NUMBER` resta 5 (i cicli che chiudono «tutte le valvole» arrivano ad A4): conviene aggiungere una chiusura
esplicita di `INLET_RLY` all'avvio, accanto a quella dei riscaldatori, e nel punto in cui si fermano tutte le uscite.

**Modifica B – controllo del sensore MIN a volume, non a tempo.** Oggi, se il sensore MIN non si bagna entro 20 s
dall'inizio del flusso (`FILL_MIN_CONFIRM_MS`), il riempimento si ferma con `FILL_NOLEVEL`. Nella v6, a bagno vuoto,
servono circa **9 litri** prima che l'acqua arrivi al MIN (110 mm): con un rubinetto da 3–5 l/min sono 2–3 minuti.
Meglio legare il controllo all'acqua già entrata, che il flussimetro misura:

```c
#define FILL_MIN_CONFIRM_ML   11000    /* a bagno vuoto il MIN si bagna dopo circa 9,3 l */
...
if (!minConfirmed && s_fillVolumeMl >= FILL_MIN_CONFIRM_ML) { s_fillState = FILL_NOLEVEL; break; }
```

E alzare `MACHINE_FILL_TIMEOUT_MS` (oggi 10 minuti, pensato per 5 l) a **20 minuti**: il bagno ora tiene 12,8 l fino
al livello di lavoro. Il resto (flussimetro che conferma l'arrivo dell'acqua entro 4 s, stop sul MAX, Stop manuale)
va bene così. Il MAX del bagno è a 168 mm e il troppopieno comincia a 174 mm: se il MAX non scatta, l'acqua esce dal
troppopieno, non dal bagno. Il beccuccio sta 26 mm sopra il troppopieno (salto d'aria: l'acqua del bagno non può
tornare nella rete).

Il riempimento di una **vaschetta** da WB (`FILL_TARGET_CHEM`, pompa WB → C1) non funziona con questo impianto, per lo
stesso motivo del punto 3: lasciarlo spento, o rifarlo col trasferimento del punto 4.

### 1.2 Sensori di livello delle chimiche: cosa significano

- **MAX** (B1, B3, B5): sensore sulla parete della vaschetta, sopra l'acqua del bagno (a 200 mm). Si bagna quando nella
  vaschetta ci sono circa **1,03 l**. Si legge con `chemLevelMaxDetected()`: va bene così.
- **MIN** (B0, B2, B4): sensore sulla **camera di livello C15**, un tratto di linea trasparente subito sopra la valvola.
  La linea è un sifone che scavalca il bordo del bagno: una volta piena resta piena (la valvola chiusa la tiene in
  basso, il liquido della vaschetta in alto). La camera si svuota quando la pompa, aspirando, trova la vaschetta vuota e
  tira aria: è il segnale «vaschetta vuota» più affidabile che si possa avere, perché arriva proprio quando finisce il
  liquido. Quando la chimica torna nella vaschetta (pompa al contrario) la camera si riempie di nuovo.
- **Attenzione**: con una vaschetta appena riempita a mano e la linea mai adescata, la camera è vuota e il MIN dice
  «vuota» anche se la vaschetta è piena. Quindi il MIN va letto come «la linea ha liquido», non come «la vaschetta ha
  liquido»: lo svuotamento (`element_drainPopup.c`, `!chemLevelMinDetected()` = vuota) lo usa già nel modo giusto. Per
  sapere se una vaschetta è piena prima del primo adescamento si usa il MAX.
- Con `CLEAN_USE_LEVEL_SENSORS` a 1 pulizia e svuotamento si fermano sui sensori invece che a tempo. Ha senso solo
  insieme alla modifica del punto 4 (trasferimento attraverso la tank).

## 2. Cosa funziona senza toccare il codice

- **Tutto il processo di sviluppo**: riempimento e svuotamento della tank (`sendValueToRelay()` apre una valvola e
  fa girare la pompa in un verso), ritorno della chimica nella sua vaschetta, scarto, agitazione, controllo della
  temperatura con i due riscaldatori, consenso del sensore Hall alla partenza.
- **Risciacqui con l'acqua del bagno** (passi con sorgente `WB`, sempre scartati).
- **Riempimento manuale del bagno**: impostazione *Ingresso acqua* spenta, si versa dalla tacca del coperchio
  trasparente. Con le modifiche del punto 1.1 si accende *Ingresso acqua* e il bagno si riempie da solo.
- **Volumi**: usare *Volume chimica = Basso* con *Tank = Piccola* (250 ml) o *Media* (350 ml). Con 250 ml il
  liquido arriva a 34 mm dal fondo, con 350 ml a 46 mm; l'asse della spirale è a 50 mm, il troppopieno a circa 500 ml.
  **Non usare 550 ml né il volume Alto.** Il troppopieno smaltisce circa 0,4 l/min, mentre il riempimento ne porta
  quasi 2 (250 ml in 8 s): l'eccesso non farebbe in tempo a uscire e passerebbe nel vano del caricatore. Conviene
  togliere dal menu, o bloccare, le combinazioni oltre 350 ml.
- **Tempi di riempimento**: rifare la taratura dei tempi (chimica e WB) con la velocità pompa scelta. Tenere la
  pompa bassa: è molto più grande di quanto serve.

## 3. Cosa non funziona con una pompa e un collettore unico

Il codice usa due primitive. `sendValueToRelay(valvola, verso)` apre **una** valvola: funziona, perché la pompa sta tra
il collettore e la tank. `cleanRelayManager(da, a, verso)` apre **due** valvole insieme e fa girare la pompa: i due
recipienti sono però dalla stessa parte della pompa, quindi il liquido non va dall'uno all'altro ma finisce (o viene
preso) nella tank. Il commento in `handleLineRinse()` lo anticipa: la combinazione valvole/pompa «dipende
dall'impianto reale».

| Funzione | Cosa fa il codice | Cosa succede qui | Cosa fare |
|---|---|---|---|
| Risciacquo linea (`lineRinseEnabled`) | WB aperta + pompa OUT | spinge verso il bagno quello che resta nel tubo della tank | spegnerla; mettere nel processo un passo WB breve dopo ogni chimica (stesso effetto) |
| Pulizia (popup) | WB + scarico aperte, pompa IN | riempie la tank senza fermarsi: il troppopieno non basta, il liquido passa nel vano del caricatore | **non avviarla**; non serve: le vaschette si lavano al lavandino; per tubi e tank creare un processo «Pulizia» con 2–3 passi WB |
| Svuota contenitori (popup) | Cx + scarico aperte, pompa IN | manda la chimica nella tank, che ne tiene mezzo litro | **non avviarla** senza la modifica del punto 4; per riversare una chimica nella sua bottiglia si appoggia la vaschetta sull'imbuto C20 |
| Autotest, fasi di flusso (WB→Cx, Cx→WB, WB→scarico) | due valvole + pompa | flussi diversi da quelli attesi | saltare queste fasi oppure applicare la modifica del punto 4 |
| Riempimento vaschetta da WB | WB + C1, pompa IN, sensori di livello | non riempie la vaschetta: riempie la tank | **non avviarlo** senza la modifica del punto 4; le vaschette si riempiono a mano |
| Riempimento macchina con flussimetro | apre WB senza pompa (acqua in pressione) | WB porta alla pompa, non alla rete: non entra acqua, e dopo 4 s esce con *nessun flusso* | modifiche del punto 1.1 |
| Svuotamento del bagno | — | con WB + scarico aperte e pompa IN il bagno passa dalla tank: stesso problema | **rubinetto del bagno** sul pannello destro (manuale, svuota fino al fondo) |

I sensori delle chimiche sono letti solo da queste funzioni di manutenzione e dalla pagina di diagnostica: il
processo non li usa. Nella diagnostica i sei pallini ora si accendono (MIN dopo il primo adescamento, MAX con la
vaschetta piena).

## 4. Modifica minima consigliata: trasferire passando per la tank

Se si vogliono tenere pulizia, svuotamento e autotest, basta cambiare **solo** `cleanRelayManager()`: invece di aprire
due valvole insieme, alterna «riempi la tank dalla sorgente» e «svuota la tank nella destinazione» finché viene
fermata. La tank fa da misurino (250–350 ml per ciclo). Tutto il resto del codice (popup, tempi, interfaccia) resta com'è.

Traccia **non provata**, da adattare ai nomi reali:

```c
/* accessories.c - trasferimento a due tempi attraverso la tank */
#define XFER_FILL_S   8      /* secondi di riempimento della tank (taratura a 250 ml) */
#define XFER_DRAIN_S  10     /* secondi di svuotamento, un po' piu' lungo del riempimento */

static lv_timer_t *s_xferTimer = NULL;
static uint8_t s_xferFrom, s_xferTo, s_xferPhase;   /* fase 0 = riempie, 1 = svuota */
static uint16_t s_xferSecs;

static void xfer_timer_cb(lv_timer_t *t) {
    if (++s_xferSecs < (s_xferPhase == 0 ? XFER_FILL_S : XFER_DRAIN_S)) return;
    s_xferSecs = 0;
    pump_stop();
    mcp23017_digitalWrite(&mcp, s_xferPhase == 0 ? s_xferFrom : s_xferTo, 0);
    s_xferPhase ^= 1;
    mcp23017_digitalWrite(&mcp, s_xferPhase == 0 ? s_xferFrom : s_xferTo, 1);
    pump_run(s_xferPhase == 0, pumpSpeedToDuty());      /* true = IN (riempie), false = OUT (svuota) */
}

void cleanRelayManager(uint8_t pumpFrom, uint8_t pumpTo, uint8_t pumpDir, bool activePump) {
    (void)pumpDir;                                       /* il verso lo decide la fase */
    if (s_xferTimer) { lv_timer_delete(s_xferTimer); s_xferTimer = NULL; }
    pump_stop();
    for (uint8_t i = 0; i < RELAY_NUMBER; i++) mcp23017_digitalWrite(&mcp, i, 0);
    if (!activePump) {
        /* alla fine la tank puo' essere piena: svuotarla con sendValueToRelay(pumpTo, PUMP_OUT_RLY, ...) */
        return;
    }
    s_xferFrom = pumpFrom; s_xferTo = pumpTo; s_xferPhase = 0; s_xferSecs = 0;
    mcp23017_digitalWrite(&mcp, pumpFrom, 1);
    pump_run(true, pumpSpeedToDuty());
    s_xferTimer = lv_timer_create(xfer_timer_cb, 1000, NULL);
}
```

Da decidere a banco: i due tempi, e uno svuotamento finale della tank quando il trasferimento viene fermato a metà.
Per il risciacquo linea, la stessa idea in `handleLineRinse()`: `sendValueToRelay(WB, PUMP_IN)` per qualche secondo,
poi `sendValueToRelay(WASTE, PUMP_OUT)`.

## 5. Caricamento della pellicola e taglierina: implementato (da provare sulla macchina)

Il caricamento c'è nel firmware e nell'app (commit `90b7ef5` *Film Load*). La tank e il caricatore della v6 sono
quelli della v5: cambiano solo le due slitte della guida, ora bloccate da una vite zigrinata. Nessuna modifica al codice.
Il firmware compila per ESP32-P4 (ESP-IDF 5.5.1) senza avvisi nei file toccati; la logica è provata solo nel
simulatore e nei test automatici, non sulla scheda.

### Come funziona

| Fase | Cosa fa | Come finisce |
|---|---|---|
| Avvolgimento | motore della spirale avanti alla *velocità di caricamento* (40 % di default); conta gli impulsi Hall (4 per giro) | la spirale si ferma, oppure *Taglia ora*, oppure un errore |
| Taglio | servo da riposo a fine taglio in 0,9 s, fermo 0,3 s, ritorno in 0,6 s, poi il segnale si spegne | fine del movimento |
| Coda | mezzo giro (2 impulsi) per tirare dentro la coda | 2 impulsi oppure 4 s |
| Fatto | motore fermo, un bip sul display | — |

- **Fine del rullino**: la spirale è considerata ferma se non arriva un impulso per 3 volte l'intervallo medio degli
  ultimi impulsi, mai meno di 2,5 s. Funziona a qualunque velocità.
- **Taglio automatico**: il motore resta acceso, la cinghia slitta e tiene tesa la pellicola sulla lama.
  **Taglia ora** (per esempio quando nel 120 compare il nastro): la pellicola si sta ancora muovendo, quindi il motore
  si ferma per il taglio e riparte per la coda.
- **Protezioni** (motore fermo, lama a riposo, pellicola **non** tagliata):

| Errore | Quando | Valori di partenza |
|---|---|---|
| La spirale non gira | nessun impulso dopo l'avvio | 5 s |
| Ferma troppo presto | stallo prima dei giri minimi: clip sganciata o pellicola incastrata | 135: 4 giri, 120: 2 giri |
| Troppi giri | la fine della pellicola non è trattenuta | 135: 15 giri, 120: 9 giri |
| Troppo lungo | tempo massimo dell'avvolgimento | 180 s |

  Giri attesi, per la barra: 9 per il 135 × 36, 5 per il 120. Tutte le soglie sono in testa a `main/film_loader.c`.
- **Blocchi**: il caricamento non parte durante un processo; durante il caricamento vengono rifiutati l'avvio di un
  processo (`start_rejected`) e il test del motore da Tune.
- **Registro**: ogni caricamento scrive nel log (e quindi nel file di avvio sulla SD) formato, impulsi e giri:
  dopo qualche rullino vero si correggono le soglie.

### File

| File | Cosa contiene |
|---|---|
| `main/film_loader.c` | macchina a stati, sequenza del servo, task da 20 ms; nel simulatore un modello di spirale/Hall/servo |
| `drivers/servo.c`, `drivers/include/servo.h` | PWM a 50 Hz su LEDC timer 3 / canale 3 |
| `drivers/sensors.c` | contatore di impulsi Hall in interrupt (fronte di discesa, antirimbalzo 30 ms); `sensors_hall_magnet_detected()` resta com'era |
| `c_elements/element_loadPopup.c` | popup *Carica pellicola*: 135/120, stato, barra dei giri, Chiudi/Annulla · Prova lama/Taglia ora · Avvia/Stop |
| `c_pages/page_tools.c`, `main/ui_profile_800x480.inc` | nuova riga in Manutenzione; le sezioni sotto scendono di 57 px |
| `main/ws_server.c` | comandi e campi di stato (sotto) |
| `main/accessories.c`, `c_pages/page_settings.c` | tre impostazioni nuove salvate nel config; GPIO28 tenuto basso all'avvio |
| `boards/board_jc4880p433.h` | `TEST_PIN` → `CUTTER_SERVO_PIN` |
| `main/lang.c`, `lang.h`, `FilMachine.h`, `scripts/gen_lang.py` | 15 testi EN/IT |
| `tests/test_film_loader.c` | 11 test; il runner accetta `FM_TEST_ONLY=film_loader` (o una lista separata da virgole) |

### Impostazioni (config JSON, `set_setting`, *Ripristina predefiniti*)

| Chiave | Default | Limiti |
|---|---|---|
| `loadSpeed` | 40 % | 10–100 |
| `cutterRestUs` | 500 µs (manovella 0°) | 400–2600 |
| `cutterCutUs` | 2400 µs (manovella 180°) | 400–2600 |

### Protocollo WebSocket

```
{"cmd":"load_start","format":0|1}     0 = 135, 1 = 120; apre lo stesso popup sul display
{"cmd":"load_stop"}                    motore fermo, lama a riposo
{"cmd":"load_cut"}                     taglia ora (solo durante l'avvolgimento)
{"cmd":"cutter_test","angle":0..180}   tiene la manovella in posizione per 4 s (taratura)
{"cmd":"cutter_cycle"}                 prova lama: su e giù, motore fermo
```
Stato: `loadToolState` (0 fermo, 1 avvolgimento, 2 taglio, 3 coda, 4 fatto, 5 interrotto, 6–9 errori),
`loadToolFormat`, `loadToolPulses`, `loadToolExpected`, `loadToolServo`, più `loadSpeed`, `cutterRestUs`,
`cutterCutUs`. Eventi: `load_rejected` (`processing` o `busy`), `start_rejected` (`loading`).

### App

Strumenti → *Caricatore pellicola* apre la schermata *Caricamento pellicola*: scelta 135/120, istruzioni del formato,
stato con le stesse frasi del display, barra dei giri, e in *Taratura* la velocità di caricamento e i due fine corsa
del servo con il pulsante *Prova*. I pulsanti in basso sono gli stessi del display; uscire durante il caricamento
equivale ad Annulla. Testi del firmware rigenerati con `scripts/gen_app_strings.py`.

### Verifiche fatte

- Firmware: build ESP-IDF 5.5.1 per esp32p4 riuscita; simulatore e runner compilati; 11 test nuovi passati;
  131 test delle altre suite uguali a prima (le due suite che già fallivano, *Settings* e *Board constants*,
  falliscono allo stesso modo anche senza le modifiche).
- Popup provato nel simulatore in inglese e in italiano, compreso l'errore più lungo su tre righe.
- App: `flutter analyze` con gli stessi 18 avvisi di prima, nessuno nei file nuovi; 259 test passati (11 nuovi).

### Ordine di prova a banco

1. Flash, poi Strumenti → Carica pellicola → *Prova lama* senza servo collegato: nel log compare il movimento.
2. Servo collegato, senza lama: dall'app tarare `cutterRestUs` (lama sotto la fessura) e `cutterCutUs` (filo oltre il
   cielo della fessura sui due bordi) con *Prova*.
3. Conteggio Hall: avviare un caricamento a vuoto e far girare la spirale; i giri devono salire di 1 ogni 4 magneti.
4. Stallo: frenare la spirale con la mano dopo 5 giri → deve tagliare; dopo 2 giri → *Ferma troppo presto*.
5. Spezzone di pellicola di scarto agganciato a un rocchetto, poi un rullino vero; leggere nel log i giri e
   correggere le soglie.

## 6. Riscaldatori

Ogni riscaldatore assorbe 8,3 A. Come deciso resta il modulo IRF540: a quella corrente i due MOSFET scaldano (qualche
watt ciascuno), quindi vanno i dissipatori adesivi in distinta e il modulo sta sul ripiano basso, accanto alle griglie
d'aria del pannello sinistro; l'alimentatore LRS-350 ha la ventola che parte da sola col caldo. Stessi segnali (B6 e B7
alti = acceso), nessuna modifica al codice. Se il modulo scalda troppo alla prova, un modulo da 15 A a bassa resistenza
(tipo D4184) si monta al suo posto senza cambiare niente altro.

Il bagno è più grande (12,8 l): da 20 a 38 °C con 200 W servono circa 99 minuti, da 33 a 38 °C circa 28. Con
l'ingresso dell'acqua conviene riempire con acqua già tiepida dal miscelatore.
