/*
 * Smart Parking Allocation and Reservation Platform  (DAA Project 46)
 * Menu-driven CLI + syllabus workbench + benchmark runner.
 */
#include "parking.h"
#include <ctype.h>

#define DATA_FILE "parking_lot.csv"
#define LOT_SIZE  12

/* ---------- input helpers ---------- */
static void read_line(const char *prompt, char *buf, size_t n) {
    printf("%s", prompt);
    if (!fgets(buf, (int)n, stdin)) { buf[0] = 0; return; }
    buf[strcspn(buf, "\r\n")] = 0;
}
static int read_int(const char *prompt, int def) {
    char b[32]; read_line(prompt, b, sizeof b);
    return b[0] ? atoi(b) : def;
}
static void read_plate(char *out) {
    char b[64]; read_line("Vehicle number plate: ", b, sizeof b);
    int k = 0;
    for (int i = 0; b[i] && k < PLATE_LEN - 1; i++) if (!isspace((unsigned char)b[i]) && b[i] != ',') out[k++] = (char)toupper((unsigned char)b[i]);
    out[k] = 0;
}
static int current_hour(time_t t) { return localtime(&t)->tm_hour; }
static void hdr(const char *s) { printf("\n=== %s ===\n", s); }

/* ---------- operational menu actions ---------- */
static void act_park(Lot *L) {
    char p[PLATE_LEN]; read_plate(p);
    if (!p[0]) { printf("Invalid plate.\n"); return; }
    time_t now = time(NULL);
    int s = lot_park(L, p, now, current_hour(now));
    if (s == -2) printf("This vehicle is already parked.\n");
    else if (s < 0) printf("Parking is full (or remaining spaces are reserved).\n");
    else {
        int dist[MAXN], prev[MAXN], path[MAXN], len = 0;
        dijkstra(&L->road, 0, dist, prev);
        for (int v = s + 1; v != -1; v = prev[v]) path[len++] = v;
        printf("Allocated space %d (price Rs.%d/hour, %d m from gate).\nRoute: GATE", L->sp[s].id, L->sp[s].price, L->sp[s].dist);
        for (int i = len - 2; i >= 0; i--) printf(" -> S%d", path[i]);
        printf("\n");
    }
    lot_save(L, DATA_FILE);
}
static void act_exit(Lot *L) {
    char p[PLATE_LEN]; read_plate(p);
    int s; long h;
    long fee = lot_exit(L, p, time(NULL), &s, &h);
    if (fee < 0) printf("Vehicle not found.\n");
    else printf("Space %d freed. Billed %ld hour(s). Amount due: Rs. %ld\n", s + 1, h, fee);
    lot_save(L, DATA_FILE);
}
static void act_reserve(Lot *L) {
    char p[PLATE_LEN]; read_plate(p);
    int st = read_int("Start hour (0-23): ", -1), en = read_int("End hour (1-24): ", -1);
    int s = lot_reserve(L, p, st, en, current_hour(time(NULL)));
    if (s < 0) printf("Could not reserve (invalid time or no space free in that window).\n");
    else printf("Reserved space %d from %02d:00 to %02d:00.\n", L->sp[s].id, st, en);
    lot_save(L, DATA_FILE);
}

/* ---------- syllabus workbench: every module on parking data ---------- */
static const int SR[8][3] = {{0,3,30},{2,5,45},{4,7,40},{6,9,50},{8,11,35},{1,4,25},{5,8,60},{9,12,30}};

static void load_samples(Reservation *r) {
    for (int i = 0; i < 8; i++) { r[i].id = i + 1; snprintf(r[i].plate, PLATE_LEN, "CAR%d", i + 1); r[i].start = SR[i][0]; r[i].end = SR[i][1]; r[i].value = SR[i][2]; r[i].space = 0; }
}

