/* Flow, bipartite matching and Chinese Remainder Theorem */
#include "parking.h"

static int ff_dfs(int n, int r[MAXN][MAXN], int u, int t, int f, int *vis) {
    OP();
    if (u == t) return f;
    vis[u] = 1;
    for (int v = 0; v < n; v++) {
        if (!vis[v] && r[u][v] > 0) {
            int d = ff_dfs(n, r, v, t, f < r[u][v] ? f : r[u][v], vis);
            if (d > 0) { r[u][v] -= d; r[v][u] += d; return d; }
        }
    }
    return 0;
}

/* Ford-Fulkerson with DFS augmenting paths. res (optional) receives the residual graph. */
int max_flow(int n, int cap[MAXN][MAXN], int s, int t, int res[MAXN][MAXN]) {
    static int r[MAXN][MAXN];
    memcpy(r, cap, sizeof r);
    int total = 0, vis[MAXN];
    for (;;) {
        memset(vis, 0, sizeof vis);
        int f = ff_dfs(n, r, s, t, INF, vis);
        if (f == 0) break;
        total += f;
    }
    if (res) memcpy(res, r, sizeof r);
    return total;
}

/* Maximum bipartite matching (drivers -> spaces) derived from max flow on a unit network. */
int bipartite_match(int nl, int nr, int adj[MAX_DRV][MAX_SPACES], int *matchL) {
    static int cap[MAXN][MAXN], res[MAXN][MAXN];
    if (nl > MAX_DRV || nr > MAX_SPACES || nl + nr + 2 > MAXN) return -1;
    memset(cap, 0, sizeof cap);
    int s = 0, t = nl + nr + 1;
    for (int i = 0; i < nl; i++) cap[s][1 + i] = 1;
    for (int j = 0; j < nr; j++) cap[1 + nl + j][t] = 1;
    for (int i = 0; i < nl; i++) for (int j = 0; j < nr; j++) if (adj[i][j]) cap[1 + i][1 + nl + j] = 1;
    int f = max_flow(nl + nr + 2, cap, s, t, res);
    for (int i = 0; i < nl; i++) {
        matchL[i] = -1;
        for (int j = 0; j < nr; j++)
            if (cap[1 + i][1 + nl + j] == 1 && res[1 + i][1 + nl + j] == 0) matchL[i] = j;
    }
    return f;
}

static long egcd(long a, long b, long *x, long *y) {
    if (!b) { *x = 1; *y = 0; return a; }
    long x1, y1, g = egcd(b, a % b, &x1, &y1);
    *x = y1; *y = x1 - (a / b) * y1;
    return g;
}
/* x = rem[i] (mod mod[i]) for pairwise-coprime moduli; returns 0 if not coprime. */
int crt(const int *rem, const int *mod, int n, long *x, long *M) {
    long m = 1, res = 0;
    for (int i = 0; i < n; i++) m *= mod[i];
    for (int i = 0; i < n; i++) {
        long Mi = m / mod[i], xi, yi;
        if (egcd(Mi % mod[i], mod[i], &xi, &yi) != 1) return 0;
        xi = ((xi % mod[i]) + mod[i]) % mod[i];
        res = (res + (((long)rem[i] % mod[i]) * Mi % m) * xi) % m;
    }
    *x = res; *M = m;
    return 1;
}
