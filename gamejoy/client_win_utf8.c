/* client_win_utf8.c */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h> 
#include "common_utf8.h"
#include "client_func_utf8.h"
#include "joyconlib.h"  // Joy-Con用ライブラリ
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h> 
#include <stdlib.h> 
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define BACKGROUND_IMAGE "22823124.jpg" 
#define RESULT_IMAGE "Chatgpt.png"     
#define RESULT_BACK_IMAGE "2535410.jpg" 
#define YELLOW_WEAPON_ICON_IMAGE "1192635.png" // ID 2 用 (黄色)
#define BLUE_WEAPON_ICON_IMAGE "862582.png"   // ID 1 用 (青)
#define RED_WEAPON_ICON_IMAGE "23667746.png"  // ID 0 用 (赤)
#define GREEN_WEAPON_ICON_IMAGE "1499296.png" // ID 3 用 (緑)
#define PLAYER_0_IMAGE   "character_orihime_01.png"
#define PLAYER_1_IMAGE  "character_monster_yeti_02_blue.png"
#define PLAYER_2_IMAGE "character_monster_kyuketsuki_02_blue.png"
#define PLAYER_3_IMAGE "character_monster_zombie_green.png"
#define WALL_IMAGE "file_00000000f2ac71f89100b764e7b42a72.png"  // 壁用の画像
#define FONT_PATH "/usr/share/fonts/opentype/ipafont-gothic/ipagp.ttf"
#define DEFAULT_WINDOW_WIDTH 1300
#define DEFAULT_WINDOW_HEIGHT 1000
#define SCREEN_STATE_LOBBY_WAIT 0
#define SCREEN_STATE_GAME_SCREEN 1
#define SCREEN_STATE_RESULT 2
#define SCREEN_STATE_TITLE 3

#define MODE_MOVE 0
#define MODE_FIRE 1


#define HP_BAR_HEIGHT 5 // HPバーの高さ
#define HP_BAR_WIDTH 50 // HPバーの幅 
#define MAX_HP 150      // 最大体力 


SDL_Window *gMainWindow = NULL;
static SDL_Renderer *gMainRenderer = NULL;
static SDL_Texture *gBackgroundTexture = NULL;
static SDL_Texture *gResultTexture = NULL;
static SDL_Texture *gResultBackTexture = NULL;
static SDL_Texture *gYellowWeaponIconTexture = NULL; // ID 2 用
static SDL_Texture *gBlueWeaponIconTexture = NULL;   // ID 1 用
static SDL_Texture *gRedWeaponIconTexture = NULL;    // ID 0 用
static SDL_Texture *gGreenWeaponIconTexture = NULL;  // ID 3 用
static SDL_Texture *gPlayerTextures[MAX_CLIENTS] = {0};  //キャラクター画像を貼り付ける用のテキスチャー
static SDL_Texture *gWallTexture = NULL; // 壁のテクスチャ用
static TTF_Font *gFontLarge = NULL;
static TTF_Font *gFontNormal = NULL;
static TTF_Font *gFontCountdown = NULL;
static TTF_Font *gFontRank = NULL; 
static TTF_Font *gFontName = NULL;
static char gAllClientNames[MAX_CLIENTS][MAX_NAME_SIZE];
static int gClientCount = 0;
Uint32 gLastFireTime = 0; 
int gMyClientID = -1;
int gXPressedFlags[MAX_CLIENTS] = {0};
int gCurrentScreenState = SCREEN_STATE_LOBBY_WAIT; 
static int gWeaponSent = 0;
int gControlMode = MODE_MOVE; 
static int gPlayerPosX[MAX_CLIENTS];
static int gPlayerPosY[MAX_CLIENTS];
static joyconlib_t gJcR; // Joy-Con(R) 用の管理構造体
static int gIsJcRConnected = 0;
static int gPlayerMoveStep[MAX_CLIENTS]; 

int gSelectedWeaponID = -1;

static Mix_Chunk *gSoundReady = NULL; 
Mix_Chunk *gSoundFire = NULL;  
int gCountdownValue = -1;      
static Uint32 gCountdownStartTime = 0; 
static int IsHitWall(SDL_Rect *rect);

/* --- バトル制限時間（60秒） --- */
#define BATTLE_TIME_LIMIT_MS 90000
static Uint32 gBattleStartTime = 0;
static int gBattleTimerActive = 0;
static Uint32 gLastTimerRedraw = 0;

extern int gTrapActive;
extern int gTrapX;
extern int gTrapY;
extern int gTrapType;
extern void InitJoycon();         // joyconlib.c で定義
extern void ProcessJoyconInput(); // joyconlib.c で定義
extern void CleanupJoycon();      // joyconlib.c で定義
Projectile gProjectiles[MAX_PROJECTILES];

int gPlayerHP[MAX_CLIENTS]; 
int gWeaponStats[MAX_WEAPONS][MAX_STATS_PER_WEAPON] = {
    { 1000, 400, 10},  
    { 600, 200, 60}, 
    { 800, 1200, 40}, 
    { 400, 300, 20}
};
// ステータス名の定義（表示用）
char gStatNames[MAX_STATS_PER_WEAPON][MAX_STAT_NAME_SIZE] = {
    "クールタイム(ms)", 
    "球の飛距離(px)", 
    "球1つの威力", 
};

void InitProjectiles(void)
{
    memset(gProjectiles, 0, sizeof(gProjectiles));
}


