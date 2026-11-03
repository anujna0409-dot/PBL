#ifndef IPC_COMMON_H
#define IPC_COMMON_H

#include <mqueue.h>

/* =========================================
   POSIX MESSAGE QUEUE NAMES
   ========================================= */

#define UI_TO_CORE_QUEUE      "/ui_to_core"
#define CORE_TO_UI_QUEUE      "/core_to_ui"
#define CORE_TO_LOGGER_QUEUE  "/core_to_logger"


/* =========================================
   GENERAL CONSTANTS
   ========================================= */

#define MAX_DATA 256
#define MEMORY_SIZE 256
#define STACK_SIZE 100
#define QUEUE_SIZE 100


/* =========================================
   MESSAGE TYPES
   ========================================= */

typedef enum {

    /* ---------- UI → CORE ---------- */

    CMD_LOAD = 1,
    CMD_ADD,
    CMD_SUB,
    CMD_PUSH,
    CMD_POP,
    CMD_STORE,
    CMD_LOADM,
    CMD_PRINT,
    CMD_SHOW_CPU,
    CMD_SHOW_MEMORY,
    CMD_SHOW_STACK,
    CMD_SHOW_QUEUE,
    CMD_EXIT,


    /* ---------- CORE → UI ---------- */

    RESPONSE_SUCCESS,
    RESPONSE_ERROR,
    RESPONSE_CPU,
    RESPONSE_MEMORY,
    RESPONSE_STACK,
    RESPONSE_QUEUE,


    /* ---------- CORE → LOGGER ---------- */

    LOG_EXECUTION,
    LOG_ERROR,
    LOG_SYSTEM

} MessageType;


/* =========================================
   MESSAGE STRUCTURE
   ========================================= */

typedef struct {

    MessageType type;

    int value;

    int address;

    char data[MAX_DATA];

} Message;

#endif
