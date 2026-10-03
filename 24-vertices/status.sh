#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE="${BASE:-$SCRIPT_DIR}"
LOG="$BASE/progress.log"

shopt -s nullglob
parts=("$BASE"/parts/part_*.g6)
done_parts=("$BASE"/results/part_*.txt)
total=${#parts[@]}
done_count=${#done_parts[@]}

echo "Partes concluidas: $done_count/$total"

if [ ! -f "$LOG" ]; then
    echo "Nenhuma execucao registrada ainda."
    exit 0
fi

last=$(tail -n 1 "$LOG")
running=$(awk '$4 == "START" { p = $3 } $4 ~ /DONE|FAIL|INTERRUPTED/ && $3 == p { p = "" } END { print p }' "$LOG")

if [ -n "$running" ] && [ ! -f "$BASE/results/$running.txt" ]; then
    bar=$(grep -a -o '\[[#-]*\] *[0-9]*%  [0-9]*/[0-9]*' "$BASE/logs/$running.progress" 2>/dev/null | tail -n 1)
    echo "Em execucao: $running ${bar:-(sem progresso ainda)}"
fi

avg=$(awk '$4 == "DONE" { split($5, a, "="); s += a[2] + 0; n++ } END { if (n) printf "%d", s / n }' "$LOG")
remaining=$(( total - done_count ))
if [ -n "$avg" ] && [ "$remaining" -gt 0 ]; then
    eta=$(( avg * remaining ))
    printf "Media por parte: %dh%02dm | ETA restante (aprox.): %dh%02dm\n" \
        $((avg / 3600)) $((avg % 3600 / 60)) $((eta / 3600)) $((eta % 3600 / 60))
fi

echo "Ultimo evento: $last"
