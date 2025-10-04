/**
 * Broker TCP (Windows / Winsock2) para un sistema Publicador–Suscriptor.
 *
 * Protocolo textual (línea terminada en '\n'):
 *   - SUB <topic>                 -> Un cliente se registra como suscriptor de <topic>.
 *   - PUB <topic> <mensaje...>    -> Un cliente publica un <mensaje> para el <topic>.
 *   - Respuesta a SUB: "OK SUB <topic>\n"
 *   - Reenvío a suscriptores: "MSG <topic> <payload>\n"
 *
 * Diseño:
 *   - Este broker acepta múltiples conexiones TCP y usa select() para multiplexar I/O.
 *   - Cada cliente puede ser "suscriptor" de un único topic (campo is_subscriber=1 y topic asignado).
 *   - Los "publishers" no necesitan identificarse; envían "PUB ..." y el broker reenvía a quienes estén suscritos.
 *
 * Notas (Windows):
 *   - Requiere inicializar Winsock con winsock_init() y limpiar con winsock_cleanup().
 *   - Cierre de sockets con tcp_close() (closesocket) en lugar de close().
 *   - select() utiliza FD_SET/FD_ISSET/FD_ZERO y FD_SETSIZE como límite de descriptores.
 */

#include "tcp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Número máximo de clientes manejados por select().
 * FD_SETSIZE es definido por Winsock; por defecto es suficiente para una práctica
 * de laboratorio. Si esperas MUCHAS conexiones, considera un poll/IOCP o select
 * con ajuste de FD_SETSIZE en compilación.
 */
#define MAX_CLIENTS  FD_SETSIZE

/* Estructura de cliente:
 *  - fd: socket del cliente
 *  - topic: si el cliente es suscriptor, aquí se guarda el topic al que está suscrito
 *  - is_subscriber: 1 si es suscriptor; 0 si no (publisher o desconocido)
 */
typedef struct {
    socket_t fd;
    char     topic[MAX_TOPIC]; // si es sub, guarda su tópico
    int      is_subscriber;    // 1=sub, 0=publisher/unknown
} client_t;

/* Tabla de clientes:
 *  - Índice i en [0, MAX_CLIENTS) identifica una "ranura" de cliente.
 *  - Una ranura vacía se marca con fd == INVALID_SOCKET.
 */
static client_t clients[MAX_CLIENTS];

/* trim_newline: elimina '\r' o '\n' al final de una cadena (si aparecen). */
static void trim_newline(char *s) {
    for (int i=0; s[i]; ++i)
        if (s[i]=='\r' || s[i]=='\n') { s[i]=0; break; }
}

/* broadcast_to_topic:
 *  - Construye una línea "MSG <topic> <payload>\n"
 *  - La envía a todos los clientes suscriptores cuyo topic coincide exactamente.
 */
static void broadcast_to_topic(const char *topic, const char *payload) {
    char out[MAX_LINE];
    int n = snprintf(out, sizeof(out), "MSG %s %s\n", topic, payload);
    if (n < 0) return;

    for (int i=0;i<MAX_CLIENTS;i++) {
        if (clients[i].fd != INVALID_SOCKET &&
            clients[i].is_subscriber == 1 &&
            strncmp(clients[i].topic, topic, MAX_TOPIC) == 0) {
            (void)writen(clients[i].fd, out, n);
        }
    }
}

/* handle_line:
 *   - Procesa un comando textual de un cliente (índice idx en la tabla).
 *   - Comandos soportados:
 *       SUB <topic>
 *       PUB <topic> <mensaje...>
 *     Cualquier otro comando responde con "ERR unknown command\n".
 */
