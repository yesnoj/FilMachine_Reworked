# -*- coding: utf-8 -*-
"""Manufacturing outputs for PCBWay: Gerbers + drill, centroid, BOM (PCBWay template), PDFs, STEP."""
import sys, os, subprocess, csv, zipfile, collections, shutil
import openpyxl
from openpyxl.styles import Font, PatternFill, Alignment
import design

OUT = sys.argv[1]
PCB = f"{OUT}/FilMachine_Mainboard.kicad_pcb"
SCH = f"{OUT}/FilMachine_Mainboard.kicad_sch"
FAB = f"{OUT}/fab"
shutil.rmtree(FAB, ignore_errors=True)
os.makedirs(f"{FAB}/gerber", exist_ok=True)

def run(*a):
    r = subprocess.run(list(a), capture_output=True, text=True)
    if r.returncode:
        print("ERR", " ".join(a[:4]), r.stdout[-500:], r.stderr[-500:])
    return r

LAYERS = "F.Cu,In1.Cu,In2.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts"
run("kicad-cli", "pcb", "export", "gerbers", "-o", f"{FAB}/gerber/", "-l", LAYERS,
    "--no-x2", "--subtract-soldermask", PCB)
run("kicad-cli", "pcb", "export", "drill", "-o", f"{FAB}/gerber/", "--format", "excellon", "--drill-origin", "absolute",
    "--excellon-units", "mm", "--excellon-separate-th", "--generate-map", "--map-format", "gerberx2", PCB)
z = zipfile.ZipFile(f"{FAB}/FilMachine_Mainboard_Gerber.zip", "w", zipfile.ZIP_DEFLATED)
for fn in sorted(os.listdir(f"{FAB}/gerber")):
    z.write(f"{FAB}/gerber/{fn}", fn)
z.close()

# centroid (top only — single-sided assembly)
run("kicad-cli", "pcb", "export", "pos", "-o", f"{FAB}/pos_raw.csv", "--format", "csv", "--units", "mm",
    "--side", "front", "--exclude-dnp", PCB)
rows = list(csv.DictReader(open(f"{FAB}/pos_raw.csv")))
with open(f"{FAB}/FilMachine_Mainboard_CPL.csv", "w", newline="") as fh:
    w = csv.writer(fh)
    w.writerow(["Designator", "Mid X(mm)", "Mid Y(mm)", "Layer", "Rotation"])
    for r in rows:
        w.writerow([r["Ref"], r["PosX"], r["PosY"], "Top" if r["Side"] == "top" else "Bottom", r["Rot"]])
os.remove(f"{FAB}/pos_raw.csv")
cpl_refs = {r["Ref"] for r in rows}

# BOM in PCBWay layout
THT_PREFIX = ("TerminalBlock", "Fuseholder", "CP_Radial", "USB_A", "USB_C", "IDC-Header", "JST_XH", "PinHeader")
groups = collections.OrderedDict()
for p in design.PARTS:
    if p["sym"].startswith(("Mechanical", "Jumper")):
        continue
    key = (p["value"], p["fp"], p["mpn"], p["dnp"])
    groups.setdefault(key, []).append(p)
wb = openpyxl.Workbook(); ws = wb.active; ws.title = "BOM"
hdr = ["Item #", "*Designator", "*Qty", "Manufacturer", "*Mfg Part #", "Description / Value", "*Package/Footprint",
       "Type", "Your Instructions / Notes"]
ws.append(hdr)
for c in ws[1]:
    c.font = Font(bold=True, color="FFFFFF"); c.fill = PatternFill("solid", fgColor="1F4E79")
    c.alignment = Alignment(wrap_text=True, vertical="center")
item = 0
def natkey(r):
    import re
    m = re.match(r"([A-Z]+)(\d+)", r)
    return (m.group(1), int(m.group(2))) if m else (r, 0)
for (val, fp, mpn, dnp), ps in sorted(groups.items(), key=lambda kv: (kv[0][3], natkey(kv[1][0]["ref"]))):
    item += 1
    refs = ",".join(sorted((q["ref"] for q in ps), key=natkey))
    fpn = fp.split(":")[1]
    typ = "THT" if fpn.startswith(THT_PREFIX) else "SMD"
    note = ps[0]["desc"]
    if dnp:
        note = "DO NOT PLACE — " + note
    if fpn.startswith("Fuseholder"):
        note += " | I fusibili sono le due voci in fondo alla distinta: vanno INSERITI in questi portafusibili"
    mfr = ps[0]["mfr"]
    if mpn.startswith(("0603 ", "MLCC ")):
        note = (note + " | " if note else "") + "Equivalente generico accettato (tolleranza 1% R, X7R/X5R C, tensione >= indicata)"
    ws.append([item, refs, 0 if dnp else len(ps), mfr, mpn, val, fpn, typ, note])
# Fusibili mini-blade: non hanno footprint (si innestano nei portafusibili Keystone 3568), ma la scheda deve
# arrivare pronta: PCBWay li fornisce e li inserisce a mano dopo la saldatura (nessuna saldatura sul fusibile).
FUSES = [("F1,F2,F3", 3, "Littelfuse", "0297015.WXNV", "Fusibile MINI (ATM) 15 A 32 V",
          "F1, F2 = riscaldatori; F3 = pompa"),
         ("F4", 1, "Littelfuse", "0297007.WXNV", "Fusibile MINI (ATM) 7,5 A 32 V",
          "F4 = valvole, motore, logica")]
for refs, qty, mfr, mpn, val, where in FUSES:
    item += 1
    ws.append([item, refs, qty, mfr, mpn, val, "Mini blade fuse (in Keystone 3568)", "Hand insert",
               "INSERIRE A MANO nel portafusibile del designator indicato, dopo la saldatura (premere fino in "
               "fondo; nessuna saldatura). " + where + ". Non lasciare fusibili sciolti nella confezione."])
widths = [7, 34, 6, 22, 24, 18, 44, 7, 80]
for i, w_ in enumerate(widths):
    ws.column_dimensions[openpyxl.utils.get_column_letter(i + 1)].width = w_
for row in ws.iter_rows(min_row=2):
    for c in row:
        c.alignment = Alignment(wrap_text=True, vertical="top")
wb.save(f"{FAB}/FilMachine_Mainboard_BOM.xlsx")

# consistency: every placed BOM part has a centroid line and vice versa
bom_refs = {p["ref"] for p in design.PARTS if not p["dnp"] and not p["sym"].startswith(("Mechanical", "Jumper"))}
print("BOM placed refs:", len(bom_refs), "CPL refs:", len(cpl_refs),
      "missing in CPL:", sorted(bom_refs - cpl_refs), "extra in CPL:", sorted(cpl_refs - bom_refs))

# documentation
run("kicad-cli", "sch", "export", "pdf", "-o", f"{FAB}/FilMachine_Mainboard_Schema.pdf", SCH)
run("kicad-cli", "pcb", "export", "pdf", "-o", f"{FAB}/FilMachine_Mainboard_Assembly_Top.pdf",
    "-l", "F.Fab,F.Silkscreen,Edge.Cuts", "--include-border-title", PCB)
run("kicad-cli", "pcb", "export", "step", "-o", f"{FAB}/FilMachine_Mainboard.step", "--subst-models", "--force", PCB)
print("export done:", sorted(os.listdir(FAB)))
