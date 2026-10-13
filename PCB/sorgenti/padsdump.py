import sys, pcbnew
from placement import ORIGIN
b = pcbnew.LoadBoard(sys.argv[1])
want = sys.argv[2:]
for r in want:
    f = b.FindFootprintByReference(r)
    c = f.GetCourtyard(pcbnew.F_CrtYd).BBox()
    print(f"{r:4s} crtyd x {c.GetLeft()/1e6-ORIGIN[0]:6.2f}..{c.GetRight()/1e6-ORIGIN[0]:6.2f} y {c.GetTop()/1e6-ORIGIN[1]:6.2f}..{c.GetBottom()/1e6-ORIGIN[1]:6.2f}  rot {f.GetOrientationDegrees():5.1f}")
    for p in f.Pads():
        pos = p.GetPosition(); sz = p.GetSize()
        print(f"      pad {p.GetNumber():4s} {p.GetNetname():12s} at ({pos.x/1e6-ORIGIN[0]:6.2f},{pos.y/1e6-ORIGIN[1]:6.2f}) size {sz.x/1e6:.2f}x{sz.y/1e6:.2f}")
