#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "sim_interface.h"
#include "sync.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "ipc_common.h"


/* =========================================================
   CPU
   ========================================================= */

typedef struct
{
    int accumulator;
    int program_counter;

} CPU;


/* =========================================================
   MEMORY
   ========================================================= */

typedef struct
{
    int data[MEMORY_SIZE];

} Memory;


/* =========================================================
   STACK
   ========================================================= */

typedef struct
{
    int data[STACK_SIZE];
    int top;

} Stack;


/* =========================================================
   INSTRUCTION QUEUE
   ========================================================= */

typedef struct
{
    Message data[QUEUE_SIZE];

    int front;
    int rear;
    int count;

} InstructionQueue;


/* =========================================================
   GLOBAL CORE COMPONENTS
   ========================================================= */

CPU cpu;

Memory memory;

Stack stack;

InstructionQueue instruction_queue;


/* =========================================================
   MESSAGE QUEUE DESCRIPTORS
   ========================================================= */

/* UI <-> Core now uses a UNIX SOCKET (Rishabh's part).
 * State is mirrored into SHARED MEMORY (Nishanth's part),
 * guarded by SEMAPHORES (Jovita's part). */

int listen_fd = -1;

int client_fd = -1;

SharedState *shared = NULL;

void sync_shared(void);


/*
 * Core -> Logger now uses an anonymous PIPE (pipe + fork).
 * Core creates the pipe, forks the Logger as a child process,
 * and keeps only the WRITE end here.
 */

int logger_pipe_fd = -1;

pid_t logger_pid = -1;


/* =========================================================
   CPU FUNCTIONS
   ========================================================= */

void cpu_init(void)
{
    cpu.accumulator = 0;
    cpu.program_counter = 0;
}


void cpu_load(int value)
{
    cpu.accumulator = value;
}


void cpu_add(int value)
{
    cpu.accumulator += value;
}


void cpu_sub(int value)
{
    cpu.accumulator -= value;
}


void cpu_increment_pc(void)
{
    cpu.program_counter++;
}


/* =========================================================
   MEMORY FUNCTIONS
   ========================================================= */

void memory_init(void)
{
    int i;

    for (i = 0; i < MEMORY_SIZE; i++)
    {
        memory.data[i] = 0;
    }
}


int memory_valid_address(int address)
{
    if (address >= 0 && address < MEMORY_SIZE)
    {
        return 1;
    }

    return 0;
}


int memory_store(int address, int value)
{
    if (!memory_valid_address(address))
    {
        return 0;
    }

    memory.data[address] = value;

    return 1;
}


int memory_load(int address, int *value)
{
    if (!memory_valid_address(address))
    {
        return 0;
    }

    *value = memory.data[address];

    return 1;
}


/* =========================================================
   STACK FUNCTIONS
   ========================================================= */

void stack_init(void)
{
    stack.top = -1;
}


int stack_is_empty(void)
{
    return stack.top == -1;
}


int stack_is_full(void)
{
    return stack.top == STACK_SIZE - 1;
}


int stack_push(int value)
{
    if (stack_is_full())
    {
        return 0;
    }

    stack.top++;

    stack.data[stack.top] = value;

    return 1;
}


int stack_pop(int *value)
{
    if (stack_is_empty())
    {
        return 0;
    }

    *value = stack.data[stack.top];

    stack.top--;

    return 1;
}


int stack_size(void)
{
    return stack.top + 1;
}


/* =========================================================
   INSTRUCTION QUEUE FUNCTIONS
   ========================================================= */

void queue_init(void)
{
    instruction_queue.front = 0;
    instruction_queue.rear = -1;
    instruction_queue.count = 0;
}


int queue_is_empty(void)
{
    return instruction_queue.count == 0;
}


int queue_is_full(void)
{
    return instruction_queue.count == QUEUE_SIZE;
}


int queue_enqueue(Message message)
{
    if (queue_is_full())
    {
        return 0;
    }

    instruction_queue.rear =
        (instruction_queue.rear + 1) % QUEUE_SIZE;

    instruction_queue.data[instruction_queue.rear] = message;

    instruction_queue.count++;

    return 1;
}


int queue_dequeue(Message *message)
{
    if (queue_is_empty())
    {
        return 0;
    }

    *message = instruction_queue.data[instruction_queue.front];

    instruction_queue.front =
        (instruction_queue.front + 1) % QUEUE_SIZE;

    instruction_queue.count--;

    return 1;
}


