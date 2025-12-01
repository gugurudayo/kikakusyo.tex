/*****************************************************************
ファイル名	: client_win_utf8.c
機能		: クライアントのユーザーインターフェース処理（修正版）
*****************************************************************/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "common_utf8.h"
#include "client_func_utf8.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h> 

#define BACKGROUND_IMAGE "22823124.jpg" 
#define RESULT_IMAGE "Chatgpt.png"     
#define RESULT_BACK_IMAGE "2535410.jpg" 
#define FONT_PATH "/usr/share/fonts/opentype/ipafont-gothic/ipagp.ttf"
#define DEFAULT_WINDOW_WIDTH 1300
#define DEFAULT_WINDOW_HEIGHT 1000
/* SCREEN_STATE_* は common 側と揃える */

#define SCREEN_STATE_LOBBY_WAIT 0
#define SCREEN_STATE_GAME_SCREEN 1
#define SCREEN_STATE_RESULT 2
#define SCREEN_STATE_TITLE 3


/* グローバルUI/状態変数 */
static SDL_Window *gMainWindow = NULL;
static SDL_Renderer *gMainRenderer = NULL;
static SDL_Texture *gBackgroundTexture = NULL;
static SDL_Texture *gResultTexture = NULL;
static SDL_Texture *gResultBackTexture = NULL;
static TTF_Font *gFontLarge = NULL;
static TTF_Font *gFontNormal = NULL;
static TTF_Font *gFontRank = NULL; 
static TTF_Font *gFontName = NULL;
static char gAllClientNames[MAX_CLIENTS][MAX_NAME_SIZE];
static int gClientCount = 0;
int gMyClientID = -1;
static int gXPressedFlags[MAX_CLIENTS] = {0};
static int gCurrentScreenState = SCREEN_STATE_LOBBY_WAIT;
static int gWeaponSent = 0;

// SetScreenState は下部で定義されているため、ここで宣言が必要
void SetScreenState(int state);

// 画面状態をタイトルに戻すタイマーコールバック
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
static void DrawImageAndText(void);
// DrawText_Internal は、ロビー待機画面でのみ利用するため、機能を維持
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

