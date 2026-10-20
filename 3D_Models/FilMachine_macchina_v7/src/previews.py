"""Anteprime PNG per la documentazione (VTK fuori schermo) e sezioni - rilascio 7."""
import glob, os, subprocess, sys
import machine_assembly as M, viz
from machine import *
from common import OUT
import mrefs as R, tank as T, drive as D

def all_bodies():
    P = {}
    for f in M.GROUPS: P.update(f())
    return P

GLASS = {"REF_bagno": 0.25, "REF_contenitore": 0.35, "REF_acqua": 0.15, "REF_tappo": 0.9}
SHELL = ("REF_pannello", "D06", "D07")

if __name__ == "__main__":
    P = all_bodies(); c = M.color_of; ALT = ("REF_rullo_120",)
    tank_ref = [n for n in M.tank_bodies() if n.startswith("REF_")]
    not_tank = tuple(n for n in P if not (n[0] in "AB" or n in tank_ref))
    # macchina chiusa
    viz.render(P, "macchina_fronte_sinistra", azim=-125, elev=26, color_of=c, hide=ALT, zoom=1.0, opacity=GLASS)
    viz.render(P, "macchina_fronte_destra", azim=-52, elev=24, color_of=c, hide=ALT, zoom=1.0, opacity=GLASS)
    viz.render(P, "macchina_fronte", azim=-90, elev=8, color_of=c, hide=ALT, zoom=1.0, opacity=GLASS)
    viz.render(P, "macchina_retro", azim=62, elev=30, color_of=c, hide=ALT, zoom=1.0, opacity=GLASS)
    viz.render(P, "macchina_dall_alto", azim=-90, elev=89.5, color_of=c, hide=ALT + ("REF_pannello_copertura", "REF_bagno_coperchio"), zoom=1.0, size=(1700, 900), opacity=GLASS)
    # senza scocca: bagno, tank immersa, vano motore, piedistallo
    viz.render(P, "macchina_senza_scocca", azim=-125, elev=30, color_of=c, zoom=1.0, hide=ALT + SHELL + ("REF_bagno_coperchio",), opacity={"REF_bagno": 0.18, "REF_contenitore": 0.3, "REF_acqua": 0.12})
    viz.render(P, "macchina_senza_scocca_destra", azim=-50, elev=30, color_of=c, zoom=1.0, hide=ALT + SHELL + ("REF_bagno_coperchio",), opacity={"REF_bagno": 0.18, "REF_contenitore": 0.3, "REF_acqua": 0.12})
    ped = {n: p for n, p in P.items() if p.val().BoundingBox().zmax < PZ + 1 and not n.startswith(SHELL + ("REF_base", "REF_piano", "REF_bagno_fondo"))}
    viz.render(ped, "piedistallo", azim=-60, elev=40, color_of=c, zoom=1.0)
    viz.render(ped, "piedistallo_retro", azim=120, elev=35, color_of=c, zoom=1.0)
    bay = {n: p for n, p in P.items() if p.val().BoundingBox().xmax < BX0 + 1 and p.val().BoundingBox().zmin > PZ - 1 and not n.startswith(SHELL)}
    bay.update({n: P[n] for n in ("B01_corpo_tank", "REF_bagno_sinistra_PMMA4", "B16_mozzo_magnetico", "B17_tappo_mozzo", "A03_tubo_baionetta") if n in P})
    viz.render(bay, "vano_motore", azim=-150, elev=22, color_of=c, zoom=1.0, tol=0.15, opacity={"B01": 0.35, "REF_bagno": 0.3})
    hyd = {n: p for n, p in P.items() if n.startswith(("REF_valvola", "REF_tubo", "REF_gomit", "REF_spezzone", "REF_silicone", "REF_adattatore", "REF_passaparete", "REF_attacco",
                                                       "REF_pompa_", "REF_flussimetro", "REF_uscita", "REF_rubinetto", "REF_contenitore", "REF_chimica", "C01", "C02", "C03", "REF_riscald", "C06", "REF_pompetta", "REF_sonda", "C05", "C14"))}
    viz.render(hyd, "idraulica", azim=-60, elev=32, color_of=c, zoom=1.0, opacity={"REF_contenitore": 0.3})
    viz.render(hyd, "idraulica_retro", azim=130, elev=30, color_of=c, zoom=1.0, opacity={"REF_contenitore": 0.3})
    bath = {n: p for n, p in P.items() if n.startswith(("REF_bagno", "REF_contenitore", "REF_tappo", "REF_chimica", "REF_riscald", "C06", "REF_pompetta", "REF_sonda", "C05", "C14",
                                                        "REF_passaparete", "REF_sensore", "B01", "B02", "REF_silicone_8x12_tank", "REF_gommino", "B22", "REF_guarnizione_finestra", "REF_acqua"))}
    viz.render(bath, "bagno", azim=-60, elev=30, color_of=c, zoom=1.0, opacity={"REF_bagno": 0.2, "REF_contenitore": 0.3, "REF_acqua": 0.12, "B01": 0.6})
    # tank e trasmissione magnetica
    viz.render(P, "tank_caricatore_135", azim=-150, elev=38, color_of=c, zoom=1.0, tol=0.15, hide=not_tank + ALT + ("B02",))
    viz.render(P, "tank_caricatore_120", azim=-150, elev=38, color_of=c, zoom=1.0, tol=0.15, hide=not_tank + ("REF_caricatore_135", "B02"))
    drv = {n: P[n] for n in P if n.startswith(("B16", "B17", "B18", "B19", "B20", "B21", "B22", "REF_magnete", "REF_perno", "REF_rondella", "REF_motoriduttore", "REF_sensore_Hall", "REF_guarnizione_finestra", "A0"))}
    drv["B01_corpo_tank"] = P["B01_corpo_tank"]; drv["REF_bagno_sinistra_PMMA4"] = P["REF_bagno_sinistra_PMMA4"]
    viz.render(drv, "trasmissione_magnetica", azim=-150, elev=20, color_of=c, zoom=1.0, tol=0.12, opacity={"B01": 0.3, "REF_bagno": 0.3, "B22": 0.5})
    viz.render(drv, "trasmissione_magnetica_interno", azim=40, elev=22, color_of=c, zoom=1.0, tol=0.12, opacity={"B01": 0.25, "REF_bagno": 0.3, "B22": 0.4, "B18": 0.5, "B20": 0.4, "REF_motoriduttore": 0.3})
    M.section(P, "X", X_T - YO1 + 1.0, "sez_testata", window=(-10, D_ + 10, PZ - 20, H + 10), size=(13, 7), hide=ALT,
              title="Sezione nella testata della tank (vista da sinistra): finestra della parete del bagno, cornice, incavo del disco, Hall a sinistra del disco")
    M.section(P, "Y", AXIS_Y := Y_T, "sez_macchina_trasversale", window=(-10, W + 10, -15, H + 10), size=(14, 8), hide=ALT,
              title=f"Sezione trasversale sull'asse della spirale (Y = {AXIS_Y:.0f}): vano motore, giunto magnetico, tank nel bagno, contenitori, piedistallo")
    M.section(P, "Y", CY, "sez_contenitori", window=(-10, W + 10, -15, H + 10), size=(14, 8), hide=ALT,
              title=f"Sezione sull'asse dei contenitori (Y = {CY:.0f}): passaparete, camere di livello, valvole e collettore, rubinetti")
    M.section(P, "X", X_T, "sez_macchina_tank", window=(-10, D_ + 10, -15, H + 10), size=(13, 8), hide=ALT, title=f"Sezione sull'asse della spirale (X = {X_T:.0f}): caricatore, tank immersa, troppopieno, piedistallo")
    M.section(P, "X", CX[1], "sez_bagno_C2", window=(-10, D_ + 10, -15, H + 10), size=(13, 8), hide=ALT, title=f"Sezione per C2 (X = {CX[1]:.0f}): contenitore, striscia di servizio, valvola C2 e camera")
    # tank e spirale da sole (riferimento della tank)
    import tank, loader, reel, layout
    viz.render({"B01_corpo_tank": tank.body()}, "tank_corpo", azim=-125, elev=35, color_of=c, zoom=1.0, tol=0.1)
    viz.render({"B01_corpo_tank": tank.body()}, "tank_testata", azim=90, elev=10, color_of=c, zoom=1.0, tol=0.1)
    viz.render({"B16_mozzo_magnetico": D.mag_hub(), "B18_disco_motore": D.mag_disc().translate((0, 60, 0))}, "mozzo_e_disco", azim=120, elev=25, color_of=c, zoom=1.0, tol=0.05)
    viz.render({"B12_culla_universale": loader.cradle(), "REF_caricatore_135": loader.cassette135()}, "culla_universale_135", azim=-150, elev=35, color_of=c, zoom=1.0, tol=0.1)
    fa, tb, fb = reel.assembled("135"); ra, rb = reel.fin_rings_assembled("135")
    ex = {"A05_anello_alette_1": ra.translate((0, 0, -62)), "A01_flangia_135_A": fa.translate((0, 0, -34)), "A03_tubo_baionetta": tb,
          "A02_flangia_135_B": fb.translate((0, 0, 34)), "A05_anello_alette_2": rb.translate((0, 0, 62))}
    viz.render(ex, "spirale_esplosa", azim=-60, elev=12, color_of=lambda n: {"A01": "#3d7ab8", "A02": "#b85c3d", "A03": "#cfcfcf", "A05": "#5c9a5c"}[n[:3]], zoom=1.0, tol=0.08, size=(1700, 1000))
    G, _ = loader.place_guide("135", 30.0)
    sc = dict(zip(("B13_barra_guida", "B14_slitta_DX", "B15_slitta_SX", "REF_viti_zigrinate"), G))
    sc.update({f"A0{i}_spirale": p for i, p in enumerate(layout.place_reel("135"))})
    viz.render(sc, "scivolo_sulla_spirale", azim=-150, elev=40, color_of=c, zoom=1.0, tol=0.1, size=(1300, 950))
    subprocess.run([sys.executable, "rib_plot.py"]); subprocess.run([sys.executable, "fascetta_tubo.py"])
    for pat in ("_tmp_*.png",):
        for p in glob.glob(os.path.join(OUT, "png", pat)): os.remove(p)
    print("anteprime:", len(glob.glob(os.path.join(OUT, "png", "*.png"))))
