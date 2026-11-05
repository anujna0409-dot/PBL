#ifndef RAG_H
#define RAG_H

#include "common.h"

/* Writes the Resource Allocation Graph edge list as a JSON array.
   Allocation: Resource -> Process. Request: Process -> Resource.
   Returns the number of characters written. */
int rag_write_json(State *s, char *buf, int size);

#endif
