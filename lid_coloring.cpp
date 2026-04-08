/*
  Calcula o numero cromatico LID (chi_lid) de um grafo por forca bruta.

  LID-Coloracao: coloracao propria onde, para todo par de vertices
  adjacentes (u,v) que NAO sejam gemeos verdadeiros, C(N[u]) != C(N[v]).

  Compilar: gcc -O2 -c graphio.c -o graphio.o && g++ -O2 -o lid_coloring lid_coloring.cpp graphio.o
  Executar: ./lid_coloring <arquivo.g6>
*/

#include "graphio.h"
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <map>

using namespace std;

int n, m;
vector<set<int>> viz_fechada;
vector<pair<int, int>> pares_checar;
vector<vector<pair<int, int>>> checar_no_passo;

// Vizinhanca fechada: N[v] = vizinhos(v) U {v}
set<int> calc_viz_fechada(unsigned long *g, int v) {
    set<int> nv;
    nv.insert(v);
    unsigned long *row = g + (long)v * m;
    int j = -1;
    while ((j = nextelement(row, m, j)) >= 0)
        nv.insert(j);
    return nv;
}

// Checa se atribuir 'cor' ao vertice 'v' mantem coloracao propria
bool cor_valida(int v, int cor, vector<int> &coloring, unsigned long *g) {
    unsigned long *row = g + (long)v * m;
    int j = -1;
    while ((j = nextelement(row, m, j)) >= 0) {
        if (coloring[j] == cor)
            return false;
    }
    return true;
}

// Backtracking com poda incremental de LID
bool backtrack(int v, int k, vector<int> &coloring, unsigned long *g) {
    if (v == n)
        return true;

    for (int cor = 0; cor < k; cor++) {
        if (cor_valida(v, cor, coloring, g)) {
            coloring[v] = cor;

            // Checar arestas LID que se tornam verificaveis neste passo
            bool lid_ok = true;
            for (auto &par : checar_no_passo[v]) {
                set<int> cn_a, cn_b;
                for (int w : viz_fechada[par.first])
                    cn_a.insert(coloring[w]);
                for (int w : viz_fechada[par.second])
                    cn_b.insert(coloring[w]);
                if (cn_a == cn_b) {
                    lid_ok = false;
                    break;
                }
            }

            if (lid_ok && backtrack(v + 1, k, coloring, g))
                return true;
            coloring[v] = -1;
        }
    }
    return false;
}

