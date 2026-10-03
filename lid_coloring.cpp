/*
  Calcula o numero cromatico LID (chi_lid) de um grafo por forca bruta.

  LID-Coloracao: coloracao propria onde, para todo par de vertices
  adjacentes (u,v) que NAO sejam gemeos verdadeiros, C(N[u]) != C(N[v]).

  Compilar: gcc -O2 -c graphio.c -o graphio.o && g++ -std=c++17 -O2 -fopenmp -o lid_coloring lid_coloring.cpp graphio.o
  Executar: ./lid_coloring <arquivo.g6>

  Suporta grafos com ate 128 vertices (__uint128_t para bitmasks de vizinhanca).
*/

#include "graphio.h"
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <map>
#include <cstdint>
#include <omp.h>
#include <cstring>

using namespace std;

thread_local int n, m;
thread_local vector<__uint128_t> viz_fechada;
thread_local vector<pair<int, int>> pares_checar;
thread_local vector<vector<pair<int, int>>> checar_no_passo;
thread_local vector<int> perm;

inline int ctz128(__uint128_t x) {
    uint64_t lo = (uint64_t)x;
    if (lo) return __builtin_ctzll(lo);
    return 64 + __builtin_ctzll((uint64_t)(x >> 64));
}

inline __uint128_t calc_viz_fechada(unsigned long *g, int v) {
    __uint128_t mask = (__uint128_t)1 << v;
    unsigned long *row = g + (long)v * m;
    int j = -1;
    while ((j = nextelement(row, m, j)) >= 0)
        mask |= (__uint128_t)1 << j;
    return mask;
}

int contar_twins(unsigned long *g, const vector<__uint128_t> &vf) {
    int cnt = 0;
    for (int u = 0; u < n; u++)
        for (int v = u + 1; v < n; v++)
            if (sao_vizinhos(g, u, v, m) && vf[u] == vf[v])
                cnt++;
    return cnt;
}

