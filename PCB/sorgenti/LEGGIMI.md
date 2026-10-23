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

La versione in `../kicad/` (v1.2) è uscita da `MAZE_CLR=0.25 bash make.sh out` (router di rifinitura a 0,25 mm; le
net ad alta corrente restano a 0,3 mm), più le correzioni di `fix_v12.py` (descritte nel file: HEAT1_DRV con vie da
0,5 mm via `MAZE_VIA=0.5 MAZE_CLR=0.22 maze.py`, R62 ruotata per legare il suo pad di massa, due tratti +3V3 ristretti
vicino a U7, una scheggia di massa isolata eliminata, C56 a rame pieno), poi — revisione del 23/10/2026 —
`fix_v12b.py` (R80/R82 4,7k, piazzole di L1 da datasheet, rilievi termici sulle piazzole passanti, vie HC 0,8/0,4),
`silk.py` rilanciato (scritte ≥ 1,0 mm) e `export.py`. `design.py`, `mkfp.py`, `build_pcb.py`, `fullconn.py`,
`power.py` e `silk.py` contengono già le stesse scelte, così una rigenerazione completa dà lo stesso risultato
(lo schema rigenerato da `gen_sch.py` differisce solo nei campi di R80/R82 e J25: gli UUID sono deterministici).
Strumenti di ritocco:
- `ripup.py board NET1,NET2` rifà alcune net con il router a griglia;
- `maze.py board` chiude i collegamenti rimasti (`MAZE_VIA=0.5` per le vie strette, `--tie REF.PAD` lega un pad al piano);
- `gndvia.py board REF PAD` cerca una via di massa accanto a un pad (`GNDVIA_D=0.5` per vie da 0,5 mm);
- `handroute.py` verifica (e con `--add` aggiunge) un percorso disegnato a mano;
- `eco.py` copia il rame di una scheda già instradata su una scheda rigenerata (modifiche locali);
- `fullconn.py` collega a rame pieno le piazzole di massa THT che il DRC segnala come "starved thermal";
- `padsdump.py board REF...` stampa courtyard e piazzole (coordinate scheda) dei componenti indicati;
- `zones.py render.png uscita.png` disegna la mappa delle zone (`../FilMachine_Mainboard_zone.png`) su un render
  dall'alto della scheda (comando del render in testa al file).

Il CPL è riferito al footprint JLCPCB/EasyEDA di ogni codice LCSC: `fetch_easyeda.sh` scarica i footprint in
`easyeda/`, `jlcpos.py` li confronta con quelli KiCad e scrive `jlcpos.json`, che `export.py` usa per posizioni e
rotazioni. `cplcheck.py board CPL BOM` rifà il controllo: piazza i pad EasyEDA dove li metterà JLCPCB e misura la
distanza dalle piazzole (gli scarti di qualche decimo su induttore, TO-263, SMA e 1206 sono differenze di passo tra i
due footprint, con il componente centrato). `cpl_rotations_db.csv` (tabella pubblica) resta solo come riserva per
footprint senza confronto.
