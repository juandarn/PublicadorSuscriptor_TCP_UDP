#define _CRT_SECURE_NO_WARNINGS
#include "udp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int winsock_init(void) {
    WSADATA wsa;
    int r = WSAStartup(MAKEWORD(2,2), &wsa);
    if (r != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", r);
        return -1;
    }
    return 0;
}

void winsock_cleanup(void) {
    WSACleanup();
}

socket_t udp_bind_any(uint16_t port) {
    socket_t s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        exit(1);
    }
    BOOL yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed: %d\n", WSAGetLastError());
        CLOSESOCK(s); exit(1);
    }
    return s;
}

socket_t udp_socket_unbound(void) {
    socket_t s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        exit(1);
    }
    return s;
}

int udp_close(socket_t s) {
    return CLOSESOCK(s);
}

int udp_sendto_str(socket_t s, const char *str, const struct sockaddr_in *dst) {
    int len = (int)strlen(str);
    int sent = sendto(s, str, len, 0, (const struct sockaddr*)dst, sizeof(*dst));
    if (sent == SOCKET_ERROR) return -1;
    return sent;
}

// Recibe 1 datagrama y lo "recorta" a una línea (añade '\0')
int udp_recvfrom_line(socket_t s, char *buf, int maxlen, struct sockaddr_in *src) {
    int srclen = sizeof(*src);
    int n = recvfrom(s, buf, maxlen - 1, 0, (struct sockaddr*)src, &srclen);
    if (n == SOCKET_ERROR) return -1;
    if (n < 0) n = 0;
    buf[n] = '\0';
    // normaliza fin de línea por si el emisor mandó \n
    char *p = strpbrk(buf, "\r\n");
    if (p) *p = '\0';
    return n;
}

int resolve_ipv4(const char *host, uint16_t port, struct sockaddr_in *out) {
    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;
    out->sin_port   = htons(port);

    // Primero intenta IP literal
    if (inet_pton(AF_INET, host, &out->sin_addr) == 1) return 0;

    // Luego DNS
    struct addrinfo hints, *res = NULL;
    char portstr[16]; snprintf(portstr, sizeof(portstr), "%u", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;

    int r = getaddrinfo(host, portstr, &hints, &res);
    if (r != 0 || !res) return -1;

    struct sockaddr_in *a = (struct sockaddr_in*)res->ai_addr;
    memcpy(out, a, sizeof(*out));
    freeaddrinfo(res);
    return 0;
}