/* DrawImageAndText: 状態に応じた描画 */
static void DrawImageAndText(void){
    int w=800,h=600;
    SDL_GetWindowSize(gMainWindow,&w,&h);
    SDL_RenderClear(gMainRenderer);

    if (gCurrentScreenState == SCREEN_STATE_LOBBY_WAIT){
        // 背景描画
        if (gBackgroundTexture) SDL_RenderCopy(gMainRenderer,gBackgroundTexture,NULL,NULL);
        else { SDL_SetRenderDrawColor(gMainRenderer,0,100,0,255); SDL_RenderFillRect(gMainRenderer,NULL); }

        // タイトル
        int textW=0;
        if (gFontLarge) TTF_SizeUTF8(gFontLarge,"Tetra Conflict",&textW,NULL);
        DrawText_Internal("Tetra Conflict",(w-textW)/2,20,255,255,255,gFontLarge);

        // 参加者見出し
        DrawText_Internal("参加者:", (w/2)-100, h/2, 255,255,255, gFontName);

        int baseY = h/2 + 40;
        int lineH = 40; // フォントサイズ大きくしたので行間も広め

        for (int i = 0; i < 4; i++){ // 最大4人
            int nameX = (w/2) - 100;
            int nameY = baseY + i * lineH;

            const char* nameToDraw = (i < gClientCount) ? gAllClientNames[i] : "";

            DrawText_Internal(nameToDraw, nameX, nameY, 255,255,255, gFontName);

            if (i < gClientCount && gXPressedFlags[i] == 1){
                DrawText_Internal(" X Pressed", nameX + 200, nameY, 255,200,0, gFontName);
            }
        }
    }
    else if (gCurrentScreenState == SCREEN_STATE_GAME_SCREEN){
        // 背景描画
        if (gBackgroundTexture) SDL_RenderCopy(gMainRenderer,gBackgroundTexture,NULL,NULL);
        else { SDL_SetRenderDrawColor(gMainRenderer,0,100,0,255); SDL_RenderFillRect(gMainRenderer,NULL); }

        // 武器4つ描画
        const int P=20, Y_OFF=50;
        int rectW=(w-P*4)/2;  
        int rectH=(h-P*7)/2;  
        int leftX=(w-(rectW*2+P))/2;
        int rightX=leftX+rectW+P;
        int topY=(h-(rectH*2+P))/2 + Y_OFF;
        int bottomY=topY+rectH+P;
        SDL_Rect r;

        r.x=leftX; r.y=topY; r.w=rectW; r.h=rectH;
        SDL_SetRenderDrawColor(gMainRenderer,255,0,0,255); SDL_RenderFillRect(gMainRenderer,&r);
        r.x=rightX;
        SDL_SetRenderDrawColor(gMainRenderer,0,0,255,255); SDL_RenderFillRect(gMainRenderer,&r);
        r.x=leftX; r.y=bottomY;
        SDL_SetRenderDrawColor(gMainRenderer,255,255,0,255); SDL_RenderFillRect(gMainRenderer,&r);
        r.x=rightX;
        SDL_SetRenderDrawColor(gMainRenderer,0,255,0,255); SDL_RenderFillRect(gMainRenderer,&r);
    }
    else if (gCurrentScreenState == SCREEN_STATE_RESULT){
        if (gResultTexture) SDL_RenderCopy(gMainRenderer,gResultTexture,NULL,NULL);
        else { SDL_SetRenderDrawColor(gMainRenderer,153,204,0,255); SDL_RenderFillRect(gMainRenderer,NULL); }
    }
    else if (gCurrentScreenState == SCREEN_STATE_TITLE){
        // 背景
        if (gResultBackTexture) SDL_RenderCopy(gMainRenderer,gResultBackTexture,NULL,NULL);
        else { SDL_SetRenderDrawColor(gMainRenderer,0,0,255,255); SDL_RenderFillRect(gMainRenderer,NULL); }

        // タイトル描画
        int titleBottomY = 0;
        if (gFontLarge){
            int textW=0, textH=0;
            TTF_SizeUTF8(gFontLarge,"結果発表!!",&textW,&textH);
            DrawText_Internal("結果発表!!",(w-textW)/2,40,255,255,255,gFontLarge);
            titleBottomY = 40 + textH;
        }

        // 補助テキスト
        int helpTextY = h - 40;
        if (gFontNormal){
            const char* msg = "参加者全員がXボタンを押すと、ゲームが終了します。";
            int textW=0,textH=0;
            TTF_SizeUTF8(gFontNormal,msg,&textW,&textH);
            helpTextY = h - textH - 40;
            DrawText_Internal(msg,(w-textW)/2,helpTextY,0,0,200,gFontNormal);
        }

        // 結果ボックス描画
        int topMargin = 20;
        int bottomMargin = 20;
        int rectCount = 4;
        int spacing = 10;
        int availableHeight = (helpTextY - bottomMargin) - (titleBottomY + topMargin);
        int rectH = (availableHeight - spacing*(rectCount-1)) / rectCount;
        int rectW = 350;
        int startX = (w - rectW)/2;
        SDL_SetRenderDrawColor(gMainRenderer,255,0,0,255);
        SDL_Rect rect;
        for (int i=0; i<rectCount; i++){
            rect.x = startX;
            rect.y = titleBottomY + topMargin + i*(rectH + spacing);
            rect.w = rectW;
            rect.h = rectH;
            SDL_RenderDrawRect(gMainRenderer,&rect);
        }

        // ラベル & 名前
        const char* rankLabels[4] = {"1st:","2nd:","3rd:","4th:"};
        for(int i=0;i<rectCount;i++){
            int rectY = titleBottomY + topMargin + i*(rectH + spacing);
            int textW=0,textH=0;
            TTF_SizeUTF8(gFontRank, rankLabels[i], &textW, &textH);
            int labelX = startX + 15;
            int labelY = rectY + (rectH - textH)/2;
            DrawText_Internal(rankLabels[i], labelX, labelY, 0,0,255,gFontRank);

            const char* nameToDraw = (i < gClientCount) ? gAllClientNames[i] : "";
            int nameW=0,nameH=0;
            TTF_SizeUTF8(gFontName,nameToDraw,&nameW,&nameH);
            int nameX = labelX + textW + 20;
            int nameY = rectY + (rectH - nameH)/2;
            DrawText_Internal(nameToDraw,nameX,nameY,0,0,255,gFontName);
        }
    }

    SDL_RenderPresent(gMainRenderer);
}


