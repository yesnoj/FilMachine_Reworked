# -*- coding: utf-8 -*-
"""Engineering change without re-autorouting the whole board (file-level merge).
Usage: eco.py <routed_board> <fresh_board> <out_board> NET1,NET2,...
  <fresh_board>: gen_sch + mkpro + build_pcb output of the NEW design (correct footprints, placement, nets, paths).
  Copies every track, arc, via and zone (copper pours, HC areas, keepouts) of <routed_board> into <fresh_board>,
  remapping net numbers by name, except the copper of the listed nets (to be re-routed with ripup.py).
  The copied copper must not touch footprints that moved: list every net of moved parts."""
import sys, re

SRC, FRESH, OUT, RIP = sys.argv[1], sys.argv[2], sys.argv[3], set(sys.argv[4].split(","))
# optional 5th arg: "x0:y0:x1:y1;..." board-local mm rectangles (origin 100,100) cleared of ALL copied tracks/vias
CLEAR = [tuple(float(v) for v in r.split(":")) for r in sys.argv[5].split(";")] if len(sys.argv) > 5 else []
OX, OY = 100.0, 100.0

def seg_hits_rect(x1, y1, x2, y2, hw, r):
    rx0, ry0, rx1, ry1 = r[0] - hw, r[1] - hw, r[2] + hw, r[3] + hw
    # Liang-Barsky clip
    dx, dy = x2 - x1, y2 - y1
    t0, t1 = 0.0, 1.0
    for p, q in ((-dx, x1 - rx0), (dx, rx1 - x1), (-dy, y1 - ry0), (dy, ry1 - y1)):
        if p == 0:
            if q < 0: return False
        else:
            t = q / p
            if p < 0:
                if t > t1: return False
                t0 = max(t0, t)
            else:
                if t < t0: return False
                t1 = min(t1, t)
    return True

def top_items(text):
    """split the board body into its top-level s-expressions: (start, end, head)"""
    items, depth, i, start, instr = [], 0, 0, None, False
    n = len(text)
    while i < n:
        c = text[i]
        if instr:
            if c == "\\": i += 2; continue
            if c == '"': instr = False
        elif c == '"': instr = True
        elif c == "(":
            depth += 1
            if depth == 2: start = i
        elif c == ")":
            if depth == 2:
                m = re.match(r"\((\w+)", text[start:start + 40])
                items.append((start, i + 1, m.group(1) if m else ""))
            depth -= 1
        i += 1
    return items

def net_table(text, items):
    t = {}
    for s, e, h in items:
        if h == "net":
            m = re.match(r'\(net (\d+) "((?:[^"\\]|\\.)*)"\)', text[s:e])
            if m: t[int(m.group(1))] = m.group(2)
    return t

src = open(SRC).read(); fr = open(FRESH).read()
si = top_items(src); fi = top_items(fr)
snet = net_table(src, si); fnet = {v: k for k, v in net_table(fr, fi).items()}
keep, dropped = [], 0
CLEARED = set()
for s, e, h in si:
    if h not in ("segment", "arc", "via", "zone"):
        continue
    body = src[s:e]
    m = re.search(r"\(net (\d+)\)", body)
    name = snet.get(int(m.group(1)), "") if m else ""
    if h == "zone":
        mz = re.search(r'\(net_name "((?:[^"\\]|\\.)*)"\)', body)
        if mz: name = mz.group(1)
    if name in RIP:
        dropped += 1; continue
    if h in ("segment", "arc", "via") and CLEAR:
        w = re.search(r"\((?:width|size) ([\d.]+)\)", body); hw = float(w.group(1)) / 2 if w else 0.3
        if h == "via":
            a = re.search(r"\(at ([-\d.]+) ([-\d.]+)\)", body); pts = [(float(a.group(1)), float(a.group(2)))] * 2
        else:
            a = re.search(r"\(start ([-\d.]+) ([-\d.]+)\)", body); b_ = re.search(r"\(end ([-\d.]+) ([-\d.]+)\)", body)
            pts = [(float(a.group(1)), float(a.group(2))), (float(b_.group(1)), float(b_.group(2)))]
        (x1, y1), (x2, y2) = [(x - OX, y - OY) for x, y in pts]
        if any(seg_hits_rect(x1, y1, x2, y2, hw, r) for r in CLEAR):
            dropped += 1; CLEARED.add(name); continue
    if m:
        if name and name not in fnet:
            raise SystemExit(f"net {name} missing in fresh board")
        body = body[:m.start()] + f"(net {fnet[name] if name else 0})" + body[m.end():]
    keep.append(body)
# insert before the final closing paren of the fresh board
end = fr.rstrip().rfind(")")
out = fr[:end] + "\n\t" + "\n\t".join(keep) + "\n" + fr[end:]
open(OUT, "w").write(out)
print(f"eco: copied {len(keep)} copper items, dropped {dropped} on {sorted(RIP)}")
print("cleared-area nets:", ",".join(sorted(CLEARED - RIP)))
