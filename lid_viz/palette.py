"""ColorBrewer Set1 palette para colorações de grafos."""

# 8 cores: print-safe, daltonismo-safe com marcadores, até chi_lid=7
_COLORS = [
    "#e41a1c",  # 0 vermelho
    "#377eb8",  # 1 azul
    "#4daf4a",  # 2 verde
    "#ff7f00",  # 3 laranja
    "#984ea3",  # 4 roxo
    "#a65628",  # 5 marrom
    "#f781bf",  # 6 rosa
    "#999999",  # 7 cinza
]

_MARKERS = ["o", "s", "^", "D", "v", "P", "*", "X"]


def get_color(c: int) -> str:
    return _COLORS[c % len(_COLORS)]


def get_marker(c: int) -> str:
    return _MARKERS[c % len(_MARKERS)]


def node_colors(coloring: list[int]) -> list[str]:
    return [get_color(c) for c in coloring]


def node_markers(coloring: list[int]) -> list[str]:
    return [get_marker(c) for c in coloring]
