/*
  Verifica se uma coloracao informada e uma LID-coloracao valida do grafo.

  LID-Coloracao: coloracao propria onde, para todo par de vertices
  adjacentes (u,v) que NAO sejam gemeos verdadeiros (N[u] != N[v]),
  C(N[u]) != C(N[v]) como conjunto de cores.

  Compilar:
    gcc -O2 -c graphio.c -o graphio.o
    g++ -std=c++17 -O2 -o verify_lid_coloring verify_lid_coloring.cpp graphio.o

  Uso:
    ./verify_lid_coloring <arquivo.g6> <coloracao> [--index N] [--zero-based]

  - <coloracao>: sequencia de digitos (uma cor por vertice).
    Por padrao espera cores 1..9 (1-indexado). Use --zero-based para 0..9.
  - --index N: escolhe o N-esimo grafo do arquivo (0-indexado, padrao 0).

  Suporta grafos com ate 128 vertices (__uint128_t para bitmasks).
*/

#include "graphio.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <set>

using namespace std;

static int ctz128(__uint128_t x) {
    uint64_t lo = (uint64_t)x;
    if (lo) return __builtin_ctzll(lo);
    return 64 + __builtin_ctzll((uint64_t)(x >> 64));
}

static __uint128_t calc_viz_fechada(unsigned long *g, int v, int m) {
    __uint128_t mask = (__uint128_t)1 << v;
    unsigned long *row = g + (long)v * m;
    int j = -1;
    while ((j = nextelement(row, m, j)) >= 0)
        mask |= (__uint128_t)1 << j;
    return mask;
}

static set<int> color_set(__uint128_t nbr_mask, const vector<int> &coloring) {
    set<int> cores;
    while (nbr_mask) {
        int w = ctz128(nbr_mask);
        nbr_mask &= nbr_mask - 1;
        cores.insert(coloring[w]);
    }
    return cores;
}

static string set_to_str(const set<int> &s, int offset) {
    string out = "{";
    bool first = true;
    for (int c : s) {
        if (!first) out += ",";
        out += to_string(c + offset);
        first = false;
    }
    out += "}";
    return out;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,
            "Uso: %s <arquivo.g6> <coloracao> [--index N] [--zero-based]\n",
            argv[0]);
        return 1;
    }

    const char *g6_file = nullptr;
    const char *coloracao_str = nullptr;
    int index = 0;
    bool zero_based = false;

    int positional = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--index") == 0 && i + 1 < argc) {
            index = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--zero-based") == 0) {
            zero_based = true;
        } else if (positional == 0) {
            g6_file = argv[i];
            positional++;
        } else if (positional == 1) {
            coloracao_str = argv[i];
            positional++;
        } else {
            fprintf(stderr, "Argumento desconhecido: %s\n", argv[i]);
            return 1;
        }
    }
    if (!g6_file || !coloracao_str) {
        fprintf(stderr, "Erro: arquivo .g6 e coloracao sao obrigatorios.\n");
        return 1;
    }

    FILE *f = fopen(g6_file, "r");
    if (!f) { fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", g6_file); return 1; }

    vector<string> grafos;
    char *s;
    while ((s = showg_getline(f)) != NULL)
        grafos.push_back(string(s));
    fclose(f);

    if (grafos.empty()) { fprintf(stderr, "Erro: arquivo vazio\n"); return 1; }
    if (index < 0 || index >= (int)grafos.size()) {
        fprintf(stderr, "Erro: indice %d fora do intervalo [0,%d]\n",
                index, (int)grafos.size() - 1);
        return 1;
    }

    vector<char> buf(grafos[index].begin(), grafos[index].end());
    buf.push_back('\0');

    int n = graphsize(buf.data());
    if (n > 128) {
        fprintf(stderr, "Erro: grafo com %d vertices excede limite de 128\n", n);
        return 1;
    }
    int m = graph_row_words(n);

    vector<unsigned long> g((size_t)n * m);
    stringtograph(buf.data(), g.data(), m);

    int len = (int)strlen(coloracao_str);
    if (len != n) {
        fprintf(stderr,
            "Erro: tamanho da coloracao (%d) != numero de vertices (%d)\n",
            len, n);
        return 1;
    }

    vector<int> coloring(n);
    int offset_print = zero_based ? 0 : 1;
    for (int v = 0; v < n; v++) {
        char ch = coloracao_str[v];
        if (ch < '0' || ch > '9') {
            fprintf(stderr, "Erro: caractere invalido '%c' na posicao %d\n", ch, v);
            return 1;
        }
        int c = ch - '0';
        if (!zero_based) {
            if (c < 1) {
                fprintf(stderr,
                    "Erro: cor '0' invalida na posicao %d (use --zero-based para 0..9)\n", v);
                return 1;
            }
            c -= 1;
        }
        coloring[v] = c;
    }

    int num_cores = 0;
    for (int v = 0; v < n; v++) num_cores = max(num_cores, coloring[v] + 1);

    cout << "Grafo: " << g6_file << " (indice " << index << ")\n";
    cout << "Vertices: " << n << "\n";
    cout << "Cores usadas: " << num_cores << "\n\n";

    bool propria_ok = true;
    int violacoes_proprias = 0;
    for (int u = 0; u < n; u++) {
        unsigned long *row = g.data() + (long)u * m;
        int j = -1;
        while ((j = nextelement(row, m, j)) >= 0) {
            if (j <= u) continue;
            if (coloring[u] == coloring[j]) {
                propria_ok = false;
                violacoes_proprias++;
                cout << "  [PROPRIA] aresta (" << u << "," << j
                     << ") tem mesma cor "
                     << (coloring[u] + offset_print) << "\n";
            }
        }
    }

    vector<__uint128_t> viz_fechada(n);
    for (int v = 0; v < n; v++)
        viz_fechada[v] = calc_viz_fechada(g.data(), v, m);

    bool lid_ok = true;
    int violacoes_lid = 0;
    int pares_checados = 0;
    int pares_gemeos = 0;

    for (int u = 0; u < n; u++) {
        for (int v = u + 1; v < n; v++) {
            if (!sao_vizinhos(g.data(), u, v, m)) continue;
            if (viz_fechada[u] == viz_fechada[v]) { pares_gemeos++; continue; }
            pares_checados++;
            set<int> Cu = color_set(viz_fechada[u], coloring);
            set<int> Cv = color_set(viz_fechada[v], coloring);
            if (Cu == Cv) {
                lid_ok = false;
                violacoes_lid++;
                cout << "  [LID] aresta (" << u << "," << v
                     << ") tem C(N[u])=C(N[v])="
                     << set_to_str(Cu, offset_print) << "\n";
            }
        }
    }

    cout << "\nResumo:\n";
    cout << "  Pares adjacentes nao-gemeos verificados: " << pares_checados << "\n";
    cout << "  Pares adjacentes gemeos verdadeiros (ignorados): " << pares_gemeos << "\n";
    cout << "  Coloracao propria: "
         << (propria_ok ? "SIM" : "NAO")
         << " (" << violacoes_proprias << " violacao(oes))\n";
    cout << "  Condicao LID: "
         << (lid_ok ? "SIM" : "NAO")
         << " (" << violacoes_lid << " violacao(oes))\n";

    bool ok = propria_ok && lid_ok;
    cout << "\nResultado: "
         << (ok ? "VALIDA" : "INVALIDA") << "\n";
    return ok ? 0 : 2;
}
