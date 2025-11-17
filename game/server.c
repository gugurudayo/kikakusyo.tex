// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERVER_PORT 50000

#define WORLD_WIDTH  1920
#define WORLD_HEIGHT 1080

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PLAYER_SPEED 5.0f

#define MAX_BULLETS 32
#define BULLET_SPEED 10.0f

typedef struct {
    int player_id; // 0 or 1
    int dx;        // -1, 0, 1
    int dy;        // -1, 0, 1
    int shoot;     // 0: 発射なし, 1: 発射
} MovePacket;

typedef struct {
    float x[2];
    float y[2];

    // 弾
    float bullet_x[2][MAX_BULLETS];
    float bullet_y[2][MAX_BULLETS];
    int   bullet_active[2][MAX_BULLETS];
} StatePacket;

int main(void) {
    int listen_fd, client_fd[2] = {-1, -1};
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // 弾の向き (1: 下向き, -1: 上向き)
    int bullet_dir[2][MAX_BULLETS] = {0};  // ★ ここで一度だけ宣言＆0初期化

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

    // クライアント2人接続を待つ
    for (int i = 0; i < 2; i++) {
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
    memset(&state, 0, sizeof(state));

    state.x[0] = 300.0f;
    state.y[0] = 300.0f;
    state.x[1] = 1500.0f;
    state.y[1] = 800.0f;

    // 最初の状態を全員に送信しておく
    for (int i = 0; i < 2; i++) {
        if (client_fd[i] >= 0) {
            if (send(client_fd[i], &state, sizeof(state), 0) != sizeof(state)) {
                perror("send initial state");
            }
        }
    }

    // メインループ
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = -1;
        for (int i = 0; i < 2; i++) {
            if (client_fd[i] >= 0) {
                FD_SET(client_fd[i], &readfds);
                if (client_fd[i] > maxfd) maxfd = client_fd[i];
            }
        }

        // もし全クライアント切れてたら終了
        if (maxfd == -1) {
            printf("All clients disconnected.\n");
            break;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 16000;  // 約60fps

        int ret = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select");
            break;
        }

        // ---- クライアントからの入力処理 ----
        if (ret > 0) {
            for (int i = 0; i < 2; i++) {
                if (client_fd[i] < 0) continue;
                if (!FD_ISSET(client_fd[i], &readfds)) continue;

                MovePacket move;
                int n = recv(client_fd[i], &move, sizeof(move), 0);
                if (n <= 0) {
                    printf("Client %d disconnected.\n", i);
                    close(client_fd[i]);
                    client_fd[i] = -1;
                    continue;
                }
                if (n != sizeof(move)) {
                    printf("Received unexpected size from client %d\n", i);
                    continue;
                }

                // 移動処理
                int id = move.player_id;
                if (id < 0 || id > 1) continue;

                state.x[id] += move.dx * PLAYER_SPEED;
                state.y[id] += move.dy * PLAYER_SPEED;

                // ワールド外に出ないようにクリップ
                if (state.x[id] < 0) state.x[id] = 0;
                if (state.x[id] > WORLD_WIDTH - 40)  state.x[id] = WORLD_WIDTH  - 40;
                if (state.y[id] < 0) state.y[id] = 0;
                if (state.y[id] > WORLD_HEIGHT - 40) state.y[id] = WORLD_HEIGHT - 40;

                // 弾発射リクエストが来たら弾を生成
                if (move.shoot) {
                    for (int k = 0; k < MAX_BULLETS; k++) {
                        if (!state.bullet_active[id][k]) {
                            state.bullet_active[id][k] = 1;
                            state.bullet_x[id][k] = state.x[id] + 20.0f; // プレイヤ中心あたり

                            if (id == 0) {
                                // 赤: 下向きに発射
                                state.bullet_y[id][k] = state.y[id] + 40.0f;
                                bullet_dir[id][k] = 1;
                            } else {
                                // 青: 上向きに発射
                                state.bullet_y[id][k] = state.y[id];
                                bullet_dir[id][k] = -1;
                            }
                            break; // 1発だけ生成
                        }
                    }
                }
            }
        }

        // ---- 弾の移動 ----
        for (int id = 0; id < 2; id++) {
            for (int k = 0; k < MAX_BULLETS; k++) {
                if (!state.bullet_active[id][k]) continue;

                state.bullet_y[id][k] += BULLET_SPEED * bullet_dir[id][k];

                // ワールド外に出たら消す
                if (state.bullet_y[id][k] < 0 || state.bullet_y[id][k] > WORLD_HEIGHT) {
                    state.bullet_active[id][k] = 0;
                }
            }
        }

        // ---- 現在の状態を全クライアントに送信 ----
        for (int j = 0; j < 2; j++) {
            if (client_fd[j] >= 0) {
                int s = send(client_fd[j], &state, sizeof(state), 0);
                if (s != sizeof(state)) {
                    perror("send state");
                }
            }
        }
    }

cleanup:
    for (int i = 0; i < 2; i++) {
        if (client_fd[i] >= 0) close(client_fd[i]);
    }
    close(listen_fd);
    return 0;
}