int main(int argc, char *argv[]) {
    // Verifica se o usuario passou o nome do arquivo como argumento
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.g6>\n", argv[0]);
        return 1;
    }

    // Abre o arquivo no formato graph6 (.g6), que contem um grafo por linha
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", argv[1]);
        return 1;
    }

    char *s;          // string graph6 da linha atual
    int graph_num = 0;
    map<int, int> dist_chi;   // dist_chi[k] = quantos grafos tiveram chi_lid == k
    int sem_coloracao = 0;

    // Processa um grafo por vez; showg_getline le a proxima linha do arquivo
    while ((s = showg_getline(f)) != NULL) {
        graph_num++;

        // n = numero de vertices; m = palavras de bits por linha na matriz de adjacencia
        n = graphsize(s);
        m = graph_row_words(n);

        // Decodifica a string graph6 na matriz de adjacencia compacta (bitset por linha)
        vector<unsigned long> g((size_t)n * m);
        stringtograph(s, g.data(), m);

        // Reinicia as estruturas globais para nao herdar dados do grafo anterior
        viz_fechada.clear();
        viz_fechada.resize(n);    // viz_fechada[v] = N[v] = vizinhos(v) U {v}
        pares_checar.clear();     // arestas (u,v) que precisam satisfazer C(N[u]) != C(N[v])
        checar_no_passo.clear();
        checar_no_passo.resize(n); // checar_no_passo[t] = arestas verificaveis apos colorir vertice t

        cout << "=== Grafo " << graph_num << ": n=" << n << " vertices ===" << endl;

        // --- Passo 1: calcular N[v] para cada vertice ---
        for (int v = 0; v < n; v++)
            viz_fechada[v] = calc_viz_fechada(g.data(), v);

        // --- Passo 2: identificar arestas LID a verificar ---
        // Uma aresta (u,v) e "gemeos verdadeiros" quando N[u] == N[v];
        // nesses casos a restricao C(N[u]) != C(N[v]) nunca pode ser satisfeita
        // (os multiconjuntos seriam identicos), entao a definicao LID os dispensa.
        // Todas as demais arestas adjacentes entram em pares_checar.
        int gemeos = 0;
        int arestas_total = 0;
        for (int u = 0; u < n; u++) {
            for (int v = u + 1; v < n; v++) {
                if (!sao_vizinhos(g.data(), u, v, m))
                    continue;
                arestas_total++;
                if (viz_fechada[u] == viz_fechada[v]) {
                    // Gemeos verdadeiros: N[u] == N[v], restricao LID nao se aplica
                    gemeos++;
                } else {
                    pares_checar.push_back({u, v});
                }
            }
        }
        cout << "Arestas: " << arestas_total
             << " | A verificar (LID): " << pares_checar.size()
             << " | Gemeos ignorados: " << gemeos << endl;

        // --- Passo 3: precomputar em qual "passo" cada aresta LID pode ser verificada ---
        // Durante o backtracking os vertices sao coloridos na ordem 0, 1, ..., n-1.
        // A restricao C(N[u]) != C(N[v]) so pode ser checada quando TODOS os vertices
        // de N[u] U N[v] ja tiverem recebido cor.  Isso ocorre quando o vertice de
        // maior indice em N[u] U N[v] for colorido.
        // max_idx[v] = maior indice de vertice em N[v]
        vector<int> max_idx(n);
        for (int v = 0; v < n; v++) {
            int mx = 0;
            for (int w : viz_fechada[v])
                if (w > mx) mx = w;
            max_idx[v] = mx;
        }
        // Para cada aresta (u,v), o passo de verificacao e max(max_idx[u], max_idx[v])
        for (auto &par : pares_checar) {
            int passo = max(max_idx[par.first], max_idx[par.second]);
            checar_no_passo[passo].push_back(par);
        }

        // --- Passo 4: busca pelo menor k tal que existe uma k-coloracao LID valida ---
        // Testa k = 2, 3, 4, ... ate encontrar (ou esgotar n cores, limite trivial)
        bool encontrou = false;
        for (int k = 2; k <= n; k++) {
            // Inicializa coloracao com todos os vertices sem cor (-1),
            // fixando o vertice 0 com a cor 0 para quebrar simetria de permutacao de cores
            vector<int> coloring(n, -1);
            coloring[0] = 0;
            if (backtrack(1, k, coloring, g.data())) {
                // Encontrou coloracao LID com k cores: imprime resultado
                cout << "chi_lid = " << k << "  Coloracao: [";
                for (int v = 0; v < n; v++) {
                    if (v > 0) cout << ", ";
                    cout << coloring[v];
                }
                cout << "]" << endl;
                dist_chi[k]++;
                encontrou = true;
                break;
            }
            // Nenhuma coloracao valida com k cores: tenta k+1
        }
        if (!encontrou) {
            cout << "Nenhuma coloracao LID encontrada!" << endl;
            sem_coloracao++;
        }

        cout << endl;
    }

    fclose(f);

    // Avisa se o arquivo estava vazio (nenhum grafo lido)
    if (graph_num == 0) {
        fprintf(stderr, "Erro: arquivo vazio\n");
        return 1;
    }

    cout << "========================================" << endl;
    cout << "Total: " << graph_num << " grafos processados." << endl;
    cout << "Distribuicao de chi_lid:" << endl;
    for (auto &[k, cnt] : dist_chi)
        cout << "  chi_lid = " << k << ": " << cnt << " grafo(s)" << endl;
    if (sem_coloracao > 0)
        cout << "  sem coloracao LID: " << sem_coloracao << " grafo(s)" << endl;
    return 0;
}
