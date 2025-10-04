/**
 * @file udp_utils.h
 * @brief Definiciones y prototipos de utilidades para sockets UDP (Winsock2 / Windows).
 *
 * Este módulo encapsula funciones básicas para trabajar con **UDP** en Windows:
 *  - Inicialización y limpieza de Winsock.
 *  - Creación de sockets UDP (ligado a puerto o efímero).
 *  - Envío de cadenas con `sendto()` y recepción “por línea” con `recvfrom()`.
 *  - Resolución IPv4 por IP literal o DNS.
 *
 * Compilación (GCC / MinGW-w64):
 * @code
 * gcc <archivos>.c udp_utils.c -o salida.exe -lws2_32
 * @endcode
 *
 * Compatibilidad: **solo Windows** (Winsock2).
 */

#ifndef UDP_UTILS_H
#define UDP_UTILS_H

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET socket_t;
  #define CLOSESOCK(s) closesocket(s)
#else
  #error "Este build es solo para Windows/Winsock2"
#endif

#include <stdint.h>

/** Puerto por defecto del broker UDP (evita colisión con TCP 8080). */
#define BROKER_UDP_PORT 8081
/** Tamaño máximo de línea para buffers de E/S de texto. */
#define MAX_LINE        1024
/** Longitud máxima permitida para nombres de tópicos. */
#define MAX_TOPIC       64

/**
 * @brief Inicializa la pila de sockets de Windows (WSAStartup).
 * @return 0 si correcto, -1 en error.
 */
int  winsock_init(void);

/**
 * @brief Libera los recursos de Winsock (WSACleanup).
 */
void winsock_cleanup(void);

/**
 * @brief Crea un socket UDP y lo liga a INADDR_ANY:port (modo broker/servidor).
 * @param port Puerto local en orden de host.
 * @return Socket válido listo para `recvfrom()`.
 */
socket_t udp_bind_any(uint16_t port);

/**
 * @brief Crea un socket UDP sin bind explícito (cliente con puerto efímero).
 * @return Socket válido listo para `sendto()` / `recvfrom()`.
 */
socket_t udp_socket_unbound(void);

/**
 * @brief Cierra un socket UDP (wrapper de `closesocket()`).
 * @param s Socket a cerrar.
 * @return 0 si ok, `SOCKET_ERROR` si falla.
 */
int udp_close(socket_t s);

/**
 * @brief Envía una cadena ASCII a un destino UDP con `sendto()`.
 * @param s   Socket UDP.
 * @param str Cadena a enviar (se usa `strlen(str)`).
 * @param dst Dirección destino (`sockaddr_in`) con IP:puerto configurados.
 * @return Bytes enviados o -1 en error.
 */
int  udp_sendto_str(socket_t s, const char *str,
                    const struct sockaddr_in *dst);

/**
 * @brief Recibe un datagrama UDP y lo normaliza a “línea” terminada en `'\0'`.
 *
 * Trunca en `\r` o `\n` si aparecen, y asegura terminación nula.
 *
 * @param s      Socket UDP.
 * @param buf    Buffer de salida.
 * @param maxlen Tamaño del buffer.
 * @param src    Salida con la dirección del emisor (IP:puerto).
 * @return Bytes recibidos (>=0), -1 en error.
 */
int  udp_recvfrom_line(socket_t s, char *buf, int maxlen,
                       struct sockaddr_in *src);

/**
 * @brief Resuelve una dirección IPv4 para `host:port` (IP literal o DNS).
 * @param host Cadena con IP (p.ej. "127.0.0.1") o nombre DNS.
 * @param port Puerto en orden de host.
 * @param out  Estructura de salida con `AF_INET`, IP y puerto listos.
 * @return 0 si ok, -1 si falla.
 */
int  resolve_ipv4(const char *host, uint16_t port, struct sockaddr_in *out);

#endif /* UDP_UTILS_H */
