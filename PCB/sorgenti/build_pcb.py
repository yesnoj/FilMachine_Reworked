# -*- coding: utf-8 -*-
"""Build FilMachine_Mainboard.kicad_pcb from design.py + placement.py (footprints, nets, outline, rules)."""
import sys, json, os
import pcbnew
import design
from placement import PLACE, BOARD_W, BOARD_H, ORIGIN

OUT = sys.argv[1] if len(sys.argv) > 1 else "out"
FPDIR = "/usr/share/kicad/footprints"
MM = pcbnew.FromMM

uu = json.load(open(f"{OUT}/uuids.json"))
board = pcbnew.BOARD()
board.SetCopperLayerCount(4)

# ---------------------------------------------------------------- nets
nets = {}
def net(n):
    if n not in nets:
        ni = pcbnew.NETINFO_ITEM(board, n)
        board.Add(ni)
        nets[n] = ni
    return nets[n]
for n in sorted(design.nets()):
    net(n)

# ---------------------------------------------------------------- footprints
def place_fp(fp, x, y, rot, side):
    """(x, y) = desired TOP-LEFT corner of the rotated courtyard bbox, in board coords."""
    fp.SetPosition(pcbnew.VECTOR2I(0, 0))
    fp.SetOrientationDegrees(rot)
    if side == "B":
        fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    cy = fp.GetCourtyard(pcbnew.B_CrtYd if side == "B" else pcbnew.F_CrtYd)
    bb = cy.BBox() if cy.OutlineCount() else fp.GetBoundingBox(False)
    dx = MM(ORIGIN[0] + x) - bb.GetLeft()
    dy = MM(ORIGIN[1] + y) - bb.GetTop()
    fp.SetPosition(pcbnew.VECTOR2I(dx, dy))

fps = {}
for p in design.PARTS:
    lib, name = p["fp"].split(":")
    fp = pcbnew.FootprintLoad(os.path.join(os.path.dirname(os.path.abspath(__file__)), "FM.pretty") if lib == "FM" else f"{FPDIR}/{lib}.pretty", name)
    if fp is None:
        raise SystemExit("missing fp " + p["fp"])
    fp.SetReference(p["ref"])
    fp.SetValue(p["value"])
    fp.SetFPIDAsString(p["fp"])
    sheet_u = uu["sheets"][p["sheet"]]
    sym_u = uu["symbols"][p["ref"]]
    fp.SetPath(pcbnew.KIID_PATH(f"/{sheet_u}/{sym_u}"))
    fp.SetSheetname(dict(design.SHEETS)[p["sheet"]])
    fp.SetSheetfile(f"{p['sheet']}.kicad_sch")
    for fld, val in (("MPN", p["mpn"]), ("Manufacturer", p["mfr"]), ("Description", p["desc"]), ("LCSC", p.get("lcsc", ""))):
        fp.SetField(fld, val)
        f = fp.GetFieldByName(fld); f.SetVisible(False); f.SetLayer(pcbnew.F_Fab)
    if p["dnp"]:
        fp.SetDNP(True); fp.SetExcludedFromBOM(True); fp.SetExcludedFromPosFiles(True)
    if p["sym"].startswith("Mechanical"):
        fp.SetExcludedFromBOM(True); fp.SetExcludedFromPosFiles(True)
    for pad in fp.Pads():
        num = pad.GetNumber()
        # 23/10/2026: through-hole pads get thermal reliefs (wave soldering on 4 layers with 2 oz outer copper), with
        # wide spokes where current flows: 1.0 mm on the 12 V inputs / heater / pump terminals, 0.8 mm on the
        # electrolytics, 0.6 mm elsewhere (the zones give the 0.3 mm gap). J6 (not mounted) keeps solid GND.
        if pad.GetAttribute() == pcbnew.PAD_ATTRIB_PTH and num in p["pins"] and p["pins"][num] is not None:
            if p["ref"] == "J6" and p["pins"][num] == "GND":
                pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
            else:
                pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_THERMAL)
                pad.SetLocalThermalSpokeWidthOverride(MM(1.0 if p["ref"] in ("J1", "J2", "J3", "J23", "J24", "J25")
                                                         else 0.8 if p["ref"] in ("C1", "C55", "C56") else 0.6))
        if name.startswith("TI_SO-PowerPAD") and num == "9":
            pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
        if num in p["pins"] and p["pins"][num] is not None:
            pad.SetNet(net(p["pins"][num]))
    board.Add(fp)
    fps[p["ref"]] = fp

# fixed placements
for ref, (x, y, rot, side) in PLACE.items():
    place_fp(fps[ref], x, y, rot, side)
