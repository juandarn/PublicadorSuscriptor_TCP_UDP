/**
 * @file broker_udp.c
 * @brief Broker UDP para el sistema Publicador–Suscriptor (Winsock2 / Windows)
 *
 * Este programa implementa el componente **broker** para un modelo
 * *Publicador–Suscriptor* sobre **UDP**, usando la API de **Winsock2**.
 *
 * A diferencia de TCP:
 *  - No hay conexiones persistentes: cada mensaje se envía como un datagrama independiente.
 *  - El broker mantiene manualmente una tabla de suscriptores (`subs[]`), asociando cada
 *    dirección (IP:puerto) con un topic.
 *
 * **Protocolo textual** (líneas terminadas en '\n'):
 *
 *  | Comando del cliente | Descripción |
 *  |----------------------|-------------|
 *  | `SUB <topic>`        | El cliente se suscribe a un topic |
 *  | `PUB <topic> <msg>`  | Un publicador envía un mensaje sobre un topic |
 *
 *  **Respuestas del broker:**
 *  - A `SUB`: `OK SUB <topic>\n`
 *  - A `PUB`: retransmite `MSG <topic> <payload>\n` a todos los suscriptores del topic.
 *  - En error: `ERR unknown command\n`
 *
 * **Uso:**
 * @code
 *   broker_udp.exe
 * @endcode
 *
 * **Compilación:**
 * @code
 *   gcc broker_udp.c udp_utils.c -o output/broker_udp.exe -lws2_32
 * @endcode
 *
 * **Notas:**
 *  - Usa UDP, por lo tanto los mensajes pueden perderse, duplicarse o llegar fuera de orden.
 *  - Ideal para comparar comportamiento con la versión TCP en el laboratorio.
 */

#include "udp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SUBS  256  ///< Tamaño máximo de la tabla de suscriptores.

/**
 * @brief Estructura que representa un suscriptor (dirección y topic asociado).
 */
typedef struct {
    int   used;                     ///< 1 si está ocupado, 0 si libre.
    char  topic[MAX_TOPIC];         ///< Nombre del topic.
    struct sockaddr_in addr;        ///< Dirección (IP + puerto) del suscriptor.
} sub_t;

static sub_t subs[MAX_SUBS];        ///< Tabla de suscriptores.

/**
 * @brief Compara dos direcciones UDP (IP y puerto).
 * @return 1 si son iguales, 0 si difieren.
 */
static int same_addr(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

/**
 * @brief Registra o actualiza un suscriptor para un topic dado.
 *
 * Si el cliente ya estaba suscrito al mismo topic, no se duplica.
 * Si no existe, se inserta en la primera posición libre.
 *
 * @param topic Nombre del topic.
 * @param addr  Dirección del cliente (IP + puerto).
 */
static void add_or_update_sub(const char *topic, const struct sockaddr_in *addr) {
    // Verificar si ya existe
    for (int i=0; i<MAX_SUBS; i++) {
        if (subs[i].used && same_addr(&subs[i].addr, addr) &&
            strncmp(subs[i].topic, topic, MAX_TOPIC) == 0) {
            return; // ya estaba registrado
        }
    }

    // Insertar nuevo
    for (int i=0; i<MAX_SUBS; i++) {
        if (!subs[i].used) {
            subs[i].used = 1;
            strncpy(subs[i].topic, topic, MAX_TOPIC-1);
            subs[i].topic[MAX_TOPIC-1] = '\0';
            subs[i].addr = *addr;
            return;
        }
    }

    fprintf(stderr, "[broker-udp] tabla de suscriptores llena\n");
}

/**
 * @brief Envía un mensaje a todos los suscriptores de un topic.
 *
 * @param topic   Tópico asociado al mensaje.
 * @param payload Contenido del mensaje.
 * @param s       Socket UDP para envío.
 */
static void broadcast_topic(const char *topic, const char *payload, socket_t s) {
    char out[MAX_LINE];
    snprintf(out, sizeof(out), "MSG %s %s\n", topic, payload);

    for (int i=0; i<MAX_SUBS; i++) {
        if (subs[i].used && strncmp(subs[i].topic, topic, MAX_TOPIC) == 0) {
            (void)udp_sendto_str(s, out, &subs[i].addr);
        }
    }
}

/**
 * @brief Programa principal: ciclo del broker UDP.
 *
 * - Inicializa Winsock.
 * - Crea socket UDP ligado a BROKER_UDP_PORT.
 * - Recibe datagramas y los procesa según el comando recibido.
 */
int main(void) {
    if (winsock_init() != 0) return 1;
    memset(subs, 0, sizeof(subs));

    socket_t s = udp_bind_any(BROKER_UDP_PORT);
    printf("[broker-udp] escuchando UDP en puerto %d...\n", BROKER_UDP_PORT);

    char buf[MAX_LINE];
    struct sockaddr_in src;

    // Bucle principal: escucha datagramas y procesa comandos
    while (1) {
        int n = udp_recvfrom_line(s, buf, sizeof(buf), &src);
        if (n <= 0) continue;

        // --- Protocolo ---
        // SUB <topic>
        // PUB <topic> <mensaje...>
        if (strncmp(buf, "SUB ", 4) == 0) {
            const char *topic = buf + 4;
            add_or_update_sub(topic, &src);

            char ok[MAX_LINE];
            snprintf(ok, sizeof(ok), "OK SUB %s\n", topic);
            (void)udp_sendto_str(s, ok, &src);

        } else if (strncmp(buf, "PUB ", 4) == 0) {
            char *p = buf + 4;
            char *sp = strchr(p, ' ');
            if (!sp) continue;
            *sp = '\0';

            const char *topic   = p;
            const char *payload = sp + 1;
            broadcast_topic(topic, payload, s);

        } else {
            const char *err = "ERR unknown command\n";
            (void)udp_sendto_str(s, err, &src);
        }
    }

    udp_close(s);
    winsock_cleanup();
    return 0;
}
