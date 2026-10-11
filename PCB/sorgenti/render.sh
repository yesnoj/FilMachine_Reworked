#!/bin/bash
# render.sh <pcb> <png> <layers> [width]
kicad-cli pcb export svg --mode-single --fit-page-to-board --exclude-drawing-sheet -l "$3" -o /tmp/r.svg "$1" >/dev/null 2>&1 && rsvg-convert -w ${4:-1400} -b white /tmp/r.svg -o "$2"
