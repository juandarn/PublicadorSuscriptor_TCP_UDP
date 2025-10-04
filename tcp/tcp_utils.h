/**
 * @file tcp_utils.h
 * @brief Definiciones y funciones auxiliares para sockets TCP con Winsock2.
 *
 * Este módulo encapsula funciones de red básicas para programas cliente/servidor
 * en C usando la API de **Winsock2 (Windows)**.
 *
 * Incluye:
 *  - Inicialización y limpieza de Winsock (WSAStartup / WSACleanup)
 *  - Creación de sockets de servidor (bind + listen)
 *  - Conexión TCP a un host remoto
 *  - Lectura por línea y escritura completa
 *  - Utilidades de cierre y modo no bloqueante
 *
 * **Compatibilidad:** solo Windows.
 *
 * Para compilar con GCC:
 * @code
 * gcc archivo.c tcp_utils.c -o salida.exe -lws2_32
 * @endcode
 */

#ifndef TCP_UTILS_H
#define TCP_UTILS_H

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

/** Puerto por defecto del broker TCP */
#define BROKER_PORT 8080
/** Tamaño máximo de línea de texto en buffers */
#define MAX_LINE    1024
/** Tamaño máximo permitido para nombres de tópicos */
#define MAX_TOPIC   64

/**
 * @brief Inicializa la pila de sockets de Windows (WSAStartup).
 * @return 0 si correcto, -1 si hay error.
 */
int  winsock_init(void);

/**
 * @brief Libera los recursos de Winsock (WSACleanup).
 */
void winsock_cleanup(void);

/**
 * @brief Crea un socket TCP en escucha ligado a cualquier interfaz local.
 *
 * @param port Puerto a escuchar (en orden de host).
 * @return Socket válido listo para accept().
 */
socket_t tcp_listen_any(uint16_t port);

/**
 * @brief Establece una conexión TCP con un host remoto.
 *
 * @param host Dirección o nombre del host (por ejemplo "127.0.0.1").
 * @param port Puerto remoto (en orden de host).
 * @return Socket conectado, o termina el programa si hay error.
 */
socket_t tcp_connect(const char *host, uint16_t port);

/**
 * @brief Configura un socket en modo no bloqueante.
 * @param s Socket a modificar.
 * @return 0 si correcto, SOCKET_ERROR si falla.
 */
int set_nonblock(socket_t s);

/**
 * @brief Cierra un socket (wrapper de closesocket()).
 * @param s Socket a cerrar.
 * @return 0 si ok, SOCKET_ERROR si falla.
 */
int tcp_close(socket_t s);

/**
 * @brief Lee una línea completa (bloqueante) desde un socket TCP.
 *
 * @param s Socket desde el cual leer.
 * @param buf Buffer de salida (terminado en '\0').
 * @param maxlen Tamaño máximo del buffer.
 * @return Número de bytes leídos, 0 si conexión cerrada, -1 si error.
 */
int readline(socket_t s, char *buf, int maxlen);

/**
 * @brief Envía de forma bloqueante todo el contenido del buffer.
 *
 * @param s Socket destino.
 * @param buf Datos a enviar.
 * @param len Tamaño de buf.
 * @return Número total de bytes enviados o -1 si error.
 */
int writen(socket_t s, const char *buf, int len);

#endif /* TCP_UTILS_H */