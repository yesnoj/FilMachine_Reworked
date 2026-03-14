#!/bin/bash
# flash.sh — Copy from simulator to firmware repo and flash the board
set -e

SIM=~/Desktop/FilMachine_Simulator_v2
REPO="/Users/francescoprochilo/Documents/GitHub/FilMachine_Pete"
PORT="/dev/cu.usbserial-02454091"

echo "=== Copying sources to firmware repo ==="
cp -r "$SIM"/{main,c_pages,c_elements,c_fonts,c_graphics} "$REPO/"

echo "=== Building and flashing ==="
cd "$REPO"
source $HOME/esp/esp-idf/export.sh
idf.py -p "$PORT" flash monitor
