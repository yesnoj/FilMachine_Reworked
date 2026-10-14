# -*- coding: utf-8 -*-
"""Derive, for every footprint of the design, the transform between the JLCPCB/EasyEDA footprint of the LCSC part
(origin + 0 deg orientation = what JLCPCB expects in the CPL) and the KiCad footprint used on the board.
Output: a table {kicad_fp_name: (dtheta_deg, tx_mm, ty_mm, notes)} saved to jlcpos.json, where
  JLC rotation = KiCad rotation + dtheta,  JLC Mid = KiCad anchor + R(KiCad rotation) * (tx, ty)
Matching: by geometry for all 4 rotations; polarity of diodes/LEDs/electrolytics from the EasyEDA silkscreen
(cathode bar / chamfer / plus mark), pin numbers for ICs and connectors."""
import json, math, os, sys, re
import pcbnew
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE); sys.path.insert(0, "/home/claude/pcb")
import design
CACHE = os.path.join(HERE, "easyeda")       # EasyEDA footprint JSON per LCSC code (fetched by fetch_easyeda.sh)
FMDIR = os.path.join(HERE, "FM.pretty") if os.path.isdir(os.path.join(HERE, "FM.pretty")) else "/home/claude/pcb/FM.pretty"
U = 0.254   # EasyEDA unit (10 mil) -> mm
FPDIR = "/usr/share/kicad/footprints"

def eda_pinnames(code):
    """pin number -> pin name from the EasyEDA schematic symbol ('+', '-', 'A', 'K', '1'...)"""
    d = json.load(open(os.path.join(CACHE, f"{code}.json")))
    names = {}
    for s in d["result"].get("dataStr", {}).get("shape", []):
        if not s.startswith("P~"): continue
        segs = s.split("^^")
        num = segs[0].split("~")[3]
        for sg in segs[1:]:
            f = sg.split("~")
            if len(f) > 4 and f[0] == "0" and f[4] not in ("", "start", "end") and not f[4].lstrip("-").isdigit():
                names[num] = f[4]; break
    return names

def eda_fp(code):
    d = json.load(open(os.path.join(CACHE, f"{code}.json")))
    ds = d["result"]["packageDetail"]["dataStr"]; hx, hy = float(ds["head"]["x"]), float(ds["head"]["y"])
    pads, silk = [], []
    for s in ds["shape"]:
        f = s.split("~")
        if f[0] == "PAD":
            pads.append({"num": f[8], "x": (float(f[2]) - hx) * U, "y": (float(f[3]) - hy) * U, "tht": f[6] == "11",
                         "w": float(f[4]) * U, "h": float(f[5]) * U, "shape": f[1]})
        elif f[0] == "TRACK" and f[2] == "3":
            pts = [float(v) for v in f[4].split()]
            silk.append([((pts[i] - hx) * U, (pts[i + 1] - hy) * U) for i in range(0, len(pts), 2)])
        elif f[0] == "SOLIDREGION" and f[1] == "3":
            pts = [float(v) for v in re.findall(r"-?\d+\.?\d*", f[3])]
            silk.append([((pts[i] - hx) * U, (pts[i + 1] - hy) * U) for i in range(0, len(pts) - 1, 2)])
        elif f[0] == "CIRCLE" and f[4] == "3":
            silk.append([((float(f[1]) - hx) * U, (float(f[2]) - hy) * U)])
    return d["result"]["packageDetail"]["title"], pads, silk

def kicad_fp(fpid):
    lib, name = fpid.split(":")
    fp = pcbnew.FootprintLoad(FMDIR if lib == "FM" else f"{FPDIR}/{lib}.pretty", name)
    pads = []
    for p in fp.Pads():
        if not p.GetNumber(): continue          # unnumbered helper pads (split thermal tabs)
        pads.append({"num": p.GetNumber(), "x": p.GetPosition().x / 1e6, "y": p.GetPosition().y / 1e6,
                     "tht": p.GetAttribute() == pcbnew.PAD_ATTRIB_PTH})
    c = fp.GetCourtyard(pcbnew.F_CrtYd).BBox()
    body = ((c.GetLeft() + c.GetRight()) / 2e6, (c.GetTop() + c.GetBottom()) / 2e6)
    return fp, pads, body

def rot(x, y, th):
    c, s = math.cos(math.radians(th)), math.sin(math.radians(th))
    return (x * c + y * s, -x * s + y * c)        # KiCad: positive = CCW on screen (y down)

