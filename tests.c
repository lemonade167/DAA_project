/* Automated unit, integration, edge-case and oracle tests. Run: make test */
#include "parking.h"

static int passed = 0, failed = 0;
#define CHECK(c, msg) do { if (c) passed++; else { failed++; printf("FAIL: %s (line %d)\n", msg, __LINE__); } } while (0)

static int is_sorted(const int *a, int n) { for (int i = 1; i < n; i++) if (a[i-1] > a[i]) return 0; return 1; }

static void test_dc(void) {
    int a[500], b[500], c[500];
    for (int t = 0; t < 20; t++) {
        int n = 1 + rnd(500);
        for (int i = 0; i < n; i++) a[i] = b[i] = rnd(1000) - 500;
        merge_sort(a, n); quick_sort(b, 0, n - 1);
        CHECK(is_sorted(a, n) && is_sorted(b, n) && !memcmp(a, b, n * sizeof(int)), "sorts agree");
        int key = a[rnd(n)];
        int i1 = bsearch_iter(a, n, key), i2 = bsearch_rec(a, 0, n - 1, key);
        CHECK(i1 >= 0 && a[i1] == key && i2 >= 0 && a[i2] == key, "binary search finds key");
        CHECK(bsearch_iter(a, n, 100000) == -1 && bsearch_rec(a, 0, n - 1, 100000) == -1, "binary search miss");
    }
    for (int i = 0; i < 100; i++) c[i] = i;
    quick_sort(c, 0, 99); CHECK(is_sorted(c, 100), "quick sort adversarial (already sorted)");
    int x[] = {-2, 1, -3, 4, -1, 2, 1, -5, 4}, l, r;
    CHECK(max_subarray(x, 0, 8, &l, &r) == 6 && l == 3 && r == 6, "max subarray classic");
    int neg[] = {-5, -2, -9};
    CHECK(max_subarray(neg, 0, 2, &l, &r) == -2, "max subarray all negative");
    CHECK(fast_pow(2, 10, 1000) == 24 && fast_pow(5, 0, 7) == 1 && fast_pow(7, 1, 13) == 7, "fast pow");
    long slow = 1; for (int i = 0; i < 200; i++) slow = slow * 3 % 13;
    CHECK(fast_pow(3, 200, 13) == slow, "fast pow vs loop");
    int k[CEL_N][CEL_N] = {{0}};
    for (int i = 0; i < 6; i++) if (i != 3) k[i][3] = 1;
    CHECK(find_celebrity(6, k) == 3, "celebrity found");
    k[3][0] = 1; CHECK(find_celebrity(6, k) == -1, "no celebrity when candidate knows someone");
    long A[64], B[64], C1[64], C2[64];
    for (int i = 0; i < 64; i++) { A[i] = rnd(10) - 5; B[i] = rnd(10) - 5; }
    mat_classic(A, B, C1, 8); mat_strassen(A, B, C2, 8);
    CHECK(!memcmp(C1, C2, sizeof C1), "strassen equals classical");
}

static void test_greedy(void) {
    Reservation r[6] = {{1,"A",0,3,0,0},{2,"B",2,5,0,0},{3,"C",4,7,0,0},{4,"D",1,4,0,0},{5,"E",5,8,0,0},{6,"F",8,10,0,0}};
    int ch[6], as[6];
    Reservation t1[6]; memcpy(t1, r, sizeof r);
    CHECK(activity_selection(t1, 6, ch) == 3, "activity selection");
    memcpy(t1, r, sizeof r);
    int need = interval_partition(t1, 6, as);
    int depth = 0;
    for (int h = 0; h < 24; h++) { int d = 0; for (int i = 0; i < 6; i++) if (r[i].start <= h && h < r[i].end) d++; if (d > depth) depth = d; }
    CHECK(need == depth, "interval partitioning equals max overlap depth");
    int w[] = {10, 20, 30}, v[] = {60, 100, 120}; double fr[3];
    CHECK((int)fractional_knapsack(w, v, 3, 50, fr) == 240, "fractional knapsack");
    char codes[256][64]; long bits;
    CHECK(huffman("aaabbc", codes, &bits) == 3 && bits == 9, "huffman bits");
    CHECK(huffman("zzzz", codes, &bits) == 1 && bits == 4, "huffman single symbol");
    CHECK(huffman("", codes, &bits) == 0, "huffman empty");
    Graph g;
    for (int trial = 0; trial < 10; trial++) {
        int n = 10 + rnd(15); graph_init(&g, n);
        for (int i = 1; i < n; i++) graph_add(&g, i, rnd(i), 1 + rnd(20));
        for (int e = 0; e < n * 2; e++) { int a = rnd(n), b = rnd(n); if (a != b) graph_add(&g, a, b, 1 + rnd(20)); }
        int d1[MAXN], d2[MAXN], d3[MAXN][MAXN], pv[MAXN], par[MAXN], ok = 1;
        dijkstra(&g, 0, d1, pv); bellman_ford(&g, 0, d2); floyd_warshall(&g, d3);
        for (int i = 0; i < n; i++) if (d1[i] != d2[i] || d1[i] != d3[0][i]) ok = 0;
        CHECK(ok, "dijkstra == bellman-ford == floyd-warshall");
        Edge e[MAXN]; int cnt;
        CHECK(prim(&g, par) == kruskal(&g, e, &cnt) && cnt == n - 1, "prim == kruskal");
    }
    graph_init(&g, 4); graph_add(&g, 0, 1, 1);
    int par[MAXN], d[MAXN], pv[MAXN];
    CHECK(prim(&g, par) == -1, "prim detects disconnected graph");
    dijkstra(&g, 0, d, pv); CHECK(d[3] == INF, "dijkstra unreachable stays INF");
}

