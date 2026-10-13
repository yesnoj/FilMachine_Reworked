# -*- coding: utf-8 -*-
"""Generate a hierarchical KiCad 9 schematic from design.py.
Every pin gets a short wire stub and a global label with its net name; NC pins get a no-connect flag.
"""
import uuid, copy, re, math, json, sys, os
import sexpdata
from sexpdata import Symbol as Sym
import design

PROJECT = "FilMachine_Mainboard"
OUT = sys.argv[1] if len(sys.argv) > 1 else "out"
LIBDIR = "/usr/share/kicad/symbols"
G = 1.27
STUB = 2.54

_ucount = [0]
def U():
    # deterministic UUIDs: regenerating the schematic keeps the links to the PCB
    _ucount[0] += 1
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "filmachine-mainboard/%d" % _ucount[0]))
def S_(x): return x.value() if isinstance(x, Sym) else x

# ---------------------------------------------------------------- s-expr writer
def fmt(x):
    if isinstance(x, Sym): return x.value()
    if isinstance(x, bool): return "yes" if x else "no"
    if isinstance(x, (int,)): return str(x)
    if isinstance(x, float):
        s = ("%.4f" % x).rstrip("0").rstrip(".")
        return "0" if s in ("-0", "") else s
    if isinstance(x, str): return '"' + x.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'
    if isinstance(x, list): return "(" + " ".join(fmt(e) for e in x) + ")"
    raise TypeError(x)

def dump(x, ind=0):
    # pretty-ish: newline before nested lists at top levels
    if isinstance(x, list) and x and any(isinstance(e, list) for e in x) and ind < 3:
        head = [fmt(e) for e in x if not isinstance(e, list)]
        out = "\t" * ind + "(" + " ".join(head)
        for e in x:
            if isinstance(e, list):
                out += "\n" + dump(e, ind + 1)
        return out + "\n" + "\t" * ind + ")"
    return "\t" * ind + fmt(x)

# ---------------------------------------------------------------- library symbols
_libcache = {}
def lib(name):
    if name not in _libcache:
        _libcache[name] = sexpdata.loads(open(f"{LIBDIR}/{name}.kicad_sym").read())
    return _libcache[name]

def raw_sym(libname, name):
    for e in lib(libname)[1:]:
        if isinstance(e, list) and S_(e[0]) == "symbol" and e[1] == name:
            return e
    raise KeyError(f"{libname}:{name}")

def flatten(libname, name):
    e = copy.deepcopy(raw_sym(libname, name))
    ext = [x for x in e if isinstance(x, list) and S_(x[0]) == "extends"]
    if not ext:
        return e
    base = flatten(libname, ext[0][1])
    props = {x[1]: x for x in e if isinstance(x, list) and S_(x[0]) == "property"}
    out = [Sym("symbol"), name]
    for x in base[2:]:
        if isinstance(x, list) and S_(x[0]) == "property" and x[1] in props:
            out.append(props.pop(x[1]))
        elif isinstance(x, list) and S_(x[0]) == "symbol":
            y = copy.deepcopy(x)
            y[1] = re.sub("^" + re.escape(base[1]), name, y[1])
            out.append(y)
        else:
            out.append(x)
    # other derived-only properties
    for k, v in props.items():
        out.insert(2, v)
    return out

def lib_symbol_for_sch(lib_id):
    libname, name = lib_id.split(":")
    e = flatten(libname, name)
    e = copy.deepcopy(e)
    e[1] = lib_id
    return e

def sym_pins(e):
    """return list of dict(unit, num, name, type, x, y, ang, hidden)"""
    out = []
    for sub in e:
        if isinstance(sub, list) and S_(sub[0]) == "symbol" and isinstance(sub[1], str):
            m = re.search(r"_(\d+)_(\d+)$", sub[1])
            unit = int(m.group(1)) if m else 0
            for x in sub:
                if isinstance(x, list) and S_(x[0]) == "pin":
                    d = {S_(y[0]): y for y in x if isinstance(y, list)}
                    hidden = any(S_(y) == "hide" for y in x if isinstance(y, Sym)) or \
                             ("hide" in d and S_(d["hide"][1]) == "yes")
                    out.append(dict(unit=unit, num=d["number"][1], name=d["name"][1], type=S_(x[1]),
                                    x=float(d["at"][1]), y=float(d["at"][2]), ang=float(d["at"][3]),
                                    length=float(d["length"][1]) if "length" in d else 2.54, hidden=hidden))
    return out

def sym_units(e):
    us = set()
    for sub in e:
        if isinstance(sub, list) and S_(sub[0]) == "symbol" and isinstance(sub[1], str):
            m = re.search(r"_(\d+)_(\d+)$", sub[1])
            if m and int(m.group(1)) > 0: us.add(int(m.group(1)))
    return sorted(us) or [1]

