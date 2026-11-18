/* Divide and conquer modules */
#include "parking.h"

static void ms(int *a, int *t, int lo, int hi) {
    if (hi - lo < 2) return;
    int mid = (lo + hi) / 2;
    ms(a, t, lo, mid);
    ms(a, t, mid, hi);
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) { OP(); t[k++] = (a[i] <= a[j]) ? a[i++] : a[j++]; }
    while (i < mid) t[k++] = a[i++];
    while (j < hi)  t[k++] = a[j++];
    memcpy(a + lo, t + lo, (size_t)(hi - lo) * sizeof(int));
}
void merge_sort(int *a, int n) {
    if (n < 2) return;
    int *t = talloc((size_t)n * sizeof(int));
    ms(a, t, 0, n);
    free(t);
}

void quick_sort(int *a, int lo, int hi) {            /* Lomuto, last-element pivot */
    if (lo >= hi) return;
    int p = a[hi], i = lo - 1, tmp;
    for (int j = lo; j < hi; j++) {
        OP();
        if (a[j] <= p) { i++; tmp = a[i]; a[i] = a[j]; a[j] = tmp; }
    }
    tmp = a[i + 1]; a[i + 1] = a[hi]; a[hi] = tmp;
    quick_sort(a, lo, i);
    quick_sort(a, i + 2, hi);
}

int bsearch_iter(const int *a, int n, int key) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        OP();
        int m = lo + (hi - lo) / 2;
        if (a[m] == key) return m;
        if (a[m] < key) lo = m + 1; else hi = m - 1;
    }
    return -1;
}
int bsearch_rec(const int *a, int lo, int hi, int key) {
    if (lo > hi) return -1;
    OP();
    int m = lo + (hi - lo) / 2;
    if (a[m] == key) return m;
    return (a[m] < key) ? bsearch_rec(a, m + 1, hi, key) : bsearch_rec(a, lo, m - 1, key);
}

/* maximum-sum contiguous subarray (busiest consecutive hours of net arrivals) */
long max_subarray(const int *a, int lo, int hi, int *l, int *r) {
    if (lo == hi) { *l = *r = lo; return a[lo]; }
    int mid = (lo + hi) / 2, l1, r1, l2, r2;
    long L = max_subarray(a, lo, mid, &l1, &r1);
    long R = max_subarray(a, mid + 1, hi, &l2, &r2);
    long s = 0, bl = -(1L << 60), br = -(1L << 60);
    int cl = mid, cr = mid + 1;
    for (int i = mid; i >= lo; i--) { OP(); s += a[i]; if (s > bl) { bl = s; cl = i; } }
    s = 0;
    for (int i = mid + 1; i <= hi; i++) { OP(); s += a[i]; if (s > br) { br = s; cr = i; } }
    long C = bl + br;
    if (L >= R && L >= C) { *l = l1; *r = r1; return L; }
    if (R >= C) { *l = l2; *r = r2; return R; }
    *l = cl; *r = cr; return C;
}

long fast_pow(long b, long e, long m) {              /* b^e mod m, T(e)=T(e/2)+O(1) */
    OP();
    if (e == 0) return 1 % m;
    long h = fast_pow(b, e / 2, m);
    h = h * h % m;
    if (e & 1) h = h * (b % m) % m;
    return h;
}

/* celebrity = zone everybody prefers (k[i][c]=1) that prefers nobody (k[c][i]=0) */
static int cel_cand(int k[CEL_N][CEL_N], int lo, int hi) {
    if (lo == hi) return lo;
    int m = (lo + hi) / 2;
    int a = cel_cand(k, lo, m), b = cel_cand(k, m + 1, hi);
    OP();
    return k[a][b] ? b : a;       /* if a knows b, a cannot be celebrity; else b cannot */
}
int find_celebrity(int n, int k[CEL_N][CEL_N]) {
    if (n <= 0 || n > CEL_N) return -1;
    int c = cel_cand(k, 0, n - 1);
    for (int i = 0; i < n; i++) {
        if (i == c) continue;
        OP();
        if (k[c][i] || !k[i][c]) return -1;
    }
    return c;
}

void mat_classic(const long *A, const long *B, long *C, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            long s = 0;
            for (int k = 0; k < n; k++) { OP(); s += A[i * n + k] * B[k * n + j]; }
            C[i * n + j] = s;
        }
}
static void madd(const long *A, const long *B, long *C, int sz) { for (int i = 0; i < sz; i++) C[i] = A[i] + B[i]; }
static void msub(const long *A, const long *B, long *C, int sz) { for (int i = 0; i < sz; i++) C[i] = A[i] - B[i]; }

void mat_strassen(const long *A, const long *B, long *C, int n) {
    if (n <= 2) { mat_classic(A, B, C, n); return; }
    int h = n / 2, sz = h * h;
    long *buf = talloc((size_t)sz * 17 * sizeof(long));
    long *a11 = buf, *a12 = a11 + sz, *a21 = a12 + sz, *a22 = a21 + sz;
    long *b11 = a22 + sz, *b12 = b11 + sz, *b21 = b12 + sz, *b22 = b21 + sz;
    long *m1 = b22 + sz, *m2 = m1 + sz, *m3 = m2 + sz, *m4 = m3 + sz, *m5 = m4 + sz, *m6 = m5 + sz, *m7 = m6 + sz;
    long *t1 = m7 + sz, *t2 = t1 + sz;
    for (int i = 0; i < h; i++)
        for (int j = 0; j < h; j++) {
            a11[i*h+j] = A[i*n+j];       a12[i*h+j] = A[i*n+j+h];
            a21[i*h+j] = A[(i+h)*n+j];   a22[i*h+j] = A[(i+h)*n+j+h];
            b11[i*h+j] = B[i*n+j];       b12[i*h+j] = B[i*n+j+h];
            b21[i*h+j] = B[(i+h)*n+j];   b22[i*h+j] = B[(i+h)*n+j+h];
        }
    madd(a11, a22, t1, sz); madd(b11, b22, t2, sz); mat_strassen(t1, t2, m1, h);
    madd(a21, a22, t1, sz);                          mat_strassen(t1, b11, m2, h);
    msub(b12, b22, t2, sz);                          mat_strassen(a11, t2, m3, h);
    msub(b21, b11, t2, sz);                          mat_strassen(a22, t2, m4, h);
    madd(a11, a12, t1, sz);                          mat_strassen(t1, b22, m5, h);
    msub(a21, a11, t1, sz); madd(b11, b12, t2, sz); mat_strassen(t1, t2, m6, h);
    msub(a12, a22, t1, sz); madd(b21, b22, t2, sz); mat_strassen(t1, t2, m7, h);
    for (int i = 0; i < h; i++)
        for (int j = 0; j < h; j++) {
            int x = i * h + j;
            C[i*n+j]         = m1[x] + m4[x] - m5[x] + m7[x];
            C[i*n+j+h]       = m3[x] + m5[x];
            C[(i+h)*n+j]     = m2[x] + m4[x];
            C[(i+h)*n+j+h]   = m1[x] - m2[x] + m3[x] + m6[x];
        }
    free(buf);
}
