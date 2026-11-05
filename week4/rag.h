#ifndef RAG_H
#define RAG_H

#include "common.h"

/* Writes Resource Allocation Graph as JSON */
int rag_write_json(State *s, char *buf, int size);

#endif