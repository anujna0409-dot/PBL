/*
 * srtf_priority.c - Member 2: SRTF, Non-preemptive Priority, Preemptive Priority
 *
 * Uses the shared scheduler.h. Each function fills, for every process:
 * completion_time, turnaround_time, waiting_time, response_time,
 * and records the timeline (CPU idle = pid -1) in the GanttChart.
 * Input arrays are not reordered.
 *
 * PRIORITY CONVENTION: a LOWER number means HIGHER priority.
 * To flip it, change the one macro below.
 *
 * Tie-breaking
 *   SRTF / preemptive priority : equal key -> keep the running process
 *                                (no needless preemption), then earlier
 *                                arrival, then smaller pid.
 *   Non-preemptive priority    : equal priority -> earlier arrival, then
 *                                smaller pid.
 *
 * Self-test (supplies stand-ins for the shared calculate_metrics /
 * gantt_display so it can run alone):
 *   gcc -Wall -Wextra -DSRTF_PRIORITY_TEST -o srtf_priority srtf_priority.c && ./srtf_priority
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

#define PRIORITY_BEATS(a, b) ((a) < (b))   /* lower number = higher priority */

enum { MODE_SRTF, MODE_PRIORITY };

/* ---------- helpers ---------- */

static void add_segment(GanttChart *g, int pid, int start, int end)
{
    if (start >= end || g->count >= MAX_SEGMENTS) return;
    /* merge contiguous blocks of the same pid (including idle) */
    if (g->count > 0 &&
        g->segments[g->count - 1].pid == pid &&
        g->segments[g->count - 1].end == start) {
        g->segments[g->count - 1].end = end;
        return;
    }
    g->segments[g->count].pid   = pid;
    g->segments[g->count].start = start;
    g->segments[g->count].end   = end;
    g->count++;
}

static void finish_process(Process *pr, int first_start, int end)
{
    pr->completion_time = end;
    pr->turnaround_time = end - pr->arrival_time;
    pr->waiting_time    = pr->turnaround_time - pr->burst_time;
    pr->response_time   = first_start - pr->arrival_time;
}

/* earlier arrival, then smaller pid, then input position */
static int arrives_first(const Process p[], int a, int b)
{
    if (p[a].arrival_time != p[b].arrival_time)
        return p[a].arrival_time < p[b].arrival_time;
    if (p[a].pid != p[b].pid)
        return p[a].pid < p[b].pid;
    return a < b;
}

/* Scheduling key for the chosen algorithm (smaller = better, except that
   priority uses PRIORITY_BEATS). Returns 1 if i strictly beats j. */
static int beats(const Process p[], const int rem[], int mode, int i, int j)
{
    int ki = (mode == MODE_SRTF) ? rem[i] : p[i].priority;
    int kj = (mode == MODE_SRTF) ? rem[j] : p[j].priority;
    if (ki != kj)
        return (mode == MODE_SRTF) ? (ki < kj) : PRIORITY_BEATS(ki, kj);
    return arrives_first(p, i, j);
}

static int same_key(const Process p[], const int rem[], int mode, int i, int j)
{
    return (mode == MODE_SRTF) ? rem[i] == rem[j]
                               : p[i].priority == p[j].priority;
}

/* ---------- shared preemptive engine ---------- */

static void preemptive(Process p[], int n, GanttChart *g, int mode)
{
    int rem[MAX_PROCESSES], first[MAX_PROCESSES];
    int time = 0, completed = 0, running = -1;

    g->count = 0;
    if (n <= 0 || n > MAX_PROCESSES) return;

    for (int i = 0; i < n; i++) {
        rem[i] = p[i].burst_time;
        first[i] = -1;
    }
    /* zero-length bursts finish the moment they arrive */
    for (int i = 0; i < n; i++)
        if (rem[i] <= 0) { finish_process(&p[i], p[i].arrival_time, p[i].arrival_time); completed++; }

    while (completed < n) {
        /* pick the best ready process */
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (rem[i] <= 0 || p[i].arrival_time > time) continue;
            if (best == -1 || beats(p, rem, mode, i, best)) best = i;
        }
        /* tie with the one already running -> don't preempt */
        if (best != -1 && running != -1 && rem[running] > 0 &&
            running != best && same_key(p, rem, mode, running, best))
            best = running;

        if (best == -1) {                         /* CPU idle until next arrival */
            int next = -1;
            for (int i = 0; i < n; i++)
                if (rem[i] > 0 && (next == -1 || p[i].arrival_time < next))
                    next = p[i].arrival_time;
            add_segment(g, -1, time, next);
            time = next;
            running = -1;
            continue;
        }

        /* run until it finishes or a new process arrives (only arrivals
           can change the decision, since the running key never worsens) */
        int slice = rem[best];
        for (int i = 0; i < n; i++)
            if (rem[i] > 0 && p[i].arrival_time > time &&
                p[i].arrival_time - time < slice)
                slice = p[i].arrival_time - time;

        if (first[best] == -1) first[best] = time;
        add_segment(g, p[best].pid, time, time + slice);
        time += slice;
        rem[best] -= slice;
        running = best;

        if (rem[best] == 0) {
            finish_process(&p[best], first[best], time);
            completed++;
            running = -1;
        }
    }
}

