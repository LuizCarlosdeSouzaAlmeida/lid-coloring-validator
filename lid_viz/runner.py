"""Executa ./lid_coloring como subprocesso."""

import subprocess
from pathlib import Path
from typing import List, Optional


def run_lid_coloring(g6_file: Path, flags: Optional[List[str]] = None) -> str:
    """Executa ./lid_coloring, mostra barra de progresso no stderr, retorna stdout."""
    cmd = ["./lid_coloring", str(g6_file)] + (flags or ["--output-all"])
    result = subprocess.run(cmd, capture_output=False, stdout=subprocess.PIPE)
    if result.returncode != 0:
        raise RuntimeError(f"lid_coloring terminou com código {result.returncode}")
    return result.stdout.decode()
