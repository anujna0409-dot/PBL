#include <stdio.h>
#include "scheduler.h"

/*
 * The scheduling algorithms calculate completion time and response time.
 * Recalculate the dependent metrics here so all algorithms use one formula.
 */
void calculate_metrics(Process p[], int n)
{
    if (p == NULL || n <= 0 || n > MAX_PROCESSES) {
        return;
    }

    for (int i = 0; i < n; i++) {
        /* A zero-burst process completes at its arrival and uses no CPU. */
        if (p[i].burst_time == 0) {
            p[i].completion_time = p[i].arrival_time;
            p[i].turnaround_time = 0;
            p[i].waiting_time = 0;
            p[i].response_time = 0;
            continue;
        }

        p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
        p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;

        /* Avoid presenting impossible negative values if an algorithm has a bug. */
        if (p[i].turnaround_time < 0) {
            p[i].turnaround_time = 0;
        }
        if (p[i].waiting_time < 0) {
            p[i].waiting_time = 0;
        }
    }
}

void display_results(const Process p[], int n)
{
    double total_wt = 0.0;
    double total_tat = 0.0;
    double total_rt = 0.0;

    if (p == NULL || n <= 0 || n > MAX_PROCESSES) {
        printf("No process results to display.\n");
        return;
    }

    printf("\n========== PROCESS METRICS ==========\n");
    printf("%-8s %-8s %-8s %-8s %-8s\n",
           "PID", "CT", "TAT", "WT", "RT");

    for (int i = 0; i < n; i++) {
        printf("%-8d %-8d %-8d %-8d %-8d\n",
               p[i].pid,
               p[i].completion_time,
               p[i].turnaround_time,
               p[i].waiting_time,
               p[i].response_time);

        total_wt += p[i].waiting_time;
        total_tat += p[i].turnaround_time;
        total_rt += p[i].response_time;
    }

    printf("\nAverage Waiting Time: %.2f\n", total_wt / n);
    printf("Average Turnaround Time: %.2f\n", total_tat / n);
    printf("Average Response Time: %.2f\n", total_rt / n);
}

#include <string.h>

typedef void (*SchedulingFunction)(Process [], int, GanttChart *);

static void print_comparison_row(const char *name, const Process original[],
                                 int n, SchedulingFunction schedule)
{
    Process work[MAX_PROCESSES];
    GanttChart chart;
    double total_wt = 0.0;
    double total_tat = 0.0;
    double total_rt = 0.0;

    memcpy(work, original, (size_t)n * sizeof work[0]);
    for (int i = 0; i < n; i++) {
        work[i].completion_time = 0;
        work[i].waiting_time = 0;
        work[i].turnaround_time = 0;
        work[i].response_time = 0;
    }

    chart.count = 0;
    schedule(work, n, &chart);
    calculate_metrics(work, n);

    for (int i = 0; i < n; i++) {
        total_wt += work[i].waiting_time;
        total_tat += work[i].turnaround_time;
        total_rt += work[i].response_time;
    }

    printf("%-28s %-12.2f %-12.2f %-12.2f\n",
           name, total_wt / n, total_tat / n, total_rt / n);
}

static void run_round_robin_comparison(const Process original[], int n,
                                       int quantum)
{
    Process work[MAX_PROCESSES];
    GanttChart chart;
    double total_wt = 0.0;
    double total_tat = 0.0;
    double total_rt = 0.0;

    memcpy(work, original, (size_t)n * sizeof work[0]);
    for (int i = 0; i < n; i++) {
        work[i].completion_time = 0;
        work[i].waiting_time = 0;
        work[i].turnaround_time = 0;
        work[i].response_time = 0;
    }

    chart.count = 0;
    round_robin(work, n, quantum, &chart);
    calculate_metrics(work, n);

    for (int i = 0; i < n; i++) {
        total_wt += work[i].waiting_time;
        total_tat += work[i].turnaround_time;
        total_rt += work[i].response_time;
    }

    printf("%-28s %-12.2f %-12.2f %-12.2f\n",
           "Round Robin", total_wt / n, total_tat / n, total_rt / n);
}

void compare_algorithms(const Process p[], int n, int quantum)
{
    if (p == NULL || n <= 0 || n > MAX_PROCESSES || quantum <= 0) {
        printf("Cannot compare algorithms: invalid input or quantum.\n");
        return;
    }

    printf("\n========== ALGORITHM COMPARISON ==========\n");
    printf("%-28s %-12s %-12s %-12s\n",
           "Algorithm", "Average WT", "Average TAT", "Average RT");
    printf("--------------------------------------------------------------------------\n");
    print_comparison_row("FCFS", p, n, fcfs);
    print_comparison_row("SJF (Non-preemptive)", p, n, sjf);
    print_comparison_row("SRTF (Preemptive SJF)", p, n, srtf);
    print_comparison_row("Priority (Non-preemptive)", p, n, priority_np);
    print_comparison_row("Priority (Preemptive)", p, n, priority_p);
    run_round_robin_comparison(p, n, quantum);
    printf("\nRound Robin time quantum: %d\n", quantum);
    printf("Note: smaller priority numbers represent higher priority.\n");
}