# block packing (row-wise, courtyard based, 0.25 mm gap)
from placement import BLOCKS
GAP = 0.25
placed_in_block = set()
for (x0, y0, x1, y1), refs, rot in BLOCKS:
    cx, cy, rowh = x0, y0, 0.0
    for ref in refs:
        fp = fps[ref]
        place_fp(fp, 0, 0, rot, "F")
        c = fp.GetCourtyard(pcbnew.F_CrtYd)
        bb = c.BBox() if c.OutlineCount() else fp.GetBoundingBox(False)
        w, h = bb.GetWidth() / 1e6, bb.GetHeight() / 1e6
        if cx + w > x1 + 1e-6:
            cx = x0; cy += rowh + GAP; rowh = 0
        if cy + h > y1 + 1e-6:
            raise SystemExit(f"block overflow at {ref} in {(x0, y0, x1, y1)}")
        place_fp(fp, cx, cy, rot, "F")
        placed_in_block.add(ref)
        cx += w + GAP; rowh = max(rowh, h)
missing = [r for r in fps if r not in PLACE and r not in placed_in_block]
if missing:
    raise SystemExit("unplaced: " + " ".join(missing))

# ---------------------------------------------------------------- outline (rounded rectangle, r = 3 mm)
def seg(a, b, layer=pcbnew.Edge_Cuts, w=0.1):
    s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(pcbnew.VECTOR2I(MM(ORIGIN[0] + a[0]), MM(ORIGIN[1] + a[1])))
    s.SetEnd(pcbnew.VECTOR2I(MM(ORIGIN[0] + b[0]), MM(ORIGIN[1] + b[1])))
    s.SetLayer(layer); s.SetWidth(MM(w)); board.Add(s)
def arc(c, start, ang):
    s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_ARC)
    s.SetCenter(pcbnew.VECTOR2I(MM(ORIGIN[0] + c[0]), MM(ORIGIN[1] + c[1])))
    s.SetStart(pcbnew.VECTOR2I(MM(ORIGIN[0] + start[0]), MM(ORIGIN[1] + start[1])))
    s.SetArcAngleAndEnd(pcbnew.EDA_ANGLE(ang, pcbnew.DEGREES_T), True)
    s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(MM(0.1)); board.Add(s)
W, H, r = BOARD_W, BOARD_H, 3.0
seg((r, 0), (W - r, 0)); seg((W, r), (W, H - r)); seg((W - r, H), (r, H)); seg((0, H - r), (0, r))
arc((W - r, r), (W - r, 0), 90); arc((W - r, H - r), (W, H - r), 90)
arc((r, H - r), (r, H), 90); arc((r, r), (0, r), 90)

# ---------------------------------------------------------------- design rules (PCBWay 4L, 2 oz outer)
ds = board.GetDesignSettings()
ds.m_TrackMinWidth = MM(0.2)
ds.m_MinClearance = MM(0.2)
ds.m_ViasMinSize = MM(0.6)
ds.m_MinThroughDrill = MM(0.3)
ds.m_HoleClearance = MM(0.25)
ds.m_CopperEdgeClearance = MM(0.4)
ds.m_MinResolvedSpokes = 1          # signal GND pins may get a single spoke on F/B (plus the In1 plane)
ds.SetBoardThickness(MM(1.6))

pcbnew.SaveBoard(f"{OUT}/FilMachine_Mainboard.kicad_pcb", board)
print("saved", len(fps), "footprints")

# stackup (2 oz outer / 1 oz inner, 1.6 mm) written into the setup section: not exposed by the Python API
STACKUP = """(stackup
			(layer "F.SilkS" (type "Top Silk Screen"))
			(layer "F.Paste" (type "Top Solder Paste"))
			(layer "F.Mask" (type "Top Solder Mask") (thickness 0.01))
			(layer "F.Cu" (type "copper") (thickness 0.07))
			(layer "dielectric 1" (type "prepreg") (thickness 0.2) (material "FR4") (epsilon_r 4.5) (loss_tangent 0.02))
			(layer "In1.Cu" (type "copper") (thickness 0.035))
			(layer "dielectric 2" (type "core") (thickness 0.97) (material "FR4") (epsilon_r 4.5) (loss_tangent 0.02))
			(layer "In2.Cu" (type "copper") (thickness 0.035))
			(layer "dielectric 3" (type "prepreg") (thickness 0.2) (material "FR4") (epsilon_r 4.5) (loss_tangent 0.02))
			(layer "B.Cu" (type "copper") (thickness 0.07))
			(layer "B.Mask" (type "Bottom Solder Mask") (thickness 0.01))
			(layer "B.Paste" (type "Bottom Solder Paste"))
			(layer "B.SilkS" (type "Bottom Silk Screen"))
			(copper_finish "HAL lead-free")
			(dielectric_constraints no)
		)"""
fn = f"{OUT}/FilMachine_Mainboard.kicad_pcb"
s = open(fn).read()
if "(stackup" not in s:
    s = s.replace("(setup", "(setup\n\t\t" + STACKUP, 1)
    open(fn, "w").write(s)
