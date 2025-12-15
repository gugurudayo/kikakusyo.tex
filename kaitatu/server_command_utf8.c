/*server_command_utf8.c*/

#include "server_common_utf8.h"
#include "server_func_utf8.h"
#include <arpa/inet.h>
#include <SDL2/SDL.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define PROJECTILE_SIZE 5
#define PLAYER_SIZE 50
#define SERVER_PROJECTILE_STEP 20
#define MAX_WEAPONS 4
#define MAX_STATS_PER_WEAPON 6
#define STAT_DAMAGE 2 

// ★ 追加: 衝突判定を広げるマージン (例: 5ピクセル) ★
#define HIT_MARGIN 5

// クライアント側のInitWindowsロジックに基づく定数を定義
#define DEFAULT_WINDOW_WIDTH 1300
#define DEFAULT_WINDOW_HEIGHT 1000
const int PADDING = 50; 


typedef struct {
    int x;
    int y;
    int firedByClientID; 
    int active;
    char direction; 
} ServerProjectile;


/* 状態 */
static char gClientHands[MAX_CLIENTS] = {0};          // 0: 未選択, 1..: 選択済み(weaponID+1)
static int gHandsCount = 0;
static int gXPressedClientFlags[MAX_CLIENTS] = {0};   // X押下フラグ（サーバー側保持）
static int gXPressedCount = 0;

static int gActiveProjectileCount = 0; // 現在アクティブな発射体の総数
static ServerProjectile gServerProjectiles[MAX_PROJECTILES]; // サーバーが管理する弾のリスト

static void SetIntData2DataBlock(void *data,int intData,int *dataSize);
static void SetCharData2DataBlock(void *data,char charData,int *dataSize);
static int gClientWeaponID[MAX_CLIENTS]; 
static int gServerInitialized = 0; // 初期化フラグ

int gServerWeaponStats[MAX_WEAPONS][MAX_STATS_PER_WEAPON] = {
    // 武器 0: 高速アタッカー (ダメージ10)
    { 500, 1000, 10, 100, 3, 20 },
    // 武器 1: ヘビーシューター (ダメージ30)
    { 1500, 1500, 30, 120, 1, 5 },
    // 武器 2: バランス型 (ダメージ20)
    { 1000, 1200, 20, 110, 2, 10 },
    // 武器 3: タフネス機 (ダメージ15)
    { 800, 800, 15, 150, 4, 15 }
};

int gPlayerPosX[MAX_CLIENTS] = {0}; 
int gPlayerPosY[MAX_CLIENTS] = {0};

extern CLIENT gClients[MAX_CLIENTS];
extern int GetClientNum(void);

extern int gPlayerPosX[MAX_CLIENTS];
extern int gPlayerPosY[MAX_CLIENTS];

static int CheckCollision(ServerProjectile *bullet, int playerID) {
    // プレイヤーの矩形
    int px = gPlayerPosX[playerID];
    int py = gPlayerPosY[playerID];
    int pw = PLAYER_SIZE;
    int ph = PLAYER_SIZE;

    // 弾の矩形
    int bx = bullet->x;
    int by = bullet->y;
    int bw = PROJECTILE_SIZE;
    int bh = PROJECTILE_SIZE;

    // ★ 修正: マージンを追加し、判定領域を広げる ★
    const int margin = HIT_MARGIN; 
    
    // 矩形同士の衝突判定 (AABB)
    return (bx < px + pw + margin &&    
            bx + bw > px - margin &&    
            by < py + ph + margin &&    
            by + bh > py - margin);     
}

