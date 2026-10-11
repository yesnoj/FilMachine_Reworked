import json, sys
OUT = sys.argv[1]
d = json.load(open('/usr/share/kicad/template/kicad.kicad_pro'))
def nc(name, track, clr=0.2, via=0.6, drill=0.3, prio=0):
    return {"name": name, "clearance": clr, "track_width": track, "via_diameter": via, "via_drill": drill,
            "microvia_diameter": 0.3, "microvia_drill": 0.1, "diff_pair_width": 0.2, "diff_pair_gap": 0.25,
            "diff_pair_via_gap": 0.25, "wire_width": 6, "bus_width": 12, "line_style": 0,
            "schematic_color": "rgba(0, 0, 0, 0.000)", "pcb_color": "rgba(0, 0, 0, 0.000)", "priority": prio}
HC = ["+12V_IN_H", "+12V_H1", "+12V_H2", "HEAT1_N", "HEAT2_N", "+12V_IN_M", "+12V_PUMP", "PUMP_OUT_A", "PUMP_OUT_B"]
PWR1 = ["+5V", "BUCK_SW", "MOT_OUT_A", "MOT_OUT_B"]
PWR = ["+12V", "+3V3", "+5V_SENS", "5V_DISP", "5V_DISP_F", "+5V_USB", "VBUS_C", "USBC_S", "SERVO_SIG", "3V3_DISP"] + [f"VLV{i}_N" for i in range(1, 9)]
d["net_settings"] = {
    "classes": [nc("Default", 0.25, prio=2147483647), nc("Power", 0.6, prio=1), nc("Power1", 1.0, prio=1), nc("HC", 2.0, clr=0.3, via=0.8, drill=0.4, prio=0)],
    "meta": {"version": 4},
    "net_colors": None,
    "netclass_assignments": None,
    "netclass_patterns": [{"netclass": "HC", "pattern": n} for n in HC] + [{"netclass": "Power", "pattern": n} for n in PWR] + [{"netclass": "Power1", "pattern": n} for n in PWR1],
}
d["meta"]["filename"] = "FilMachine_Mainboard.kicad_pro"
d["sheets"] = []
json.dump(d, open(f"{OUT}/FilMachine_Mainboard.kicad_pro", "w"), indent=2)
print("pro ok")
