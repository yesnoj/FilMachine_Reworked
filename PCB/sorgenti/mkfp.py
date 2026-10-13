# -*- coding: utf-8 -*-
"""Custom footprints for the LCSC/JLCPCB parts that have no exact KiCad library footprint.
Writes FM.pretty/*.kicad_mod (dimensions from the LCSC datasheets, see comments)."""
import os, pcbnew
MM = pcbnew.FromMM
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "FM.pretty")
os.makedirs(OUT, exist_ok=True)

def new(name, descr, smd):
    fp = pcbnew.FOOTPRINT(pcbnew.BOARD())
    fp.SetFPIDAsString("FM:" + name)
    fp.SetLibDescription(descr)
    fp.SetAttributes(pcbnew.FP_SMD if smd else pcbnew.FP_THROUGH_HOLE)
    fp.Reference().SetText("REF**"); fp.Reference().SetPosition(pcbnew.VECTOR2I(0, MM(-1)))
    fp.Value().SetText(name); fp.Value().SetLayer(pcbnew.F_Fab)
    return fp

def pad_tht(fp, num, x, y, drill, dia, square=False, oblong=None):
    p = pcbnew.PAD(fp)
    p.SetNumber(str(num)); p.SetAttribute(pcbnew.PAD_ATTRIB_PTH)
    p.SetPosition(pcbnew.VECTOR2I(MM(x), MM(y)))
    if oblong:  # (hole_w, hole_h, pad_w, pad_h)
        hw, hh, pw, ph = oblong
        p.SetShape(pcbnew.PAD_SHAPE_OVAL); p.SetDrillShape(pcbnew.PAD_DRILL_SHAPE_OBLONG)
        p.SetDrillSize(pcbnew.VECTOR2I(MM(hw), MM(hh))); p.SetSize(pcbnew.VECTOR2I(MM(pw), MM(ph)))
    else:
        p.SetShape(pcbnew.PAD_SHAPE_RECT if square else pcbnew.PAD_SHAPE_CIRCLE)
        p.SetDrillSize(pcbnew.VECTOR2I(MM(drill), MM(drill))); p.SetSize(pcbnew.VECTOR2I(MM(dia), MM(dia)))
    p.SetLayerSet(pcbnew.PAD.PTHMask())
    fp.Add(p); return p

def pad_smd(fp, num, x, y, w, h, rr=0.2):
    p = pcbnew.PAD(fp)
    p.SetNumber(str(num)); p.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
    p.SetShape(pcbnew.PAD_SHAPE_ROUNDRECT); p.SetRoundRectRadiusRatio(rr)
    p.SetPosition(pcbnew.VECTOR2I(MM(x), MM(y))); p.SetSize(pcbnew.VECTOR2I(MM(w), MM(h)))
    p.SetLayerSet(pcbnew.PAD.SMDMask())
    fp.Add(p); return p

def rect(fp, x0, y0, x1, y1, layer, w=0.1):
    pts = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
    for (ax, ay), (bx, by) in zip(pts, pts[1:] + pts[:1]):
        line(fp, ax, ay, bx, by, layer, w)

