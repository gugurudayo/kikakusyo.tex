// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERVER_PORT 50000
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PLAYER_SPEED 5.0f
#define MAX_PLAYER 4

typedef struct {
    int player_id; // 0 or 1
    int dx;        // -1, 0, 1
    int dy;        // -1, 0, 1
} MovePacket;

typedef struct {
    float x[MAX_PLAYER];
    float y[MAX_PLAYER];
} StatePacket;

int main(void) {
    int listen_fd, client_fd[MAX_PLAYER] = {-1, -1, -1, -1};
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // ソケット作成
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 2) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("Server: waiting for 2 clients on port %d...\n", SERVER_PORT);

    // クライアント4人接続を待つ
    for (int i = 0; i < MAX_PLAYER; i++) {
        client_fd[i] = accept(listen_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd[i] < 0) {
            perror("accept");
            close(listen_fd);
            return 1;
        }
        printf("Client %d connected.\n", i);
    }

    // プレイヤー初期位置
    StatePacket state;
    state.x[0] = 100.0f;
    state.y[0] = 300.0f;

    state.x[1] = 660.0f;
    state.y[1] = 300.0f;

    state.x[2] = 100.0f;
    state.y[2] = 100.0f;

    state.x[3] = 660.0f;
    state.y[3] = 100.0f;

    // 最初の状態を全員に送信しておく
    for (int i = 0; i < MAX_PLAYER; i++) {
        if (send(client_fd[i], &state, sizeof(state), 0) != sizeof(state)) {
            perror("send initial state");
        }
    }

    // メインループ
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = -1;
        for (int i = 0; i < MAX_PLAYER; i++) {
            FD_SET(client_fd[i], &readfds);
            if (client_fd[i] > maxfd) maxfd = client_fd[i];
        }

        // クライアントのどちらかからのデータを待つ
        int ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select");
            break;
        }

        for (int i = 0; i < MAX_PLAYER; i++) {
            if (FD_ISSET(client_fd[i], &readfds)) {
                MovePacket move;
                int n = recv(client_fd[i], &move, sizeof(move), 0);
                if (n <= 0) {
                    printf("Client %d disconnected.\n", i);
                    close(client_fd[i]);
                    client_fd[i] = -1;
                    // 本当は再接続対応などを考える
                    goto cleanup;
                }
                if (n != sizeof(move)) {
                    printf("Received unexpected size from client %d\n", i);
                    continue;
                }

                // 移動処理
                int id = move.player_id;
                if (id < 0 || id >= MAX_PLAYER) continue;

                state.x[id] += move.dx * PLAYER_SPEED;
                state.y[id] += move.dy * PLAYER_SPEED;

                // 画面外に出ないようにクリップ
                if (state.x[id] < 0) state.x[id] = 0;
                if (state.x[id] > WINDOW_WIDTH - 40) state.x[id] = WINDOW_WIDTH - 40;
                if (state.y[id] < 0) state.y[id] = 0;
                if (state.y[id] > WINDOW_HEIGHT - 40) state.y[id] = WINDOW_HEIGHT - 40;

                // 新しい状態を全クライアントに送信
                for (int j = 0; j < MAX_PLAYER; j++) {
                    if (client_fd[j] >= 0) {
                        int s = send(client_fd[j], &state, sizeof(state), 0);
                        if (s != sizeof(state)) {
                            perror("send state");
                        }
                    }
                }
            }
        }
    }

cleanup:
    for (int i = 0; i < MAX_PLAYER; i++) {
        if (client_fd[i] >= 0) close(client_fd[i]);
    }
    close(listen_fd);
    return 0;
}
