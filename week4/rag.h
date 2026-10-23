#ifndef RAG_H
#define RAG_H
#include "common.h"

/* Member 3: Resource Allocation Graph.
   Writes the edge list as a JSON array into buf, e.g.
   [{"from":"R0","to":"P1","type":"alloc","count":2},{"from":"P1","to":"R2","type":"request","count":1}]
   alloc = Resource -> Process, request = Process -> Resource. Returns characters written. */
int rag_write_json(State *s, char *buf, int size);

#endif