/* =========================================================
   SOCKET HELPERS
   A stream socket can return fewer bytes than asked, so we
   loop until the whole Message is written / read.
   ========================================================= */

int write_full(int fd, const void *buffer, size_t size)
{
    const char *p = buffer;
    size_t done = 0;

    while (done < size)
    {
        ssize_t n = write(fd, p + done, size - done);

        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return -1;
        }

        done += (size_t)n;
    }

    return 0;
}


/* Returns bytes read (== size), 0 if the other side closed, -1 on error. */

ssize_t read_full(int fd, void *buffer, size_t size)
{
    char *p = buffer;
    size_t done = 0;

    while (done < size)
    {
        ssize_t n = read(fd, p + done, size - done);

        if (n == 0)
        {
            return 0;
        }

        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return -1;
        }

        done += (size_t)n;
    }

    return (ssize_t)done;
}


/* =========================================================
   SEND MESSAGE TO UI
   ========================================================= */

void send_to_ui(
    MessageType type,
    int value,
    int address,
    const char *text
)
{
    Message message;

    memset(&message, 0, sizeof(Message));

    message.type = type;

    message.value = value;

    message.address = address;

    if (text != NULL)
    {
        strncpy(
            message.data,
            text,
            MAX_DATA - 1
        );

        message.data[MAX_DATA - 1] = '\0';
    }


    /*
     * Update shared memory BEFORE the reply goes out, so when the
     * UI gets the reply and reads the shared state, it is current.
     */

    sync_shared();


    if (write_full(
            client_fd,
            &message,
            sizeof(Message)
        ) == -1)
    {
        perror("Error sending message to UI");
    }
}


/* =========================================================
   SEND MESSAGE TO LOGGER
   ========================================================= */

void send_to_logger(
    MessageType type,
    int value,
    int address,
    const char *text
)
{
    Message message;

    memset(&message, 0, sizeof(Message));

    message.type = type;

    message.value = value;

    message.address = address;

    if (text != NULL)
    {
        strncpy(
            message.data,
            text,
            MAX_DATA - 1
        );

        message.data[MAX_DATA - 1] = '\0';
    }


    if (logger_pipe_fd != -1)
    {
        if (write(
                logger_pipe_fd,
                &message,
                sizeof(Message)
            ) != (ssize_t)sizeof(Message))
        {
            perror("Error sending message to Logger");
        }
    }
}


/* =========================================================
   SEND SUCCESS
   ========================================================= */

void send_success(
    int value,
    int address,
    const char *text
)
{
    send_to_ui(
        RESPONSE_SUCCESS,
        value,
        address,
        text
    );
}


/* =========================================================
   SEND ERROR
   ========================================================= */

void send_error(const char *text)
{
    send_to_ui(
        RESPONSE_ERROR,
        0,
        -1,
        text
    );

    send_to_logger(
        LOG_ERROR,
        0,
        -1,
        text
    );
}


/* =========================================================
   SHOW CPU
   ========================================================= */

void show_cpu(void)
{
    char text[MAX_DATA];

    snprintf(
        text,
        MAX_DATA,
        "ACC=%d, PC=%d",
        cpu.accumulator,
        cpu.program_counter
    );


    send_to_ui(
        RESPONSE_CPU,
        cpu.accumulator,
        cpu.program_counter,
        text
    );


    send_to_logger(
        LOG_SYSTEM,
        cpu.accumulator,
        cpu.program_counter,
        "CPU state displayed."
    );
}


/* =========================================================
   SHOW MEMORY
   ========================================================= */

void show_memory(int address)
{
    char text[MAX_DATA];

    if (!memory_valid_address(address))
    {
        send_error("Invalid memory address.");

        return;
    }


    snprintf(
        text,
        MAX_DATA,
        "Memory[%d] = %d",
        address,
        memory.data[address]
    );


    send_to_ui(
        RESPONSE_MEMORY,
        memory.data[address],
        address,
        text
    );


    send_to_logger(
        LOG_SYSTEM,
        memory.data[address],
        address,
        "Memory value displayed."
    );
}


/* =========================================================
   SHOW STACK
   ========================================================= */

