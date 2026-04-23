#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Calcula a diferença entre dois arquivos .g6: A - B (grafos em A que não estão em B).

Uso:
  python g6_diff.py A.g6 B.g6              # imprime resultado na stdout
  python g6_diff.py A.g6 B.g6 -o saida.g6 # salva em arquivo
  python g6_diff.py A.g6 B.g6 --stats      # mostra contagens

Exemplo (girth exato):
  python g6_diff.py girth_ge5.g6 girth_ge6.g6 -o girth_eq5.g6
"""

import argparse
import sys
from pathlib import Path


def load_g6(path):
    graphs = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith(">>"):
                graphs.append(line)
    return graphs


def main():
    parser = argparse.ArgumentParser(description="Diferença de arquivos .g6: A - B")
    parser.add_argument("a", help="Arquivo A.g6 (minuendo)")
    parser.add_argument("b", help="Arquivo B.g6 (subtraendo)")
    parser.add_argument("-o", "--output", help="Arquivo de saída (padrão: stdout)")
    parser.add_argument("--stats", action="store_true", help="Mostra contagens")
    args = parser.parse_args()

    graphs_a = load_g6(args.a)
    set_b = set(load_g6(args.b))

    diff = [g for g in graphs_a if g not in set_b]

    if args.stats:
        print(f"|A|       = {len(graphs_a)}", file=sys.stderr)
        print(f"|B|       = {len(set_b)}", file=sys.stderr)
        print(f"|A - B|   = {len(diff)}", file=sys.stderr)

    if args.output:
        Path(args.output).write_text("\n".join(diff) + ("\n" if diff else ""))
        print(f"Salvo em {args.output} ({len(diff)} grafos).", file=sys.stderr)
    else:
        for g in diff:
            print(g)


if __name__ == "__main__":
    main()
