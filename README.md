# PBL
Team number 10
# Week 4 — Multi-Process Simulator & IPC

## 1. Multi-Process Architecture

The simulator was redesigned as three independent processes:

```text
                         COMMANDS
                    +---------------+
                    |       UI      |
                    |    Process    |
                    +---------------+
                           |
                           | UI → Core
                           | POSIX pipe
                           v
                    +---------------+
                    |      CORE     |
                    |    Process    |
                    |---------------|
                    | CPU           |
                    | Memory        |
                    | Stack         |
                    | Queue         |
                    +---------------+
                           |
                           | Core → Logger
                           | POSIX pipe
                           v
                    +---------------+
                    |    LOGGER     |
                    |    Process    |
                    +---------------+
                           |
                           v
                    simulator.log





                    ## 2. IPC Mechanism

### Selected IPC: POSIX Unnamed Pipes

The simulator uses POSIX unnamed pipes for communication between the three processes.

Three pipes are used:

| Pipe | Direction | Purpose |
|---|---|---|
| `ui_to_core` | UI → Core | Sends simulator commands |
| `core_to_ui` | Core → UI | Sends responses from Core |
| `core_to_logger` | Core → Logger | Sends log messages |

POSIX unnamed pipes were selected because the processes are created using `fork()`. Pipes provide simple communication between related processes and are suitable for the small fixed-size messages used by the simulator.




                     ## 3. Process Responsibilities

### UI Process

The UI process accepts commands from the user, sends them to Core, and displays Core responses.

### Core Process

The Core process manages:

- CPU
- Memory
- Stack
- Queue

It receives commands from UI, performs the requested operation, sends responses back to UI, and sends log messages to Logger.

### Logger Process

The Logger process receives log messages from Core and writes them to `simulator.log`.




                           ## 4. Integration

The Team Leader integration program creates the required pipes and forks the three processes.

The processes close the pipe file descriptors that they do not need. The parent process waits for all three child processes to terminate.

The simulator is compiled using:

```bash
gcc -Wall -Wextra -std=c11 ui.c core.c logger.c main.c -o simulator







                ## 5. IPC Testing

The following operations were tested successfully:

| Test | Command | Result |
|---|---|---|
| 1 | `status` | Core status displayed |
| 2 | `run 10` | CPU executed 10 ticks |
| 3 | `store 2 50` | Value stored |
| 4 | `load 2` | Stored value retrieved |
| 5 | `push 100` | Value pushed onto stack |
| 6 | `stack` | Stack contents displayed |
| 7 | `enqueue 25` | Value added to queue |
| 8 | `queue` | Queue contents displayed |
| 9 | `dequeue` | Value removed from queue |
| 10 | Empty `dequeue` | Error correctly reported |
| 11 | `quit` | All three processes terminated |





          ## 6. Performance Benchmark

A benchmark of 1,000 repetitions was performed using these seven operations:

```text
run 10
store 2 50
load 2
push 100
pop
enqueue 25
dequeue




### Performance Analysis

The multi-process implementation has higher execution time because it introduces inter-process communication, process scheduling, context switching, and logging overhead.

The multi-process benchmark recorded 67,245 voluntary context switches compared with 16 for the standalone benchmark. This demonstrates the additional scheduling and communication activity introduced by the multi-process architecture.

The benchmark demonstrates the trade-off between the performance of a standalone process and the modularity and separation provided by the multi-process architecture.







           ## 7. Conclusion

The Week 4 implementation successfully separates the simulator into three independent processes:

```text
UI → Core → Logger




POSIX unnamed pipes provide communication between the processes.

The integrated simulator was compiled, executed, and tested successfully. Commands are transferred from UI to Core, Core responses are returned to UI, and Core log messages are transferred to Logger and stored in `simulator.log`.