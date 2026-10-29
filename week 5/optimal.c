
#include <stdio.h>
#include "page_replacement.h"

Stats optimal(const int pages[], int n, int frame_count)
{
    Stats stats = {0, 0, n};

    if (pages == NULL || n <= 0 ||
        frame_count <= 0 || frame_count > MAX_FRAMES) {
        printf("Error: Invalid input for Optimal algorithm.\n");
        stats.references = 0;
        return stats;
    }

    int frames[MAX_FRAMES];

    for (int i = 0; i < frame_count; i++)
        frames[i] = -1;

    printf("\n--- Optimal Page Replacement ---\n");
    printf("Page\tFrames\t\tStatus\n");

    for (int i = 0; i < n; i++) {
        int page = pages[i];
        int found = -1;

        for (int j = 0; j < frame_count; j++) {
            if (frames[j] == page) {
                found = j;
                break;
            }
        }

        if (found != -1) {
            stats.hits++;
            printf("%d\t", page);

            for (int j = 0; j < frame_count; j++) {
                if (frames[j] == -1)
                    printf("- ");
                else
                    printf("%d ", frames[j]);
            }

            printf("\tHIT\n");
        } else {
            stats.faults++;
            int victim = -1;

            /* Use an empty frame first. */
            for (int j = 0; j < frame_count; j++) {
                if (frames[j] == -1) {
                    victim = j;
                    break;
                }
            }

            /* Otherwise, replace the page needed farthest in the future. */
            if (victim == -1) {
                int farthest = -1;

                for (int j = 0; j < frame_count; j++) {
                    int next_use = n;

                    for (int k = i + 1; k < n; k++) {
                        if (pages[k] == frames[j]) {
                            next_use = k;
                            break;
                        }
                    }

                    if (next_use > farthest) {
                        farthest = next_use;
                        victim = j;
                    }
                }
            }

            frames[victim] = page;

            printf("%d\t", page);

            for (int j = 0; j < frame_count; j++) {
                if (frames[j] == -1)
                    printf("- ");
                else
                    printf("%d ", frames[j]);
            }

            printf("\tFAULT\n");
        }
    }

    printf("\nTotal References: %d\n", stats.references);
    printf("Total Hits: %d\n", stats.hits);
    printf("Total Page Faults: %d\n", stats.faults);

    return stats;
}
