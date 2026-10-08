# FilMachine – caricatore automatico della spirale (prototipo da banco v1)

Carica la pellicola sulla spirale **dal centro verso l'esterno**, come la Rondinax: un motore fa girare la
spirale, una fascetta con clip tira la pellicola fuori dal caricatore, un braccio guida la incurva e la posa
sulle costole, una taglierina la taglia a fine avvolgimento. Formati: **135 e 120** con la stessa spirale.

> **Cosa è e cosa non è.** È un banco prova *aperto* per validare la meccanica alla luce con pellicola di
> scarto. Non è ancora a tenuta di luce né di liquidi: quelle sono la fase successiva, da disegnare attorno
> a una meccanica che funziona.

## Contenuto

| Cartella | Cosa c'è |
|---|---|
| `step/` | 23 pezzi + 1 provino in STEP, già nell'orientamento di stampa, + `ASSIEME_135.step` e `ASSIEME_120.step` |
| `stl/` | gli stessi pezzi in STL |
| `png/` | anteprime e sezioni |
| `src/` | modello parametrico (Python + CadQuery): `python3 build_all.py` rigenera tutto |
| `verifiche.txt` | esito dei controlli geometrici dell'ultima generazione |

Gli assiemi contengono corpi con nome e colore: i pezzi usati in quel formato (19 nel 120; 21 nel 135, che ha
anche i due anelli alette) più le 4 sagome `REF_` (motore NEMA17, servo, lama, caricatore 135 o rocchetto 120). In Shapr3D ogni corpo si può nascondere.
Tutte le facce dei pezzi sono piani, cilindri e coni, quindi modificabili direttamente.

## Come funziona

1. **Spirale regolabile (pezzi 01–04).** Stesso principio della OpenReel/Paterson, adattato al carico dal
   centro: un tubo centrale con piste a baionetta, due flange che si infilano e si bloccano con 30° di
   rotazione in **due posizioni (35 mm e 120)**, un tamburo centrale con l'ancoraggio della fascetta e la
   tasca della clip. Le flange si spostano in modo simmetrico, così la pellicola resta centrata in entrambi
   i formati. Ø esterno 93,5 mm e foro del tubo Ø 26,3 mm, come rilevati dai tuoi STEP della OpenReel.
   Le costole non hanno imbocco esterno né cricchetto: la pellicola, in tensione, si appoggia con i bordi
   sul dorso delle costole (0,9 mm per lato), l'emulsione non tocca nulla.
   **Flange aperte**: mozzo, corona e 16 razze; tutto il resto del disco è vuoto, quindi il liquido entra tra
   i giri di pellicola da entrambe le facce (luce libera circa 2400 mm² per flangia, il 52 % della corona).
   La costola a spirale parte dal piatto di stampa, quindi non ci sono ponti.
   **Anelli alette (22_A e 22_B, speculari tra loro)**, solo per il 35 mm: si infilano sul tubo all'esterno
   delle flange e si bloccano nelle stazioni 120 della baionetta, che a 35 mm restano libere. Otto alette
   per lato, inclinate di 50° rispetto alla flangia come le ali della spirale 135 della Lab-Box; il piede di
   ogni aletta cade a metà di un vano tra due razze. A 120 non c'è spazio e non si montano.
   Si possono montare in due modi, a seconda di come gira la spirale durante lo sviluppo:
   - **rotazione alternata** (consigliato, è quello degli assiemi): **lo stesso anello sui due lati**, cioè
     22_A stampato due volte. Un lato spinge il liquido dentro e l'altro lo tira fuori, quindi il liquido
     attraversa la pellicola da un bordo all'altro; a ogni inversione i due lati si scambiano i ruoli.
   - **rotazione sempre nello stesso verso**: **anelli speculari**, 22_A sul lato motore e 22_B sul lato
     perno folle. Girando nel verso dell'avvolgimento (orario visto dal lato perno folle) spingono dentro
     tutti e due; per il verso opposto basta scambiarli di lato.
2. **Braccio guida (05 per il 135, 06 per il 120).** Oscilla su due viti. Le pareti convergenti stringono i
   bordi e incurvano la pellicola ad arco (freccia 6,5 mm sul 135, 8,7 mm sul 120): così passa tra le punte
   delle costole e, appena esce dalla punta del braccio, si riallarga e si posa sul giro giusto. La suola
   appoggia sul giro precedente, quindi il braccio sale da solo man mano che la spirale si riempie. A
   spirale vuota lo regge una vite di fermo sulla guancia.
