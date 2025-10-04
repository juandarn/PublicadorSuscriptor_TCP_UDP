#include "tcp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Uso:
//   subscriber_tcp.exe 127.0.0.1 PartidoA

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <host> <topic>\n", argv[0]);
        return 1;
    }
    if (winsock_init() != 0) return 1;

    const char *host  = argv[1];
    const char *topic = argv[2];

    socket_t s = tcp_connect(host, BROKER_PORT);

    char line[MAX_LINE];
    if (readline(s, line, sizeof(line)) > 0) {
        fprintf(stderr, "%s", line); // "OK broker ready"
    }

    char subline[MAX_LINE];
    int n = snprintf(subline, sizeof(subline), "SUB %s\n", topic);
    (void)writen(s, subline, n);

    if (readline(s, line, sizeof(line)) > 0) {
        fprintf(stderr, "%s", line); // "OK SUB <topic>"
    }

    while (1) {
        int r = readline(s, line, sizeof(line));
        if (r <= 0) { fprintf(stderr, "desconectado\n"); break; }
        printf("%s", line); // "MSG <topic> <payload>"
        fflush(stdout);
    }

    tcp_close(s);
    winsock_cleanup();
    return 0;
}