static void wb_dc(void) {
    hdr("Divide and conquer");
    int a[16], b[16], l, r;
    rnd_seed(7);
    for (int i = 0; i < 16; i++) a[i] = b[i] = 5 + rnd(95);
    metrics_reset(); merge_sort(a, 16); long m1 = g_ops;
    metrics_reset(); quick_sort(b, 0, 15); long m2 = g_ops;
    printf("Space distances sorted (merge: %ld cmp, quick: %ld cmp): ", m1, m2);
    for (int i = 0; i < 16; i++) printf("%d ", a[i]);
    int key = a[9];
    metrics_reset(); int i1 = bsearch_iter(a, 16, key); long o1 = g_ops;
    metrics_reset(); int i2 = bsearch_rec(a, 0, 15, key); long o2 = g_ops;
    printf("\nBinary search for %d m: iterative idx %d (%ld steps), recursive idx %d (%ld steps)\n", key, i1, o1, i2, o2);
    int net[24] = {-1,-1,0,0,1,2,6,12,9,4,-2,1,3,2,-3,-1,5,9,11,4,-4,-8,-6,-3};
    long best = max_subarray(net, 0, 23, &l, &r);
    printf("Busiest stretch of net arrivals: hours %d-%d, +%ld vehicles\n", l, r + 1, best);
    printf("Fast power: 7^128 mod 1009 = %ld (hash/pricing helper)\n", fast_pow(7, 128, 1009));
    int k[CEL_N][CEL_N] = {{0}};
    for (int i = 0; i < 6; i++) if (i != 2) k[i][2] = 1;
    k[0][1] = 1; k[4][5] = 1;
    int c = find_celebrity(6, k);
    if (c >= 0) printf("Celebrity zone (preferred by all, prefers none): zone %d\n", c);
    else printf("Celebrity zone: none\n");
    long A[16], B[16], C1[16], C2[16];
    for (int i = 0; i < 16; i++) { A[i] = i % 5 + 1; B[i] = (i * 3) % 7; }
    metrics_reset(); mat_classic(A, B, C1, 4); long mc = g_ops;
    metrics_reset(); mat_strassen(A, B, C2, 4); long ms_ = g_ops;
    printf("Zone-transfer 4x4 product: classical %ld mults, Strassen-module %ld mults, results %s\n", mc, ms_, memcmp(C1, C2, sizeof C1) ? "DIFFER" : "identical");
}

static void wb_greedy(Lot *L) {
    hdr("Greedy methods");
    Reservation r[8], t[8]; int ch[8], as[8];
    load_samples(r);
    memcpy(t, r, sizeof r);
    int n = activity_selection(t, 8, ch);
    printf("Activity selection (max bookings for one premium space): %d ->", n);
    for (int i = 0; i < 8; i++) if (ch[i]) printf(" %s[%d-%d)", t[i].plate, t[i].start, t[i].end);
    memcpy(t, r, sizeof r);
    printf("\nInterval partitioning: %d spaces needed for all 8 bookings\n", interval_partition(t, 8, as));
    int w[5] = {30, 20, 50, 10, 40}, v[5] = {90, 80, 100, 40, 60}; double fr[5];
    printf("Fractional knapsack (60 kW charger power shared): best value %.1f\n", fractional_knapsack(w, v, 5, 60, fr));
    char codes[256][64]; long bits;
    const char *log = "ENTRY EXIT ENTRY ENTRY RESERVE ENTRY EXIT ENTRY";
    int syms = huffman(log, codes, &bits);
    printf("Huffman on event log: %d symbols, %ld bits vs %zu bits fixed 8-bit\n", syms, bits, strlen(log) * 8);
    int dist[MAXN], prev[MAXN];
    dijkstra(&L->road, 0, dist, prev);
    int far = 1; for (int i = 1; i <= L->n; i++) if (dist[i] > dist[far]) far = i;
    printf("Dijkstra: farthest space is S%d at %d m\n", far, dist[far]);
    int par[MAXN], cnt; Edge e[MAXN];
    printf("Sensor cabling MST: Prim = %d m, Kruskal = %d m\n", prim(&L->road, par), kruskal(&L->road, e, &cnt));
}

static void wb_dp(Lot *L) {
    hdr("Dynamic programming");
    Reservation r[8]; load_samples(r);
    int w[8], v[8], pick[8];
    for (int i = 0; i < 8; i++) { w[i] = r[i].end - r[i].start; v[i] = r[i].value; }
    printf("0/1 knapsack (10 space-hours available): revenue Rs.%d, bookings:", knapsack_dp(w, v, 8, 10, pick));
    for (int i = 0; i < 8; i++) if (pick[i]) printf(" %s", r[i].plate);
    int dims[5] = {10, 30, 5, 60, 8}; char par[256];
    printf("\nMatrix chain (zone-cost matrices): %d mults, order %s\n", matrix_chain(dims, 4, par), par);
    char out[32];
    printf("LCS of plates KA01AB1234 / KA05AB1243: length %d (%s)\n", lcs("KA01AB1234", "KA05AB1243", out), out);
    int freq[6] = {20, 5, 30, 10, 25, 10};
    printf("Optimal BST for space-lookup frequencies: cost %d\n", optimal_bst(freq, 6));
    int d1[MAXN], d2[MAXN], d3[MAXN][MAXN];
    int ok = bellman_ford(&L->road, 0, d1); floyd_warshall(&L->road, d3);
    int same = 1; for (int i = 0; i <= L->n; i++) if (d1[i] != d3[0][i]) same = 0;
    (void)d2;
    printf("Bellman-Ford (no negative cycle: %s) matches Floyd-Warshall row 0: %s\n", ok ? "yes" : "no", same ? "yes" : "no");
    int k = L->n + 1 < 9 ? L->n + 1 : 9;
    Graph tg; graph_init(&tg, k);
    for (int i = 0; i < k; i++) for (int j = 0; j < k; j++) tg.w[i][j] = d3[i][j];
    printf("TSP patrol tour of gate + %d spaces (Held-Karp): %d m\n", k - 1, tsp_dp(&tg, k));
}