3. **Torretta (07–14).** Due guance, il ponte con la fessura pellicola e la fessura lama, la culla del
   caricatore 135 oppure quella del rullo 120 (stesse quattro viti).
4. **Taglierina a pendolo (10–12).** Un micro-servo fa oscillare un braccio con una normale **lama da
   cutter da 9 mm**: la punta attraversa la fessura da un lato all'altro e taglia la pellicola contro il
   cielo del ponte, come una taglierina a scorrimento. Taglia tutta la larghezza, anche il 120.
5. **Trascinamento (15–19).** NEMA17 su un montante, trascinatore unico per i due formati (codolo nel foro
   del tubo + due denti), perno folle a tampone sull'altro montante, basamento.
6. **Clip (20–21).** Ganascia a cuneo con dente, chiusa da un cursore; resta nella tasca del tamburo.

## Stampa

PETG, ugello 0,4. Nessun supporto: tutti i pezzi sono già orientati.

| Pezzi | Note |
|---|---|
| 01, 02 flange | strato 0,12–0,16; faccia esterna sul piatto, nessun ponte |
| 03 tubo | in piedi; le gole a V della baionetta sono a 45°, strato 0,12–0,16 |
| 05, 06 bracci guida | capovolti (tetto sul piatto); pareti da 0,6 mm in punta: attiva le pareti sottili / Arachne |
| 09 ponte | in piedi sul lato corto, con brim |
| 20 clip ganascia | di fianco, strato 0,12; è il pezzo più fine |
| 22_A, 22_B anelli alette | lato flangia sul piatto; le alette salgono con 40° di sbalzo, senza supporti |
| 23 provino | di piatto; serve solo a scegliere il gioco, vedi sotto |
| tutti gli altri | strato 0,2, 3 perimetri, riempimento 15–20 % |

**Accoppiamento flange/tubo**: gioco 0,06 mm (`REEL_FIT` in `src/common.py`), sia radiale sul foro sia
assiale sui fianchi dei denti; nella prima versione era 0,15. Fori e anelli hanno uno smusso d'invito che
compensa anche la "zampa d'elefante" del primo strato. Il valore giusto dipende dalla stampante: il
**provino 23** ha tre fori con i denti, a gioco 0,10 / 0,06 / 0,02 mm (1, 2, 3 tacche sul bordo). Provalo
sul tubo già stampato (infila e ruota in una stazione), scegli quello che ti piace e, se non è lo 0,06,
cambia `REEL_FIT` e rigenera flange e anelli. Il tubo non cambia.

## Componenti da procurare

- Motore passo-passo **NEMA17** (albero Ø5 × 24 mm) + driver **TMC2209**
- Micro-servo **MG90S** (o SG90) con la squadretta a un braccio
- **Lama da cutter 9 mm** (0,4 mm), uno spezzone di almeno 45 mm
- Viti M3: 12 × M3×12 (ponte, culla, supporto servo), 9 × M3×10 (piedi torretta, piedi montanti, fermo
  perno folle), 10 × M3×8 (motore, perno e fermo del braccio, morsetto lama), 1 grano M3×6 + 1 dado M3
- Filamento 1,75 mm: fa da perno della fascetta (tamburo e clip) e da spina tamburo–tubo (2 pezzi da 6 mm)
- **Fascetta**: striscia 16 × ~135 mm di pellicola di scarto o PET da 0,15–0,2 mm (115 mm utili)

I fori contrassegnati come "autofilettanti" sono Ø2,6: la vite M3 fa il filetto nel PETG.

## Montaggio

1. Tubo + tamburo: infila il tamburo a metà tubo e fissalo con le due spine di filamento.
2. Fascetta: un capo attorno a un pezzo di filamento infilato nel foro del tamburo, l'altro nella testa della clip.
3. Flange: infilale sul tubo con i denti nei canali, spingi fino alla posizione voluta (35 o 120), ruota di
   30° fino alla battuta. La flangia A va dal lato motore. A 35 mm, dopo ogni flangia infila un anello
   alette (lato piatto verso la flangia) e bloccalo allo stesso modo nella stazione esterna.
4. Torretta: avvita ponte, culla e supporto servo tra le due guance, poi il braccio guida con le due viti
   perno (devono lasciarlo oscillare libero) e la vite di fermo.
