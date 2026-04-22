"""Calcula posicionamento dos vértices para desenho."""

from typing import Dict
import networkx as nx
import numpy as np


def compute_layout(G: nx.Graph, seed: int = 42) -> Dict[int, np.ndarray]:
    n = G.number_of_nodes()
    if n <= 12:
        return nx.kamada_kawai_layout(G)
    if n <= 30:
        return nx.spring_layout(G, k=2 / (n ** 0.5), iterations=200, seed=seed)
    init = nx.spectral_layout(G)
    return nx.spring_layout(G, pos=init, k=2 / (n ** 0.5), iterations=100, seed=seed)
