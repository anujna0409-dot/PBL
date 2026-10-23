#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_PROCESSES 100
#define MAX_SEGMENTS 10000

typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int priority;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int response_time;
} Process;

typedef struct {
    int pid;       /* -1 means CPU idle */
    int start;
    int end;
} GanttSegment;

typedef struct {
    GanttSegment segments[MAX_SEGMENTS];
    int count;
} GanttChart;

/* Scheduling algorithms */
void fcfs(Process p[], int n, GanttChart *g);
void sjf(Process p[], int n, GanttChart *g);
void srtf(Process p[], int n, GanttChart *g);
void priority_np(Process p[], int n, GanttChart *g);
void priority_p(Process p[], int n, GanttChart *g);
void round_robin(Process p[], int n, int quantum, GanttChart *g);

/* Shared input, metrics, display, and comparison functions */
int input_processes(Process p[], int n);
void calculate_metrics(Process p[], int n);
void display_results(const Process p[], int n);
void compare_algorithms(const Process p[], int n, int quantum);
void gantt_display(const GanttChart *g);

#endif
