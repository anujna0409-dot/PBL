#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "common.h"

int main(void)
{
    int ui_to_core[2];
    int core_to_ui[2];
    int core_to_logger[2];

    pid_t ui_pid;
    pid_t core_pid;
    pid_t logger_pid;

    /* Create pipes */

    if (pipe(ui_to_core) == -1 ||
        pipe(core_to_ui) == -1 ||
        pipe(core_to_logger) == -1) {

        perror("pipe");
        return EXIT_FAILURE;
    }

    /*
     * ------------------------------------------------
     * Create Logger process
     * ------------------------------------------------
     */

    logger_pid = fork();

    if (logger_pid == -1) {
        perror("fork logger");
        return EXIT_FAILURE;
    }

    if (logger_pid == 0) {

        /* Logger only reads from core_to_logger */

        close(ui_to_core[0]);
        close(ui_to_core[1]);

        close(core_to_ui[0]);
        close(core_to_ui[1]);

        close(core_to_logger[1]);

        run_logger(core_to_logger[0], "simulator.log");

        _exit(EXIT_SUCCESS);
    }

    /*
     * ------------------------------------------------
     * Create Core process
     * ------------------------------------------------
     */

    core_pid = fork();

    if (core_pid == -1) {
        perror("fork core");
        return EXIT_FAILURE;
    }

    if (core_pid == 0) {

        /*
         * Core:
         *
         * reads  from UI
         * writes to UI
         * writes to Logger
         */

        close(ui_to_core[1]);

        close(core_to_ui[0]);

        close(core_to_logger[0]);

        run_core(
            ui_to_core[0],
            core_to_ui[1],
            core_to_logger[1]
        );

        _exit(EXIT_SUCCESS);
    }

    /*
     * ------------------------------------------------
     * Create UI process
     * ------------------------------------------------
     */

    ui_pid = fork();

    if (ui_pid == -1) {
        perror("fork ui");
        return EXIT_FAILURE;
    }

    if (ui_pid == 0) {

        /*
         * UI:
         *
         * writes commands to Core
         * reads responses from Core
         */

        close(ui_to_core[0]);

        close(core_to_ui[1]);

        close(core_to_logger[0]);
        close(core_to_logger[1]);

        run_ui(
            ui_to_core[1],
            core_to_ui[0]
        );

        _exit(EXIT_SUCCESS);
    }

    /*
     * ------------------------------------------------
     * Parent / Team Leader
     * ------------------------------------------------
     *
     * Parent does not use any pipe.
     */

    close(ui_to_core[0]);
    close(ui_to_core[1]);

    close(core_to_ui[0]);
    close(core_to_ui[1]);

    close(core_to_logger[0]);
    close(core_to_logger[1]);

    /*
     * Wait for all three processes.
     */

    waitpid(ui_pid, NULL, 0);
    waitpid(core_pid, NULL, 0);
    waitpid(logger_pid, NULL, 0);

    printf("All three processes terminated.\n");
    printf("Integration complete.\n");

    return EXIT_SUCCESS;
}