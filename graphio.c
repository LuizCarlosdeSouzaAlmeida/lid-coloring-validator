// Funcoes de tratamento para grafos armazenados no formato .g6
// Funcoes extraidas do arquivo "showg.c" disponivel em: https://users.cecs.anu.edu.au/~bdm/data/formats.html

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define BIAS6 63
#define SMALLN 62
#define SMALLISHN 258047
#define TOPBIT6 32

#define SIZELEN(n) ((n) <= SMALLN ? 1 : ((n) <= SMALLISHN ? 4 : 8))
#define B(i) (1 << ((i)-1))
#define M(i) ((1 << (i)) - 1)

#ifdef __alpha
typedef unsigned int setword;
#else
typedef unsigned long setword;
#endif
typedef setword set;
typedef setword graph;
typedef int boolean;

#define WORDSIZE 32
#define TIMESWORDSIZE(w) ((w) << 5)
#define SETWD(pos) ((pos) >> 5)
#define SETBT(pos) ((pos) & 037)
#define ISELEMENT(setadd, pos) (((setadd)[SETWD(pos)] & bit[SETBT(pos)]) != 0)
#define ADDELEMENT(setadd, pos) ((setadd)[SETWD(pos)] |= bit[SETBT(pos)])
#define GRAPHROW(g, v, m) ((set *)(g) + (long)(v) * (long)(m))
#define FALSE 0
#define TRUE 1

static setword bit[32] = {020000000000, 010000000000, 04000000000, 02000000000,
                          01000000000,  0400000000,   0200000000,  0100000000,
                          040000000,    020000000,    010000000,   04000000,
                          02000000,     01000000,     0400000,     0200000,
                          0100000,      040000,       020000,      010000,
                          04000,        02000,        01000,       0400,
                          0200,         0100,         040,         020,
                          010,          04,           02,          01};