static void wb_bt(void) {
    hdr("Backtracking and branch-and-bound");
    int first[16]; int cnt = nqueens(8, first);
    printf("N-Queens validation: 8x8 has %d solutions; first = ", cnt);
    for (int i = 0; i < 8; i++) printf("%d ", first[i]);
    int dur[8] = {3, 3, 3, 3, 3, 3, 3, 3}, pk[8];
    int a[6] = {2, 3, 5, 7, 11, 13}, p2[6];
    int ok = subset_sum(a, 6, 20, p2);
    printf("\nSum of subsets: can bookings of %s fill exactly 20 space-hours? %s", "{2,3,5,7,11,13}", ok ? "yes:" : "no");
    if (ok) for (int i = 0; i < 6; i++) if (p2[i]) printf(" %d", a[i]);
    (void)dur; (void)pk;
    int w[10] = {5, 7, 3, 9, 4, 6, 8, 2, 10, 5}, v[10] = {40, 42, 12, 50, 25, 35, 38, 10, 60, 30};
    long nd[3];
    int bt = knapsack_bt(w, v, 10, 30);
    int b0 = knapsack_bb(w, v, 10, 30, 0, &nd[0]), b1 = knapsack_bb(w, v, 10, 30, 1, &nd[1]), b2 = knapsack_bb(w, v, 10, 30, 2, &nd[2]);
    printf("\nKnapsack by backtracking = %d; branch-and-bound FIFO = %d (%ld nodes), LIFO = %d (%ld nodes), least-cost = %d (%ld nodes)\n",
           bt, b0, nd[0], b1, nd[1], b2, nd[2]);
}

static void wb_flow_num_str(void) {
    hdr("Flow, matching, number theory, string matching");
    int cap[MAXN][MAXN] = {{0}};
    cap[0][1] = 10; cap[0][2] = 8; cap[1][3] = 6; cap[1][4] = 5; cap[2][3] = 4; cap[2][4] = 7; cap[3][5] = 9; cap[4][5] = 8;
    printf("Gate-to-lot lane capacity (Ford-Fulkerson): max %d vehicles/min\n", max_flow(6, cap, 0, 5, NULL));
    int adj[MAX_DRV][MAX_SPACES] = {{0}}, m[MAX_DRV];
    for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++) adj[i][j] = (i + j) % 3 != 0 || i == j;
    int f = bipartite_match(6, 6, adj, m);
    printf("Driver-to-space matching: %d of 6 drivers matched:", f);
    for (int i = 0; i < 6; i++) if (m[i] >= 0) printf(" D%d->S%d", i + 1, m[i] + 1);
    int rem[3] = {2, 3, 2}, mod[3] = {3, 5, 7}; long x, M;
    crt(rem, mod, 3, &x, &M);
    printf("\nCRT: ticket counters (2 mod 3, 3 mod 5, 2 mod 7) reconstruct ticket no. %ld (mod %ld)\n", x, M);
    const char *text = "KA01AB1234 IN;MH12XY9 IN;KA01AB1234 OUT;KA01AB1234 IN;DL08CD5 IN";
    const char *pat = "KA01AB1234";
    int pos[16];
    int (*fn[4])(const char *, const char *, int *, int) = {s_naive, s_rabin_karp, s_automaton, s_kmp};
    const char *nm[4] = {"naive", "Rabin-Karp", "automaton", "KMP"};
    for (int i = 0; i < 4; i++) {
        metrics_reset(); int c = fn[i](text, pat, pos, 16);
        printf("  %-10s found %d occurrences of %s, %ld operations\n", nm[i], c, pat, g_ops);
    }
}

