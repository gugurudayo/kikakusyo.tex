// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include<joyconlib.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct {
    int player_id; // 0 or 1
    int dx;        // -1, 0, 1
    int dy;        // -1, 0, 1
} MovePacket;

typedef struct {
    float x[2];
    float y[2];
} StatePacket;

// ソケットをノンブロッキングにする
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
	    joyconlib_t jcL, jcR;

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
    addr.sin_port = htons(50000);
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

    // 非ブロッキングにする（recvで固まらないように）
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

    // Joy-Con を開く
	
    if (joycon_open(&jcL, JOYCON_L) < 0) {
    printf("左Joy-Conが開けません\n");
	}
    if (joycon_open(&jcR, JOYCON_R) < 0) {
    printf("右Joy-Conが開けません\n");
	}

    SDL_Window *window = SDL_CreateWindow(
        "2P Network Shooting (sample)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        0
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        close(sock);
        return 1;
    }

    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        close(sock);
        return 1;
    }

    int quit = 0;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }
	// ===== Joy-Con 入力取得 =====
	joycon_get_state(&jcL);
	joycon_get_state(&jcR);

	int j_dx = 0, j_dy = 0;

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

	// Player 0 → 左Joy-Conで操作
	if (player_id == 0) {
    	if (jcL.stick.x < -0.3) j_dx = -1;
    	if (jcL.stick.x >  0.3) j_dx =  1;
    	if (jcL.stick.y < -0.3) j_dy = -1;
	if (jcL.stick.y >  0.3) j_dy =  1;
	}

	// Player 1 → 右Joy-Conで操作
	if (player_id == 1) {
    	if (jcR.stick.x < -0.3) j_dx = -1;
    	if (jcR.stick.x >  0.3) j_dx =  1;
    	if (jcR.stick.y < -0.3) j_dy = -1;
    	if (jcR.stick.y >  0.3) j_dy =  1;
	}

	// Joy-Con の入力を dx/dy に統合
	if (dx == 0) dx = j_dx;
	if (dy == 0) dy = j_dy;


        // 移動パケット送信
        MovePacket move;
        move.player_id = player_id;
        move.dx = dx;
        move.dy = dy;

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
                    // これ以上受け取るものはない
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
                // 受け取れたら state を更新
                state = tmp;
            } else {
                // 中途半端なサイズは今回は無視（本当はバッファリングが必要）
                break;
            }
        }

        // 描画
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // 背景: 黒
        SDL_RenderClear(renderer);

        SDL_Rect player0 = { (int)state.x[0], (int)state.y[0], 40, 40 };
        SDL_Rect player1 = { (int)state.x[1], (int)state.y[1], 40, 40 };

        // プレイヤー0: 赤
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &player0);

        // プレイヤー1: 青
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &player1);

        SDL_RenderPresent(renderer);

        SDL_Delay(16); // 約60fps
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    joycon_close(&jcL);
    joycon_close(&jcR);
    SDL_Quit();
    close(sock);
    return 0;
}