void FillPolygon(SDL_Renderer *renderer, int centerX, int centerY, int radius, int sides) {
    if (sides < 3) 
    return;
    SDL_Point points[32]; 
    if (sides > 30) sides = 30;
    for (int i = 0; i < sides; i++) {
        float angle = i * 2.0f * (float)M_PI / sides - (float)M_PI / 2.0f;
        points[i].x = centerX + (int)(cosf(angle) * (float)radius);
        points[i].y = centerY + (int)(sinf(angle) * (float)radius);
    }

    int minY = centerY - radius;
    int maxY = centerY + radius;
    for (int y = minY; y <= maxY; y++) {
        int x_intersections[32];
        int count = 0;

        for (int i = 0; i < sides; i++) {
            int next = (i + 1) % sides;
            int x1 = points[i].x, y1 = points[i].y;
            int x2 = points[next].x, y2 = points[next].y;

            if ((y1 <= y && y2 > y) || (y2 <= y && y1 > y)) {
                x_intersections[count++] = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
            }
        }

        for (int i = 0; i < count - 1; i++) {
            for (int j = i + 1; j < count; j++) {
                if (x_intersections[i] > x_intersections[j]) {
                    int tmp = x_intersections[i];
                    x_intersections[i] = x_intersections[j];
                    x_intersections[j] = tmp;
                }
            }
        }

        for (int i = 0; i < count; i += 2) {
            SDL_RenderDrawLine(renderer, x_intersections[i], y, x_intersections[i+1], y);
        }
    }
}

void UpdateAndDrawProjectiles(void) {
    const int SIZE = 25;
    const int RADIUS = SIZE / 2;

    // ループ内での計算(移動・飛距離・壁判定)をすべて削除し、
    // 現在保持されている座標(gProjectiles[i].x, y)で描画するだけにします。
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!gProjectiles[i].active) continue;

        // ===== 描画処理のみ実行 =====
        int centerX = gProjectiles[i].x + RADIUS;
        int centerY = gProjectiles[i].y + RADIUS;

        SDL_SetRenderDrawColor(gMainRenderer, 0, 0, 0, 255);

        int weaponType = gProjectiles[i].clientID;
        switch (weaponType) {
            case 0:
                FillPolygon(gMainRenderer, centerX, centerY, RADIUS, 16);
                break;
            case 1: {
                SDL_Rect r = { gProjectiles[i].x, gProjectiles[i].y, SIZE, SIZE };
                SDL_RenderFillRect(gMainRenderer, &r);
                break;
            }
            case 2:
                FillPolygon(gMainRenderer, centerX, centerY, RADIUS, 3);
                break;
            case 3:
                FillPolygon(gMainRenderer, centerX, centerY, RADIUS, 5);
                break;
            default: {
                SDL_Rect r = { gProjectiles[i].x, gProjectiles[i].y, SIZE, SIZE };
                SDL_RenderFillRect(gMainRenderer, &r);
                break;
            }
        }
    }
}
void SendFireCommand(char direction)
{
    // サーバーに FIRE_COMMAND を送信
    unsigned char data[MAX_DATA];
    int dataSize = 0;
    SetCharData2DataBlock(data, FIRE_COMMAND, &dataSize); 
    // 自分のIDと現在の座標 (発射元の位置) を情報として送信
    SetIntData2DataBlock(data, gMyClientID, &dataSize);
    // プレイヤーの正方形(50px)の中央付近(20pxオフセット)を送信
    SetIntData2DataBlock(data, gPlayerPosX[gMyClientID] + 20, &dataSize); 
    SetIntData2DataBlock(data, gPlayerPosY[gMyClientID], &dataSize);
    SetCharData2DataBlock(data, direction, &dataSize);
    
    SendData(data, dataSize);
}

void SetScreenState(int state);

Uint32 BackToTitleScreen(Uint32 interval, void *param)
{
    printf("[DEBUG] BackToTitleScreen called - post user event\n");
    fflush(stdout);
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = SDL_USEREVENT;
    ev.user.code = 0;
    ev.user.data1 = (void*)(intptr_t)SCREEN_STATE_TITLE;
    ev.user.data2 = NULL;
    SDL_PushEvent(&ev);
    return 0; 
}