bool eh_claw_free(unsigned long *g) {
    for (int v = 0; v < n; v++) {
        vector<int> nbrs;
        unsigned long *row = g + (long)v * m;
        int j = -1;
        while ((j = nextelement(row, m, j)) >= 0)
            nbrs.push_back(j);
        int d = (int)nbrs.size();
        for (int i = 0; i < d; i++)
            for (int j2 = i + 1; j2 < d; j2++)
                for (int k = j2 + 1; k < d; k++) {
                    int a = nbrs[i], b = nbrs[j2], c = nbrs[k];
                    if (!sao_vizinhos(g, a, b, m) &&
                        !sao_vizinhos(g, a, c, m) &&
                        !sao_vizinhos(g, b, c, m))
                        return false;
                }
    }
    return true;
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

inline uint32_t color_set(__uint128_t nbr_mask, const vector<int> &coloring) {
    uint32_t colors = 0;
    while (nbr_mask) {
        int w = ctz128(nbr_mask);
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

struct Resultado { int chi, chi_lid, claw_free, twins; bool encontrou; vector<int> coloring_lid; vector<int> coloring_chi; };

Resultado processar_grafo(const string &gs, bool with_chi, bool with_colorings, bool with_claw_free, bool with_twins) {
    vector<char> buf(gs.begin(), gs.end());
    buf.push_back('\0');

    n = graphsize(buf.data());
    if (n > 128) {
        fprintf(stderr, "Erro: grafo com %d vertices excede limite de 128 (__uint128_t bitmask)\n", n);
        exit(1);
    }
    m = graph_row_words(n);

    vector<unsigned long> g((size_t)n * m);
    stringtograph(buf.data(), g.data(), m);

    int claw_free = with_claw_free ? (eh_claw_free(g.data()) ? 1 : 0) : -1;

    viz_fechada.resize(n);
    pares_checar.clear();
    checar_no_passo.assign(n, {});

    for (int v = 0; v < n; v++)
        viz_fechada[v] = calc_viz_fechada(g.data(), v);

    int twins = with_twins ? contar_twins(g.data(), viz_fechada) : -1;

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
        __uint128_t union_mask = viz_fechada[par.first] | viz_fechada[par.second];
        int max_passo = 0;
        __uint128_t tmp = union_mask;
        while (tmp) {
            int w = ctz128(tmp);
            tmp &= tmp - 1;
            if (inv[w] > max_passo) max_passo = inv[w];
        }
        checar_no_passo[max_passo].push_back(par);
    }

    int chi = -1;
    vector<int> chi_coloring;
    if (with_chi) {
        for (int k = 2; k <= n; k++) {
            vector<int> coloring(n, -1);
            coloring[perm[0]] = 0;
            if (backtrack_simples(1, k, 0, coloring, g.data())) {
                chi = k;
                if (with_colorings) chi_coloring = coloring;
                break;
            }
        }
    }

    int start_k = (chi != -1) ? chi : 2;
    for (int k = start_k; k <= n; k++) {
        vector<int> coloring(n, -1);
        coloring[perm[0]] = 0;
        if (backtrack(1, k, 0, coloring, g.data())) {
            vector<int> lid_col = with_colorings ? coloring : vector<int>{};
            return {chi, k, claw_free, twins, true, lid_col, chi_coloring};
        }
    }
    return {chi, -1, claw_free, twins, false, {}, chi_coloring};
}

// linhas_anteriores: quantas linhas foram impressas na chamada anterior
// (para apagar com cursor-up antes de redesenhar)
static int linhas_anteriores = 0;

static void imprimir_progresso(int feitos, int total,
                               const map<tuple<int,int,int,int>,int> &dist,
                               int sem_coloracao,
                               bool with_chi) {
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
    for (auto &[t, cnt] : dist) {
        auto [c, cl, cf, tw] = t;
        fprintf(stderr, "  ");
        if (with_chi) fprintf(stderr, "chi=%d ", c);
        fprintf(stderr, "chi_lid=%d", cl);
        if (cf != -1) fprintf(stderr, " claw_free=%s", cf ? "true" : "false");
        if (tw != -1) fprintf(stderr, " twins=%d", tw);
        fprintf(stderr, ": %d\n", cnt);
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
        fprintf(stderr, "Uso: %s <arquivo.g6> [--with-chi] [--with-colorings] [--output-all] [--chi-lid-filter N] [--claw-free] [--twins]\n", argv[0]);
        return 1;
    }

    bool output_all = false;
    bool with_chi = false;
    bool with_colorings = false;
    bool with_claw_free = false;
    bool with_twins = false;
    int chi_lid_filter = -1;
    const char *g6_file = nullptr;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--output-all") == 0) {
            output_all = true;
            with_chi = true;
            with_colorings = true;
            chi_lid_filter = -1;
        } else if (strcmp(argv[i], "--with-chi") == 0) {
            with_chi = true;
        } else if (strcmp(argv[i], "--with-colorings") == 0) {
            with_colorings = true;
        } else if (strcmp(argv[i], "--claw-free") == 0) {
            with_claw_free = true;
        } else if (strcmp(argv[i], "--twins") == 0) {
            with_twins = true;
        } else if (strcmp(argv[i], "--chi-lid-filter") == 0 && i + 1 < argc) {
            chi_lid_filter = atoi(argv[++i]);
        } else {
            g6_file = argv[i];
        }
    }
    if (!g6_file) { fprintf(stderr, "Erro: nenhum arquivo .g6 especificado\n"); return 1; }

    FILE *f = fopen(g6_file, "r");
    if (!f) { fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", g6_file); return 1; }

    vector<string> grafos;
    char *s;
    while ((s = showg_getline(f)) != NULL)
        grafos.push_back(string(s));
    fclose(f);

    if (grafos.empty()) { fprintf(stderr, "Erro: arquivo vazio\n"); return 1; }

    int total = (int)grafos.size();
    map<tuple<int,int,int,int>, int> dist_pares;
    int sem_coloracao = 0, feitos = 0;
    double ultimo_tick = omp_get_wtime();

    fprintf(stderr, "Processando %d grafos com %d thread(s)...\n",
            total, omp_get_max_threads());
    imprimir_progresso(0, total, dist_pares, sem_coloracao, with_chi);

    vector<Resultado> resultados(total);

    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < total; i++) {
        resultados[i] = processar_grafo(grafos[i], with_chi, with_colorings, with_claw_free, with_twins);

        #pragma omp critical
        {
            Resultado &r = resultados[i];
            if (r.encontrou) dist_pares[{with_chi ? r.chi : -1, r.chi_lid, r.claw_free, r.twins}]++;
            else sem_coloracao++;
            feitos++;
            double agora = omp_get_wtime();
            if (agora - ultimo_tick >= 0.5 || feitos == total) {
                imprimir_progresso(feitos, total, dist_pares, sem_coloracao, with_chi);
                ultimo_tick = agora;
            }
        }
    }
    fprintf(stderr, "\n");

    cout << "========================================\n";
    cout << "Total: " << total << " grafos processados.\n";

    bool print_per_graph = output_all || (chi_lid_filter != -1);
    if (print_per_graph) {
        cout << "\nResultados por grafo:\n";
        for (int i = 0; i < total; i++) {
            Resultado &r = resultados[i];
            if (!r.encontrou) continue;
            if (!output_all && r.chi_lid != chi_lid_filter) continue;

            cout << "graph " << i << ":";
            if (with_chi)
                cout << " chi=" << r.chi;
            cout << " chi_lid=" << r.chi_lid;
            if (r.claw_free != -1)
                cout << " claw_free=" << (r.claw_free ? "true" : "false");
            if (r.twins != -1)
                cout << " twins=" << r.twins;
            if (with_colorings) {
                cout << " chi_coloring=[";
                for (int v = 0; v < (int)r.coloring_chi.size(); v++)
                    cout << (v ? "," : "") << r.coloring_chi[v];
                cout << "] lid_coloring=[";
                for (int v = 0; v < (int)r.coloring_lid.size(); v++)
                    cout << (v ? "," : "") << r.coloring_lid[v];
                cout << "]";
            }
            cout << "\n";
        }
    }

    {
        cout << "\nDistribuicao por (";
        if (with_chi) cout << "chi, ";
        cout << "chi_lid";
        if (with_claw_free) cout << ", claw_free";
        if (with_twins) cout << ", twins";
        cout << "):\n";
        for (auto &[t, cnt] : dist_pares) {
            auto [c, cl, cf, tw] = t;
            cout << "  ";
            if (with_chi) cout << "chi=" << c << " ";
            cout << "chi_lid=" << cl;
            if (cf != -1) cout << " claw_free=" << (cf ? "true" : "false");
            if (tw != -1) cout << " twins=" << tw;
            cout << ": " << cnt << " grafo(s)\n";
        }
    }
    if (sem_coloracao > 0)
        cout << "  sem coloracao LID: " << sem_coloracao << " grafo(s)\n";
    return 0;
}
