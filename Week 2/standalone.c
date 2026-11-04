#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "ipc_common.h"

/* CPU */
typedef struct {
    int accumulator;
    int program_counter;
} CPU;

/* Memory */
typedef struct {
    int data[MEMORY_SIZE];
} Memory;

/* Stack */
typedef struct {
    int data[STACK_SIZE];
    int top;
} Stack;

/* Instruction Queue */
typedef struct {
    Message data[QUEUE_SIZE];
    int front, rear, count;
} InstructionQueue;

CPU cpu;
Memory memory;
Stack stack;
InstructionQueue queue;

/* Initialization */
void initialize(void)
{
    cpu.accumulator = 0;
    cpu.program_counter = 0;

    memset(&memory, 0, sizeof(memory));

    stack.top = -1;

    queue.front = 0;
    queue.rear = -1;
    queue.count = 0;
}

/* Queue operations */
int enqueue(Message m)
{
    if (queue.count == QUEUE_SIZE)
        return 0;

    queue.rear = (queue.rear + 1) % QUEUE_SIZE;
    queue.data[queue.rear] = m;
    queue.count++;
    return 1;
}

int dequeue(Message *m)
{
    if (queue.count == 0)
        return 0;

    *m = queue.data[queue.front];
    queue.front = (queue.front + 1) % QUEUE_SIZE;
    queue.count--;
    return 1;
}

/* Execute simulator command */
void execute(Message m)
{
    int value;

    switch (m.type)
    {
        case CMD_LOAD:
            cpu.accumulator = m.value;
            cpu.program_counter++;
            printf("ACC = %d\n", cpu.accumulator);
            break;

        case CMD_ADD:
            cpu.accumulator += m.value;
            cpu.program_counter++;
            printf("ACC = %d\n", cpu.accumulator);
            break;

        case CMD_SUB:
            cpu.accumulator -= m.value;
            cpu.program_counter++;
            printf("ACC = %d\n", cpu.accumulator);
            break;

        case CMD_PUSH:
            if (stack.top == STACK_SIZE - 1)
                printf("Stack overflow.\n");
            else {
                stack.data[++stack.top] = m.value;
                cpu.program_counter++;
                printf("Pushed %d\n", m.value);
            }
            break;

        case CMD_POP:
            if (stack.top == -1)
                printf("Stack underflow.\n");
            else {
                value = stack.data[stack.top--];
                cpu.accumulator = value;
                cpu.program_counter++;
                printf("Popped %d, ACC = %d\n",
                       value, cpu.accumulator);
            }
            break;

        case CMD_STORE:
            if (m.address < 0 || m.address >= MEMORY_SIZE)
                printf("Invalid memory address.\n");
            else {
                memory.data[m.address] = cpu.accumulator;
                cpu.program_counter++;
                printf("Memory[%d] = %d\n",
                       m.address, cpu.accumulator);
            }
            break;

        case CMD_LOADM:
            if (m.address < 0 || m.address >= MEMORY_SIZE)
                printf("Invalid memory address.\n");
            else {
                cpu.accumulator = memory.data[m.address];
                cpu.program_counter++;
                printf("ACC = %d\n", cpu.accumulator);
            }
            break;

        case CMD_PRINT:
            cpu.program_counter++;
            printf("ACC = %d\n", cpu.accumulator);
            break;

        case CMD_SHOW_CPU:
            printf("ACC = %d, PC = %d\n",
                   cpu.accumulator, cpu.program_counter);
            break;

        case CMD_SHOW_MEMORY:
            if (m.address >= 0 && m.address < MEMORY_SIZE) {
                printf("Memory[%d] = %d\n",
                       m.address, memory.data[m.address]);
            } else {
                for (int i = 0; i < MEMORY_SIZE; i++)
                    if (memory.data[i] != 0)
                        printf("Memory[%d] = %d\n",
                               i, memory.data[i]);
            }
            break;

        case CMD_SHOW_STACK:
            printf("Stack size = %d, Top = %d\n",
                   stack.top + 1, stack.top);
            break;

        case CMD_SHOW_QUEUE:
            printf("Instruction Queue size = %d\n",
                   queue.count);
            break;

        default:
            printf("Invalid command.\n");
    }
}

/* Main: all components run in this one process */
int main(void)
{
    char line[128];
    char command[32], second[32];
    int a;
    Message message;

    initialize();

    printf("Standalone CPU Simulator\n");
    printf("Enter EXIT to quit.\n");

    while (1)
    {
        printf("\n> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        command[0] = '\0';
        second[0] = '\0';
        a = 0;

        sscanf(line, "%31s %31s %d", command, second, &a);

        memset(&message, 0, sizeof(message));
        message.address = -1;

        if (!strcasecmp(command, "EXIT"))
            break;

        if (!strcasecmp(command, "LOAD"))
            { message.type = CMD_LOAD; sscanf(line, "%*s %d", &message.value); }
        else if (!strcasecmp(command, "ADD"))
            { message.type = CMD_ADD; sscanf(line, "%*s %d", &message.value); }
        else if (!strcasecmp(command, "SUB"))
            { message.type = CMD_SUB; sscanf(line, "%*s %d", &message.value); }
        else if (!strcasecmp(command, "PUSH"))
            { message.type = CMD_PUSH; sscanf(line, "%*s %d", &message.value); }
        else if (!strcasecmp(command, "POP"))
            message.type = CMD_POP;
        else if (!strcasecmp(command, "STORE"))
            { message.type = CMD_STORE; sscanf(line, "%*s %d", &message.address); }
        else if (!strcasecmp(command, "LOADM"))
            { message.type = CMD_LOADM; sscanf(line, "%*s %d", &message.address); }
        else if (!strcasecmp(command, "PRINT"))
            message.type = CMD_PRINT;
        else if (!strcasecmp(command, "SHOW") &&
                 !strcasecmp(second, "CPU"))
            message.type = CMD_SHOW_CPU;
        else if (!strcasecmp(command, "SHOW") &&
                 !strcasecmp(second, "MEMORY")) {
            message.type = CMD_SHOW_MEMORY;
            message.address = (sscanf(line, "%*s %*s %d", &a) == 1)
                              ? a : -1;
        }
        else if (!strcasecmp(command, "SHOW") &&
                 !strcasecmp(second, "STACK"))
            message.type = CMD_SHOW_STACK;
        else if (!strcasecmp(command, "SHOW") &&
                 !strcasecmp(second, "QUEUE"))
            message.type = CMD_SHOW_QUEUE;
        else {
            printf("Unknown command.\n");
            continue;
        }

        if (enqueue(message)) {
            if (dequeue(&message))
                execute(message);
        } else {
            printf("Instruction queue is full.\n");
        }
    }

    printf("Standalone simulator stopped.\n");
    return 0;
}
