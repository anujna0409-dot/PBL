
#include <stdio.h>
#include <string.h>
#include "banker.h"

void compute_need(State *s) {
    for (int i = 0; i < s->n; i++) {
        for (int j = 0; j < s->m; j++) {
            s->need[i][j] = s->max[i][j] - s->alloc[i][j];
        }
    }
}

/* Safety algorithm */
int is_safe(State *s, int seq[]) {
    int work[MAXR], fin[MAXP] = {0};
    int cnt = 0, na = 0, progress = 1;

    memcpy(work, s->avail, sizeof work);

    for (int i = 0; i < s->n; i++) {
        fin[i] = !s->active[i];
        if (s->active[i]) na++;
    }

    while (progress) {
        progress = 0;

        for (int i = 0; i < s->n; i++) {
            if (fin[i]) continue;

            int ok = 1;

            for (int j = 0; j < s->m; j++) {
                if (s->need[i][j] > work[j]) {
                    ok = 0;
                    break;
                }
            }

            if (ok) {
                for (int j = 0; j < s->m; j++) {
                    work[j] += s->alloc[i][j];
                }

                fin[i] = 1;
                seq[cnt++] = i;
                progress = 1;
            }
        }
    }

    seq[cnt] = -1;
    return cnt == na;
}

/* Banker's resource request algorithm */
int request_resources(State *s, int p, const int r[], char *msg) {
    int seq[MAXP + 1];

    if (p < 0 || p >= s->n || !s->active[p]) {
        strcpy(msg, "Invalid or terminated process");
        return 0;
    }

    for (int j = 0; j < s->m; j++) {
        if (r[j] < 0) {
            strcpy(msg, "Negative resource request");
            return 0;
        }

        if (r[j] > s->need[p][j]) {
            sprintf(msg, "P%d exceeded its maximum claim", p);
            return 0;
        }
    }

    /* Record the pending request */
    for (int j = 0; j < s->m; j++) {
        s->req[p][j] = r[j];
    }

    /* Keep the request pending if resources are unavailable */
    for (int j = 0; j < s->m; j++) {
        if (r[j] > s->avail[j]) {
            sprintf(msg, "P%d must WAIT: resources not available", p);
            return 0;
        }
    }

    /* Temporarily allocate resources */
    for (int j = 0; j < s->m; j++) {
        s->avail[j] -= r[j];
        s->alloc[p][j] += r[j];
        s->need[p][j] -= r[j];
    }

    if (is_safe(s, seq)) {
        for (int j = 0; j < s->m; j++) {
            s->req[p][j] = 0;
        }

        sprintf(msg, "Request GRANTED to P%d (state stays SAFE)", p);
        return 1;
    }

    /* Roll back unsafe allocation; retain pending request */
    for (int j = 0; j < s->m; j++) {
        s->avail[j] += r[j];
        s->alloc[p][j] -= r[j];
        s->need[p][j] += r[j];
    }

    sprintf(msg, "Request DENIED for P%d: unsafe state", p);
    return 0;
}

/* Resource utilization */
void utilization(State *s, double per[], double *overall) {
    int used = 0, tot = 0;

    for (int j = 0; j < s->m; j++) {
        int u = s->total[j] - s->avail[j];

        per[j] = s->total[j]
            ? 100.0 * u / s->total[j]
            : 0;

        used += u;
        tot += s->total[j];
    }

    *overall = tot ? 100.0 * used / tot : 0;
}
