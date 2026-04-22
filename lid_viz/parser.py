"""Parseia a saída stdout do programa lid_coloring."""

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

_NEW_RE = re.compile(
    r"^graph\s+(\d+):\s+chi=(\d+)\s+chi_lid=(\d+)"
    r"\s+chi_coloring=\[([^\]]*)\]"
    r"\s+lid_coloring=\[([^\]]*)\]"
)
_OLD_RE = re.compile(
    r"^\s*linha\s+(\d+)\s+\(indice\s+(\d+)\):\s+chi=(\d+)\s+chi_lid=(\d+)"
    r"\s+coloracao=\[([^\]]*)\]"
)
# linha por grafo sem colorações (ex: --chi-lid-filter sem --with-colorings)
_MINIMAL_RE = re.compile(
    r"^graph\s+(\d+):\s+(?:chi=(\d+)\s+)?chi_lid=(\d+)\s*$"
)
# distribuição só por chi_lid (sem --with-chi)
_DIST_ONLY_RE = re.compile(r"^\s+chi_lid=(\d+):\s+(\d+)\s+grafo")
_DIST_RE = re.compile(r"chi=(\d+)\s+chi_lid=(\d+):\s+(\d+)\s+grafo")


def _parse_ints(s: str) -> list[int]:
    s = s.strip()
    if not s:
        return []
    return [int(x) for x in s.split(",")]


@dataclass
class GraphResult:
    index: int
    chi: Optional[int]
    chi_lid: int
    chi_coloring: list[int] = field(default_factory=list)
    lid_coloring: list[int] = field(default_factory=list)
    graph_str: Optional[str] = None


@dataclass
class RunSummary:
    total: int
    results: list[GraphResult]
    distribution: dict[tuple[int, int], int]


def parse_output(text: str) -> RunSummary:
    results: list[GraphResult] = []
    distribution: dict[tuple[int, int], int] = {}
    total = 0

    for line in text.splitlines():
        m = _NEW_RE.match(line)
        if m:
            idx = int(m.group(1))
            chi = int(m.group(2))
            chi_lid = int(m.group(3))
            chi_col = _parse_ints(m.group(4))
            lid_col = _parse_ints(m.group(5))
            results.append(GraphResult(idx, chi, chi_lid, chi_col, lid_col))
            continue

        m = _OLD_RE.match(line)
        if m:
            idx = int(m.group(2))
            chi = int(m.group(3))
            chi_lid = int(m.group(4))
            lid_col = _parse_ints(m.group(5))
            results.append(GraphResult(idx, chi, chi_lid, [], lid_col))
            continue

        m = _MINIMAL_RE.match(line)
        if m:
            chi_val = int(m.group(2)) if m.group(2) else None
            results.append(GraphResult(int(m.group(1)), chi_val, int(m.group(3))))
            continue

        m = _DIST_RE.search(line)
        if m:
            distribution[(int(m.group(1)), int(m.group(2)))] = int(m.group(3))
            continue

        m = _DIST_ONLY_RE.search(line)
        if m:
            distribution[(-1, int(m.group(1)))] = int(m.group(2))
            continue

        m = re.search(r"Total:\s+(\d+)", line)
        if m:
            total = int(m.group(1))

    return RunSummary(total=total, results=results, distribution=distribution)


def load_g6_strings(g6_file: Path) -> list[str]:
    lines = []
    with open(g6_file, "rb") as f:
        for line in f:
            s = line.strip()
            if s:
                lines.append(s.decode("ascii", errors="replace"))
    return lines
