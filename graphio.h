// Cabecalho personalizado para que as funcoes do arquivo "graphio.c" possam ser utilizadas em outros arquivos

#ifndef GRAPHIO_H
#define GRAPHIO_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int boolean;

char* showg_getline(FILE *f);

int graphsize(char *s);

int graph_row_words(int n);

void stringtograph(char *s, unsigned long *g, int m);

void putam(FILE *f, unsigned long *g, int linelength, int space, int triang, int m, int n);

boolean sao_vizinhos(unsigned long *g, int a, int b, int m);

int nextelement(unsigned long *set1, int m, int pos);

void putsetx(FILE *f, unsigned long *set1, int *curlenp, int linelength, int m, int compress, int start, int labelorg);

#ifdef __cplusplus
}
#endif

#endif // GRAPHIO_H
