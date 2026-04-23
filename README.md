# LID Coloring Validator

Validador do **número cromático LID** (chi_lid) de grafos, via busca por força bruta com backtracking.

## O que é LID-coloração?

Uma **LID-coloração** (*Locating-Identifying Coloring*) de um grafo G é uma coloração própria dos vértices tal que, para todo par de vértices adjacentes u e v que **não** sejam gêmeos verdadeiros, os multiconjuntos de cores de suas vizinhanças fechadas sejam distintos:

```
C(N[u]) ≠ C(N[v])   para todo par {u,v} ∈ E(G) com N[u] ≠ N[v]
```

onde `N[v]` denota a vizinhança fechada de v (v e todos os seus vizinhos) e `C(S)` denota o conjunto (ou multiconjunto) de cores dos vértices de S.

O **número cromático LID**, denotado `chi_lid(G)`, é o menor número de cores necessário para uma LID-coloração de G.

A condição de identificação exige que vértices adjacentes não-gêmeos possam ser distinguidos pela "assinatura de cores" de suas vizinhanças, combinando propriedades das colorações identificadoras e das colorações localizadoras.

## Técnicas de redução da árvore de busca

O algoritmo usa backtracking para encontrar o menor `k` tal que existe uma k-coloração LID válida. Diversas técnicas são empregadas para tornar a busca tratável:

### 1. Ordenação de vértices por BFS

Os vértices são coloridos na ordem de uma busca em largura (BFS) a partir de cada componente conexa. Essa ordem garante que, ao colorir um vértice `v`, grande parte de sua vizinhança já está colorida, o que:

- Maximiza a propagação de restrições da coloração própria logo nos primeiros passos;
- Permite verificar as restrições LID mais cedo na árvore de busca.

### 2. Verificação incremental das restrições LID

Para cada par de vértices adjacentes não-gêmeos `{u, v}` que precisa ser distinguido, a restrição LID só pode ser verificada quando **todos** os vértices de `N[u] ∪ N[v]` já tiverem sido coloridos. O algoritmo pré-computa, para cada par, o índice do último vértice dessa união na ordem BFS (`max_passo`), e registra o par na lista `checar_no_passo[max_passo]`.

Assim, a cada passo `v` do backtracking, apenas os pares cujo último vértice relevante é exatamente `v` são verificados — evitando verificações redundantes e podando ramos inviáveis o mais cedo possível.

### 3. Quebra de simetria

Para eliminar colorações equivalentes por permutação de cores, o primeiro vértice da ordem BFS é fixado na cor 0. Além disso, ao expandir qualquer vértice, apenas cores no intervalo `[0, max_cor_usado + 1]` são tentadas, onde `max_cor_usado` é o maior índice de cor já atribuído. Isso evita que a busca explore atribuições que diferem apenas pela renomeação das cores.

### 4. Busca em dois estágios: chi antes de chi_lid (opt-in via `--with-chi`)

Com a flag `--with-chi`, o algoritmo primeiro calcula o número cromático clássico `chi(G)` com um backtracking simples (sem as restrições LID). Esse valor é então usado como limite inferior para a busca do `chi_lid`, pois toda LID-coloração é também uma coloração própria. Isso evita testar valores de `k` menores que `chi(G)`. Por padrão (sem a flag), a busca LID começa diretamente em `k=2`.

### 5. Paralelismo com OpenMP

Quando o arquivo de entrada contém múltiplos grafos, cada grafo é processado de forma independente. O laço principal é paralelizado com `#pragma omp parallel for schedule(dynamic, 1)`, distribuindo os grafos entre as threads disponíveis. Cada thread mantém seu próprio estado local (`thread_local`) para evitar condições de corrida. A diretiva `schedule(dynamic, 1)` balanceia a carga dinamicamente, uma vez que grafos diferentes podem ter tempos de resolução muito distintos.

## Build

```bash
gcc -O2 -c graphio.c -o graphio.o
g++ -std=c++17 -O2 -fopenmp -o lid_coloring lid_coloring.cpp graphio.o
```

## Uso

```bash
./lid_coloring <arquivo.g6> [--with-chi] [--with-colorings] [--output-all] [--chi-lid-filter N]
```

O programa aceita arquivos no formato **graph6** (`.g6`), suportando grafos com até 64 vértices.

### Modo rápido (padrão)

Sem flags, calcula apenas `chi_lid` e imprime a distribuição. Não há saída por grafo, não há backtracking para `chi`:

```bash
./lid_coloring dataset/cub08.g6
```
```
Distribuicao por chi_lid:
  chi_lid=3: 5 grafo(s)
  chi_lid=4: 17 grafo(s)
```

### `--with-chi`

Computa `chi` separadamente (backtracking extra) e usa-o como limite inferior para a busca LID. Distribui os resultados por pares `(chi, chi_lid)`:

```bash
./lid_coloring dataset/cub08.g6 --with-chi
```
```
Distribuicao por (chi, chi_lid):
  chi=2 chi_lid=3: 5 grafo(s)
  chi=3 chi_lid=4: 17 grafo(s)
```

### `--with-colorings`

Armazena e imprime as colorações reais por grafo. Requer `--output-all` ou `--chi-lid-filter` para que as linhas apareçam:

```bash
./lid_coloring dataset/cub08.g6 --chi-lid-filter 4 --with-chi --with-colorings
```
```
graph 1: chi=3 chi_lid=4 chi_coloring=[0,1,2,...] lid_coloring=[0,1,2,...]
```

### `--output-all`

Atalho que ativa `--with-chi`, `--with-colorings` e imprime todos os grafos. Equivalente ao comportamento completo de análise (retrocompatível):

```bash
./lid_coloring dataset/cub08.g6 --output-all
```

### `--chi-lid-filter N`

Imprime linhas apenas dos grafos com `chi_lid = N`. Combine com `--with-chi` e/ou `--with-colorings` conforme necessário:

```bash
./lid_coloring dataset/cub08.g6 --chi-lid-filter 5 --with-chi --with-colorings
```

---

## Visualizador Python

Programa para desenhar grafos com as colorações χ e χ\_LID lado a lado.

### Pré-requisitos

- Python 3.9+ (sistema) — **não usar** versões do pyenv compiladas sem `libbz2-dev`
- Virtualenv com as dependências do `requirements.txt`

### Setup

```bash
python3.9 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### Uso

```bash
# Executar o C++ e visualizar grafos com chi_lid=5 (modo interativo)
python visualize_lid.py dataset/cub08.g6 --chi-lid 5

# Listar distribuição sem desenhar
python visualize_lid.py dataset/cub08.g6 --output-all --list

# Visualizar grafos específicos por índice e salvar PNGs
python visualize_lid.py dataset/cub08.g6 --indices 0,2 --save-dir figuras/ --no-show

# Usar saída C++ já salva (evita reprocessar)
./lid_coloring dataset/cub10.g6 --output-all > saida.txt
python visualize_lid.py dataset/cub10.g6 --from-output saida.txt --chi-lid 5

# Gerar HTML interativo com pyvis
python visualize_lid.py dataset/cub08.g6 --indices 0 --html --save-dir figuras/ --no-show
```

### O que é gerado

Cada figura contém dois painéis lado a lado com o mesmo posicionamento dos vértices:

- **Esquerda:** coloração própria mínima (χ cores)
- **Direita:** coloração LID mínima (χ\_LID cores)

Cada classe de cor recebe uma cor e um marcador distintos, tornando as figuras legíveis também em impressão em escala de cinza.