void show_stack(void)
{
    char text[MAX_DATA];

    snprintf(
        text,
        MAX_DATA,
        "Stack size=%d, Top=%d",
        stack_size(),
        stack.top
    );


    send_to_ui(
        RESPONSE_STACK,
        stack_size(),
        stack.top,
        text
    );


    send_to_logger(
        LOG_SYSTEM,
        stack_size(),
        stack.top,
        "Stack state displayed."
    );
}


/* =========================================================
   SHOW INSTRUCTION QUEUE
   ========================================================= */

void show_queue(void)
{
    char text[MAX_DATA];

    snprintf(
        text,
        MAX_DATA,
        "Instruction Queue size=%d",
        instruction_queue.count
    );


    send_to_ui(
        RESPONSE_QUEUE,
        instruction_queue.count,
        instruction_queue.front,
        text
    );


    send_to_logger(
        LOG_SYSTEM,
        instruction_queue.count,
        instruction_queue.front,
        "Instruction queue state displayed."
    );
}


/* =========================================================
   EXECUTE COMMAND
   ========================================================= */

int execute_command(Message *command)
{
    int value;


    switch (command->type)
    {

        /* -------------------------------------------------
           LOAD
           ------------------------------------------------- */

        case CMD_LOAD:

            cpu_load(command->value);

            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                -1,
                "LOAD executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                command->value,
                -1,
                "LOAD executed."
            );

            break;


        /* -------------------------------------------------
           ADD
           ------------------------------------------------- */

        case CMD_ADD:

            cpu_add(command->value);

            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                -1,
                "ADD executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                command->value,
                -1,
                "ADD executed."
            );

            break;


        /* -------------------------------------------------
           SUB
           ------------------------------------------------- */

        case CMD_SUB:

            cpu_sub(command->value);

            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                -1,
                "SUB executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                command->value,
                -1,
                "SUB executed."
            );

            break;


        /* -------------------------------------------------
           PUSH
           ------------------------------------------------- */

        case CMD_PUSH:

            if (!stack_push(command->value))
            {
                send_error(
                    "Stack overflow. PUSH failed."
                );

                return 0;
            }


            cpu_increment_pc();

            send_success(
                command->value,
                stack.top,
                "PUSH executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                command->value,
                stack.top,
                "PUSH executed."
            );

            break;


        /* -------------------------------------------------
           POP
           ------------------------------------------------- */

        case CMD_POP:

            if (!stack_pop(&value))
            {
                send_error(
                    "Stack underflow. POP failed."
                );

                return 0;
            }


            cpu.accumulator = value;

            cpu_increment_pc();

            send_success(
                value,
                stack.top,
                "POP executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                value,
                stack.top,
                "POP executed."
            );

            break;


        /* -------------------------------------------------
           STORE
           ------------------------------------------------- */

        case CMD_STORE:

            if (!memory_store(
                    command->address,
                    cpu.accumulator
                ))
            {
                send_error(
                    "Invalid memory address. STORE failed."
                );

                return 0;
            }


            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                command->address,
                "STORE executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                cpu.accumulator,
                command->address,
                "STORE executed."
            );

            break;


        /* -------------------------------------------------
           LOADM
           ------------------------------------------------- */

        case CMD_LOADM:

            if (!memory_load(
                    command->address,
                    &value
                ))
            {
                send_error(
                    "Invalid memory address. LOADM failed."
                );

                return 0;
            }


            cpu.accumulator = value;

            cpu_increment_pc();

            send_success(
                value,
                command->address,
                "LOADM executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                value,
                command->address,
                "LOADM executed."
            );

            break;


        /* -------------------------------------------------
           PRINT
           ------------------------------------------------- */

        case CMD_PRINT:

            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                -1,
                "PRINT executed."
            );

            send_to_logger(
                LOG_EXECUTION,
                cpu.accumulator,
                -1,
                "PRINT executed."
            );

            break;


        /* -------------------------------------------------
           SHOW CPU
           ------------------------------------------------- */

        case CMD_SHOW_CPU:

            show_cpu();

            break;


        /* -------------------------------------------------
           SHOW MEMORY
           ------------------------------------------------- */

        case CMD_SHOW_MEMORY:

            show_memory(command->address);

            break;


        /* -------------------------------------------------
           SHOW STACK
           ------------------------------------------------- */

        case CMD_SHOW_STACK:

            show_stack();

            break;


        /* -------------------------------------------------
           SHOW QUEUE
           ------------------------------------------------- */

        case CMD_SHOW_QUEUE:

            show_queue();

            break;


        /* -------------------------------------------------
           EXIT
           ------------------------------------------------- */

        case CMD_EXIT:

            send_success(
                0,
                -1,
                "Core received EXIT command."
            );

            send_to_logger(
                LOG_SYSTEM,
                0,
                -1,
                "Core received EXIT command."
            );

            return 1;


        /* -------------------------------------------------
           INVALID COMMAND
           ------------------------------------------------- */

        default:

            send_error(
                "Invalid command received by Core."
            );

            return 0;
    }


    return 0;
}


