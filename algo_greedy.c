/* Greedy modules */
#include "parking.h"

static void sort_by_end(Reservation *r, int n) {
    for (int i = 1; i < n; i++) {
        Reservation x = r[i]; int j = i - 1;
        while (j >= 0 && r[j].end > x.end) { OP(); r[j + 1] = r[j]; j--; }
        r[j + 1] = x;
    }
}
static void sort_by_start(Reservation *r, int n) {
    for (int i = 1; i < n; i++) {
        Reservation x = r[i]; int j = i - 1;
        while (j >= 0 && r[j].start > x.start) { OP(); r[j + 1] = r[j]; j--; }
        r[j + 1] = x;
    }
}

/* Max number of non-overlapping reservations on ONE space. Sorts r in place by end time. */
int activity_selection(Reservation *r, int n, int *chosen) {
    sort_by_end(r, n);
    int last = -INF, c = 0;
    for (int i = 0; i < n; i++) {
        OP();
        chosen[i] = 0;
        if (r[i].start >= last) { chosen[i] = 1; c++; last = r[i].end; }
    }
    return c;
}

/* Minimum number of spaces needed for all reservations. Sorts r in place by start time. */
int interval_partition(Reservation *r, int n, int *assign) {
    sort_by_start(r, n);
    int ends[MAX_RES], k = 0;
    for (int i = 0; i < n; i++) {
        int p = -1;
        for (int j = 0; j < k; j++) { OP(); if (ends[j] <= r[i].start) { p = j; break; } }
        if (p < 0) p = k++;
        ends[p] = r[i].end;
        assign[i] = p;
    }
    return k;
}

/* Fractional knapsack: divisible resource (e.g. EV-charger kW) given by value density. */
double fractional_knapsack(const int *w, const int *v, int n, double cap, double *frac) {
    int idx[MAX_RES];
    for (int i = 0; i < n; i++) { idx[i] = i; frac[i] = 0; }
    for (int i = 0; i < n - 1; i++) {
        int b = i;
        for (int j = i + 1; j < n; j++) {
            OP();
            if ((long)v[idx[j]] * w[idx[b]] > (long)v[idx[b]] * w[idx[j]]) b = j;
        }
        int t = idx[i]; idx[i] = idx[b]; idx[b] = t;
    }
    double tot = 0;
    for (int k = 0; k < n && cap > 0; k++) {
        int i = idx[k];
        if (w[i] <= cap) { frac[i] = 1; cap -= w[i]; tot += v[i]; }
        else { frac[i] = cap / w[i]; tot += v[i] * frac[i]; cap = 0; }
    }
    return tot;
}

/* Huffman coding of an event log */
typedef struct { long f; int l, r, ch, alive; } HNode;
static void hcode(HNode *t, int i, char *buf, int d, char codes[256][64]) {
    if (t[i].l < 0) {
        if (d == 0) { buf[0] = '0'; d = 1; }
        buf[d] = 0;
        snprintf(codes[t[i].ch], 64, "%s", buf);
        return;
    }
    buf[d] = '0'; hcode(t, t[i].l, buf, d + 1, codes);
    buf[d] = '1'; hcode(t, t[i].r, buf, d + 1, codes);
}
int huffman(const char *text, char codes[256][64], long *bits) {
    long cnt[256] = {0};
    for (const char *p = text; *p; p++) cnt[(unsigned char)*p]++;
    HNode t[512]; int nn = 0;
    for (int c = 0; c < 256; c++) { codes[c][0] = 0; if (cnt[c]) { t[nn].f = cnt[c]; t[nn].l = t[nn].r = -1; t[nn].ch = c; t[nn].alive = 1; nn++; } }
    int syms = nn, alive = nn;
    *bits = 0;
    if (!syms) return 0;
    while (alive > 1) {
        int a = -1, b = -1;
        for (int i = 0; i < nn; i++) {
            if (!t[i].alive) continue;
            OP();
            if (a < 0 || t[i].f < t[a].f) { b = a; a = i; }
            else if (b < 0 || t[i].f < t[b].f) b = i;
        }
        t[a].alive = t[b].alive = 0;
        t[nn].f = t[a].f + t[b].f; t[nn].l = a; t[nn].r = b; t[nn].ch = -1; t[nn].alive = 1; nn++;
        alive--;
    }
    char buf[256];
    hcode(t, nn - 1, buf, 0, codes);
    for (int c = 0; c < 256; c++) if (cnt[c]) *bits += cnt[c] * (long)strlen(codes[c]);
    return syms;
}

void dijkstra(const Graph *g, int s, int *dist, int *prev) {
    int done[MAXN] = {0}, n = g->n;
    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[s] = 0;
    for (int it = 0; it < n; it++) {
        int u = -1;
        for (int i = 0; i < n; i++) { OP(); if (!done[i] && dist[i] < INF && (u < 0 || dist[i] < dist[u])) u = i; }
        if (u < 0) break;
        done[u] = 1;
        for (int v = 0; v < n; v++) {
            OP();
            if (v != u && g->w[u][v] < INF && dist[u] + g->w[u][v] < dist[v]) { dist[v] = dist[u] + g->w[u][v]; prev[v] = u; }
        }
    }
}

int prim(const Graph *g, int *parent) {
    int n = g->n, key[MAXN], in[MAXN] = {0}, tot = 0;
    for (int i = 0; i < n; i++) { key[i] = INF; parent[i] = -1; }
    key[0] = 0;
    for (int it = 0; it < n; it++) {
        int u = -1;
        for (int i = 0; i < n; i++) { OP(); if (!in[i] && key[i] < INF && (u < 0 || key[i] < key[u])) u = i; }
        if (u < 0) return -1;                      /* disconnected */
        in[u] = 1; tot += key[u];
        for (int v = 0; v < n; v++)
            if (!in[v] && v != u && g->w[u][v] < key[v]) { key[v] = g->w[u][v]; parent[v] = u; }
    }
    return tot;
}

static int uf_find(int *p, int x) { while (p[x] != x) { p[x] = p[p[x]]; x = p[x]; } return x; }
int kruskal(const Graph *g, Edge *out, int *cnt) {
    static Edge e[MAXN * MAXN / 2];
    int m = 0, n = g->n, par[MAXN], tot = 0;
    for (int u = 0; u < n; u++) for (int v = u + 1; v < n; v++) if (g->w[u][v] < INF) { e[m].u = u; e[m].v = v; e[m].w = g->w[u][v]; m++; }
    for (int i = 1; i < m; i++) {                  /* insertion sort by weight */
        Edge x = e[i]; int j = i - 1;
        while (j >= 0 && e[j].w > x.w) { OP(); e[j + 1] = e[j]; j--; }
        e[j + 1] = x;
    }
    for (int i = 0; i < n; i++) par[i] = i;
    *cnt = 0;
    for (int i = 0; i < m && *cnt < n - 1; i++) {
        OP();
        int a = uf_find(par, e[i].u), b = uf_find(par, e[i].v);
        if (a != b) { par[a] = b; out[(*cnt)++] = e[i]; tot += e[i].w; }
    }
    return (*cnt == n - 1) ? tot : -1;
}
