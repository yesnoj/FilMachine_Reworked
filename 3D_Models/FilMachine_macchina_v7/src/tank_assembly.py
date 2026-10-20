"""Assieme del modulo tank + caricatore, per il 135 e per il 120: pezzi stampati in posizione + sagome REF_."""
import itertools, sys
import cadquery as cq
from common import *
import reel, layout, tank, loader, drive, refs

def build(fmt, r_guide=None, theta=0.0, with_lid=True):
    """Tutti i corpi del modulo nel riferimento della tank. r_guide: raggio su cui appoggia la guida; theta: manovella."""
    fa, tb, fb, ra, rb = layout.place_reel(fmt)
    (bar, sr, sl, scr), phi = loader.place_guide(fmt, EMPTY_R if r_guide is None else r_guide)
    dz = loader.lift(theta)
    fl_a, fl_b = ("A01_flangia_135_A", "A02_flangia_135_B") if fmt == "135" else ("A10_flangia_120_A", "A11_flangia_120_B")
    P = {fl_a: fa, fl_b: fb, "A03_tubo_baionetta": tb,
         "A05_anello_alette_lato_A": ra, "A05_anello_alette_lato_B": rb,
         "B01_corpo_tank": tank.body(), "B03_coperchietto_vano_servo": tank.bay_cover(),
         "B04_blocco_ponte_DX": loader.bridge_block(1), "B05_blocco_ponte_SX": loader.bridge_block(-1), "B06_tetto_ponte": loader.roof(),
         "B07_portalama": loader.carrier(dz), "B08_morsetto_lama_DX": loader.clamp(1, dz), "B09_morsetto_lama_SX": loader.clamp(-1, dz),
         "B10_biella": loader.rod(theta), "B11_manovella": loader.crank(theta),
         "B12_culla_universale": loader.cradle(),
         "B13_barra_guida": bar, "B14_slitta_DX": sr, "B15_slitta_SX": sl, "REF_viti_zigrinate_M3x6": scr,
         "B16_mozzo_magnetico": drive.mag_hub(), "B17_tappo_mozzo": drive.hub_cap(), "B18_disco_motore": drive.mag_disc(), "B19_tappo_disco": drive.disc_cap(),
         "B20_staffa_motore": drive.motor_bracket(), "B21_staffa_hall": drive.hall_bracket(), "B22_cornice_finestra": drive.window_frame(),
         "B24_perno_folle": drive.idle_pin()}
    if with_lid:
        P["B02_coperchio_tank"] = tank.lid()
    P.update({"REF_perno_inox_6x22": refs.pin(), "REF_rondella_PTFE": refs.ptfe_washer(), "REF_molla_perno": refs.spring(),
              "REF_motoriduttore_JGB37-520": refs.motor(), "REF_servo_MG90S": refs.servo(), "REF_sensore_Hall_KY-003": refs.hall(),
              "REF_guarnizione_finestra_EPDM": drive.window_gasket(), "REF_guarnizione_vano_servo": tank.bay_gasket(),
              "REF_lama_cutter_DX": loader.blade(1, dz), "REF_lama_cutter_SX": loader.blade(-1, dz)})
    P.update(refs.magnets())
    P["REF_caricatore_135" if fmt == "135" else "REF_rullo_120"] = loader.cassette135() if fmt == "135" else loader.roll120()
    return P

# coppie che si toccano per costruzione con accoppiamento preciso (sedi a pressione): non sono interferenze
PRESS = [("B16_mozzo_magnetico", "REF_magnete"), ("B18_disco_motore", "REF_magnete"), ("B01_corpo_tank", "REF_perno_inox"), ("B16_mozzo_magnetico", "REF_perno_inox")]

def interferences(P, tol=1e-3):
    bad = []
    for a, b in itertools.combinations(list(P), 2):
        sa, sb = P[a].val(), P[b].val()
        ba, bb = sa.BoundingBox(), sb.BoundingBox()
        if (ba.xmin > bb.xmax or bb.xmin > ba.xmax or ba.ymin > bb.ymax or bb.ymin > ba.ymax
                or ba.zmin > bb.zmax or bb.zmin > ba.zmax):
            continue
        if any((a.startswith(x) and b.startswith(y)) or (b.startswith(x) and a.startswith(y)) for x, y in PRESS): continue
        v = sa.intersect(sb).Volume()
        if v > tol: bad.append((a, b, round(v, 3)))
    return bad

if __name__ == "__main__":
    for fmt in (sys.argv[1:] or ["135", "120"]):
        P = build(fmt)
        inv = [n for n, p in P.items() if not p.val().isValid()]
        bad = interferences(P)
        print(f"modulo tank {fmt}: {len(P)} corpi, non validi {inv if inv else 'nessuno'}")
        for b in bad: print("   INTERFERENZA", b)
        if not bad: print("   nessuna interferenza")
