
#include <stdio.h>
#include <string.h>
#include "detect.h"

/* Deadlock Detection Algorithm */
int detect_deadlock(State *s, int dead[]) {
    int work[MAXR], fin[MAXP] = {0};
    int cnt = 0, progress = 1;

    memcpy(work, s->avail, sizeof work);

    /* Inactive processes are already finished */
    for (int i = 0; i < s->n; i++) {
        fin[i] = !s->active[i];
    }

    while (progress) {
        progress = 0;

        for (int i = 0; i < s->n; i++) {
            if (fin[i]) continue;

            int ok = 1;

            for (int j = 0; j < s->m; j++) {
                if (s->req[i][j] > work[j]) {
                    ok = 0;
                    break;
                }
            }

            if (ok) {
                for (int j = 0; j < s->m; j++) {
                    work[j] += s->alloc[i][j];
                }

                fin[i] = 1;
                progress = 1;
            }
        }
    }

    /* Remaining active processes are deadlocked */
    for (int i = 0; i < s->n; i++) {
        if (!fin[i]) {
            dead[cnt++] = i;
        }
    }

    return cnt;
}

/* Recovery by terminating the process holding the most resources */
int recover_one(State *s, char *msg) {
    int dead[MAXP];
    int nd = detect_deadlock(s, dead);

    if (nd == 0) {
        strcpy(msg, "No deadlock - nothing to recover");
        return 0;
    }

    int v = dead[0], best = -1;

    for (int k = 0; k < nd; k++) {
        int sum = 0;

        for (int j = 0; j < s->m; j++) {
            sum += s->alloc[dead[k]][j];
        }

        if (sum > best) {
            best = sum;
            v = dead[k];
        }
    }

    for (int j = 0; j < s->m; j++) {
        s->avail[j] += s->alloc[v][j];
        s->alloc[v][j] = 0;
        s->max[v][j] = 0;
        s->need[v][j] = 0;
        s->req[v][j] = 0;
    }

    s->active[v] = 0;

    /* Retry other waiting processes */
    char tmp[160];

    for (int i = 0; i < s->n; i++) {
        int r[MAXR], any = 0;

        if (!s->active[i]) continue;

        for (int j = 0; j < s->m; j++) {
            r[j] = s->req[i][j];
            any |= r[j];
        }

        if (any) {
            request_resources(s, i, r, tmp);
        }
    }

    sprintf(msg, "Recovery: terminated P%d and released its resources", v);

    return 1;
}