static void test_dp_bt(void) {
    int w[] = {1, 3, 4, 5}, v[] = {1, 4, 5, 7}, pick[4];
    CHECK(knapsack_dp(w, v, 4, 7, pick) == 9, "knapsack dp classic");
    CHECK(knapsack_dp(w, v, 4, 0, pick) == 0, "knapsack zero capacity");
    for (int t = 0; t < 8; t++) {
        int n = 10 + rnd(8), ww[32], vv[32], cap = 20 + rnd(40);
        for (int i = 0; i < n; i++) { ww[i] = 1 + rnd(15); vv[i] = 1 + rnd(50); }
        int a = knapsack_dp(ww, vv, n, cap, NULL), b = knapsack_bt(ww, vv, n, cap);
        long nd;
        CHECK(a == b && a == knapsack_bb(ww, vv, n, cap, 0, &nd) && a == knapsack_bb(ww, vv, n, cap, 1, &nd)
              && a == knapsack_bb(ww, vv, n, cap, 2, &nd), "knapsack dp == backtracking == B&B (FIFO/LIFO/LC)");
    }
    int p[] = {10, 30, 5, 60}; char s[256];
    CHECK(matrix_chain(p, 3, s) == 4500 && !strcmp(s, "((A1A2)A3)"), "matrix chain");
    char out[32];
    CHECK(lcs("ABCBDAB", "BDCABA", out) == 4 && strlen(out) == 4, "lcs length");
    CHECK(lcs("", "abc", out) == 0, "lcs empty");
    int f[] = {34, 8, 50};
    CHECK(optimal_bst(f, 3) == 142, "optimal bst");
    Graph g; graph_init(&g, 4);
    g.w[0][1] = 10; g.w[0][2] = 15; g.w[0][3] = 20; g.w[1][0] = 10; g.w[2][0] = 15; g.w[3][0] = 20;
    g.w[1][2] = g.w[2][1] = 35; g.w[1][3] = g.w[3][1] = 25; g.w[2][3] = g.w[3][2] = 30;
    CHECK(tsp_dp(&g, 4) == 80, "tsp held-karp");
    Graph ng; graph_init(&ng, 4); ng.w[0][1] = 4; ng.w[0][2] = 5; ng.w[2][1] = -3; ng.w[1][3] = 2;
    int d[MAXN];
    CHECK(bellman_ford(&ng, 0, d) && d[1] == 2 && d[3] == 4, "bellman-ford negative edge");
    ng.w[1][2] = -3;
    CHECK(!bellman_ford(&ng, 0, d), "bellman-ford negative cycle");
    CHECK(nqueens(8, NULL) == 92 && nqueens(6, NULL) == 4 && nqueens(1, NULL) == 1 && nqueens(3, NULL) == 0, "n-queens counts");
    int first[16]; nqueens(5, first);
    int ok = 1; for (int i = 0; i < 5; i++) for (int j = i + 1; j < 5; j++) if (first[i] == first[j] || abs(first[i]-first[j]) == j - i) ok = 0;
    CHECK(ok, "n-queens first solution valid");
    int a[] = {3, 34, 4, 12, 5, 2}, pk[6], sum = 0;
    CHECK(subset_sum(a, 6, 9, pk), "subset sum exists");
    for (int i = 0; i < 6; i++) if (pk[i]) sum += a[i];
    CHECK(sum == 9, "subset sum picks add up");
    CHECK(!subset_sum(a, 6, 100, pk), "subset sum impossible");
}

