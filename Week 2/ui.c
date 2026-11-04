#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <strings.h>

#include "sim_interface.h"
#include "sync.h"

/* =========================================
   IPC HANDLES
   Socket (commands + replies), shared memory (live state)
   ========================================= */

int sock_fd = -1;

SharedState *shared = NULL;


/* Read exactly 'size' bytes. Returns size, 0 if Core closed, -1 on error. */

ssize_t read_full(int fd, void *buffer, size_t size)
{
    char *p = buffer;
    size_t done = 0;

    while (done < size)
    {
        ssize_t n = read(fd, p + done, size - done);

        if (n == 0)
            return 0;

        if (n < 0)
        {
            if (errno == EINTR)
                continue;

            return -1;
        }

        done += (size_t)n;
    }

    return (ssize_t)done;
}


/* Write exactly 'size' bytes. Returns 0 or -1. */

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
                continue;

            return -1;
        }

        done += (size_t)n;
    }

    return 0;
}


/* =========================================
   LIVE STATE (reads SHARED MEMORY)
   lock_state / unlock_state are the semaphores
   so we never read half-written data.
   ========================================= */

void show_live_state(void)
{
    SharedState copy;

    lock_state();
    copy = *shared;
    unlock_state();

    printf("\n");
    printf("======= LIVE STATE (shared memory) =======\n");
    printf("ACC = %d    PC = %d\n", copy.accumulator, copy.program_counter);
    printf("Stack : size=%d", copy.stack_top + 1);

    for (int i = 0; i <= copy.stack_top && i < STACK_SIZE; i++)
        printf("%s%d", i == 0 ? "  [" : ", ", copy.stack[i]);

    if (copy.stack_top >= 0)
        printf("]");

    printf("\n");
    printf("Queue : %d instruction(s) waiting\n", copy.queue_count);
    printf("Memory (non-zero cells):");

    int shown = 0;

    for (int i = 0; i < MEMORY_SIZE; i++)
    {
        if (copy.memory[i] != 0)
        {
            printf(" [%d]=%d", i, copy.memory[i]);
            shown++;
        }
    }

    if (shown == 0)
        printf(" none");

    printf("\n==========================================\n");
}


/* =========================================
   SHOW ALL NON-ZERO MEMORY CELLS
   Read the Core's shared-memory copy while
   holding the state semaphore.
   ========================================= */
void show_memory_state(void)
{
    SharedState copy;

    lock_state();
    copy = *shared;
    unlock_state();

    printf("\n");
    printf("=========== MEMORY (non-zero cells) ===========\n");

    int shown = 0;

    for (int i = 0; i < MEMORY_SIZE; i++)
    {
        if (copy.memory[i] != 0)
        {
            printf("Memory[%d] = %d\n", i, copy.memory[i]);
            shown++;
        }
    }

    if (shown == 0)
        printf("All memory cells are zero.\n");

    printf("===============================================\n");
}


/* =========================================
   DISPLAY MENU
   ========================================= */

void display_menu(void)
{
    printf("\n");
    printf("========================================\n");
    printf("           CPU SIMULATOR UI\n");
    printf("========================================\n");
    printf("LOAD <value>       - Load value into ACC\n");
    printf("ADD <value>        - Add value to ACC\n");
    printf("SUB <value>        - Subtract value from ACC\n");
    printf("PUSH <value>       - Push value onto stack\n");
    printf("POP                - Pop value into ACC\n");
    printf("STORE <address>    - Store ACC in memory\n");
    printf("LOADM <address>    - Load memory into ACC\n");
    printf("PRINT              - Print ACC\n");
    printf("SHOW CPU           - Show CPU information\n");
    printf("SHOW MEMORY        - Show all non-zero memory cells\n");
    printf("SHOW STACK         - Show stack information\n");
    printf("SHOW QUEUE         - Show instruction queue\n");
    printf("STATE              - Live state (shared memory)\n");
    printf("EXIT               - Exit simulator\n");
    printf("========================================\n");
}


