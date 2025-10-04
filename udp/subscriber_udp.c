/**
 * @file subscriber_udp.c
 * @brief Suscriptor UDP para el sistema Publicador–Suscriptor (Winsock2 / Windows)
 *
 * Este programa implementa el componente **suscriptor** (cliente receptor) del modelo
 * *Publicador–Suscriptor* sobre **UDP**, utilizando la API de **Winsock2**.
 *
 * A diferencia de TCP, el suscriptor:
 *  - No mantiene una conexión persistente con el broker.
 *  - Se registra enviando un datagrama "SUB <topic>" al broker.
 *  - Luego queda a la espera de datagramas "MSG <topic> <payload>" enviados por el broker.
 *
 * **Protocolo textual:**
 *  - Petición: `SUB <topic>\n`
 *  - Confirmación: `OK SUB <topic>\n`
 *  - Mensajes reenviados: `MSG <topic> <mensaje>\n`
 *
 * **Uso:**
 * @code
 *   subscriber_udp.exe 127.0.0.1 PartidoA
 * @endcode
 *
 * **Compilación:**
 * @code
 *   gcc subscriber_udp.c udp_utils.c -o output/subscriber_udp.exe -lws2_32
 * @endcode
 *
 * **Notas:**
 *  - El socket se crea sin bind explícito (puerto efímero).
 *  - UDP no garantiza entrega ni orden.
 *  - Se recomienda ejecutar en paralelo con broker_udp.exe y publisher_udp.exe.
 */

#include "udp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Uso:
//   subscriber_udp.exe 127.0.0.1 PartidoA

int main(int argc, char **argv) {
    // Validación de argumentos
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <host_broker> <topic>\n", argv[0]);
        return 1;
    }

    // Inicialización de Winsock
    if (winsock_init() != 0) return 1;

    const char *host  = argv[1];  // Dirección IP o hostname del broker
    const char *topic = argv[2];  // Tópico a suscribirse

    // Resolver la dirección IP y puerto del broker
    struct sockaddr_in broker;
    if (resolve_ipv4(host, BROKER_UDP_PORT, &broker) != 0) {
        fprintf(stderr, "No se pudo resolver broker %s:%d\n", host, BROKER_UDP_PORT);
        return 1;
    }

    // Crear socket UDP sin necesidad de bind (el SO asigna un puerto efímero)
    socket_t s = udp_socket_unbound();

    // Enviar comando SUB para registrar la suscripción (nuestro IP:puerto)
    char submsg[MAX_LINE];
    snprintf(submsg, sizeof(submsg), "SUB %s\n", topic);
    (void)udp_sendto_str(s, submsg, &broker);

    // Variables para recibir mensajes
    char buf[MAX_LINE];
    struct sockaddr_in src;

    // Leer confirmación inicial (opcional): "OK SUB <topic>"
    udp_recvfrom_line(s, buf, sizeof(buf), &src);
    fprintf(stderr, "%s\n", buf);

    // Bucle principal de recepción de mensajes
    while (1) {
        int n = udp_recvfrom_line(s, buf, sizeof(buf), &src);
        if (n <= 0) continue;

        // (Opcional) Validar que los mensajes provengan del broker
        // if (!same_addr(&src, &broker)) continue;

        // Imprimir mensajes: "MSG <topic> <payload>"
        printf("%s\n", buf);
        fflush(stdout);
    }

    // Cierre ordenado y limpieza
    udp_close(s);
    winsock_cleanup();
    return 0;
}