static void test_flow_str(void) {
    int cap[MAXN][MAXN] = {{0}};
    cap[0][1] = 16; cap[0][2] = 13; cap[1][2] = 10; cap[2][1] = 4; cap[1][3] = 12; cap[3][2] = 9;
    cap[2][4] = 14; cap[4][3] = 7; cap[3][5] = 20; cap[4][5] = 4;
    CHECK(max_flow(6, cap, 0, 5, NULL) == 23, "max flow CLRS example");
    int adj[MAX_DRV][MAX_SPACES] = {{0}}, m[MAX_DRV];
    for (int i = 0; i < 3; i++) adj[i][i] = 1;
    CHECK(bipartite_match(3, 3, adj, m) == 3 && m[0] == 0 && m[2] == 2, "perfect matching");
    for (int i = 0; i < 4; i++) { adj[i][0] = 1; adj[i][1] = 1; }
    CHECK(bipartite_match(4, 2, adj, m) == 2, "matching limited by spaces");
    int rem[] = {2, 3, 2}, mod[] = {3, 5, 7}; long x, M;
    CHECK(crt(rem, mod, 3, &x, &M) && x == 23 && M == 105, "crt");
    int bad[] = {4, 6}; CHECK(!crt(rem, bad, 2, &x, &M), "crt rejects non-coprime moduli");
    const char *t = "abababab"; int pos[16];
    int c1 = s_naive(t, "abab", pos, 16), c2 = s_rabin_karp(t, "abab", pos, 16), c3 = s_automaton(t, "abab", pos, 16), c4 = s_kmp(t, "abab", pos, 16);
    CHECK(c1 == 3 && c2 == 3 && c3 == 3 && c4 == 3 && pos[0] == 0, "string matchers overlapping matches");
    CHECK(s_naive("abc", "abcd", pos, 16) == 0 && s_kmp("abc", "", pos, 16) == 0, "pattern longer / empty");
    char txt[2000];
    for (int i = 0; i < 1999; i++) txt[i] = "ACGT"[rnd(4)];
    txt[1999] = 0;
    int ok = 1;
    for (int k = 0; k < 20; k++) {
        char p[8]; int L = 2 + rnd(5); for (int i = 0; i < L; i++) p[i] = "ACGT"[rnd(4)]; p[L] = 0;
        int a = s_naive(txt, p, pos, 16), b = s_rabin_karp(txt, p, pos, 16), c = s_automaton(txt, p, pos, 16), d = s_kmp(txt, p, pos, 16);
        if (a != b || a != c || a != d) ok = 0;
    }
    CHECK(ok, "four matchers agree on random DNA text");
}

static void test_lot(void) {
    Lot L; lot_init(&L, 12);
    time_t t0 = 1000000;
    int s1 = lot_park(&L, "AAA", t0, 10), s2 = lot_park(&L, "BBB", t0, 10);
    CHECK(s1 >= 0 && s2 >= 0 && s1 != s2, "park two vehicles");
    CHECK(L.sp[s1].dist <= L.sp[s2].dist, "nearest space allocated first");
    CHECK(lot_park(&L, "AAA", t0, 10) == -2, "duplicate vehicle rejected");
    int sp; long h;
    CHECK(lot_exit(&L, "AAA", t0 + 100, &sp, &h) == L.sp[s1].price && h == 1, "minimum one-hour fee");
    CHECK(lot_exit(&L, "BBB", t0 + 3601, &sp, &h) == 2L * L.sp[s2].price && h == 2, "hour rounding up");
    CHECK(lot_exit(&L, "ZZZ", t0, &sp, &h) == -1, "unknown vehicle exit");
    int r1 = lot_reserve(&L, "RES1", 9, 12, -1), r2 = lot_reserve(&L, "RES2", 10, 13, -1);
    CHECK(r1 >= 0 && r2 >= 0 && r1 != r2, "overlapping reservations get different spaces");
    CHECK(lot_reserve(&L, "BAD", 12, 12, -1) == -1 && lot_reserve(&L, "BAD", -1, 5, -1) == -1 && lot_reserve(&L, "BAD", 5, 25, -1) == -1, "invalid reservation times");
    CHECK(lot_park(&L, "RES1", t0, 10) == r1, "reserved space honoured for owner");
    int other = lot_park(&L, "OTHER", t0, 10);
    CHECK(other != r1 && other != r2, "reserved spaces protected from others");
    lot_save(&L, "/tmp/test_lot.csv");
    Lot M; lot_init(&M, 12); lot_load(&M, "/tmp/test_lot.csv");
    CHECK(M.nres == L.nres && M.sp[r1].occupied && M.revenue == L.revenue, "persistence round trip");
    Lot B; lot_init(&B, 12);
    int busy = lot_park(&B, "NOW", t0, 10);
    CHECK(lot_reserve(&B, "LATER", 9, 12, 10) != busy, "reservation avoids space occupied right now");
    Lot F; lot_init(&F, 2); lot_park(&F, "A", t0, 0); lot_park(&F, "B", t0, 0);
    CHECK(lot_park(&F, "C", t0, 0) == -1, "full lot");
}

int main(void) {
    rnd_seed(42);
    test_dc(); test_greedy(); test_dp_bt(); test_flow_str(); test_lot();
    printf("Tests passed: %d, failed: %d\n", passed, failed);
    return failed ? 1 : 0;
}
