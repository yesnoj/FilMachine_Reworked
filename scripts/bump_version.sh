#!/bin/bash
# bump_version.sh — Incrementa il campo BUILD di version.txt (MAJOR.MINOR.PATCH.BUILD)
# Fonte unica di verità della versione firmware: la stessa mostrata su Splash e Tools.
# Uso:
#   ./scripts/bump_version.sh            # incrementa BUILD (es. 0.0.0.1 -> 0.0.0.2)
#   ./scripts/bump_version.sh --show     # stampa la versione corrente senza modificarla
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
VF="$DIR/version.txt"

[ -f "$VF" ] || printf "0.0.0.0\n" > "$VF"
CUR="$(tr -d ' \r\n' < "$VF")"

if [ "$1" = "--show" ]; then
    echo "$CUR"
    exit 0
fi

MAJOR="$(echo "$CUR" | cut -d. -f1)"
MINOR="$(echo "$CUR" | cut -d. -f2)"
PATCH="$(echo "$CUR" | cut -d. -f3)"
BUILD="$(echo "$CUR" | cut -d. -f4)"
: "${MAJOR:=0}"; : "${MINOR:=0}"; : "${PATCH:=0}"; : "${BUILD:=0}"

BUILD=$((BUILD + 1))
NEW="$MAJOR.$MINOR.$PATCH.$BUILD"
printf "%s\n" "$NEW" > "$VF"
echo "Version: $CUR -> $NEW"