TOL = 0.4
def match(kpads, epads, th, tol=None):
    """translation that maps rotated EDA pads onto KiCad pads; returns (t, pairs) or None"""
    ep = [(p, rot(p["x"], p["y"], th)) for p in epads]
    kx = sum(p["x"] for p in kpads) / len(kpads); ky = sum(p["y"] for p in kpads) / len(kpads)
    ex = sum(q[0] for _, q in ep) / len(ep); ey = sum(q[1] for _, q in ep) / len(ep)
    t = (kx - ex, ky - ey)
    pairs, used = [], set()
    for kp in kpads:
        best = None
        for i, (p, q) in enumerate(ep):
            if i in used: continue
            d = math.hypot(kp["x"] - (q[0] + t[0]), kp["y"] - (q[1] + t[1]))
            if best is None or d < best[0]: best = (d, i, p)
        if best is None or best[0] > (tol or TOL): return None
        used.add(best[1]); pairs.append((kp, best[2]))
    return t, pairs

def eda_mark_side(code, epads, silk, kind):
    """index of the EDA pad that carries the polarity mark (cathode bar/chamfer/triangle, or '+')"""
    pts = [pt for poly in silk for pt in poly]
    if not pts: return None
    xs = [p["x"] for p in epads]
    axis = "x" if (max(xs) - min(xs)) > 0.3 else "y"
    if kind == "cap":   # '+' mark: a small cross = two short segments crossing; take the silk point farthest from centre on the pad axis... use short tracks (len < 1.5 mm)
        small = [poly for poly in silk if len(poly) == 2 and math.hypot(poly[1][0] - poly[0][0], poly[1][1] - poly[0][1]) < 1.6]
        if not small: return None
        cx = sum(p[0] for poly in small for p in poly) / sum(len(poly) for poly in small)
        cy = sum(p[1] for poly in small for p in poly) / sum(len(poly) for poly in small)
        return min(range(len(epads)), key=lambda i: math.hypot(epads[i]["x"] - cx, epads[i]["y"] - cy))
    # diodes / LEDs: the cathode is marked by a bar (short segment perpendicular to the axis, near the centre) or a
    # chamfered outline corner or a triangle apex. Use the most robust: the vertical/horizontal bar segment nearest the
    # centre whose direction is perpendicular to the pad axis and length < body width, plus chamfers.
    cands = []
    for poly in silk:
        for (a, b) in zip(poly, poly[1:]):
            dx, dy = b[0] - a[0], b[1] - a[1]; L = math.hypot(dx, dy)
            if L < 0.3 or L > 4.0: continue
            perp = abs(dx) < 0.05 if axis == "x" else abs(dy) < 0.05
            if perp:
                m = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
                cands.append((abs(m[0] if axis == "x" else m[1]), m))
    if not cands: return None
    # the bar is the perpendicular segment closest to the footprint centre but offset toward one pad; the outline ends
    # (|coord| large) are excluded by taking the smallest |offset| among segments that are not at the pad outer edge
    cands.sort()
    off = cands[0][1][0] if axis == "x" else cands[0][1][1]
    side = -1 if off < 0 else 1
    if abs(off) < 0.05:   # bar exactly at centre: use chamfer side instead (corners with diagonal segments)
        diag = []
        for poly in silk:
            for (a, b) in zip(poly, poly[1:]):
                dx, dy = b[0] - a[0], b[1] - a[1]
                if abs(dx) > 0.2 and abs(dy) > 0.2 and math.hypot(dx, dy) < 3.0:
                    diag.append(((a[0] + b[0]) / 2) if axis == "x" else ((a[1] + b[1]) / 2))
        if not diag: return None
        side = -1 if sum(diag) / len(diag) < 0 else 1
    return min(range(len(epads)), key=lambda i: abs((epads[i]["x"] if axis == "x" else epads[i]["y"]) - side * 100))

result = {}
by_fp = {}
for p in design.PARTS:
    if p["dnp"] or not p.get("lcsc"): continue
    by_fp.setdefault((p["fp"], p["lcsc"]), []).append(p)
