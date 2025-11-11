/*****************************************************************
ファイル名   : client_win_utf8.c
機能      : クライアントのユーザーインターフェース処理 (画像と全参加者表示に改変)
*****************************************************************/

#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h> // 画像ロード用
#include <SDL2/SDL_ttf.h> // フォント描画用
#include"common_utf8.h"
#include"client_func_utf8.h"

// 画像のファイル名 (仮定)
#define BACKGROUND_IMAGE "22823124.jpg"
#define FONT_PATH "/usr/share/fonts/opentype/ipafont-gothic/ipagp.ttf"

static SDL_Window *gMainWindow;
static SDL_Renderer *gMainRenderer;
static SDL_Texture *gBackgroundTexture = NULL; // 背景画像用テクスチャ
static TTF_Font *gFontLarge = NULL; // フォント
static TTF_Font *gFontNormal = NULL;

static char gAllClientNames[MAX_CLIENTS][MAX_NAME_SIZE]; // 全クライアント名を保持
static int gClientCount = 0; // クライアント数を保持
static int gMyClientID = -1; // 自身のクライアントIDを保持
static int gXPressedFlags[MAX_CLIENTS] = {0}; // 全クライアントの終了フラグ配列
static int gCurrentScreenState = SCREEN_STATE_LOBBY; // 【追加】画面状態を管理

static void DrawText_Internal(const char* text,int x,int y,Uint8 r,Uint8 g,Uint8 b, TTF_Font* font);
static void DrawImageAndText(void);


