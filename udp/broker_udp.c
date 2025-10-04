#include "udp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SUBS  256

typedef struct {
    int   used;
    char  topic[MAX_TOPIC];
    struct sockaddr_in addr;
} sub_t;

static sub_t subs[MAX_SUBS];

static int same_addr(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

static void add_or_update_sub(const char *topic, const struct sockaddr_in *addr) {
    // ya existe?
    for (int i=0;i<MAX_SUBS;i++) {
        if (subs[i].used && same_addr(&subs[i].addr, addr) &&
            strncmp(subs[i].topic, topic, MAX_TOPIC) == 0) {
            return; // ya estaba
        }
    }
    // insertar nuevo
    for (int i=0;i<MAX_SUBS;i++) {
        if (!subs[i].used) {
            subs[i].used = 1;
            strncpy(subs[i].topic, topic, MAX_TOPIC-1);
            subs[i].topic[MAX_TOPIC-1] = '\0';
            subs[i].addr = *addr;
            return;
        }
    }
    fprintf(stderr, "[broker-udp] tabla de subs llena\n");
}

static void broadcast_topic(const char *topic, const char *payload, socket_t s) {
    char out[MAX_LINE];
    snprintf(out, sizeof(out), "MSG %s %s\n", topic, payload);
    for (int i=0;i<MAX_SUBS;i++) {
        if (subs[i].used && strncmp(subs[i].topic, topic, MAX_TOPIC) == 0) {
            (void)udp_sendto_str(s, out, &subs[i].addr);
        }
    }
}

int main(void) {
    if (winsock_init() != 0) return 1;
    memset(subs, 0, sizeof(subs));

    socket_t s = udp_bind_any(BROKER_UDP_PORT);
    printf("[broker-udp] escuchando UDP en puerto %d...\n", BROKER_UDP_PORT);

    char buf[MAX_LINE];
    struct sockaddr_in src;

    while (1) {
        int n = udp_recvfrom_line(s, buf, sizeof(buf), &src);
        if (n <= 0) continue;

        // Protocolo:
        //   SUB <topic>
        //   PUB <topic> <mensaje...>
        if (strncmp(buf, "SUB ", 4) == 0) {
            const char *topic = buf + 4;
            add_or_update_sub(topic, &src);
            char ok[MAX_LINE]; snprintf(ok, sizeof(ok), "OK SUB %s\n", topic);
            (void)udp_sendto_str(s, ok, &src);

        } else if (strncmp(buf, "PUB ", 4) == 0) {
            char *p = buf + 4;
            char *sp = strchr(p, ' ');
            if (!sp) continue;
            *sp = '\0';
            const char *topic = p;
            const char *payload = sp + 1;
            broadcast_topic(topic, payload, s);

        } else {
            const char *err = "ERR unknown command\n";
            (void)udp_sendto_str(s, err, &src);
        }
    }

    udp_close(s);
    winsock_cleanup();
    return 0;
}
