# Note per il firmware (FilMachine_Reworked)

Cosa funziona così com'è con questa macchina, cosa no e qual è la modifica più piccola possibile.
Ho letto il codice del repository (`main/accessories.c`, `c_pages/page_checkup.c`, i popup in `c_elements/`,
`boards/board_jc4880p433.h`); **non ho potuto provarlo su una macchina vera**.

## 1. Collegamenti

I pin sono quelli di `boards/board_jc4880p433.h`; l'unico cambiamento è GPIO28, ora dedicato al servo della taglierina. Lo schema è in `schema_elettrico.pdf`.

| Funzione | Pin | Dove va |
|---|---|---|
| Motore spirale: ENA / IN1 / IN2 | GPIO32 / 33 / 34 (JP1 19 / 8 / 17) | ponte H, canale A |
| Pompa: ENA / IN1 / IN2 | GPIO51 / 49 / 50 (JP1 9 / 13 / 11) | ponte H, canale B |
| I2C SDA / SCL | GPIO7 / 8 (JP1 23 / 25) | driver Adafruit #6318 (MCP23017, 0x20) |
| Elettrovalvole C1, C2, C3, WB, scarico | MCP23017 A0…A4 | uscite MOSFET del driver (sul collettore le valvole stanno nell'ordine C1, WB, C2, C3, scarico: conta il collegamento, non la posizione) |
| Riscaldatori 1 e 2 | MCP23017 B6, B7 | due moduli MOSFET |
| Sonde DS18B20 (0 = bagno, 1 = chimica) | GPIO35 (JP1 15) | bus OneWire |
| Livello bagno MIN / MAX | GPIO29 / 30 (JP1 14 / 12) | due XKC-Y21 sulla parete del bagno |
| Sensore Hall | GPIO31 (JP1 10) | KY-003 sulla ruota magneti (4 magneti) |
| Servo della taglierina | GPIO28 (JP1 21, ora `CUTTER_SERVO_PIN`) | MG90S, LEDC timer 3 / canale 3 a 50 Hz; alimentazione 5 V dal convertitore, massa comune, 10 kΩ verso massa sul segnale |

Non collegati su questa macchina: i sei ingressi B0…B5 (livelli delle vaschette) e il flussimetro (GPIO52).
Con il servo su GPIO28 non resta nessun pin libero su JP1; se servisse, GPIO52 si libera solo cambiando il codice del riempimento.

## 2. Cosa funziona senza toccare il codice

- **Tutto il processo di sviluppo**: riempimento e svuotamento della tank (`sendValueToRelay()` apre una valvola e
  fa girare la pompa in un verso), ritorno della chimica nella sua vaschetta, scarto, agitazione, controllo della
  temperatura con i due riscaldatori, consenso del sensore Hall alla partenza.
- **Risciacqui con l'acqua del bagno** (passi con sorgente `WB`, sempre scartati).
- **Riempimento manuale del bagno**: impostazione *Ingresso acqua* spenta.
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
| Svuota contenitori (popup) | Cx + scarico aperte, pompa IN | manda la chimica nella tank, che ne tiene mezzo litro | **non avviarla**; non serve: le vaschette si sollevano e si versano; il bagno si svuota dal rubinetto manuale |
| Autotest, fasi di flusso (WB→Cx, Cx→WB, WB→scarico) | due valvole + pompa | flussi diversi da quelli attesi | saltare queste fasi oppure applicare la modifica del punto 4 |
| Riempimento vaschetta da WB | WB + C1, pompa IN, sensori di livello | non riempie la vaschetta: riempie la tank, e senza sensori non si ferma | **non avviarlo**; non serve: le vaschette si riempiono a mano |
| Riempimento macchina con flussimetro | apre WB senza pompa (acqua in pressione) | non c'è ingresso in pressione | lasciare *Ingresso acqua* spento |

I sensori di livello delle vaschette (`chemLevelMinDetected()` / `chemLevelMaxDetected()`) sono letti solo da queste
funzioni di manutenzione e dalla pagina di diagnostica: senza sensori il processo non cambia, restano grigi i sei
pallini nella diagnostica.

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

Il caricamento c'è nel firmware e nell'app. Le modifiche sono nei due repository locali, **non ancora committate**.
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

## 6. Riscaldatori: una cosa da cambiare nell'hardware

Ogni riscaldatore assorbe 8,3 A. Il modulo IRF540 in distinta, pilotato così, dissipa circa 5 W per canale e senza
dissipatore si surriscalda. Nella nuova distinta ci sono due moduli MOSFET da 15 A a bassa resistenza: stessi
segnali (B6 e B7 alti = acceso), nessuna modifica al codice.