def body_bbox(e, unit):
    xs, ys = [], []
    def walk(n):
        for x in n:
            if isinstance(x, list):
                h = S_(x[0])
                if h in ("xy", "start", "end", "center", "at") and len(x) >= 3 and all(isinstance(v, (int, float)) for v in x[1:3]):
                    xs.append(float(x[1])); ys.append(float(x[2]))
                elif h == "property":
                    continue
                else:
                    walk(x)
    for sub in e:
        if isinstance(sub, list) and S_(sub[0]) == "symbol" and isinstance(sub[1], str):
            m = re.search(r"_(\d+)_(\d+)$", sub[1])
            u = int(m.group(1)) if m else 0
            if u in (0, unit): walk(sub)
    if not xs: return (-2.54, -2.54, 2.54, 2.54)
    return (min(xs), min(ys), max(xs), max(ys))

# ---------------------------------------------------------------- geometry
DIRS = {0: (1, 0), 90: (0, 1), 180: (-1, 0), 270: (0, -1)}   # lib coords (y up)

def snap(v): return round(v / G) * G

# ---------------------------------------------------------------- build
root_uuid = U()
sheet_uuids = {s: U() for s, _ in design.SHEETS}
by_sheet = {s: [] for s, _ in design.SHEETS}
for p in design.PARTS:
    by_sheet[p["sheet"]].append(p)

symbol_uuid = {}      # ref -> symbol uuid of unit 1 (used for footprint path)
pin_positions = []    # for verification

def text_w(s): return 1.27 * 0.75 * len(s) + 3.0

def place_sheet(sheet, parts, flags):
    items = []
    used_libs = {}
    # layout cells
    cells = []
    for p in parts:
        e = lib_symbol_for_sch(p["sym"])
        used_libs[p["sym"]] = e
        pins = sym_pins(e)
        units = sym_units(e)
        for u in units:
            upins = [q for q in pins if q["unit"] in (0, u)]
            if not upins and p["pins"] and u != units[0]:
                continue
            bx0, by0, bx1, by1 = body_bbox(e, u)
            # extents including stubs + labels
            L = R_ = T = B = 0.0
            for q in upins:
                net = p["pins"].get(q["num"], "__absent__")
                lab = 0 if net in (None, "__absent__") else text_w(net)
                ox, oy = DIRS[int(q["ang"] + 180) % 360]
                ex = q["x"] + ox * (STUB + lab); ey = q["y"] + oy * (STUB + lab)
                L = min(L, ex); R_ = max(R_, ex); B = min(B, ey); T = max(T, ey)
            L = min(L, bx0) - 2; R_ = max(R_, bx1) + 2; B = min(B, by0) - 5; T = max(T, by1) + 5
            cells.append(dict(p=p, e=e, unit=u, upins=upins, L=L, R=R_, T=T, B=B))
    for f in flags:
        cells.append(dict(flag=f, L=-12, R=12, T=6, B=-14))
    # row packing: try A3 first, then A2
    def pack(W):
        x0, y0 = 20.0, 22.0
        x, y, rowh = x0, y0, 0
        placed = []
        for c in cells:
            w = c["R"] - c["L"]; h = c["T"] - c["B"]
            if x + w > W:
                x = x0; y += rowh + 2; rowh = 0
            sx = snap(x - c["L"]); sy = snap(y + c["T"])
            placed.append((c, sx, sy))
            x += w + 2; rowh = max(rowh, h)
        return placed, y + rowh
    for paper, W, Hmax in (("A3", 400, 255), ("A2", 575, 375), ("A1", 820, 540)):
        placed, height = pack(W)
        if height <= Hmax: break

    path = f"/{root_uuid}/{sheet_uuids[sheet]}"
    for c, sx, sy in placed:
        if "flag" in c:
            net, idx = c["flag"]
            su = U()
            ref = f"#FLG0{idx:02d}"
            items.append([Sym("symbol"), [Sym("lib_id"), "power:PWR_FLAG"], [Sym("at"), sx, sy, 0], [Sym("unit"), 1],
                          [Sym("exclude_from_sim"), Sym("no")], [Sym("in_bom"), Sym("yes")], [Sym("on_board"), Sym("yes")],
                          [Sym("dnp"), Sym("no")], [Sym("uuid"), su],
                          prop("Reference", ref, sx, sy - 3.8, hide=True), prop("Value", "PWR_FLAG", sx, sy - 3.8),
                          prop("Footprint", "", sx, sy, hide=True), prop("Datasheet", "~", sx, sy, hide=True),
                          prop("Description", "", sx, sy, hide=True),
                          [Sym("pin"), "1", [Sym("uuid"), U()]],
                          [Sym("instances"), [Sym("project"), PROJECT, [Sym("path"), path, [Sym("reference"), ref], [Sym("unit"), 1]]]]])
            used_libs["power:PWR_FLAG"] = lib_symbol_for_sch("power:PWR_FLAG")
            # pin at origin pointing up into body -> stub downwards
            add_stub_label(items, sx, sy, (0, 1), net)
            continue
        p, e, u = c["p"], c["e"], c["unit"]
        su = U()
        if u == sym_units(e)[0]:
            symbol_uuid[p["ref"]] = su
        fields = [prop("Reference", p["ref"], sx, sy + (c["B"] + 3.5) * -1 + 0, ),
                  ]
        # reference above body, value below body
        e_bb = body_bbox(e, u)
        top_y = sy - e_bb[3] - 1.5
        bot_y = sy - e_bb[1] + 1.5
        sym = [Sym("symbol"), [Sym("lib_id"), p["sym"]], [Sym("at"), sx, sy, 0], [Sym("unit"), u],
               [Sym("exclude_from_sim"), Sym("no")], [Sym("in_bom"), Sym("no") if p["sym"].startswith("Mechanical") else Sym("yes")],
               [Sym("on_board"), Sym("yes")], [Sym("dnp"), Sym("yes") if p["dnp"] else Sym("no")],
               [Sym("fields_autoplaced"), Sym("yes")], [Sym("uuid"), su],
               prop("Reference", p["ref"], sx, top_y),
               prop("Value", p["value"], sx, bot_y),
               prop("Footprint", p["fp"], sx, sy, hide=True),
               prop("Datasheet", "~", sx, sy, hide=True),
               prop("Description", p["desc"], sx, sy, hide=True),
               prop("MPN", p["mpn"], sx, sy, hide=True),
               prop("Manufacturer", p["mfr"], sx, sy, hide=True),
               prop("LCSC", p.get("lcsc", ""), sx, sy, hide=True)]
        for q in c["upins"]:
            sym.append([Sym("pin"), q["num"], [Sym("uuid"), U()]])
        sym.append([Sym("instances"), [Sym("project"), PROJECT, [Sym("path"), path, [Sym("reference"), p["ref"]], [Sym("unit"), u]]]])
        items.append(sym)
        for q in c["upins"]:
            px = sx + q["x"]; py = sy - q["y"]
            net = p["pins"].get(q["num"], "__absent__")
            ox, oy = DIRS[int(q["ang"] + 180) % 360]
            pin_positions.append((sheet, p["ref"], q["num"], px, py, net))
            if net == "__absent__":
                if q["type"] == "no_connect" or q["hidden"]:
                    continue
                raise SystemExit(f"pin {p['ref']}.{q['num']} ({q['name']}) unassigned")
            if net is None:
                items.append([Sym("no_connect"), [Sym("at"), px, py], [Sym("uuid"), U()]])
                continue
            add_stub_label(items, px, py, (ox, -oy), net)
    return items, used_libs, paper

