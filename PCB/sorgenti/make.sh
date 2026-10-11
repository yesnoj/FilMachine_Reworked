#!/bin/bash
# full pipeline: design.py -> schematic -> pcb -> power copper -> autoroute -> finish -> maze/plane ties -> GND stitching -> DRC
set -e
# abort if routing failed
O=${1:-out}
rm -rf $O && mkdir -p $O
python3 gen_sch.py $O > /dev/null
python3 mkpro.py $O > /dev/null
python3 build_pcb.py $O
python3 mkpro.py $O > /dev/null   # after build: saving a fresh board rewrites the project file
python3 power.py $O
python3 fanout.py $O/FilMachine_Mainboard.kicad_pcb --all 2>&1 | grep -v assert
IN2_POWER=1 python3 route.py $O/FilMachine_Mainboard.kicad_pcb ${MP:-150} ${MT:-0}
[ -f $O/FilMachine_Mainboard.ses ] || { echo "NO SES"; exit 1; }
B=$O/FilMachine_Mainboard.kicad_pcb
python3 finish.py $B 2>&1 | grep -v assert
python3 fill_drc.py $B > /dev/null 2>&1
python3 fanout.py $B --from-drc --auto 2>&1 | grep -v assert
for i in 1 2; do
  python3 fill_drc.py $B 2>&1 | grep "unconnected:"
  python3 maze.py $B 2>&1 | grep "maze routed"
  python3 fill_drc.py $B > /dev/null 2>&1
  python3 maze.py $B --tie 2>&1 | grep "tied"
  python3 gndfix.py $B 2>&1 | grep -v "assert\|no fanout"
done
python3 fill_drc.py $B 2>&1 | grep -v "silk\|starved_thermal |"
python3 silk.py $B 2>&1 | grep "silk ok"
python3 fill_drc.py $B 2>&1 | grep -v "silk\|starved_thermal |"
