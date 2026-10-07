#include <signal.h>
#include "common.h"

#define MEM_SIZE    16
#define STACK_SIZE  10
#define QUEUE_SIZE  10
#define MAX_VALUE   9999    /* values must be between -9999 and 9999 */
#define MAX_TICKS   1000    /* one "run" command may execute at most this many ticks */

/* Everything the simulator remembers */
typedef struct {
    long total_ticks;           /* CPU: ticks executed so far */
    int  run_count;             /* CPU: how many "run" commands were done */
    int  memory[MEM_SIZE];      /* Memory cells */
    int  stack[STACK_SIZE];     /* Stack items */
    int  sp;                    /* Stack: number of items stored */
    int  queue[QUEUE_SIZE];     /* Queue items */
    int  head;                  /* Queue: index of the oldest item */
    int  count;                 /* Queue: number of items stored */
} Sim;

/* ---------- small helper functions ---------- */

/* Send one message to the Logger */
static void write_log(int log_fd, int level, const char *text)
{
    send_msg(log_fd, MSG_LOG, level, "%s", text);
}

/* Send one response to the UI */
static void reply(int resp_fd, const char *text)
{
    send_msg(resp_fd, MSG_RESP, LOG_INFO, "%s", text);
}

/* Convert text to an int. Returns 0 if OK, -1 if it is not a valid number
   or is outside -MAX_VALUE..MAX_VALUE. */
static int to_int(const char *s, int *value)
{
    char *end;
    errno = 0;
    long n = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || n < -MAX_VALUE || n > MAX_VALUE)
        return -1;
    *value = (int)n;
    return 0;
}

/* Put the simulator back to its starting state */
static void sim_reset(Sim *s)
{
    memset(s, 0, sizeof *s);
}

/* ---------- command processing ---------- */

/* Run one command. The reply text is written into out.
   Returns 0 if the command worked, -1 if it was an error. */
static int process_command(Sim *s, const char *text, char *out, size_t size)
{
    char cmd[32], a1[32], a2[32];
    int x, y;
    int n = sscanf(text, "%31s %31s %31s", cmd, a1, a2);   /* n = number of words read */

    if (n < 1) {
        snprintf(out, size, "Error: empty command. Type help.");
        return -1;
    }

    /* ----- general ----- */
    if (strcmp(cmd, "help") == 0) {
        snprintf(out, size, "Commands: help status run <n> store <addr> <val> load <addr> "
                            "memory push <v> pop stack enqueue <v> dequeue queue reset quit");
        return 0;
    }
    if (strcmp(cmd, "status") == 0) {
        snprintf(out, size, "CPU: %ld ticks in %d runs | Stack: %d/%d | Queue: %d/%d",
                 s->total_ticks, s->run_count, s->sp, STACK_SIZE, s->count, QUEUE_SIZE);
        return 0;
    }
    if (strcmp(cmd, "reset") == 0) {
        sim_reset(s);
        snprintf(out, size, "Simulator reset.");
        return 0;
    }

    /* ----- CPU ----- */
    if (strcmp(cmd, "run") == 0) {
        if (n < 2 || to_int(a1, &x) < 0 || x < 1 || x > MAX_TICKS) {
            snprintf(out, size, "Error: usage: run <ticks 1-%d>", MAX_TICKS);
            return -1;
        }
        s->total_ticks += x;        /* the CPU "executes" x ticks */
        s->run_count++;
        snprintf(out, size, "CPU executed %d ticks. Total: %ld ticks.", x, s->total_ticks);
        return 0;
    }

    /* ----- Memory ----- */
    if (strcmp(cmd, "store") == 0) {
        if (n < 3 || to_int(a1, &x) < 0 || to_int(a2, &y) < 0) {
            snprintf(out, size, "Error: usage: store <address 0-%d> <value>", MEM_SIZE - 1);
            return -1;
        }
        if (x < 0 || x >= MEM_SIZE) {
            snprintf(out, size, "Error: address must be 0 to %d.", MEM_SIZE - 1);
            return -1;
        }
        s->memory[x] = y;
        snprintf(out, size, "Stored %d at address %d.", y, x);
        return 0;
    }
    if (strcmp(cmd, "load") == 0) {
        if (n < 2 || to_int(a1, &x) < 0 || x < 0 || x >= MEM_SIZE) {
            snprintf(out, size, "Error: usage: load <address 0-%d>", MEM_SIZE - 1);
            return -1;
        }
        snprintf(out, size, "Address %d holds %d.", x, s->memory[x]);
        return 0;
    }
    if (strcmp(cmd, "memory") == 0) {
        size_t pos = (size_t)snprintf(out, size, "Memory:");
        for (int i = 0; i < MEM_SIZE && pos < size; i++)
            pos += (size_t)snprintf(out + pos, size - pos, " [%d]=%d", i, s->memory[i]);
        return 0;
    }

    /* ----- Stack ----- */
    if (strcmp(cmd, "push") == 0) {
        if (n < 2 || to_int(a1, &x) < 0) {
            snprintf(out, size, "Error: usage: push <value>");
            return -1;
        }
        if (s->sp >= STACK_SIZE) {
            snprintf(out, size, "Error: stack overflow (stack is full).");
            return -1;
        }
        s->stack[s->sp++] = x;
        snprintf(out, size, "Pushed %d. Stack has %d items.", x, s->sp);
        return 0;
    }
    if (strcmp(cmd, "pop") == 0) {
        if (s->sp == 0) {
            snprintf(out, size, "Error: stack underflow (stack is empty).");
            return -1;
        }
        x = s->stack[--s->sp];
        snprintf(out, size, "Popped %d. Stack has %d items.", x, s->sp);
        return 0;
    }
    if (strcmp(cmd, "stack") == 0) {
        size_t pos = (size_t)snprintf(out, size, "Stack (bottom to top):");
        for (int i = 0; i < s->sp && pos < size; i++)
            pos += (size_t)snprintf(out + pos, size - pos, " %d", s->stack[i]);
        if (s->sp == 0)
            snprintf(out, size, "Stack is empty.");
        return 0;
    }

    /* ----- Queue ----- */
    if (strcmp(cmd, "enqueue") == 0) {
        if (n < 2 || to_int(a1, &x) < 0) {
            snprintf(out, size, "Error: usage: enqueue <value>");
            return -1;
        }
        if (s->count >= QUEUE_SIZE) {
            snprintf(out, size, "Error: queue is full.");
            return -1;
        }
        s->queue[(s->head + s->count) % QUEUE_SIZE] = x;   /* add at the back */
        s->count++;
        snprintf(out, size, "Enqueued %d. Queue has %d items.", x, s->count);
        return 0;
    }
    if (strcmp(cmd, "dequeue") == 0) {
        if (s->count == 0) {
            snprintf(out, size, "Error: queue is empty.");
            return -1;
        }
        x = s->queue[s->head];                              /* take from the front */
        s->head = (s->head + 1) % QUEUE_SIZE;
        s->count--;
        snprintf(out, size, "Dequeued %d. Queue has %d items.", x, s->count);
        return 0;
    }
    if (strcmp(cmd, "queue") == 0) {
        size_t pos = (size_t)snprintf(out, size, "Queue (front to back):");
        for (int i = 0; i < s->count && pos < size; i++)
            pos += (size_t)snprintf(out + pos, size - pos, " %d",
                                    s->queue[(s->head + i) % QUEUE_SIZE]);
        if (s->count == 0)
            snprintf(out, size, "Queue is empty.");
        return 0;
    }

    /* ----- anything else ----- */
    snprintf(out, size, "Error: unknown command '%s'. Type help.", cmd);
    return -1;
}

