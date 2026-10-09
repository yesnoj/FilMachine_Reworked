"""PDF 3D dell'assieme: maglie -> IDTF -> U3D (IDTFConverter) -> PDF (LaTeX, pacchetto media9).
Ogni corpo e' un nodo con il suo nome, raccolto in un gruppo: in Adobe Acrobat Reader si accendono e spengono
dall'albero del modello. Uso: python3 make_pdf3d.py [percorso di IDTFConverter]"""
import os, sys, subprocess, math, shutil, time
import numpy as np
import vtk
from vtk.util import numpy_support as ns
import matplotlib.colors as mc
from common import OUT
import machine_assembly as M, viz

CONV = sys.argv[1] if len(sys.argv) > 1 else shutil.which("IDTFConverter") or "IDTFConverter"
WORK = os.path.join(OUT, "docs", "_pdf3d")
GROUPS = [("A_spirale_e_accessori", lambda n: n.startswith("A")),
          ("B_tank_e_caricatore", lambda n: n.startswith("B")),
          ("C_bagno_e_idraulica_stampati", lambda n: n.startswith("C")),
          ("D_struttura_ed_elettronica_stampati", lambda n: n.startswith("D"))]
REF_GROUPS = ["REF_tank_componenti_acquistati", "REF_bagno_e_idraulica_componenti_acquistati", "REF_elettronica_componenti_acquistati"]

def mesh(shape, tol=0.3, ang=0.35):
    """Maglia con normali per vertice (spigoli vivi divisi): restituisce punti, normali, triangoli."""
    pd = viz.polydata(shape, tol, ang)
    cl = vtk.vtkCleanPolyData(); cl.SetInputData(pd); cl.Update()
    nr = vtk.vtkPolyDataNormals(); nr.SetInputConnection(cl.GetOutputPort()); nr.SetFeatureAngle(35); nr.SplittingOn()
    nr.ConsistencyOn(); nr.AutoOrientNormalsOn(); nr.ComputePointNormalsOn(); nr.Update()
    o = nr.GetOutput()
    pts = ns.vtk_to_numpy(o.GetPoints().GetData()).astype(np.float64)
    nrm = ns.vtk_to_numpy(o.GetPointData().GetNormals()).astype(np.float64)
    tri = ns.vtk_to_numpy(o.GetPolys().GetData()).reshape(-1, 4)[:, 1:]
    return pts, nrm, tri

IDENT = "\t\t\t1.000000 0.000000 0.000000 0.0\n\t\t\t0.000000 1.000000 0.000000 0.0\n\t\t\t0.000000 0.000000 1.000000 0.0\n\t\t\t0.000000 0.000000 0.000000 1.0\n"
def node(kind, name, parent, resource=None):
    s = f'NODE "{kind}" {{\n\tNODE_NAME "{name}"\n\tPARENT_LIST {{\n\t\tPARENT_COUNT 1\n\t\tPARENT 0 {{\n\t\t\tPARENT_NAME "{parent}"\n\t\t\tPARENT_TM {{\n{IDENT}\t\t\t}}\n\t\t}}\n\t}}\n'
    if resource: s += f'\tRESOURCE_NAME "{resource}"\n'
    return s + "}\n\n"

