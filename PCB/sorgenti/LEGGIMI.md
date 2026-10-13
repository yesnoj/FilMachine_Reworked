# Sorgenti della scheda

`design.py` è l'unica fonte: componenti, valori, codici LCSC e collegamenti. `placement.py` contiene il piazzamento,
`power.py` le zone di rame di potenza (disegnate a mano) e le vie di cucitura, `mkfp.py` genera `FM.pretty` (footprint
dei morsetti a vite, dell'induttore e dei fusibili 2410, presi dai disegni dei costruttori LCSC).
`make.sh out` rigenera tutto (schema KiCad, PCB, zone di potenza, fan-out, autorouting con Freerouting 1.9,
chiusura dei collegamenti residui, cucitura di massa, serigrafia, DRC); `export.py out` produce i file per JLCPCB
(Gerber, BOM e CPL nel loro formato, PDF, STEP, render).
Requisiti: KiCad 9 (con modulo Python `pcbnew`), Java + freerouting-1.9.0.jar in /opt, xvfb, rsvg-convert, Python con
sexpdata, numpy, scipy, Pillow, openpyxl. Il progetto in `../kicad/` si può anche aprire e modificare a mano in KiCad
(la libreria `FM.pretty` è referenziata dal `fp-lib-table` del progetto).

La versione in `../kicad/` è uscita da `MAZE_CLR=0.25 bash make.sh out` (router di rifinitura a 0,25 mm; le net
ad alta corrente restano a 0,3 mm) seguito da `fullconn.py`, poi `export.py`. Strumenti di ritocco:
- `ripup.py board NET1,NET2` rifà alcune net con il router a griglia;
- `eco.py` copia il rame di una scheda già instradata su una scheda rigenerata (modifiche locali);
- `fullconn.py` collega a rame pieno le piazzole di massa THT che il DRC segnala come "starved thermal";
- `padsdump.py board REF...` stampa courtyard e piazzole (coordinate scheda) dei componenti indicati.

Le rotazioni nel CPL seguono la tabella pubblica `cpl_rotations_db.csv` (convenzioni JLCPCB per SOT-23, SOIC, SSOP,
VSSOP, ...); per i footprint senza regola la rotazione è quella di KiCad e va confermata nell'anteprima di JLCPCB.
