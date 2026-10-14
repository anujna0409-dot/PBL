
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

void fcfs(Process p[], int n, GanttChart *g);
void sjf(Process p[], int n, GanttChart *g);
void srtf(Process p[], int n, GanttChart *g);
void priority_np(Process p[], int n, GanttChart *g);
void priority_p(Process p[], int n, GanttChart *g);
void round_robin(Process p[], int n, int quantum,
                 GanttChart *g);

void calculate_metrics(Process p[], int n);
void gantt_display(const GanttChart *g);

#endif