/* GetRenderer (client_command から参照するため) */
SDL_Renderer* GetRenderer(void){ return gMainRenderer; }

/* InitWindows: 初期化 */
int InitWindows(int clientID,int num,char name[][MAX_NAME_SIZE]){
	int windowW = DEFAULT_WINDOW_WIDTH;
    int windowH = DEFAULT_WINDOW_HEIGHT;
	gMyClientID = clientID;
	gClientCount = num;
	for(int i=0;i<num;i++){ strncpy(gAllClientNames[i], name[i], MAX_NAME_SIZE-1); gAllClientNames[i][MAX_NAME_SIZE-1]='\0'; }

	// TTF_Init() を維持
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 || !(IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG) & IMG_INIT_JPG)) {
		fprintf(stderr,"SDL init failed: %s\n", SDL_GetError()); return -1;
	}
	
	// TTF_Init() の呼び出しを維持
	if (TTF_Init() == -1) {
		fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
	}


	// gFontLarge, gFontNormal のロード処理を維持
	gFontLarge = TTF_OpenFont(FONT_PATH, 40);
	gFontNormal = TTF_OpenFont(FONT_PATH, 24);
	gFontRank  = TTF_OpenFont(FONT_PATH, 36);
	gFontName  = TTF_OpenFont(FONT_PATH, 36);
	if (!gFontLarge || !gFontNormal) {
		fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
	}

	SDL_Surface *bg = IMG_Load(BACKGROUND_IMAGE);
	SDL_Surface *res = IMG_Load(RESULT_IMAGE);
	SDL_Surface *resBack = IMG_Load(RESULT_BACK_IMAGE); // ★ 新しい画像をロード ★

	gMainWindow = SDL_CreateWindow("Game UI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowW, windowH, 0);
	gMainRenderer = SDL_CreateRenderer(gMainWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (bg) { gBackgroundTexture = SDL_CreateTextureFromSurface(gMainRenderer,bg); SDL_FreeSurface(bg); }
	if (res) { gResultTexture = SDL_CreateTextureFromSurface(gMainRenderer,res); SDL_FreeSurface(res); }
	if (resBack) { gResultBackTexture = SDL_CreateTextureFromSurface(gMainRenderer,resBack); SDL_FreeSurface(resBack); } // ★ テクスチャを作成 ★

	/* 初期画面はロビー待機画面 (SCREEN_STATE_LOBBY_WAIT) に変更 */
	gCurrentScreenState = SCREEN_STATE_LOBBY_WAIT;
	DrawImageAndText();
	return 0;
}

/* DestroyWindow: 終了処理 */
void DestroyWindow(void){
	// TTF_CloseFont を維持
	if (gFontLarge) TTF_CloseFont(gFontLarge);
	if (gFontNormal) TTF_CloseFont(gFontNormal);
	if (gFontRank) TTF_CloseFont(gFontRank); // ★ フォント破棄を追加 ★
	if (gResultTexture) SDL_DestroyTexture(gResultTexture);
	if (gBackgroundTexture) SDL_DestroyTexture(gBackgroundTexture);
	if (gResultBackTexture) SDL_DestroyTexture(gResultBackTexture); // ★ 破棄処理を追加 ★
	if (gMainRenderer) SDL_DestroyRenderer(gMainRenderer);
	if (gMainWindow) SDL_DestroyWindow(gMainWindow);
	// TTF_Quit を維持
	IMG_Quit(); TTF_Quit(); SDL_Quit();
}

/* WindowEvent: 入力処理 */
void WindowEvent(int num){
	SDL_Event event;
	if (SDL_PollEvent(&event)){
		switch(event.type){
			case SDL_QUIT:
				// ウィンドウの閉じるボタンで終了コマンドを送信
				gXPressedFlags[gMyClientID] = 1;
				DrawImageAndText();
				SendEndCommand();
				break;

			case SDL_KEYDOWN:
				// ロビー待機画面でのみ X キー入力を受け付ける
				if (event.key.keysym.sym == SDLK_x && gCurrentScreenState == SCREEN_STATE_LOBBY_WAIT){
					// Xキーを押した際の処理
					if (gXPressedFlags[gMyClientID] == 0) {
						// 自分の準備フラグを立ててサーバに通知する（未押下の場合のみ）
						gXPressedFlags[gMyClientID] = 1;
						DrawImageAndText();

						unsigned char data[MAX_DATA];
						int dataSize = 0;
						SetCharData2DataBlock(data, X_COMMAND, &dataSize);
						SetIntData2DataBlock(data, gMyClientID, &dataSize);
						SendData(data, dataSize);
					}
					// 画面遷移自体はサーバが全員確認してから START_GAME_COMMAND を送る
				}
				else if (event.key.keysym.sym == SDLK_m){
					// Mキーで終了コマンド
					SendEndCommand();
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				if (gCurrentScreenState == SCREEN_STATE_GAME_SCREEN){
					// 武器選択画面でのマウスクリック処理
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
					
					// クリックされた位置から武器IDを判定
					if (x >= leftX && x < leftX + rectW){
						if (y >= topY && y < topY + rectH) selectedID = 0; // 左上
						else if (y >= bottomY && y < bottomY + rectH) selectedID = 2; // 左下
					} else if (x >= rightX && x < rightX + rectW){
						if (y >= topY && y < topY + rectH) selectedID = 1; // 右上
						else if (y >= bottomY && y < bottomY + rectH) selectedID = 3; // 右下
					}

					if (selectedID != -1 && gWeaponSent == 0){
						// 武器が選択され、まだ送信していない場合
						unsigned char data[MAX_DATA];
						int dataSize = 0;
						SetCharData2DataBlock(data, SELECT_WEAPON_COMMAND, &dataSize);
						SetIntData2DataBlock(data, selectedID, &dataSize);
						SendData(data, dataSize);
						gWeaponSent = 1; // 選択済みフラグを立てる
					}
				}
				break;
            case SDL_USEREVENT:
                // タイマーなど別スレッドからの画面切替要求をメインスレッドで実行
                {
                    int reqState = (int)(intptr_t)event.user.data1;
                    printf("[DEBUG] Received USEREVENT -> SetScreenState: %d\n", reqState);
                    fflush(stdout);
                    SetScreenState(reqState);
                }
                break;
		}
	}
	
	// DrawImageAndText() はサーバーからのコマンド受信時/状態変更時のみとする
}

/* SetXPressedFlag: サーバーから UPDATE_X_COMMAND を受けたら表示更新 */
void SetXPressedFlag(int clientID){
	if (clientID < 0 || clientID >= MAX_CLIENTS) return;
	gXPressedFlags[clientID] = 1;
	DrawImageAndText();
}

/* SetScreenState: サーバーからのコマンドに応じて画面を切り替え */
void SetScreenState(int state){
     printf("[DEBUG] SetScreenState: state=%d\n", state);
    gCurrentScreenState = state;
    
    if (state == SCREEN_STATE_GAME_SCREEN) {
        // ゲーム画面への遷移時、武器選択フラグをリセット
        gWeaponSent = 0;
    } else if (state == SCREEN_STATE_LOBBY_WAIT) {
        // ロビー待機画面への遷移時、全員のX押下状態をリセット
        memset(gXPressedFlags, 0, sizeof(gXPressedFlags));
    }
    else if (state == SCREEN_STATE_RESULT) {
        // 結果画面表示後3秒後に自動遷移は、ここでは行わない
        // 代わりに下記で処理
         printf("[DEBUG] RESULT state - timer will be set\n");
    }
    
    DrawImageAndText();
    
    // 結果画面表示時に3秒タイマーを設定
    if (state == SCREEN_STATE_RESULT) {
        SDL_AddTimer(3000, BackToTitleScreen, NULL);
    }
}
