#!/bin/sh
# Rigenera tutto il pacchetto. Uso: sh all.sh /percorso/IDTFConverter
set -e
python3 build_all.py
python3 previews.py
python3 schemi.py
python3 dima.py
python3 make_bom.py
python3 make_pdf3d.py "$1"
