#ifndef COMMON_H
#define COMMON_H

#define MAXP 10
#define MAXR 10

typedef struct {
    int n, m;                       /* processes, resource types */
    int total[MAXR], avail[MAXR];
    int alloc[MAXP][MAXR], max[MAXP][MAXR], need[MAXP][MAXR];
    int req[MAXP][MAXR];            /* pending (waiting) requests */
    int active[MAXP];               /* 0 = terminated by recovery */
} State;

/* banker.c  (Member 2) */
void compute_need(State *s);
int  is_safe(State *s, int seq[]);                  /* seq size MAXP+1, ends with -1 */
int  request_resources(State *s, int p, const int r[], char *msg);
void utilization(State *s, double per[], double *overall);

/* detect.c  (Member 3) */
int  detect_deadlock(State *s, int dead[]);         /* returns count */
int  recover_one(State *s, char *msg);              /* terminates one victim */

#endif
