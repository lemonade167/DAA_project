/* Dynamic programming modules */
#include "parking.h"

int knapsack_dp(const int *w, const int *v, int n, int cap, int *pick) {
    int W = cap + 1;
    int *dp = talloc((size_t)(n + 1) * W * sizeof(int));
    for (int i = 1; i <= n; i++)
        for (int c = 0; c <= cap; c++) {
            OP();
            dp[i*W + c] = dp[(i-1)*W + c];
            if (w[i-1] <= c) {
                int t = dp[(i-1)*W + c - w[i-1]] + v[i-1];
                if (t > dp[i*W + c]) dp[i*W + c] = t;
            }
        }
    int best = dp[n*W + cap];
    if (pick) {
        int c = cap;
        for (int i = n; i >= 1; i--) {
            if (dp[i*W + c] != dp[(i-1)*W + c]) { pick[i-1] = 1; c -= w[i-1]; } else pick[i-1] = 0;
        }
    }
    free(dp);
    return best;
}

static void mc_paren(int s[21][21], int i, int j, char *buf) {
    if (i == j) { char t[16]; snprintf(t, sizeof t, "A%d", i); strcat(buf, t); return; }
    strcat(buf, "(");
    mc_paren(s, i, s[i][j], buf);
    mc_paren(s, s[i][j] + 1, j, buf);
    strcat(buf, ")");
}
int matrix_chain(const int *p, int n, char *paren) {      /* n matrices (<=20), p has n+1 dims */
    int m[21][21] = {{0}}, s[21][21] = {{0}};
    if (n > 20) n = 20;
    for (int len = 2; len <= n; len++)
        for (int i = 1; i + len - 1 <= n; i++) {
            int j = i + len - 1; m[i][j] = INF;
            for (int k = i; k < j; k++) {
                OP();
                int q = m[i][k] + m[k+1][j] + p[i-1] * p[k] * p[j];
                if (q < m[i][j]) { m[i][j] = q; s[i][j] = k; }
            }
        }
    if (paren) { paren[0] = 0; mc_paren(s, 1, n, paren); }
    return m[1][n];
}

int lcs(const char *a, const char *b, char *out) {
    int n = (int)strlen(a), m = (int)strlen(b), W = m + 1;
    int *L = talloc((size_t)(n + 1) * W * sizeof(int));
    for (int i = 1; i <= n; i++)
        for (int j = 1; j <= m; j++) {
            OP();
            if (a[i-1] == b[j-1]) L[i*W+j] = L[(i-1)*W+j-1] + 1;
            else L[i*W+j] = L[(i-1)*W+j] > L[i*W+j-1] ? L[(i-1)*W+j] : L[i*W+j-1];
        }
    int len = L[n*W + m];
    if (out) {
        int i = n, j = m, k = len;
        out[k] = 0;
        while (i > 0 && j > 0) {
            if (a[i-1] == b[j-1]) { out[--k] = a[i-1]; i--; j--; }
            else if (L[(i-1)*W+j] >= L[i*W+j-1]) i--; else j--;
        }
    }
    free(L);
    return len;
}

int optimal_bst(const int *f, int n) {                    /* successful-search frequencies, sorted keys */
    if (n <= 0) return 0;
    if (n > 30) n = 30;
    int c[32][32] = {{0}}, pre[32] = {0};
    for (int i = 1; i <= n; i++) { pre[i] = pre[i-1] + f[i-1]; c[i][i] = f[i-1]; }
    for (int len = 2; len <= n; len++)
        for (int i = 1; i + len - 1 <= n; i++) {
            int j = i + len - 1; c[i][j] = INF;
            for (int r = i; r <= j; r++) {
                OP();
                int left = (r > i) ? c[i][r-1] : 0, right = (r < j) ? c[r+1][j] : 0;
                int t = left + right + pre[j] - pre[i-1];
                if (t < c[i][j]) c[i][j] = t;
            }
        }
    return c[1][n];
}

int bellman_ford(const Graph *g, int s, int *dist) {      /* directed arcs; returns 0 on negative cycle */
    int n = g->n;
    for (int i = 0; i < n; i++) dist[i] = INF;
    dist[s] = 0;
    for (int it = 1; it < n; it++) {
        int ch = 0;
        for (int u = 0; u < n; u++) for (int v = 0; v < n; v++) {
            if (u == v || g->w[u][v] >= INF) continue;
            OP();
            if (dist[u] < INF && dist[u] + g->w[u][v] < dist[v]) { dist[v] = dist[u] + g->w[u][v]; ch = 1; }
        }
        if (!ch) break;
    }
    for (int u = 0; u < n; u++) for (int v = 0; v < n; v++)
        if (u != v && g->w[u][v] < INF && dist[u] < INF && dist[u] + g->w[u][v] < dist[v]) return 0;
    return 1;
}

void floyd_warshall(const Graph *g, int d[MAXN][MAXN]) {
    int n = g->n;
    for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) d[i][j] = g->w[i][j];
    for (int k = 0; k < n; k++) for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) {
        OP();
        if (d[i][k] < INF && d[k][j] < INF && d[i][k] + d[k][j] < d[i][j]) d[i][j] = d[i][k] + d[k][j];
    }
}

int tsp_dp(const Graph *g, int n) {                       /* Held-Karp, small n (<=13), tour from node 0 */
    if (n > 13) n = 13;
    if (n < 2) return 0;
    int S = 1 << n;
    int *dp = talloc((size_t)S * n * sizeof(int));
    for (int i = 0; i < S * n; i++) dp[i] = INF;
    dp[1 * n + 0] = 0;
    for (int mask = 1; mask < S; mask++) {
        if (!(mask & 1)) continue;
        for (int last = 0; last < n; last++) {
            if (!((mask >> last) & 1) || dp[mask*n + last] >= INF) continue;
            for (int nx = 0; nx < n; nx++) {
                if ((mask >> nx) & 1 || g->w[last][nx] >= INF) continue;
                OP();
                int nm = mask | (1 << nx), t = dp[mask*n + last] + g->w[last][nx];
                if (t < dp[nm*n + nx]) dp[nm*n + nx] = t;
            }
        }
    }
    int best = INF;
    for (int last = 1; last < n; last++)
        if (dp[(S-1)*n + last] < INF && g->w[last][0] < INF && dp[(S-1)*n + last] + g->w[last][0] < best)
            best = dp[(S-1)*n + last] + g->w[last][0];
    free(dp);
    return best;
}
