"""Elenco di tutti i pezzi da stampare: nome, forma (nel riferimento in cui e' modellata), rotazioni per portarla
nell'orientamento di stampa, quantita', note di stampa."""
from common import *
import reel, accessories, tank, loader, drive, hydro as H, frame as F

def all_parts():
    P = [
        # ---- A: spirale regolabile 135 / 120 e accessori
        ("A01_flangia_A", reel.flange(), [], 1, ""), ("A02_flangia_B", reel.flange(mirror=True), [], 1, ""),
        ("A03_tubo_baionetta", reel.tube(), [], 1, ""), ("A04_tamburo", reel.drum(), [], 1, ""),
        ("A05_anello_alette", reel.fin_ring("A"), [], 2, ""), ("A06_anello_alette_variante_B", reel.fin_ring("B"), [], 0, "facoltativo: alette concordi"),
        ("A07_clip_ganascia", accessories.jaw(), [], 1, ""), ("A08_clip_cursore", accessories.slider(), [], 1, ""),
        ("A09_provino_accoppiamento", reel.fit_gauge(), [], 1, "da stampare per primo: sceglie il gioco della baionetta"),
        # ---- B: tank con caricatore
        ("B01_corpo_tank", tank.body(), [], 1, "in piedi, senza supporti"), ("B02_coperchio_tank", tank.lid(), [("X", 180)], 1, "capovolto"),
        ("B03_coperchietto_vano_servo", tank.bay_cover(), [], 1, ""),
        ("B04_blocco_ponte_DX", loader.bridge_block(1), [("X", -90)], 1, ""), ("B05_blocco_ponte_SX", loader.bridge_block(-1), [("X", 90)], 1, ""),
        ("B06_tetto_ponte", loader.roof(), [("X", 180)], 1, "capovolto"), ("B07_portalama", loader.carrier(), [("Y", -90)], 1, ""),
        ("B08_morsetto_lama_DX", loader.clamp(1), [("Y", 90)], 1, ""), ("B09_morsetto_lama_SX", loader.clamp(-1), [("Y", 90)], 1, ""),
        ("B10_biella", loader.rod(0.0), [("X", 90)], 1, ""), ("B11_manovella", loader.crank(0.0), [("X", 90)], 1, ""),
        ("B12_culla_universale", loader.cradle(), [], 1, "un solo pezzo per caricatore 135 e rullo 120"),
        ("B13_barra_guida", loader.bar_local(), [], 1, ""), ("B14_slitta_DX", loader.slide_local(), [("X", 180)], 1, "capovolta"),
        ("B15_slitta_SX", loader.slide_local().mirror("XZ"), [("X", 180)], 1, "capovolta"),
        ("B16_trascinatore", drive.dog(), [("X", -90)], 1, ""), ("B17_cartuccia_cuscinetti", drive.cartridge(), [("X", -90)], 1, "supporti solo sotto la flangia"),
        ("B18_distanziale_paraolio", drive.spacer(drive.SEAL_Y[1], drive.BRG_Y[0][0]), [("X", 90)], 1, ""),
        ("B19_distanziale_cuscinetti", drive.spacer(drive.BRG_Y[0][1], drive.BRG_Y[1][0]), [("X", 90)], 1, ""),
        ("B20_rasamento_puleggia", drive.shim(), [("X", 90)], 1, ""), ("B21_puleggia_spirale", drive.pulley_reel(), [("X", 90)], 1, ""),
        ("B22_puleggia_motore", drive.pulley_motor(), [("X", -90)], 1, ""), ("B23_piastra_motore", drive.motor_plate(), [("X", 90)], 1, ""),
        ("B24_perno_folle", drive.idle_pin(), [("X", -90)], 1, ""), ("B25_staffa_hall", drive.hall_bracket(), [("Y", 90)], 1, ""),
        # ---- C: bagno e idraulica
        ("C01_sella_testina", H.saddle(0), [("X", -90)], 3, "in piedi sui due blocchetti"), ("C02_culla_sifone", H.siphon_holder(0), [("X", 90)], 3, "col davanti sul piatto: la gola ha il cielo a capanna"),
        ("C03_staffa_ganci", H.latch(0), [("X", 90)], 3, "PETG: i due bracci sono molle"), ("C04_fermo_posteriore", H.keeper(0), [("X", -90)], 3, ""),
        ("C05_testata_telaio_SX", H.frame_end(-1), [("X", 180)], 1, "capovolta"), ("C05_testata_telaio_DX", H.frame_end(1), [("X", 180)], 1, "capovolta"),
        ("C06_portasonde", H.probe_holder(), [], 1, ""), ("C07_collettore", H.manifold(), [], 1, "4 perimetri, riempimento 100 %"),
        ("C08_pettine_valvole", H.valve_comb(), [("X", -90)], 1, ""), ("C09_staffa_scarichi", H.drain_bracket(), [], 1, ""),
        ("C10_carter_riscaldatori", H.heater_cover(), [("Y", 90)], 1, ""),
        # ---- D: struttura ed elettronica
        ("D01_piano_cavalletto", F.stand_deck(), [("X", 180)], 1, "capovolto"), ("D02_fianco_cavalletto_SX", F.stand_side(-1), [("Y", -90)], 1, ""),
        ("D03_fianco_cavalletto_DX", F.stand_side(1), [("Y", 90)], 1, ""), ("D04_frontale_cavalletto", F.stand_front(), [("X", 90)], 1, ""),
        ("D05_carter_trasmissione", F.drive_cover(), [("X", 180)], 1, "capovolto"),
        ("D06_fianco_elettronica_SX", F.el_side(-1), [("Y", -90)], 1, ""), ("D07_fianco_elettronica_DX", F.el_side(1), [("Y", 90)], 1, ""),
        ("D08_frontale_elettronica", F.el_front(), [("X", 90)], 1, ""), ("D09_plancia_display", F.bezel_local(), [("X", 180)], 1, "faccia esterna sul piatto"),
        ("D10_tetto_elettronica", F.el_top(), [], 1, ""), ("D11_paratia_posteriore", F.el_rear(), [("X", 90)], 1, ""),
        ("D12_ripiano_griglia", F.shelf(0), [], 3, ""), ("D13_colonnina_bassa", F.column(0, 0, 0, F.SH_Z[0]), [], 4, ""),
        ("D14_colonnina_alta", F.column(0, 0, 0, F.SH_Z[1] - F.SH_Z[0] - F.SH_T), [], 8, ""),
        ("D15_morsetto_scheda", F.clamp(0, 0, 0), [("X", 90)], 10, ""), ("D16_zoccolo_alimentatore", F.psu_socket(), [], 1, ""),
        ("D17_linguetta_display", F.disp_tab_local(), [], 4, "tengono la scheda display: si possono ruotare"),
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