for (fpid, code), parts in sorted(by_fp.items()):
    sym = parts[0]["sym"]
    kind = "cap" if sym.startswith("Device:C_Polarized") or parts[0]["fp"].startswith("Capacitor_THT:CP") else \
           ("led" if sym == "Device:LED" else ("diode" if sym.startswith(("Device:D", "Diode")) else "other"))
    try:
        title, epads, silk = eda_fp(code)
    except Exception as e:
        lib, name = fpid.split(":")
        if name.startswith(("R_0603", "R_0805", "C_0603", "C_0805", "C_1206")):   # plain chip: same convention as the others
            result[f"{fpid}|{code}"] = {"dtheta": 0, "tx": 0.0, "ty": 0.0, "eda": "(chip, EDA fetch failed: assumed standard)", "lcsc": code, "note": "assumed", "nsol": 1, "refs": [q["ref"] for q in parts]}
            print(fpid, code, "EDA fetch failed -> assumed standard chip (0,0,0)"); continue
        result[f"{fpid}|{code}"] = {"error": str(e)}; print(fpid, code, "EDA ERROR", e); continue
    fp, kpads, body = kicad_fp(fpid)
    if len(kpads) != len(epads):
        # KiCad PowerPAD footprints split the thermal pad into several pads with the same number; merge duplicates
        seen = {}
        for kp in kpads:
            seen.setdefault(kp["num"], []).append(kp)
        kpads = [{"num": n, "x": sum(q["x"] for q in v) / len(v), "y": sum(q["y"] for q in v) / len(v), "tht": v[0]["tht"]} for n, v in seen.items()]
    sols = []
    note = ""
    for tol in (0.4, 1.0, 1.6):
        for th in (0, 90, 180, 270):
            m = match(kpads, epads, th, tol)
            if m: sols.append((th, m[0], m[1]))
        if sols:
            if tol > 0.4: note = f"(loose match, tol {tol} mm: land patterns differ) "
            break
    if not sols:
        result[f"{fpid}|{code}"] = {"error": "no geometric match", "eda": title}; print(f"{fpid:50s} {code:>10} NO MATCH ({title})"); continue
    if kind in ("led", "diode", "cap"):
        names = eda_pinnames(code)
        wantname = "+" if kind == "cap" else "-"
        eda_num = [n for n, nm in names.items() if nm.strip().upper() in ((wantname,) if kind == "cap" else ("-", "K", "C", "CATHODE", "K-"))]
        good = []
        if eda_num:
            for th, t, pairs in sols:
                kp1 = [ep for kp, ep in pairs if kp["num"] == "1"][0]
                if kp1["num"] == eda_num[0]: good.append((th, t, pairs))
            note = f"EDA {'positive' if kind == 'cap' else 'cathode'} pad is #{eda_num[0]} (symbol pin names {names})" + (" -> numbering differs from KiCad" if eda_num[0] != "1" else "")
        else:
            note = f"no polarity pin name in EDA symbol ({names}): matched by pad number"
            good = [s_ for s_ in sols if all(kp["num"] == ep["num"] for kp, ep in s_[2])]
        sols = good
    else:
        good = [s_ for s_ in sols if all(kp["num"] == ep["num"] for kp, ep in s_[2])]
        if good: sols = good
        elif len(sols) > 1: note += "AMBIGUOUS (pad numbers differ): " + ",".join(str(s_[0]) for s_ in sols)
    if not sols:
        result[f"{fpid}|{code}"] = {"error": "no polarity-consistent match", "eda": title}; print(f"{fpid:50s} {code:>10} NO POLARITY MATCH ({title})"); continue
    th, t, pairs = sols[0]
    # body side check for connectors: EDA silk centroid vs KiCad courtyard centre, after the transform
    if silk and len(kpads) >= 2:
        pts = [pt for poly in silk for pt in poly]
        sx = sum(p[0] for p in pts) / len(pts); sy = sum(p[1] for p in pts) / len(pts)
        r = rot(sx, sy, th); eb = (r[0] + t[0], r[1] + t[1])
        if math.hypot(eb[0] - body[0], eb[1] - body[1]) > 1.5:
            note += f" | body centre differs: EDA silk centroid at ({eb[0]:.1f},{eb[1]:.1f}) vs KiCad courtyard ({body[0]:.1f},{body[1]:.1f})"
    result[f"{fpid}|{code}"] = {"dtheta": th, "tx": round(t[0], 3), "ty": round(t[1], 3), "eda": title, "lcsc": code, "note": note,
                    "nsol": len(sols), "refs": [q["ref"] for q in parts]}
    print(f"{fpid:50s} {code:>10} dθ={th:3d} t=({t[0]:+.2f},{t[1]:+.2f}) {title[:40]:40} {note}")
json.dump(result, open(os.path.join(HERE, "jlcpos.json"), "w"), indent=1)
