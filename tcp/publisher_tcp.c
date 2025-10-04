#include "tcp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Uso:
//   publisher_tcp.exe 127.0.0.1 PartidoA "Gol EquipoA min32"

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <host> <topic> <mensaje...>\n", argv[0]);
        return 1;
    }
    if (winsock_init() != 0) return 1;

    const char *host  = argv[1];
    const char *topic = argv[2];

    char payload[MAX_LINE];
    payload[0] = '\0';
    for (int i=3; i<argc; ++i) {
        if (i>3) strncat(payload, " ", sizeof(payload)-strlen(payload)-1);
        strncat(payload, argv[i], sizeof(payload)-strlen(payload)-1);
    }

    socket_t s = tcp_connect(host, BROKER_PORT);

    char line[MAX_LINE];
    (void)readline(s, line, sizeof(line)); // banner opcional

    char out[MAX_LINE];
    int n = snprintf(out, sizeof(out), "PUB %s %s\n", topic, payload);
    (void)writen(s, out, n);

    tcp_close(s);
    winsock_cleanup();
    return 0;
}
