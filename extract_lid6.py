#!/usr/bin/env python3
"""
Extrai todos os grafos com chi_lid=6 dos arquivos cub04-cub22.g6
e salva em results/lid6_collection.g6.

Suporta retomada: arquivos já processados são pulados (registrados em
results/lid6_extraction_state.txt).

Uso:
  python extract_lid6.py                                    # processa cub04-cub22
  python extract_lid6.py dataset/cub10.g6 ...               # arquivos específicos
  python extract_lid6.py -o results/saida.g6 ...            # arquivo de saída customizado
  python extract_lid6.py --reset                             # recomeça do zero
"""

import re
import subprocess
import sys
import time
from pathlib import Path

DEFAULT_FILES = [
    f"dataset/cub{n:02d}.g6" for n in range(4, 23, 2)
]

DEFAULT_OUTPUT = Path("results/lid6_collection.g6")

GRAPH_COUNTS = {
    "dataset/cub04.g6":       1,
    "dataset/cub06.g6":       2,
    "dataset/cub08.g6":       5,
    "dataset/cub10.g6":      19,
    "dataset/cub12.g6":      85,
    "dataset/cub14.g6":     509,
    "dataset/cub16.g6":    4060,
    "dataset/cub18.g6":   41301,
    "dataset/cub20.g6":  510489,
    "dataset/cub22.g6": 7319447,
}
SLOW_THRESHOLD = 10_000


def load_state_from(state_file: Path) -> set[str]:
    if state_file.exists():
        return set(state_file.read_text().splitlines())
    return set()


def save_state_to(state_file: Path, done: set[str]) -> None:
    state_file.write_text("\n".join(sorted(done)))


def run_lid_filter(g6_file: str) -> str:
    result = subprocess.run(
        ["./lid_coloring", g6_file, "--chi-lid-filter", "6"],
        stdout=subprocess.PIPE,
        stderr=None,
    )
    return result.stdout.decode()


def parse_indices(output: str) -> list[int]:
    return [int(m.group(1)) for m in re.finditer(r"^graph\s+(\d+):", output, re.MULTILINE)]


def extract_lines(g6_file: str, indices: list[int]) -> list[str]:
    idx_set = set(indices)
    lines = []
    with open(g6_file) as f:
        for i, line in enumerate(f):
            if i in idx_set:
                s = line.strip()
                if s:
                    lines.append(s)
    return lines


def main() -> None:
    args = sys.argv[1:]

    # extrai -o/--output
    output_path = DEFAULT_OUTPUT
    for flag in ("-o", "--output"):
        if flag in args:
            idx = args.index(flag)
            output_path = Path(args[idx + 1])
            args = args[:idx] + args[idx + 2:]
            break

    state_file = output_path.with_suffix(".state.txt")

    if "--reset" in args:
        output_path.unlink(missing_ok=True)
        state_file.unlink(missing_ok=True)
        print("Estado resetado.")
        args = [a for a in args if a != "--reset"]

    files = args if args else DEFAULT_FILES
    output_path.parent.mkdir(exist_ok=True)

    done = load_state_from(state_file)
    grand_total = 0

    # conta grafos já salvos para retomada limpa
    if output_path.exists() and done:
        grand_total = sum(1 for line in open(output_path) if line.strip())

    with open(output_path, "a") as out:
        for g6_path in files:
            if not Path(g6_path).exists():
                print(f"[aviso] {g6_path} não encontrado, pulando.")
                continue

            if g6_path in done:
                n_prev = GRAPH_COUNTS.get(g6_path, "?")
                print(f"[ok] {g6_path} ({n_prev} grafos) — já processado, pulando.")
                continue

            n_graphs = GRAPH_COUNTS.get(g6_path, "?")
            if isinstance(n_graphs, int) and n_graphs >= SLOW_THRESHOLD:
                mins = n_graphs // 5000
                print(f"Processando {g6_path} ({n_graphs:,} grafos — estimativa: ~{mins} min)...",
                      flush=True)
            else:
                print(f"Processando {g6_path} ({n_graphs} grafos)...", flush=True)

            t0 = time.time()
            output = run_lid_filter(g6_path)
            elapsed = time.time() - t0

            indices = parse_indices(output)
            g6_lines = extract_lines(g6_path, indices)

            for line in g6_lines:
                out.write(line + "\n")
            out.flush()

            grand_total += len(g6_lines)
            done.add(g6_path)
            save_state_to(state_file, done)

            print(f"  -> {len(g6_lines)} grafos com chi_lid=6  "
                  f"(tempo: {elapsed:.1f}s  |  total acumulado: {grand_total})")

    print(f"\nConcluído. {grand_total} grafos salvos em {output_path}")


if __name__ == "__main__":
    main()
