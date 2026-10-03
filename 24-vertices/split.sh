#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE="${BASE:-$SCRIPT_DIR}"
SRC="${SRC:-$SCRIPT_DIR/../dataset/Generated_graphs.24.03.g6}"
PART_SIZE="${PART_SIZE:-10000000}"
PARTS_DIR="$BASE/parts"

if compgen -G "$PARTS_DIR/part_*.g6" > /dev/null; then
    echo "Erro: $PARTS_DIR ja contem partes. Apague-as antes de dividir novamente." >&2
    exit 1
fi

mkdir -p "$PARTS_DIR"
split -l "$PART_SIZE" -d -a 2 --additional-suffix=.g6 "$SRC" "$PARTS_DIR/part_"

total_src=$(wc -l < "$SRC")
total_parts=$(cat "$PARTS_DIR"/part_*.g6 | wc -l)
wc -l "$PARTS_DIR"/part_*.g6

if [ "$total_src" -ne "$total_parts" ]; then
    echo "Erro: origem tem $total_src linhas, partes somam $total_parts" >&2
    exit 1
fi

echo "$total_src" > "$BASE/total_graphs.txt"
echo "OK: $total_src grafos em $(ls "$PARTS_DIR"/part_*.g6 | wc -l) partes"
