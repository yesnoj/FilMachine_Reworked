"""Elenco di tutti i pezzi da stampare: nome, forma (nel riferimento in cui e' modellata), rotazioni per portarla
nell'orientamento di stampa, quantita', note di stampa."""
from common import *
import reel, tank, loader, drive, hydro as H, frame as F, shell as S

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
        # ---- B: tank con caricatore
        ("B01_corpo_tank", tank.body(), [], 1, "in piedi, senza supporti"), ("B02_coperchio_tank", tank.lid(), [("X", 180)], 1, "capovolto"),
        ("B03_coperchietto_vano_servo", tank.bay_cover(), [], 1, ""),
        ("B04_blocco_ponte_DX", loader.bridge_block(1), [("X", -90)], 1, ""), ("B05_blocco_ponte_SX", loader.bridge_block(-1), [("X", 90)], 1, ""),
        ("B06_tetto_ponte", loader.roof(), [("X", 180)], 1, "capovolto"), ("B07_portalama", loader.carrier(), [("Y", -90)], 1, ""),
        ("B08_morsetto_lama_DX", loader.clamp(1), [("Y", 90)], 1, ""), ("B09_morsetto_lama_SX", loader.clamp(-1), [("Y", 90)], 1, ""),
        ("B10_biella", loader.rod(0.0), [("X", 90)], 1, ""), ("B11_manovella", loader.crank(0.0), [("X", 90)], 1, ""),
        ("B12_culla_universale", loader.cradle(), [], 1, "un solo pezzo per caricatore 135 e rullo 120"),
        ("B13_barra_guida", loader.bar_local(), [], 1, "quattro fossette: posizioni 135 (strette) e 120 (larghe)"),
        ("B14_slitta_DX", loader.slide_local(), [("Y", -90)], 1, "in piedi sulla coda; supporti solo dal piatto (sotto il piano e l'aletta); si blocca con una vite zigrinata M3 x 6"),
        ("B15_slitta_SX", loader.slide_local().mirror("XZ"), [("Y", -90)], 1, "in piedi sulla coda; supporti solo dal piatto (sotto il piano e l'aletta); si blocca con una vite zigrinata M3 x 6"),
        ("B16_trascinatore", drive.dog(), [("X", -90)], 1, ""), ("B17_cartuccia_cuscinetti", drive.cartridge(), [("X", -90)], 1, "supporti solo sotto la flangia"),
        ("B18_distanziale_paraolio", drive.spacer(drive.SEAL_Y[1], drive.BRG_Y[0][0]), [("X", 90)], 1, ""),
        ("B19_distanziale_cuscinetti", drive.spacer(drive.BRG_Y[0][1], drive.BRG_Y[1][0]), [("X", 90)], 1, ""),
        ("B20_rasamento_puleggia", drive.shim(), [("X", 90)], 1, ""), ("B21_puleggia_spirale", drive.pulley_reel(), [("X", 90)], 1, ""),
        ("B22_puleggia_motore", drive.pulley_motor(), [("X", -90)], 1, ""), ("B23_piastra_motore", drive.motor_plate(), [("X", 90)], 1, ""),
        ("B24_perno_folle", drive.idle_pin(), [("X", -90)], 1, ""), ("B25_staffa_hall", drive.hall_bracket(), [("Y", 90)], 1, ""),
        # ---- C: bagno e idraulica
        ("C03_fermo_girevole", H.turn_button_local(), [], 6, "due per vaschetta (davanti e dietro); vite autofilettante 2,9 x 13 a testa svasata"),
        ("C04_portasensore_MAX", H.sensor_holder(0), [("X", 90)], 3, "di piatto sulla faccia anteriore"),
        ("C05_testata_telaio_SX", H.frame_end(-1), [("X", -90)], 1, "in piedi (alta 202 mm), brim consigliato; solo ponti corti"), ("C05_testata_telaio_DX", H.frame_end(1), [("X", -90)], 1, "in piedi (alta 202 mm), brim consigliato; solo ponti corti"),
        ("C06_portasonde", H.probe_holder(), [], 1, "in piedi, come montato"),
        ("C07_collettore", H.manifold(), [], 1, "4 perimetri, riempimento 100 %"),
        ("C08_pettine_valvole", H.valve_comb(), [("X", -90)], 1, ""), ("C09_piastra_scarichi", H.drain_plate(), [("Y", 90)], 1, ""),
        ("C10_carter_riscaldatore", H.heater_cover(0), [("Y", 90)], 2, "uno per riscaldatore"),
        ("C19_gancio_portasonda", H.probe_hook(), [("X", 180)], 1, "capovolto (lingua sul piatto)"),
        ("C20_imbuto_svuotamento", H.drain_funnel_local(), [], 1, "beccuccio sul piatto; 4 perimetri; uno serve per tutte e tre le vaschette"),
        ("C11_corpo_innesto", H.inlet_body_local(), [("X", 180)], 3, "capovolto, flangia sul piatto; 4 perimetri, riempimento 100 %"),
        ("C12_ghiera_codolo", H.nut_spigot_local(), [("X", 180)], 3, "dado sul piatto; 4 perimetri, riempimento 100 %"),
        ("C13_otturatore", H.poppet_local(), [], 3, "in piedi sul naso; riempimento 100 %"),
        ("C14_presa", H.socket(0), [], 3, "in piedi; 4 perimetri, riempimento 100 % (deve pesare e non galleggiare)"),
        ("C15_camera_livello", H.chamber(0), [], 3, "in piedi, PETG trasparente; 3 perimetri, riempimento 100 %"),
        ("C16_piastra_ingresso", H.inlet_plate(), [("Y", 90)], 1, ""), ("C17_staffa_beccuccio", H.spout_clip(), [("X", 180)], 1, "capovolta"),
        ("C18_staffa_fascetta", H.tie_brackets()[0], [], 2, "flussimetro e valvola dell'ingresso"),
        # ---- D: struttura, elettronica, scocca
        ("D01_piano_cavalletto", F.stand_deck(), [("X", 180)], 1, "capovolto"), ("D02_fianco_cavalletto_SX", F.stand_side(-1), [("Y", -90)], 1, ""),
        ("D03_fianco_cavalletto_DX", F.stand_side(1), [("Y", 90)], 1, ""), ("D04_frontale_cavalletto", F.stand_front(), [("X", 90)], 1, ""),
        ("D05_carter_trasmissione", F.drive_cover(), [("X", 180)], 1, "capovolto"),
        ("D06_montante_inferiore", S.post_local(True), [], 4, "in piedi; uguale per i quattro angoli"),
        ("D07_montante_superiore", S.post_local(False), [("X", 180)], 4, "capovolto; uguale per i quattro angoli"),
        ("D08_appoggio_piano", S.deck_support(0.0), [], 4, ""),
        ("D09_cruscotto_display", S.wedge(), [("X", 180.0 - S.W_ANG)], 1, "faccia del display sul piatto"),
        ("D10_linguetta_display", S.disp_tab_local(), [], 4, "tengono la scheda display: si possono ruotare"),
        ("D11_guida_paratia", F.panel_guide(True), [], 1, ""), ("D11_guida_divisorio", F.panel_guide(False), [], 1, ""),
        ("D12_ripiano_griglia", F.shelf(0), [], 3, ""), ("D13_piede_colonna", F.col_foot(0, 0), [], 4, ""),
        ("D14_colonnina", F.column(0, 0, 0, F.SH_Z[1] - F.SH_Z[0] - F.SH_T), [], 8, ""),
        ("D15_morsetto_scheda", F.clamp(0, 0, 0), [("X", 90)], 10, ""), ("D16_zoccolo_alimentatore", F.psu_socket(), [], 1, ""),
        ("D17_staffa_alimentatore", F.psu_strap(), [("X", 180)], 1, "capovolta"), ("D19_supporto_altoparlante", F.speaker_holder(), [("Y", 90)], 1, ""),
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
