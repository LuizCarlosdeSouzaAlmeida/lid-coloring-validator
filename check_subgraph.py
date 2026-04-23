#!/usr/bin/env python3
"""
Verifica se um grafo é subgrafo (ou subgrafo induzido) de algum grafo
na coleção de grafos com chi_lid=6.

Uso:
  python check_subgraph.py <grafo.g6>
  python check_subgraph.py "<string_graph6>"
  python check_subgraph.py <grafo.g6> --induced
  python check_subgraph.py <grafo.g6> --collection outro_arquivo.g6
  python check_subgraph.py <grafo.g6> --all     # lista todos os matches
"""

import argparse
import sys
from pathlib import Path

import networkx as nx
from networkx.algorithms import isomorphism


def load_collection(path: Path) -> list[nx.Graph]:
    graphs = []
    with open(path) as f:
        for line in f:
            s = line.strip()
            if s:
                graphs.append(nx.from_graph6_bytes(s.encode()))
    return graphs


def parse_query(arg: str) -> nx.Graph:
    p = Path(arg)
    if p.exists():
        with open(p, "rb") as f:
            line = f.readline().strip()
        return nx.from_graph6_bytes(line)
    return nx.from_graph6_bytes(arg.strip().encode())


def is_induced_subgraph(host: nx.Graph, query: nx.Graph) -> bool:
    """Verifica se host contém um subgrafo induzido isomorfo a query."""
    gm = isomorphism.GraphMatcher(host, query)
    for mapping in gm.subgraph_isomorphisms_iter():
        # mapping: host_node -> query_node
        # subgraph_isomorphisms garante que arestas de query existem em host.
        # Para subgrafo induzido: arestas extras em host entre nós mapeados não podem existir.
        host_nodes = list(mapping.keys())
        induced_ok = True
        for i, u in enumerate(host_nodes):
            for v in host_nodes[i + 1:]:
                if host.has_edge(u, v) and not query.has_edge(mapping[u], mapping[v]):
                    induced_ok = False
                    break
            if not induced_ok:
                break
        if induced_ok:
            return True
    return False


def check_collection(
    query: nx.Graph,
    collection: list[nx.Graph],
    induced: bool,
    find_all: bool,
) -> list[tuple[int, nx.Graph]]:
    matches = []
    qn = query.number_of_nodes()
    qm = query.number_of_edges()

    for i, host in enumerate(collection):
        if qn > host.number_of_nodes():
            continue
        if qm > host.number_of_edges():
            continue

        if induced:
            found = is_induced_subgraph(host, query)
        else:
            gm = isomorphism.GraphMatcher(host, query)
            found = gm.subgraph_is_isomorphic()

        if found:
            matches.append((i, host))
            if not find_all:
                break

    return matches


def main() -> None:
    p = argparse.ArgumentParser(
        description="Verifica se um grafo é subgrafo de algum grafo na coleção lid6",
        epilog=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("query", help="Grafo: caminho para .g6 ou string graph6")
    p.add_argument(
        "--collection",
        default="results/lid6_collection.g6",
        metavar="FILE",
        help="Coleção de grafos (padrão: results/lid6_collection.g6)",
    )
    p.add_argument(
        "--induced",
        action="store_true",
        help="Verificar subgrafo induzido (padrão: subgrafo não-induzido)",
    )
    p.add_argument(
        "--all",
        action="store_true",
        dest="find_all",
        help="Listar todos os matches, não só o primeiro",
    )
    args = p.parse_args()

    coll_path = Path(args.collection)
    if not coll_path.exists():
        sys.exit(
            f"Erro: coleção não encontrada em '{coll_path}'.\n"
            "Execute primeiro:  python extract_lid6.py"
        )

    print(f"Carregando coleção de '{coll_path}'...", flush=True)
    collection = load_collection(coll_path)
    print(f"{len(collection)} grafos na coleção.\n")

    try:
        query = parse_query(args.query)
    except Exception as e:
        sys.exit(f"Erro ao ler o grafo de consulta: {e}")

    mode = "subgrafo induzido" if args.induced else "subgrafo"
    print(
        f"Grafo de consulta: n={query.number_of_nodes()}  m={query.number_of_edges()}\n"
        f"Modo: {mode}\n"
        "Buscando...",
        flush=True,
    )

    matches = check_collection(query, collection, args.induced, args.find_all)

    print()
    if matches:
        print(f"Resultado: SIM — encontrado em {len(matches)} grafo(s) da coleção.")
        for rank, (idx, host) in enumerate(matches, 1):
            print(
                f"  #{rank}: índice {idx} na coleção  "
                f"(n={host.number_of_nodes()}  m={host.number_of_edges()})"
            )
    else:
        print(f"Resultado: NÃO — o grafo não é {mode} de nenhum grafo na coleção.")


if __name__ == "__main__":
    main()
