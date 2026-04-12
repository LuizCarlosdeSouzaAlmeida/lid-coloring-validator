# LID Coloring Validator

Validator for **Locating-Identifying (LID) coloring** of graphs.

## What it does

Given a set of graphs (in `.g6` format), computes the LID chromatic number for each using brute force search.

## Build

```bash
gcc -c graphio.c
g++ -o lid_coloring lid_coloring.cpp graphio.o
```

## Usage

```bash
./lid_coloring <graph_file.g6>
```

## Data

- `list_70_graphs.g6` — 70 cubic graphs with girth ≥ 5 (main dataset)
- `cub20-gir5.g6`, `cub20-gir6.g6` — auxiliary graph lists
