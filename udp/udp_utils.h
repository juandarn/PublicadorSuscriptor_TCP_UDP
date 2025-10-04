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

#define BROKER_UDP_PORT 8081
#define MAX_LINE        1024
#define MAX_TOPIC       64

int  winsock_init(void);
void winsock_cleanup(void);

socket_t udp_bind_any(uint16_t port);                     // broker
socket_t udp_socket_unbound(void);                        // publisher/subscriber
int      udp_close(socket_t s);

int  udp_sendto_str(socket_t s, const char *str,
                    const struct sockaddr_in *dst);
int  udp_recvfrom_line(socket_t s, char *buf, int maxlen,
                       struct sockaddr_in *src);

int  resolve_ipv4(const char *host, uint16_t port, struct sockaddr_in *out);

#endif