/* =========================================================
   COPY CORE STATE INTO SHARED MEMORY
   Lock (semaphore) -> copy -> unlock.
   ========================================================= */

void sync_shared(void)
{
    if (shared == NULL)
    {
        return;
    }

    lock_state();

    shared->accumulator     = cpu.accumulator;
    shared->program_counter = cpu.program_counter;

    memcpy(shared->memory, memory.data, sizeof(shared->memory));

    memcpy(shared->stack, stack.data, sizeof(shared->stack));

    shared->stack_top   = stack.top;
    shared->queue_count = instruction_queue.count;

    unlock_state();
}


/* =========================================================
   OPEN MESSAGE QUEUES
   ========================================================= */

int open_queues(void)
{
    /* ---------- 1. Semaphores (Jovita) ---------- */

    if (sync_create() == -1)
    {
        return 0;
    }


    /* ---------- 2. Shared memory (Nishanth) ---------- */

    int shm_fd = shm_open(SIM_SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd == -1)
    {
        perror("shm_open");

        return 0;
    }

    if (ftruncate(shm_fd, sizeof(SharedState)) == -1)
    {
        perror("ftruncate");

        close(shm_fd);

        return 0;
    }

    shared = mmap(
        NULL,
        sizeof(SharedState),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );

    close(shm_fd);

    if (shared == MAP_FAILED)
    {
        perror("mmap");

        shared = NULL;

        return 0;
    }


    /* ---------- 3. Socket server (Rishabh's UI connects here) ---------- */

    struct sockaddr_un addr;

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (listen_fd == -1)
    {
        perror("socket");

        return 0;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, SIM_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SIM_SOCKET_PATH);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");

        return 0;
    }

    if (listen(listen_fd, 1) == -1)
    {
        perror("listen");

        return 0;
    }


    /* ---------- 4. Tell UI: everything is ready ---------- */

    sync_shared();

    signal_core_ready();

    return 1;
}


/* =========================================================
   WAIT FOR THE UI TO CONNECT
   ========================================================= */

int accept_ui(void)
{
    do
    {
        client_fd = accept(listen_fd, NULL, NULL);
    }
    while (client_fd == -1 && errno == EINTR);

    if (client_fd == -1)
    {
        perror("accept");

        return 0;
    }

    return 1;
}


/* =========================================================
   CLOSE MESSAGE QUEUES
   ========================================================= */

void close_queues(void)
{
    if (client_fd != -1)
    {
        close(client_fd);

        client_fd = -1;
    }

    if (listen_fd != -1)
    {
        close(listen_fd);

        listen_fd = -1;

        unlink(SIM_SOCKET_PATH);
    }

    if (shared != NULL)
    {
        munmap(shared, sizeof(SharedState));

        shared = NULL;

        shm_unlink(SIM_SHM_NAME);
    }

    sync_destroy();
}


/* =========================================================
   START LOGGER PROCESS (PIPE + FORK)
   ========================================================= */

int start_logger(void)
{
    int fds[2];     /* fds[0] = read end, fds[1] = write end */


    /*
     * If the Logger dies, writing to the pipe would
     * kill Core with SIGPIPE. Ignore it; write() will
     * just return an error instead.
     */

    signal(SIGPIPE, SIG_IGN);


    if (pipe(fds) == -1)
    {
        perror("pipe");

        return 0;
    }


    logger_pid = fork();


    if (logger_pid == -1)
    {
        perror("fork");

        close(fds[0]);
        close(fds[1]);

        return 0;
    }


    if (logger_pid == 0)
    {
        /* ---------- CHILD: becomes the Logger ---------- */

        close(fds[1]);              /* child does not write */

        if (dup2(fds[0], STDIN_FILENO) == -1)
        {
            perror("dup2");

            _exit(EXIT_FAILURE);
        }

        close(fds[0]);

        /* Try the usual places where the logger program can be. */

        execl("./logger", "logger", "--pipe", (char *)NULL);
        execl("../logger/logger", "logger", "--pipe", (char *)NULL);
        execl("./logger/logger", "logger", "--pipe", (char *)NULL);

        perror("exec logger");

        _exit(EXIT_FAILURE);
    }


    /* ---------- PARENT: Core ---------- */

    close(fds[0]);                  /* parent does not read */

    logger_pipe_fd = fds[1];

    return 1;
}


