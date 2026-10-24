#ifndef DETECT_H
#define DETECT_H
#include "common.h"

int detect_deadlock(State *s, int dead[]);   /* returns number of deadlocked processes, fills dead[] */
int recover_one(State *s, char *msg);        /* terminates one victim; returns 1 if something was done */

#endif
