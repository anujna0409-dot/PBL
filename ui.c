/* ui.c - User Interface process */

#include "common.h"

void run_ui(int cmd_fd, int resp_fd)
{
    char input[MSG_SIZE];

    printf("\n=================================\n");
    printf("        UI PROCESS STARTED\n");
    printf("=================================\n");

    while (1) {
        printf("\nEnter command (type quit to exit): ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0)
            continue;

        if (strcmp(input, "quit") == 0) {
            send_msg(cmd_fd, MSG_QUIT, LOG_INFO, "quit");
            printf("UI: Quit command sent.\n");
            break;
        }

        if (send_msg(cmd_fd, MSG_CMD, LOG_INFO, "%s", input) < 0) {
            perror("UI: failed to send command");
            break;
        }

        Msg response;

        if (recv_msg(resp_fd, &response) < 0) {
            perror("UI: failed to receive response");
            break;
        }

        if (response.type == MSG_RESP) {
            printf("Core: %s\n", response.text);
        } else if (response.type == MSG_LOG) {
            printf("Log: %s\n", response.text);
        } else if (response.type == MSG_QUIT) {
            printf("Core requested UI to quit.\n");
            break;
        }
    }

    close(cmd_fd);
    close(resp_fd);

    printf("\nUI process terminated.\n");
}