def prop(name, val, x, y, hide=False, ang=0):
    eff = [Sym("effects"), [Sym("font"), [Sym("size"), 1.27, 1.27]]]
    if hide: eff.append([Sym("hide"), Sym("yes")])
    return [Sym("property"), name, val, [Sym("at"), x, y, ang], eff]

def add_stub_label(items, px, py, d, net):
    dx, dy = d   # screen direction (y down)
    ex, ey = px + dx * STUB, py + dy * STUB
    items.append([Sym("wire"), [Sym("pts"), [Sym("xy"), px, py], [Sym("xy"), ex, ey]],
                  [Sym("stroke"), [Sym("width"), 0], [Sym("type"), Sym("default")]], [Sym("uuid"), U()]])
    ang = {(1, 0): 0, (-1, 0): 180, (0, -1): 90, (0, 1): 270}[(dx, dy)]
    just = "left" if ang in (0, 90) else "right"
    items.append([Sym("global_label"), net, [Sym("shape"), Sym("passive")], [Sym("at"), ex, ey, ang],
                  [Sym("fields_autoplaced"), Sym("yes")],
                  [Sym("effects"), [Sym("font"), [Sym("size"), 1.27, 1.27]], [Sym("justify"), Sym(just)]],
                  [Sym("uuid"), U()],
                  [Sym("property"), "Intersheetrefs", "${INTERSHEET_REFS}", [Sym("at"), ex, ey, 0],
                   [Sym("effects"), [Sym("font"), [Sym("size"), 1.27, 1.27]], [Sym("hide"), Sym("yes")]]]])

