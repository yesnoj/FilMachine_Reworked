"""Anteprime PNG per la documentazione (VTK fuori schermo) e sezioni - rilascio 6."""
import glob, os
import machine_assembly as M, viz
from machine import *
from common import OUT

def all_bodies():
    P = {}
    for f in M.GROUPS: P.update(f())
    return P

SHELL_OPEN = ("REF_pannello_frontale", "REF_pannello_sinistro", "REF_piano_anteriore", "REF_coperchio", "D07", "D09", "D10", "REF_display")
PLEXI = {"REF_coperchio_plexiglass": 0.28}

if __name__ == "__main__":
    P = all_bodies(); c = M.color_of; ALT = ("REF_rullo_120",)
    tank_ref = [n for n in M.tank_bodies() if n.startswith("REF_")]
    not_tank = tuple(n for n in P if not (n[0] in "AB" or n in tank_ref))
    hyd = set(M.hydro_bodies())
    # macchina chiusa: coperchio trasparente
    viz.render(P, "macchina_fronte_sinistra", azim=-125, elev=28, color_of=c, hide=ALT, zoom=1.0, opacity=PLEXI)
    viz.render(P, "macchina_fronte_destra", azim=-52, elev=26, color_of=c, hide=ALT, zoom=1.0, opacity=PLEXI)
    viz.render(P, "macchina_retro", azim=62, elev=30, color_of=c, hide=ALT, zoom=1.0, opacity=PLEXI)
    viz.render(P, "macchina_dall_alto", azim=-90, elev=89.5, color_of=c, hide=ALT, zoom=1.0, size=(1400, 1650), opacity=PLEXI)
    # interno: senza frontale, fianco sinistro, piano, coperchio e montanti alti
    viz.render(P, "macchina_interno", azim=-125, elev=30, color_of=c, zoom=1.0, hide=ALT + SHELL_OPEN + ("B02",))
    viz.render(P, "macchina_interno_destra", azim=-55, elev=30, color_of=c, zoom=1.0,
               hide=ALT + ("REF_pannello_frontale", "REF_pannello_destro", "REF_piano_anteriore", "REF_coperchio", "D07", "D09", "D10", "REF_display", "B02"))
    bay = {n: p for n, p in P.items() if not n.startswith(SHELL_OPEN + ("REF_pannello_posteriore",))}
    bay = {n: p for n, p in bay.items() if p.val().BoundingBox().xmin < 196 and p.val().BoundingBox().ymin < 130}
    viz.render(bay, "vano_elettronica", azim=-130, elev=24, color_of=c, zoom=1.0)
    viz.render(P, "bagno_e_idraulica", azim=-105, elev=32, color_of=c, zoom=1.0,
               hide=tuple(n for n in P if not (n[0] == "C" or n in hyd)) + ("REF_base",))
    viz.render(P, "innesto_vaschetta", azim=-140, elev=22, color_of=c, zoom=1.0, tol=0.15,
               hide=tuple(n for n in P if not (n.endswith("_C1") or n.startswith(("REF_longherone", "C06", "C14_presa_C1", "C15_camera_livello_C1")) or n in ("REF_tubo_WB", "REF_sonda_DS18B20_bagno"))))
    # sonde, riscaldatori e rubinetti in evidenza (rosso: sonde e portasonde; arancio: riscaldatori e carter; viola: svuotamento)
    def hi(n):
        if "DS18B20" in n or n.startswith("C19") or n.startswith("C06"): return "#d62828"
        if "riscaldatore" in n: return "#f77f00"
        if "svuotamento" in n or "rubinetto" in n: return "#7b2cbf"
        if n.startswith("REF_base"): return c(n)
        return "#c9ccd0"
    keep = lambda n: (n.startswith(("REF_contenitore", "REF_vaschetta", "REF_longherone", "REF_base", "C05", "C04", "C14", "C07", "REF_elettrovalvola",
                                    "REF_sonda", "C19", "C06", "REF_riscaldatore", "C10", "REF_tubo_WB", "REF_pannello_destro", "REF_pannello_sinistro"))
                      or "svuotamento" in n or "rubinetto" in n)
    sub = {n: p for n, p in P.items() if keep(n)}
    op = {"REF_contenitore": 0.25, "REF_vaschetta": 0.35, "REF_pannello": 0.18}
    viz.render(sub, "sonde_riscaldatori_rubinetti", azim=-60, elev=30, color_of=hi, zoom=1.0, opacity=op)
    viz.render(sub, "sonde_riscaldatori_rubinetti_retro", azim=-140, elev=26, color_of=hi, zoom=1.0, opacity=op)
    inlet_keys = ("ingresso", "flussimetro", "beccuccio", "C16", "C17", "C18", "INGRESSO")
    viz.render(P, "ingresso_acqua", azim=-60, elev=28, color_of=c, zoom=1.0, tol=0.15,
               hide=tuple(n for n in P if not any(k in n for k in inlet_keys)))
    viz.render(P, "tank_caricatore_135", azim=-150, elev=38, color_of=c, zoom=1.0, tol=0.15, hide=not_tank + ALT + ("B02",))
    viz.render(P, "tank_caricatore_120", azim=-150, elev=38, color_of=c, zoom=1.0, tol=0.15, hide=not_tank + ("REF_caricatore_135", "B02"))
    viz.render(P, "tank_trasmissione", azim=160, elev=18, color_of=c, zoom=1.0, tol=0.15, hide=not_tank + ALT)
    M.section(P, "X", VAS_X[0], "sez_innesto_vaschetta", window=(190, 515, -15, 260), size=(13, 10), hide=ALT,
              title=f"Sezione sull'asse della vaschetta C1 (X = {VAS_X[0]:.0f}): innesto dal basso, presa C14, linea che scende nel bagno, fermi e portasensore MAX")
    M.section(P, "X", X_T, "sez_macchina_tank", window=(-10, 515, -15, 260), size=(14, 7.5), hide=ALT, title=f"Sezione sull'asse della spirale (X = {X_T:.0f}): caricatore, tank a filo del piano, cavalletto con pompa, bagno")
    M.section(P, "Y", Y_T, "sez_macchina_trasversale", window=(-10, 430, -15, 270), size=(13, 8), hide=ALT, title=f"Sezione trasversale sull'asse della spirale (Y = {Y_T:.0f}): vano elettronica, cruscotto, tank")
    M.section(P, "Y", 384.5, "sez_bagno_trasversale", window=(-10, 430, -15, 260), size=(13, 8), hide=ALT, title="Sezione trasversale del bagno sull'asse delle vaschette (Y = 384.5)")
    # tank e spirale da sole (riferimento della tank)
    import tank, loader, reel
    viz.render({"B01_corpo_tank": tank.body()}, "tank_corpo", azim=-125, elev=35, color_of=c, zoom=1.0, tol=0.1)
    viz.render({"B12_culla_universale": loader.cradle(), "REF_caricatore_135": loader.cassette135()}, "culla_universale_135", azim=-150, elev=35, color_of=c, zoom=1.0, tol=0.1)
    viz.render({"B12_culla_universale": loader.cradle(), "REF_rullo_120": loader.roll120()}, "culla_universale_120", azim=-150, elev=35, color_of=c, zoom=1.0, tol=0.1)
    fa, tb, fb = reel.assembled("135"); ra, rb = reel.fin_rings_assembled("135")
    ex = {"A05_anello_alette_1": ra.translate((0, 0, -62)), "A01_flangia_135_A": fa.translate((0, 0, -34)), "A03_tubo_baionetta": tb,
          "A02_flangia_135_B": fb.translate((0, 0, 34)), "A05_anello_alette_2": rb.translate((0, 0, 62))}
    viz.render(ex, "spirale_esplosa", azim=-60, elev=12, color_of=lambda n: {"A01": "#3d7ab8", "A02": "#b85c3d", "A03": "#cfcfcf", "A04": "#8a8a8a", "A05": "#5c9a5c"}[n[:3]], zoom=1.0, tol=0.08, size=(1700, 1000))
    # scivolo tipo Lab-Box: da solo e sulla spirale (135, a meta' riempimento)
    import loader, layout
    G, _ = loader.place_guide("135", 30.0)
    sc = dict(zip(("B13_barra_guida", "B14_slitta_DX", "B15_slitta_SX", "REF_viti_zigrinate"), G))
    viz.render(sc, "scivolo_tipo_labbox", azim=-140, elev=35, color_of=c, zoom=1.0, tol=0.1, size=(1300, 950))
    sc.update({f"A0{i}_spirale": p for i, p in enumerate(layout.place_reel("135"))})
    viz.render(sc, "scivolo_sulla_spirale", azim=-150, elev=40, color_of=c, zoom=1.0, tol=0.1, size=(1300, 950))
    # imbuto di svuotamento con la vaschetta appoggiata (riferimento della vaschetta) e confronto dell'avvio della spirale
    import hydro as H
    fun = {"C20_imbuto_svuotamento": H.drain_funnel_local(), "C11_corpo_innesto": H.inlet_body_local(), "C12_ghiera_codolo": H.nut_spigot_local(),
           "C13_otturatore": H.poppet_local(3.5)}
    viz.render(fun, "imbuto_svuotamento", azim=-60, elev=20, color_of=c, zoom=1.0, tol=0.05, size=(900, 1100))
    import subprocess, sys
    subprocess.run([sys.executable, "rib_plot.py"]); subprocess.run([sys.executable, "scivolo_sezioni.py"]); subprocess.run([sys.executable, "fascetta_tubo.py"])
    # immagini di lavoro dei concept (restano in concept/)
    for pat in ("concept*.png", "guida_1*_*.png", "v6_*.png"):
        for p in glob.glob(os.path.join(OUT, "png", pat)): os.remove(p)
    print("anteprime:", len(glob.glob(os.path.join(OUT, "png", "*.png"))))
