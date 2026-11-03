/* =====================================================================
 * sim_interface.h  -  Common interface for the 3-process CPU simulator
 * Owner: Jovita (Team Leader / Integration)
 *
 * Everyone includes this file so all processes agree on the names and
 * the shared data layout.
 *
 *   UI (Rishabh)   --socket-->        Core (Nishanth) --pipe--> Logger (Rithvik)
 *   UI and Core    <-- shared memory (state), guarded by SEMAPHORES (Jovita)
 * ===================================================================== */
#ifndef SIM_INTERFACE_H
#define SIM_INTERFACE_H

#include "ipc_common.h"     /* Message, MEMORY_SIZE, STACK_SIZE, QUEUE_SIZE */

/* ---------- Socket (UI <-> Core) : Rishabh + Nishanth ---------- */
#define SIM_SOCKET_PATH   "/tmp/sim_core.sock"

/* ---------- Shared memory (Core writes, UI reads) : Nishanth ---------- */
#define SIM_SHM_NAME      "/sim_state"

typedef struct {
    int accumulator;
    int program_counter;

    int memory[MEMORY_SIZE];

    int stack[STACK_SIZE];
    int stack_top;

    int queue_count;            /* how many instructions are waiting */
} SharedState;

/* ---------- Semaphores (names) : Jovita ---------- */
#define SEM_STATE_LOCK    "/sim_state_lock"   /* starts at 1: one user at a time   */
#define SEM_CORE_READY    "/sim_core_ready"   /* starts at 0: Core posts when ready */

/* ---------- Pipe (Core -> Logger) : Rithvik ----------
 * Core writes whole Message structs (see ipc_common.h) into the pipe. */

#endif
