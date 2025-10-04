/**
 * @file udp_utils.c
 * @brief Utilidades para sockets UDP en Windows (Winsock2).
 *
 * Proporciona funciones de apoyo para:
 *  - Inicializar / limpiar Winsock (WSAStartup / WSACleanup).
 *  - Crear socket UDP ligado a cualquier interfaz local (broker).
 *  - Crear socket UDP sin bind (puerto efímero) para clientes.
 *  - Enviar cadenas (sendto) y recibir datagramas como líneas (recvfrom).
 *  - Resolver IPv4 por IP literal o DNS.
 *
 * Compilación (GCC / MinGW-w64):
 *   gcc <archivos>.c udp_utils.c -o salida.exe -lws2_32
 *
 * Notas:
 *  - Este módulo es específico de Windows (Winsock2).
 *  - Los helpers devuelven errores básicos; en caso crítico finalizan el proceso.
 */

#define _CRT_SECURE_NO_WARNINGS
#include "udp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Inicializa la librería Winsock.
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
 * @brief Libera los recursos de Winsock.
 */
void winsock_cleanup(void) {
    WSACleanup();
}

/**
 * @brief Crea y liga un socket UDP a INADDR_ANY:port (modo servidor/broker).
 *
 * @param port Puerto local (host order) al que se ligará el socket.
 * @return SOCKET válido listo para recvfrom(); aborta el proceso si hay error.
 */
socket_t udp_bind_any(uint16_t port) {
    socket_t s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        exit(1);
    }

    // Permitir reusar la dirección al reiniciar rápidamente el broker.
    BOOL yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed: %d\n", WSAGetLastError());
        CLOSESOCK(s); exit(1);
    }
    return s;
}

/**
 * @brief Crea un socket UDP sin bind explícito (cliente con puerto efímero).
 *
 * @return SOCKET válido listo para sendto()/recvfrom(); aborta el proceso si hay error.
 */
socket_t udp_socket_unbound(void) {
    socket_t s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        exit(1);
    }
    return s;
}

/**
 * @brief Cierra un socket UDP (wrapper de closesocket()).
 * @param s Socket a cerrar.
 * @return 0 si ok, SOCKET_ERROR si falla.
 */
int udp_close(socket_t s) {
    return CLOSESOCK(s);
}

/**
 * @brief Envía una cadena ASCII a un destino UDP (sendto).
 *
 * @param s   Socket UDP.
 * @param str Cadena a enviar (se usa strlen(str)).
 * @param dst Dirección de destino (sockaddr_in) con IP:puerto ya configurados.
 * @return Bytes enviados o -1 si error.
 */
int udp_sendto_str(socket_t s, const char *str, const struct sockaddr_in *dst) {
    int len  = (int)strlen(str);
    int sent = sendto(s, str, len, 0, (const struct sockaddr*)dst, sizeof(*dst));
    if (sent == SOCKET_ERROR) return -1;
    return sent;
}

/**
 * @brief Recibe un datagrama UDP y lo normaliza a "línea" terminada en '\0'.
 *
 * - Recorta a maxlen-1 y agrega terminador '\0'.
 * - Si encuentra '\r' o '\n' dentro del datagrama, trunca allí (modo “línea”).
 *
 * @param s     Socket UDP.
 * @param buf   Buffer de salida.
 * @param maxlen Tamaño de buf.
 * @param src   Salida con dirección del emisor (IP:puerto).
 * @return Bytes recibidos (>=0), -1 si error.
 */
int udp_recvfrom_line(socket_t s, char *buf, int maxlen, struct sockaddr_in *src) {
    int srclen = sizeof(*src);
    int n = recvfrom(s, buf, maxlen - 1, 0, (struct sockaddr*)src, &srclen);
    if (n == SOCKET_ERROR) return -1;
    if (n < 0) n = 0;
    buf[n] = '\0';

    // Normaliza fin de línea por si el emisor mandó \n o \r\n
    char *p = strpbrk(buf, "\r\n");
    if (p) *p = '\0';
    return n;
}

/**
 * @brief Resuelve una dirección IPv4 para host:port (IP literal o DNS).
 *
 * @param host Cadena con IP (ej. "127.0.0.1") o nombre DNS.
 * @param port Puerto (host order).
 * @param out  Estructura de salida con familia AF_INET, IP y puerto listos.
 * @return 0 si ok, -1 si falla.
 */
int resolve_ipv4(const char *host, uint16_t port, struct sockaddr_in *out) {
    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;
    out->sin_port   = htons(port);

    // Primero intenta como IP literal (ej. "127.0.0.1")
    if (inet_pton(AF_INET, host, &out->sin_addr) == 1) return 0;

    // Si no, intenta resolver como DNS
    struct addrinfo hints, *res = NULL;
    char portstr[16]; snprintf(portstr, sizeof(portstr), "%u", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int r = getaddrinfo(host, portstr, &hints, &res);
    if (r != 0 || !res) return -1;

    struct sockaddr_in *a = (struct sockaddr_in*)res->ai_addr;
    memcpy(out, a, sizeof(*out));
    freeaddrinfo(res);
    return 0;
}
