#!/usr/bin/env python3
import os
import re
import sys
from collections import Counter
from pathlib import Path

BASE = Path(os.environ.get("BASE", Path(__file__).resolve().parent))
RESULTS_DIR = BASE / "results"
PARTS_DIR = BASE / "parts"
TOTAL_FILE = BASE / "total_graphs.txt"
OUTPUT = BASE / "resultados_24.md"
DATASET_NAME = "Generated_graphs.24.03.g6"

PAIR_LINE = re.compile(r"^\s*chi=(\d+) chi_lid=(\d+): (\d+) grafo\(s\)$")
NO_COLORING_LINE = re.compile(r"^\s*sem coloracao LID: (\d+) grafo\(s\)$")
TOTAL_LINE = re.compile(r"^Total: (\d+) grafos processados\.$")


def fmt_int(value):
    return f"{value:,}".replace(",", ".")


def fmt_pct(count, total):
    pct = 100 * count / total
    if pct < 0.01:
        return "< 0,01%"
    return f"{pct:.2f}%".replace(".", ",")


def parse_result(path):
    pairs = Counter()
    no_coloring = 0
    total = None
    for line in path.read_text().splitlines():
        if match := PAIR_LINE.match(line):
            pairs[(int(match[1]), int(match[2]))] += int(match[3])
        elif match := NO_COLORING_LINE.match(line):
            no_coloring = int(match[1])
        elif match := TOTAL_LINE.match(line):
            total = int(match[1])
    if total is None:
        sys.exit(f"Erro: {path} sem linha 'Total:'")
    if sum(pairs.values()) + no_coloring != total:
        sys.exit(f"Erro: {path} distribuicao nao soma o total ({total})")
    return pairs, no_coloring, total


def table(pairs, no_coloring, total, with_pct):
    header = "| χ | χ_lid | grafos |" + (" % |" if with_pct else "")
    rows = [header, "|---|---|---|" + ("---|" if with_pct else "")]
    for (chi, chi_lid), count in sorted(pairs.items()):
        row = f"| {chi} | {chi_lid} | {fmt_int(count)} |"
        if with_pct:
            row += f" {fmt_pct(count, total)} |"
        rows.append(row)
    if no_coloring:
        row = f"| — | sem coloração | {fmt_int(no_coloring)} |"
        if with_pct:
            row += f" {fmt_pct(no_coloring, total)} |"
        rows.append(row)
    return rows


def main():
    part_files = sorted(PARTS_DIR.glob("part_*.g6"))
    result_files = sorted(RESULTS_DIR.glob("part_*.txt"))
    done_names = {path.stem for path in result_files}
    missing = [path.stem for path in part_files if path.stem not in done_names]

    partials = [(path.stem, *parse_result(path)) for path in result_files]
    pairs = Counter()
    no_coloring = 0
    processed = 0
    for _, part_pairs, part_no_coloring, part_total in partials:
        pairs.update(part_pairs)
        no_coloring += part_no_coloring
        processed += part_total

    expected = int(TOTAL_FILE.read_text()) if TOTAL_FILE.exists() else None
    complete = not missing and expected == processed

    lines = ["# Resultados LID-Coloração — cúbicos n = 24", ""]
    if complete:
        lines.append("Execução completa (100%). Distribuição por (χ, χ_lid).")
    else:
        lines.append(
            f"**RESULTADO PARCIAL** — {fmt_int(processed)} de "
            f"{fmt_int(expected) if expected else '?'} grafos processados. "
            f"Partes pendentes: {', '.join(missing) or 'nenhuma'}."
        )
    lines += ["", "---", "", "## Resumo Geral", ""]
    lines += [f"### Série: todos os cúbicos — `{DATASET_NAME}` (n = 24)", ""]
    lines += [f"**{fmt_int(processed)} grafos** processados no total.", ""]
    if processed:
        lines += table(pairs, no_coloring, processed, with_pct=True)
        lines.append("")
    if pairs:
        max_lid = max(chi_lid for _, chi_lid in pairs)
        lid6 = sum(count for (_, chi_lid), count in pairs.items() if chi_lid == 6)
        lines.append(f"- **χ_lid máximo observado: {max_lid}**")
        lines.append(f"- Grafos com χ_lid = 6: {fmt_int(lid6)}")
        if max_lid > 6:
            lines.append("- **ATENÇÃO: χ_lid > 6 encontrado (conjectura χ_lid ≤ 6 violada)**")
    lines += ["", "---"]

    for name, part_pairs, part_no_coloring, part_total in partials:
        lines += ["", f"## {name}.g6", f"**{fmt_int(part_total)} grafos** | cúbicos n=24, parte de `{DATASET_NAME}`", ""]
        lines += table(part_pairs, part_no_coloring, part_total, with_pct=False)
        lines += ["", "---"]

    OUTPUT.write_text("\n".join(lines) + "\n")
    print(f"Gerado {OUTPUT} ({len(partials)} parte(s), {fmt_int(processed)} grafos)")
    if not complete:
        print("AVISO: resultado parcial — nem todas as partes foram concluidas", file=sys.stderr)


if __name__ == "__main__":
    main()
