/* common.h - shared message format + pipe helpers (agreed interface for all 3 processes) */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

#define MSG_SIZE 256            /* fixed size < PIPE_BUF => each write() is atomic */

enum { MSG_CMD, MSG_RESP, MSG_LOG, MSG_QUIT };
enum { LOG_INFO, LOG_ERROR };

typedef struct {
    int  type;                  /* MSG_CMD / MSG_RESP / MSG_LOG / MSG_QUIT */
    int  level;                 /* LOG_INFO / LOG_ERROR (used by MSG_LOG)  */
    char text[MSG_SIZE - 8];
} Msg;

static inline int send_msg(int fd, int type, int level, const char *fmt, ...)
{
    Msg m;
    memset(&m, 0, sizeof m);
    m.type = type;
    m.level = level;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m.text, sizeof m.text, fmt, ap);
    va_end(ap);
    return write(fd, &m, sizeof m) == (ssize_t)sizeof m ? 0 : -1;
}

static inline int recv_msg(int fd, Msg *m)
{
    size_t got = 0;
    while (got < sizeof *m) {
        ssize_t n = read(fd, (char *)m + got, sizeof *m - got);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;  /* EOF or error */
        got += (size_t)n;
    }
    return 0;
}

void run_ui(int cmd_fd, int resp_fd);                 /* ui.c     */
void run_core(int cmd_fd, int resp_fd, int log_fd);   /* core.c   */
void run_logger(int log_fd, const char *file);        /* logger.c */

#endif