/*****************************************************************
関数名     : DrawText_Internal
機能      : 単一の文字列を画面に描画する
*****************************************************************/
static void DrawText_Internal(const char* text,int x,int y,Uint8 r,Uint8 g,Uint8 b, TTF_Font* font){
    SDL_Color color = {r ,g ,b ,255};
    SDL_Surface* surface = NULL;
    SDL_Texture* texture = NULL;
    SDL_Rect dest = {x ,y ,0, 0};

    if(!font){
        fprintf(stderr, "Font is not loaded. Cannot draw text.\n");
        return ;
    }

    surface = TTF_RenderUTF8_Blended(font, text, color);

    if(surface) {
        texture = SDL_CreateTextureFromSurface(gMainRenderer, surface);
        if(texture){
            dest.w = surface->w;
            dest.h = surface->h;
            // draw
            SDL_RenderCopy(gMainRenderer, texture, NULL, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

/*****************************************************************
関数名     : DrawImageAndText
機能      : 背景画像を描画し、その上に全参加者リストを描画する
*****************************************************************/
// client_win_utf8.c

// ... (既存のコード) ...

/*****************************************************************
関数名     : DrawImageAndText
機能      : 背景画像を描画し、状態に応じてテキストや図形を描画する
*****************************************************************/
static void DrawImageAndText(void) {
    int windowWidth = 0;
    int windowHeight = 0;
    int i;
    int y_pos;
    const int lineHeight = 30;
    char participantStr[MAX_NAME_SIZE + 20]; 
    
    SDL_RenderClear(gMainRenderer);
    SDL_GetWindowSize(gMainWindow, &windowWidth, &windowHeight);

    // 1. 背景画像を描画
    if (gBackgroundTexture) {
        SDL_RenderCopy(gMainRenderer, gBackgroundTexture, NULL, NULL);
    } else {
        SDL_SetRenderDrawColor(gMainRenderer, 0, 100, 0, 255);
        SDL_RenderFillRect(gMainRenderer, NULL);
    }

    // 2. 上部に "Tetra Conflict" を描画 (中央寄せ)
    int textWidth_Title = 0;
    if (gFontLarge) {
        TTF_SizeUTF8(gFontLarge, "Tetra Conflict", &textWidth_Title, NULL);
    }
    int x_title = (windowWidth / 2) - (textWidth_Title / 2);
    DrawText_Internal("Tetra Conflict", x_title, 20, 255, 255, 255, gFontLarge);
    
    // --- 画面状態による描画の分岐 ---
    if (gCurrentScreenState == SCREEN_STATE_LOBBY) {
        // --- 参加者リストの描画 (ロビー画面) ---
        
        // リストの最大幅を計算
        int maxWidth = 0;
        int tempWidth = 0;
        if (gFontNormal) {
            TTF_SizeUTF8(gFontNormal, "参加者:", &maxWidth, NULL);
            for (i = 0; i < gClientCount; i++) {
                char fullStr[MAX_NAME_SIZE + 20];
                if (gXPressedFlags[i]) { 
                    sprintf(fullStr, "%s X pressed", gAllClientNames[i]);
                } else {
                    sprintf(fullStr, "%s", gAllClientNames[i]);
                }
                TTF_SizeUTF8(gFontNormal, fullStr, &tempWidth, NULL);
                
                if (tempWidth > maxWidth) {
                    maxWidth = tempWidth;
                }
            }
        }
        
        // リスト全体を中央寄せするための開始X座標
        int startX = (windowWidth / 2) - (maxWidth / 2);

        // リスト全体の縦幅と開始Y座標を計算
        int totalLines = gClientCount + 1;
        int listHeight = totalLines * lineHeight; 
        int startY = (windowHeight / 2) - (listHeight / 2); 
        
        // 3.1. 「参加者:」ヘッダーを描画
        DrawText_Internal("参加者:", startX, startY, 0, 0, 0, gFontNormal);

        // 3.2. 全クライアント名をリスト描画
        for (i = 0; i < gClientCount; i++) {
            sprintf(participantStr, "%s", gAllClientNames[i]);
            
            int itemWidth = 0;
            if (gFontNormal) {
                 TTF_SizeUTF8(gFontNormal, participantStr, &itemWidth, NULL);
            }

            int x_centered = startX + (maxWidth - itemWidth) / 2;
            y_pos = startY + (lineHeight * (i + 1));
            
            DrawText_Internal(participantStr, x_centered, y_pos, 0, 0, 0, gFontNormal);

            if (gXPressedFlags[i]) { 
                DrawText_Internal(" X pressed", x_centered + itemWidth, y_pos, 255, 0, 0, gFontNormal);
            }
        }
    } 
    // 【追加】ゲーム画面に移行した場合の描画 (3秒後に実行される)
    else if (gCurrentScreenState == SCREEN_STATE_GAME_SCREEN) { 
        
        // 4つの長方形の基本的な高さとパディングを設定
        const int PADDING = 10; // 長方形間のスペース
        
        // 1. 幅を画面幅の約 40% に設定 (左右に寄せ、中央に空間を作る)
        const int TARGET_WIDTH_RATIO = windowWidth * 0.40; 
        
        // 高さの計算は維持 (縦に4つ並べるための高さ)
        int availableHeight = windowHeight - (PADDING * 5); 
        const int RECT_HEIGHT = availableHeight / 4; 
        
        const int RECT_WIDTH = TARGET_WIDTH_RATIO;
        
        // 中央の隙間（GAP）を計算
        int totalWidth = (RECT_WIDTH * 2) + PADDING; 
        int centerGap = windowWidth - totalWidth; // 中央の隙間の幅
        
        // 左側の長方形のX座標 (左端から PADDING)
        int leftX = PADDING; 
        // 右側の長方形のX座標 (左側の長方形 + 幅 + 中央の隙間)
        int rightX = windowWidth - PADDING - RECT_WIDTH; // 右端から PADDING 分内側
        
        // 描画用のSDL_Rect構造体
        SDL_Rect rect;
        rect.w = RECT_WIDTH;
        rect.h = RECT_HEIGHT;

        int currentY = PADDING; // 最初の長方形の開始Y座標

        // 1. 赤 (上左)
        SDL_SetRenderDrawColor(gMainRenderer, 255, 0, 0, 128); 
        rect.x = leftX; // 左側
        rect.y = currentY;
        SDL_RenderFillRect(gMainRenderer, &rect);

        // 2. 黄 (上右)
        SDL_SetRenderDrawColor(gMainRenderer, 255, 255, 0, 128); 
        rect.x = rightX; // 右側
        rect.y = currentY;
        SDL_RenderFillRect(gMainRenderer, &rect);
        currentY += RECT_HEIGHT + PADDING; // Y座標を次の行へ移動

        // 3. 青 (下左)
        SDL_SetRenderDrawColor(gMainRenderer, 0, 0, 255, 128); 
        rect.x = leftX; // 左側
        rect.y = currentY;
        SDL_RenderFillRect(gMainRenderer, &rect);

        // 4. 緑 (下右)
        SDL_SetRenderDrawColor(gMainRenderer, 0, 255, 0, 128); 
        rect.x = rightX; // 右側
        rect.y = currentY;
        SDL_RenderFillRect(gMainRenderer, &rect);

        // 次の長方形のための Y 座標更新 (もし必要なら)
        // currentY += RECT_HEIGHT + PADDING; 
        
        SDL_SetRenderDrawColor(gMainRenderer, 0, 0, 0, 255); 
    }
    
    SDL_RenderPresent(gMainRenderer);
}

/*****************************************************************
関数名 : InitWindows
機能  : メインウインドウの表示，設定を行い，画像とフォントを設定する
*****************************************************************/
int InitWindows(int clientID,int num,char name[][MAX_NAME_SIZE])
{
    SDL_Surface *imageSurface = NULL;
    int windowWidth = 800;
    int windowHeight = 600;
    char title[10];
    int i; 

    /* SDL/TTF/IMG の初期化 */
    if(SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() == -1 || !(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
        fprintf(stderr, "SDL/TTF/IMG initialization failed.\n");
        return -1;
    }

    /* クライアント情報のコピー */
    gMyClientID = clientID; // 自身のIDを保持
    gClientCount = num;
    for (i = 0; i < num; i++) { 
        strncpy(gAllClientNames[i], name[i], MAX_NAME_SIZE - 1);
        gAllClientNames[i][MAX_NAME_SIZE - 1] = '\0';
    }

    /* フォントのロード */
    gFontLarge = TTF_OpenFont(FONT_PATH, 40); // タイトル用
    gFontNormal = TTF_OpenFont(FONT_PATH, 24); // 参加者名用
    if (gFontLarge == NULL || gFontNormal == NULL) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
    }

    /* 画像のロード */
    imageSurface = IMG_Load(BACKGROUND_IMAGE);
    if (imageSurface) {
        windowWidth = imageSurface->w;
        windowHeight = imageSurface->h;
    } else {
        fprintf(stderr, "Failed to load image %s. Using default size.\n", BACKGROUND_IMAGE);
    }
    
    /* メインのウインドウを作成する */
    if((gMainWindow = SDL_CreateWindow("Game UI", SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, 0)) == NULL) {
        return -1;
    }

    gMainRenderer = SDL_CreateRenderer(gMainWindow, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    /* ウインドウのタイトルをセット */
    sprintf(title,"%d",clientID);
    SDL_SetWindowTitle(gMainWindow, title);

    /* ロードした画像からテクスチャを作成 */
    if (imageSurface) {
        gBackgroundTexture = SDL_CreateTextureFromSurface(gMainRenderer, imageSurface);
        SDL_FreeSurface(imageSurface);
    }

    // 初回描画
    DrawImageAndText(); 

    return 0;
}

/*****************************************************************
関数名 : DestroyWindow
機能  : SDLを終了する
*****************************************************************/
void DestroyWindow(void)
{
    if (gFontLarge) TTF_CloseFont(gFontLarge);
    if (gFontNormal) TTF_CloseFont(gFontNormal);
    TTF_Quit();

    if (gBackgroundTexture) {
        SDL_DestroyTexture(gBackgroundTexture);
    }
    if (gMainRenderer) {
        SDL_DestroyRenderer(gMainRenderer);
    }
    if (gMainWindow) {
        SDL_DestroyWindow(gMainWindow);
    }
    IMG_Quit();
    SDL_Quit();
}

/*****************************************************************
関数名 : WindowEvent
機能  : メインウインドウに対するイベント処理を行う
*****************************************************************/
void WindowEvent(int num)
{
    SDL_Event event;

    if(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT:
                // ウインドウの閉じるボタン (マウス操作) が押された場合
               gXPressedFlags[gMyClientID] = 1; // 【修正】自分のフラグ配列を立てる 
                DrawImageAndText(); 
                SendEndCommand();
                break;
            case SDL_KEYDOWN:
    if (event.key.keysym.sym == SDLK_x) { // Xキーが押された
        gXPressedFlags[gMyClientID] = 1;
        DrawImageAndText();

        // ---- サーバーへ送信 ----
        unsigned char data[MAX_DATA];
        int dataSize = 0;
        SetCharData2DataBlock(data, X_COMMAND, &dataSize);
        // 誰が押したかを伝える（自分のID）
        SetIntData2DataBlock(data, gMyClientID, &dataSize);
        SendData(data, dataSize);
    }
    else if (event.key.keysym.sym == SDLK_m) {
        SendEndCommand();
    }
    break;

        }
    }
}

/*****************************************************************
関数名 : SetXPressedFlag
機能  : 指定されたクライアントの「X pressed」状態を更新して再描画
*****************************************************************/
void SetXPressedFlag(int clientID)
{
    if (clientID < 0 || clientID >= MAX_CLIENTS) return;

    gXPressedFlags[clientID] = 1; // フラグON
    DrawImageAndText();           // 再描画
}

/*****************************************************************
関数名 : SetScreenState
機能  : 画面の状態を更新して再描画
*****************************************************************/
// 【重要】 'static' キーワードをつけないこと
void SetScreenState(int state) 
{
    gCurrentScreenState = state;
    DrawImageAndText();
}
