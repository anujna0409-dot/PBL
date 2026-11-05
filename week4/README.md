# Week 4 – Interactive Deadlock Simulation System

## Project Overview
The Interactive Deadlock Simulation System is a C-based project designed to demonstrate deadlock handling in operating systems. It provides an interactive interface to simulate resource allocation, detect deadlocks, check system safety, and recover from deadlocked states.

## Features
- Resource allocation and deallocation
- Banker's Algorithm for deadlock avoidance
- Deadlock detection
- Deadlock recovery by terminating a process
- Resource Allocation Graph (RAG) visualization
- Interactive web-based interface
- Safe sequence identification
- Handling of pending resource requests

## Technologies Used
- C
- HTML
- CSS
- JavaScript

## Project Files
| File | Description |
|---|---|
| `main.c` | Main server-side program |
| `banker.c`, `banker.h` | Banker's Algorithm and resource management |
| `detect.c`, `detect.h` | Deadlock detection |
| `rag.c`, `rag.h` | Resource Allocation Graph functionality |
| `common.h` | Shared data structures and definitions |
| `index.html` | Web interface |
| `style.css` | Interface styling |
| `app.js` | Frontend interaction |
| `rag.js` | Resource Allocation Graph visualization |
| `api.md` | API documentation |

## Compilation and Execution

Open a terminal in the Week 4 project folder and run:

```bash
gcc -Wall -Wextra -std=c11 -o server main.c banker.c detect.c rag.c
```

Then start the server:

```bash
./server
```

Open the following address in your browser:

```text
http://localhost:8080
```

## Algorithms and Concepts
1. Banker's Algorithm – checks whether resource requests keep the system in a safe state.
2. Deadlock Detection – identifies processes involved in a deadlock.
3. Deadlock Recovery – terminates a selected process and releases its allocated resources.
4. Resource Allocation Graph – represents processes, resources, and their relationships.

## Conclusion
This project demonstrates important operating-system concepts related to resource allocation, deadlock avoidance, detection, and recovery through an interactive simulation.
