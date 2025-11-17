/*****************************************************************
ファイル名   : client_win_utf8.c
機能      : クライアントのユーザーインターフェース処理（修正版）
*****************************************************************/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "common_utf8.h"
#include "client_func_utf8.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define BACKGROUND_IMAGE "22823124.jpg"
#define RESULT_IMAGE "Chatgpt.png"
#define FONT_PATH "/usr/share/fonts/opentype/ipafont-gothic/ipagp.ttf"

/* SCREEN_STATE_* は common 側と揃える */
#ifndef SCREEN_STATE_TITLE
#define SCREEN_STATE_TITLE 3
#endif

static SDL_Window *gMainWindow = NULL;
static SDL_Renderer *gMainRenderer = NULL;
static SDL_Texture *gBackgroundTexture = NULL;
static SDL_Texture *gResultTexture = NULL;
static TTF_Font *gFontLarge = NULL;
static TTF_Font *gFontNormal = NULL;

static char gAllClientNames[MAX_CLIENTS][MAX_NAME_SIZE];
static int gClientCount = 0;
 int gMyClientID = -1;
static int gXPressedFlags[MAX_CLIENTS] = {0};
static int gCurrentScreenState = SCREEN_STATE_TITLE;
static int gWeaponSent = 0;

/* プロトタイプ */
static void DrawImageAndText(void);
static void DrawText_Internal(const char* text,int x,int y,Uint8 r,Uint8 g,Uint8 b, TTF_Font* font);

/* GetRenderer (client_command から参照するため) */
SDL_Renderer* GetRenderer(void){ return gMainRenderer; }

