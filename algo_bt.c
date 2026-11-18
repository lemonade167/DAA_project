/* Backtracking and branch-and-bound modules */
#include "parking.h"

static int nq_rec(int n, int r, int *col, int *d1, int *d2, int *pos, int *first, int *found) {
    if (r == n) { if (!*found) { memcpy(first, pos, (size_t)n * sizeof(int)); *found = 1; } return 1; }
    int c = 0;
    for (int j = 0; j < n; j++) {
        OP();
        if (!col[j] && !d1[r + j] && !d2[r - j + n - 1]) {
            col[j] = d1[r + j] = d2[r - j + n - 1] = 1; pos[r] = j;
            c += nq_rec(n, r + 1, col, d1, d2, pos, first, found);
            col[j] = d1[r + j] = d2[r - j + n - 1] = 0;
        }
    }
    return c;
}
int nqueens(int n, int *first) {                          /* counts all solutions (n<=16) */
    if (n < 1 || n > 16) return 0;
    int col[16] = {0}, d1[32] = {0}, d2[32] = {0}, pos[16], found = 0, dummy[16];
    return nq_rec(n, 0, col, d1, d2, pos, first ? first : dummy, &found);
}

static int ss_rec(const int *a, int n, int i, int target, const int *suf, int *pick) {
    OP();
    if (target == 0) return 1;
    if (i == n || target < 0 || suf[i] < target) return 0;
    pick[i] = 1;
    if (ss_rec(a, n, i + 1, target - a[i], suf, pick)) return 1;
    pick[i] = 0;
    return ss_rec(a, n, i + 1, target, suf, pick);
}
int subset_sum(const int *a, int n, int target, int *pick) {   /* positive integers, n<=MAX_RES */
    int suf[MAX_RES + 1];
    suf[n] = 0;
    for (int i = n - 1; i >= 0; i--) suf[i] = suf[i + 1] + a[i];
    for (int i = 0; i < n; i++) pick[i] = 0;
    return ss_rec(a, n, 0, target, suf, pick);
}

static void kb_rec(const int *w, const int *v, const int *suf, int n, int i, int cap, int val, int *best) {
    OP();
    if (val > *best) *best = val;
    if (i == n || val + suf[i] <= *best) return;
    if (w[i] <= cap) kb_rec(w, v, suf, n, i + 1, cap - w[i], val + v[i], best);
    kb_rec(w, v, suf, n, i + 1, cap, val, best);
}
int knapsack_bt(const int *w, const int *v, int n, int cap) {
    int suf[MAX_RES + 1], best = 0;
    suf[n] = 0;
    for (int i = n - 1; i >= 0; i--) suf[i] = suf[i + 1] + v[i];
    kb_rec(w, v, suf, n, 0, cap, 0, &best);
    return best;
}

/* ---- branch and bound for 0/1 knapsack: FIFO, LIFO, least-cost ---- */
#define BB_POOL 200000
typedef struct { int lvl, p, wt; double b; } BBNode;

static double kbound(const int *w, const int *v, int n, int cap, int lvl, int p, int wt) {
    if (wt > cap) return 0;
    double b = p; int rem = cap - wt, i = lvl;
    for (; i < n && w[i] <= rem; i++) { OP(); rem -= w[i]; b += v[i]; }
    if (i < n) b += (double)v[i] * rem / w[i];
    return b;
}
int knapsack_bb(const int *w, const int *v, int n, int cap, int mode, long *nodes) {
    int sw[64], sv[64];
    if (n > 64) n = 64;
    for (int i = 0; i < n; i++) {                         /* sort by value density, descending */
        int xw = w[i], xv = v[i], j = i - 1;
        while (j >= 0 && (long)sv[j] * xw < (long)xv * sw[j]) { sw[j+1] = sw[j]; sv[j+1] = sv[j]; j--; }
        sw[j+1] = xw; sv[j+1] = xv;
    }
    BBNode *pool = talloc(BB_POOL * sizeof(BBNode));
    int *live = talloc(BB_POOL * sizeof(int));
    int np = 0, lo = 0, hi = 0, best = 0;
    long expanded = 0;
    pool[np].lvl = -1; pool[np].p = 0; pool[np].wt = 0; pool[np].b = kbound(sw, sv, n, cap, 0, 0, 0);
    live[hi++] = np++;
    while (lo < hi) {
        int id;
        if (mode == 0) id = live[lo++];                   /* FIFO queue  */
        else if (mode == 1) id = live[--hi];              /* LIFO stack  */
        else {                                            /* least cost = best bound first */
            int b = lo;
            for (int k = lo + 1; k < hi; k++) if (pool[live[k]].b > pool[live[b]].b) b = k;
            id = live[b]; live[b] = live[--hi];
        }
        BBNode cur = pool[id];
        if (cur.b <= best) continue;
        expanded++;
        int k = cur.lvl + 1;
        if (k >= n || np + 2 >= BB_POOL) { if (k < n) break; continue; }
        if (cur.wt + sw[k] <= cap) {
            BBNode c; c.lvl = k; c.p = cur.p + sv[k]; c.wt = cur.wt + sw[k];
            if (c.p > best) best = c.p;
            c.b = kbound(sw, sv, n, cap, k + 1, c.p, c.wt);
            if (c.b > best) { pool[np] = c; live[hi++] = np++; }
        }
        BBNode c2; c2.lvl = k; c2.p = cur.p; c2.wt = cur.wt;
        c2.b = kbound(sw, sv, n, cap, k + 1, c2.p, c2.wt);
        if (c2.b > best) { pool[np] = c2; live[hi++] = np++; }
    }
    free(pool); free(live);
    if (nodes) *nodes = expanded;
    return best;
}