static void wb_complexity(void) {
    hdr("Complexity classes and reduction");
    printf("P            : single-space booking maximisation (greedy), min spaces needed (partitioning),\n"
           "               shortest route, cabling MST, driver-space matching and lane capacity (flow).\n"
           "NP-complete  : 'can booked revenue >= K within C space-hours?' (0/1 knapsack decision, weakly NP-complete),\n"
           "               'do durations sum exactly to C?' (subset sum), 'is there a patrol tour <= K?' (TSP decision).\n"
           "NP-hard      : the optimisation versions (max revenue selection, shortest patrol tour).\n"
           "Reduction    : SUBSET-SUM <=p 0/1-KNAPSACK. Given numbers a_i and target T, make item i with weight = value = a_i,\n"
           "               capacity T, ask for value >= T. A subset sums to T iff the knapsack reaches value T.\n"
           "               The map is O(n), so knapsack is at least as hard as subset sum.\n");
}

static void workbench(Lot *L) {
    wb_dc(); wb_greedy(L); wb_dp(L); wb_bt(); wb_flow_num_str(); wb_complexity();
}

/* ---------- benchmark runner ---------- */
static void bench_sort(void) {
    hdr("Benchmark: sorting (ms, comparisons)");
    printf("%-8s %-18s %-18s %-22s\n", "n", "merge (random)", "quick (random)", "quick (sorted, worst)");
    int sizes[3] = {2000, 20000, 100000};
    for (int s = 0; s < 3; s++) {
        int n = sizes[s];
        int *a = talloc((size_t)n * sizeof(int)), *b = talloc((size_t)n * sizeof(int));
        rnd_seed(1);
        for (int i = 0; i < n; i++) a[i] = b[i] = rnd(1000000);
        metrics_reset(); double t0 = now_ms(); merge_sort(a, n); double tm = now_ms() - t0; long om = g_ops;
        metrics_reset(); t0 = now_ms(); quick_sort(b, 0, n - 1); double tq = now_ms() - t0; long oq = g_ops;
        int wn = n > 6000 ? 6000 : n;                       /* worst case O(n^2): keep recursion depth modest */
        for (int i = 0; i < wn; i++) b[i] = i;
        metrics_reset(); t0 = now_ms(); quick_sort(b, 0, wn - 1); double tw = now_ms() - t0; long ow = g_ops;
        printf("%-8d %7.2f ms %-9ld %7.2f ms %-9ld n=%d: %6.2f ms %ld\n", n, tm, om, tq, oq, wn, tw, ow);
        free(a); free(b);
    }
}
static void bench_paths(void) {
    hdr("Benchmark: shortest paths (operations)");
    printf("%-6s %-12s %-14s %-16s\n", "nodes", "Dijkstra", "Bellman-Ford", "Floyd-Warshall (all pairs)");
    for (int n = 16; n <= 64; n *= 2) {
        Graph g; graph_init(&g, n); rnd_seed(3);
        for (int i = 1; i < n; i++) graph_add(&g, i, rnd(i), 1 + rnd(20));
        for (int e = 0; e < n * 3; e++) { int a = rnd(n), b = rnd(n); if (a != b) graph_add(&g, a, b, 1 + rnd(20)); }
        int d[MAXN], p[MAXN], fw[MAXN][MAXN];
        metrics_reset(); dijkstra(&g, 0, d, p); long o1 = g_ops;
        metrics_reset(); bellman_ford(&g, 0, d);  long o2 = g_ops;
        metrics_reset(); floyd_warshall(&g, fw);  long o3 = g_ops;
        printf("%-6d %-12ld %-14ld %-16ld\n", n, o1, o2, o3);
    }
}
static void bench_knap(void) {
    hdr("Benchmark: 0/1 knapsack, four strategies (value, ops/nodes, ms)");
    printf("%-4s %-6s %-14s %-14s %-16s %-16s %-16s\n", "n", "best", "DP ops", "backtrack ops", "B&B FIFO nodes", "B&B LIFO nodes", "B&B LC nodes");
    for (int n = 12; n <= 24; n += 4) {
        int w[64], v[64], cap = 0;
        rnd_seed(9 + n);
        for (int i = 0; i < n; i++) { w[i] = 3 + rnd(20); v[i] = 5 + rnd(60); cap += w[i]; }
        cap /= 3;
        long nd[3];
        metrics_reset(); int a = knapsack_dp(w, v, n, cap, NULL); long o1 = g_ops;
        metrics_reset(); int b = knapsack_bt(w, v, n, cap); long o2 = g_ops;
        int c0 = knapsack_bb(w, v, n, cap, 0, &nd[0]), c1 = knapsack_bb(w, v, n, cap, 1, &nd[1]), c2 = knapsack_bb(w, v, n, cap, 2, &nd[2]);
        printf("%-4d %-6d %-14ld %-14ld %-16ld %-16ld %-16ld %s\n", n, a, o1, o2, nd[0], nd[1], nd[2], (a == b && a == c0 && a == c1 && a == c2) ? "" : "MISMATCH");
    }
}
static void bench_str(void) {
    hdr("Benchmark: string matching on event stream (comparisons/ops)");
    printf("%-9s %-10s %-12s %-12s %-12s %-12s\n", "text len", "pattern", "naive", "Rabin-Karp", "automaton", "KMP");
    int lens[3] = {10000, 100000, 500000};
    for (int s = 0; s < 3; s++) {
        int n = lens[s]; char *t = talloc((size_t)n + 1);
        rnd_seed(5);
        for (int i = 0; i < n; i++) t[i] = "ACGT"[rnd(4)];
        for (int adv = 0; adv < 2; adv++) {
            const char *pat = adv ? "AAAAAAAAB" : "ACGTTGCAAC";
            if (adv) for (int i = 0; i < n; i++) t[i] = (i % 1000 == 999) ? 'B' : 'A';   /* adversarial for naive */
            long o[4]; int pos[4];
            int (*fn[4])(const char *, const char *, int *, int) = {s_naive, s_rabin_karp, s_automaton, s_kmp};
            for (int k = 0; k < 4; k++) { metrics_reset(); fn[k](t, pat, pos, 4); o[k] = g_ops; }
            printf("%-9d %-10s %-12ld %-12ld %-12ld %-12ld\n", n, adv ? "adversarial" : "random", o[0], o[1], o[2], o[3]);
        }
        free(t);
    }
}
static void bench_matrix(void) {
    hdr("Benchmark: classical vs Strassen matrix multiplication (scalar mults, ms)");
    printf("%-6s %-14s %-12s %-14s %-12s\n", "n", "classical", "ms", "Strassen", "ms");
    for (int n = 16; n <= 128; n *= 2) {
        long *A = talloc((size_t)n * n * sizeof(long)), *B = talloc((size_t)n * n * sizeof(long));
        long *C1 = talloc((size_t)n * n * sizeof(long)), *C2 = talloc((size_t)n * n * sizeof(long));
        for (int i = 0; i < n * n; i++) { A[i] = rnd(10); B[i] = rnd(10); }
        metrics_reset(); double t0 = now_ms(); mat_classic(A, B, C1, n); double t1 = now_ms() - t0; long o1 = g_ops;
        metrics_reset(); t0 = now_ms(); mat_strassen(A, B, C2, n); double t2 = now_ms() - t0; long o2 = g_ops;
        printf("%-6d %-14ld %-12.2f %-14ld %-12.2f %s\n", n, o1, t1, o2, t2, memcmp(C1, C2, (size_t)n * n * sizeof(long)) ? "MISMATCH" : "");
        free(A); free(B); free(C1); free(C2);
    }
}
static void benchmarks(void) { bench_sort(); bench_paths(); bench_knap(); bench_str(); bench_matrix(); }

