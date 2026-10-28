/* Deadlock Simulation - HTTP server (Leader). No external libraries.
   Build (from project root):  gcc -Wall -Wextra backend/*.c -o server   (Windows MinGW: add -lws2_32)
   Run   (from project root):  ./server   then open http://localhost:8080 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "common.h"

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET sock_t;
#define CLOSESOCK closesocket
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
typedef int sock_t;
#define CLOSESOCK close
#endif

#define PORT 8080
static State S;
static char out[32768];
static int olen;

/* ---------- tiny JSON writer ---------- */
static void J(const char *f, ...) {
    va_list a; va_start(a, f);
    olen += vsnprintf(out + olen, sizeof out - olen, f, a);
    va_end(a);
}
static void jvec(const int *v, int n) {
    J("[");
    for (int i = 0; i < n; i++) J("%s%d", i ? "," : "", v[i]);
    J("]");
}
static void jmat(int a[][MAXR], int n, int m) {
    J("[");
    for (int i = 0; i < n; i++) { if (i) J(","); jvec(a[i], m); }
    J("]");
}

/* ---------- full system status as JSON (used by every route) ---------- */
static void status_json(const char *msg) {
    int seq[MAXP + 1], dead[MAXP], safe = 0, nd = 0, i, j;
    double per[MAXR] = {0}, all = 0;
    olen = 0;
    if (S.n > 0) {
        compute_need(&S);
        safe = is_safe(&S, seq);
        nd = detect_deadlock(&S, dead);
        utilization(&S, per, &all);
    }
    const char *state = S.n == 0 ? "NOT_SET" : nd ? "DEADLOCKED" : safe ? "SAFE" : "UNSAFE";

    J("{\"message\":\"%s\",\"state\":\"%s\",\"n\":%d,\"m\":%d,", msg, state, S.n, S.m);
    J("\"total\":");      jvec(S.total, S.m);
    J(",\"available\":"); jvec(S.avail, S.m);
    J(",\"allocation\":"); jmat(S.alloc, S.n, S.m);
    J(",\"max\":");       jmat(S.max, S.n, S.m);
    J(",\"need\":");      jmat(S.need, S.n, S.m);
    J(",\"requests\":");  jmat(S.req, S.n, S.m);
    J(",\"active\":");    jvec(S.active, S.n);
    J(",\"safeSequence\":[");
    if (safe) for (i = 0; seq[i] >= 0; i++) J("%s%d", i ? "," : "", seq[i]);
    J("],\"deadlocked\":"); jvec(dead, nd);
    J(",\"utilization\":{\"overall\":%.1f,\"perResource\":[", all);
    for (j = 0; j < S.m; j++) J("%s%.1f", j ? "," : "", per[j]);
    J("]},\"edges\":");
    olen += rag_write_json(&S, out + olen, sizeof out - olen);   /* Resource Allocation Graph (rag.c) */
    J("}");
}

static int nextint(const char **s, int *v) {
    char *e; long x = strtol(*s, &e, 10);
    if (e == *s) { *v = 0; return 0; }
    *v = (int)x; *s = e; return 1;
}

