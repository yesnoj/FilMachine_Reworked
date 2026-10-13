# -*- coding: utf-8 -*-
"""Manufacturing outputs for JLCPCB (PCB + SMT/THT assembly): Gerbers + drill (zip), BOM CSV, CPL CSV with the
JLCPCB rotation conventions, schematic/assembly PDFs, STEP, renders.  Usage: export.py <out_dir>"""
import sys, os, re, subprocess, csv, zipfile, collections, shutil
import pcbnew
import design

OUT = sys.argv[1]
PCB = f"{OUT}/FilMachine_Mainboard.kicad_pcb"
SCH = f"{OUT}/FilMachine_Mainboard.kicad_sch"
FAB = f"{OUT}/fab"
ROT_DB = "/home/claude/jlc/cpl_rotations_db.csv"
shutil.rmtree(FAB, ignore_errors=True)
os.makedirs(f"{FAB}/gerber", exist_ok=True)

def run(*a):
    r = subprocess.run(list(a), capture_output=True, text=True)
    if r.returncode:
        print("ERR", " ".join(a[:4]), r.stdout[-500:], r.stderr[-500:])
    return r

# ── Gerbers (Protel extensions, no X2 attributes, silk minus mask) + Excellon drill (PTH+NPTH in one file) ──
LAYERS = "F.Cu,In1.Cu,In2.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts"
run("kicad-cli", "pcb", "export", "gerbers", "-o", f"{FAB}/gerber/", "-l", LAYERS,
    "--no-x2", "--no-netlist", "--subtract-soldermask", PCB)
run("kicad-cli", "pcb", "export", "drill", "-o", f"{FAB}/gerber/", "--format", "excellon", "--drill-origin", "absolute",
    "--excellon-units", "mm", "--excellon-zeros-format", "decimal", "--generate-map", "--map-format", "gerberx2", PCB)
z = zipfile.ZipFile(f"{FAB}/FilMachine_Mainboard_Gerber.zip", "w", zipfile.ZIP_DEFLATED)
for fn in sorted(os.listdir(f"{FAB}/gerber")):
    z.write(f"{FAB}/gerber/{fn}", fn)
z.close()

# ── CPL: Designator, Mid X, Mid Y, Layer, Rotation ─────────────────────────────────────────────────
# Position: footprint anchor for SMD library parts (KiCad puts it at the body centre) and for the FM custom
# footprints (anchored at the body centre on purpose); courtyard centre for KiCad THT parts, whose anchor is pin 1.
# Rotation: KiCad angle + the per-footprint correction of the public JLCPCB rotation table (matthewlai), so the
# angle in the file follows the JLCPCB library orientation, not KiCad's.
rot_rules = []
for row in csv.reader(open(ROT_DB)):
    if not row or row[0].startswith("Footprint pattern") or row[0].startswith("#"):
        continue
    rot_rules.append((re.compile(row[0]), float(row[1])))
rot_rules += [(re.compile(r"^TI_SO-PowerPAD-8"), 270.0)]      # SOIC-8 with pad: same convention as SOIC
def jlc_rotation(fpname, kicad_rot):
    corr = 0.0
    for rx, r in rot_rules:
        if rx.search(fpname):
            corr = r; break
    return (kicad_rot + corr) % 360.0, corr

board = pcbnew.LoadBoard(PCB)
cpl_rows = []
nocorr = collections.OrderedDict()
for fp in board.GetFootprints():
    if fp.IsDNP() or fp.IsExcludedFromPosFiles():
        continue
    if fp.GetLayer() != pcbnew.F_Cu:
        raise SystemExit("bottom-side part: " + fp.GetReference())
    name = str(fp.GetFPID().GetLibItemName())
    lib = str(fp.GetFPID().GetLibNickname())
    tht = fp.GetAttributes() & pcbnew.FP_THROUGH_HOLE
    if tht and lib != "FM":
        c = fp.GetCourtyard(pcbnew.F_CrtYd).BBox()
        cx, cy = (c.GetLeft() + c.GetRight()) / 2e6, (c.GetTop() + c.GetBottom()) / 2e6
    else:
        pos = fp.GetPosition(); cx, cy = pos.x / 1e6, pos.y / 1e6
    rot, corr = jlc_rotation(name, fp.GetOrientationDegrees())
    if corr == 0.0:
        nocorr.setdefault(name, []).append(fp.GetReference())
    # KiCad pos files negate Y (Gerber coordinates grow upwards); do the same so CPL and Gerbers agree
    cpl_rows.append((fp.GetReference(), round(cx, 4), round(-cy, 4), "Top", round(rot, 1)))
def natkey(r):
    m = re.match(r"([A-Z]+)(\d+)", r)
    return (m.group(1), int(m.group(2))) if m else (r, 0)
cpl_rows.sort(key=lambda r: natkey(r[0]))
with open(f"{FAB}/FilMachine_Mainboard_CPL.csv", "w", newline="") as fh:
    w = csv.writer(fh)
    w.writerow(["Designator", "Mid X", "Mid Y", "Layer", "Rotation"])
    for r in cpl_rows:
        w.writerow(r)
