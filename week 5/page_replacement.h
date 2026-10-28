#ifndef PAGE_REPLACEMENT_H
#define PAGE_REPLACEMENT_H

#define MAX_PAGES 1000
#define MAX_FRAMES 100

typedef enum {
    ALG_FIFO = 1,
    ALG_LRU = 2,
    ALG_OPTIMAL = 3
} Algorithm;

typedef struct {
    int hits;
    int faults;
    int references;
} Stats;

/*
 * All algorithm functions print a step-by-step frame table and return
 * statistics. Pages are integer page numbers; n is the number of references.
 */
Stats fifo(const int pages[], int n, int frame_count);
Stats lru(const int pages[], int n, int frame_count);
Stats optimal(const int pages[], int n, int frame_count);

void print_summary(const Stats results[], int count);
void print_algorithm_result(const char *name, Stats stats);

#endif
