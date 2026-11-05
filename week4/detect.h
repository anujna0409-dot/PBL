#ifndef DETECT_H
#define DETECT_H

#include "common.h"

/* Detect deadlocked processes */
int detect_deadlock(State *s, int dead[]);

/* Recover from deadlock */
int recover_one(State *s, char *msg);

#endif