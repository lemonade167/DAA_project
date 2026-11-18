#include "parking.h"
#include <stdint.h>

long g_ops = 0;
size_t g_mem = 0;
static uint64_t g_seed = 12345;

void metrics_reset(void) { g_ops = 0; g_mem = 0; }

void *talloc(size_t n) {
    void *p = calloc(1, n ? n : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(1); }
    g_mem += n;
    return p;
}

double now_ms(void) { return 1000.0 * (double)clock() / CLOCKS_PER_SEC; }

void rnd_seed(unsigned s) { g_seed = s; }
int rnd(int m) {
    g_seed = g_seed * 6364136223846793005ULL + 1442695040888963407ULL;
    return (int)((g_seed >> 33) % (uint64_t)m);
}

void graph_init(Graph *g, int n) {
    g->n = n;
    for (int i = 0; i < MAXN; i++)
        for (int j = 0; j < MAXN; j++) g->w[i][j] = (i == j) ? 0 : INF;
}
void graph_add(Graph *g, int u, int v, int w) { g->w[u][v] = w; g->w[v][u] = w; }
