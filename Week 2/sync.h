/* =====================================================================
 * sync.h  -  Semaphore helpers for the 3-process CPU simulator
 * Owner: Jovita (Integration)
 *
 * Two named POSIX semaphores (see sim_interface.h):
 *   SEM_STATE_LOCK  - protects the shared memory (mutual exclusion)
 *   SEM_CORE_READY  - Core tells UI "shared memory is ready, you can start"
 *
 * All functions return 0 on success and -1 on failure (errno is set).
 * Link with:  -pthread   (and -lrt on older systems)
 * ===================================================================== */
#ifndef SYNC_H
#define SYNC_H

/* CORE calls this first: creates both semaphores fresh.
 * (Removes old ones left over from a crash.) */
int sync_create(void);

/* UI and LOGGER call this: opens the semaphores Core created. */
int sync_open(void);

/* CORE: shared memory is created and filled, UI may start. */
int signal_core_ready(void);

/* UI: wait until Core is ready. Gives up after 'seconds'. */
int wait_core_ready(int seconds);

/* Use around EVERY read or write of the shared memory:
 *     lock_state();  ...touch SharedState...  unlock_state();
 * Keep the locked part short. */
int lock_state(void);
int unlock_state(void);

/* Every process: close my handles. */
void sync_close(void);

/* CORE only, at exit: close and remove the semaphore names. */
void sync_destroy(void);

#endif