static Uint32 ServerGameLoop(Uint32 interval, void *param) 
{
    int numClients = GetClientNum();
    
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (gServerProjectiles[i].active) {
            
            int shooterID = gServerProjectiles[i].firedByClientID;
            int weaponID = gClientWeaponID[shooterID];
            int attackDamage = (weaponID >= 0 && weaponID < MAX_WEAPONS) ? 
                               gServerWeaponStats[weaponID][STAT_DAMAGE] : 10;
            
            char dir = gServerProjectiles[i].direction;
            
            // 弾の移動を細分化して判定を行う
            int step = SERVER_PROJECTILE_STEP; // 現在の値は 20
            int hit = 0; 
            
            for (int k = 0; k < step; k++) {
                // 1. 1ピクセルずつ移動
                if (dir == DIR_UP) {
                    gServerProjectiles[i].y -= 1;
                } else if (dir == DIR_DOWN) {
                    gServerProjectiles[i].y += 1;
                } else if (dir == DIR_LEFT) {
                    gServerProjectiles[i].x -= 1;
                } else if (dir == DIR_RIGHT) {
                    gServerProjectiles[i].x += 1;
                }
                
                // 2. 衝突判定
                for (int j = 0; j < numClients; j++) {
                    // 発射元と自分自身は判定しない
                    if (j == shooterID) continue;

                    if (CheckCollision(&gServerProjectiles[i], j)) {
                        hit = 1;
                        
                        // 衝突発生時の処理 (既存のログ出力とコマンド送信)
                        printf(">> HIT! Player %d (%s) was hit by Player %d (%s). Damage: %d\n",
                               j, gClients[j].name,
                               shooterID, gClients[shooterID].name,
                               attackDamage); 
                               
                        gServerProjectiles[i].active = 0;
                        gActiveProjectileCount--;
                        
                        // ダメージ適用コマンドを全クライアントに送信
                        unsigned char data[MAX_DATA];
                        int dataSize = 0;
                        SetCharData2DataBlock(data, APPLY_DAMAGE_COMMAND, &dataSize);
                        SetIntData2DataBlock(data, j, &dataSize);          
                        SetIntData2DataBlock(data, attackDamage, &dataSize); 
                        SendData(ALL_CLIENTS, data, dataSize);

                        break; // プレイヤー (j) のループから抜ける
                    }
                } // プレイヤー (j) のループ終了
                
                if (hit) break; // 衝突したら、この弾のステップループ (k) も終了
            } // 弾のステップループ (k) 終了

            // 衝突しなかった場合は画面外チェック
            if (!gServerProjectiles[i].active) {
                // 衝突または画面外で非アクティブ化されていたら次の弾へ
                continue; 
            }
            
            // 画面外チェック (衝突しなかった場合のみ実行)
            if (gServerProjectiles[i].y < -100 || gServerProjectiles[i].y > 1500 ||
                gServerProjectiles[i].x < -100 || gServerProjectiles[i].x > 1500) {
                gServerProjectiles[i].active = 0;
                gActiveProjectileCount--;
                continue;
            }
        }
    }
    
    // 継続してタイマーを呼び出す
    return interval; 
}

typedef struct {
    unsigned char cmd;  // 送信コマンド
} TimerParam;

static Uint32 SendCommandAfterDelay(Uint32 interval, void *param)
{
    TimerParam *p = (TimerParam*)param;
    unsigned char data[MAX_DATA];
    int dataSize = 0;
    SetCharData2DataBlock(data, p->cmd, &dataSize);
    SendData(ALL_CLIENTS, data, dataSize);
    free(param);
    printf("[SERVER] Sent command 0x%02X after 3 seconds delay\n", p->cmd);
    return 0; 
}

