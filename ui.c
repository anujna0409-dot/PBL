/*
 * ui.c - UI Process
 *
 * The UI only talks to the Core process:
 *   cmd_fd  -> UI WRITES commands to Core
 *   resp_fd -> UI READS responses from Core
 *
 * The pipes are created by the Team Leader,
 * not inside this file.
 */

#include <signal.h>
#include "common.h"

#define INPUT_SIZE (MSG_SIZE - 8)

/* Show the menu to the user */
static void show_menu(void)
{
    printf("\n===== Multi-Process Simulator =====\n");
    printf("Type a command and press Enter.\n");
    printf("  help : show this menu\n");
    printf("  quit : exit the simulator\n");
    printf("Anything else is sent to Core as a command.\n");
    printf("===================================\n");
}

void run_ui(int cmd_fd, int resp_fd)
{
    char input[INPUT_SIZE];
    Msg resp;

    /*
     * If Core has terminated, writing to the command pipe
     * should return an error instead of terminating UI
     * because of SIGPIPE.
     */
    signal(SIGPIPE, SIG_IGN);

    show_menu();

    while (1) {
        printf("\n> ");
        fflush(stdout);

        /*
         * Read user input safely.
         */
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nInput closed. Shutting down.\n");

            if (send_msg(cmd_fd, MSG_QUIT, LOG_INFO, "quit") < 0)
                printf("Error: could not send quit to Core.\n");

            break;
        }

        /*
         * If the input does not contain '\n', the command
         * may have been too long. Remove the remaining input.
         */
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int ch;

            while ((ch = getchar()) != '\n' && ch != EOF)
                ;

            printf("Error: command too long.\n");
            continue;
        }

        /* Remove newline */
        input[strcspn(input, "\n")] = '\0';

        /* Ignore empty input */
        if (input[0] == '\0')
            continue;

        /*
         * Help is handled locally by UI.
         */
        if (strcmp(input, "help") == 0) {
            show_menu();
            continue;
        }

        /*
         * Quit command.
         */
        if (strcmp(input, "quit") == 0) {

            if (send_msg(cmd_fd, MSG_QUIT, LOG_INFO, "quit") < 0) {
                printf("Error: could not send quit to Core.\n");
            } else {
                printf("Quit sent to Core. Goodbye!\n");
            }

            break;
        }

        /*
         * Send all other commands to Core.
         */
        if (send_msg(cmd_fd, MSG_CMD, LOG_INFO, "%s", input) < 0) {
            printf("Error: could not send command to Core.\n");
            break;
        }

        /*
         * Wait for Core's response.
         */
        if (recv_msg(resp_fd, &resp) < 0) {
            printf("Error: no response from Core. Exiting.\n");
            break;
        }

        /*
         * Display Core's response.
         */
        if (resp.type == MSG_RESP) {
            printf("Core: %s\n", resp.text);
        }
        else if (resp.type == MSG_QUIT) {
            printf("Core is shutting down: %s\n", resp.text);
            break;
        }
        else {
            printf("Unexpected message type %d from Core.\n",
                   resp.type);
        }
    }
}