/* DrawText_Internal: 文字描画 */
static void DrawText_Internal(const char* text,int x,int y,Uint8 r,Uint8 g,Uint8 b, TTF_Font* font){
    if (!font) return;
    SDL_Color color = {r,g,b,255};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font,text,color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(gMainRenderer,surf);
    SDL_Rect dst = {x,y,surf->w,surf->h};
    SDL_FreeSurface(surf);
    SDL_RenderCopy(gMainRenderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

/* DrawImageAndText: 状態に応じた描画 */
static void DrawImageAndText(void){
    int w=800,h=600;
    SDL_GetWindowSize(gMainWindow,&w,&h);
    SDL_RenderClear(gMainRenderer);

    if (gBackgroundTexture) SDL_RenderCopy(gMainRenderer,gBackgroundTexture,NULL,NULL);
    else { SDL_SetRenderDrawColor(gMainRenderer,0,100,0,255); SDL_RenderFillRect(gMainRenderer,NULL); }

    if (gCurrentScreenState == SCREEN_STATE_TITLE){
    int textW=0;
    if (gFontLarge) TTF_SizeUTF8(gFontLarge,"Tetra Conflict",&textW,NULL);
    DrawText_Internal("Tetra Conflict",(w-textW)/2,20,255,255,255,gFontLarge);

    DrawText_Internal("参加者:", (w/2)-100, h/2, 255,255,255, gFontNormal);

    // --------------------------
    // ★ 全員の名前を表示する部分 ★
    // --------------------------
    int baseY = h/2 + 40;     // 「参加者:」 の下
    int lineH = 30;

    for (int i = 0; i < gClientCount; i++){
    int nameX = (w/2) - 100;
    int nameY = baseY + i * lineH;

    // 名前を描画
    DrawText_Internal(gAllClientNames[i],
                      nameX,
                      nameY,
                      255,255,255,
                      gFontNormal);

    // ★ Xキーを押したクライアントには横に "X pressed" を表示
    if (gXPressedFlags[i] == 1){
        DrawText_Internal("  X pressed",
                          nameX + 200,   // 名前の右側に表示（位置は調整可）
                          nameY,
                          255, 200, 0,   // 目立つ黄色系
                          gFontNormal);
    }
}

}

    else if (gCurrentScreenState == SCREEN_STATE_GAME_SCREEN){
        int tw=0;
        if (gFontLarge) TTF_SizeUTF8(gFontLarge,"武器を選択",&tw,NULL);
        DrawText_Internal("武器を選択",(w-tw)/2,20,255,255,255,gFontLarge);

        const int P=20, Y_OFF=50;
        int rectW=(w-P*4)/2;
        int rectH=(h-P*7)/2;
        int leftX=(w-(rectW*2+P))/2;
        int rightX=leftX+rectW+P;
        int topY=(h-(rectH*2+P))/2 + Y_OFF;
        int bottomY=topY+rectH+P;
        SDL_Rect r={leftX,topY,rectW,rectH};
        SDL_SetRenderDrawColor(gMainRenderer,255,0,0,255); SDL_RenderFillRect(gMainRenderer,&r);
        r.x=rightX; SDL_SetRenderDrawColor(gMainRenderer,0,0,255,255); SDL_RenderFillRect(gMainRenderer,&r);
        r.x=leftX; r.y=bottomY; SDL_SetRenderDrawColor(gMainRenderer,255,255,0,255); SDL_RenderFillRect(gMainRenderer,&r);
        r.x=rightX; SDL_SetRenderDrawColor(gMainRenderer,0,255,0,255); SDL_RenderFillRect(gMainRenderer,&r);
    }
    else if (gCurrentScreenState == SCREEN_STATE_RESULT){
        if (gResultTexture) SDL_RenderCopy(gMainRenderer,gResultTexture,NULL,NULL);
        else { SDL_SetRenderDrawColor(gMainRenderer,153,204,0,255); SDL_RenderFillRect(gMainRenderer,NULL); }
    }

    SDL_RenderPresent(gMainRenderer);
}

/* InitWindows: 初期化 */
int InitWindows(int clientID,int num,char name[][MAX_NAME_SIZE]){
    int windowW=800, windowH=600;
    gMyClientID = clientID;
    gClientCount = num;
    for(int i=0;i<num;i++){ strncpy(gAllClientNames[i], name[i], MAX_NAME_SIZE-1); gAllClientNames[i][MAX_NAME_SIZE-1]='\0'; }

    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() == -1 || !(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
        fprintf(stderr,"SDL init failed\n"); return -1;
    }

    gFontLarge = TTF_OpenFont(FONT_PATH, 40);
    gFontNormal = TTF_OpenFont(FONT_PATH, 24);

    SDL_Surface *bg = IMG_Load(BACKGROUND_IMAGE);
    SDL_Surface *res = IMG_Load(RESULT_IMAGE);

    gMainWindow = SDL_CreateWindow("Game UI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowW, windowH, 0);
    gMainRenderer = SDL_CreateRenderer(gMainWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (bg) { gBackgroundTexture = SDL_CreateTextureFromSurface(gMainRenderer,bg); SDL_FreeSurface(bg); }
    if (res) { gResultTexture = SDL_CreateTextureFromSurface(gMainRenderer,res); SDL_FreeSurface(res); }

    /* 初期画面はタイトル */
    gCurrentScreenState = SCREEN_STATE_TITLE;
    DrawImageAndText();
    return 0;
}

/* DestroyWindow: 終了処理 */
void DestroyWindow(void){
    if (gFontLarge) TTF_CloseFont(gFontLarge);
    if (gFontNormal) TTF_CloseFont(gFontNormal);
    if (gResultTexture) SDL_DestroyTexture(gResultTexture);
    if (gBackgroundTexture) SDL_DestroyTexture(gBackgroundTexture);
    if (gMainRenderer) SDL_DestroyRenderer(gMainRenderer);
    if (gMainWindow) SDL_DestroyWindow(gMainWindow);
    IMG_Quit(); TTF_Quit(); SDL_Quit();
}

/* WindowEvent: 入力処理（X押下はサーバへ送信、遷移はサーバの START_GAME による） */
void WindowEvent(int num){
    SDL_Event event;
    if (SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT:
                gXPressedFlags[gMyClientID] = 1;
                DrawImageAndText();
                SendEndCommand();
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_x){
                    // 自分の準備フラグを立ててサーバに通知する
                    gXPressedFlags[gMyClientID] = 1;
                    DrawImageAndText();

                    unsigned char data[MAX_DATA];
                    int dataSize = 0;
                    SetCharData2DataBlock(data, X_COMMAND, &dataSize);
                    SetIntData2DataBlock(data, gMyClientID, &dataSize);
                    SendData(data, dataSize);

                    // 画面遷移自体はサーバが全員確認してから START_GAME_COMMAND を送る
                }
                else if (event.key.keysym.sym == SDLK_m){
                    SendEndCommand();
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (gCurrentScreenState == SCREEN_STATE_GAME_SCREEN){
                    int x = event.button.x;
                    int y = event.button.y;
                    int w, h;
                    SDL_GetWindowSize(gMainWindow, &w, &h);
                    const int P = 20, Y_OFF = 50;
                    int rectW = (w - P*4)/2;
                    int rectH = (h - P*7)/2;
                    int leftX = (w - (rectW*2 + P))/2;
                    int rightX = leftX + rectW + P;
                    int topY = (h - (rectH*2 + P))/2 + Y_OFF;
                    int bottomY = topY + rectH + P;
                    int selectedID = -1;
                    if (x >= leftX && x < leftX + rectW){
                        if (y >= topY && y < topY + rectH) selectedID = 0;
                        else if (y >= bottomY && y < bottomY + rectH) selectedID = 2;
                    } else if (x >= rightX && x < rightX + rectW){
                        if (y >= topY && y < topY + rectH) selectedID = 1;
                        else if (y >= bottomY && y < bottomY + rectH) selectedID = 3;
                    }

                    if (selectedID != -1 && gWeaponSent == 0){
                        unsigned char data[MAX_DATA];
                        int dataSize = 0;
                        SetCharData2DataBlock(data, SELECT_WEAPON_COMMAND, &dataSize);
                        SetIntData2DataBlock(data, selectedID, &dataSize);
                        SendData(data, dataSize);
                        gWeaponSent = 1;
                    }
                }
                break;
        }
    }
}

/* SetXPressedFlag: サーバーから UPDATE_X_COMMAND を受けたら表示更新 */
void SetXPressedFlag(int clientID){
    if (clientID < 0 || clientID >= MAX_CLIENTS) return;
    gXPressedFlags[clientID] = 1;
    DrawImageAndText();
}

/* SetScreenState: サーバーからの START_GAME / NEXT_SCREEN を呼ぶ */
void SetScreenState(int state){
    gCurrentScreenState = state;
    if (state == SCREEN_STATE_GAME_SCREEN) gWeaponSent = 0;
    DrawImageAndText();
}

