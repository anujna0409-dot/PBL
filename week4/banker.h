#ifndef BANKER_H
#define BANKER_H
#include "common.h"

/* Member 2: Banker's algorithm core */
void compute_need(State *s);                         /* Need = Max - Allocation */
int  is_safe(State *s, int seq[]);                   /* 1 = SAFE; seq has MAXP+1 slots, ends with -1 */
int  request_resources(State *s, int p, const int r[], char *msg);  /* 1 = granted */
void utilization(State *s, double per[], double *overall);

#endif
