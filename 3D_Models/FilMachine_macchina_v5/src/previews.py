"""Anteprime PNG per la documentazione (VTK fuori schermo) e sezioni."""
import glob, os
import machine_assembly as M, viz
from machine import *
from common import OUT

def all_bodies():
    P = {}
    for f in (M.hydro_bodies, M.tank_bodies, M.stand_bodies, M.electronics_bodies): P.update(f())
    return P

if __name__ == "__main__":
    P = all_bodies(); c = M.color_of; ALT = ("REF_rullo_120",)
    tank_ref = [n for n in M.tank_bodies() if n.startswith("REF_")]
    not_tank = tuple(n for n in P if not (n[0] in "AB" or n in tank_ref))
    hyd = set(M.hydro_bodies())
    viz.render(P, "macchina_fronte_sinistra", azim=-125, elev=28, color_of=c, hide=ALT, zoom=1.0)
    viz.render(P, "macchina_fronte_destra", azim=-52, elev=26, color_of=c, hide=ALT, zoom=1.0)
    viz.render(P, "macchina_retro", azim=62, elev=30, color_of=c, hide=ALT, zoom=1.0)
    viz.render(P, "macchina_dall_alto", azim=-90, elev=89.5, color_of=c, hide=ALT, zoom=1.0, size=(1400, 1650))
    viz.render(P, "macchina_interno", azim=-125, elev=30, color_of=c, zoom=1.0,
               hide=ALT + ("B02", "D05", "D06", "D08", "D09", "D10", "D04", "D17", "REF_display"))
    viz.render(P, "bagno_e_idraulica", azim=-105, elev=32, color_of=c, zoom=1.0,
               hide=tuple(n for n in P if not (n[0] == "C" or n in hyd)) + ("REF_base",))
    viz.render(P, "innesto_vaschetta", azim=-140, elev=22, color_of=c, zoom=1.0, tol=0.15,
               hide=tuple(n for n in P if not (n.endswith("_C1") or n.startswith("REF_longherone") or n.startswith("C06") or n in ("REF_tubo_WB", "REF_sonda_DS18B20_bagno"))))
    viz.render(P, "tank_caricatore_135", azim=-150, elev=38, color_of=c, zoom=1.0, tol=0.15, hide=not_tank + ALT + ("B02",))
    viz.render(P, "tank_caricatore_120", azim=-150, elev=38, color_of=c, zoom=1.0, tol=0.15, hide=not_tank + ("REF_caricatore_135", "B02"))
    viz.render(P, "tank_trasmissione", azim=160, elev=18, color_of=c, zoom=1.0, tol=0.15, hide=not_tank + ALT)
    M.section(P, "X", VAS_X[0], "sez_innesto_vaschetta", window=(195, 500, 0, 215), size=(13, 9), title="Sezione sull'asse della vaschetta C1 (X = 96): testina, sifone, gancio a scatto, fermo posteriore")
    M.section(P, "X", X_T, "sez_macchina_tank", window=(0, 505, -15, 245), size=(14, 7.5), hide=ALT, title=f"Sezione sull'asse della spirale (X = {X_T:.0f}): caricatore, tank, cavalletto con pompa, bagno")
    M.section(P, "Y", Y_T, "sez_macchina_trasversale", window=(-5, 425, -15, 245), size=(13, 8), hide=ALT, title=f"Sezione trasversale sull'asse della spirale (Y = {Y_T:.0f})")
    # tank e spirale da sole (riferimento della tank)
    import tank, loader, reel
    viz.render({"B01_corpo_tank": tank.body()}, "tank_corpo", azim=-125, elev=35, color_of=c, zoom=1.0, tol=0.1)
    viz.render({"B12_culla_universale": loader.cradle(), "REF_caricatore_135": loader.cassette135()}, "culla_universale_135", azim=-150, elev=35, color_of=c, zoom=1.0, tol=0.1)
    viz.render({"B12_culla_universale": loader.cradle(), "REF_rullo_120": loader.roll120()}, "culla_universale_120", azim=-150, elev=35, color_of=c, zoom=1.0, tol=0.1)
    fa, tb, dr, fb = reel.assembled("135"); ra, rb = reel.fin_rings_assembled("135")
    ex = {"A05_anello_alette_1": ra.translate((0, 0, -62)), "A01_flangia_A": fa.translate((0, 0, -34)), "A03_tubo_baionetta": tb, "A04_tamburo": dr,
          "A02_flangia_B": fb.translate((0, 0, 34)), "A05_anello_alette_2": rb.translate((0, 0, 62))}
    viz.render(ex, "spirale_esplosa", azim=-60, elev=12, color_of=lambda n: {"A01": "#3d7ab8", "A02": "#b85c3d", "A03": "#cfcfcf", "A04": "#8a8a8a", "A05": "#5c9a5c"}[n[:3]], zoom=1.0, tol=0.08, size=(1700, 1000))
    for f in ("ponte_culla135", "tank_135_vista", "tank_135_interno", "culla_universale_sez", "sez_tank_X", "sez_tank_Y", "idraulica_vista", "idraulica_innesto", "macchina_vista_fronte", "macchina_vista_retro", "macchina_pianta", "sez_innesto_X96", "sez_gancio_X112"):
        p = os.path.join(OUT, "png", f + ".png")
        if os.path.exists(p): os.remove(p)
    print("anteprime:", len(glob.glob(os.path.join(OUT, "png", "*.png"))))