/* =========================================
   CREATE MESSAGE FROM USER COMMAND
   ========================================= */

int create_command(char *input, Message *msg)
{
    char command[20];
    char option[20];
    int address;

    memset(msg, 0, sizeof(Message));

    /* -----------------------------------------
       STORE <address>
       ----------------------------------------- */

    if (sscanf(input, "%19s %d", command, &address) == 2)
    {
        if (strcmp(command, "STORE") == 0)
        {
            msg->type = CMD_STORE;
            msg->address = address;
            return 1;
        }

        if (strcmp(command, "LOADM") == 0)
        {
            msg->type = CMD_LOADM;
            msg->address = address;
            return 1;
        }

        if (strcmp(command, "LOAD") == 0)
        {
            msg->type = CMD_LOAD;
            msg->value = address;
            return 1;
        }

        if (strcmp(command, "ADD") == 0)
        {
            msg->type = CMD_ADD;
            msg->value = address;
            return 1;
        }

        if (strcmp(command, "SUB") == 0)
        {
            msg->type = CMD_SUB;
            msg->value = address;
            return 1;
        }

        if (strcmp(command, "PUSH") == 0)
        {
            msg->type = CMD_PUSH;
            msg->value = address;
            return 1;
        }

        return 0;
    }


    /* -----------------------------------------
       SHOW CPU / MEMORY / STACK / QUEUE
       ----------------------------------------- */

    if (sscanf(input, "%19s %19s", command, option) == 2)
    {
        if (strcmp(command, "SHOW") != 0)
            return 0;

        if (strcmp(option, "CPU") == 0)
        {
            msg->type = CMD_SHOW_CPU;
            return 1;
        }

        if (strcmp(option, "MEMORY") == 0)
        {
            msg->type = CMD_SHOW_MEMORY;
            return 1;
        }

        if (strcmp(option, "STACK") == 0)
        {
            msg->type = CMD_SHOW_STACK;
            return 1;
        }

        if (strcmp(option, "QUEUE") == 0)
        {
            msg->type = CMD_SHOW_QUEUE;
            return 1;
        }

        return 0;
    }


    /* -----------------------------------------
       COMMANDS WITHOUT ARGUMENTS
       ----------------------------------------- */

    if (sscanf(input, "%19s", command) == 1)
    {
        if (strcmp(command, "POP") == 0)
        {
            msg->type = CMD_POP;
            return 1;
        }

        if (strcmp(command, "PRINT") == 0)
        {
            msg->type = CMD_PRINT;
            return 1;
        }

        if (strcmp(command, "EXIT") == 0)
        {
            msg->type = CMD_EXIT;
            return 1;
        }
    }

    return 0;
}


/* =========================================
   SEND COMMAND TO CORE
   ========================================= */

int send_to_core(Message *msg)
{
    if (write_full(sock_fd, msg, sizeof(Message)) == -1)
    {
        perror("Error sending message to Core");
        return 0;
    }

    return 1;
}


/* =========================================
   RECEIVE RESPONSE FROM CORE
   ========================================= */

void receive_from_core(void)
{
    Message response;

    ssize_t n = read_full(sock_fd, &response, sizeof(Message));

    if (n <= 0)
    {
        if (n == 0)
            printf("\nCore closed the connection.\n");
        else
            perror("Error receiving response from Core");

        return;
    }

    switch (response.type)
    {
        case RESPONSE_SUCCESS:

            printf("\nSUCCESS: ");

            if (strlen(response.data) > 0)
                printf("%s", response.data);
            else
                printf("Command executed successfully.");

            printf("\n");
            break;


        case RESPONSE_ERROR:

            printf("\nERROR: %s\n", response.data);
            break;


        case RESPONSE_CPU:

            printf("\n");
            printf("------------- CPU -------------\n");
            printf("%s\n", response.data);
            printf("-------------------------------\n");
            break;


        case RESPONSE_MEMORY:

            printf("\n");
            printf("----------- MEMORY ------------\n");
            printf("%s\n", response.data);
            printf("-------------------------------\n");
            break;


        case RESPONSE_STACK:

            printf("\n");
            printf("------------ STACK ------------\n");
            printf("%s\n", response.data);
            printf("-------------------------------\n");
            break;


        case RESPONSE_QUEUE:

            printf("\n");
            printf("------------ QUEUE ------------\n");
            printf("%s\n", response.data);
            printf("-------------------------------\n");
            break;


        default:

            printf("\nUnknown response received from Core.\n");
            break;
    }
}


