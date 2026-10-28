#include <stdio.h>
#include "page_replacement.h"

/*
 * First-In, First-Out (FIFO) page replacement.
 * Replaces the page that has been in memory the longest.
 */
Stats fifo(const int pages[], int n, int frame_count)
{
    Stats stats = {0, 0, 0};
    int frames[MAX_FRAMES];
    int next_replace = 0;
    int loaded = 0;

    if (pages == NULL || n < 0 || n > MAX_PAGES ||
        frame_count <= 0 || frame_count > MAX_FRAMES) {
        fprintf(stderr, "Error: invalid FIFO input or frame count.\n");
        return stats;
    }

    stats.references = n;

    for (int i = 0; i < frame_count; ++i) {
        frames[i] = 0; /* Contents are ignored until the frame is loaded. */
    }

    printf("FIFO Page Replacement\n");
    printf("Page\t");
    for (int i = 0; i < frame_count; ++i) {
        printf("Frame %d\t", i + 1);
    }
    printf("Result\n");

    for (int i = 0; i < n; ++i) {
        int page = pages[i];
        int hit = 0;

        /* Check whether the requested page is already in a frame. */
        for (int j = 0; j < loaded; ++j) {
            if (frames[j] == page) {
                hit = 1;
                break;
            }
        }

        if (hit) {
            ++stats.hits;
        } else {
            ++stats.faults;

            if (loaded < frame_count) {
                /* Fill empty frames before replacing any page. */
                frames[loaded++] = page;
                if (loaded == frame_count) {
                    next_replace = 0;
                }
            } else {
                /* Replace the oldest page, then advance the FIFO pointer. */
                frames[next_replace] = page;
                next_replace = (next_replace + 1) % frame_count;
            }
        }

        printf("%d\t", page);
        for (int j = 0; j < frame_count; ++j) {
            if (j < loaded) {
                printf("%d\t", frames[j]);
            } else {
                printf("-\t");
            }
        }
        printf("%s\n", hit ? "Hit" : "Fault");
    }

    printf("Total hits: %d\n", stats.hits);
    printf("Total faults: %d\n", stats.faults);

    return stats;
}