/* ---------- API routes ---------- */
static void handle(const char *path, const char *body) {
    char msg[200] = "OK";
    int i, j;

    if (!strcmp(path, "/api/setup")) {
        /* body: n m | Available[m] | Allocation[n*m] | Max[n*m]  (plain whitespace-separated ints) */
        const char *q = body; int n, m, ok = 1;
        if (!nextint(&q, &n) || !nextint(&q, &m) || n < 1 || m < 1 || n > MAXP || m > MAXR) {
            status_json("Invalid sizes (1-10 processes, 1-10 resources)"); return;
        }
        State T; memset(&T, 0, sizeof T); T.n = n; T.m = m;
        for (j = 0; j < m; j++) ok &= nextint(&q, &T.avail[j]);
        for (i = 0; i < n; i++) for (j = 0; j < m; j++) ok &= nextint(&q, &T.alloc[i][j]);
        for (i = 0; i < n; i++) for (j = 0; j < m; j++) ok &= nextint(&q, &T.max[i][j]);
        for (j = 0; j < m; j++) if (T.avail[j] < 0) ok = 0;
        for (i = 0; i < n; i++) for (j = 0; j < m; j++)
            if (T.alloc[i][j] < 0 || T.alloc[i][j] > T.max[i][j]) ok = 0;
        if (!ok) { status_json("Invalid data (need all numbers, no negatives, Allocation <= Max)"); return; }
        for (i = 0; i < n; i++) T.active[i] = 1;
        for (j = 0; j < m; j++) {
            T.total[j] = T.avail[j];
            for (i = 0; i < n; i++) T.total[j] += T.alloc[i][j];
        }
        S = T; compute_need(&S);
        status_json("System created");
    }
    else if (!strcmp(path, "/api/status")) status_json("Status");
    else if (S.n == 0) status_json("Create the system first (Setup)");
    else if (!strcmp(path, "/api/request")) {
        /* body: p r0 r1 ... r(m-1) */
        const char *q = body; int p, r[MAXR] = {0}, ok = nextint(&q, &p);
        for (j = 0; j < S.m; j++) ok &= nextint(&q, &r[j]);
        if (!ok) status_json("Bad request format");
        else { request_resources(&S, p, r, msg); status_json(msg); }
    }
    else if (!strcmp(path, "/api/safety")) {
        int seq[MAXP + 1];
        compute_need(&S);
        if (is_safe(&S, seq)) {
            char *w = msg; w += sprintf(w, "SAFE. Safe sequence:");
            for (i = 0; seq[i] >= 0; i++) w += sprintf(w, " P%d", seq[i]);
        } else strcpy(msg, "UNSAFE: no safe sequence exists");
        status_json(msg);
    }
    else if (!strcmp(path, "/api/detect")) {
        int dead[MAXP], nd = detect_deadlock(&S, dead);
        if (nd == 0) strcpy(msg, "No deadlock detected");
        else { char *w = msg; w += sprintf(w, "DEADLOCK! Processes involved:");
               for (i = 0; i < nd; i++) w += sprintf(w, " P%d", dead[i]); }
        status_json(msg);
    }
    else if (!strcmp(path, "/api/recover")) { recover_one(&S, msg); status_json(msg); }
    else status_json("Unknown route");
}

/* ---------- minimal HTTP server ---------- */
static void send_all(sock_t c, const char *d, int len) {
    while (len > 0) { int k = send(c, d, len, 0); if (k <= 0) return; d += k; len -= k; }
}
static void respond(sock_t c, int code, const char *type, const char *body, int len) {
    char h[256];
    int hl = snprintf(h, sizeof h, "HTTP/1.1 %d OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", code, type, len);
    send_all(c, h, hl); send_all(c, body, len);
}
static void serve_file(sock_t c, const char *path) {
    char fn[300];
    if (strstr(path, "..")) { respond(c, 403, "text/plain", "Forbidden", 9); return; }
    snprintf(fn, sizeof fn, "frontend%s%s", path, strcmp(path, "/") ? "" : "index.html");
    FILE *f = fopen(fn, "rb");
    if (!f) { respond(c, 404, "text/plain", "Not found", 9); return; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *data = malloc(sz + 1); fread(data, 1, sz, f); fclose(f);
    const char *t = strstr(fn, ".js") ? "application/javascript" : strstr(fn, ".css") ? "text/css" : "text/html";
    respond(c, 200, t, data, (int)sz); free(data);
}

int main(void) {
#ifdef _WIN32
    WSADATA w; WSAStartup(MAKEWORD(2, 2), &w);
#endif
    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1; setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof yes);
    struct sockaddr_in a; memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = htons(PORT);
    if (bind(srv, (struct sockaddr *)&a, sizeof a) < 0 || listen(srv, 8) < 0) { perror("bind/listen"); return 1; }
    printf("Deadlock Simulator running at http://localhost:%d\n", PORT);

    for (;;) {
        sock_t c = accept(srv, NULL, NULL);
        char buf[65536]; int len = 0, hdr = -1, want = 0;
        while (len < (int)sizeof buf - 1) {
            int k = recv(c, buf + len, sizeof buf - 1 - len, 0);
            if (k <= 0) break;
            len += k; buf[len] = 0;
            char *e = strstr(buf, "\r\n\r\n");
            if (e) {
                hdr = (int)(e - buf) + 4;
                char *cl = strstr(buf, "Content-Length:"); if (!cl) cl = strstr(buf, "content-length:");
                want = cl ? atoi(cl + 15) : 0;
                if (len >= hdr + want) break;
            }
        }
        if (hdr >= 0) {
            char method[8], path[256];
            if (sscanf(buf, "%7s %255s", method, path) == 2) {
                if (!strncmp(path, "/api/", 5)) { handle(path, buf + hdr); respond(c, 200, "application/json", out, olen); }
                else serve_file(c, path);
            }
        }
        CLOSESOCK(c);
    }
}
