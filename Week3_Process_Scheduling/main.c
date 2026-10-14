#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

int main(void)
{
    Process p[MAX_PROCESSES];
    GanttChart g;
    int n, choice, quantum;
    int i;

    printf("\n====================================\n");
    printf("      CPU SCHEDULING SIMULATOR\n");
    printf("====================================\n");

    printf("Enter the number of processes (1-%d): ",
           MAX_PROCESSES);

    if (scanf("%d", &n) != 1 ||
        n < 1 || n > MAX_PROCESSES) {
        printf("Invalid number of processes.\n");
        return 1;
    }

    for (i = 0; i < n; i++) {
        printf("\nEnter details for Process %d\n", i + 1);

        printf("PID: ");
        if (scanf("%d", &p[i].pid) != 1) {
            printf("Invalid PID.\n");
            return 1;
        }

        printf("Arrival time: ");
        if (scanf("%d", &p[i].arrival_time) != 1 ||
            p[i].arrival_time < 0) {
            printf("Invalid arrival time.\n");
            return 1;
        }

        printf("Burst time: ");
        if (scanf("%d", &p[i].burst_time) != 1 ||
            p[i].burst_time < 0) {
            printf("Invalid burst time.\n");
            return 1;
        }

        printf("Priority (smaller number = higher priority): ");
        if (scanf("%d", &p[i].priority) != 1) {
            printf("Invalid priority.\n");
            return 1;
        }

        p[i].completion_time = 0;
        p[i].waiting_time = 0;
        p[i].turnaround_time = 0;
        p[i].response_time = 0;
    }

    do {
        printf("\n========== SCHEDULING MENU ==========\n");
        printf("1. First Come First Served (FCFS)\n");
        printf("2. Shortest Job First (Non-preemptive)\n");
        printf("3. Shortest Remaining Time First (Preemptive SJF)\n");
        printf("4. Priority (Non-preemptive)\n");
        printf("5. Priority (Preemptive)\n");
        printf("6. Round Robin\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid menu input.\n");
            return 1;
        }

        if (choice == 0) {
            printf("Exiting CPU Scheduling Simulator.\n");
            break;
        }

        if (choice < 1 || choice > 6) {
            printf("Invalid choice. Select 0 to 6.\n");
            continue;
        }

        if (choice == 6) {
            printf("Enter time quantum: ");

            if (scanf("%d", &quantum) != 1 || quantum <= 0) {
                printf("Time quantum must be a positive integer.\n");
                return 1;
            }
        }

        for (i = 0; i < n; i++) {
            p[i].completion_time = 0;
            p[i].waiting_time = 0;
            p[i].turnaround_time = 0;
            p[i].response_time = 0;
        }

        g.count = 0;

        switch (choice) {
            case 1:
                fcfs(p, n, &g);
                break;
            case 2:
                sjf(p, n, &g);
                break;
            case 3:
                srtf(p, n, &g);
                break;
            case 4:
                priority_np(p, n, &g);
                break;
            case 5:
                priority_p(p, n, &g);
                break;
            case 6:
                round_robin(p, n, quantum, &g);
                break;
        }

        printf("\n========== GANTT CHART ==========\n");
        gantt_display(&g);

        calculate_metrics(p, n);

        printf("\n========== PROCESS METRICS ==========\n");
        printf("%-8s %-8s %-8s %-8s %-8s\n",
               "PID", "CT", "TAT", "WT", "RT");

        {
            double total_wt = 0.0;
            double total_tat = 0.0;

            for (i = 0; i < n; i++) {
                printf("%-8d %-8d %-8d %-8d %-8d\n",
                       p[i].pid,
                       p[i].completion_time,
                       p[i].turnaround_time,
                       p[i].waiting_time,
                       p[i].response_time);

                total_wt += p[i].waiting_time;
                total_tat += p[i].turnaround_time;
            }

            printf("\nAverage Waiting Time: %.2f\n",
                   total_wt / n);
            printf("Average Turnaround Time: %.2f\n",
                   total_tat / n);
        }

    } while (choice != 0);

    return 0;
}
