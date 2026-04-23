#!/usr/bin/env python3
"""
Visualizador de colorações chi e chi_LID para grafos .g6.

Uso:
  python visualize_lid.py <arquivo.g6> [opções]
  python visualize_lid.py cub08.g6 --chi-lid 5
  python visualize_lid.py cub08.g6 --indices 0,2 --save-dir figuras/
  python visualize_lid.py cub08.g6 --list
  python visualize_lid.py cub08.g6 --from-output saida.txt
"""

import argparse
import sys
from pathlib import Path

import networkx as nx
import matplotlib
matplotlib.use("Agg")

import matplotlib.pyplot as plt

from lid_viz import parser as prs
from lid_viz import runner
from lid_viz import layout as lay
from lid_viz import drawing as drw
from lid_viz import selector as sel


def parse_indices(s: str) -> list[int]:
    result = []
    for part in s.split(","):
        part = part.strip()
        if "-" in part:
            a, b = part.split("-", 1)
            result.extend(range(int(a), int(b) + 1))
        else:
            result.append(int(part))
    return result


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Visualiza colorações χ e χ_LID de grafos .g6",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("g6_file", nargs="?", help="Arquivo .g6 de entrada")
    p.add_argument("--from-output", metavar="FILE",
                   help="Usar saída C++ já salva em arquivo (pula execução)")
    p.add_argument("--cpp-flags", metavar="FLAGS",
                   help="Flags extras para ./lid_coloring (padrão: --output-all)")
    p.add_argument("--indices", metavar="N[,N|N-M]",
                   help="Índices 0-based dos grafos a visualizar, ex: '0,3' ou '0-5'")
    p.add_argument("--chi", type=int, metavar="N", help="Filtrar por valor de χ")
    p.add_argument("--chi-lid", type=int, metavar="N", help="Filtrar por valor de χ_LID")
    p.add_argument("--save-dir", metavar="DIR", default="results",
                   help="Salvar figuras neste diretório (padrão: results/)")
    p.add_argument("--format", choices=["png", "pdf", "svg"], default="png",
                   help="Formato das figuras salvas (padrão: png)")
    p.add_argument("--no-show", action="store_true",
                   help="Não abrir janela (útil com --save-dir)")
    p.add_argument("--list", action="store_true",
                   help="Mostrar tabela de distribuição e sair")
    p.add_argument("--html", action="store_true",
                   help="Gerar HTML interativo com pyvis (além do matplotlib)")
    p.add_argument("--interactive", action="store_true",
                   help="Forçar seleção interativa mesmo com --indices")
    return p


def main() -> None:
    args = build_arg_parser().parse_args()

    # ── 1. Obter saída do C++ ────────────────────────────────────────────────
    if args.from_output:
        text = Path(args.from_output).read_text()
        g6_path = Path(args.g6_file) if args.g6_file else None
    elif args.g6_file:
        g6_path = Path(args.g6_file)
        if not g6_path.exists():
            sys.exit(f"Erro: arquivo não encontrado: {g6_path}")
        if args.cpp_flags:
            cpp_flags = args.cpp_flags.split()
        elif args.chi_lid is not None:
            cpp_flags = ["--chi-lid-filter", str(args.chi_lid), "--with-chi", "--with-colorings"]
        else:
            cpp_flags = ["--output-all"]
        print(f"Executando ./lid_coloring {g6_path} {' '.join(cpp_flags)} ...")
        text = runner.run_lid_coloring(g6_path, cpp_flags)
    else:
        sys.exit("Erro: informe um arquivo .g6 ou use --from-output.")

    # ── 2. Parsear ───────────────────────────────────────────────────────────
    summary = prs.parse_output(text)
    print(f"Total: {summary.total} grafos  |  com coloração: {len(summary.results)}")

    # ── 3. Anexar strings graph6 ─────────────────────────────────────────────
    if g6_path:
        g6_strings = prs.load_g6_strings(g6_path)
        for r in summary.results:
            if r.index < len(g6_strings):
                r.graph_str = g6_strings[r.index]

    # ── 4. Modo --list ───────────────────────────────────────────────────────
    if args.list:
        sel._print_table(summary.results, summary.distribution)
        return

    # ── 5. Selecionar grafos ─────────────────────────────────────────────────
    indices = parse_indices(args.indices) if args.indices else None
    selected = sel.select_from_args(summary.results, indices, args.chi, args.chi_lid)

    if not selected or args.interactive:
        selected = sel.select_interactive(summary)

    if not selected:
        print("Nenhum grafo selecionado.")
        return

    print(f"{len(selected)} grafo(s) para visualizar.")

    # ── 6. Desenhar ──────────────────────────────────────────────────────────
    save_dir = Path(args.save_dir) if args.save_dir else None
    show = not args.no_show

    for result in selected:
        if not result.graph_str:
            print(f"  [aviso] grafo {result.index}: sem string graph6, pulando.")
            continue

        G = nx.from_graph6_bytes(result.graph_str.encode())
        pos = lay.compute_layout(G)

        save_path = None
        if save_dir:
            chi_part = f"_chi{result.chi}" if result.chi is not None else ""
            fname = f"graph_{result.index}{chi_part}_chil{result.chi_lid}.{args.format}"
            save_path = save_dir / fname

        fig = drw.draw_comparison(G, result, pos=pos, save_path=save_path, show=show)
        plt.close(fig)

        if args.html and save_dir:
            chi_part = f"_chi{result.chi}" if result.chi is not None else ""
            html_path = save_dir / f"graph_{result.index}{chi_part}_chil{result.chi_lid}.html"
            drw.draw_html(G, result, html_path)

        if save_path:
            print(f"  salvo: {save_path}")


if __name__ == "__main__":
    main()