5. Taglierina: incolla la squadretta del servo nella sede del braccio lama, monta la lama con il filo verso
   il lato di taglio e la punta a **75 mm dall'asse del servo** (19 mm oltre la testa del braccio).
6. Montanti e basamento; trascinatore sull'albero motore col grano.

## Uso

**135** – Caricatore nella culla con le labbra in alto verso il ponte, coda rifilata dritta. Braccio alzato.
Porta la clip fino al caricatore passando sotto il braccio e nel passaggio centrale del ponte, pinzala sulla
pellicola (ne basta 1 cm fuori), abbassa il braccio. Avvia: il motore avvolge, a fine pellicola si ferma,
la lama taglia, il motore raccoglie la coda.

**120** – Flange in posizione 120, braccio 06, culla 14. Rullo nella culla, svolgi la carta finché compare
la pellicola, pinza la clip sulla pellicola. Mentre il motore avvolge, accompagna la carta verso l'alto e
indietro (come sulla Rondinax 60). Quando arriva il nastro adesivo: stop, avanza di 20 mm, taglio.

## Sequenza per il firmware (valori di partenza)

Giri di spirale: 0,83 di fascetta, poi **8,2 giri per un 135 da 36 pose** (5,9 per 24 pose) e **4,6 giri per
un 120**. Lunghezza avvolta dopo θ radianti: `L = 23·θ + 0,175·θ²` mm.

1. Servo in parcheggio (−26,8° dalla verticale).
2. Avvolgi a 15–20 giri/min, senso orario visto dal lato perno folle.
3. Fine pellicola 135: stallo del motore (StallGuard) con corrente bassa, oppure numero massimo di giri.
4. Taglio: servo da −26,8° a +26,5° e ritorno.
5. Raccogli la coda: altri 0,6 giri.

Corrente motore, soglia di stallo e impulsi del servo vanno tarati sul banco: non li ho misurati.

## Verifiche fatte sul modello (vedi `verifiche.txt`)

- Tutti i pezzi sono solidi validi e singoli; i due assiemi non hanno nessuna coppia di corpi in interferenza.
- Flange e anelli alette col nuovo gioco: stesse verifiche di infilaggio, rotazione e tenuta sul tubo invariato.
- Verso delle alette, calcolato sulla faccia d'attacco: con lo stesso anello sui due lati un lato spinge dentro
  e l'altro fuori, e si scambiano invertendo la rotazione; con gli anelli speculari spingono dentro tutti e
  due per il verso dell'avvolgimento. È una verifica geometrica: l'effetto sul liquido va provato in tank.
- Baionetta: infilaggio libero lungo il canale, rotazione libera fino alla battuta, bloccata oltre la
  battuta e in sfilamento, in entrambe le posizioni.
- Braccio guida: nessuna interferenza con la spirale a tre raggi di avvolgimento, gioco 0,2 mm per lato.
- Taglierina: spazzata senza interferenze in 7 posizioni; la punta supera il cielo della fessura di 1,1 mm
  ai bordi del 120; in parcheggio la lama resta a 0,9 mm dalla zona pellicola 120 e 12,8 mm dalla 135.
- Capacità della spirale: 1,83 m di pellicola.

## Da validare al banco (non garantito dal solo CAD)

1. **Formazione dell'arco e posa sulle costole**: è il cuore del sistema. Parametri da ritoccare se serve:
   `RIB_H`, `TIP_CLEAR`, `WALL_TIP`, `H_EDGE`.
2. **Tenuta della clip** sotto trazione. Per le prime prove va bene anche un pezzo di nastro adesivo.
3. **Baionetta senza scatto**: durante l'avvolgimento l'attrito la tiene in battuta; per l'agitazione con
   inversioni servirà un fermo positivo (una chiavetta nel canale).
4. **Forza di taglio**: stimata, non misurata. Se l'MG90S non basta, il supporto va adattato a un servo più grande.
5. **Fine pellicola**: stallo da tarare sul 135; sul 120 in questa versione è manuale, come l'estrazione della carta.
6. La compatibilità con la tua tank: il tubo è lungo 71 mm anche in configurazione 35 mm.

## Modificare

Tutte le quote sono in `src/common.py`. Dopo una modifica: `cd src && python3 build_all.py` (serve
`pip install cadquery`): riesporta pezzi e assiemi e riesegue tutti i controlli.
