#include "udp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Uso:
//   subscriber_udp.exe 127.0.0.1 PartidoA

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <host_broker> <topic>\n", argv[0]);
        return 1;
    }
    if (winsock_init() != 0) return 1;

    const char *host  = argv[1];
    const char *topic = argv[2];

    struct sockaddr_in broker;
    if (resolve_ipv4(host, BROKER_UDP_PORT, &broker) != 0) {
        fprintf(stderr, "No se pudo resolver broker %s:%d\n", host, BROKER_UDP_PORT);
        return 1;
    }

    socket_t s = udp_socket_unbound();

    // Enviar SUB para registrar nuestro (IP:puerto) en el broker
    char submsg[MAX_LINE];
    snprintf(submsg, sizeof(submsg), "SUB %s\n", topic);
    (void)udp_sendto_str(s, submsg, &broker);

    // Esperar confirmación y luego mensajes
    char buf[MAX_LINE];
    struct sockaddr_in src;

    // (opcional) leer un OK SUB
    udp_recvfrom_line(s, buf, sizeof(buf), &src);
    fprintf(stderr, "%s\n", buf);

    // loop de recepción
    while (1) {
        int n = udp_recvfrom_line(s, buf, sizeof(buf), &src);
        if (n <= 0) continue;
        // Puedes verificar que venga del broker (src == broker) si quieres
        printf("%s\n", buf); // "MSG <topic> <payload>"
        fflush(stdout);
    }

    udp_close(s);
    winsock_cleanup();
    return 0;
}
