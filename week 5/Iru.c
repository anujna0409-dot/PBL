#include "page_replacement.h"

Stats lru(const int pages[], int n, int frame_count)
{
Stats stats = {0, 0, n};

if (pages == NULL || n <= 0 || frame_count <= 0) {
    stats.references = 0;
    return stats;
}

int frames[frame_count];
int last_used[frame_count];

// Initialize frames and last-used timestamps.
for (int i = 0; i < frame_count; i++) {
    frames[i] = -1;
    last_used[i] = -1;
}

// Process each page reference.
for (int i = 0; i < n; i++) {
    int found = -1;

    // Check whether the page is already in memory.
    for (int j = 0; j < frame_count; j++) {
        if (frames[j] == pages[i]) {
            found = j;
            break;
        }
    }

    if (found != -1) {
        // Page hit: update its last-used time.
        stats.hits++;
        last_used[found] = i;
    } else {
        // Page fault.
        stats.faults++;

        int replace = -1;

        // Use an empty frame if one is available.
        for (int j = 0; j < frame_count; j++) {
            if (frames[j] == -1) {
                replace = j;
                break;
            }
        }

        // If all frames are occupied, find the least recently used page.
        if (replace == -1) {
            replace = 0;

            for (int j = 1; j < frame_count; j++) {
                if (last_used[j] < last_used[replace]) {
                    replace = j;
                }
            }
        }

        // Load the new page into the selected frame.
        frames[replace] = pages[i];
        last_used[replace] = i;
    }
}

return stats;

}