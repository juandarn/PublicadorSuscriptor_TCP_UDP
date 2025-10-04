/**
 * Publisher TCP (Windows / Winsock2) para un sistema Publicador–Suscriptor.
 *
 * Rol:
 *   - Conecta al broker TCP, espera el banner inicial y envía un comando
 *     PUB con el topic y el mensaje (payload) que compone a partir de argv.
 *
 * Protocolo textual (líneas terminadas en '\n'):
 *   - Petición:  "PUB <topic> <mensaje...>\n"
 *   - Respuesta esperada del broker: no requerida para el publisher (envío fire-and-forget)
 *
 * Uso:
 *   publisher_tcp.exe 127.0.0.1 PartidoA "Gol EquipoA min32"
 *
 * Notas (Windows/Winsock):
 *   - Se debe inicializar Winsock con winsock_init() antes de usar sockets
 *     y limpiar con winsock_cleanup() al final.
 *   - La conexión TCP se establece con tcp_connect(host, BROKER_PORT).
 *   - Las utilidades readline() y writen() son bloqueantes y simples.
 */

#include "tcp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Uso:
//   publisher_tcp.exe 127.0.0.1 PartidoA "Gol EquipoA min32"

int main(int argc, char **argv) {
    // Validación mínima de argumentos: host, topic y al menos una palabra de mensaje.
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <host> <topic> <mensaje...>\n", argv[0]);
        return 1;
    }

    // Inicializa la pila de sockets de Windows.
    if (winsock_init() != 0) return 1;

    const char *host  = argv[1];  // IP o nombre del broker (p.ej., "127.0.0.1")
    const char *topic = argv[2];  // Tópico al que se publica (p.ej., "PartidoA")

    // Construir el payload uniendo argv[3..] con espacios.
    char payload[MAX_LINE];
    payload[0] = '\0';
    for (int i=3; i<argc; ++i) {
        if (i>3) strncat(payload, " ", sizeof(payload)-strlen(payload)-1);
        strncat(payload, argv[i], sizeof(payload)-strlen(payload)-1);
    }

    // Establecer conexión TCP con el broker en BROKER_PORT (definido en tcp_utils.h).
    socket_t s = tcp_connect(host, BROKER_PORT);

    // (Opcional) Leer el banner inicial del broker, si lo envía (p.ej., "OK broker ready").
    char line[MAX_LINE];
    (void)readline(s, line, sizeof(line)); // ignoramos el contenido; solo sincroniza

    // Formatear y enviar el comando PUB con topic + payload.
    char out[MAX_LINE];
    int n = snprintf(out, sizeof(out), "PUB %s %s\n", topic, payload);
    (void)writen(s, out, n);

    // Cierre ordenado y limpieza de Winsock.
    tcp_close(s);
    winsock_cleanup();
    return 0;
}
