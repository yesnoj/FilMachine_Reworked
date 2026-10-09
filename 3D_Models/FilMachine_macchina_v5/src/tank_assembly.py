"""Assieme del modulo tank + caricatore, per il 135 e per il 120: pezzi stampati in posizione + sagome REF_."""
import itertools, sys
import cadquery as cq
from common import *
import reel, accessories, layout, tank, loader, drive, refs

def clip_in_pocket():
    """Clip appoggiata nella tasca del tamburo (ganasce disegnate aperte, cursore parcheggiato sul collo)."""
    j = accessories.jaw().rotate((0, 0, 0), (0, 0, 1), 90).translate((CLIP_FLOOR + 2.3, -CLIP_L / 2, -CLIP_W / 2))
    s = accessories.slider().rotate((0, 0, 0), (1, 1, 1), -120).translate((CLIP_FLOOR + 2.3, -CLIP_L / 2 + 4.3, 0))
    return [layout.to_tank(p.rotate((0, 0, 0), (0, 0, 1), -30)) for p in (j, s)]

def build(fmt, r_guide=None, theta=0.0, with_lid=True):
    """Tutti i corpi del modulo nel riferimento della tank. r_guide: raggio su cui appoggia la guida; theta: manovella."""
    fa, tb, dr, fb, ra, rb = layout.place_reel(fmt)
    (bar, sr, sl), phi = loader.place_guide(fmt, CORE_R if r_guide is None else r_guide)
    dz = loader.lift(theta)
    cj, cs = clip_in_pocket()
    P = {"A01_flangia_A": fa, "A02_flangia_B": fb, "A03_tubo_baionetta": tb, "A04_tamburo": dr,
         "A05_anello_alette_lato_A": ra, "A05_anello_alette_lato_B": rb, "A07_clip_ganascia": cj, "A08_clip_cursore": cs,
         "B01_corpo_tank": tank.body(), "B03_coperchietto_vano_servo": tank.bay_cover(),
         "B04_blocco_ponte_DX": loader.bridge_block(1), "B05_blocco_ponte_SX": loader.bridge_block(-1), "B06_tetto_ponte": loader.roof(),
         "B07_portalama": loader.carrier(dz), "B08_morsetto_lama_DX": loader.clamp(1, dz), "B09_morsetto_lama_SX": loader.clamp(-1, dz),
         "B10_biella": loader.rod(theta), "B11_manovella": loader.crank(theta),
         "B12_culla_universale": loader.cradle(),
         "B13_barra_guida": bar, "B14_slitta_DX": sr, "B15_slitta_SX": sl,
         "B16_trascinatore": drive.dog(), "B17_cartuccia_cuscinetti": drive.cartridge(),
         "B18_distanziale_paraolio": drive.spacer(drive.SEAL_Y[1], drive.BRG_Y[0][0]), "B19_distanziale_cuscinetti": drive.spacer(drive.BRG_Y[0][1], drive.BRG_Y[1][0]),
         "B20_rasamento_puleggia": drive.shim(), "B21_puleggia_spirale": drive.pulley_reel(), "B22_puleggia_motore": drive.pulley_motor(),
         "B23_piastra_motore": drive.motor_plate(), "B24_perno_folle": drive.idle_pin(), "B25_staffa_hall": drive.hall_bracket()}
    if with_lid:
        P["B02_coperchio_tank"] = tank.lid()
    P.update({"REF_albero_inox_8x61": refs.shaft(), "REF_cuscinetto_688_1": refs.bearing(0), "REF_cuscinetto_688_2": refs.bearing(1),
              "REF_paraolio_8x16x5": refs.seal(), "REF_rondella_M8": refs.washer(), "REF_molla_perno": refs.spring(),
              "REF_motoriduttore_JGB37-520": refs.motor(), "REF_servo_MG90S": refs.servo(), "REF_cinghia_ORing_80x3": refs.belt(),
              "REF_magneti_6x3": refs.magnets(), "REF_sensore_Hall_KY-003": refs.hall(),
              "REF_lama_cutter_DX": loader.blade(1, dz), "REF_lama_cutter_SX": loader.blade(-1, dz)})
    P["REF_caricatore_135" if fmt == "135" else "REF_rullo_120"] = loader.cassette135() if fmt == "135" else loader.roll120()
    return P

# coppie che si toccano per costruzione con accoppiamento preciso (sedi a pressione): non sono interferenze
PRESS = []

def interferences(P, tol=1e-3):
    bad = []
    for a, b in itertools.combinations(list(P), 2):
        sa, sb = P[a].val(), P[b].val()
        ba, bb = sa.BoundingBox(), sb.BoundingBox()
        if (ba.xmin > bb.xmax or bb.xmin > ba.xmax or ba.ymin > bb.ymax or bb.ymin > ba.ymax
                or ba.zmin > bb.zmax or bb.zmin > ba.zmax):
            continue
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