def write_idtf(P, group_of, path, tol=0.3):
    names = list(P); cols = sorted({M.color_of(n) for n in names}); stats = []
    with open(path, "w") as f:
        f.write('FILE_FORMAT "IDTF"\nFORMAT_VERSION 100\n\n')
        for g in sorted(set(group_of.values())):
            f.write(node("GROUP", g, "<NULL>"))
        for n in names:
            f.write(node("MODEL", n, group_of[n], n))
        f.write(f'RESOURCE_LIST "MODEL" {{\n\tRESOURCE_COUNT {len(names)}\n')
        for i, n in enumerate(names):
            pts, nrm, tri = mesh(P[n], tol); stats.append((n, len(tri)))
            f.write(f'\tRESOURCE {i} {{\n\t\tRESOURCE_NAME "{n}"\n\t\tMODEL_TYPE "MESH"\n\t\tMESH {{\n\t\t\tFACE_COUNT {len(tri)}\n\t\t\tMODEL_POSITION_COUNT {len(pts)}\n'
                    f'\t\t\tMODEL_NORMAL_COUNT {len(nrm)}\n\t\t\tMODEL_DIFFUSE_COLOR_COUNT 0\n\t\t\tMODEL_SPECULAR_COLOR_COUNT 0\n\t\t\tMODEL_TEXTURE_COORD_COUNT 0\n'
                    '\t\t\tMODEL_BONE_COUNT 0\n\t\t\tMODEL_SHADING_COUNT 1\n\t\t\tMODEL_SHADING_DESCRIPTION_LIST {\n\t\t\t\tSHADING_DESCRIPTION 0 {\n'
                    '\t\t\t\t\tTEXTURE_LAYER_COUNT 0\n\t\t\t\t\tSHADER_ID 0\n\t\t\t\t}\n\t\t\t}\n')
            faces = "\n".join(f"{a} {b} {c}" for a, b, c in tri)
            f.write("\t\t\tMESH_FACE_POSITION_LIST {\n" + faces + "\n\t\t\t}\n\t\t\tMESH_FACE_NORMAL_LIST {\n" + faces + "\n\t\t\t}\n")
            f.write("\t\t\tMESH_FACE_SHADING_LIST {\n" + "\n".join("0" for _ in tri) + "\n\t\t\t}\n")
            f.write("\t\t\tMODEL_POSITION_LIST {\n" + "\n".join(f"{x:.4f} {y:.4f} {z:.4f}" for x, y, z in pts) + "\n\t\t\t}\n")
            f.write("\t\t\tMODEL_NORMAL_LIST {\n" + "\n".join(f"{x:.5f} {y:.5f} {z:.5f}" for x, y, z in nrm) + "\n\t\t\t}\n\t\t}\n\t}\n")
        f.write("}\n\n")
        f.write(f'RESOURCE_LIST "SHADER" {{\n\tRESOURCE_COUNT {len(cols)}\n')
        for i, c in enumerate(cols):
            f.write(f'\tRESOURCE {i} {{\n\t\tRESOURCE_NAME "S{c[1:]}"\n\t\tSHADER_MATERIAL_NAME "M{c[1:]}"\n\t\tSHADER_ACTIVE_TEXTURE_COUNT 0\n\t}}\n')
        f.write(f'}}\n\nRESOURCE_LIST "MATERIAL" {{\n\tRESOURCE_COUNT {len(cols)}\n')
        for i, c in enumerate(cols):
            r, g, b = mc.to_rgb(c)
            f.write(f'\tRESOURCE {i} {{\n\t\tRESOURCE_NAME "M{c[1:]}"\n\t\tMATERIAL_AMBIENT {r * 0.45:.4f} {g * 0.45:.4f} {b * 0.45:.4f}\n\t\tMATERIAL_DIFFUSE {r:.4f} {g:.4f} {b:.4f}\n'
                    '\t\tMATERIAL_SPECULAR 0.150000 0.150000 0.150000\n\t\tMATERIAL_EMISSIVE 0.000000 0.000000 0.000000\n\t\tMATERIAL_REFLECTIVITY 0.100000\n\t\tMATERIAL_OPACITY 1.000000\n\t}\n')
        f.write("}\n\n")
        for n in names:
            f.write(f'MODIFIER "SHADING" {{\n\tMODIFIER_NAME "{n}"\n\tPARAMETERS {{\n\t\tSHADER_LIST_COUNT 1\n\t\tSHADER_LIST_LIST {{\n\t\t\tSHADER_LIST 0 {{\n'
                    f'\t\t\t\tSHADER_COUNT 1\n\t\t\t\tSHADER_NAME_LIST {{\n\t\t\t\t\tSHADER 0 NAME: "S{M.color_of(n)[1:]}"\n\t\t\t\t}}\n\t\t\t}}\n\t\t}}\n\t}}\n}}\n\n')
    return stats

