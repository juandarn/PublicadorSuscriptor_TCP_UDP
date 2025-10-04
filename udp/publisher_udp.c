#include "udp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Uso:
//   publisher_udp.exe 127.0.0.1 PartidoA "Gol EquipoA min32"

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <host_broker> <topic> <mensaje...>\n", argv[0]);
        return 1;
    }
    if (winsock_init() != 0) return 1;

    const char *host  = argv[1];
    const char *topic = argv[2];

    char payload[MAX_LINE]; payload[0] = '\0';
    for (int i=3; i<argc; ++i) {
        if (i>3) strncat(payload, " ", sizeof(payload)-strlen(payload)-1);
        strncat(payload, argv[i], sizeof(payload)-strlen(payload)-1);
    }

    struct sockaddr_in broker;
    if (resolve_ipv4(host, BROKER_UDP_PORT, &broker) != 0) {
        fprintf(stderr, "No se pudo resolver broker %s:%d\n", host, BROKER_UDP_PORT);
        return 1;
    }

    socket_t s = udp_socket_unbound();

    char out[MAX_LINE];
    snprintf(out, sizeof(out), "PUB %s %s\n", topic, payload);
    (void)udp_sendto_str(s, out, &broker);

    udp_close(s);
    winsock_cleanup();
    return 0;
}
