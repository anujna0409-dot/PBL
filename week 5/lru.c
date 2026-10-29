#include <stdio.h>
#include "page_replacement.h"

Stats lru(const int pages[], int n, int frame_count)
{
    Stats stats = {0, 0, n};

    if (pages == NULL || n <= 0 ||
        frame_count <= 0 || frame_count > MAX_FRAMES) {
        printf("Error: Invalid input for LRU algorithm.\n");
        stats.references = 0;
        return stats;
    }

    int frames[MAX_FRAMES];
    int last_used[MAX_FRAMES];

    // Initialize frames and timestamps.
    for (int i = 0; i < frame_count; i++) {
        frames[i] = -1;
        last_used[i] = -1;
    }

    printf("\n--- LRU Page Replacement ---\n");
    printf("Page\tFrames\t\tStatus\n");

    // Process each page reference.
    for (int i = 0; i < n; i++) {
        int page = pages[i];
        int found = -1;

        // Check whether the page is already in memory.
        for (int j = 0; j < frame_count; j++) {
            if (frames[j] == page) {
                found = j;
                break;
            }
        }

        if (found != -1) {
            // Page hit.
            stats.hits++;
            last_used[found] = i;
        } else {
            // Page fault.
            stats.faults++;

            int replace = -1;

            // Find an empty frame.
            for (int j = 0; j < frame_count; j++) {
                if (frames[j] == -1) {
                    replace = j;
                    break;
                }
            }

            // Otherwise, replace the least recently used page.
            if (replace == -1) {
                replace = 0;

                for (int j = 1; j < frame_count; j++) {
                    if (last_used[j] < last_used[replace]) {
                        replace = j;
                    }
                }
            }

            frames[replace] = page;
            last_used[replace] = i;
        }

        // Display the current frame contents.
        printf("%d\t", page);

        for (int j = 0; j < frame_count; j++) {
            if (frames[j] == -1) {
                printf("- ");
            } else {
                printf("%d ", frames[j]);
            }
        }

        if (found != -1) {
            printf("\tHIT\n");
        } else {
            printf("\tFAULT\n");
        }
    }

    printf("\nTotal References: %d\n", stats.references);
    printf("Total Hits: %d\n", stats.hits);
    printf("Total Page Faults: %d\n", stats.faults);

    return stats;
}