/* =========================================
   MAIN UI PROCESS
   ========================================= */

int main(void)
{
    char input[MAX_DATA];
    Message message;

    printf("\n");
    printf("========================================\n");
    printf("       CPU SIMULATOR - UI PROCESS\n");
    printf("========================================\n");


    /* -----------------------------------------
       Semaphores: wait until Core is ready
       ----------------------------------------- */

    signal(SIGPIPE, SIG_IGN);

    if (sync_open() == -1)
    {
        printf("Make sure the Core process is running.\n");
        return EXIT_FAILURE;
    }

    if (wait_core_ready(10) == -1)
    {
        sync_close();
        return EXIT_FAILURE;
    }


    /* -----------------------------------------
       Shared memory: map Core's state
       ----------------------------------------- */

    int shm_fd = shm_open(SIM_SHM_NAME, O_RDWR, 0666);

    if (shm_fd == -1)
    {
        perror("Failed to open shared memory");
        sync_close();
        return EXIT_FAILURE;
    }

    shared = mmap(NULL, sizeof(SharedState),
                  PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    close(shm_fd);

    if (shared == MAP_FAILED)
    {
        perror("Failed to map shared memory");
        sync_close();
        return EXIT_FAILURE;
    }


    /* -----------------------------------------
       Socket: connect to Core
       ----------------------------------------- */

    struct sockaddr_un addr;

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1)
    {
        perror("socket");
        sync_close();
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SIM_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("Failed to connect to Core");
        printf("Make sure the Core process is running.\n");
        close(sock_fd);
        sync_close();
        return EXIT_FAILURE;
    }


    printf("Connected to Core process.\n");
    printf("You can now enter commands.\n");


    /* -----------------------------------------
       Main command loop
       ----------------------------------------- */

    while (1)
    {
        display_menu();

        printf("\nEnter command: ");
        fflush(stdout);


        /* Read user input */

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\nInput closed.\n");
            break;
        }


        /* Remove newline */

        input[strcspn(input, "\n")] = '\0';


        /* Ignore empty input */

        if (strlen(input) == 0)
            continue;


        /* STATE and SHOW MEMORY read the synchronized shared-memory copy. */

        if (strcasecmp(input, "STATE") == 0)
        {
            show_live_state();
            continue;
        }

        if (strcasecmp(input, "SHOW MEMORY") == 0)
        {
            show_memory_state();
            continue;
        }


        /* Convert command to Message */

        if (!create_command(input, &message))
        {
            printf("\nInvalid command.\n");
            printf("Please enter a valid command from the menu.\n");
            continue;
        }


        /* Send command to Core */

        if (!send_to_core(&message))
            continue;


        /* -----------------------------------------
           EXIT command
           ----------------------------------------- */

        if (message.type == CMD_EXIT)
        {
            /*
             * Core sends a final success response
             * before terminating.
             */
            receive_from_core();

            printf("\nUI process terminating...\n");
            break;
        }


        /* -----------------------------------------
           Receive Core response
           ----------------------------------------- */

        receive_from_core();
    }


    /* -----------------------------------------
       Close message queues
       ----------------------------------------- */

    close(sock_fd);
    munmap(shared, sizeof(SharedState));
    sync_close();


    printf("UI process terminated.\n");

    return EXIT_SUCCESS;
}

