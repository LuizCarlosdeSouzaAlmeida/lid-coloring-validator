"""Seleção de grafos para visualização."""

from typing import List, Optional
from .parser import GraphResult, RunSummary


def select_from_args(
    results: List[GraphResult],
    indices: Optional[List[int]] = None,
    chi: Optional[int] = None,
    chi_lid: Optional[int] = None,
) -> List[GraphResult]:
    filtered = results
    if chi is not None:
        filtered = [r for r in filtered if r.chi == chi]
    if chi_lid is not None:
        filtered = [r for r in filtered if r.chi_lid == chi_lid]
    if indices is not None:
        idx_set = set(indices)
        filtered = [r for r in filtered if r.index in idx_set]
    return filtered


def _print_table(results: list[GraphResult], distribution: dict) -> None:
    has_chi = any(r.chi is not None for r in results)
    if has_chi:
        print(f"\n{'Índice':>7}  {'chi':>4}  {'chi_lid':>7}")
        print("-" * 24)
        for r in results[:50]:
            chi_str = str(r.chi) if r.chi is not None else "-"
            print(f"{r.index:>7}  {chi_str:>4}  {r.chi_lid:>7}")
    else:
        print(f"\n{'Índice':>7}  {'chi_lid':>7}")
        print("-" * 17)
        for r in results[:50]:
            print(f"{r.index:>7}  {r.chi_lid:>7}")
    if len(results) > 50:
        print(f"  ... e mais {len(results) - 50} grafos")
    print()
    if distribution:
        if any(c != -1 for (c, _) in distribution):
            print("Distribuição (chi, chi_lid):")
            for (c, cl), cnt in sorted(distribution.items()):
                print(f"  chi={c} chi_lid={cl}: {cnt}")
        else:
            print("Distribuição chi_lid:")
            for (_, cl), cnt in sorted(distribution.items()):
                print(f"  chi_lid={cl}: {cnt}")
    print()


def select_interactive(summary: RunSummary) -> list[GraphResult]:
    _print_table(summary.results, summary.distribution)
    print("Selecione grafos para visualizar:")
    print("  all          → todos")
    print("  0,3,7        → índices específicos")
    print("  0-10         → faixa de índices")
    print("  chi_lid=5    → filtrar por chi_lid")
    print("  chi=3        → filtrar por chi")
    print("  chi=3,chi_lid=5")
    print("  q            → sair")

    while True:
        try:
            resp = input("\n> ").strip()
        except (EOFError, KeyboardInterrupt):
            return []

        if resp == "q":
            return []
        if resp == "all":
            return summary.results

        # filtros nomeados
        if "=" in resp:
            chi = chi_lid = None
            for part in resp.split(","):
                part = part.strip()
                if part.startswith("chi_lid="):
                    chi_lid = int(part.split("=")[1])
                elif part.startswith("chi="):
                    chi = int(part.split("=")[1])
            return select_from_args(summary.results, chi=chi, chi_lid=chi_lid)

        # faixa
        if "-" in resp:
            parts = resp.split("-")
            try:
                lo, hi = int(parts[0]), int(parts[1])
                return select_from_args(summary.results, indices=list(range(lo, hi + 1)))
            except (ValueError, IndexError):
                pass

        # lista de índices
        try:
            idxs = [int(x.strip()) for x in resp.split(",")]
            selected = select_from_args(summary.results, indices=idxs)
            if not selected:
                print("Nenhum grafo encontrado com esses índices.")
                continue
            return selected
        except ValueError:
            pass

        print("Entrada inválida, tente novamente.")
