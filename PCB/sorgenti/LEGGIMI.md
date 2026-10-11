# Sorgenti della scheda

`design.py` è l'unica fonte: componenti, valori, codici e collegamenti. `placement.py` contiene il piazzamento.
`make.sh out` rigenera tutto (schema KiCad, PCB, zone di potenza, fan-out, autorouting con Freerouting 1.9,
chiusura dei collegamenti residui, cucitura di massa, serigrafia, DRC); `export.py out` produce i file per PCBWay.
Requisiti: KiCad 9 (con modulo Python `pcbnew`), Java + freerouting-1.9.0.jar in /opt, xvfb, Python con
sexpdata, numpy, scipy, Pillow, openpyxl. Il progetto in `../kicad/` si può anche aprire e modificare a mano in KiCad.

La versione in `../kicad/` è uscita da `MAZE_CLR=0.25 bash make.sh out` (router di rifinitura a 0,25 mm; le net
ad alta corrente restano a 0,3 mm) seguito da `fullconn.py`, poi `export.py`. Strumenti di ritocco:
- `ripup.py board NET1,NET2` rifà alcune net con il router a griglia;
- `eco.py` copia il rame di una scheda già instradata su una scheda rigenerata (modifiche locali);
- `fullconn.py` collega a rame pieno le piazzole di massa THT che il DRC segnala come "starved thermal".
