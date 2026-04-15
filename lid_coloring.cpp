/*
  Calcula o numero cromatico LID (chi_lid) de um grafo por forca bruta.

  LID-Coloracao: coloracao propria onde, para todo par de vertices
  adjacentes (u,v) que NAO sejam gemeos verdadeiros, C(N[u]) != C(N[v]).

  Compilar: gcc -O2 -c graphio.c -o graphio.o && g++ -std=c++17 -O2 -fopenmp -o lid_coloring lid_coloring.cpp graphio.o
  Executar: ./lid_coloring <arquivo.g6>

  Suporta grafos com ate 64 vertices (uint64_t para bitmasks de vizinhanca).
*/

#include "graphio.h"
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <map>
#include <cstdint>
#include <omp.h>

using namespace std;

thread_local int n, m;
thread_local vector<uint64_t> viz_fechada;
thread_local vector<pair<int, int>> pares_checar;
thread_local vector<vector<pair<int, int>>> checar_no_passo;
thread_local vector<int> perm;

inline uint64_t calc_viz_fechada(unsigned long *g, int v) {
    uint64_t mask = (1ULL << v);
    unsigned long *row = g + (long)v * m;
    int j = -1;
    while ((j = nextelement(row, m, j)) >= 0)
        mask |= (1ULL << j);
    return mask;
}

inline bool cor_valida(int v, int cor, const vector<int> &coloring, unsigned long *g) {
    unsigned long *row = g + (long)v * m;
    int j = -1;
    while ((j = nextelement(row, m, j)) >= 0) {
        if (coloring[j] == cor)
            return false;
    }
    return true;
}

inline uint32_t color_set(uint64_t nbr_mask, const vector<int> &coloring) {
    uint32_t colors = 0;
    while (nbr_mask) {
        int w = __builtin_ctzll(nbr_mask);
        nbr_mask &= nbr_mask - 1;
        colors |= (1u << coloring[w]);
    }
    return colors;
}

vector<int> bfs_order(unsigned long *g) {
    vector<int> order;
    order.reserve(n);
    vector<bool> visited(n, false);
    queue<int> q;
    for (int s = 0; s < n; s++) {
        if (visited[s]) continue;
        q.push(s);
        visited[s] = true;
        while (!q.empty()) {
            int v = q.front(); q.pop();
            order.push_back(v);
            unsigned long *row = g + (long)v * m;
            int j = -1;
            while ((j = nextelement(row, m, j)) >= 0) {
                if (!visited[j]) {
                    visited[j] = true;
                    q.push(j);
                }
            }
        }
    }
    return order;
}

bool backtrack_simples(int v, int k, int max_cor, vector<int> &coloring, unsigned long *g) {
    if (v == n) return true;
    int vv = perm[v];
    int limite = min(max_cor + 1, k - 1);
    for (int cor = 0; cor <= limite; cor++) {
        if (cor_valida(vv, cor, coloring, g)) {
            coloring[vv] = cor;
            if (backtrack_simples(v + 1, k, max(max_cor, cor), coloring, g))
                return true;
            coloring[vv] = -1;
        }
    }
    return false;
}

bool backtrack(int v, int k, int max_cor, vector<int> &coloring, unsigned long *g) {
    if (v == n) return true;
    int vv = perm[v];
    int limite = min(max_cor + 1, k - 1);
    for (int cor = 0; cor <= limite; cor++) {
        if (cor_valida(vv, cor, coloring, g)) {
            coloring[vv] = cor;

            bool lid_ok = true;
            for (auto &par : checar_no_passo[v]) {
                if (color_set(viz_fechada[par.first],  coloring) ==
                    color_set(viz_fechada[par.second], coloring)) {
                    lid_ok = false;
                    break;
                }
            }

            if (lid_ok && backtrack(v + 1, k, max(max_cor, cor), coloring, g))
                return true;
            coloring[vv] = -1;
        }
    }
    return false;
}

struct Resultado { int chi, chi_lid; bool encontrou; };

