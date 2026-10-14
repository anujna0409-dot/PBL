/*
 * gantt.c - Member 3: Gantt chart display
 *
 * void gantt_display(const GanttChart *g)
 * Prints the timeline as boxes with the time boundaries underneath, e.g.
 *
 *   +------+------+--------+------+
 *   |  P1  |  P2  |  IDLE  |  P3  |
 *   +------+------+--------+------+
 *   0      4      7        10     12
 *
 * pid -1 is shown as IDLE. Long charts wrap onto several rows so they fit
 * a normal terminal (GANTT_ROW_WIDTH columns).
 */
#include <stdio.h>
#include <string.h>
#include "scheduler.h"

#define GANTT_ROW_WIDTH 76

static int digits(int v)
{
    char b[16];
    return (int)snprintf(b, sizeof b, "%d", v);
}

/* label for a segment; returns its length */
static int label_of(const GanttSegment *s, char *buf, size_t sz)
{
    if (s->pid == -1) return (int)snprintf(buf, sz, "IDLE");
    return (int)snprintf(buf, sz, "P%d", s->pid);
}

/* inner width (between the bars) of the box for segment s */
static int cell_width(const GanttSegment *s)
{
    char lab[24];
    int w = label_of(s, lab, sizeof lab) + 4;           /* 2 spaces each side */
    int d = digits(s->start) + 1;
    if (d > w) w = d;
    d = digits(s->end) + 1;
    if (d > w) w = d;
    return w;
}

static void put_text(char *line, int col, const char *s)
{
    memcpy(line + col, s, strlen(s));
}

void gantt_display(const GanttChart *g)
{
    if (g == NULL || g->count <= 0) {
        printf("Gantt Chart: (empty)\n");
        return;
    }

    printf("\nGantt Chart:\n");

    int i = 0;
    while (i < g->count) {
        /* choose how many segments fit on this row */
        int j = i, width = 1;
        while (j < g->count) {
            int w = cell_width(&g->segments[j]) + 1;
            if (j > i && width + w > GANTT_ROW_WIDTH) break;
            width += w;
            j++;
        }

        char border[GANTT_ROW_WIDTH + 64], cells[GANTT_ROW_WIDTH + 64], times[GANTT_ROW_WIDTH + 64];
        memset(border, ' ', sizeof border);
        memset(cells,  ' ', sizeof cells);
        memset(times,  ' ', sizeof times);

        int col = 0;
        char tmp[24];
        border[col] = '+';
        cells[col]  = '|';
        snprintf(tmp, sizeof tmp, "%d", g->segments[i].start);
        put_text(times, col, tmp);

        for (int k = i; k < j; k++) {
            int w = cell_width(&g->segments[k]);
            char lab[24];
            int len = label_of(&g->segments[k], lab, sizeof lab);

            for (int c = 1; c <= w; c++) border[col + c] = '-';
            int pad = (w - len) / 2;
            put_text(cells, col + 1 + pad, lab);

            col += w + 1;
            border[col] = '+';
            cells[col]  = '|';
            snprintf(tmp, sizeof tmp, "%d", g->segments[k].end);
            put_text(times, col, tmp);
        }

        border[col + 1] = cells[col + 1] = '\0';
        times[col + 1 + 12] = '\0';
        /* trim trailing spaces on the time line */
        for (int t = col + 12; t >= 0 && times[t] == ' '; t--) times[t] = '\0';

        printf("%s\n%s\n%s\n%s\n", border, cells, border, times);
        i = j;
        if (i < g->count) printf("\n");
    }
}
