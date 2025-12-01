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

#define MAX_PLAYERS 4
#define MAX_BULLETS 32

typedef struct {
    int player_id; // 0..3
    int dx;        // -1, 0, 1
    int dy;        // -1, 0, 1
    int shoot;     // 0 or 1
} MovePacket;

typedef struct {
    float x[MAX_PLAYERS];
    float y[MAX_PLAYERS];

    float bullet_x[MAX_PLAYERS][MAX_BULLETS];
    float bullet_y[MAX_PLAYERS][MAX_BULLETS];
    int   bullet_active[MAX_PLAYERS][MAX_BULLETS];

    int   hp[MAX_PLAYERS];
} StatePacket;

// ソケットをノンブロッキングにする
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("使い方: %s <server_ip> <player_id(0..3)>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int player_id = atoi(argv[2]);
    if (player_id < 0 || player_id >= MAX_PLAYERS) {
        printf("player_id は 0〜3 を指定してください\n");
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

    // 最初の状態を受信
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

    if (set_nonblocking(sock) < 0) {
        perror("set_nonblocking");
        close(sock);
        return 1;
    }

    // SDL 初期化
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
        "4P Network Shooting",
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
    int shootCooldown = 0;

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = 1;
        }

        // キーボード状態
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        int dx = 0, dy = 0;

        // プレイヤーごとにキー割り当て
        if (player_id == 0) {
            // P0: WASD
            if (keys[SDL_SCANCODE_A]) dx = -1;
            if (keys[SDL_SCANCODE_D]) dx =  1;
            if (keys[SDL_SCANCODE_W]) dy = -1;
            if (keys[SDL_SCANCODE_S]) dy =  1;
        } else if (player_id == 1) {
            // P1: 矢印
            if (keys[SDL_SCANCODE_LEFT])  dx = -1;
            if (keys[SDL_SCANCODE_RIGHT]) dx =  1;
            if (keys[SDL_SCANCODE_UP])    dy = -1;
            if (keys[SDL_SCANCODE_DOWN])  dy =  1;
        } else if (player_id == 2) {
            // P2: IJKL
            if (keys[SDL_SCANCODE_J]) dx = -1;
            if (keys[SDL_SCANCODE_L]) dx =  1;
            if (keys[SDL_SCANCODE_I]) dy = -1;
            if (keys[SDL_SCANCODE_K]) dy =  1;
        } else {
            // P3: TFGH
            if (keys[SDL_SCANCODE_F]) dx = -1;
            if (keys[SDL_SCANCODE_H]) dx =  1;
            if (keys[SDL_SCANCODE_T]) dy = -1;
            if (keys[SDL_SCANCODE_G]) dy =  1;
        }

        // クールタイム
        if (shootCooldown > 0) shootCooldown--;

        int wantShoot = 0;
        if (player_id == 0) {
            if (keys[SDL_SCANCODE_SPACE] && shootCooldown == 0) {
                wantShoot = 1;
                shootCooldown = 10;
            }
        } else if (player_id == 1) {
            if (keys[SDL_SCANCODE_RCTRL] && shootCooldown == 0) {
                wantShoot = 1;
                shootCooldown = 10;
            }
        } else if (player_id == 2) {
            if (keys[SDL_SCANCODE_RSHIFT] && shootCooldown == 0) {
                wantShoot = 1;
                shootCooldown = 10;
            }
        } else {
            if (keys[SDL_SCANCODE_RETURN] && shootCooldown == 0) {
                wantShoot = 1;
                shootCooldown = 10;
            }
        }

        // パケット送信
        MovePacket move;
        memset(&move, 0, sizeof(move));
        move.player_id = player_id;
        move.dx        = dx;
        move.dy        = dy;
        move.shoot     = wantShoot;

        int s = send(sock, &move, sizeof(move), 0);
        if (s < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("send");
            quit = 1;
        }

        // 状態受信
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
                break;
            }
        }

        // HP をタイトルに表示
        char title[128];
        snprintf(title, sizeof(title),
                 "P0:%d  P1:%d  P2:%d  P3:%d",
                 state.hp[0], state.hp[1], state.hp[2], state.hp[3]);
        SDL_SetWindowTitle(window, title);

        // カメラ
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

        // 弾描画
        for (int owner = 0; owner < MAX_PLAYERS; owner++) {
            if (owner == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);   // P0弾: 黄
            } else if (owner == 1) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);   // P1弾: シアン
            } else if (owner == 2) {
                SDL_SetRenderDrawColor(renderer, 255, 128, 0, 255);   // P2弾: オレンジ
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);   // P3弾: マゼンタ
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
                    (int)bx - 2,
                    (int)by - 8,
                    4, 8
                };
                SDL_RenderFillRect(renderer, &brect);
            }
        }

        // プレイヤー描画
        for (int i = 0; i < MAX_PLAYERS; i++) {
            float px = state.x[i] - camera_x;
            float py = state.y[i] - camera_y;

            SDL_Rect prect = {
                (int)px,
                (int)py,
                40, 40
            };

            if (i == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);     // 赤
            } else if (i == 1) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);     // 青
            } else if (i == 2) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);     // 緑
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // 白
            }

            SDL_RenderFillRect(renderer, &prect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(bg);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    close(sock);
    return 0;
}