/* プロトタイプ */
void DrawImageAndText(void);
static void DrawText_Internal(const char* text,int x,int y,Uint8 r,Uint8 g,Uint8 b, TTF_Font* font){
    // フォントがロードされていれば描画する
    if (!font || !text) return; 
    SDL_Color color = {r,g,b,255};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font,text,color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(gMainRenderer,surf);
    SDL_Rect dst = {x,y,surf->w,surf->h};
    SDL_FreeSurface(surf);
    SDL_RenderCopy(gMainRenderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

static int IsHitWall(SDL_Rect *rect)
{
    int w, h;
    SDL_GetWindowSize(gMainWindow, &w, &h);

    int blockCount = 8;
    int blockSize = 150;
    int cols = 4;
    int rows = 2;
    int cell_w = w / cols;
    int cell_h = h / rows;

    for (int i = 0; i < blockCount; i++) {
        SDL_Rect blockRect;
        blockRect.x = (i % cols) * cell_w + (cell_w / 2) - (blockSize / 2);
        blockRect.y = (i / cols) * cell_h + (cell_h / 2) - (blockSize / 2);
        blockRect.w = blockSize;
        blockRect.h = blockSize;

        if (SDL_HasIntersection(rect, &blockRect)) {
            return 1; // 壁に衝突
        }
    }
    return 0;
}

// プレイヤーIDとそのHPを保持する構造体
typedef struct {
    int id;
    int hp;
} PlayerScore;

// qsort用の比較関数 (HPが高い順にソート)
int ComparePlayerScores(const void *a, const void *b) {
    const PlayerScore *scoreA = (const PlayerScore *)a;
    const PlayerScore *scoreB = (const PlayerScore *)b;
    return scoreB->hp - scoreA->hp;
}

// HPに基づいてプレイヤーIDの順位リストを返す関数
void GetRankedPlayerIDs(int *rankedIDs) {
    PlayerScore scores[MAX_CLIENTS];
    int numClients = gClientCount; 
    for (int i = 0; i < numClients; i++) {
        scores[i].id = i;
        scores[i].hp = gPlayerHP[i]; // 現在のHPを使用
    }

    // HPが高い順にソート
    qsort(scores, numClients, sizeof(PlayerScore), ComparePlayerScores);

    for (int i = 0; i < numClients; i++) {
        rankedIDs[i] = scores[i].id;
    }
}

/* DrawImageAndText: 状態に応じた描画 */
void DrawImageAndText(void){
    int w=800,h=600;
    SDL_GetWindowSize(gMainWindow,&w,&h);
    SDL_RenderClear(gMainRenderer);

    if (gCurrentScreenState == SCREEN_STATE_LOBBY_WAIT){
        /* --- ロビー待機画面の描画 --- */
        if (gBackgroundTexture) {
            SDL_RenderCopy(gMainRenderer, gBackgroundTexture, NULL, NULL);
        } else { 
            SDL_SetRenderDrawColor(gMainRenderer, 0, 100, 0, 255); 
            SDL_RenderFillRect(gMainRenderer, NULL); 
        }

        int textW = 0;
        int titleY = 20; 
        if (gFontLarge) TTF_SizeUTF8(gFontLarge, "Tetra Conflict", &textW, NULL);
        DrawText_Internal("Tetra Conflict", (w - textW) / 2, titleY, 255, 255, 255, gFontLarge);

        const char* promptMsg = "Xキーを押してください";
        int promptW = 0;
        int promptY = titleY + 60;
        if (gFontName) TTF_SizeUTF8(gFontName, promptMsg, &promptW, NULL); 
        DrawText_Internal(promptMsg, (w - promptW) / 2, promptY, 255, 200, 0, gFontName);

        DrawText_Internal("参加者:", (w / 2) - 100, h / 2, 255, 255, 255, gFontName);
        int baseY = h / 2 + 40;
        int lineH = 40; 
        for (int i = 0; i < 4; i++){ 
            if (i >= gClientCount) 
                continue;
            int iconSize = 48;
            int iconX = (w / 2) - 160;
            int iconY = baseY + i * lineH - 8;
            int nameX = (w / 2) - 100;
            int nameY = baseY + i * lineH;
            const char* nameToDraw = (i < gClientCount) ? gAllClientNames[i] : "";
	    if (gPlayerTextures[i]) {
                        SDL_Rect dst = { iconX, iconY, iconSize, iconSize };
                        SDL_RenderCopy(gMainRenderer, gPlayerTextures[i], NULL, &dst);
                }
            DrawText_Internal(nameToDraw, nameX, nameY, 255, 255, 255, gFontName);

            if (i < gClientCount && gXPressedFlags[i] == 1){
                DrawText_Internal(" X Pressed", nameX + 200, nameY, 255, 200, 0, gFontName);
            }
        }
    }
    else if (gCurrentScreenState == SCREEN_STATE_GAME_SCREEN){
        /* --- 武器選択画面の描画 --- */
        if (gBackgroundTexture) {
            SDL_RenderCopy(gMainRenderer, gBackgroundTexture, NULL, NULL);
        } else { 
            SDL_SetRenderDrawColor(gMainRenderer, 0, 100, 0, 255); 
            SDL_RenderFillRect(gMainRenderer, NULL); 
        }

        const int P = 20, Y_OFF = 50;
        int rectW = (w - P * 4) / 2;  
        int rectH = (h - P * 7) / 2;  
        int leftX = (w - (rectW * 2 + P)) / 2;
        int rightX = leftX + rectW + P;
        int topY = (h - (rectH * 2 + P)) / 2 + Y_OFF;
        int bottomY = topY + rectH + P;
        
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        for (int i = 0; i < MAX_WEAPONS; i++) {
            SDL_Rect r;
            if (i == 0) 
                { r.x = leftX;  r.y = topY; }
            else if (i == 1) 
                { r.x = rightX; r.y = topY; }
            else if (i == 2) 
                { r.x = leftX;  r.y = bottomY; }
            else if (i == 3) 
                { r.x = rightX; r.y = bottomY; }
            r.w = rectW; 
            r.h = rectH;

            Uint8 baseR = 180, baseG = 190, baseB = 200;
            Uint8 hoverR = 140, hoverG = 150, hoverB = 160;

            int isHover = (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h);
            Uint8 rColor = baseR, gColor = baseG, bColor = baseB;
            if (isHover || gSelectedWeaponID == i) {
                rColor = hoverR; gColor = hoverG; bColor = hoverB;
            }

            SDL_SetRenderDrawColor(gMainRenderer, rColor, gColor, bColor, 255); 
            SDL_RenderFillRect(gMainRenderer, &r);

            int textPadding = 10;
            int lineHeight = 22; 
            for (int j = 0; j < MAX_STATS_PER_WEAPON; j++) {   
            char statText[64];
            sprintf(statText, "%s: %d", gStatNames[j], gWeaponStats[i][j]);
            DrawText_Internal(statText, r.x + textPadding, r.y + textPadding + (j * lineHeight), 255, 255, 255, gFontNormal);
            }

            SDL_Texture *iconToDraw = NULL;
            if (i == 0) 
                iconToDraw = gRedWeaponIconTexture;
            else if (i == 1) 
                iconToDraw = gBlueWeaponIconTexture;
            else if (i == 2) 
                iconToDraw = gYellowWeaponIconTexture;
            else if (i == 3) 
                iconToDraw = gGreenWeaponIconTexture; 

            if (iconToDraw) {
                int iconW = 150, iconH = 64, margin = 10; 
                SDL_Rect destRect = { r.x + textPadding, r.y + r.h - iconH - margin, iconW, iconH }; 
                SDL_RenderCopy(gMainRenderer, iconToDraw, NULL, &destRect);
            }
        }
    }
    else if (gCurrentScreenState == SCREEN_STATE_RESULT) {
        /* --- メインゲーム（バトル）画面の描画 --- */
        if (gResultTexture) {
            SDL_RenderCopy(gMainRenderer, gResultTexture, NULL, NULL);
        } else {
            SDL_SetRenderDrawColor(gMainRenderer, 153, 204, 0, 255); 
            SDL_RenderFillRect(gMainRenderer, NULL); 
        }

        {
            int blockCount = 8;
            int blockSize = 150;
            int cols = 4;
            int rows = 2;
            int cell_w = w / cols;
            int cell_h = h / rows;

            for (int i = 0; i < blockCount; i++) {
                int r = i / cols;
                int c = i % cols;

                SDL_Rect blockRect;
                blockRect.x = c * cell_w + (cell_w / 2) - (blockSize / 2);
                blockRect.y = r * cell_h + (cell_h / 2) - (blockSize / 2);
                blockRect.w = blockSize;
                blockRect.h = blockSize;

                if (gWallTexture) {
                    SDL_RenderCopy(gMainRenderer, gWallTexture, NULL, &blockRect);
                } else {
                    SDL_SetRenderDrawColor(gMainRenderer, 100, 100, 100, 255);
                    SDL_RenderFillRect(gMainRenderer, &blockRect);
                    SDL_SetRenderDrawColor(gMainRenderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(gMainRenderer, &blockRect);
                }
            }
        }

        if (gTrapActive) {
            SDL_Rect tr = { gTrapX, gTrapY, 80, 80 };
            if (gTrapType == TRAP_TYPE_HEAL) 
                SDL_SetRenderDrawColor(gMainRenderer, 255, 255, 0, 255);
            else if (gTrapType == TRAP_TYPE_DAMAGE) 
                SDL_SetRenderDrawColor(gMainRenderer, 255, 0, 0, 255);
            else if (gTrapType == TRAP_TYPE_POISON) 
                SDL_SetRenderDrawColor(gMainRenderer, 0, 200, 0, 255);
            else
            SDL_SetRenderDrawColor(gMainRenderer, 0, 180, 255, 255);
            SDL_RenderFillRect(gMainRenderer, &tr);
            SDL_SetRenderDrawColor(gMainRenderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(gMainRenderer, &tr);
        }

        char modeMsg[64];
        if (gControlMode == MODE_MOVE) 
            sprintf(modeMsg, "MODE: 移動 (Cキーで発射モードへ)");
        else 
            sprintf(modeMsg, "MODE: 発射 (Cキーで移動モードへ)");

        DrawText_Internal(modeMsg, 10, 10, 255, 255, 0, gFontNormal);

        /* --- 制限時間タイマー表示（画面上部中央） --- */
        if (gBattleTimerActive) {
    Uint32 now = SDL_GetTicks();

    // ★ 3秒待ってからカウント開始
    Uint32 countStartTime = gBattleStartTime + 3000;

    int remainSec;

    if (now < countStartTime) {
        // まだ3秒待機中
        remainSec = BATTLE_TIME_LIMIT_MS / 1000;
    } else {
        // 通常のカウントダウン
        remainSec = (int)((BATTLE_TIME_LIMIT_MS - (int)(now - countStartTime) + 999) / 1000);
    }

    if (remainSec < 0) remainSec = 0;
    if (remainSec > 90) remainSec = 90;

    char timerStr[64];
    sprintf(timerStr, "TIME: %d", remainSec);

    int tW = 0, tH = 0;
    if (gFontNormal) {
        TTF_SizeUTF8(gFontNormal, timerStr, &tW, &tH);
        DrawText_Internal(timerStr, (w - tW) / 2, 10, 255, 255, 255, gFontNormal);
    }
}


        const int SQ_SIZE = 50; 
        for (int i = 0; i < gClientCount; i++) {
            int currentHP = gPlayerHP[i]; 
            SDL_Rect playerRect = { gPlayerPosX[i], gPlayerPosY[i], SQ_SIZE, SQ_SIZE };
            Uint8 r = 0, g = 0, b = 0;
            if (currentHP <= 0) {
                r = 128; g = 128; b = 128;
            } else {
                if (i == 0)      
                    { r = 255; g = 0;   b = 0;   }
                else if (i == 1) 
                    { r = 0;   g = 0;   b = 255; }
                else if (i == 2) 
                    { r = 255; g = 255; b = 0;   }
                else if (i == 3) 
                    { r = 0;   g = 255; b = 0;   }
            }

	    SDL_RenderCopy(gMainRenderer, gPlayerTextures[i], NULL, &playerRect);
            
            if (currentHP > 0) {
                /* HP数値の表示 */
                char hpText[16];
                sprintf(hpText, "HP: %d", currentHP);
                Uint8 textR = 255, textG = 255, textB = 255;
                if (currentHP <= MAX_HP / 4) 
                    { textR = 255; textG = 0; textB = 0; }
                    
                DrawText_Internal(hpText, gPlayerPosX[i], gPlayerPosY[i] - 20, textR, textG, textB, gFontNormal);

                /* クールタイム数値の表示 (自機のみ) */
                if (i == gMyClientID) {
                    Uint32 currentTime = SDL_GetTicks();
                    int weaponID = (gSelectedWeaponID < 0) ? 0 : gSelectedWeaponID;
                    int totalCT = gWeaponStats[weaponID][STAT_CT_TIME];
                    Uint32 elapsed = currentTime - gLastFireTime;

                    // クールタイム中のみ数値を表示
                    if (elapsed < (Uint32)totalCT) {
                        float remaining = (float)(totalCT - elapsed) / 1000.0f;
                        char ctText[32]; 
                        sprintf(ctText, "%.1fs", remaining);
                        DrawText_Internal(ctText, gPlayerPosX[i], gPlayerPosY[i] - 45, 255, 100, 100, gFontNormal);
                    }
                }
            }
        }
        UpdateAndDrawProjectiles();
        /* カウントダウン表示 */
        if (gCountdownValue > 0) {
            Uint32 currentTime = SDL_GetTicks();
            Uint32 elapsed = currentTime - gCountdownStartTime;

            if (elapsed < 3000) {
                // 残り秒数を計算 (3, 2, 1)
                gCountdownValue = 3 - (elapsed / 1000);
                
                char countStr[4];
                sprintf(countStr, "%d", gCountdownValue);
                
                int textW = 0, textH = 0;
                if (gFontCountdown) {
            TTF_SizeUTF8(gFontCountdown, countStr, &textW, &textH);

            DrawText_Internal(countStr, (w - textW) / 2, (h - textH) / 2, 0, 0, 0, gFontCountdown);
        }
            } else {
                gCountdownValue = 0; // 3秒経過で終了
            }
        }
    }
    else if (gCurrentScreenState == SCREEN_STATE_TITLE) {
        /* --- 結果発表画面の描画 --- */
        int rankedIDs[MAX_CLIENTS];
        GetRankedPlayerIDs(rankedIDs);

        if (gResultBackTexture) 
            SDL_RenderCopy(gMainRenderer, gResultBackTexture, NULL, NULL);
        else 
            { SDL_SetRenderDrawColor(gMainRenderer, 0, 0, 255, 255); SDL_RenderFillRect(gMainRenderer, NULL); }

        int isDraw = 0;
        if (gClientCount >= 2 && gPlayerHP[rankedIDs[0]] == gPlayerHP[rankedIDs[1]]) 
            isDraw = 1;
        else if (gClientCount == 1 && gPlayerHP[rankedIDs[0]] <= 0) 
            isDraw = 1;

        int titleBottomY = 0;
        if (gFontLarge) {
            int textW = 0, textH = 0;
            const char* titleMsg = isDraw ? "引き分け！ (DRAW)" : "結果発表!!";
            TTF_SizeUTF8(gFontLarge, titleMsg, &textW, &textH);
            DrawText_Internal(titleMsg, (w - textW) / 2, 40, 255, 255, 255, gFontLarge);
            titleBottomY = 40 + textH;
        }

        int helpTextY = h - 40;
        if (gFontNormal) {
            const char* msg = "参加者全員がXボタンを押すと、ゲームが終了します。";
            int textW = 0, textH = 0;
            TTF_SizeUTF8(gFontNormal, msg, &textW, &textH);
            helpTextY = h - textH - 40;
            DrawText_Internal(msg, (w - textW) / 2, helpTextY, 0, 0, 200, gFontNormal);
        }

        int topMargin = 20, rectCount = gClientCount, spacing = 10;
        int availableHeight = (helpTextY - 20) - (titleBottomY + topMargin);
        int rectH = (availableHeight - spacing * (rectCount - 1)) / rectCount;
        int rectW = 450, startX = (w - rectW) / 2;

        for (int i = 0; i < rectCount; i++) {
            int rectY = titleBottomY + topMargin + i * (rectH + spacing);
	    int iconSize = rectH - 10;
            int iconX = startX + 10;
            int iconY = rectY + (rectH - iconSize) / 2;
            int textBaseX = startX + iconSize + 25;
            int clientID = rankedIDs[i];
            int displayRank = i + 1;
            if (i > 0 && gPlayerHP[rankedIDs[i]] == gPlayerHP[rankedIDs[i - 1]]) 
                displayRank = i;

	    if (gPlayerTextures[clientID]) {
                    SDL_Rect iconRect = { iconX, iconY, iconSize, iconSize };
                    SDL_RenderCopy(gMainRenderer, gPlayerTextures[clientID], NULL, &iconRect);
            }

            char rankText[16];
            sprintf(rankText, "%d位:", displayRank);
            int tw = 0, th = 0;
            TTF_SizeUTF8(gFontRank, rankText, &tw, &th);
	    DrawText_Internal(rankText, textBaseX, rectY + (rectH - th) / 2, 0, 0, 255, gFontRank);

            char nameAndHP[MAX_NAME_SIZE + 20]; 
            sprintf(nameAndHP, "%s (HP: %d)", gAllClientNames[clientID], gPlayerHP[clientID]);
            int nw = 0, nh = 0;
            TTF_SizeUTF8(gFontName, nameAndHP, &nw, &nh);
	    DrawText_Internal(nameAndHP, textBaseX + tw + 20, rectY + (rectH - nh) / 2, 0, 0, 255, gFontName);

            if (gXPressedFlags[clientID] == 1) {
                DrawText_Internal("X Pressed", startX + tw + nw + 310, rectY + (rectH - nh) / 2 + 25, 255, 0, 0, gFontName);
            }
        }
    }
    SDL_RenderPresent(gMainRenderer);
}
SDL_Renderer* GetRenderer(void){ return gMainRenderer; }

int InitWindows(int clientID, int num, char name[][MAX_NAME_SIZE]) {
    int windowW = DEFAULT_WINDOW_WIDTH;
    int windowH = DEFAULT_WINDOW_HEIGHT;
    gMyClientID = clientID;
    gClientCount = num;
    for (int i = 0; i < num; i++) {
        strncpy(gAllClientNames[i], name[i], MAX_NAME_SIZE - 1);
        gAllClientNames[i][MAX_NAME_SIZE - 1] = '\0';
    }

    /* --- 1. SDLの初期化に AUDIO を追加 --- */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0 || !(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & IMG_INIT_JPG)) 
        {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return -1;
    }

    /* --- 2. SDL_mixer の初期化 --- */
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "SDL_mixer init failed: %s\n", Mix_GetError());
       
    }
    InitJoycon();

    /* --- 3. 音声ファイルの読み込み --- */
    gSoundReady = Mix_LoadWAV("銃を構える.mp3");
    if (gSoundReady == NULL) {
        fprintf(stderr, "Failed to load sound! SDL_mixer Error: %s\n", Mix_GetError());
    }

    gSoundFire = Mix_LoadWAV("拳銃を撃つ.mp3");
    if (gSoundFire == NULL) {
        fprintf(stderr, "Failed to load fire sound! SDL_mixer Error: %s\n", Mix_GetError());
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
    }

    // --- フォント読み込み ---
    gFontLarge = TTF_OpenFont(FONT_PATH, 40);
    gFontNormal = TTF_OpenFont(FONT_PATH, 24);
    
    gFontCountdown = TTF_OpenFont(FONT_PATH, 200);
    gFontRank = TTF_OpenFont(FONT_PATH, 36);
    gFontName = TTF_OpenFont(FONT_PATH, 36);
    if (!gFontLarge || !gFontNormal) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
    }

    // --- 画像読み込み（既存通り） ---
    SDL_Surface *bg = IMG_Load(BACKGROUND_IMAGE);
    SDL_Surface *res = IMG_Load(RESULT_IMAGE);
    SDL_Surface *resBack = IMG_Load(RESULT_BACK_IMAGE);
    SDL_Surface *icon_yellow = IMG_Load(YELLOW_WEAPON_ICON_IMAGE);
    SDL_Surface *icon_blue = IMG_Load(BLUE_WEAPON_ICON_IMAGE);
    SDL_Surface *icon_red = IMG_Load(RED_WEAPON_ICON_IMAGE);
    SDL_Surface *icon_green = IMG_Load(GREEN_WEAPON_ICON_IMAGE);
    SDL_Surface *p0 = IMG_Load(PLAYER_0_IMAGE);
    SDL_Surface *p1 = IMG_Load(PLAYER_1_IMAGE);
    SDL_Surface *p2 = IMG_Load(PLAYER_2_IMAGE);
    SDL_Surface *p3 = IMG_Load(PLAYER_3_IMAGE);
    SDL_Surface *wallSurf = IMG_Load("file_00000000f2ac71f89100b764e7b42a72.png");
    const char *myWindowTitle = gAllClientNames[gMyClientID];
    gMainWindow = SDL_CreateWindow(myWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowW, windowH, 0);

    gMainRenderer = SDL_CreateRenderer(gMainWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (bg) 
        { gBackgroundTexture = SDL_CreateTextureFromSurface(gMainRenderer, bg); SDL_FreeSurface(bg); }
    if (res) 
        { gResultTexture = SDL_CreateTextureFromSurface(gMainRenderer, res); SDL_FreeSurface(res); }
    if (resBack) 
        { gResultBackTexture = SDL_CreateTextureFromSurface(gMainRenderer, resBack); SDL_FreeSurface(resBack); }

    if (icon_yellow) 
        { gYellowWeaponIconTexture = SDL_CreateTextureFromSurface(gMainRenderer, icon_yellow); SDL_FreeSurface(icon_yellow); }
    if (icon_blue) 
        { gBlueWeaponIconTexture = SDL_CreateTextureFromSurface(gMainRenderer, icon_blue); SDL_FreeSurface(icon_blue); }
    if (icon_red)
        { gRedWeaponIconTexture = SDL_CreateTextureFromSurface(gMainRenderer, icon_red); SDL_FreeSurface(icon_red); }
    if (icon_green) 
        { gGreenWeaponIconTexture = SDL_CreateTextureFromSurface(gMainRenderer, icon_green); SDL_FreeSurface(icon_green); }
    if (p0) 
        { gPlayerTextures[0] = SDL_CreateTextureFromSurface(gMainRenderer, p0); SDL_FreeSurface(p0); }
    if (p1) 
        { gPlayerTextures[1] = SDL_CreateTextureFromSurface(gMainRenderer, p1); SDL_FreeSurface(p1); }
    if (p2) 
        { gPlayerTextures[2] = SDL_CreateTextureFromSurface(gMainRenderer, p2); SDL_FreeSurface(p2); }
    if (p3) 
        { gPlayerTextures[3] = SDL_CreateTextureFromSurface(gMainRenderer, p3); SDL_FreeSurface(p3); }

    if (wallSurf) {
        gWallTexture = SDL_CreateTextureFromSurface(gMainRenderer, wallSurf);
        SDL_FreeSurface(wallSurf);
    } else {
        fprintf(stderr, "Failed to load wall image: %s\n", IMG_GetError());
    }

    gCurrentScreenState = SCREEN_STATE_LOBBY_WAIT;

    int w = DEFAULT_WINDOW_WIDTH;
    int h = DEFAULT_WINDOW_HEIGHT;
    const int SQ_SIZE = 50;
    const int PADDING = 50;
    const int S_SIZE = SQ_SIZE;

    for (int i = 0; i < gClientCount; i++) {
        gPlayerMoveStep[i] = 10;
        switch (i) {
            case 0: gPlayerPosX[i] = PADDING; gPlayerPosY[i] = PADDING; break;
            case 1: gPlayerPosX[i] = w - PADDING - S_SIZE; gPlayerPosY[i] = h - PADDING - S_SIZE; break;
            case 2: gPlayerPosX[i] = w - PADDING - S_SIZE; gPlayerPosY[i] = PADDING; break;
            case 3: gPlayerPosX[i] = PADDING; gPlayerPosY[i] = h - PADDING - S_SIZE; break;
        }
        gPlayerHP[i] = MAX_HP;
    }
    InitProjectiles();
    DrawImageAndText();
    return 0;
}

/* DestroyWindow: 終了処理 */
void DestroyWindow(void){
    CleanupJoycon();

    // TTF_CloseFont を維持
    if (gFontLarge) 
        TTF_CloseFont(gFontLarge);
    if (gFontNormal) 
        TTF_CloseFont(gFontNormal);
    if (gFontCountdown) 
        TTF_CloseFont(gFontCountdown); 
    if (gFontRank) 
        TTF_CloseFont(gFontRank); 
    if (gResultTexture) 
        SDL_DestroyTexture(gResultTexture);
    if (gBackgroundTexture) 
        SDL_DestroyTexture(gBackgroundTexture);
    if (gResultBackTexture) 
        SDL_DestroyTexture(gResultBackTexture); 
    if (gYellowWeaponIconTexture) 
        SDL_DestroyTexture(gYellowWeaponIconTexture);
    if (gBlueWeaponIconTexture) 
        SDL_DestroyTexture(gBlueWeaponIconTexture); 
    if (gRedWeaponIconTexture) 
        SDL_DestroyTexture(gRedWeaponIconTexture); 
    if (gGreenWeaponIconTexture) 
        SDL_DestroyTexture(gGreenWeaponIconTexture); 
    if (gMainRenderer) 
        SDL_DestroyRenderer(gMainRenderer);
    if (gMainWindow) 
        SDL_DestroyWindow(gMainWindow);
    if (gSoundFire) 
        Mix_FreeChunk(gSoundFire); 
    if (gSoundReady) 
        Mix_FreeChunk(gSoundReady);
    if (gWallTexture) 
        SDL_DestroyTexture(gWallTexture);

    Mix_CloseAudio();
    
    IMG_Quit(); 
    TTF_Quit(); 
    SDL_Quit();
}

/* --- マウスとJoy-Con共通の武器選択処理 --- */
void HandleWeaponSelection(int x, int y) {
    if (gCurrentScreenState != SCREEN_STATE_GAME_SCREEN) 
        return;

    int win_w, win_h;
    SDL_GetWindowSize(gMainWindow, &win_w, &win_h);
    const int P = 20, Y_OFF = 50;
    int rectW = (win_w - P * 4) / 2;
    int rectH = (win_h - P * 7) / 2;
    int leftX = (win_w - (rectW * 2 + P)) / 2;
    int rightX = leftX + rectW + P;
    int topY = (win_h - (rectH * 2 + P)) / 2 + Y_OFF;
    int bottomY = topY + rectH + P;
    int selectedID = -1;

    if (x >= leftX && x < leftX + rectW) {
        if (y >= topY && y < topY + rectH) 
            selectedID = 0;
        else if (y >= bottomY && y < bottomY + rectH) 
            selectedID = 2;
    } else if (x >= rightX && x < rightX + rectW) 
        {
        if (y >= topY && y < topY + rectH) 
            selectedID = 1;
        else if (y >= bottomY && y < bottomY + rectH) 
            selectedID = 3;
        }

    if (selectedID != -1 && gWeaponSent == 0) {
        gSelectedWeaponID = selectedID;
        DrawImageAndText();
        unsigned char data[MAX_DATA];
        int dataSize = 0;
        SetCharData2DataBlock(data, SELECT_WEAPON_COMMAND, &dataSize);
        SetIntData2DataBlock(data, selectedID, &dataSize);
        SendData(data, dataSize);
        gWeaponSent = 1;
    }
}
/* WindowEvent: 入力処理  */
void WindowEvent(int num){
    if(gCurrentScreenState == SCREEN_STATE_RESULT &&gPlayerHP[gMyClientID] <= 0)
    {
	    return;
    }
      ProcessJoyconInput();

    /* --- バトル制限時間の進行と強制遷移 --- */
    if (gCurrentScreenState == SCREEN_STATE_RESULT && gBattleTimerActive) {
        Uint32 now = SDL_GetTicks();

        /* 60秒経過で強制的に結果発表画面へ */
        if (now - gBattleStartTime >= BATTLE_TIME_LIMIT_MS) {
            gBattleTimerActive = 0;
            SetScreenState(SCREEN_STATE_TITLE);
            return;
        }

        /* タイマー表示を更新するため、一定間隔で再描画 */
        if (now - gLastTimerRedraw >= 200) {
            gLastTimerRedraw = now;
            DrawImageAndText();
        }
    }
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        if (gCurrentScreenState == SCREEN_STATE_RESULT && gCountdownValue > 0) {
            if (event.type != SDL_QUIT) {
                return; 
            }
        }

        switch (event.type) {
            case SDL_QUIT:
                gXPressedFlags[gMyClientID] = 1;
                DrawImageAndText();
                SendEndCommand();
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_x && (gCurrentScreenState == SCREEN_STATE_LOBBY_WAIT || gCurrentScreenState == SCREEN_STATE_TITLE))
                {
                    if (gXPressedFlags[gMyClientID] == 0) 
                    {
                        gXPressedFlags[gMyClientID] = 1;
                        DrawImageAndText();
                        SendXCommandWithState(gMyClientID, gCurrentScreenState);
                    }
                }
                else if (event.key.keysym.sym == SDLK_m) 
                {
                    SendEndCommand();
                }

                if (gCurrentScreenState == SCREEN_STATE_RESULT) {   
                    if (gPlayerHP[gMyClientID] <= 0) 
                    {
                        break; 
                    }

                    if (event.key.keysym.sym == SDLK_c) 
                    {
                        gControlMode = (gControlMode == MODE_MOVE) ? MODE_FIRE : MODE_MOVE;
                        DrawImageAndText(); 
                        break;
                    }

                    if (gControlMode == MODE_MOVE) 
                    {
                        const Uint8 *keyState = SDL_GetKeyboardState(NULL);
                        char direction = 0;
                        if (keyState[SDL_SCANCODE_UP] && keyState[SDL_SCANCODE_LEFT]) 
                            direction = DIR_UP_LEFT;
                        else if (keyState[SDL_SCANCODE_UP] && keyState[SDL_SCANCODE_RIGHT]) 
                            direction = DIR_UP_RIGHT;
                        else if (keyState[SDL_SCANCODE_DOWN] && keyState[SDL_SCANCODE_LEFT]) 
                            direction = DIR_DOWN_LEFT;
                        else if (keyState[SDL_SCANCODE_DOWN] && keyState[SDL_SCANCODE_RIGHT]) 
                            direction = DIR_DOWN_RIGHT;
                        else if (keyState[SDL_SCANCODE_UP]) 
                            direction = DIR_UP;
                        else if (keyState[SDL_SCANCODE_DOWN]) 
                            direction = DIR_DOWN;
                        else if (keyState[SDL_SCANCODE_LEFT]) 
                            direction = DIR_LEFT;
                        else if (keyState[SDL_SCANCODE_RIGHT]) 
                            direction = DIR_RIGHT;

                        if (direction != 0) 
                        {
                            unsigned char data[MAX_DATA];
                            int dataSize = 0;
                            SetCharData2DataBlock(data, MOVE_COMMAND, &dataSize);
                            SetCharData2DataBlock(data, direction, &dataSize);
                            SendData(data, dataSize);
                        }
                    }
                    else if (gControlMode == MODE_FIRE) 
                    {
                        if (event.key.keysym.sym == SDLK_SPACE) 
                        {
                            Uint32 currentTime = SDL_GetTicks();
                            int weaponID = gSelectedWeaponID < 0 ? 0 : gSelectedWeaponID;
                            int ct = gWeaponStats[weaponID][STAT_CT_TIME];
                            if (currentTime - gLastFireTime >= (Uint32)ct) 
                            {
                                const Uint8 *state = SDL_GetKeyboardState(NULL);
                                char fireDirection = 0;
                                if (state[SDL_SCANCODE_UP] && state[SDL_SCANCODE_LEFT]) 
                                    fireDirection = DIR_UP_LEFT;
                                else if (state[SDL_SCANCODE_UP] && state[SDL_SCANCODE_RIGHT]) 
                                    fireDirection = DIR_UP_RIGHT;
                                else if (state[SDL_SCANCODE_DOWN] && state[SDL_SCANCODE_LEFT]) 
                                    fireDirection = DIR_DOWN_LEFT;
                                else if (state[SDL_SCANCODE_DOWN] && state[SDL_SCANCODE_RIGHT]) 
                                    fireDirection = DIR_DOWN_RIGHT;
                                else if (state[SDL_SCANCODE_UP]) 
                                    fireDirection = DIR_UP;
                                else if (state[SDL_SCANCODE_DOWN]) 
                                    fireDirection = DIR_DOWN;
                                else if (state[SDL_SCANCODE_LEFT]) 
                                    fireDirection = DIR_LEFT;
                                else if (state[SDL_SCANCODE_RIGHT]) 
                                    fireDirection = DIR_RIGHT;
                                
                                if (fireDirection != 0) {
                                    SendFireCommand(fireDirection);
                                    gLastFireTime = currentTime;
                                    if (gSoundFire != NULL) {
                                        Mix_PlayChannel(-1, gSoundFire, 0);
                                    }
                                }
                            }
                        }
                    }
                } 
                break; 

           case SDL_MOUSEBUTTONDOWN: 
               HandleWeaponSelection(event.button.x, event.button.y);
                break;

            case SDL_USEREVENT: {
                    int reqState = (int)(intptr_t)event.user.data1;
                    SetScreenState(reqState);
                }
                break;
        } 
    } 
}

