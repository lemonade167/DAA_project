/* Smart Parking Allocation and Reservation Platform - shared header (C17) */
#ifndef PARKING_H
#define PARKING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INF         1000000000
#define MAXN        64      /* max graph nodes            */
#define MAX_SPACES  32
#define MAX_DRV     24      /* max drivers in matching    */
#define MAX_RES     128
#define PLATE_LEN   16
#define CEL_N       8

/* ---------- analysis framework (operation counts, time, memory) ---------- */
extern long   g_ops;                 /* basic-operation counter */
extern size_t g_mem;                 /* bytes allocated by algorithms */
#define OP() (g_ops++)
void   metrics_reset(void);
void  *talloc(size_t n);
double now_ms(void);
void   rnd_seed(unsigned s);
int    rnd(int m);                   /* deterministic 0..m-1 */

/* ---------- domain model ---------- */
typedef struct { int u, v, w; } Edge;
typedef struct { int n; int w[MAXN][MAXN]; } Graph;      /* INF = no edge */
typedef struct { int id; char plate[PLATE_LEN]; int start, end, value, space; } Reservation;
typedef struct { int id, dist, price, occupied; char plate[PLATE_LEN]; time_t entry; } Space;
typedef struct {
    Space sp[MAX_SPACES]; int n;
    Graph road;                      /* node 0 = gate, node i = space i-1 */
    Reservation res[MAX_RES]; int nres, nextRes;
    long revenue;
} Lot;

void graph_init(Graph *g, int n);
void graph_add(Graph *g, int u, int v, int w);           /* undirected */

/* ---------- divide and conquer ---------- */
void merge_sort(int *a, int n);
void quick_sort(int *a, int lo, int hi);
int  bsearch_iter(const int *a, int n, int key);
int  bsearch_rec(const int *a, int lo, int hi, int key);
long max_subarray(const int *a, int lo, int hi, int *l, int *r);
long fast_pow(long b, long e, long m);
int  find_celebrity(int n, int k[CEL_N][CEL_N]);
void mat_classic(const long *A, const long *B, long *C, int n);
void mat_strassen(const long *A, const long *B, long *C, int n);   /* n = power of 2 */

/* ---------- greedy ---------- */
int    activity_selection(Reservation *r, int n, int *chosen);
int    interval_partition(Reservation *r, int n, int *assign);
double fractional_knapsack(const int *w, const int *v, int n, double cap, double *frac);
int    huffman(const char *text, char codes[256][64], long *bits);
void   dijkstra(const Graph *g, int src, int *dist, int *prev);
int    prim(const Graph *g, int *parent);
int    kruskal(const Graph *g, Edge *out, int *cnt);

/* ---------- dynamic programming ---------- */
int  knapsack_dp(const int *w, const int *v, int n, int cap, int *pick);
int  matrix_chain(const int *p, int n, char *paren);
int  lcs(const char *a, const char *b, char *out);
int  optimal_bst(const int *freq, int n);
int  bellman_ford(const Graph *g, int src, int *dist);
void floyd_warshall(const Graph *g, int d[MAXN][MAXN]);
int  tsp_dp(const Graph *g, int n);

/* ---------- backtracking and branch & bound ---------- */
int nqueens(int n, int *first);
int subset_sum(const int *a, int n, int target, int *pick);
int knapsack_bt(const int *w, const int *v, int n, int cap);
int knapsack_bb(const int *w, const int *v, int n, int cap, int mode, long *nodes);
/* mode: 0 = FIFO, 1 = LIFO, 2 = least-cost (best bound first) */

/* ---------- flow, matching, number theory ---------- */
int max_flow(int n, int cap[MAXN][MAXN], int s, int t, int res[MAXN][MAXN]);
int bipartite_match(int nl, int nr, int adj[MAX_DRV][MAX_SPACES], int *matchL);
int crt(const int *rem, const int *mod, int n, long *x, long *M);

/* ---------- string matching (return match count, store first 'mx' positions) ---------- */
int s_naive(const char *t, const char *p, int *pos, int mx);
int s_rabin_karp(const char *t, const char *p, int *pos, int mx);
int s_automaton(const char *t, const char *p, int *pos, int mx);
int s_kmp(const char *t, const char *p, int *pos, int mx);

/* ---------- parking domain / persistence ---------- */
void lot_init(Lot *L, int nspaces);
int  lot_park(Lot *L, const char *plate, time_t now, int hour);   /* space idx, -1 full, -2 duplicate */
long lot_exit(Lot *L, const char *plate, time_t now, int *space, long *hours); /* fee or -1 */
int  lot_reserve(Lot *L, const char *plate, int start, int end, int now_hour); /* space idx or -1; now_hour=-1 ignores occupancy */
void lot_show(const Lot *L);
void lot_show_res(const Lot *L);
int  lot_save(const Lot *L, const char *path);
int  lot_load(Lot *L, const char *path);

#endif
