#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <math.h> 

#define MAX_PLAYER_BULLETS 100
#define MAX_BULLETS 100

typedef struct {
    SDL_Rect rect;
    int active;
} PlayerBullet;

typedef struct {
    SDL_Rect rect;
    int active;
    float vx, vy;
} Bullet;

int main(void){

    int gameOver = 0;
    int gameClear = 0;
    Uint32 endTimer = 0;
    Uint32 gameStartTime = 0;
    int hitCount = 0;
    int difficulty = 0; // 0: Normal, 1: Hard
    int inTitle = 1;    // 1: タイトル画面, 0: ゲーム中
    int selectedOption = 0; // 0: Normal, 1: Hard
    

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("IMG_Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("NotoSans-Regular.ttf", 24);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return 1;
    }


    PlayerBullet playerBullets[MAX_PLAYER_BULLETS];

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        playerBullets[i].active = 0;
    }


    


    int shootTimer = 0;
    int playerShootCooldown = 0;

    SDL_Window* window;
    window = SDL_CreateWindow("shooting", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,640, 480, 0);
    if(window == NULL){
        printf("Failed");
        exit(-1);
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface* surface = IMG_Load("ship_L.png");
    if (!surface) {
        printf("IMG_Load Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Texture* playerTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    

    SDL_Rect player = { 640 / 2 - 32, 480 - 70, 64, 64 };
    int speed = 5;

    

    int running = 1;
    SDL_Event event;

    while (running) {

        

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            } else if (inTitle && event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) {
                    selectedOption = 1 - selectedOption;
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    difficulty = selectedOption;
                    inTitle = 0;
                    gameStartTime = SDL_GetTicks();
                }
            }
        }


        const Uint8* keys = SDL_GetKeyboardState(NULL);

        // ゲーム中のみゲームロジックを更新
        if (!gameOver && !gameClear && !inTitle) {
            if (keys[SDL_SCANCODE_LEFT])  player.x -= speed;
            if (keys[SDL_SCANCODE_RIGHT]) player.x += speed;
            if (keys[SDL_SCANCODE_UP])    player.y -= speed;
            if (keys[SDL_SCANCODE_DOWN])  player.y += speed;

            // 範囲制限
            if (player.x < 0) player.x = 0;
            if (player.x + player.w > 640) player.x = 640 - player.w;
            if (player.y < 0) player.y = 0;
            if (player.y + player.h > 480) player.y = 480 - player.h;

           

            // プレイヤーの弾の発射
            if (playerShootCooldown > 0) {
                playerShootCooldown--;
            }
            if (keys[SDL_SCANCODE_SPACE] && playerShootCooldown == 0) {
                for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                    if (!playerBullets[i].active) {
                        playerBullets[i].rect.x = player.x + player.w / 2 - 3;
                        playerBullets[i].rect.y = player.y - 10;
                        playerBullets[i].rect.w = 6;
                        playerBullets[i].rect.h = 15;
                        playerBullets[i].active = 1;
                        playerShootCooldown = 10;
                        break;
                    }
                }
            }

            // プレイヤーの弾の更新
            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                if (playerBullets[i].active) {
                    playerBullets[i].rect.y -= 10;
                    if (playerBullets[i].rect.y + playerBullets[i].rect.h < 0) {
                        playerBullets[i].active = 0;
                    }
                }
            }

            

            // 敵とプレイヤーの当たり判定
            SDL_Rect playerHitbox = { player.x + 16, player.y + 16, 32, 32 };
            

            // 敵の弾とプレイヤーの当たり判定
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (enemyBullets[i].active) {
                    if (SDL_HasIntersection(&enemyBullets[i].rect, &playerHitbox)) {
                        printf("YOU LOSE\n");
                        enemyBullets[i].active = 0;
                        gameOver = 1;
                        endTimer = SDL_GetTicks();
                        
                    }
                 }
            }

            // プレイヤーの弾と敵の当たり判定
            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                if (playerBullets[i].active) {
                    if (SDL_HasIntersection(&playerBullets[i].rect, &enemyHitbox)) {
                        printf("hit!\n");
                        playerBullets[i].active = 0;
                        hitCount++;
                        if (hitCount >= 50) {
                            gameClear = 1;
                            endTimer = SDL_GetTicks();
                            
                        }
                    }
                }
            }

            // 敵との接触判定
            if (SDL_HasIntersection(&playerHitbox, &enemyHitbox)) {
                printf("ENEMY COLLISION - YOU LOSE\n");
                gameOver = 1;
                endTimer = SDL_GetTicks();
                
            }
        }


        // 描画処理
        if (inTitle) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // タイトル枠
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect titleRect = { 180, 100, 280, 40 };
            SDL_RenderDrawRect(renderer, &titleRect);

            // Normalボタン
            SDL_Rect normalRect = { 220, 200, 200, 40 };
            if (selectedOption == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 102, 102, 255);   // 赤
            } else {
                SDL_SetRenderDrawColor(renderer, 68, 68, 255, 255); // 青
            }
            SDL_RenderFillRect(renderer, &normalRect);

            // Hardボタン
            SDL_Rect hardRect = { 220, 260, 200, 40 };
            if (selectedOption == 1) {
                SDL_SetRenderDrawColor(renderer, 255, 102, 102, 255);   // 赤
            } else {
                SDL_SetRenderDrawColor(renderer, 68, 68, 255, 255); // 青
            }
            SDL_RenderFillRect(renderer, &hardRect);

            // タイトルテキスト
            SDL_Color textColor = { 255, 255, 255, 255};
            SDL_Surface *titleSurface = TTF_RenderText_Solid(font, "Shooting", textColor);
            SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleTextRect = { titleRect.x + 90, titleRect.y + 8, titleSurface->w, titleSurface->h };
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleTextRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);

            // Normalテキスト描画
            SDL_Surface *normalSurface = TTF_RenderText_Solid(font, "Normal", textColor);
            SDL_Texture *normalTexture = SDL_CreateTextureFromSurface(renderer, normalSurface);
            SDL_Rect normalTextRect = { normalRect.x + 60, normalRect.y + 8, normalSurface->w, normalSurface->h };
            SDL_RenderCopy(renderer, normalTexture, NULL, &normalTextRect);
            SDL_FreeSurface(normalSurface);
            SDL_DestroyTexture(normalTexture);

            // Hardテキスト描画
            SDL_Surface *hardSurface = TTF_RenderText_Solid(font, "Hard", textColor);
            SDL_Texture *hardTexture = SDL_CreateTextureFromSurface(renderer, hardSurface);
            SDL_Rect hardTextRect = { hardRect.x + 70, hardRect.y + 8, hardSurface->w, hardSurface->h };
            SDL_RenderCopy(renderer, hardTexture, NULL, &hardTextRect);
            SDL_FreeSurface(hardSurface);
            SDL_DestroyTexture(hardTexture);

            SDL_RenderPresent(renderer);
        } else if (gameOver || gameClear) {
            // ゲームオーバー/クリア画面の描画（2秒間表示）
            const char *msg = gameOver ? "Game Over" : "Game Clear!";
            SDL_Color color = { 255, 255, 255, 255 };

            SDL_Surface *msgSurface = TTF_RenderText_Solid(font, msg, color);
            SDL_Texture *msgTexture = SDL_CreateTextureFromSurface(renderer, msgSurface);

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            SDL_Rect msgRect;
            msgRect.w = msgSurface->w;
            msgRect.h = msgSurface->h;
            msgRect.x = 640 / 2 - msgRect.w / 2;
            msgRect.y = 480 / 2 - msgRect.h / 2;

            SDL_RenderCopy(renderer, msgTexture, NULL, &msgRect);

            SDL_FreeSurface(msgSurface);
            SDL_DestroyTexture(msgTexture);

            SDL_RenderPresent(renderer);

            // 2秒経過したらタイトルに戻る
            if (SDL_GetTicks() - endTimer >= 2000) {
                gameOver = 0;
                gameClear = 0;
                inTitle = 1;
                hitCount = 0;

                // プレイヤー初期位置
                player.x = 640 / 2 - 32;
                player.y = 480 - 70;

                // 弾のリセット
                for (int i = 0; i < MAX_PLAYER_BULLETS; i++) playerBullets[i].active = 0;
                
            }
        } else {
            // 通常のゲーム画面の描画
            SDL_SetRenderDrawColor(renderer, 40, 0, 60, 255);
            SDL_RenderClear(renderer);

            SDL_RenderCopy(renderer, playerTexture, NULL, &player);
            +6----
            

            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                if (playerBullets[i].active) {
                    SDL_RenderFillRect(renderer, &playerBullets[i].rect);
                }
            }

            SDL_RenderPresentTTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyTexture(playerTexture);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}(renderer);
        }

        SDL_Delay(16); 
    }

    TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyTexture(playerTexture);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
