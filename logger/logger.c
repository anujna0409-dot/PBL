#include "../common.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/* Return the name of the log level */
static const char *get_log_level(int level)
{
    switch (level)
    {
        case LOG_ERROR:
            return "ERROR";
        default:
            return "INFO";
    }
}

/*
 * Logger Process
 *
 * log_fd : Read end of the Core -> Logger pipe
 * file   : Log file name
 */
void run_logger(int log_fd, const char *file)
{
    FILE *fp;
    Msg msg;

    /* Open the log file in append mode */
    fp = fopen(file, "a");

    if (fp == NULL)
    {
        perror("Logger: Unable to open log file");
        close(log_fd);
        return;
    }

    /* Receive messages until MSG_QUIT */
    while (1)
    {
        /* Receive a message from Core */
        if (recv_msg(log_fd, &msg) != 0)
        {
            fprintf(stderr, "Logger: Pipe closed or read error\n");
            break;
        }

        /* Stop the logger when Core sends MSG_QUIT */
        if (msg.type == MSG_QUIT)
        {
            break;
        }

        /* Process only MSG_LOG messages */
        if (msg.type == MSG_LOG)
        {
            const char *level;

            /* Make sure the text is always null-terminated */
            msg.text[sizeof(msg.text) - 1] = '\0';

            level = get_log_level(msg.level);

            /* Display the log on the terminal */
            printf("[%s] %s\n", level, msg.text);
            fflush(stdout);

            /* Store the log in the file */
            fprintf(fp, "[%s] %s\n", level, msg.text);

            /* Write the log immediately */
            fflush(fp);
        }
    }

    /* Cleanup */
    fclose(fp);
    close(log_fd);
}

/*
 * Reference version of recv_msg (put this in your common.c,
 * replacing the existing one, if it does not already loop on partial reads).
 *
 * int recv_msg(int fd, Msg *msg)
 * {
 *     size_t total = 0;
 *     char *p = (char *)msg;
 *
 *     while (total < sizeof(Msg))
 *     {
 *         ssize_t n = read(fd, p + total, sizeof(Msg) - total);
 *         if (n == 0)
 *             return -1;          // EOF
 *         if (n < 0)
 *         {
 *             if (errno == EINTR)
 *                 continue;
 *             return -1;
 *         }
 *         total += n;
 *     }
 *     return 0;
 * }
 */