int ExecuteCommand(char command,int pos)
{
    if (gServerInitialized == 0) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            gClientWeaponID[i] = -1;
        }
        gServerInitialized = 1;
    }

    unsigned char data[MAX_DATA];
    int dataSize;
    int endFlag = 1;
    assert(0 <= pos && pos < MAX_CLIENTS);
    switch(command){
        case END_COMMAND:
            dataSize = 0;
            SetCharData2DataBlock(data, END_COMMAND, &dataSize);
            SendData(ALL_CLIENTS, data, dataSize);
            endFlag = 0;
            break;
        case X_COMMAND:
        {
            int senderID;
            RecvIntData(pos, &senderID);
            if (senderID < 0 || senderID >= GetClientNum()) 
                break;
            if (gXPressedClientFlags[senderID] == 0) {
                gXPressedClientFlags[senderID] = 1;
                gXPressedCount++;
            }
            dataSize = 0;
            SetCharData2DataBlock(data, UPDATE_X_COMMAND, &dataSize);
            SetIntData2DataBlock(data, senderID, &dataSize);
            SendData(ALL_CLIENTS, data, dataSize);
            // もし全員押していたら => 3秒後に START_GAME_COMMAND を送信
            if (gXPressedCount == GetClientNum()) {
                // クライアントのInitWindowsに合わせてサーバー側座標を初期化
                int w = DEFAULT_WINDOW_WIDTH;
                int h = DEFAULT_WINDOW_HEIGHT;
                const int S_SIZE = PLAYER_SIZE; 
                
                for (int i = 0; i < GetClientNum(); i++) {
                    switch (i) {
                        case 0: // 1人目: 左上
                            gPlayerPosX[i] = PADDING;
                            gPlayerPosY[i] = PADDING;
                            break;
                        case 1: // 2人目: 右下
                            gPlayerPosX[i] = w - PADDING - S_SIZE;
                            gPlayerPosY[i] = h - PADDING - S_SIZE;
                            break;
                        case 2: // 3人目: 右上
                            gPlayerPosX[i] = w - PADDING - S_SIZE;
                            gPlayerPosY[i] = PADDING;
                            break;
                        case 3: // 4人目: 左下
                            gPlayerPosX[i] = PADDING;
                            gPlayerPosY[i] = h - PADDING - S_SIZE;
                            break;
                    }
                }

                TimerParam *tparam = malloc(sizeof(TimerParam));
                tparam->cmd = START_GAME_COMMAND;
                SDL_AddTimer(3000, SendCommandAfterDelay, tparam);
                // サーバー側フラグをリセット（次ラウンド用）
                gXPressedCount = 0;
                memset(gXPressedClientFlags, 0, sizeof(gXPressedClientFlags));
            }
            break;
        }
        case SELECT_WEAPON_COMMAND:
        {
            int senderID = pos;
            int selectedWeaponID;
            RecvIntData(senderID, &selectedWeaponID);
            if (senderID < 0 || senderID >= GetClientNum()) 
                break;

            if (gClientHands[senderID] == 0) {
                gClientHands[senderID] = (char)(selectedWeaponID + 1);
                gClientWeaponID[senderID] = selectedWeaponID; // ★ 武器IDを記録 ★
                gHandsCount++;
            }
            if (gHandsCount == GetClientNum()) {
                TimerParam *tparam = malloc(sizeof(TimerParam));
                tparam->cmd = NEXT_SCREEN_COMMAND;
                SDL_AddTimer(3000, SendCommandAfterDelay, tparam);
                gHandsCount = 0;
                memset(gClientHands, 0, sizeof(gClientHands));
            }
            break;
        }
        case MOVE_COMMAND:
        {
            int senderID = pos;
            char direction;
            RecvCharData(senderID, &direction); 
            int step = 10; 
            if (direction == DIR_UP) gPlayerPosY[senderID] -= step;
            else if (direction == DIR_DOWN) gPlayerPosY[senderID] += step;
            else if (direction == DIR_LEFT) gPlayerPosX[senderID] -= step;
            else if (direction == DIR_RIGHT) gPlayerPosX[senderID] += step;
            dataSize = 0;
            SetCharData2DataBlock(data, UPDATE_MOVE_COMMAND, &dataSize);
            SetIntData2DataBlock(data, senderID, &dataSize);
            SetCharData2DataBlock(data, direction, &dataSize);
            SendData(ALL_CLIENTS, data, dataSize);
            break;
        }
        case FIRE_COMMAND: 
        {
            int clientID, x, y;
            char direction; 
            RecvIntData(pos, &clientID); // 発射元ID
            RecvIntData(pos, &x);        // 初期X座標
            RecvIntData(pos, &y);        // 初期Y座標
            RecvCharData(pos, &direction);
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (!gServerProjectiles[i].active) {
                    gServerProjectiles[i].active = 1;
                    gServerProjectiles[i].firedByClientID = clientID;
                    gServerProjectiles[i].x = x;
                    gServerProjectiles[i].y = y;
                    gServerProjectiles[i].direction = direction;
                    gActiveProjectileCount++;
                    break;
                }
            }
            static int timerInitialized = 0;
            if (!timerInitialized) {
                SDL_AddTimer(1000 / 60, ServerGameLoop, NULL); 
                timerInitialized = 1;
            }
            printf("[SERVER] Client %d fired! Active projectiles: %d\n",clientID,gActiveProjectileCount);
            dataSize = 0;
            SetCharData2DataBlock(data, UPDATE_PROJECTILE_COMMAND, &dataSize); 
            SetIntData2DataBlock(data, clientID, &dataSize);                   
            SetIntData2DataBlock(data, x, &dataSize);                          
            SetIntData2DataBlock(data, y, &dataSize);                        
            SetCharData2DataBlock(data, direction, &dataSize);                      
            SendData(ALL_CLIENTS, data, dataSize);
            
            break;
        }
        default:
            fprintf(stderr,"Unknown command: 0x%02x\n", (unsigned char)command);
            break;
    }
    return endFlag;
}
static void SetIntData2DataBlock(void *data,int intData,int *dataSize)
{
    int tmp;
    assert(data!=NULL);
    assert(0 <= (*dataSize));
    tmp = htonl(intData);
    memcpy((char*)data + (*dataSize), &tmp, sizeof(int));
    (*dataSize) += sizeof(int);
}

static void SetCharData2DataBlock(void *data,char charData,int *dataSize)
{
    assert(data!=NULL);
    assert(0 <= (*dataSize));
    *(char *)((char*)data + (*dataSize)) = charData;
    (*dataSize) += sizeof(char);
}
