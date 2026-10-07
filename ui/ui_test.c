
/* ui_test.c - TEMPORARY test harness (not part of the project) */

#include <sys/wait.h>
#include <string.h>
#include "common.h"

/* Fake Core: echoes every command back as MSG_RESP */
static void fake_core(int cmd_rd, int resp_wr)
{
    Msg m;
    FILE *trace = fopen("core_trace.txt", "w");

    if (trace == NULL)
        _exit(1);

    while (recv_msg(cmd_rd, &m) == 0) {
        fprintf(trace, "type=%d text=[%s]\n", m.type, m.text);
        fflush(trace);

        if (m.type == MSG_QUIT)
            break;

        if (strcmp(m.text, "die") == 0)      /* simulate a Core crash */
            break;

        send_msg(resp_wr, MSG_RESP, LOG_INFO, "OK echo: %s", m.text);
    }

    fclose(trace);
    _exit(0);
}

int main(void)
{
    int cmd_pipe[2], resp_pipe[2];

    if (pipe(cmd_pipe) < 0 || pipe(resp_pipe) < 0) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {                          /* child = fake Core */
        close(cmd_pipe[1]);
        close(resp_pipe[0]);

        fake_core(cmd_pipe[0], resp_pipe[1]);
    }

    close(cmd_pipe[0]);                      /* parent = UI */
    close(resp_pipe[1]);

    run_ui(cmd_pipe[1], resp_pipe[0]);

    close(cmd_pipe[1]);
    close(resp_pipe[0]);

    waitpid(pid, NULL, 0);

    printf("[harness] run_ui returned\n");

    return 0;
}