def collect():
    """Corpi dell'assieme e gruppo di appartenenza di ciascuno."""
    P, G = {}, {}
    for fn, ref_group in ((M.hydro_bodies, REF_GROUPS[1]), (M.tank_bodies, REF_GROUPS[0]), (M.stand_bodies, REF_GROUPS[2]), (M.electronics_bodies, REF_GROUPS[2])):
        for n, p in fn().items():
            P[n] = p
            G[n] = ref_group if n.startswith("REF_") else next(g for g, t in GROUPS if t(n))
    return P, G

# ------------------------------------------------------------------ viste predefinite e PDF
COO = (210.0, 250.0, 105.0)
def views(P, G):
    """Testo del file delle viste (formato del pacchetto media9): camera + corpi nascosti per ogni vista."""
    names = list(P)
    alt = "REF_rullo_120_alternativa"; cas = "REF_caricatore_135"
    tank_ref = [n for n in names if G[n] == REF_GROUPS[0]]
    def hid(pred): return [n for n in names if pred(n)]
    covers = ["B02_coperchio_tank", "D05_carter_trasmissione", "D06_fianco_elettronica_SX", "D08_frontale_elettronica", "D09_plancia_display",
              "D10_tetto_elettronica", "D04_frontale_cavalletto", "REF_display_JC4880P443C"] + [n for n in names if n.startswith("D17_")]
    tank_only = lambda n: not (n[0] in "AB" or n in tank_ref)
    V = [("01 Vista generale", COO, (-0.55, -0.70, 0.46), 1350, [alt]),
         ("02 Fronte destra", COO, (0.60, -0.68, 0.42), 1350, [alt]),
         ("03 Retro", COO, (0.35, 0.80, 0.48), 1350, [alt]),
         ("04 Dall'alto", COO, (0.0, -0.02, 1.0), 1450, [alt]),
         ("05 Interno - senza coperchi e pannelli", COO, (-0.55, -0.70, 0.46), 1350, [alt] + covers),
         ("06 Solo bagno e idraulica", COO, (-0.30, -0.80, 0.52), 1350, hid(lambda n: not (n[0] == "C" or G[n] == REF_GROUPS[1]))),
         ("07 Tank e caricatore - 135", (X_T, 110.0, 175.0), (-0.35, -0.75, 0.56), 560, hid(tank_only) + [alt, "B02_coperchio_tank"]),
         ("08 Tank e caricatore - 120", (X_T, 110.0, 175.0), (-0.35, -0.75, 0.56), 560, hid(tank_only) + [cas, "B02_coperchio_tank"]),
         ("09 Solo pezzi stampati", COO, (-0.55, -0.70, 0.46), 1350, hid(lambda n: n.startswith("REF_"))),
         ("10 Solo componenti acquistati", COO, (-0.55, -0.70, 0.46), 1350, hid(lambda n: not n.startswith("REF_")))]
    out = []
    for name, coo, c2c, roo, hidden in V:
        out.append(f"VIEW={name}\n  BGCOLOR=1 1 1\n  LIGHTS=CAD\n  RENDERMODE=Solid\n  ROLL=0\n  C2C={c2c[0]} {c2c[1]} {c2c[2]}\n  COO={coo[0]} {coo[1]} {coo[2]}\n  ROO={roo}\n  AAC=30\n")
        for n in sorted(set(hidden)):
            out.append(f"  PART={n}\n    VISIBLE=false\n  END\n")
        out.append("END\n")
    return "".join(out), [v[0] for v in V]

from machine import X_T