static void handle_line(int idx, char *line) {
    trim_newline(line);

    // SUB <topic>  -> el cliente se registra como suscriptor del topic
    if (strncmp(line, "SUB ", 4) == 0) {
        char *topic = line + 4;

        // Evitar overflow si envían un topic larguísimo
        if ((int)strlen(topic) >= MAX_TOPIC) topic[MAX_TOPIC-1] = '\0';

        // Guardar estado del cliente como suscriptor
        strncpy(clients[idx].topic, topic, MAX_TOPIC);
        clients[idx].is_subscriber = 1;

        // Confirmación
        char ok[MAX_LINE];
        int n = snprintf(ok, sizeof(ok), "OK SUB %s\n", clients[idx].topic);
        (void)writen(clients[idx].fd, ok, n);

    // PUB <topic> <mensaje...>  -> reenviar a todos los suscriptores de ese topic
    } else if (strncmp(line, "PUB ", 4) == 0) {
        char *p = line + 4;
        char *space = strchr(p, ' ');
        if (!space) return; // formato inválido (sin payload)
        *space = '\0';

        const char *topic   = p;
        const char *payload = space + 1;

        broadcast_to_topic(topic, payload);

    } else {
        // Comando no reconocido
        const char *err = "ERR unknown command\n";
        (void)writen(clients[idx].fd, err, (int)strlen(err));
    }
}

int main(void) {
    // Inicializa la pila de Winsock (WSAStartup). Obligatorio en Windows.
    if (winsock_init() != 0) return 1;

    // Crea socket de escucha, lo liga a INADDR_ANY:PORT y lo pone en listen()
    socket_t listenfd = tcp_listen_any(BROKER_PORT);
    printf("[broker] escuchando en puerto %d...\n", BROKER_PORT);

    // Inicializa tabla de clientes a "vacío"
    for (int i=0;i<MAX_CLIENTS;i++) clients[i].fd = INVALID_SOCKET;

    // Conjuntos de descriptores para select()
    fd_set allset, rset;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    socket_t maxfd = listenfd;  // máximo descriptor a vigilar

    while (1) {
        // rset es el conjunto "temporal" que select va a modificar
        rset = allset;

        // Bloquea hasta que haya sockets listos para leer
        int nready = select((int)maxfd+1, &rset, NULL, NULL, NULL);
        if (nready == SOCKET_ERROR) {
            fprintf(stderr, "select() err: %d\n", WSAGetLastError());
            break;
        }

        // ¿Hay una nueva conexión entrante en el listenfd?
        if (FD_ISSET(listenfd, &rset)) {
            struct sockaddr_in cliaddr; int len = sizeof(cliaddr);
            socket_t connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            if (connfd != INVALID_SOCKET) {
                // Buscar un hueco libre en la tabla de clientes
                int i;
                for (i=0;i<MAX_CLIENTS;i++)
                    if (clients[i].fd == INVALID_SOCKET) { clients[i].fd = connfd; break; }

                if (i == MAX_CLIENTS) {
                    // Sin espacio: rechazar y avisar
                    const char *msg="ERR too many clients\n";
                    writen(connfd, msg, (int)strlen(msg));
                    tcp_close(connfd);
                } else {
                    // Inicializar estado del nuevo cliente
                    clients[i].is_subscriber = 0;
                    clients[i].topic[0] = '\0';

                    // Añadir a la lista vigilada por select()
                    FD_SET(connfd, &allset);
                    if (connfd > maxfd) maxfd = connfd;

                    // Enviar banner informativo
                    const char *hello="OK broker ready\n";
                    writen(connfd, hello, (int)strlen(hello));
                }
            }
            if (--nready <= 0) continue; // No quedan más sockets listos
        }

        // Iterar sobre todos los clientes y procesar los que tengan datos
        for (int i=0;i<MAX_CLIENTS;i++) {
            socket_t fd = clients[i].fd;
            if (fd == INVALID_SOCKET) continue;

            if (FD_ISSET(fd, &rset)) {
                char line[MAX_LINE];

                // readline() lee hasta '\n' o cierre del peer
                int n = readline(fd, line, sizeof(line));
                if (n <= 0) {
                    // El cliente cerró o error: limpiar su estado y sacarlo del set
                    tcp_close(fd);
                    FD_CLR(fd, &allset);
                    clients[i].fd = INVALID_SOCKET;
                    continue;
                }

                // Despachar el comando recibido
                handle_line(i, line);
            }
        }
    }

    // Cierre ordenado del socket de escucha y limpieza de Winsock
    tcp_close(listenfd);
    winsock_cleanup();
    return 0;
}
