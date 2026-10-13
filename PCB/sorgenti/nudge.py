# -*- coding: utf-8 -*-
"""Fix the grid-rounding clearance violations the maze router leaves (actual >= required - 0.03 mm) by moving
the offending via, or the offending track segment (with everything attached to its ends), 0.04 mm away from the
other item. Re-runs DRC until clean or no progress. Usage: nudge.py board.kicad_pcb"""
import sys, os, json, math, subprocess, re
import pcbnew
PCB = sys.argv[1]
MM = pcbnew.FromMM
REP = PCB.replace(".kicad_pcb", "_drc.json")

def drc():
    subprocess.run(["kicad-cli", "pcb", "drc", "--format", "json", "--severity-all", "-o", REP, PCB], capture_output=True)
    d = json.load(open(REP))
    out = []
    for v in d["violations"]:
        if v["type"] not in ("clearance", "hole_clearance"): continue
        m = re.search(r"clearance ([0-9.]+) mm; actual ([0-9.]+) mm", v["description"])
        if not m: continue
        req, act = float(m.group(1)), float(m.group(2))
        if req - act > 0.03: continue
        out.append((v["items"], req - act))
    return out

def nearest_on_segment(s, e, p):
    dx, dy = e.x - s.x, e.y - s.y; L2 = dx * dx + dy * dy
    t = max(0.0, min(1.0, ((p.x - s.x) * dx + (p.y - s.y) * dy) / L2)) if L2 else 0.0
    return pcbnew.VECTOR2I(int(s.x + t * dx), int(s.y + t * dy))

for _round in range(6):
    viol = drc()
    if not viol:
        print("nudge: clean"); break
    b = pcbnew.LoadBoard(PCB)
    tracks = [t for t in b.GetTracks() if t.GetClass() == "PCB_TRACK"]
    vias = [t for t in b.GetTracks() if t.GetClass() == "PCB_VIA"]
    pads = [p for f in b.GetFootprints() for p in f.Pads()]
    fixed = 0
    for items, short in viol:
        descs = [it["description"] for it in items]
        poss = [pcbnew.VECTOR2I(MM(it["pos"]["x"]), MM(it["pos"]["y"])) for it in items]
        def find(desc, pos):
            if desc.startswith("Via"):
                return min(vias, key=lambda v: (v.GetPosition() - pos).EuclideanNorm())
            if desc.startswith("Track"):
                m = re.search(r"\[(.*?)\]", desc); net = m.group(1) if m else ""
                cand = [t for t in tracks if t.GetNetname() == net]
                return min(cand, key=lambda t: (nearest_on_segment(t.GetStart(), t.GetEnd(), pos) - pos).EuclideanNorm()) if cand else None
            if desc.startswith("Pad"):
                m = re.match(r"Pad (\S+) \[.*?\] of (\S+)", desc)
                if m:
                    for p in pads:
                        if p.GetParentFootprint().GetReference() == m.group(2) and p.GetNumber() == m.group(1): return p
            return None
        a, c = find(descs[0], poss[0]), find(descs[1], poss[1])
        if a is None or c is None: continue
        # choose what to move: a via if present, else a track (never a pad)
        mover, other = (a, c) if a.GetClass() == "PCB_VIA" else ((c, a) if c.GetClass() == "PCB_VIA" else ((a, c) if a.GetClass() == "PCB_TRACK" else (c, a)))
        if mover.GetClass() not in ("PCB_VIA", "PCB_TRACK"): continue
        # direction: from the other item's nearest point to the mover
        if mover.GetClass() == "PCB_VIA":
            mp = mover.GetPosition()
        else:
            opos = other.GetPosition() if other.GetClass() != "PCB_TRACK" else nearest_on_segment(other.GetStart(), other.GetEnd(), mover.GetStart())
            mp = nearest_on_segment(mover.GetStart(), mover.GetEnd(), opos)
        if other.GetClass() == "PCB_TRACK":
            op = nearest_on_segment(other.GetStart(), other.GetEnd(), mp)
        else:
            op = other.GetPosition()
        d = (mp - op).EuclideanNorm()
        if d == 0: continue
        ux, uy = (mp.x - op.x) / d, (mp.y - op.y) / d
        sh = MM(0.04)
        delta = pcbnew.VECTOR2I(int(ux * sh), int(uy * sh))
        ends = [mover.GetPosition()] if mover.GetClass() == "PCB_VIA" else [pcbnew.VECTOR2I(mover.GetStart()), pcbnew.VECTOR2I(mover.GetEnd())]
        net = mover.GetNetCode()
        for pt in ends:
            npt = pt + delta
            for it in b.GetTracks():
                if it.GetNetCode() != net: continue
                if it.GetClass() == "PCB_VIA":
                    if it.GetPosition() == pt: it.SetPosition(npt)
                else:
                    if it.GetStart() == pt: it.SetStart(npt)
                    if it.GetEnd() == pt: it.SetEnd(npt)
        fixed += 1
        print("nudged", mover.GetClass(), mover.GetNetname(), "away from", descs[1 if mover is a else 0][:50], "short by %.4f" % short)
    if not fixed:
        print("nudge: nothing movable"); break
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    pcbnew.SaveBoard(PCB, b)
