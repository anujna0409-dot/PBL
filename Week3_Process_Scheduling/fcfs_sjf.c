/*
 * fcfs_sjf.c - Member 1: FCFS and non-preemptive SJF
 *
 * Uses the shared scheduler.h. Each function fills, for every process:
 * completion_time, turnaround_time, waiting_time, response_time,
 * and records the execution timeline (including CPU idle periods,
 * pid = -1) in the GanttChart. Input arrays are not reordered.
 *
 * Self-test (supplies its own stand-ins for the shared
 * calculate_metrics / gantt_display so it can run alone):
 *   gcc -Wall -Wextra -DFCFS_SJF_TEST -o fcfs_sjf fcfs_sjf.c && ./fcfs_sjf
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

/* ---------- helpers ---------- */

static void add_segment(GanttChart *g, int pid, int start, int end)
{
    if (start >= end || g->count >= MAX_SEGMENTS) return;
    /* merge back-to-back idle blocks */
    if (pid == -1 && g->count > 0 &&
        g->segments[g->count - 1].pid == -1 &&
        g->segments[g->count - 1].end == start) {
        g->segments[g->count - 1].end = end;
        return;
    }
    g->segments[g->count].pid   = pid;
    g->segments[g->count].start = start;
    g->segments[g->count].end   = end;
    g->count++;
}

/* Record the results for one process that ran from 'start' to 'end'. */
static void finish_process(Process *pr, int start, int end)
{
    pr->completion_time = end;
    pr->turnaround_time = end - pr->arrival_time;
    pr->waiting_time    = pr->turnaround_time - pr->burst_time;
    pr->response_time   = start - pr->arrival_time;
}

/* Order by arrival time; ties -> smaller pid, then input position. */
static int earlier(const Process p[], int a, int b)
{
    if (p[a].arrival_time != p[b].arrival_time)
        return p[a].arrival_time < p[b].arrival_time;
    if (p[a].pid != p[b].pid)
        return p[a].pid < p[b].pid;
    return a < b;
}

/* ---------- FCFS ---------- */

void fcfs(Process p[], int n, GanttChart *g)
{
    int order[MAX_PROCESSES];
    int time = 0;

    g->count = 0;
    if (n <= 0 || n > MAX_PROCESSES) return;

    for (int i = 0; i < n; i++) order[i] = i;

    /* insertion sort of indices by arrival (stable) */
    for (int i = 1; i < n; i++) {
        int key = order[i], j = i - 1;
        while (j >= 0 && earlier(p, key, order[j])) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }

    for (int k = 0; k < n; k++) {
        Process *cur = &p[order[k]];
        if (time < cur->arrival_time) {            /* CPU idle */
            add_segment(g, -1, time, cur->arrival_time);
            time = cur->arrival_time;
        }
        add_segment(g, cur->pid, time, time + cur->burst_time);
        finish_process(cur, time, time + cur->burst_time);
        time += cur->burst_time;
    }
}

/* ---------- Non-preemptive SJF ---------- */

void sjf(Process p[], int n, GanttChart *g)
{
    char done[MAX_PROCESSES] = {0};
    int time = 0, completed = 0;

    g->count = 0;
    if (n <= 0 || n > MAX_PROCESSES) return;

    while (completed < n) {
        /* shortest burst among arrived, unfinished processes;
           ties -> earlier arrival, then smaller pid */
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (done[i] || p[i].arrival_time > time) continue;
            if (best == -1 ||
                p[i].burst_time < p[best].burst_time ||
                (p[i].burst_time == p[best].burst_time && earlier(p, i, best)))
                best = i;
        }

        if (best == -1) {                          /* nothing ready: idle */
            int next = -1;
            for (int i = 0; i < n; i++)
                if (!done[i] && (next == -1 || p[i].arrival_time < next))
                    next = p[i].arrival_time;
            add_segment(g, -1, time, next);
            time = next;
            continue;
        }

        add_segment(g, p[best].pid, time, time + p[best].burst_time);
        finish_process(&p[best], time, time + p[best].burst_time);
        time += p[best].burst_time;
        done[best] = 1;
        completed++;
    }
}

/* ---------- self-test only ---------- */
#ifdef FCFS_SJF_TEST

void calculate_metrics(Process p[], int n)   /* stand-in for shared version */
{
    double sw = 0, st = 0, sr = 0;
    printf("\n%-5s %-8s %-6s %-11s %-6s %-6s %-6s\n",
           "PID", "Arrival", "Burst", "Completion", "WT", "TAT", "RT");
    for (int i = 0; i < n; i++) {
        printf("P%-4d %-8d %-6d %-11d %-6d %-6d %-6d\n", p[i].pid,
               p[i].arrival_time, p[i].burst_time, p[i].completion_time,
               p[i].waiting_time, p[i].turnaround_time, p[i].response_time);
        sw += p[i].waiting_time; st += p[i].turnaround_time;
        sr += p[i].response_time;
    }
    printf("Avg WT = %.2f   Avg TAT = %.2f   Avg RT = %.2f\n",
           sw / n, st / n, sr / n);
}

void gantt_display(const GanttChart *g)      /* stand-in for shared version */
{
    printf("Gantt: ");
    for (int i = 0; i < g->count; i++) {
        if (g->segments[i].pid == -1) printf("[%d IDLE %d] ", g->segments[i].start, g->segments[i].end);
        else printf("[%d P%d %d] ", g->segments[i].start, g->segments[i].pid, g->segments[i].end);
    }
    printf("\n");
}

static void load(Process p[], const int d[][2], int n)
{
    memset(p, 0, n * sizeof *p);
    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        p[i].arrival_time = d[i][0];
        p[i].burst_time = d[i][1];
    }
}

static void run(const char *name, const int d[][2], int n)
{
    static GanttChart g;
    Process p[MAX_PROCESSES];

    printf("\n######## %s ########\n--- FCFS ---\n", name);
    load(p, d, n); fcfs(p, n, &g); gantt_display(&g); calculate_metrics(p, n);
    printf("\n--- SJF (non-preemptive) ---\n");
    load(p, d, n); sjf(p, n, &g);  gantt_display(&g); calculate_metrics(p, n);
}

int main(void)
{
    const int t1[][2] = {{0,6},{0,8},{0,7},{0,3}};
    const int t2[][2] = {{0,7},{2,4},{4,1},{5,4}};
    const int t3[][2] = {{12,2},{3,4},{20,3},{3,1},{9,5}};
    run("TEST 1: same arrival time", t1, 4);
    run("TEST 2: different arrival times", t2, 4);
    run("TEST 3: CPU idle periods", t3, 5);
    return 0;
}
#endif
