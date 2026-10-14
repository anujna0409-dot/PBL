/*
 * round_robin.c - Member 3: Round Robin scheduling
 *
 * Uses the shared scheduler.h. Fills, for every process: completion_time,
 * turnaround_time, waiting_time, response_time, and records the execution
 * timeline (CPU idle = pid -1) in the GanttChart. The input array is not
 * reordered. If the CPU runs the same process in back-to-back slices (e.g.
 * it is the only one left), those slices are merged into one Gantt segment.
 *
 * Queue rule: processes that arrive during a time slice, or exactly when it
 * ends, are placed in the ready queue BEFORE the preempted process is put
 * back at the tail. Arrival ties are broken by smaller pid.
 *
 * Self-test (needs gantt.c; supplies a stand-in calculate_metrics):
 *   gcc -Wall -Wextra -DRR_TEST -o rr round_robin.c gantt.c && ./rr
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

static void add_segment(GanttChart *g, int pid, int start, int end)
{
    if (start >= end || g->count >= MAX_SEGMENTS) return;
    if (g->count > 0 &&
        g->segments[g->count - 1].pid == pid &&
        g->segments[g->count - 1].end == start) {
        g->segments[g->count - 1].end = end;      /* merge contiguous slices */
        return;
    }
    g->segments[g->count].pid   = pid;
    g->segments[g->count].start = start;
    g->segments[g->count].end   = end;
    g->count++;
}

static int arrives_first(const Process p[], int a, int b)
{
    if (p[a].arrival_time != p[b].arrival_time)
        return p[a].arrival_time < p[b].arrival_time;
    if (p[a].pid != p[b].pid)
        return p[a].pid < p[b].pid;
    return a < b;
}

void round_robin(Process p[], int n, int quantum, GanttChart *g)
{
    int order[MAX_PROCESSES];            /* indices sorted by arrival      */
    int rem[MAX_PROCESSES], first[MAX_PROCESSES];
    int queue[MAX_PROCESSES + 1];        /* circular ready queue           */
    int head = 0, tail = 0;              /* queue holds at most n entries  */
    int next = 0;                        /* next index in order[] to admit */
    int time = 0, completed = 0;

    g->count = 0;
    if (n <= 0 || n > MAX_PROCESSES) return;
    if (quantum <= 0) {
        fprintf(stderr, "round_robin: time quantum must be > 0\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        order[i] = i;
        rem[i] = p[i].burst_time;
        first[i] = -1;
    }
    for (int i = 1; i < n; i++) {        /* stable insertion sort by arrival */
        int key = order[i], j = i - 1;
        while (j >= 0 && arrives_first(p, key, order[j])) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }

    /* zero-length bursts finish the moment they arrive */
    for (int i = 0; i < n; i++) {
        if (rem[i] <= 0) {
            p[i].completion_time = p[i].arrival_time;
            p[i].turnaround_time = 0;
            p[i].waiting_time    = 0;
            p[i].response_time   = 0;
            completed++;
        }
    }

    while (completed < n) {
        /* admit everyone who has arrived by now */
        while (next < n && p[order[next]].arrival_time <= time) {
            int i = order[next++];
            if (rem[i] > 0) { queue[tail] = i; tail = (tail + 1) % (MAX_PROCESSES + 1); }
        }

        if (head == tail) {              /* ready queue empty -> CPU idle */
            while (next < n && rem[order[next]] <= 0) next++;
            if (next >= n) break;
            int t = p[order[next]].arrival_time;
            add_segment(g, -1, time, t);
            time = t;
            continue;
        }

        int cur = queue[head];
        head = (head + 1) % (MAX_PROCESSES + 1);

        int slice = rem[cur] < quantum ? rem[cur] : quantum;
        if (first[cur] == -1) first[cur] = time;
        add_segment(g, p[cur].pid, time, time + slice);
        time += slice;
        rem[cur] -= slice;

        /* arrivals during the slice (or exactly at its end) go first */
        while (next < n && p[order[next]].arrival_time <= time) {
            int i = order[next++];
            if (rem[i] > 0) { queue[tail] = i; tail = (tail + 1) % (MAX_PROCESSES + 1); }
        }

        if (rem[cur] > 0) {              /* preempted: back to the tail */
            queue[tail] = cur;
            tail = (tail + 1) % (MAX_PROCESSES + 1);
        } else {
            p[cur].completion_time = time;
            p[cur].turnaround_time = time - p[cur].arrival_time;
            p[cur].waiting_time    = p[cur].turnaround_time - p[cur].burst_time;
            p[cur].response_time   = first[cur] - p[cur].arrival_time;
            completed++;
        }
    }
}

/* ---------- self-test only ---------- */
#ifdef RR_TEST

void calculate_metrics(Process p[], int n)   /* stand-in for shared version */
{
    double sw = 0, st = 0, sr = 0;
    printf("%-5s %-8s %-6s %-11s %-5s %-5s %-5s\n",
           "PID", "Arrival", "Burst", "Completion", "WT", "TAT", "RT");
    for (int i = 0; i < n; i++) {
        printf("P%-4d %-8d %-6d %-11d %-5d %-5d %-5d\n", p[i].pid,
               p[i].arrival_time, p[i].burst_time, p[i].completion_time,
               p[i].waiting_time, p[i].turnaround_time, p[i].response_time);
        sw += p[i].waiting_time; st += p[i].turnaround_time;
        sr += p[i].response_time;
    }
    printf("Avg WT = %.2f   Avg TAT = %.2f   Avg RT = %.2f\n",
           sw / n, st / n, sr / n);
}

typedef struct { int arrival, burst; } Row;

static void run(const char *name, const Row r[], int n, int q)
{
    static GanttChart g;
    Process p[MAX_PROCESSES];
    memset(p, 0, sizeof p);
    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        p[i].arrival_time = r[i].arrival;
        p[i].burst_time = r[i].burst;
    }
    printf("\n######## %s  (quantum = %d) ########\n", name, q);
    round_robin(p, n, q, &g);
    gantt_display(&g);
    calculate_metrics(p, n);
}

int main(void)
{
    /* 1: classic, all arrive at 0 (q=4 -> avg WT 5.67) */
    const Row t1[] = {{0,24},{0,3},{0,3}};
    run("TEST 1: same arrival", t1, 3, 4);

    /* 2: arrivals during slices, several quantum values */
    const Row t2[] = {{0,5},{1,3},{2,1},{3,2},{4,3}};
    run("TEST 2: arrivals mid-slice", t2, 5, 1);
    run("TEST 2: arrivals mid-slice", t2, 5, 2);
    run("TEST 2: arrivals mid-slice", t2, 5, 4);
    run("TEST 2: arrivals mid-slice", t2, 5, 100);   /* behaves like FCFS */

    /* 3: idle periods, unsorted input, arrival exactly at slice end */
    const Row t3[] = {{12,2},{3,4},{20,3},{3,1},{9,5}};
    run("TEST 3: idle periods", t3, 5, 3);

    return 0;
}
#endif
