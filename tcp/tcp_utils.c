#define _CRT_SECURE_NO_WARNINGS
#include "tcp_utils.h"
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

int tcp_close(socket_t s) {
    return CLOSESOCK(s);
}

socket_t tcp_listen_any(uint16_t port) {
    socket_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        exit(1);
    }
    BOOL yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
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

int set_nonblock(socket_t s) {
    u_long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode);
}

// bloqueante, simple
int readline(socket_t s, char *buf, int maxlen) {
    int n = 0;
    while (n < maxlen - 1) {
        char c;
        int r = recv(s, &c, 1, 0);
        if (r == 1) {
            buf[n++] = c;
            if (c == '\n') break;
        } else if (r == 0) {
            break; // peer cerró
        } else {
            int e = WSAGetLastError();
            if (e == WSAEINTR) continue;
            return -1;
        }
    }
    buf[n] = '\0';
    return n;
}

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