/* ---------- main Core loop ---------- */

void run_core(int cmd_fd, int resp_fd, int log_fd)
{
    Sim  sim;
    Msg  msg;
    char out[MSG_SIZE];     /* reply text; same size as Msg, so it always fits */

    /* If UI or Logger has already closed its pipe, write() should return
       an error instead of killing Core with SIGPIPE. */
    signal(SIGPIPE, SIG_IGN);

    sim_reset(&sim);
    write_log(log_fd, LOG_INFO, "Core started.");

    while (1) {
        /* 1. Wait for a message from UI */
        if (recv_msg(cmd_fd, &msg) < 0) {
            write_log(log_fd, LOG_ERROR, "Command pipe closed. Core stopping.");
            break;
        }
        msg.text[sizeof msg.text - 1] = '\0';   /* make sure it is a valid string */

        /* 2. Shutdown: MSG_QUIT (or the text "quit" sent as a command) */
        if (msg.type == MSG_QUIT || (msg.type == MSG_CMD && strcmp(msg.text, "quit") == 0)) {
            write_log(log_fd, LOG_INFO, "Shutdown requested. Core stopping.");
            send_msg(resp_fd, MSG_QUIT, LOG_INFO, "Core shutting down.");   /* final reply to UI */
            send_msg(log_fd, MSG_QUIT, LOG_INFO, "Core shutting down.");    /* tell Logger to stop */
            break;
        }

        /* 3. Only MSG_CMD is a valid command from UI */
        if (msg.type != MSG_CMD) {
            snprintf(out, sizeof out, "Error: unexpected message type %d.", msg.type);
            write_log(log_fd, LOG_ERROR, out);
            reply(resp_fd, out);
            continue;
        }

        /* 4. Run the command, answer the UI, and tell the Logger what happened */
        int result = process_command(&sim, msg.text, out, sizeof out);
        reply(resp_fd, out);

        char logtext[MSG_SIZE - 8];
        if (result == 0) {
            snprintf(logtext, sizeof logtext, "OK: %.100s", msg.text);
            write_log(log_fd, LOG_INFO, logtext);
        } else {
            snprintf(logtext, sizeof logtext, "FAILED: %.100s -> %.100s", msg.text, out);
            write_log(log_fd, LOG_ERROR, logtext);
        }
    }
}