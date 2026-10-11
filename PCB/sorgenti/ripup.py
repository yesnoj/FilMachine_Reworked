# -*- coding: utf-8 -*-
"""Rip up the given nets and re-route them with the grid router: pads are joined one at a time to the
growing tree of already-placed copper (nearest first). Usage: ripup.py board.kicad_pcb NET1,NET2"""
import sys, os, math
os.environ.setdefault("MAZE_CLR", "0.25"); os.environ.setdefault("MAZE_MAXN", "30000000")
PCB = sys.argv[1]; NETS = sys.argv[2].split(",")
sys.argv = ["maze.py", PCB]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "maze.py")).read().split('if "--tie" in sys.argv:')[0])
import numpy as np
from PIL import Image, ImageDraw

def net_copper_mask(nc):
    """cells already holding copper of this net, per layer (pads, tracks, vias)"""
    ms = []
    for L in LAYERS:
        im = Image.new("1", (W, H), 0); d = ImageDraw.Draw(im)
        for f in b.GetFootprints():
            for p in f.Pads():
                if p.GetNetCode() == nc and p.IsOnLayer(L):
                    sp = p.GetEffectivePolygon(L, pcbnew.ERROR_INSIDE)
                    for i in range(sp.OutlineCount()):
                        ch = sp.Outline(i); d.polygon([to_c(ch.CPoint(k).x, ch.CPoint(k).y) for k in range(ch.PointCount())], fill=1)
        for t in b.GetTracks():
            if t.GetNetCode() != nc: continue
            if t.GetClass() == "PCB_VIA":
                c = to_c(t.GetPosition().x, t.GetPosition().y); r = 0.3 / RES
                d.ellipse([c[0] - r, c[1] - r, c[0] + r, c[1] + r], fill=1)
            elif t.GetLayer() == L:
                a = to_c(t.GetStart().x, t.GetStart().y); e = to_c(t.GetEnd().x, t.GetEnd().y)
                d.line([a, e], fill=1, width=max(1, int(t.GetWidth() / 1e6 / RES)))
        ms.append(np.array(im, dtype=bool))
    return ms

ok_all = True
for net in NETS:
    ni = b.FindNet(net); nc = ni.GetNetCode(); tw = netclass_width(net)
    only = os.environ.get("ONLY_PAD")      # "REF.NUM": keep the net, just join this pad to the rest
    if not only:
        for t in list(b.GetTracks()):
            if t.GetNetCode() == nc: b.Delete(t)
        refill()
    pads = [(f.GetReference(), p) for f in b.GetFootprints() for p in f.Pads() if p.GetNetCode() == nc]
    if only:
        r_, n_ = only.split(".")
        pads = [x for x in pads if not (x[0] == r_ and x[1].GetNumber() == n_)] + [x for x in pads if x[0] == r_ and x[1].GetNumber() == n_]
        pads = [pads[0]] + [pads[-1]]
    plane = net in PLANE_OF
    # start tree with the first pad (for plane nets: any pad, the tree also includes the plane via sites)
    remaining = pads[1:]
    tree_pads = [pads[0]]
    while remaining:
        cm = net_copper_mask(nc)
        # pick the remaining pad nearest to the tree pads
        def dist(rp):
            p = rp[1].GetPosition()
            return min(math.hypot(p.x - q[1].GetPosition().x, p.y - q[1].GetPosition().y) for q in tree_pads)
        remaining.sort(key=dist)
        ref, pad = remaining.pop(0)
        if cm[0][int(round(to_c(pad.GetPosition().x, pad.GetPosition().y)[1])), int(round(to_c(pad.GetPosition().x, pad.GetPosition().y)[0]))] and False:
            continue
        blocked = [raster(nc, L, CLR + tw / 2) for L in LAYERS]
        vf = via_forbidden(nc)
        viaok = ~vf & ~raster(nc, LAYERS[0], CLR + VIA_D / 2) & ~raster(nc, LAYERS[1], CLR + VIA_D / 2)
        c = to_c(pad.GetPosition().x, pad.GetPosition().y)
        A = [(li, int(round(c[0])), int(round(c[1]))) for li, L in enumerate(LAYERS) if pad.IsOnLayer(L)]
        # goal: copper of the tree that is NOT this pad
        own = net_copper_mask(-1)  # empty
        tgt = [m.copy() for m in cm]
        sp = pad.GetBoundingBox()
        for li in range(2):
            x0, y0 = to_c(sp.GetLeft(), sp.GetTop()); x1, y1 = to_c(sp.GetRight(), sp.GetBottom())
            tgt[li][int(y0) - 1:int(y1) + 2, int(x0) - 1:int(x1) + 2] = False
        if plane:
            pm = plane_mask(net) & viaok
        for li in range(2):
            blocked[li] &= ~tgt[li]
        for s in A:
            r = int((tw / 2 + CLR) / RES) + 2
            blocked[s[0]][max(0, s[2] - r):s[2] + r, max(0, s[1] - r):s[1] + r] = False
        ys, xs = np.nonzero(tgt[0] | tgt[1])
        target = (int(np.median(xs)), int(np.median(ys))) if len(xs) else None
        goal = (lambda s: tgt[s[0]][s[2], s[1]] or (plane and pm[s[2], s[1]] and math.hypot(s[1] - A[0][1], s[2] - A[0][2]) > 20))
        path = astar(A, goal, blocked, viaok, target=None)
        if path is None:
            print("  FAILED", net, ref, pad.GetNumber()); ok_all = False
            tree_pads.append((ref, pad)); continue
        L_, x_, y_ = path[-1]
        emit(path, net, tw, final_via=(plane and not tgt[L_][y_, x_]))
        refill()
        tree_pads.append((ref, pad))
    print("rerouted", net)
pcbnew.SaveBoard(PCB, b)
print("ripup done", "ok" if ok_all else "with failures")
