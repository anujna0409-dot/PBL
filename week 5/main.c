#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "page_replacement.h"

static int read_line(char *buffer, size_t size)
{
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else {
        /* Drain any extra characters so the next input starts cleanly. */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }
    return 1;
}

static int parse_positive_int(const char *text, int *value)
{
    char *end = NULL;
    long parsed;

    while (isspace((unsigned char)*text)) {
        text++;
    }
    if (*text == '\0') {
        return 0;
    }

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || parsed < 1 || parsed > INT_MAX) {
        return 0;
    }

    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != '\0') {
        return 0;
    }

    *value = (int)parsed;
    return 1;
}

static int read_pages(int pages[], int *page_count)
{
    char line[8192];
    char *cursor;
    int count = 0;

    printf("Enter page-reference string (integers separated by spaces):\n> ");
    if (!read_line(line, sizeof line)) {
        return 0;
    }

    cursor = line;
    while (*cursor != '\0') {
        char *end = NULL;
        long value;

        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }

        errno = 0;
        value = strtol(cursor, &end, 10);
        if (errno == ERANGE || end == cursor || value < INT_MIN ||
            value > INT_MAX) {
            fprintf(stderr, "Invalid page reference. Use integer page numbers.\n");
            return 0;
        }
        if (*end != '\0' && !isspace((unsigned char)*end)) {
            fprintf(stderr, "Invalid character in page-reference string.\n");
            return 0;
        }
        if (count >= MAX_PAGES) {
            fprintf(stderr, "Too many page references. Maximum is %d.\n", MAX_PAGES);
            return 0;
        }

        pages[count++] = (int)value;
        cursor = end;
    }

    if (count == 0) {
        fprintf(stderr, "The page-reference string cannot be empty.\n");
        return 0;
    }

    *page_count = count;
    return 1;
}

static int read_frame_count(int *frame_count)
{
    char line[128];

    printf("Enter number of page frames (1-%d):\n> ", MAX_FRAMES);
    if (!read_line(line, sizeof line)) {
        return 0;
    }

    if (!parse_positive_int(line, frame_count) || *frame_count > MAX_FRAMES) {
        fprintf(stderr, "Invalid frame count. Enter a number from 1 to %d.\n",
                MAX_FRAMES);
        return 0;
    }
    return 1;
}

static int read_menu_choice(int *choice)
{
    char line[128];

    printf("\nSelect an option:\n");
    printf("1. FIFO\n");
    printf("2. LRU\n");
    printf("3. Optimal\n");
    printf("4. Compare all algorithms\n");
    printf("5. Exit\n");
    printf("Choice: ");

    if (!read_line(line, sizeof line)) {
        return 0;
    }
    if (!parse_positive_int(line, choice) || *choice < 1 || *choice > 5) {
        fprintf(stderr, "Invalid choice. Select an option from 1 to 5.\n");
        return -1;
    }
    return 1;
}

int main(void)
{
    int pages[MAX_PAGES];
    int page_count = 0;
    int frame_count = 0;
    int choice;
    int input_status;
    Stats results[3];

    printf("========================================\n");
    printf("     PAGE REPLACEMENT SIMULATOR\n");
    printf("========================================\n\n");

    if (!read_pages(pages, &page_count) ||
        !read_frame_count(&frame_count)) {
        return EXIT_FAILURE;
    }

    for (;;) {
        input_status = read_menu_choice(&choice);
        if (input_status == 0) {
            break;
        }
        if (input_status < 0) {
            continue;
        }
        if (choice == 5) {
            printf("Exiting Page Replacement Simulator. Goodbye!\n");
            break;
        }

        switch (choice) {
        case ALG_FIFO:
            results[0] = fifo(pages, page_count, frame_count);
            print_algorithm_result("FIFO", results[0]);
            break;
        case ALG_LRU:
            results[1] = lru(pages, page_count, frame_count);
            print_algorithm_result("LRU", results[1]);
            break;
        case ALG_OPTIMAL:
            results[2] = optimal(pages, page_count, frame_count);
            print_algorithm_result("Optimal", results[2]);
            break;
        case 4:
            results[0] = fifo(pages, page_count, frame_count);
            results[1] = lru(pages, page_count, frame_count);
            results[2] = optimal(pages, page_count, frame_count);
            print_summary(results, 3);
            break;
        default:
            /* read_menu_choice() validates this before reaching the switch. */
            break;
        }
    }

    return EXIT_SUCCESS;
}
