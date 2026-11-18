/* String matching: naive, Rabin-Karp, finite automaton, KMP */
#include "parking.h"

#define REC(i) do { if (c < mx) pos[c] = (i); c++; } while (0)

int s_naive(const char *t, const char *p, int *pos, int mx) {
    int n = (int)strlen(t), m = (int)strlen(p), c = 0;
    if (!m) return 0;
    for (int i = 0; i + m <= n; i++) {
        int j = 0;
        while (j < m) { OP(); if (t[i + j] != p[j]) break; j++; }
        if (j == m) REC(i);
    }
    return c;
}

#define RK_Q 1000003L
int s_rabin_karp(const char *t, const char *p, int *pos, int mx) {
    int n = (int)strlen(t), m = (int)strlen(p), c = 0;
    if (!m || m > n) return 0;
    long h = fast_pow(256, m - 1, RK_Q), hp = 0, ht = 0;
    for (int i = 0; i < m; i++) {
        hp = (256 * hp + (unsigned char)p[i]) % RK_Q;
        ht = (256 * ht + (unsigned char)t[i]) % RK_Q;
    }
    for (int i = 0; i + m <= n; i++) {
        OP();
        if (hp == ht) {
            int j = 0;
            while (j < m) { OP(); if (t[i + j] != p[j]) break; j++; }
            if (j == m) REC(i);
        }
        if (i + m < n)
            ht = (((ht - (unsigned char)t[i] * h % RK_Q + RK_Q) % RK_Q) * 256 + (unsigned char)t[i + m]) % RK_Q;
    }
    return c;
}

int s_automaton(const char *t, const char *p, int *pos, int mx) {
    int n = (int)strlen(t), m = (int)strlen(p), c = 0;
    if (!m) return 0;
    int *d = talloc((size_t)(m + 1) * 256 * sizeof(int));
    d[(unsigned char)p[0]] = 1;
    for (int q = 1, X = 0; q <= m; q++) {
        memcpy(d + q * 256, d + X * 256, 256 * sizeof(int));
        if (q < m) { d[q * 256 + (unsigned char)p[q]] = q + 1; X = d[X * 256 + (unsigned char)p[q]]; }
    }
    int st = 0;
    for (int i = 0; i < n; i++) {
        OP();
        st = d[st * 256 + (unsigned char)t[i]];
        if (st == m) REC(i - m + 1);
    }
    free(d);
    return c;
}

int s_kmp(const char *t, const char *p, int *pos, int mx) {
    int n = (int)strlen(t), m = (int)strlen(p), c = 0;
    if (!m) return 0;
    int *lps = talloc((size_t)m * sizeof(int));
    for (int i = 1, k = 0; i < m; i++) {
        while (k > 0 && p[i] != p[k]) { OP(); k = lps[k - 1]; }
        OP();
        if (p[i] == p[k]) k++;
        lps[i] = k;
    }
    for (int i = 0, k = 0; i < n; i++) {
        while (k > 0 && t[i] != p[k]) { OP(); k = lps[k - 1]; }
        OP();
        if (t[i] == p[k]) k++;
        if (k == m) { REC(i - m + 1); k = lps[k - 1]; }
    }
    free(lps);
    return c;
}
