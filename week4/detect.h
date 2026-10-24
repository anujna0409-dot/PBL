cat > backend/rag.h <<'EOF'
#ifndef RAG_H
#define RAG_H
#include "common.h"

// Member 3: Resource Allocation Graph.
// Writes the edge list as a JSON array into buf.
// alloc = Resource -> Process, request = Process -> Resource.
// Returns the number of characters written.
int rag_write_json(State *s, char *buf, int size);

#endif
EOF
