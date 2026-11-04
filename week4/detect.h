#ifndef DETECT_H
#define DETECT_H

#include "common.h"

/* Returns the number of deadlocked processes and stores their IDs in dead[]. */
int detect_deadlock(State *s, int dead[]);

/* Terminates one deadlocked process, releases its resources and retries waiting requests. */
int recover_one(State *s, char *msg);

#endif
