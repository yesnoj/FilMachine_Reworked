# Note per il firmware (FilMachine_Reworked)

Cosa funziona così com'è con questa macchina, cosa no e qual è la modifica più piccola possibile.
Ho letto il codice del repository (`main/accessories.c`, `c_pages/page_checkup.c`, i popup in `c_elements/`,
`boards/board_jc4880p433.h`); **non ho potuto provarlo su una macchina vera**.

## 1. Collegamenti: nessuna modifica

I pin sono quelli già definiti in `boards/board_jc4880p433.h`. Lo schema è in `schema_elettrico.pdf`.

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
| Libero | GPIO28 (JP1 21, `TEST_PIN`) | servo della taglierina, vedi punto 5 |

Non collegati su questa macchina: i sei ingressi B0…B5 (livelli delle vaschette) e il flussimetro (GPIO52).

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

## 5. Caricamento della pellicola e taglierina: funzione nuova

Il caricatore è dentro la tank e usa il motore della spirale e il sensore Hall che ci sono già; in più c'è il servo
della taglierina. Nel firmware di oggi questa funzione non esiste: va aggiunta una pagina «Carica pellicola».

Sequenza proposta (valori di partenza, da tarare):

1. Servo a riposo (lama abbassata): manovella a 0°.
2. L'utente aggancia la clip alla coda della pellicola e chiude il coperchio.
3. Motore avanti a velocità bassa; contare gli impulsi Hall (4 per giro). Un 135 da 36 pose entra in circa 9 giri,
   un 120 in circa 5. Il rullino è finito quando gli impulsi Hall smettono di arrivare: la pellicola è trattenuta
   (dal rocchetto nel 135, dal nastro che la unisce alla carta nel 120), la spirale si ferma e la cinghia tonda
   slitta, perché fa da limitatore di coppia.
4. Fermare il motore; servo da 0° a 180° e ritorno (corsa della lama 12,8 mm, circa 1 s per verso): taglia sia il
   135 sia il 120. Poi mezzo giro per tirare dentro la coda.
5. Segnale acustico: la tank è pronta.

Collegamento del servo: segnale su GPIO28 (`TEST_PIN`, JP1 21), LEDC a 50 Hz, impulso 0,5–2,5 ms; alimentazione a 5 V
dal convertitore.

**Per provare senza scrivere codice**: motore dalla pagina di diagnostica e servo comandato da un «tester per servo»
da pochi euro (è nella nuova distinta come facoltativo).

## 6. Riscaldatori: una cosa da cambiare nell'hardware

Ogni riscaldatore assorbe 8,3 A. Il modulo IRF540 in distinta, pilotato così, dissipa circa 5 W per canale e senza
dissipatore si surriscalda. Nella nuova distinta ci sono due moduli MOSFET da 15 A a bassa resistenza: stessi
segnali (B6 e B7 alti = acceso), nessuna modifica al codice.
