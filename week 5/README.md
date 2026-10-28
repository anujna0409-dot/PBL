# Page Replacement Algorithms in Operating Systems

A C simulator for FIFO, LRU, and Optimal page replacement.

## Team leader files

- `main.c`: reads and validates input, displays the menu, calls algorithms, and coordinates comparison.
- `page_replacement.h`: shared constants, statistics structure, and function declarations.

The other team members implement `fifo.c`, `lru.c`, and `optimal.c`.

## Shared interface contract

Each algorithm must implement the matching declaration in `page_replacement.h`:

```c
Stats fifo(const int pages[], int n, int frame_count);
Stats lru(const int pages[], int n, int frame_count);
Stats optimal(const int pages[], int n, int frame_count);
```

Each function should print its step-by-step frame table and return a `Stats` value:
- `hits`: number of page hits
- `faults`: number of page faults
- `references`: number of page references processed

Each algorithm should count every page reference as either a hit or a fault. Therefore, `hits + faults == references`.

The `print_algorithm_result()` and `print_summary()` functions are declared for use by the team. They can be implemented by the team leader in a shared utility file, or in `main.c` if the build command is updated accordingly.

## Build after all modules are supplied

```bash
gcc -Wall -Wextra -std=c11 main.c fifo.c lru.c optimal.c results.c -o page_replacement
```

This assumes the team creates `results.c` to implement the two result-printing functions. Alternatively, put those functions in one existing source file and adjust the build command.

## Sample input

```text
7 0 1 2 0 3 0 4 2 3
3
```

The reference string is entered first, then the frame count. Select option 4 to compare all algorithms.