def line(fp, ax, ay, bx, by, layer, w=0.1):
    s = pcbnew.PCB_SHAPE(fp, pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(pcbnew.VECTOR2I(MM(ax), MM(ay))); s.SetEnd(pcbnew.VECTOR2I(MM(bx), MM(by)))
    s.SetLayer(layer); s.SetWidth(MM(w)); fp.Add(s)

def text(fp, s, x, y, layer, size=0.8):
    t = pcbnew.PCB_TEXT(fp); t.SetText(s); t.SetPosition(pcbnew.VECTOR2I(MM(x), MM(y)))
    t.SetLayer(layer); t.SetTextSize(pcbnew.VECTOR2I(MM(size), MM(size))); t.SetTextThickness(MM(0.12)); fp.Add(t)

def save(fp):
    io = pcbnew.PCB_IO_MGR.PluginFind(pcbnew.PCB_IO_MGR.KICAD_SEXP)
    io.FootprintSave(OUT, fp)   # writes <name>.kicad_mod
    print("wrote", fp.GetFPIDAsString())

# ── Ningbo Kangnex WJ500V-5.08-2P (LCSC C8465): KF301-style screw terminal, pitch 5.08, pin 0.9x0.8, hole 1.5 ──
# PCB layout drawing: holes 3.14 mm from the rear base edge, 4.50 mm from the front (wire entry) edge; top overhang
# brings the total depth to 10.0 mm (5.5 behind the pins). Body N x 5.08 wide, 14.07 high. Front = local +y.
fp = new("TB_WJ500V-5.08-2P", "Ningbo Kangnex WJ500V-5.08-2P screw terminal, 2 pins, pitch 5.08 mm, 18 A, LCSC C8465; wire entry +y", False)
# footprint origin = body centre (what JLCPCB expects as "Mid X/Y"); pins at (-2.54, 0.5) and (2.54, 0.5)
pad_tht(fp, 1, -2.54, 0.5, 1.5, 2.7, square=True); pad_tht(fp, 2, 2.54, 0.5, 1.5, 2.7)
rect(fp, -5.08, -5.0, 5.08, 5.0, pcbnew.F_Fab)
rect(fp, -5.08 - 0.11, -5.0 - 0.11, 5.08 + 0.11, 5.0 + 0.11, pcbnew.F_SilkS, 0.12)
line(fp, -5.08, 5.0, 5.08, 5.0, pcbnew.F_SilkS, 0.3)           # wire-entry face
line(fp, -5.08, -5.0, 5.08, -5.0, pcbnew.F_SilkS, 0.12)
rect(fp, -5.34, -5.25, 5.36, 5.25, pcbnew.F_CrtYd, 0.05)
text(fp, "1", -4.54, -2.7, pcbnew.F_Fab, 0.7)
save(fp)

# ── DORABO DB301V-3.5-2P (LCSC C695629): screw terminal, pitch 3.5, pin 0.8, hole 1.0 (we use 1.1), body 7.0 x 6.9 x 8.5 ──
# pins 3.5 mm from the front (wire entry) face, 3.4 from the rear. Front = local +y.
fp = new("TB_DB301V-3.5-2P", "DORABO DB301V-3.5-2P screw terminal, 2 pins, pitch 3.5 mm, 7 A, LCSC C695629; wire entry +y", False)
# footprint origin = body centre; pins at (-1.75, -0.05) and (1.75, -0.05)
pad_tht(fp, 1, -1.75, -0.05, 1.1, 2.1, square=True); pad_tht(fp, 2, 1.75, -0.05, 1.1, 2.1)
rect(fp, -3.5, -3.45, 3.5, 3.45, pcbnew.F_Fab)
rect(fp, -3.61, -3.56, 3.61, 3.56, pcbnew.F_SilkS, 0.12)
line(fp, -3.5, 3.45, 3.5, 3.45, pcbnew.F_SilkS, 0.3)
rect(fp, -3.75, -3.7, 3.75, 3.7, pcbnew.F_CrtYd, 0.05)
save(fp)

# ── SHOU HAN TYPE-C 6P (LCSC C456012): power-only USB-C receptacle, SMD pins + 4 THT shell slots ──
# Recommended layout: pads 1.0 long on one row (y=0): B12(-2.70, 0.8 wide) B9(-1.50, 0.7) A5(-0.50, 0.7) B5(0.50, 0.7)
# A9(1.50, 0.7) A12(2.70, 0.8); shell slots 0.6x1.4 (pad 1.0x1.8) at x=+-4.32, y=0.45 and y=4.25; product edge at y=6.85.
fp = new("USBC_6P_SHOUHAN", "SHOU HAN TYPE-C 6P power-only USB-C receptacle (LCSC C456012), 6 SMD pads + 4 shell slots; mouth at +y (6.85 mm from the pad row)", True)
# footprint origin = body centre (pad row 3.525 mm behind it); mouth at +3.325
OY = -3.525
for num, x, w in (("B12", -2.70, 0.8), ("B9", -1.50, 0.7), ("A5", -0.50, 0.7), ("B5", 0.50, 0.7), ("A9", 1.50, 0.7), ("A12", 2.70, 0.8)):
    pad_smd(fp, num, x, 0 + OY, w, 1.0, 0.25)
for x in (-4.32, 4.32):
    for y in (0.45, 4.25):
        pad_tht(fp, "S1", x, y + OY, 0, 0, oblong=(0.6, 1.4, 1.0, 1.8))
rect(fp, -4.47, 0.20 + OY, 4.47, 6.85 + OY, pcbnew.F_Fab)
line(fp, -4.6, 1.6 + OY, -4.6, 3.1 + OY, pcbnew.F_SilkS, 0.12); line(fp, 4.6, 1.6 + OY, 4.6, 3.1 + OY, pcbnew.F_SilkS, 0.12)
line(fp, -4.6, 5.4 + OY, -4.6, 6.85 + OY, pcbnew.F_SilkS, 0.12); line(fp, 4.6, 5.4 + OY, 4.6, 6.85 + OY, pcbnew.F_SilkS, 0.12)
rect(fp, -4.75, -0.9 + OY, 4.75, 7.1 + OY, pcbnew.F_CrtYd, 0.05)
# pin-1 style marker not meaningful; reference text above the pads
save(fp)

# ── Sunltech SLO0630H100MTT (LCSC C207842): 7.1 x 6.6 x 3.0 molded inductor; land pattern 3.0 x 3.6 pads, outer span 7.5 ──
fp = new("L_SLO0630H", "Sunltech SLO0630H molded power inductor 7.1x6.6x3.0 mm (LCSC C207842); pads 3.0x3.6, centres +-2.25", True)
pad_smd(fp, 1, -2.25, 0, 3.0, 3.6, 0.1); pad_smd(fp, 2, 2.25, 0, 3.0, 3.6, 0.1)
rect(fp, -3.55, -3.3, 3.55, 3.3, pcbnew.F_Fab)
line(fp, -3.66, -3.41, 3.66, -3.41, pcbnew.F_SilkS, 0.12); line(fp, -3.66, 3.41, 3.66, 3.41, pcbnew.F_SilkS, 0.12)
rect(fp, -4.0, -3.55, 4.0, 3.55, pcbnew.F_CrtYd, 0.05)
save(fp)

# ── 2410 SMD fuse (JDT JFC2410 series, LCSC C136393 / C136390): body 6.1 x 2.5, pads 2.5 x 3.0, outer span 8.0 ──
fp = new("FUSE_2410_SMD", "SMD fuse 2410 (6.1x2.5 mm), JDT JFC2410 recommended land: pads 2.5x3.0 mm, centres +-2.75 (outer span 8.0)", True)
pad_smd(fp, 1, -2.75, 0, 2.5, 3.0, 0.1); pad_smd(fp, 2, 2.75, 0, 2.5, 3.0, 0.1)
rect(fp, -3.05, -1.25, 3.05, 1.25, pcbnew.F_Fab)
line(fp, -1.3, -1.6, 1.3, -1.6, pcbnew.F_SilkS, 0.12); line(fp, -1.3, 1.6, 1.3, 1.6, pcbnew.F_SilkS, 0.12)
rect(fp, -4.25, -1.75, 4.25, 1.75, pcbnew.F_CrtYd, 0.05)
save(fp)
