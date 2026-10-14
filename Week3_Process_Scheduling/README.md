# Week 3 - CPU Process Scheduling Simulator

## Source files

- `main.c` - reads process data and lets the user choose an algorithm.
- `scheduler.h` - shared process and Gantt-chart structures and function declarations.
- `fcfs_sjf.c` - FCFS and non-preemptive SJF.
- `srtf_priority.c` - SRTF, non-preemptive Priority, and preemptive Priority.
- `round_robin.c` - Round Robin with a configurable quantum.
- `gantt.c` - prints Gantt charts and CPU idle intervals.
- `metrics.c` - calculates turnaround time and waiting time from completion times; response time is recorded by each scheduling algorithm.

Priority convention: a smaller priority number means a higher priority.

## Build and run (Ubuntu / WSL terminal)

Run these commands from this folder:

```bash
gcc -Wall -Wextra -std=c11 -o scheduler main.c fcfs_sjf.c srtf_priority.c round_robin.c gantt.c metrics.c
./scheduler
```

The program asks for the process count and each process's PID, arrival time, burst time, and priority. It then presents a menu for all six algorithms. Round Robin additionally asks for a positive time quantum.

## Standalone algorithm self-tests

Each self-test has its own test-only `main()`. Do not define the test macros when building the integrated simulator.

```bash
gcc -Wall -Wextra -std=c11 -DFCFS_SJF_TEST -o test_fcfs fcfs_sjf.c && ./test_fcfs
gcc -Wall -Wextra -std=c11 -DRR_TEST -o test_rr round_robin.c gantt.c && ./test_rr
gcc -Wall -Wextra -std=c11 -DSRTF_PRIORITY_TEST -o test_priority srtf_priority.c && ./test_priority
```

## Notes

- Use unique integer PIDs.
- Arrival times and burst times must be non-negative.
- Zero-burst processes are treated as completing at arrival time and consume no CPU time.
- Gantt segments use PID `-1` to represent CPU idle time.
- This is an educational simulator; verify results against hand-calculated examples before using them in the final report.
