# Page Replacement Algorithms in Operating Systems

A C-based simulator that demonstrates three page replacement algorithms: FIFO (First-In, First-Out), LRU (Least Recently Used), and Optimal.

## Project Files

- `main.c`: Reads and validates input, displays the menu, and coordinates algorithm execution and comparison.
- `page_replacement.h`: Contains shared constants, the statistics structure, and function declarations.
- `fifo.c`: Implements FIFO page replacement.
- `lru.c`: Implements LRU page replacement.
- `optimal.c`: Implements Optimal page replacement.
- `results.c`: Implements result and comparison summaries.

## Features

- Accepts a page-reference string and frame count.
- Displays step-by-step frame tables for each algorithm.
- Counts page hits and page faults.
- Calculates hit and fault ratios.
- Compares all three algorithms.
- Identifies the algorithm with the fewest page faults for the given input.

## Requirements

- GCC compiler
- Ubuntu/Linux or Windows Subsystem for Linux (WSL)

## Compile

Run the following command from the project root directory:

```bash
gcc -Wall -Wextra -std=c11 "week 5/main.c" "week 5/fifo.c" "week 5/lru.c" "week 5/optimal.c" "week 5/results.c" -o "week 5/page_replacement"
```

## Run

```bash
"./week 5/page_replacement"
```

## How to Use

1. Enter the page-reference string as integers separated by spaces.
2. Enter the number of page frames (between 1 and 100).
3. Select an option from the menu:
   - `1`: FIFO
   - `2`: LRU
   - `3`: Optimal
   - `4`: Compare all algorithms
   - `5`: Exit

## Sample Input

Page-reference string:

```text
7 0 1 2 0 3 0 4 2 3
```

Number of frames:

```text
3
```

For this test input, the expected results are:

| Algorithm | Page Hits | Page Faults |
|---|---:|---:|
| FIFO | 1 | 9 |
| LRU | 2 | 8 |
| Optimal | 4 | 6 |

Optimal produces the fewest page faults for this example.

## Shared Interface

Each algorithm implements the corresponding function declared in `page_replacement.h`:

```c
Stats fifo(const int pages[], int n, int frame_count);
Stats lru(const int pages[], int n, int frame_count);
Stats optimal(const int pages[], int n, int frame_count);
```

Each function returns a `Stats` structure containing the number of hits, faults, and references processed. Every reference is counted as either a hit or a fault.

Therefore:

`hits + faults == references`