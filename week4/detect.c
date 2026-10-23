/* ===== MEMBER 3: detect.c  (deadlock detection + recovery) ===== */
#include <stdio.h>
#include "detect.h"

int detect_deadlock(const State *s, int dead[]) {
    int work[MAX_R], fin[MAX_P], i, j, progress = 1, cnt = 0;
    for (j = 0; j < s->m; j++) work[j] = s->avail[j];
    for (i = 0; i < s->n; i++) {
        int held = 0;
        for (j = 0; j < s->m; j++) held += s->alloc[i][j];
        fin[i] = (!s->active[i] || held == 0);   /* holds nothing -> cannot be in deadlock */
    }
    while (progress) {
        progress = 0;
        for (i = 0; i < s->n; i++) {
            if (fin[i]) continue;
            int ok = 1;
            for (j = 0; j < s->m; j++) if (s->req[i][j] > work[j]) { ok = 0; break; }
            if (ok) {
                for (j = 0; j < s->m; j++) work[j] += s->alloc[i][j];
                fin[i] = 1; progress = 1;
            }
        }
    }
    for (i = 0; i < s->n; i++) { dead[i] = !fin[i]; cnt += dead[i]; }
    return cnt;
}

int pick_victim(const State *s, const int dead[]) {
    int i, j, best = -1, bestHeld = -1;
    for (i = 0; i < s->n; i++) {
        if (!dead[i]) continue;
        int held = 0;
        for (j = 0; j < s->m; j++) held += s->alloc[i][j];
        if (held > bestHeld) { bestHeld = held; best = i; }
    }
    return best;
}

static void release_all(State *s, int p) {
    for (int j = 0; j < s->m; j++) {
        s->avail[j] += s->alloc[p][j];
        s->alloc[p][j] = 0; s->req[p][j] = 0; s->need[p][j] = s->max[p][j];
    }
}

int recover_terminate(State *s) {
    int dead[MAX_P], killed = 0, v;
    while (detect_deadlock(s, dead) > 0) {
        v = pick_victim(s, dead);
        printf("Terminating P%d\n", v);
        release_all(s, v);
        s->active[v] = 0;
        killed++;
    }
    return killed;
}

int recover_preempt(State *s, int victim) {
    if (victim < 0 || victim >= s->n || !s->active[victim]) return -1;
    printf("Preempting all resources of P%d (rolled back, stays active)\n", victim);
    release_all(s, victim);   /* process restarts later with need = max */
    return 0;
}
