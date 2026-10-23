#include <stdio.h>
#include "scheduler.h"

int input_processes(Process p[], int n)
{
    if (p == NULL || n < 1 || n > MAX_PROCESSES) {
        printf("Invalid number of processes.\n");
        return 0;
    }

    for (int i = 0; i < n; i++) {
        printf("\nEnter details for Process %d\n", i + 1);

        printf("PID (unique integer): ");
        if (scanf("%d", &p[i].pid) != 1) {
            printf("Invalid PID.\n");
            return 0;
        }
        for (int j = 0; j < i; j++) {
            if (p[j].pid == p[i].pid) {
                printf("PIDs must be unique.\n");
                return 0;
            }
        }

        printf("Arrival time: ");
        if (scanf("%d", &p[i].arrival_time) != 1 ||
            p[i].arrival_time < 0) {
            printf("Arrival time must be a non-negative integer.\n");
            return 0;
        }

        printf("Burst time: ");
        if (scanf("%d", &p[i].burst_time) != 1 ||
            p[i].burst_time < 0) {
            printf("Burst time must be a non-negative integer.\n");
            return 0;
        }

        printf("Priority (smaller number = higher priority): ");
        if (scanf("%d", &p[i].priority) != 1) {
            printf("Invalid priority.\n");
            return 0;
        }

        p[i].completion_time = 0;
        p[i].waiting_time = 0;
        p[i].turnaround_time = 0;
        p[i].response_time = 0;
    }

    return 1;
}

static void reset_metrics(Process p[], int n)
{
    for (int i = 0; i < n; i++) {
        p[i].completion_time = 0;
        p[i].waiting_time = 0;
        p[i].turnaround_time = 0;
        p[i].response_time = 0;
    }
}

static void display_cpu_statistics(const GanttChart *g)
{
    int busy_time = 0;
    int idle_time = 0;
    int total_time = 0;

    for (int i = 0; i < g->count; i++) {
        int duration = g->segments[i].end - g->segments[i].start;
        if (g->segments[i].pid == -1) {
            idle_time += duration;
        } else {
            busy_time += duration;
        }
    }

    if (g->count > 0) {
        total_time = g->segments[g->count - 1].end - g->segments[0].start;
    }

    printf("\n========== CPU STATISTICS ==========\n");
    printf("CPU Busy Time: %d\n", busy_time);
    printf("CPU Idle Time: %d\n", idle_time);
    if (total_time > 0) {
        printf("CPU Utilization: %.2f%%\n", busy_time * 100.0 / total_time);
    } else {
        printf("CPU Utilization: 0.00%%\n");
    }
}

int main(void)
{
    Process p[MAX_PROCESSES];
    GanttChart g;
    int n, choice, quantum = 2;

    printf("\n====================================\n");
    printf("      CPU SCHEDULING SIMULATOR\n");
    printf("====================================\n");
    printf("Enter the number of processes (1-%d): ", MAX_PROCESSES);

    if (scanf("%d", &n) != 1 || n < 1 || n > MAX_PROCESSES) {
        printf("Invalid number of processes.\n");
        return 1;
    }
    if (!input_processes(p, n)) {
        return 1;
    }

    do {
        printf("\n========== SCHEDULING MENU ==========\n");
        printf("1. First Come First Served (FCFS)\n");
        printf("2. Shortest Job First (Non-preemptive)\n");
        printf("3. Shortest Remaining Time First (Preemptive SJF)\n");
        printf("4. Priority (Non-preemptive)\n");
        printf("5. Priority (Preemptive)\n");
        printf("6. Round Robin\n");
        printf("7. Compare all algorithms\n");
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
        if (choice < 1 || choice > 7) {
            printf("Invalid choice. Select 0 to 7.\n");
            continue;
        }

        if (choice == 6 || choice == 7) {
            printf("Enter time quantum for Round Robin: ");
            if (scanf("%d", &quantum) != 1 || quantum <= 0) {
                printf("Time quantum must be a positive integer.\n");
                return 1;
            }
        }

        if (choice == 7) {
            compare_algorithms(p, n, quantum);
            continue;
        }

        reset_metrics(p, n);
        g.count = 0;
        switch (choice) {
            case 1: fcfs(p, n, &g); break;
            case 2: sjf(p, n, &g); break;
            case 3: srtf(p, n, &g); break;
            case 4: priority_np(p, n, &g); break;
            case 5: priority_p(p, n, &g); break;
            case 6: round_robin(p, n, quantum, &g); break;
            default: break;
        }

        calculate_metrics(p, n);
        printf("\n========== GANTT CHART ==========\n");
        gantt_display(&g);
        display_cpu_statistics(&g);
        display_results(p, n);
    } while (choice != 0);

    return 0;
}
