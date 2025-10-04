/**
 * @file tcp_utils.c
 * @brief Utilidades de red para TCP sobre Windows (Winsock2).
 *
 * Provee funciones de inicialización/cierre de Winsock, helpers para:
 * - Crear un socket en escucha (server) ligado a INADDR_ANY:PORT.
 * - Conectar a un host:port (cliente).
 * - Cambiar a modo no bloqueante.
 * - Lectura bloqueante por líneas (hasta '\n').
 * - Escritura garantizada de un buffer completo.
 *
 * Requisitos:
 *  - Llamar a winsock_init() antes de usar cualquier función de sockets.
 *  - Llamar a winsock_cleanup() al terminar el programa.
 *
 * Notas:
 *  - Diseñado para Windows: usa SOCKET, closesocket(), ioctlsocket() y WSA*.
 *  - Enlazar con ws2_32 (gcc: -lws2_32, MSVC: ws2_32.lib).
 */

#define _CRT_SECURE_NO_WARNINGS
#include "tcp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Inicializa la librería Winsock (WSAStartup).
 * @return 0 si ok, -1 si falla.
 */
int winsock_init(void) {
    WSADATA wsa;
    int r = WSAStartup(MAKEWORD(2,2), &wsa);
    if (r != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", r);
        return -1;
    }
    return 0;
}

/**
 * @brief Limpia la librería Winsock (WSACleanup).
 */
void winsock_cleanup(void) {
    WSACleanup();
}

/**
 * @brief Cierra un socket (alias de closesocket()).
 * @param s Socket a cerrar.
 * @return 0 si ok, SOCKET_ERROR si falla.
 */
int tcp_close(socket_t s) {
    return CLOSESOCK(s);
}

/**
 * @brief Crea un socket servidor TCP, lo liga a INADDR_ANY:port y lo pone en listen().
 *
 * @param port Puerto local a escuchar (host order).
 * @return SOCKET válido listo para accept(), o termina el proceso si hay error.
 */
socket_t tcp_listen_any(uint16_t port) {
    socket_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        exit(1);
    }

    // Reusar dirección inmediatamente al reiniciar el servidor.
    BOOL yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed: %d\n", WSAGetLastError());
        tcp_close(s); exit(1);
    }
    if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "listen() failed: %d\n", WSAGetLastError());
        tcp_close(s); exit(1);
    }
    return s;
}

/**
 * @brief Conecta a un host:port por TCP (resolviendo DNS si hace falta).
 *
 * @param host Hostname o IPv4 (ej. "127.0.0.1").
 * @param port Puerto de destino (host order).
 * @return SOCKET conectado, o termina el proceso si hay error.
 */
socket_t tcp_connect(const char *host, uint16_t port) {
    socket_t s = INVALID_SOCKET;
    char portstr[16];
    snprintf(portstr, sizeof(portstr), "%u", port);

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;         // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int r = getaddrinfo(host, portstr, &hints, &res);
    if (r != 0 || !res) {
        fprintf(stderr, "getaddrinfo(%s:%s) failed: %d\n", host, portstr, r);
        exit(1);
    }

    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        freeaddrinfo(res); exit(1);
    }

    if (connect(s, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        fprintf(stderr, "connect() failed: %d\n", WSAGetLastError());
        freeaddrinfo(res); tcp_close(s); exit(1);
    }
    freeaddrinfo(res);
    return s;
}

/**
 * @brief Pone un socket en modo no bloqueante.
 * @param s SOCKET válido.
 * @return 0 si ok, SOCKET_ERROR si falla.
 */
int set_nonblock(socket_t s) {
    u_long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode);
}

/**
 * @brief Lee de forma bloqueante hasta encontrar '\n', EOF o llenar el buffer.
 *
 * @param s       SOCKET desde el que leer.
 * @param buf     Buffer de salida (se asegura terminación en '\0').
 * @param maxlen  Tamaño de buf.
 * @return número de bytes leídos (>=0). 0 si peer cerró. -1 en error.
 *
 * Detalles:
 *  - Lee byte a byte con recv() para detectar '\n'.
 *  - En Windows, si recv() devuelve SOCKET_ERROR y WSAGetLastError()==WSAEINTR,
 *    reintenta. Otros errores devuelven -1.
 */
int readline(socket_t s, char *buf, int maxlen) {
    int n = 0;
    while (n < maxlen - 1) {
        char c;
        int r = recv(s, &c, 1, 0);
        if (r == 1) {
            buf[n++] = c;
            if (c == '\n') break;
        } else if (r == 0) {
            // El peer cerró la conexión.
            break;
        } else {
            int e = WSAGetLastError();
            if (e == WSAEINTR) continue;
            return -1;
        }
    }
    buf[n] = '\0';
    return n;
}

/**
 * @brief Envía todo el buffer (bloqueante) hasta completar 'len' o error.
 *
 * @param s   SOCKET destino.
 * @param buf Datos a enviar.
 * @param len Tamaño de buf.
 * @return total de bytes enviados (==len si éxito) o -1 si error.
 *
 * Detalles:
 *  - Reintenta si send() es interrumpido (WSAEINTR).
 *  - En error distinto, devuelve -1 (no hace rollback del parcial ya enviado).
 */
int writen(socket_t s, const char *buf, int len) {
    int total = 0;
    while (total < len) {
        int w = send(s, buf + total, len - total, 0);
        if (w == SOCKET_ERROR) {
            int e = WSAGetLastError();
            if (e == WSAEINTR) continue;
            return -1;
        }
        total += w;
    }
    return total;
}