/* leftbit[b] = position (0–7) of leftmost set bit in byte b; 8 if b==0 */
static int leftbit[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define BITMASK(x) (017777777777UL >> (x))
#define FIRSTBIT(x)                                                            \
  ((x) & 037777600000UL ? ((x) & 037700000000UL ? leftbit[((x) >> 24) & 0377]  \
                                                : 8 + leftbit[(x) >> 16])      \
                        : ((x) & 0177400UL ? 16 + leftbit[(x) >> 8]            \
                                           : 24 + leftbit[(x) & 0377]))

#define FREES free
#define ALLOCS calloc
#define DYNALLSTAT(type, name, name_sz)                                        \
  static type *name;                                                           \
  static size_t name_sz = 0
#define DYNALLOC1(type, name, name_sz, sz, msg)                                \
  if ((size_t)(sz) > name_sz) {                                                \
    if (name_sz)                                                               \
      FREES(name);                                                             \
    name_sz = (sz);                                                            \
    if ((name = (type *)ALLOCS(sz, sizeof(type))) == NULL) {                   \
      if (msg)                                                                 \
        fprintf(stderr, "%s", msg);                                            \
      exit(1);                                                                 \
    }                                                                          \
  }
#define DYNREALLOC(type, name, name_sz, sz, msg)                               \
  if ((size_t)(sz) > name_sz) {                                                \
    if ((name = (type *)realloc(name, (sz) * sizeof(type))) == NULL) {         \
      if (msg)                                                                 \
        fprintf(stderr, "%s", msg);                                            \
      exit(1);                                                                 \
    } else                                                                     \
      name_sz = (sz);                                                          \
  }

char *showg_getline(FILE *f) {
  DYNALLSTAT(char, s, s_sz);
  int c;
  long i;

  DYNALLOC1(char, s, s_sz, 500, "showg_getline");

  i = 0;
  while ((c = getc(f)) != EOF && c != '\n') {
    if (i == s_sz - 2)
      DYNREALLOC(char, s, s_sz, s_sz + 1000, "showg_getline");
    s[i++] = c;
  }

  if (i == 0 && c == EOF)
    return NULL;

  if (c == '\n')
    s[i++] = '\n';
  s[i] = '\0';
  return s;
}

int graphsize(char *s) {
  char *p;
  int n;

  if (s[0] == ':' || s[0] == '&')
    p = s + 1;
  else
    p = s;
  n = *p++ - BIAS6;

  if (n > SMALLN) {
    n = *p++ - BIAS6;
    if (n > SMALLN) {
      n = *p++ - BIAS6;
      n = (n << 6) | (*p++ - BIAS6);
      n = (n << 6) | (*p++ - BIAS6);
      n = (n << 6) | (*p++ - BIAS6);
      n = (n << 6) | (*p++ - BIAS6);
      n = (n << 6) | (*p++ - BIAS6);
    } else {
      n = (n << 6) | (*p++ - BIAS6);
      n = (n << 6) | (*p++ - BIAS6);
    }
  }
  return n;
}

int graph_row_words(int n) {
  /* Minimum m such that TIMESWORDSIZE(m) >= n (words per row for n vertices).
   */
  return (n + WORDSIZE - 1) / WORDSIZE;
}

static void graphio_abort(const char *msg) {
  if (msg)
    fprintf(stderr, "%s", msg);
  exit(1);
}

void stringtograph(char *s, unsigned long *g, int m) {
  char *p;
  int n, i, j, k, v, x, nb, need;
  size_t ii;
  set *gi, *gj;
  boolean done;

  n = graphsize(s);
  if (n == 0)
    return;

  p = s + (s[0] == ':' || s[0] == '&') + SIZELEN(n);

  if (TIMESWORDSIZE(m) < n)
    graphio_abort("stringtograph: impossible m value\n");

  for (ii = (size_t)m * (size_t)n; ii > 0; --ii)
    g[ii - 1] = 0;

  if (s[0] != ':' && s[0] != '&') { /* graph6 format */
    k = 1;
    for (j = 1; j < n; ++j) {
      gj = GRAPHROW(g, j, m);
      for (i = 0; i < j; ++i) {
        if (--k == 0) {
          k = 6;
          x = *(p++) - BIAS6;
        }
        if ((x & TOPBIT6)) {
          gi = GRAPHROW(g, i, m);
          ADDELEMENT(gi, j);
          ADDELEMENT(gj, i);
        }
        x <<= 1;
      }
    }
  } else if (s[0] == '&') { /* digraph6 format */
    k = 1;
    for (i = 0; i < n; ++i) {
      gi = GRAPHROW(g, i, m);
      for (j = 0; j < n; ++j) {
        if (--k == 0) {
          k = 6;
          x = *(p++) - BIAS6;
        }
        if ((x & TOPBIT6))
          ADDELEMENT(gi, j);
        x <<= 1;
      }
    }
  } else { /* sparse6 format */
    for (i = n - 1, nb = 0; i != 0; i >>= 1, ++nb) {
    }
    k = 0;
    v = 0;
    done = FALSE;
    while (!done) {
      if (k == 0) {
        x = *(p++);
        if (x == '\n' || x == '\0') {
          done = TRUE;
          continue;
        }
        x -= BIAS6;
        k = 6;
      }
      if ((x & B(k)))
        ++v;
      --k;
      need = nb;
      j = 0;
      while (need > 0 && !done) {
        if (k == 0) {
          x = *(p++);
          if (x == '\n' || x == '\0') {
            done = TRUE;
            continue;
          }
          x -= BIAS6;
          k = 6;
        }
        if (need >= k) {
          j = (j << k) | (x & M(k));
          need -= k;
          k = 0;
        } else {
          k -= need;
          j = (j << need) | ((x >> k) & M(need));
          need = 0;
        }
      }
      if (done)
        continue;
      if (j > v)
        v = j;
      else if (v < n) {
        ADDELEMENT(GRAPHROW(g, v, m), j);
        ADDELEMENT(GRAPHROW(g, j, m), v);
      }
    }
  }
}

void putam(FILE *f, unsigned long *g, int linelength, int space, int triang,
           int m, int n) {
  set *gi;
  int i, j;
  int first;

  (void)linelength;
  for (i = 0, gi = (set *)g; i < n - (triang != 0); ++i, gi += m) {
    first = TRUE;
    for (j = triang ? i + 1 : 0; j < n; ++j) {
      if (!first && space)
        putc(' ', f);
      else
        first = FALSE;
      if (ISELEMENT(gi, j))
        putc('1', f);
      else
        putc('0', f);
    }
    putc('\n', f);
  }
}

boolean sao_vizinhos(unsigned long *g, int a, int b, int m) {
  set *gi;
  gi = (set *)g;
  gi += a * m;
  return ISELEMENT(gi, b);
}

/* Convert int i to decimal string in s; return length (excluding '\\0'). */
static int itos(int i, char *s) {
  int digit, j, k;
  char c;
  int ans;
  if (i < 0) {
    k = 0;
    i = -i;
    j = 1;
    s[0] = '-';
  } else {
    k = -1;
    j = 0;
  }
  do {
    digit = i % 10;
    i = i / 10;
    s[++k] = (char)(digit + '0');
  } while (i);
  s[k + 1] = '\0';
  ans = k + 1;
  for (; j < k; ++j, --k) {
    c = s[j];
    s[j] = s[k];
    s[k] = c;
  }
  return ans;
}

/* First element in set1 with position > pos; -1 if none. pos may be negative.
 */
int nextelement(set *set1, int m, int pos) {
  setword setwd;
  int w;
  if (pos < 0) {
    w = 0;
    setwd = set1[0];
  } else {
    w = SETWD(pos);
    setwd = set1[w] & BITMASK(SETBT(pos));
  }
  for (;;) {
    if (setwd != 0)
      return (int)(TIMESWORDSIZE(w) + FIRSTBIT(setwd));
    if (++w == m)
      return -1;
    setwd = set1[w];
  }
}

/* Write elements of set1 (m words) to f, starting from element start.
   curlenp: current line length, updated.
   linelength: wrap line when exceeding (0 = no wrap).
   compress: 1 to print ranges like "2:5" for 2,3,4,5.
   labelorg: added to each printed index (use 0 for 0-based). */
void putsetx(FILE *f, unsigned long *set1, int *curlenp, int linelength, int m,
             int compress, int start, int labelorg) {
  char s[40];
  int slen, j1, j2;
  int first = TRUE;

  j1 = start;
  while ((j1 = nextelement((set *)set1, m, j1)) >= 0) {
    j2 = j1;
    if (compress) {
      while (nextelement((set *)set1, m, j2) == j2 + 1)
        ++j2;
      if (j2 == j1 + 1)
        j2 = j1;
    }
    slen = itos(j1 + labelorg, s);
    if (j2 >= j1 + 2) {
      s[slen] = ':';
      slen += 1 + itos(j2 + labelorg, &s[slen + 1]);
    }
    if (*curlenp + slen + 1 >= linelength && linelength > 0) {
      fprintf(f, "\n ");
      *curlenp = 1;
    }
    if (first) {
      fprintf(f, "%s", s);
      *curlenp += slen;
      first = FALSE;
    } else {
      fprintf(f, " %s", s);
      *curlenp += slen + 1;
    }
    j1 = j2;
  }
}