def make_tex(n, ns, nr, view_names, poster):
    v = ", ".join(view_names)
    return r"""\documentclass[a4paper,landscape,10pt]{article}
\usepackage[T1]{fontenc}
\usepackage[utf8]{inputenc}
\usepackage[margin=9mm,top=8mm,bottom=8mm]{geometry}
\usepackage{graphicx}
\usepackage{media9}
\usepackage[pdftitle={FilMachine - assieme 3D},pdfauthor={FilMachine}]{hyperref}
\pagestyle{empty}\setlength{\parindent}{0pt}\renewcommand{\familydefault}{\sfdefault}
\begin{document}
{\Large\bfseries FilMachine -- assieme 3D della macchina completa}\hfill{\small """ + f"{n} corpi: {ns} stampati, {nr} componenti acquistati" + r"""}\par\vspace{1.5mm}
\includemedia[
  width=279mm, height=160mm,
  activate=pageopen,
  3Dtoolbar, 3Dmenu, 3Dnavpane,
  3Dbg=1 1 1, 3Dlights=CAD, 3Drender=Solid,
  3Dpartsattrs=restore,
  3Dviews=viste.vws,
]{\includegraphics[width=279mm,height=160mm,keepaspectratio]{""" + poster + r"""}}{FilMachine.u3d}
\par\vspace{1.5mm}
{\small\textbf{Si apre con Adobe Acrobat Reader} (su computer; gli altri lettori PDF mostrano solo l'immagine fissa). Se compare la barra gialla, scegliere
\emph{Opzioni $\rightarrow$ Considera sempre affidabile il documento} e fare clic sul modello.
\textbf{Ruotare}: trascinare col tasto sinistro. \textbf{Spostare}: Ctrl + trascinare. \textbf{Zoom}: rotellina.
\textbf{Mostrare e nascondere i pezzi}: nell'\emph{albero del modello} a sinistra (pulsante nella barra 3D se non \`e aperto) togliere o mettere la spunta
a un pezzo o a un intero gruppo; tasto destro sul pezzo $\rightarrow$ \emph{Isola}, \emph{Nascondi}, \emph{Trasparente}.
\textbf{Viste pronte} (menu \emph{Viste} nella barra 3D): """ + v.replace("'", "'") + r""".}
\end{document}
"""

if __name__ == "__main__":
    t0 = time.time(); os.makedirs(WORK, exist_ok=True)
    P, G = collect()
    idtf = os.path.join(WORK, "FilMachine.idtf")
    stats = write_idtf(P, G, idtf)
    print(f"IDTF: {len(P)} corpi, {sum(s[1] for s in stats)} triangoli, {os.path.getsize(idtf) / 1e6:.1f} MB ({time.time() - t0:.0f} s)")
    env = dict(os.environ, U3D_LIBDIR=os.path.dirname(CONV), LD_LIBRARY_PATH=os.path.dirname(CONV))
    r = subprocess.run([CONV, "-input", "FilMachine.idtf", "-output", "FilMachine.u3d"], cwd=WORK, capture_output=True, text=True, env=env)
    u3d = os.path.join(WORK, "FilMachine.u3d")
    assert r.returncode == 0 and os.path.exists(u3d), (r.stdout + r.stderr)[-400:]
    print(f"U3D: {os.path.getsize(u3d) / 1e6:.2f} MB ({time.time() - t0:.0f} s)")
    vws, vnames = views(P, G)
    open(os.path.join(WORK, "viste.vws"), "w").write(vws)
    poster = "poster.png"
    viz.render(P, "_poster", azim=-128, elev=27, color_of=M.color_of, hide=("REF_rullo_120",), zoom=1.0, size=(2232, 1280))
    shutil.move(os.path.join(OUT, "png", "_poster.png"), os.path.join(WORK, poster))
    ns_ = sum(1 for n in P if not n.startswith("REF_"))
    open(os.path.join(WORK, "FilMachine_3D.tex"), "w").write(make_tex(len(P), ns_, len(P) - ns_, vnames, poster))
    for i in range(3):
        r = subprocess.run(["pdflatex", "-interaction=nonstopmode", "FilMachine_3D.tex"], cwd=WORK, capture_output=True, text=True, errors="replace")
    pdf = os.path.join(WORK, "FilMachine_3D.pdf")
    assert os.path.exists(pdf), r.stdout[-1500:]
    shutil.copy(pdf, os.path.join(OUT, "docs", "FilMachine_3D.pdf")); shutil.copy(u3d, os.path.join(OUT, "docs", "FilMachine.u3d"))
    print(f"PDF 3D: {os.path.getsize(pdf) / 1e6:.2f} MB ({time.time() - t0:.0f} s)")
    print("   avvisi LaTeX:", [l for l in r.stdout.splitlines() if "Warning" in l or l.startswith("!")][:6])
