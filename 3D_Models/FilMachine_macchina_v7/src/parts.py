"""Elenco di tutti i pezzi da stampare: nome, forma (nel riferimento in cui e' modellata), rotazioni per portarla
nell'orientamento di stampa, quantita', note di stampa."""
from common import *
import reel, tank, loader, drive, hydro as H, shell as S

def all_parts():
    P = [
        # ---- A: spirale 135 / 120 (una coppia di flange per formato, il resto in comune). v6.2: la fascetta con la clip
        #      e' quella della Lab-Box, comprata gia' fatta: la clip stampata (A07, A08) non c'e' piu'
        ("A01_flangia_135_A", reel.flange(), [], 1, "per il 135"), ("A02_flangia_135_B", reel.flange(mirror=True), [], 1, "per il 135"),
        ("A03_tubo_baionetta", reel.tube(), [], 1, "passaggio della fascetta da parte a parte tra i due collarini, finestra larga e stretta (come la Lab-Box); il tamburo A04 non c'e' piu'"),
        ("A05_anello_alette", reel.fin_ring("A"), [], 2, ""), ("A06_anello_alette_variante_B", reel.fin_ring("B"), [], 0, "facoltativo: alette concordi"),
        ("A09_provino_accoppiamento", reel.fit_gauge(), [], 1, "da stampare per primo: sceglie il gioco della baionetta"),
        ("A10_flangia_120_A", reel.flange(fmt="120"), [], 1, "per il 120: giri piu' distanziati"),
        ("A11_flangia_120_B", reel.flange(mirror=True, fmt="120"), [], 1, "per il 120: giri piu' distanziati"),
        # ---- B: tank con caricatore (v7: testata magnetica, nessun passaggio d'albero)
        ("B01_corpo_tank", tank.body(), [], 1, "in piedi, senza supporti; la galleria del vecchio motore resta come vuoto chiuso"),
        ("B02_coperchio_tank", tank.lid(), [("X", 180)], 1, "capovolto"),
        ("B03_coperchietto_vano_servo", tank.bay_cover(), [], 1, "con la guarnizione piana da 1 mm (il vano e' sott'acqua)"),
        ("B04_blocco_ponte_DX", loader.bridge_block(1), [("X", -90)], 1, ""), ("B05_blocco_ponte_SX", loader.bridge_block(-1), [("X", 90)], 1, ""),
        ("B06_tetto_ponte", loader.roof(), [("X", 180)], 1, "capovolto"), ("B07_portalama", loader.carrier(), [("Y", -90)], 1, ""),
        ("B08_morsetto_lama_DX", loader.clamp(1), [("Y", 90)], 1, ""), ("B09_morsetto_lama_SX", loader.clamp(-1), [("Y", 90)], 1, ""),
        ("B10_biella", loader.rod(0.0), [("X", 90)], 1, ""), ("B11_manovella", loader.crank(0.0), [("X", 90)], 1, ""),
        ("B12_culla_universale", loader.cradle(), [], 1, "un solo pezzo per caricatore 135 e rullo 120"),
        ("B13_barra_guida", loader.bar_local(), [], 1, "quattro fossette: posizioni 135 (strette) e 120 (larghe)"),
        ("B14_slitta_DX", loader.slide_local(), [("Y", -90)], 1, "in piedi sulla coda; supporti solo dal piatto; vite zigrinata M3 x 10"),
        ("B15_slitta_SX", loader.slide_local().mirror("XZ"), [("Y", -90)], 1, "in piedi sulla coda; supporti solo dal piatto; vite zigrinata M3 x 10"),
        ("B16_mozzo_magnetico", drive.mag_hub(), [("X", -90)], 1, "faccia delle tasche sul piatto (le tasche sono fori ciechi dal piatto), nose in alto, senza supporti; 4 perimetri; i magneti si incollano con epossidica (polo sud degli indicatori verso la testata) e si chiudono col tappo B17"),
        ("B17_tappo_mozzo", drive.hub_cap(), [("X", 90)], 1, "anello da 1 mm incollato di piatto con epossidica sulla faccia delle tasche, bordo sigillato (la chimica non deve toccare i magneti)"),
        ("B18_disco_motore", drive.mag_disc(), [("X", 90)], 1, "faccia delle tasche sul piatto (fori ciechi dal piatto), mozzo in alto, senza supporti; magneti incollati con epossidica, chiusi dal tappo B19"),
        ("B19_tappo_disco", drive.disc_cap(), [("X", -90)], 1, "disco da 1 mm incollato di piatto sulla faccia delle tasche"),
        ("B20_staffa_motore", drive.motor_bracket(), [("X", 90)], 1, "di piatto; sei fori M3 del riduttore (centro 7 mm sopra l'asse), due fori passanti per le viti della staffa Hall, asole dei piedi"),
        ("B21_staffa_hall", drive.hall_bracket(), [("Y", -90)], 1, "di fianco (faccia -X sul piatto); il KY-003 si infila nella fessura da destra, col sensore piegato di piatto sul bordo; una goccia di colla a caldo lo ferma"),
        ("B22_cornice_finestra", drive.window_frame(), [("X", 90)], 1, "di piatto; stringe testata, guarnizione e parete del bagno con otto viti M3 x 12"),
        ("B24_perno_folle", drive.idle_pin(), [("X", -90)], 1, ""),
        # ---- C: idraulica, bagno, struttura interna
        ("C01_collettore", H.manifold(), [], 1, "di piatto (le gambe sul piatto); 4 perimetri, riempimento 100 %; sei sedi orizzontali con O-ring 9x2 + una verso la pompa"),
        ("C02_culla_valvole", H.valve_cradle(), [], 1, "di piatto"),
        ("C03_camera_livello", H.chamber(0), [], 3, "in piedi, PETG trasparente; 3 perimetri, riempimento 100 %; bocca laterale verso +X"),
        ("C04_culla_flussimetro", H.flow_cradle(), [], 1, ""),
        ("C05_portasonda", H.probe_holder(), [], 1, ""),
        ("C06_sella_riscaldatore", H.heater_saddle(0), [], 2, "si incolla al fondo del bagno (colla per PMMA) sotto l'estremita' libera della cartuccia"),
        ("C07_culla_valvola_ingresso", H.inlet_valve_cradle(), [], 1, ""),
        ("C08_culla_pompa", H.pump_cradle(), [], 1, ""),
        ("C09_fermo_alimentatore", H.psu_stop(0), [], 2, "uno per testa dell'alimentatore"),
        ("C10_montante_mainboard", H.board_post(0), [("Y", 90)], 2, "di fianco (faccia laterale sul piatto); i boss hanno il foro M3 autofilettante per le viti della scheda"),
        ("C12_cornice_display", H.display_bezel(), [("X", -90)], 1, "faccia del pannello sul piatto"),
        ("C13_colonna_piano", H.deck_column(*H.COLS[0]), [], 2, "in piedi"),
        ("C14_staffa_troppopieno", H.riser_clip(H.OVF_X), [], 1, "staffa a cavallo della parete posteriore per il tubo del troppopieno (asse a 40 mm dalla parete)"),
        ("C15_staffa_beccuccio", H.riser_clip(H.IN_X, H.IN_Y), [], 1, "come C14 ma col braccio corto: tubo del beccuccio a 15 mm dalla parete"),
        ("D06_montante_inferiore", S.post_local(True), [], 4, "in piedi; uguale per i quattro angoli"),
        ("D07_montante_superiore", S.post_local(False), [("X", 180)], 4, "capovolto; uguale per i quattro angoli"),
        ("D10_linguetta_display", H.display_tab_local(), [], 4, "tengono la scheda display sulla cornice"),
    ]
    return P

if __name__ == "__main__":
    import time
    t0 = time.time(); tot = 0.0
    for name, shape, rots, q, note in all_parts():
        s = to_print(shape, rots).val(); bb = s.BoundingBox(); tot += s.Volume() * max(q, 0)
        ok = s.isValid() and len(s.Solids()) == 1
        print(f"{name:32s} x{q}  {'ok' if ok else 'NON VALIDO'}  {s.Volume() / 1000:7.1f} cm3  {bb.xlen:6.1f} x {bb.ylen:6.1f} x {bb.zlen:6.1f}")
    print(f"volume totale (con le quantita'): {tot / 1000:.0f} cm3  ({time.time() - t0:.0f} s)")