void SetPlayerMoveStep(int clientID, int step) 
{
    if (clientID < 0 || clientID >= MAX_CLIENTS) return;
    gPlayerMoveStep[clientID] = step;
}

void SetXPressedFlag(int clientID)
{
    if (clientID < 0 || clientID >= MAX_CLIENTS) return;
    gXPressedFlags[clientID] = 1;
    DrawImageAndText();
}

void SetScreenState(int state)
{
    gCurrentScreenState = state;  
    if (state == SCREEN_STATE_GAME_SCREEN) 
    {
        gWeaponSent = 0;
        gSelectedWeaponID = -1;
        gBattleTimerActive = 0;
    } 
    else if (state == SCREEN_STATE_LOBBY_WAIT) 
    {
        memset(gXPressedFlags, 0, sizeof(gXPressedFlags));
        gBattleTimerActive = 0;
    }
    else if (state == SCREEN_STATE_RESULT) 
    {
        if (gSoundReady != NULL) 
        {
            Mix_PlayChannel(-1, gSoundReady, 0);
        }

        gCountdownValue = 3;
        gCountdownStartTime = SDL_GetTicks();
        gLastFireTime = SDL_GetTicks(); 

        /* バトル開始時刻を記録（ここから60秒で強制遷移） */
        gBattleStartTime = SDL_GetTicks();
        gBattleTimerActive = 1;
        gLastTimerRedraw = 0;
    } 
    else if (state == SCREEN_STATE_TITLE) 
    {
        memset(gXPressedFlags, 0, sizeof(gXPressedFlags));
        gBattleTimerActive = 0;
    }
    DrawImageAndText(); 
}