/* ---------- public API ---------- */

void srtf(Process p[], int n, GanttChart *g)
{
    preemptive(p, n, g, MODE_SRTF);
}

void priority_p(Process p[], int n, GanttChart *g)
{
    preemptive(p, n, g, MODE_PRIORITY);
}

void priority_np(Process p[], int n, GanttChart *g)
{
    char done[MAX_PROCESSES] = {0};
    int time = 0, completed = 0;

    g->count = 0;
    if (n <= 0 || n > MAX_PROCESSES) return;

    while (completed < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (done[i] || p[i].arrival_time > time) continue;
            if (best == -1 ||
                PRIORITY_BEATS(p[i].priority, p[best].priority) ||
                (p[i].priority == p[best].priority && arrives_first(p, i, best)))
                best = i;
        }

        if (best == -1) {                         /* CPU idle */
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
#ifdef SRTF_PRIORITY_TEST

void calculate_metrics(Process p[], int n)   /* stand-in for shared version */
{
    double sw = 0, st = 0, sr = 0;
    printf("%-5s %-8s %-6s %-5s %-11s %-5s %-5s %-5s\n",
           "PID", "Arrival", "Burst", "Prio", "Completion", "WT", "TAT", "RT");
    for (int i = 0; i < n; i++) {
        printf("P%-4d %-8d %-6d %-5d %-11d %-5d %-5d %-5d\n", p[i].pid,
               p[i].arrival_time, p[i].burst_time, p[i].priority,
               p[i].completion_time, p[i].waiting_time,
               p[i].turnaround_time, p[i].response_time);
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
        if (g->segments[i].pid == -1)
            printf("[%d IDLE %d] ", g->segments[i].start, g->segments[i].end);
        else
            printf("[%d P%d %d] ", g->segments[i].start, g->segments[i].pid, g->segments[i].end);
    }
    printf("\n");
}

typedef struct { int arrival, burst, prio; } Row;

static void load(Process p[], const Row r[], int n)
{
    memset(p, 0, n * sizeof *p);
    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        p[i].arrival_time = r[i].arrival;
        p[i].burst_time = r[i].burst;
        p[i].priority = r[i].prio;
    }
}

static void run(const char *name, const Row r[], int n,
                int do_srtf, int do_pnp, int do_pp)
{
    static GanttChart g;
    Process p[MAX_PROCESSES];
    printf("\n######## %s ########\n", name);
    if (do_srtf) { printf("--- SRTF ---\n");                 load(p, r, n); srtf(p, n, &g);        gantt_display(&g); calculate_metrics(p, n); }
    if (do_pnp)  { printf("--- Priority (non-preemptive) ---\n"); load(p, r, n); priority_np(p, n, &g); gantt_display(&g); calculate_metrics(p, n); }
    if (do_pp)   { printf("--- Priority (preemptive) ---\n");     load(p, r, n); priority_p(p, n, &g);  gantt_display(&g); calculate_metrics(p, n); }
}

int main(void)
{
    /* 1: classic SRTF preemption case (expected avg WT 6.50) */
    const Row t1[] = {{0,8,3},{1,4,1},{2,9,4},{3,5,2}};
    run("TEST 1: SRTF preemption on arrival", t1, 4, 1, 0, 0);

    /* 2: priority selection, NP vs preemptive; high-priority job arrives mid-run */
    const Row t2[] = {{0,10,3},{1,5,1},{2,2,4},{3,4,2}};
    run("TEST 2: priority NP vs preemptive", t2, 4, 0, 1, 1);

    /* 3: ties (equal burst / equal priority) + idle gaps, unsorted input */
    const Row t3[] = {{10,3,2},{0,4,2},{0,4,2},{2,2,1},{10,3,2},{20,1,5}};
    run("TEST 3: ties and idle periods", t3, 6, 1, 1, 1);

    return 0;
}
#endif
