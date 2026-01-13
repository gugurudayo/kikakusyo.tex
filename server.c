// server.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000

typedef struct {
    float x;
    float y;
} PlayerPos;

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr, clients[2];
    socklen_t len = sizeof(struct sockaddr_in);
    int client_count = 0;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    printf("UDP Server started : %d\n", PORT);

    while (1) {
        PlayerPos pos;
        struct sockaddr_in from;

        recvfrom(sock, &pos, sizeof(pos), 0,
                 (struct sockaddr*)&from, &len);

        int idx = -1;
        for (int i = 0; i < client_count; i++) {
            if (clients[i].sin_addr.s_addr == from.sin_addr.s_addr &&
                clients[i].sin_port == from.sin_port) {
                idx = i;
            }
        }

        if (idx == -1 && client_count < 2) {
            clients[client_count++] = from;
            printf("Client %d connected\n", client_count);
            continue;
        }

        if (client_count == 2) {
            int other = (from.sin_port == clients[0].sin_port) ? 1 : 0;
            sendto(sock, &pos, sizeof(pos), 0,
                   (struct sockaddr*)&clients[other], len);
        }
    }
}