void UpdatePlayerPos(int clientID, char direction) 
{
    if (clientID < 0 || clientID >= MAX_CLIENTS) 
    return;
    
    int step = gPlayerMoveStep[clientID];
    int currentX = gPlayerPosX[clientID];
    int currentY = gPlayerPosY[clientID];
    int newX = currentX;
    int newY = currentY;

    int windowW, windowH;
    SDL_GetWindowSize(gMainWindow, &windowW, &windowH);
    const int SQ_SIZE = 50;   

    switch (direction) {
        case DIR_UP:    
        newY = currentY - step; 
        break;

        case DIR_DOWN:  
        newY = currentY + step; 
        break;

        case DIR_LEFT:  
        newX = currentX - step; 
        break;

        case DIR_RIGHT: 
        newX = currentX + step; 
        break;

        case DIR_UP_LEFT:    
        newY = currentY - step; newX = currentX - step; 
        break;

        case DIR_UP_RIGHT:   
        newY = currentY - step; newX = currentX + step; 
        break;

        case DIR_DOWN_LEFT:  newY = currentY + step; 
        newX = currentX - step; 
        break;
        case DIR_DOWN_RIGHT: 
        newY = currentY + step; newX = currentX + step; 
        break;

    }

    if (newX < 0) 
        newX = 0; 
    else if (newX > windowW - SQ_SIZE) 
        newX = windowW - SQ_SIZE; 
    if (newY < 0) 
        newY = 0; 
    else if (newY > windowH - SQ_SIZE) 
        newY = windowH - SQ_SIZE; 
    {
        int blockCount = 8;
        int blockSize = 150;
        int cols = 4;
        int rows = 2;
        int cell_w = windowW / cols;
        int cell_h = windowH / rows;
       SDL_Rect nextRect = { newX, newY, SQ_SIZE, SQ_SIZE };
        for (int i = 0; i < blockCount; i++) {
            SDL_Rect blockRect;
            blockRect.x = (i % cols) * cell_w + (cell_w / 2) - (blockSize / 2);
            blockRect.y = (i / cols) * cell_h + (cell_h / 2) - (blockSize / 2);
            blockRect.w = blockSize;
            blockRect.h = blockSize;

            if (SDL_HasIntersection(&nextRect, &blockRect)) 
            {
                return; 
            }
        }
    }

    //バグが起こるのでプレイヤー同士の当たり判定を削除してる

   /* SDL_Rect myNextRect = { newX, newY, SQ_SIZE, SQ_SIZE };
    for (int i = 0; i < gClientCount; i++) 
    {
        if (i == clientID || gPlayerHP[i] <= 0) 
        continue;

        SDL_Rect otherRect = { gPlayerPosX[i], gPlayerPosY[i], SQ_SIZE, SQ_SIZE };
        if (SDL_HasIntersection(&myNextRect, &otherRect)) 
        return; 
    }
    */

    gPlayerPosX[clientID] = newX;
    gPlayerPosY[clientID] = newY;
}