Resultado processar_grafo(const string &gs) {
    vector<char> buf(gs.begin(), gs.end());
    buf.push_back('\0');

    n = graphsize(buf.data());
    m = graph_row_words(n);

    vector<unsigned long> g((size_t)n * m);
    stringtograph(buf.data(), g.data(), m);

    viz_fechada.resize(n);
    pares_checar.clear();
    checar_no_passo.assign(n, {});

    for (int v = 0; v < n; v++)
        viz_fechada[v] = calc_viz_fechada(g.data(), v);

    perm = bfs_order(g.data());
    vector<int> inv(n);
    for (int i = 0; i < n; i++) inv[perm[i]] = i;

    for (int u = 0; u < n; u++) {
        for (int v = u + 1; v < n; v++) {
            if (!sao_vizinhos(g.data(), u, v, m))
                continue;
            if (viz_fechada[u] != viz_fechada[v])
                pares_checar.push_back({u, v});
        }
    }

    for (auto &par : pares_checar) {
        uint64_t union_mask = viz_fechada[par.first] | viz_fechada[par.second];
        int max_passo = 0;
        uint64_t tmp = union_mask;
        while (tmp) {
            int w = __builtin_ctzll(tmp);
            tmp &= tmp - 1;
            if (inv[w] > max_passo) max_passo = inv[w];
        }
        checar_no_passo[max_passo].push_back(par);
    }

    int chi = -1;
    for (int k = 2; k <= n; k++) {
        vector<int> coloring(n, -1);
        coloring[perm[0]] = 0;
        if (backtrack_simples(1, k, 0, coloring, g.data())) { chi = k; break; }
    }

    for (int k = chi; k <= n; k++) {
        vector<int> coloring(n, -1);
        coloring[perm[0]] = 0;
        if (backtrack(1, k, 0, coloring, g.data()))
            return {chi, k, true};
    }
    return {chi, -1, false};
}

// linhas_anteriores: quantas linhas foram impressas na chamada anterior
// (para apagar com cursor-up antes de redesenhar)
static int linhas_anteriores = 0;

static void imprimir_progresso(int feitos, int total,
                               const map<pair<int,int>,int> &dist,
                               int sem_coloracao) {
    // Sobe o cursor para apagar o bloco anterior
    for (int i = 0; i < linhas_anteriores; i++)
        fprintf(stderr, "\033[A\r\033[K");

    // Barra de progresso
    int pct    = (int)((long long)feitos * 100 / total);
    int filled = (int)((long long)feitos * 40  / total);
    fprintf(stderr, "[");
    for (int j = 0; j < 40; j++)
        fprintf(stderr, "%c", j < filled ? '#' : '-');
    fprintf(stderr, "] %3d%%  %d/%d\n", pct, feitos, total);

    // Totalizadores em tempo real
    int linhas = 1;
    for (auto &[par, cnt] : dist) {
        fprintf(stderr, "  chi=%d chi_lid=%d: %d\n", par.first, par.second, cnt);
        linhas++;
    }
    if (sem_coloracao > 0) {
        fprintf(stderr, "  sem coloracao LID: %d\n", sem_coloracao);
        linhas++;
    }

    linhas_anteriores = linhas;
    fflush(stderr);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.g6>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) { fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", argv[1]); return 1; }

    vector<string> grafos;
    char *s;
    while ((s = showg_getline(f)) != NULL)
        grafos.push_back(string(s));
    fclose(f);

    if (grafos.empty()) { fprintf(stderr, "Erro: arquivo vazio\n"); return 1; }

    int total = (int)grafos.size();
    map<pair<int,int>, int> dist_pares;
    int sem_coloracao = 0, feitos = 0;
    double ultimo_tick = omp_get_wtime();

    fprintf(stderr, "Processando %d grafos com %d thread(s)...\n",
            total, omp_get_max_threads());
    imprimir_progresso(0, total, dist_pares, sem_coloracao);

    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < total; i++) {
        Resultado r = processar_grafo(grafos[i]);

        #pragma omp critical
        {
            if (r.encontrou) dist_pares[{r.chi, r.chi_lid}]++;
            else sem_coloracao++;
            feitos++;
            double agora = omp_get_wtime();
            if (agora - ultimo_tick >= 0.5 || feitos == total) {
                imprimir_progresso(feitos, total, dist_pares, sem_coloracao);
                ultimo_tick = agora;
            }
        }
    }
    fprintf(stderr, "\n");

    cout << "========================================\n";
    cout << "Total: " << total << " grafos processados.\n";
    cout << "Distribuicao por (chi, chi_lid):\n";
    for (auto &[par, cnt] : dist_pares)
        cout << "  chi=" << par.first << " chi_lid=" << par.second
             << ": " << cnt << " grafo(s)\n";
    if (sem_coloracao > 0)
        cout << "  sem coloracao LID: " << sem_coloracao << " grafo(s)\n";
    return 0;
}
