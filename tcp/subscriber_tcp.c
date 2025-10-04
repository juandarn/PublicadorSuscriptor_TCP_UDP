/**
 * Subscriber TCP (Windows / Winsock2) para un sistema Publicador–Suscriptor.
 *
 * Rol:
 *   - Se conecta al broker TCP, recibe el banner inicial, envía el comando
 *     SUB <topic> para registrarse y luego queda escuchando mensajes del broker.
 *
 * Protocolo textual (línea terminada en '\n'):
 *   - Petición de suscripción: "SUB <topic>\n"
 *   - Confirmación del broker: "OK SUB <topic>\n"
 *   - Mensajes reenviados por el broker: "MSG <topic> <payload>\n"
 *
 * Uso:
 *   subscriber_tcp.exe 127.0.0.1 PartidoA
 *
 * Notas (Windows/Winsock):
 *   - Requiere winsock_init() antes de cualquier operación de socket y
 *     winsock_cleanup() al finalizar.
 *   - tcp_connect() establece la conexión al puerto BROKER_PORT.
 *   - readline() y writen() implementan E/S bloqueante y simple por líneas.
 */

#include "tcp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Uso:
//   subscriber_tcp.exe 127.0.0.1 PartidoA

int main(int argc, char **argv) {
    // Validación de argumentos: host y topic
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <host> <topic>\n", argv[0]);
        return 1;
    }

    // Inicializa la pila de sockets de Windows (WSAStartup).
    if (winsock_init() != 0) return 1;

    const char *host  = argv[1];  // IP o nombre del broker, ej: "127.0.0.1"
    const char *topic = argv[2];  // Tópico a suscribirse, ej: "PartidoA"

    // Conexión TCP con el broker en el puerto BROKER_PORT (definido en tcp_utils.h).
    socket_t s = tcp_connect(host, BROKER_PORT);

    // Leer banner/bienvenida del broker (opcional, informativo).
    char line[MAX_LINE];
    if (readline(s, line, sizeof(line)) > 0) {
        fprintf(stderr, "%s", line); // típico: "OK broker ready"
    }

    // Construir y enviar el comando de suscripción.
    char subline[MAX_LINE];
    int n = snprintf(subline, sizeof(subline), "SUB %s\n", topic);
    (void)writen(s, subline, n);

    // Leer confirmación de suscripción.
    if (readline(s, line, sizeof(line)) > 0) {
        fprintf(stderr, "%s", line); // esperado: "OK SUB <topic>"
    }

    // Bucle principal: quedar a la espera de mensajes del broker.
    while (1) {
        int r = readline(s, line, sizeof(line));
        if (r <= 0) {                       // desconexión del broker o error
            fprintf(stderr, "desconectado\n");
            break;
        }
        // Imprime el mensaje tal cual llega: "MSG <topic> <payload>"
        printf("%s", line);
        fflush(stdout);
    }

    // Cierre ordenado y limpieza de Winsock.
    tcp_close(s);
    winsock_cleanup();
    return 0;
}
