/* =====================================================================
 * sync.c  -  Semaphore helpers (see sync.h)
 * Owner: Jovita (Integration)
 * ===================================================================== */
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>

#include "sim_interface.h"
#include "sync.h"

static sem_t *state_lock = SEM_FAILED;
static sem_t *core_ready = SEM_FAILED;


int sync_create(void)
{
    /* Remove leftovers from an earlier crash, then create fresh. */
    sem_unlink(SEM_STATE_LOCK);
    sem_unlink(SEM_CORE_READY);

    state_lock = sem_open(SEM_STATE_LOCK, O_CREAT | O_EXCL, 0666, 1);
    if (state_lock == SEM_FAILED) {
        perror("sync_create: state lock");
        return -1;
    }

    core_ready = sem_open(SEM_CORE_READY, O_CREAT | O_EXCL, 0666, 0);
    if (core_ready == SEM_FAILED) {
        perror("sync_create: core ready");
        sem_close(state_lock);
        sem_unlink(SEM_STATE_LOCK);
        state_lock = SEM_FAILED;
        return -1;
    }
    return 0;
}


int sync_open(void)
{
    state_lock = sem_open(SEM_STATE_LOCK, 0);
    if (state_lock == SEM_FAILED) {
        perror("sync_open: state lock (is Core running?)");
        return -1;
    }

    core_ready = sem_open(SEM_CORE_READY, 0);
    if (core_ready == SEM_FAILED) {
        perror("sync_open: core ready (is Core running?)");
        sem_close(state_lock);
        state_lock = SEM_FAILED;
        return -1;
    }
    return 0;
}


int signal_core_ready(void)
{
    return sem_post(core_ready);
}


int wait_core_ready(int seconds)
{
    struct timespec limit;
    clock_gettime(CLOCK_REALTIME, &limit);
    limit.tv_sec += seconds;

    while (sem_timedwait(core_ready, &limit) == -1) {
        if (errno == EINTR) {
            continue;                   /* a signal woke us; wait again */
        }
        if (errno == ETIMEDOUT) {
            fprintf(stderr, "wait_core_ready: Core did not become ready.\n");
        } else {
            perror("wait_core_ready");
        }
        return -1;
    }

    /* Put the token back so other processes (and later waits) also pass. */
    sem_post(core_ready);
    return 0;
}


int lock_state(void)
{
    while (sem_wait(state_lock) == -1) {
        if (errno != EINTR) {
            perror("lock_state");
            return -1;
        }
    }
    return 0;
}


int unlock_state(void)
{
    return sem_post(state_lock);
}


void sync_close(void)
{
    if (state_lock != SEM_FAILED) { sem_close(state_lock); state_lock = SEM_FAILED; }
    if (core_ready != SEM_FAILED) { sem_close(core_ready); core_ready = SEM_FAILED; }
}


void sync_destroy(void)
{
    sync_close();
    sem_unlink(SEM_STATE_LOCK);
    sem_unlink(SEM_CORE_READY);
}
