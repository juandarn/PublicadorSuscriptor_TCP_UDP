/**
 * @file publisher_udp.c
 * @brief Publicador UDP para el sistema Publicador–Suscriptor (Winsock2 / Windows)
 *
 * Este programa implementa el componente **publisher** (emisor) del modelo
 * *Publicador–Suscriptor* sobre **UDP**, utilizando la API de **Winsock2**.
 *
 * A diferencia del caso TCP, no existe conexión persistente: el publicador envía
 * un único datagrama al broker con la información del topic y el mensaje.
 *
 * **Protocolo textual**:
 *   - Formato del datagrama enviado:
 *     @code
 *     PUB <topic> <mensaje...>\n
 *     @endcode
 *
 * El broker UDP recibe este mensaje y lo retransmite a todos los suscriptores
 * registrados en ese topic.
 *
 * **Uso:**
 * @code
 *   publisher_udp.exe 127.0.0.1 PartidoA "Gol EquipoA min32"
 * @endcode
 *
 * **Compilación:**
 * @code
 *   gcc publisher_udp.c udp_utils.c -o output/publisher_udp.exe -lws2_32
 * @endcode
 *
 * **Notas:**
 *  - No hay confirmación del broker: el envío es “fire and forget”.
 *  - UDP no garantiza entrega ni orden de los datagramas.
 *  - Ideal para análisis de pérdida de paquetes en Wireshark.
 */

#include "udp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Uso:
//   publisher_udp.exe 127.0.0.1 PartidoA "Gol EquipoA min32"

int main(int argc, char **argv) {
    // Verificar que se proporcionen todos los argumentos necesarios.
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <host_broker> <topic> <mensaje...>\n", argv[0]);
        return 1;
    }

    // Inicializa la pila Winsock (obligatorio en Windows).
    if (winsock_init() != 0) return 1;

    const char *host  = argv[1];  // Dirección del broker, ej: "127.0.0.1"
    const char *topic = argv[2];  // Tópico del mensaje (ej. "PartidoA")

    // Construir el mensaje de texto concatenando argv[3..]
    char payload[MAX_LINE];
    payload[0] = '\0';
    for (int i = 3; i < argc; ++i) {
        if (i > 3) strncat(payload, " ", sizeof(payload) - strlen(payload) - 1);
        strncat(payload, argv[i], sizeof(payload) - strlen(payload) - 1);
    }

    // Resolver dirección IP y puerto del broker
    struct sockaddr_in broker;
    if (resolve_ipv4(host, BROKER_UDP_PORT, &broker) != 0) {
        fprintf(stderr, "No se pudo resolver broker %s:%d\n", host, BROKER_UDP_PORT);
        return 1;
    }

    // Crear socket UDP sin necesidad de bind() explícito (puerto efímero)
    socket_t s = udp_socket_unbound();

    // Formatear y enviar el datagrama con el comando PUB
    char out[MAX_LINE];
    snprintf(out, sizeof(out), "PUB %s %s\n", topic, payload);
    (void)udp_sendto_str(s, out, &broker);

    // Cierre ordenado
    udp_close(s);
    winsock_cleanup();
    return 0;
}
