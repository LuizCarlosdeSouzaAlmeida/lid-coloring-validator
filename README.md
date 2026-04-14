# LID Coloring Validator

Validator for **Locating-Identifying (LID) coloring** of graphs.

## What it does

Given a set of graphs (in `.g6` format), computes the LID chromatic number for each using brute force search.

## Build

```bash
gcc -O2 -c graphio.c -o graphio.o
g++ -std=c++17 -O2 -fopenmp -o lid_coloring lid_coloring.cpp graphio.o
```

## Usage

```bash
./lid_coloring <graph_file.g6>
```

