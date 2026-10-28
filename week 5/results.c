#include <stdio.h>
#include "page_replacement.h"

void print_algorithm_result(const char *name, Stats stats)
{
    double hit_ratio = stats.references > 0
        ? (100.0 * stats.hits / stats.references) : 0.0;
    double fault_ratio = stats.references > 0
        ? (100.0 * stats.faults / stats.references) : 0.0;

    printf("\n--- %s Summary ---\n", name);
    printf("Total references : %d\n", stats.references);
    printf("Page hits        : %d\n", stats.hits);
    printf("Page faults      : %d\n", stats.faults);
    printf("Hit ratio        : %.2f%%\n", hit_ratio);
    printf("Fault ratio      : %.2f%%\n", fault_ratio);
}

void print_summary(const Stats results[], int count)
{
    const char *names[] = {"FIFO", "LRU", "Optimal"};
    int i;

    printf("\n================ ALGORITHM COMPARISON ================\n");
    printf("%-12s %10s %12s %12s %13s\n",
           "Algorithm", "References", "Page Hits", "Page Faults", "Fault Ratio");
    printf("------------------------------------------------------"
           "----------------\n");

    for (i = 0; i < count && i < 3; i++) {
        double fault_ratio = results[i].references > 0
            ? (100.0 * results[i].faults / results[i].references) : 0.0;
        printf("%-12s %10d %12d %12d %12.2f%%\n",
               names[i], results[i].references, results[i].hits,
               results[i].faults, fault_ratio);
    }
    printf("======================================================"
           "================\n");

    if (count > 0) {
        int best = 0;
        for (i = 1; i < count && i < 3; i++) {
            if (results[i].faults < results[best].faults) {
                best = i;
            }
        }
        printf("Fewest page faults: %s (%d faults)\n",
               names[best], results[best].faults);
        printf("Note: Optimal is expected to have the minimum or tied-minimum "
               "fault count for the same input and frame count.\n");
    }
}
