# -*- coding: utf-8 -*-
"""Assemble the deliverable folder for FilMachine_Reworked/PCB from a finished pipeline output. Usage: package.py out_dir dest_dir"""
import sys, os, shutil, zipfile, subprocess
OUT, DEST = sys.argv[1], sys.argv[2]
HERE = os.path.dirname(os.path.abspath(__file__))
PATCH = "/tmp/claude-0/-home-claude/02fb7139-d569-5287-a588-5e708a9135ba/scratchpad/patch"
shutil.rmtree(DEST, ignore_errors=True)
os.makedirs(f"{DEST}/produzione"); os.makedirs(f"{DEST}/kicad"); os.makedirs(f"{DEST}/sorgenti")
for fn in os.listdir(f"{OUT}/fab"):
    if os.path.isfile(f"{OUT}/fab/{fn}"):
        shutil.copy(f"{OUT}/fab/{fn}", f"{DEST}/produzione/{fn}")
for fn in ("FilMachine_Mainboard.kicad_pcb", "FilMachine_Mainboard.kicad_pro", "FilMachine_Mainboard.kicad_sch",
           "drives.kicad_sch", "io.kicad_sch", "outputs.kicad_sch", "power.kicad_sch"):
    shutil.copy(f"{OUT}/{fn}", f"{DEST}/kicad/{fn}")
shutil.copytree(f"{HERE}/FM.pretty", f"{DEST}/kicad/FM.pretty")
open(f"{DEST}/kicad/fp-lib-table", "w").write('(fp_lib_table\n  (version 7)\n  (lib (name "FM")(type "KiCad")(uri "${KIPRJMOD}/FM.pretty")(options "")(descr "FilMachine custom footprints (LCSC parts)"))\n)\n')
for fn in ("design.py", "placement.py", "power.py", "mkfp.py", "gen_sch.py", "build_pcb.py", "mkpro.py", "fanout.py", "route.py",
           "finish.py", "maze.py", "ripup.py", "gndfix.py", "fill_drc.py", "silk.py", "stubclean.py", "fullconn.py", "nudge.py",
           "gndvia.py", "export.py", "eco.py", "cmpnet.py", "padsdump.py", "sympins.py", "render.sh", "make.sh", "package.py"):
    shutil.copy(f"{HERE}/{fn}", f"{DEST}/sorgenti/{fn}")
shutil.copy("/home/claude/jlc/cpl_rotations_db.csv", f"{DEST}/sorgenti/cpl_rotations_db.csv")
shutil.copy(f"{HERE}/sorgenti_LEGGIMI.md", f"{DEST}/sorgenti/LEGGIMI.md")
shutil.copy(f"{HERE}/LEGGIMI_JLCPCB.md", f"{DEST}/LEGGIMI.md")
shutil.copy(f"{HERE}/NOTE_firmware_mainboard.md", f"{DEST}/NOTE_firmware.md")
shutil.copy(f"{PATCH}/firmware_mainboard.patch", f"{DEST}/firmware_mainboard.patch")
shutil.copy(f"{PATCH}/firmware_mainboard_completo.patch", f"{DEST}/firmware_mainboard_completo.patch")
z = zipfile.ZipFile(f"{DEST}/FilMachine_Mainboard_v1.1_JLCPCB.zip", "w", zipfile.ZIP_DEFLATED)
for fn in sorted(os.listdir(f"{DEST}/produzione")):
    z.write(f"{DEST}/produzione/{fn}", f"produzione/{fn}")
z.write(f"{DEST}/LEGGIMI.md", "LEGGIMI.md")
z.close()
print("packaged:", subprocess.run(["du", "-sh", DEST], capture_output=True, text=True).stdout.strip())
