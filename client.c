// client.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <SDL2/SDL.h>
#include <joyconlib.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5000

typedef struct {
    float x;
    float y;
} PlayerPos;

/* ===== 共有データ ===== */
PlayerPos myPos = {100, 100};
PlayerPos enemyPos = {300, 300};

float joy_lx = 0.0f;
float joy_ly = 0.0f;

pthread_mutex_t joy_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t enemy_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ===== Joy-Con ===== */
joyconlib_t jc;

/* ===== Joy-Con入力スレッド ===== */
void* joycon_thread(void* arg)
{
    while (1) {
        joycon_get_state(&jc);

        pthread_mutex_lock(&joy_mutex);
        joy_lx = jc.stick.x;
        joy_ly = jc.stick.y;
        pthread_mutex_unlock(&joy_mutex);

        SDL_Delay(1);
    }
    return NULL;
}

/* ===== ネットワーク受信スレッド ===== */
void* net_thread(void* arg)
{
    int sock = *(int*)arg;
    PlayerPos buf;

    while (1) {
        recvfrom(sock, &buf, sizeof(buf), 0, NULL, NULL);

        pthread_mutex_lock(&enemy_mutex);
        enemyPos = buf;
        pthread_mutex_unlock(&enemy_mutex);
    }
    return NULL;
}

int main()
{
    /* ===== SDL ===== */
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window =
        SDL_CreateWindow("Joy-Con Network Game",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            800, 600, 0);

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    /* ===== Joy-Con接続 ===== */
    if (joycon_open(&jc, JOYCON_R) != JOYCON_ERR_NONE) {
        printf("Joy-Con open failed\n");
        exit(1);
    }
    printf("Joy-Con connected\n");

    /* ===== UDP ===== */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    /* ===== スレッド起動 ===== */
    pthread_t jc_th, net_th;
    pthread_create(&jc_th, NULL, joycon_thread, NULL);
    pthread_create(&net_th, NULL, net_thread, &sock);

    int running = 1;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
        }

        /* Joy-Con入力反映 */
        float lx, ly;
        pthread_mutex_lock(&joy_mutex);
        lx = joy_lx;
        ly = joy_ly;
        pthread_mutex_unlock(&joy_mutex);

        myPos.x += lx * 6.0f;
        myPos.y += ly * 6.0f;

        /* 送信 */
        sendto(sock, &myPos, sizeof(myPos), 0,
               (struct sockaddr*)&addr, sizeof(addr));

        /* 描画用コピー */
        PlayerPos drawEnemy;
        pthread_mutex_lock(&enemy_mutex);
        drawEnemy = enemyPos;
        pthread_mutex_unlock(&enemy_mutex);

        /* 描画 */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect me = {myPos.x, myPos.y, 20, 20};
        SDL_Rect en = {drawEnemy.x, drawEnemy.y, 20, 20};

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &me);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &en);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    close(sock);
    SDL_Quit();
}

