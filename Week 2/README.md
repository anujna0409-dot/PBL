# Week 2 – CPU Simulator using IPC Techniques

## Project overview

This project implements a CPU simulator using separate processes for the
user interface, CPU/core logic, and logging.

The source files contain separate `main()` functions, so they are compiled
into three independent executables. A separate `main.c` is not required by
the current implementation.

## IPC techniques present in the source code

1. **UNIX Domain Sockets** – The UI sends commands to the Core and receives
   responses over a local stream socket.
2. **Shared Memory** – The Core publishes the CPU, memory, stack, and queue
   state so the UI can display the live state.
3. **POSIX Semaphores** – The semaphores protect access to shared state and
   allow the UI to wait until the Core has initialized.
4. **Pipe** – The Core starts the Logger as a child process and sends log
   messages to it through an anonymous pipe.

The first three techniques are used for UI/Core communication and shared
state. The pipe is also used by the current Core-to-Logger implementation.

## Source files

| File | Purpose |
|---|---|
| `core.c` | CPU/core process; handles commands, socket server, shared memory, and starts the Logger |
| `ui.c` | User interface; connects to Core, sends commands, and displays live shared state |
| `logger.c` | Logger process; reads messages from the pipe in `--pipe` mode and writes `simulator.log` |
| `sync.c` | POSIX semaphore helper implementation |
| `sync.h` | Semaphore function declarations |
| `sim_interface.h` | Shared-state structure and socket/shared-memory/semaphore names |
| `ipc_common.h` | Message structure, command types, constants, and legacy message-queue names |
| `Makefile` | Builds the three executables |

## Requirements

- Linux or WSL (Ubuntu)
- GCC
- GNU Make
- POSIX shared memory, semaphores, sockets, and pipes

On Ubuntu/WSL, install the build tools if needed:

```bash
sudo apt update
sudo apt install build-essential
```

## Compile

Open a terminal in the `Week 2` directory and run:

```bash
make
```

This creates three executables:

- `core`
- `ui`
- `logger`

To remove the generated executables:

```bash
make clean
```

## Run the simulator

Use **two Ubuntu/WSL terminals**, both opened in this same `Week 2` directory.

**Terminal 1 – start Core:**

```bash
./core
```

Core initializes the shared memory and semaphores, starts the Logger automatically,
and waits for the UI to connect.

**Terminal 2 – start UI:**

```bash
./ui
```

Enter commands shown in the UI menu. For example:

```text
LOAD 10
ADD 5
SUB 2
PUSH 25
POP
STORE 3
LOADM 3
PRINT
SHOW CPU
SHOW MEMORY
SHOW STACK
SHOW QUEUE
STATE
EXIT
```

The Logger writes entries to `simulator.log` in the directory from which Core
was started.

## Notes about the current source

- `core.c`, `ui.c`, and `logger.c` each contain their own `main()` function.
  They must not be linked together into one executable.
- Core launches Logger using `./logger --pipe`; therefore, run Core from the
  project directory after building all targets with `make`.
- `logger.c` also contains a standalone POSIX message-queue mode when launched
  without `--pipe`. The current Core implementation uses pipe mode.
- `ipc_common.h` declares POSIX message-queue names and message types, but the
  current UI/Core path in `ui.c` and `core.c` uses a UNIX Domain Socket.
- Do not start `logger` manually for the normal simulator run; Core starts it.

## Cleanup

Stop the simulator with the UI's `EXIT` command. If a process is interrupted
or terminated unexpectedly, stop any remaining simulator processes before
starting Core again.
