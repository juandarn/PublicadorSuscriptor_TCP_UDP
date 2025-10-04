#include "tcp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS  FD_SETSIZE

typedef struct {
    socket_t fd;
    char     topic[MAX_TOPIC]; // si es sub, guarda su tópico
    int      is_subscriber;    // 1=sub, 0=publisher/unknown
} client_t;

static client_t clients[MAX_CLIENTS];

static void trim_newline(char *s) {
    for (int i=0; s[i]; ++i)
        if (s[i]=='\r' || s[i]=='\n') { s[i]=0; break; }
}

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

static void handle_line(int idx, char *line) {
    trim_newline(line);
    // SUB <topic>
    // PUB <topic> <mensaje...>
    if (strncmp(line, "SUB ", 4) == 0) {
        char *topic = line + 4;
        if ((int)strlen(topic) >= MAX_TOPIC) topic[MAX_TOPIC-1] = '\0';
        strncpy(clients[idx].topic, topic, MAX_TOPIC);
        clients[idx].is_subscriber = 1;

        char ok[MAX_LINE];
        int n = snprintf(ok, sizeof(ok), "OK SUB %s\n", clients[idx].topic);
        (void)writen(clients[idx].fd, ok, n);

    } else if (strncmp(line, "PUB ", 4) == 0) {
        char *p = line + 4;
        char *space = strchr(p, ' ');
        if (!space) return; // formato inválido
        *space = '\0';
        const char *topic = p;
        const char *payload = space + 1;
        broadcast_to_topic(topic, payload);

    } else {
        const char *err = "ERR unknown command\n";
        (void)writen(clients[idx].fd, err, (int)strlen(err));
    }
}

int main(void) {
    if (winsock_init() != 0) return 1;

    socket_t listenfd = tcp_listen_any(BROKER_PORT);
    printf("[broker] escuchando en puerto %d...\n", BROKER_PORT);

    for (int i=0;i<MAX_CLIENTS;i++) clients[i].fd = INVALID_SOCKET;

    fd_set allset, rset;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    socket_t maxfd = listenfd;

    while (1) {
        rset = allset;
        int nready = select((int)maxfd+1, &rset, NULL, NULL, NULL);
        if (nready == SOCKET_ERROR) {
            fprintf(stderr, "select() err: %d\n", WSAGetLastError());
            break;
        }

        if (FD_ISSET(listenfd, &rset)) {
            struct sockaddr_in cliaddr; int len = sizeof(cliaddr);
            socket_t connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            if (connfd != INVALID_SOCKET) {
                int i;
                for (i=0;i<MAX_CLIENTS;i++) if (clients[i].fd == INVALID_SOCKET) { clients[i].fd = connfd; break; }
                if (i == MAX_CLIENTS) {
                    const char *msg="ERR too many clients\n";
                    writen(connfd, msg, (int)strlen(msg));
                    tcp_close(connfd);
                } else {
                    clients[i].is_subscriber = 0;
                    clients[i].topic[0] = '\0';
                    FD_SET(connfd, &allset);
                    if (connfd > maxfd) maxfd = connfd;
                    const char *hello="OK broker ready\n";
                    writen(connfd, hello, (int)strlen(hello));
                }
            }
            if (--nready <= 0) continue;
        }

        for (int i=0;i<MAX_CLIENTS;i++) {
            socket_t fd = clients[i].fd;
            if (fd == INVALID_SOCKET) continue;
            if (FD_ISSET(fd, &rset)) {
                char line[MAX_LINE];
                int n = readline(fd, line, sizeof(line));
                if (n <= 0) {
                    // desconexión
                    tcp_close(fd);
                    FD_CLR(fd, &allset);
                    clients[i].fd = INVALID_SOCKET;
                    continue;
                }
                handle_line(i, line);
            }
        }
    }

    tcp_close(listenfd);
    winsock_cleanup();
    return 0;
}