/* =========================================================
   STOP LOGGER PROCESS
   ========================================================= */

void stop_logger(void)
{
    if (logger_pipe_fd != -1)
    {
        /* Closing the write end gives the Logger end-of-file. */

        close(logger_pipe_fd);

        logger_pipe_fd = -1;
    }


    if (logger_pid > 0)
    {
        waitpid(logger_pid, NULL, 0);

        logger_pid = -1;
    }
}


/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    Message command;

    ssize_t bytes_received;

    int running = 1;


    printf("\n");
    printf("========================================\n");
    printf("       CPU SIMULATOR - CORE PROCESS\n");
    printf("========================================\n");


    /* -----------------------------------------
       Initialize CPU
       ----------------------------------------- */

    cpu_init();


    /* -----------------------------------------
       Initialize Memory
       ----------------------------------------- */

    memory_init();


    /* -----------------------------------------
       Initialize Stack
       ----------------------------------------- */

    stack_init();


    /* -----------------------------------------
       Initialize Instruction Queue
       ----------------------------------------- */

    queue_init();


    /* -----------------------------------------
       Set up IPC: semaphores, shared memory, socket
       ----------------------------------------- */

    if (!open_queues())
    {
        fprintf(
            stderr,
            "Failed to set up socket / shared memory / semaphores.\n"
        );

        return EXIT_FAILURE;
    }


    /* -----------------------------------------
       Start Logger process (pipe + fork)
       ----------------------------------------- */

    if (!start_logger())
    {
        fprintf(
            stderr,
            "Warning: Logger could not be started. "
            "Logging is off.\n"
        );
    }


    printf("Core initialized successfully.\n");
    printf("Waiting for UI to connect...\n");

    if (!accept_ui())
    {
        stop_logger();

        close_queues();

        return EXIT_FAILURE;
    }

    printf("UI connected. Waiting for commands...\n");


    send_to_logger(
        LOG_SYSTEM,
        0,
        -1,
        "Core process started."
    );


    /* =================================================
       MAIN LOOP
       ================================================= */

    while (running)
    {

        memset(
            &command,
            0,
            sizeof(Message)
        );


        /*
         * Wait for a command from UI.
         */

        bytes_received = read_full(
            client_fd,
            &command,
            sizeof(Message)
        );


        if (bytes_received == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }


            perror("read from UI");


            send_to_logger(
                LOG_ERROR,
                0,
                -1,
                "Failed to receive command from UI."
            );


            break;
        }


        if (bytes_received == 0)
        {
            printf("UI disconnected.\n");

            send_to_logger(
                LOG_SYSTEM,
                0,
                -1,
                "UI disconnected."
            );

            break;
        }


        /*
         * Verify message size.
         */

        if ((size_t)bytes_received != sizeof(Message))
        {
            send_error(
                "Invalid message size."
            );

            continue;
        }


        /*
         * Add received command to
         * the internal instruction queue.
         */

        if (!queue_enqueue(command))
        {
            send_error(
                "Instruction queue is full."
            );

            continue;
        }


        /*
         * Fetch command from internal
         * instruction queue.
         */

        if (!queue_dequeue(&command))
        {
            send_error(
                "Unable to fetch instruction."
            );

            continue;
        }


        /*
         * Execute command.
         */

        if (execute_command(&command) == 1)
        {
            running = 0;
        }
    }


    /* =================================================
       SHUTDOWN
       ================================================= */

    send_to_logger(
        LOG_SYSTEM,
        0,
        -1,
        "Core process shutting down."
    );


    close_queues();

    stop_logger();


    printf("\n");
    printf("Core process terminated.\n");


    return EXIT_SUCCESS;
}
