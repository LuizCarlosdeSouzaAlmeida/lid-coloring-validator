"""Desenha grafos com coloração chi e chi_lid lado a lado."""

from pathlib import Path
from typing import Optional, Dict
import numpy as np

import matplotlib
import matplotlib.pyplot as plt
import networkx as nx

from . import palette as pal  # noqa: E402
from . import layout as lay
from .parser import GraphResult

matplotlib.rcParams["figure.dpi"] = 100


def _nbhd_label(v: int, G: nx.Graph, coloring: list[int]) -> str:
    """Retorna o conjunto de cores de N[v] como string, ex: {0,2,3}"""
    nbhd = sorted({coloring[v]} | {coloring[u] for u in G.neighbors(v)})
    return "{" + ",".join(str(c) for c in nbhd) + "}"


def _draw_panel(
    ax: plt.Axes,
    G: nx.Graph,
    coloring: list[int],
    pos: dict,
    title: str,
    node_size: int = 500,
) -> None:
    if not coloring:
        ax.set_title(title + "\n(coloração não disponível)")
        ax.axis("off")
        return

    labels = {v: _nbhd_label(v, G, coloring) for v in G.nodes()}

    nx.draw_networkx_edges(G, pos, ax=ax, alpha=0.5, width=1.5)
    color_groups: dict[int, list[int]] = {}
    for v, c in enumerate(coloring):
        color_groups.setdefault(c, []).append(v)

    for c, nodes in color_groups.items():
        nx.draw_networkx_nodes(
            G, pos, nodelist=nodes, ax=ax,
            node_color=[pal.get_color(c)] * len(nodes),
            node_size=node_size,
        )

    # índice do vértice dentro do nó
    nx.draw_networkx_labels(
        G, pos,
        labels={v: str(v) for v in G.nodes()},
        ax=ax, font_size=7, font_color="white",
    )
    # conjunto de cores da vizinhança fechada — ao lado do nó
    x_vals = [pos[v][0] for v in G.nodes()]
    y_vals = [pos[v][1] for v in G.nodes()]
    x_range = max(x_vals) - min(x_vals) or 1
    y_range = max(y_vals) - min(y_vals) or 1
    offset = 0.035 * max(x_range, y_range)
    for v in G.nodes():
        x, y = pos[v]
        ax.text(
            x + offset, y + offset, labels[v],
            fontsize=7, ha="left", va="bottom",
            bbox=dict(boxstyle="round,pad=0.1", fc="white", ec="none", alpha=0.6),
        )
    ax.set_title(title, fontsize=11, fontweight="bold")
    ax.axis("off")

    # legenda de cores
    handles = [
        plt.Line2D(
            [0], [0],
            marker="o",
            color="w",
            markerfacecolor=pal.get_color(c),
            markersize=10,
            label=f"cor {c}",
        )
        for c in sorted(color_groups)
    ]
    ax.legend(handles=handles, loc="best", fontsize=7, framealpha=0.7)


def draw_comparison(
    G: nx.Graph,
    result: GraphResult,
    pos: Optional[dict] = None,
    save_path: Optional[Path] = None,
    show: bool = True,
) -> plt.Figure:
    if pos is None:
        pos = lay.compute_layout(G)

    n = G.number_of_nodes()
    m_edges = G.number_of_edges()
    fig, (ax_chi, ax_lid) = plt.subplots(1, 2, figsize=(13, 5))
    chi_part = f"  χ={result.chi}" if result.chi is not None else ""
    fig.suptitle(
        f"Grafo índice {result.index}  |  n={n}  m={m_edges}"
        f"{chi_part}  χ_LID={result.chi_lid}",
        fontsize=12,
    )

    chi_label = (
        f"χ-coloração  ({result.chi} cores)"
        if result.chi is not None
        else "χ-coloração  (não computada)"
    )
    _draw_panel(ax_chi, G, result.chi_coloring, pos, chi_label)
    _draw_panel(
        ax_lid, G, result.lid_coloring, pos,
        f"χ_LID-coloração  ({result.chi_lid} cores)",
    )

    fig.tight_layout()

    if save_path is not None:
        save_path = Path(save_path)
        save_path.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(save_path, dpi=300, bbox_inches="tight")

    if show:
        import tempfile, subprocess, os
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
            tmp_path = tmp.name
        fig.savefig(tmp_path, dpi=150, bbox_inches="tight")
        subprocess.Popen(["xdg-open", tmp_path])

    return fig


def draw_html(
    G: nx.Graph,
    result: GraphResult,
    save_path: Path,
) -> None:
    """Gera visualização interativa HTML com pyvis (opcional)."""
    try:
        from pyvis.network import Network
    except ImportError:
        raise ImportError("pyvis não instalado. Execute: pip install pyvis")

    save_path = Path(save_path)
    save_path.parent.mkdir(parents=True, exist_ok=True)

    for which, coloring, suffix in [
        ("chi", result.chi_coloring, "_chi"),
        ("lid", result.lid_coloring, "_lid"),
    ]:
        if not coloring:
            continue
        net = Network(height="600px", width="100%", bgcolor="#222", font_color="white")
        for v in G.nodes():
            c = coloring[v]
            net.add_node(
                v,
                label=str(v),
                color=pal.get_color(c),
                title=f"vértice {v}, cor {c}",
            )
        for u, v in G.edges():
            net.add_edge(u, v)
        out = save_path.with_name(save_path.stem + suffix + ".html")
        net.write_html(str(out))