/* ---------- main menu ---------- */
int main(int argc, char **argv) {
    Lot L; lot_init(&L, LOT_SIZE); lot_load(&L, DATA_FILE);
    if (argc > 1 && !strcmp(argv[1], "--workbench")) { workbench(&L); return 0; }
    if (argc > 1 && !strcmp(argv[1], "--bench"))     { benchmarks(); return 0; }
    int ch;
    do {
        printf("\n====== SMART PARKING ALLOCATION AND RESERVATION PLATFORM ======\n"
               "1. Park a vehicle (nearest free space + route)\n2. Exit a vehicle and bill\n3. Reserve a space (time window)\n"
               "4. Show parking status\n5. Show reservations\n6. Run syllabus workbench (all algorithms)\n"
               "7. Run benchmarks\n0. Exit\n");
        ch = read_int("Choice: ", -1);
        switch (ch) {
            case 1: act_park(&L); break;
            case 2: act_exit(&L); break;
            case 3: act_reserve(&L); break;
            case 4: lot_show(&L); break;
            case 5: lot_show_res(&L); break;
            case 6: workbench(&L); break;
            case 7: benchmarks(); break;
            case 0: lot_save(&L, DATA_FILE); printf("Saved. Goodbye!\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (ch != 0);
    return 0;
}
