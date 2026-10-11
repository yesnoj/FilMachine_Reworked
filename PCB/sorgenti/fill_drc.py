import sys, pcbnew, json, subprocess, collections
PCB = sys.argv[1]
b = pcbnew.LoadBoard(PCB)
f = pcbnew.ZONE_FILLER(b); f.Fill(b.Zones())
pcbnew.SaveBoard(PCB, b)
rep = PCB.replace(".kicad_pcb", "_drc.json")
subprocess.run(["kicad-cli", "pcb", "drc", "--format", "json", "--severity-all", "-o", rep, PCB], capture_output=True)
d = json.load(open(rep))
c = collections.Counter(v["type"] for v in d["violations"])
print("violations:", dict(c))
print("unconnected:", len(d.get("unconnected_items", [])))
nets = collections.Counter()
for u in d.get("unconnected_items", []):
    for it in u["items"]:
        s = it["description"]
        if "[" in s: nets[s.split("[")[1].split("]")[0]] += 1
want = sys.argv[2].split(",") if len(sys.argv) > 2 else None
print("unconnected by net:", dict(nets) if not want else {k: v for k, v in nets.items() if k in want})
for v in d["violations"]:
    if v["type"] not in ("silk_overlap", "silk_over_copper", "lib_footprint_issues", "lib_footprint_mismatch", "text_height", "silk_edge_clearance"):
        print(" ", v["type"], "|", " / ".join(i["description"][:70] for i in v["items"]))
