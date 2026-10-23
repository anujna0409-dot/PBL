#include <stdio.h>
#include "rag.h"

int rag_write_json(State *s, char *buf, int size) {
    int len = snprintf(buf, size, "["), first = 1;
    for (int i = 0; i < s->n && len < size; i++) {
        if (!s->active[i]) continue;                       /* terminated processes leave the graph */
        for (int j = 0; j < s->m && len < size; j++) {
            if (s->alloc[i][j] > 0) {
                len += snprintf(buf + len, size - len,
                    "%s{\"from\":\"R%d\",\"to\":\"P%d\",\"type\":\"alloc\",\"count\":%d}",
                    first ? "" : ",", j, i, s->alloc[i][j]);
                first = 0;
            }
            if (s->req[i][j] > 0) {
                len += snprintf(buf + len, size - len,
                    "%s{\"from\":\"P%d\",\"to\":\"R%d\",\"type\":\"request\",\"count\":%d}",
                    first ? "" : ",", i, j, s->req[i][j]);
                first = 0;
            }
        }
    }
    if (len < size) len += snprintf(buf + len, size - len, "]");
    return len;
}
