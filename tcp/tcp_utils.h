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

#define BROKER_PORT 8080
#define MAX_LINE    1024
#define MAX_TOPIC   64

int  winsock_init(void);            // WSAStartup
void winsock_cleanup(void);         // WSACleanup

socket_t tcp_listen_any(uint16_t port);          // socket(), bind(), listen()
socket_t tcp_connect(const char *host, uint16_t port);
int      set_nonblock(socket_t s);               // opcional (no usado aquí)
int      tcp_close(socket_t s);

int  readline(socket_t s, char *buf, int maxlen); // lee hasta '\n'
int  writen(socket_t s, const char *buf, int len); // escribe todo el buffer

#endif