def header(u, paper, title, page):
    return [Sym("kicad_sch"), [Sym("version"), 20250114], [Sym("generator"), "eeschema"], [Sym("generator_version"), "9.0"],
            [Sym("uuid"), u], [Sym("paper"), paper],
            [Sym("title_block"), [Sym("title"), title], [Sym("date"), "2026-10-13"], [Sym("rev"), "1.1"],
             [Sym("company"), "FilMachine"], [Sym("comment"), 1, "Mainboard per JC4880P433 (ESP32-P4) — generata da design.py"]]]

# power flags: nets that only have power_in/passive pins
FLAG_NETS = ["+12V_IN_H1", "+12V_IN_H2", "+12V_IN_M", "GND", "+12V", "+12V_PUMP", "+5V", "+12V_AUX", "+5V_AUX", "+3V3"]
flags_by_sheet = {"power": [(n, i + 1) for i, n in enumerate(FLAG_NETS)]}

os.makedirs(OUT, exist_ok=True)
sheet_files = []
for idx, (sheet, title) in enumerate(design.SHEETS):
    items, used, paper = place_sheet(sheet, by_sheet[sheet], flags_by_sheet.get(sheet, []))
    doc = header(sheet_uuids[sheet], paper, f"FilMachine Mainboard — {title}", idx + 2)
    doc.append([Sym("lib_symbols")] + list(used.values()))
    doc += items
    doc.append([Sym("embedded_fonts"), Sym("no")])
    fn = f"{sheet}.kicad_sch"
    open(f"{OUT}/{fn}", "w").write(dump(doc) + "\n")
    sheet_files.append((sheet, title, fn, idx + 2))

# root sheet with 4 hierarchical sheet blocks + notes
root = header(root_uuid, "A4", "FilMachine Mainboard v1", 1)
root.append([Sym("lib_symbols")])
for i, (sheet, title, fn, page) in enumerate(sheet_files):
    x = 25 + (i % 2) * 120; y = 40 + (i // 2) * 60
    root.append([Sym("sheet"), [Sym("at"), x, y], [Sym("size"), 90, 35], [Sym("exclude_from_sim"), Sym("no")],
                 [Sym("in_bom"), Sym("yes")], [Sym("on_board"), Sym("yes")], [Sym("dnp"), Sym("no")],
                 [Sym("fields_autoplaced"), Sym("yes")],
                 [Sym("stroke"), [Sym("width"), 0.1524], [Sym("type"), Sym("solid")]],
                 [Sym("fill"), [Sym("color"), 0, 0, 0, 0.0]],
                 [Sym("uuid"), sheet_uuids[sheet]],
                 [Sym("property"), "Sheetname", title, [Sym("at"), x, y - 0.7, 0],
                  [Sym("effects"), [Sym("font"), [Sym("size"), 1.27, 1.27]], [Sym("justify"), Sym("left"), Sym("bottom")]]],
                 [Sym("property"), "Sheetfile", fn, [Sym("at"), x, y + 35.6, 0],
                  [Sym("effects"), [Sym("font"), [Sym("size"), 1.27, 1.27]], [Sym("justify"), Sym("left"), Sym("top")]]],
                 [Sym("instances"), [Sym("project"), PROJECT, [Sym("path"), f"/{root_uuid}", [Sym("page"), str(page)]]]]])
notes = ("Collegamenti per etichette globali: ogni pin ha il nome della sua net.\n"
         "Sorgente: design.py (rigenerare schema e PCB da li').\n"
         "Display JC4880P433 collegato con flat 2x13 su J5 (pin 1 = pin 1).\n"
         "Il display si alimenta dal flat JP1 (pin 2/4) tramite F7 + D8; la logica della scheda (3,3 V) viene dal display.\n"
         "J28/J29 = uscite 12 V / 5 V di servizio (PTC). J30 = GPIO liberi A6/A7 dell'MCP23017 (= SPARE OUT 1/2).\n"
         "Tre ingressi 12 V (riscaldatore 1, riscaldatore 2, principale). Parti LCSC/JLCPCB.")
root.append([Sym("text"), notes, [Sym("exclude_from_sim"), Sym("no")], [Sym("at"), 25, 160, 0],
             [Sym("effects"), [Sym("font"), [Sym("size"), 2, 2]], [Sym("justify"), Sym("left"), Sym("top")]], [Sym("uuid"), U()]])
root.append([Sym("sheet_instances"), [Sym("path"), "/", [Sym("page"), "1"]]])
root.append([Sym("embedded_fonts"), Sym("no")])
open(f"{OUT}/{PROJECT}.kicad_sch", "w").write(dump(root) + "\n")

json.dump(dict(root=root_uuid, sheets=sheet_uuids, symbols=symbol_uuid), open(f"{OUT}/uuids.json", "w"), indent=1)
print("ok", {s: len(v) for s, v in by_sheet.items()})