cpl_refs = {r[0] for r in cpl_rows}

# ── BOM: Comment, Designator, Footprint, LCSC Part # (one line per LCSC part; DNP and mechanical excluded) ──
groups = collections.OrderedDict()
for p in design.PARTS:
    if p["sym"].startswith(("Mechanical", "Jumper")) or p["dnp"]:
        continue
    if not p.get("lcsc"):
        raise SystemExit("no LCSC code: " + p["ref"])
    groups.setdefault(p["lcsc"], []).append(p)
with open(f"{FAB}/FilMachine_Mainboard_BOM.csv", "w", newline="") as fh:
    w = csv.writer(fh)
    w.writerow(["Comment", "Designator", "Footprint", "LCSC Part #"])
    for lcsc, ps in sorted(groups.items(), key=lambda kv: natkey(kv[1][0]["ref"])):
        vals = sorted({q["value"] for q in ps})
        fps = sorted({q["fp"].split(":")[1] for q in ps})
        assert len(fps) == 1, (lcsc, fps)
        w.writerow([" / ".join(vals), ",".join(sorted((q["ref"] for q in ps), key=natkey)), fps[0], lcsc])
bom_refs = {p["ref"] for ps in groups.values() for p in ps}
print("BOM lines:", len(groups), "placed refs:", len(bom_refs), "CPL refs:", len(cpl_refs),
      "missing in CPL:", sorted(bom_refs - cpl_refs), "extra in CPL:", sorted(cpl_refs - bom_refs))
print("footprints without a rotation rule (0 deg correction):")
for k, v in nocorr.items():
    print("   ", k, "->", ",".join(sorted(v, key=natkey)))

# ── a human-readable BOM too (xlsx): value, designators, MPN, manufacturer, description, LCSC ──
try:
    import openpyxl
    from openpyxl.styles import Font, PatternFill, Alignment
    wb = openpyxl.Workbook(); ws = wb.active; ws.title = "BOM"
    ws.append(["Qty", "Designator", "Value", "Manufacturer", "MPN", "LCSC", "Footprint", "Description"])
    for c in ws[1]:
        c.font = Font(bold=True, color="FFFFFF"); c.fill = PatternFill("solid", fgColor="1F4E79")
    for lcsc, ps in sorted(groups.items(), key=lambda kv: natkey(kv[1][0]["ref"])):
        ws.append([len(ps), ",".join(sorted((q["ref"] for q in ps), key=natkey)), " / ".join(sorted({q["value"] for q in ps})),
                   ps[0]["mfr"], ps[0]["mpn"], lcsc, ps[0]["fp"].split(":")[1], ps[0]["desc"]])
    dnp = [p for p in design.PARTS if p["dnp"]]
    if dnp:
        ws.append([])
        ws.append(["NON MONTATI (DNP, non sono nella BOM JLCPCB):", ",".join(p["ref"] for p in dnp), "", "", "", "", "", ""])
    for i, w_ in enumerate([5, 40, 16, 20, 26, 12, 40, 70]):
        ws.column_dimensions[openpyxl.utils.get_column_letter(i + 1)].width = w_
    for row in ws.iter_rows(min_row=2):
        for c in row:
            c.alignment = Alignment(wrap_text=True, vertical="top")
    wb.save(f"{FAB}/FilMachine_Mainboard_BOM_leggibile.xlsx")
except ImportError:
    pass

# ── documentation ──
run("kicad-cli", "sch", "export", "pdf", "-o", f"{FAB}/FilMachine_Mainboard_Schema.pdf", SCH)
run("kicad-cli", "pcb", "export", "pdf", "-o", f"{FAB}/FilMachine_Mainboard_Assembly_Top.pdf",
    "-l", "F.Fab,F.Silkscreen,Edge.Cuts", "--include-border-title", PCB)
run("kicad-cli", "pcb", "export", "step", "-o", f"{FAB}/FilMachine_Mainboard.step", "--subst-models", "--force", PCB)
# renders: 2D top/bottom (copper + silk) and a 3D view
def svgpng(layers, out, w=2400):
    svg = f"{FAB}/_r.svg"
    run("kicad-cli", "pcb", "export", "svg", "--mode-single", "--fit-page-to-board", "--exclude-drawing-sheet", "-l", layers, "-o", svg, PCB)
    run("rsvg-convert", "-w", str(w), "-b", "white", svg, "-o", out)
    if os.path.exists(svg): os.remove(svg)
svgpng("F.Cu,F.SilkS,F.Mask,Edge.Cuts", f"{FAB}/FilMachine_Mainboard_top.png")
svgpng("B.Cu,B.SilkS,Edge.Cuts", f"{FAB}/FilMachine_Mainboard_bottom.png")
run("xvfb-run", "-a", "kicad-cli", "pcb", "render", "-o", f"{FAB}/FilMachine_Mainboard_3D.png", "--side", "top",
    "--background", "opaque", "--quality", "high", "-w", "1800", "-h", "1500", "--zoom", "1.1", PCB)
print("export done:", sorted(os.listdir(FAB)))
