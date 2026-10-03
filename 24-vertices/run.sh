#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE="${BASE:-$SCRIPT_DIR}"
ROOT="$SCRIPT_DIR/.."
LID_BIN="${LID_BIN:-$ROOT/lid_coloring}"
LID_SRC="$ROOT/lid_coloring.cpp"
PARTS_DIR="$BASE/parts"
RESULTS_DIR="$BASE/results"
LOGS_DIR="$BASE/logs"
LOG="$BASE/progress.log"

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $*" | tee -a "$LOG"
}

if [ ! -x "$LID_BIN" ]; then
    echo "Erro: binario $LID_BIN nao encontrado" >&2
    exit 1
fi
if [ "$LID_SRC" -nt "$LID_BIN" ]; then
    echo "Erro: $LID_BIN mais antigo que $LID_SRC. Recompile:" >&2
    echo "  gcc -O2 -c graphio.c -o graphio.o && g++ -std=c++17 -O2 -fopenmp -o lid_coloring lid_coloring.cpp graphio.o" >&2
    exit 1
fi

mkdir -p "$RESULTS_DIR" "$LOGS_DIR"

if [ $# -gt 0 ]; then
    parts=()
    for name in "$@"; do parts+=("$PARTS_DIR/$name.g6"); done
else
    shopt -s nullglob
    parts=("$PARTS_DIR"/part_*.g6)
fi

if [ ${#parts[@]} -eq 0 ]; then
    echo "Erro: nenhuma parte em $PARTS_DIR. Rode ./split.sh primeiro." >&2
    exit 1
fi

current=""
trap '[ -n "$current" ] && log "$current INTERRUPTED"; exit 130' INT TERM

for part in "${parts[@]}"; do
    name=$(basename "$part" .g6)
    result="$RESULTS_DIR/$name.txt"

    if [ ! -f "$part" ]; then
        echo "Erro: $part nao existe" >&2
        exit 1
    fi
    if [ -f "$result" ]; then
        echo "$name ja concluida, pulando"
        continue
    fi

    expected=$(wc -l < "$part")
    current="$name"
    log "$name START graphs=$expected"
    start=$(date +%s)

    "$LID_BIN" "$part" --with-chi > "$result.tmp" 2> "$LOGS_DIR/$name.progress"
    status=$?

    elapsed=$(( $(date +%s) - start ))
    processed=$(sed -n 's/^Total: \([0-9]*\) grafos processados\.$/\1/p' "$result.tmp")

    if [ "$status" -eq 0 ] && [ "$processed" = "$expected" ]; then
        mv "$result.tmp" "$result"
        log "$name DONE elapsed=${elapsed}s"
    else
        log "$name FAIL exit=$status processed=${processed:-0}/$expected elapsed=${elapsed}s"
        exit 1
    fi
    current=""
done

log "FIM: todas as partes solicitadas concluidas"
