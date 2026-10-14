#!/bin/bash
# Scarica in ./easyeda/ il footprint EasyEDA/JLCPCB di ogni codice LCSC di design.py (serve a jlcpos.py).
# L'API limita le richieste: in caso di errore 403 rilanciare dopo qualche secondo (scarica solo i mancanti).
cd "$(dirname "$0")"; mkdir -p easyeda
for c in $(python3 -c "import design; print(' '.join(sorted({p['lcsc'] for p in design.PARTS if p.get('lcsc') and not p['dnp']})))"); do
  f=easyeda/$c.json
  if [ -s "$f" ] && grep -q '"success":true' "$f"; then continue; fi
  curl -s --max-time 40 "https://easyeda.com/api/products/$c/components?version=6.4.19.5" -o "$f"; sleep 3
  grep -q '"success":true' "$f" && echo "ok $c" || echo "FAILED $c"
done
