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

#include "banker.h"   /* Member 2: banker.c */
#include "detect.h"   /* Member 3: detect.c */
#include "rag.h"      /* Member 3: rag.c    */

#endif
