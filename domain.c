/* Parking domain model: lot setup, allocation, reservation, billing, persistence */
#include "parking.h"

void lot_init(Lot *L, int n) {
    memset(L, 0, sizeof *L);
    if (n > MAX_SPACES) n = MAX_SPACES;
    L->n = n; L->nextRes = 1;
    graph_init(&L->road, n + 1);                        /* node 0 = gate */
    graph_add(&L->road, 0, 1, 4);
    if (n >= 2) graph_add(&L->road, 0, 2, 6);
    for (int i = 1; i < n; i++) graph_add(&L->road, i, i + 1, 5 + (i * 3) % 7);
    for (int i = 4; i <= n; i++) graph_add(&L->road, i, i - 3, 9 + (i * 5) % 6);
    int dist[MAXN], prev[MAXN];
    dijkstra(&L->road, 0, dist, prev);
    for (int i = 0; i < n; i++) {
        L->sp[i].id = i + 1; L->sp[i].dist = dist[i + 1]; L->sp[i].price = 10 + 5 * ((i + 1) % 3);
    }
}

static int reserved_by_other(const Lot *L, int s, const char *plate, int hour) {
    for (int r = 0; r < L->nres; r++)
        if (L->res[r].space == s && hour >= L->res[r].start && hour < L->res[r].end && strcmp(L->res[r].plate, plate))
            return 1;
    return 0;
}

static void place(Lot *L, int s, const char *plate, time_t now) {
    L->sp[s].occupied = 1;
    snprintf(L->sp[s].plate, PLATE_LEN, "%s", plate);
    L->sp[s].entry = now;
}

int lot_park(Lot *L, const char *plate, time_t now, int hour) {
    for (int i = 0; i < L->n; i++) if (L->sp[i].occupied && !strcmp(L->sp[i].plate, plate)) return -2;
    for (int r = 0; r < L->nres; r++) {                 /* honour the driver's own reservation */
        int s = L->res[r].space;
        if (!strcmp(L->res[r].plate, plate) && hour >= L->res[r].start && hour < L->res[r].end && !L->sp[s].occupied) {
            place(L, s, plate, now); return s;
        }
    }
    int keys[MAX_SPACES], k = 0;                        /* nearest free space: sort by (distance, index) */
    for (int i = 0; i < L->n; i++)
        if (!L->sp[i].occupied && !reserved_by_other(L, i, plate, hour)) keys[k++] = L->sp[i].dist * MAX_SPACES + i;
    if (!k) return -1;
    merge_sort(keys, k);
    int s = keys[0] % MAX_SPACES;
    place(L, s, plate, now);
    return s;
}

long lot_exit(Lot *L, const char *plate, time_t now, int *space, long *hours) {
    for (int i = 0; i < L->n; i++) {
        if (L->sp[i].occupied && !strcmp(L->sp[i].plate, plate)) {
            long secs = (long)difftime(now, L->sp[i].entry);
            long h = (secs + 3599) / 3600;
            if (h < 1) h = 1;
            long fee = h * L->sp[i].price;
            L->revenue += fee;
            L->sp[i].occupied = 0; L->sp[i].plate[0] = 0;
            if (space) *space = i;
            if (hours) *hours = h;
            return fee;
        }
    }
    return -1;
}

int lot_reserve(Lot *L, const char *plate, int start, int end, int now_hour) {
    if (start < 0 || end > 24 || start >= end || L->nres >= MAX_RES || !plate[0]) return -1;
    int keys[MAX_SPACES];
    for (int i = 0; i < L->n; i++) keys[i] = L->sp[i].dist * MAX_SPACES + i;
    merge_sort(keys, L->n);
    for (int k = 0; k < L->n; k++) {
        int s = keys[k] % MAX_SPACES, clash = 0;
        if (L->sp[s].occupied && now_hour >= start && now_hour < end && strcmp(L->sp[s].plate, plate)) continue; /* busy right now */
        for (int r = 0; r < L->nres && !clash; r++)
            if (L->res[r].space == s && !(end <= L->res[r].start || start >= L->res[r].end)) clash = 1;
        if (!clash) {
            Reservation *R = &L->res[L->nres++];
            R->id = L->nextRes++; snprintf(R->plate, PLATE_LEN, "%s", plate);
            R->start = start; R->end = end; R->space = s; R->value = (end - start) * L->sp[s].price;
            return s;
        }
    }
    return -1;
}

void lot_show(const Lot *L) {
    int fr = 0;
    printf("\n%-6s %-9s %-10s %-10s %-12s\n", "Space", "Dist(m)", "Rs/hour", "Status", "Vehicle");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < L->n; i++) {
        printf("%-6d %-9d %-10d %-10s %-12s\n", L->sp[i].id, L->sp[i].dist, L->sp[i].price,
               L->sp[i].occupied ? "Occupied" : "Free", L->sp[i].occupied ? L->sp[i].plate : "-");
        if (!L->sp[i].occupied) fr++;
    }
    printf("Free: %d/%d    Revenue: Rs. %ld\n", fr, L->n, L->revenue);
}

void lot_show_res(const Lot *L) {
    if (!L->nres) { printf("No reservations.\n"); return; }
    printf("\n%-5s %-12s %-7s %-7s %-6s %-8s\n", "ID", "Vehicle", "Start", "End", "Space", "Value");
    for (int r = 0; r < L->nres; r++)
        printf("%-5d %-12s %02d:00   %02d:00   %-6d Rs.%d\n", L->res[r].id, L->res[r].plate,
               L->res[r].start, L->res[r].end, L->sp[L->res[r].space].id, L->res[r].value);
}

int lot_save(const Lot *L, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "V,%ld,%d,%d\n", L->revenue, L->nextRes, L->n);
    for (int i = 0; i < L->n; i++)
        if (L->sp[i].occupied) fprintf(f, "S,%d,%s,%ld\n", i, L->sp[i].plate, (long)L->sp[i].entry);
    for (int r = 0; r < L->nres; r++)
        fprintf(f, "R,%d,%s,%d,%d,%d,%d\n", L->res[r].id, L->res[r].plate, L->res[r].start, L->res[r].end, L->res[r].value, L->res[r].space);
    fclose(f);
    return 1;
}

int lot_load(Lot *L, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[128];
    int n = L->n;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == 'V') {
            long rev; int nr, nn;
            if (sscanf(line, "V,%ld,%d,%d", &rev, &nr, &nn) == 3 && nn == n) { L->revenue = rev; L->nextRes = nr; }
        } else if (line[0] == 'S') {
            int i; char pl[PLATE_LEN]; long t;
            if (sscanf(line, "S,%d,%15[^,],%ld", &i, pl, &t) == 3 && i >= 0 && i < n) { place(L, i, pl, (time_t)t); }
        } else if (line[0] == 'R' && L->nres < MAX_RES) {
            Reservation *R = &L->res[L->nres];
            if (sscanf(line, "R,%d,%15[^,],%d,%d,%d,%d", &R->id, R->plate, &R->start, &R->end, &R->value, &R->space) == 6
                && R->space >= 0 && R->space < n) L->nres++;
        }
    }
    fclose(f);
    return 1;
}
