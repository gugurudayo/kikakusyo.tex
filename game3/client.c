// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define WORLD_WIDTH  1024
#define WORLD_HEIGHT 1024

#define MAX_BULLETS 32

typedef struct {
    int player_id; // 0 or 1
    int dx;        // -1, 0, 1
    int dy;        // -1, 0, 1
    int shoot;     // 0 or 1
} MovePacket;

typedef struct {
    float x[2];
    float y[2];

    float bullet_x[2][MAX_BULLETS];
    float bullet_y[2][MAX_BULLETS];
    int   bullet_active[2][MAX_BULLETS];

    int hp[2];
} StatePacket;

// ソケットをノンブロッキングにする
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("使い方: %s <server_ip> <player_id(0 or 1)>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int player_id = atoi(argv[2]);
    if (player_id != 0 && player_id != 1) {
        printf("player_id は 0 または 1 を指定してください\n");
        return 1;
    }

    // サーバーへ接続
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(50000);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    // 最初の状態を受信（ブロッキングで1回だけ）
    StatePacket state;
    int received = 0;
    while (received < (int)sizeof(state)) {
        int n = recv(sock, ((char *)&state) + received, sizeof(state) - received, 0);
        if (n <= 0) {
            perror("initial recv");
            close(sock);
            return 1;
        }
        received += n;
    }

    // 非ブロッキングにする
    if (set_nonblocking(sock) < 0) {
        perror("set_nonblocking");
        close(sock);
        return 1;
    }

    // SDL初期化
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        close(sock);
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("IMG_Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        close(sock);
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "2P Network Shooting (sample)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        0
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        close(sock);
        return 1;
    }

    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        close(sock);
        return 1;
    }

    // 背景画像読み込み（同じフォルダに iroiro1.png を置いてね）
    SDL_Texture *bg = IMG_LoadTexture(renderer, "Chatgpt.png");
    if (!bg) {
        printf("Failed to load background: %s\n", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        close(sock);
        return 1;
    }

    int quit = 0;
    int shootCooldown = 0;  // クライアント側の簡易クールタイム（サーバ側と両方かけてもOK）

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        // キーボード状態取得
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        int dx = 0, dy = 0;

        if (player_id == 0) {
            // プレイヤー0: WASD
            if (keys[SDL_SCANCODE_A]) dx = -1;
            if (keys[SDL_SCANCODE_D]) dx =  1;
            if (keys[SDL_SCANCODE_W]) dy = -1;
            if (keys[SDL_SCANCODE_S]) dy =  1;
        } else {
            // プレイヤー1: 矢印キー
            if (keys[SDL_SCANCODE_LEFT])  dx = -1;
            if (keys[SDL_SCANCODE_RIGHT]) dx =  1;
            if (keys[SDL_SCANCODE_UP])    dy = -1;
            if (keys[SDL_SCANCODE_DOWN])  dy =  1;
        }

        // クールタイム処理
        if (shootCooldown > 0) shootCooldown--;

        int wantShoot = 0;
        if (player_id == 0) {
            if (keys[SDL_SCANCODE_SPACE] && shootCooldown == 0) {
                wantShoot = 1;
                shootCooldown = 10;  // クライアント側の発射間隔（調整可）
            }
        } else {
            if (keys[SDL_SCANCODE_RCTRL] && shootCooldown == 0) {
                wantShoot = 1;
                shootCooldown = 10;
            }
        }

        // 移動＋発射パケット送信
        MovePacket move;
        memset(&move, 0, sizeof(move));
        move.player_id = player_id;
        move.dx = dx;
        move.dy = dy;
        move.shoot = wantShoot;

        int s = send(sock, &move, sizeof(move), 0);
        if (s < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("send");
            quit = 1;
        }

        // サーバーからの最新状態をできるだけ受信
        while (1) {
            StatePacket tmp;
            int n = recv(sock, &tmp, sizeof(tmp), 0);
            if (n < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    break;
                } else {
                    perror("recv");
                    quit = 1;
                    break;
                }
            } else if (n == 0) {
                printf("Server closed connection.\n");
                quit = 1;
                break;
            } else if (n == sizeof(tmp)) {
                state = tmp;
            } else {
                // 中途半端なサイズは無視
                break;
            }
        }

        // HP をタイトルに表示
        char title[64];
        snprintf(title, sizeof(title), "P0 HP: %d   P1 HP: %d", state.hp[0], state.hp[1]);
        SDL_SetWindowTitle(window, title);

        // カメラ位置計算（自分中心）
        float camera_x = state.x[player_id] - WINDOW_WIDTH  / 2.0f;
        float camera_y = state.y[player_id] - WINDOW_HEIGHT / 2.0f;

        if (camera_x < 0) camera_x = 0;
        if (camera_y < 0) camera_y = 0;
        if (camera_x > WORLD_WIDTH  - WINDOW_WIDTH)  camera_x = WORLD_WIDTH  - WINDOW_WIDTH;
        if (camera_y > WORLD_HEIGHT - WINDOW_HEIGHT) camera_y = WORLD_HEIGHT - WINDOW_HEIGHT;

        // 描画
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // 背景
        SDL_Rect src = {
            (int)camera_x,
            (int)camera_y,
            WINDOW_WIDTH,
            WINDOW_HEIGHT
        };
        SDL_Rect dst = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderCopy(renderer, bg, &src, &dst);

        // 弾の描画
        for (int owner = 0; owner < 2; owner++) {
            if (owner == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // 赤の弾: 黄色
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // 青の弾: シアン
            }

            for (int k = 0; k < MAX_BULLETS; k++) {
                if (!state.bullet_active[owner][k]) continue;

                float bx = state.bullet_x[owner][k] - camera_x;
                float by = state.bullet_y[owner][k] - camera_y;

                if (bx < -10 || bx > WINDOW_WIDTH + 10 ||
                    by < -10 || by > WINDOW_HEIGHT + 10) {
                    continue;
                }

                SDL_Rect brect = {
                    (int)bx - 6,
                    (int)by - 16,
                    8, 16
                };
                SDL_RenderFillRect(renderer, &brect);
            }
        }

        // プレイヤー
        SDL_Rect player0 = {
            (int)(state.x[0] - camera_x),
            (int)(state.y[0] - camera_y),
            40, 40
        };
        SDL_Rect player1 = {
            (int)(state.x[1] - camera_x),
            (int)(state.y[1] - camera_y),
            40, 40
        };

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &player0);

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &player1);

        SDL_RenderPresent(renderer);

        SDL_Delay(16); // だいたい60fps
    }

    SDL_DestroyTexture(bg);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    close(sock);
    return 0;
}
