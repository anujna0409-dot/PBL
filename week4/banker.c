#include <stdio.h>
#include <string.h>
#include "banker.h"

void compute_need(State *s) {
    for (int i = 0; i < s->n; i++)
        for (int j = 0; j < s->m; j++)
            s->need[i][j] = s->max[i][j] - s->alloc[i][j];
}

/* Safety algorithm. Returns 1 if SAFE (seq holds the safe sequence). */
int is_safe(State *s, int seq[]) {
    int work[MAXR], fin[MAXP], cnt = 0, na = 0, progress = 1;
    memcpy(work, s->avail, sizeof work);
    for (int i = 0; i < s->n; i++) { fin[i] = !s->active[i]; na += s->active[i]; }

    while (progress) {
        progress = 0;
        for (int i = 0; i < s->n; i++) {
            if (fin[i]) continue;
            int ok = 1;
            for (int j = 0; j < s->m; j++) if (s->need[i][j] > work[j]) { ok = 0; break; }
            if (ok) {
                for (int j = 0; j < s->m; j++) work[j] += s->alloc[i][j];
                fin[i] = 1; seq[cnt++] = i; progress = 1;
            }
        }
    }
    seq[cnt] = -1;
    return cnt == na;
}

/* Banker's resource-request algorithm. Returns 1 if granted. */
int request_resources(State *s, int p, const int r[], char *msg) {
    int seq[MAXP + 1], j;
    if (p < 0 || p >= s->n || !s->active[p]) { strcpy(msg, "Invalid or terminated process"); return 0; }
    for (j = 0; j < s->m; j++) if (r[j] < 0) { strcpy(msg, "Negative request"); return 0; }
    for (j = 0; j < s->m; j++)
        if (r[j] > s->need[p][j]) { sprintf(msg, "P%d exceeded its maximum claim", p); return 0; }

    for (j = 0; j < s->m; j++) s->req[p][j] = r[j];      /* mark as pending */
    for (j = 0; j < s->m; j++)
        if (r[j] > s->avail[j]) { sprintf(msg, "P%d must WAIT: resources not available", p); return 0; }

    for (j = 0; j < s->m; j++) { s->avail[j] -= r[j]; s->alloc[p][j] += r[j]; s->need[p][j] -= r[j]; }
    if (is_safe(s, seq)) {
        for (j = 0; j < s->m; j++) s->req[p][j] = 0;
        sprintf(msg, "Request GRANTED to P%d (state stays SAFE)", p);
        return 1;
    }
    for (j = 0; j < s->m; j++) { s->avail[j] += r[j]; s->alloc[p][j] -= r[j]; s->need[p][j] += r[j]; }
    sprintf(msg, "Request DENIED for P%d: it would lead to an UNSAFE state", p);
    return 0;
}

void utilization(State *s, double per[], double *overall) {
    int used = 0, tot = 0;
    for (int j = 0; j < s->m; j++) {
        int u = s->total[j] - s->avail[j];
        per[j] = s->total[j] ? 100.0 * u / s->total[j] : 0;
        used += u; tot += s->total[j];
    }
    *overall = tot ? 100.0 * used / tot : 0;
}
