/* server_command_utf8.c */

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

#define DEFAULT_WINDOW_WIDTH 1300
#define DEFAULT_WINDOW_HEIGHT 1000
const int PADDING = 50; 

#define SCREEN_STATE_LOBBY_WAIT 0
#define SCREEN_STATE_GAME_SCREEN 1
#define SCREEN_STATE_RESULT 2
#define SCREEN_STATE_TITLE 3
#define UPPER_LEFT 3
#define LOWER_LEFT 2
#define UPPER_RIGHT 1
#define LOWER_RIGHT 4

typedef struct {
    int x;
    int y;
    int firedByClientID;
    int active;
    char direction; 
} ServerProjectile;

typedef struct {
    unsigned char cmd;
} TimerParam;

/* グローバル状態 */
static int gServerPlayerHP[MAX_CLIENTS]; 
static int gBattleEndSent = 0;           
static char gClientHands[MAX_CLIENTS] = {0};
static int gHandsCount = 0;
static int gXPressedClientFlags[MAX_CLIENTS] = {0};
static int gXPressedCount = 0;
static int gActiveProjectileCount = 0;
static ServerProjectile gServerProjectiles[MAX_PROJECTILES];
static int gClientWeaponID[MAX_CLIENTS]; 
static int gServerInitialized = 0;

static int gTrapActive = 0;   
static int gTrapType = 0;     // ★追加 0:回復(黄), 1:ダメージ(赤)
static SDL_Rect gTrapRect = {0, 0, 80, 80}; 

int gPlayerPosX[MAX_CLIENTS] = {0}; 
int gPlayerPosY[MAX_CLIENTS] = {0};

int gServerWeaponStats[MAX_WEAPONS][MAX_STATS_PER_WEAPON] = {
    { 500,  1000, 10, 100, 3, 20 }, 
    { 1500, 1500, 30, 120, 1, 5 },
    { 1000, 1200, 20, 110, 2, 10 }, 
    { 800,  800,  15, 150, 4, 15 }
};

extern CLIENT gClients[MAX_CLIENTS];
extern int GetClientNum(void);

/* ヘルパー関数群 */
static void SetIntData2DataBlock(void *data, int intData, int *dataSize) {
    int tmp = htonl(intData);
    memcpy((char*)data + (*dataSize), &tmp, sizeof(int));
    (*dataSize) += sizeof(int);
}

static void SetCharData2DataBlock(void *data, char charData, int *dataSize) {
    *(char *)((char*)data + (*dataSize)) = charData;
    (*dataSize) += sizeof(char);
}

/* ★ 解決1: HideTrap と SpawnTrap を呼び出し元より前に定義する ★ */

// トラップを消す処理
static Uint32 HideTrap(Uint32 interval, void *param) {
    gTrapActive = 0;
    unsigned char data[MAX_DATA];
    int ds = 0;
    SetCharData2DataBlock(data, UPDATE_TRAP_COMMAND, &ds);
    SetIntData2DataBlock(data, 0, &ds); // 0 = 非表示
    SendData(ALL_CLIENTS, data, ds);
    return 0;
}

// トラップを出現させる処理
void SpawnTrap(void) {
    if (gTrapActive) return;

    // 画面端から 80px 以上内側に出現させるように調整
    gTrapRect.x = rand() % (DEFAULT_WINDOW_WIDTH - 200) + 100;
    gTrapRect.y = rand() % (DEFAULT_WINDOW_HEIGHT - 200) + 100;
    gTrapActive = 1;
    gTrapType = rand() % 2; // ★追加 0 か 1 をランダムで決定

    unsigned char data[MAX_DATA];
    int ds = 0;
    SetCharData2DataBlock(data, UPDATE_TRAP_COMMAND, &ds);
    SetIntData2DataBlock(data, 1, &ds);           
    SetIntData2DataBlock(data, gTrapRect.x, &ds); 
    SetIntData2DataBlock(data, gTrapRect.y, &ds); 
    SetIntData2DataBlock(data, gTrapType, &ds); // ★追加 タイプを送信
    SendData(ALL_CLIENTS, data, ds);

    SDL_AddTimer(5000, HideTrap, NULL);
}

static Uint32 SendCommandAfterDelay(Uint32 interval, void *param) {
    TimerParam *p = (TimerParam*)param;
    unsigned char data[MAX_DATA];
    int ds = 0;

    SetCharData2DataBlock(data, p->cmd, &ds);
    SendData(ALL_CLIENTS, data, ds);

    if (p->cmd == END_COMMAND) {
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
    }

    free(param);
    return 0; 
}

/* 生存判定 */
void CheckWinnerAndTransition(void) {
    if (gBattleEndSent) return;

    int aliveCount = 0;
    int clientNum = GetClientNum();

    for (int i = 0; i < clientNum; i++) {
        if (gServerPlayerHP[i] > 0) aliveCount++;
    }

    if (aliveCount <= 1) {
        gBattleEndSent = 1;
        printf("[SERVER] Battle End. Remaining: %d\n", aliveCount);
        
        TimerParam *tp = malloc(sizeof(TimerParam));
        if (tp != NULL) {
            tp->cmd = NEXT_SCREEN_COMMAND; 
            SDL_AddTimer(3000, SendCommandAfterDelay, tp);
        }
    }
}

static int CheckCollision(ServerProjectile *bullet, int playerID) {
    int px = gPlayerPosX[playerID];
    int py = gPlayerPosY[playerID];

    return (bullet->x < px + PLAYER_SIZE && 
            bullet->x + PROJECTILE_SIZE > px &&
            bullet->y < py + PLAYER_SIZE && 
            bullet->y + PROJECTILE_SIZE > py);
}

static Uint32 ServerGameLoop(Uint32 interval, void *param) {
    int numClients = GetClientNum();

    // 1. トラップ抽選
    if (!gTrapActive && rand() % 500 == 0) {
        SpawnTrap();
    }

    // 2. トラップ判定
    if (gTrapActive) {
        for (int i = 0; i < numClients; i++) {
            if (gServerPlayerHP[i] <= 0) continue;
            SDL_Rect playerRect = {gPlayerPosX[i], gPlayerPosY[i], PLAYER_SIZE, PLAYER_SIZE};
            
            if (SDL_HasIntersection(&playerRect, &gTrapRect)) {
                int amount = (gTrapType == 0) ? -20 : 30; // ★ 0なら20回復、1なら30ダメージ

                gServerPlayerHP[i] -= amount; // ダメージ(正の値)なら減り、回復(負の値)なら増える
                if (gServerPlayerHP[i] > 150) gServerPlayerHP[i] = 150;
                if (gServerPlayerHP[i] < 0) gServerPlayerHP[i] = 0;

                unsigned char data[MAX_DATA];
                int ds = 0;
                SetCharData2DataBlock(data, APPLY_DAMAGE_COMMAND, &ds);
                SetIntData2DataBlock(data, i, &ds);     
                SetIntData2DataBlock(data, amount, &ds); // クライアントへ通知
                SendData(ALL_CLIENTS, data, ds);

                printf("[SERVER] Player %d hit Trap Type %d!\n", i, gTrapType);
                HideTrap(0, NULL);
                CheckWinnerAndTransition(); // ダメージで死ぬ可能性があるので判定
                break; 
            }
        }
    }

    // 3. 弾の処理
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!gServerProjectiles[i].active) continue;

        int sid = gServerProjectiles[i].firedByClientID;
        int dmg = gServerWeaponStats[gClientWeaponID[sid]][STAT_DAMAGE];
        char dir = gServerProjectiles[i].direction;

        for (int k = 0; k < SERVER_PROJECTILE_STEP; k++) {
            if (dir == DIR_UP) gServerProjectiles[i].y--;
            else if (dir == DIR_DOWN) gServerProjectiles[i].y++;
            else if (dir == DIR_LEFT) gServerProjectiles[i].x--;
            else if (dir == DIR_RIGHT) gServerProjectiles[i].x++;
            
            int hitFound = 0;
            for (int j = 0; j < numClients; j++) {
                if (j == sid || gServerPlayerHP[j] <= 0) continue;

                if (CheckCollision(&gServerProjectiles[i], j)) {
                    gServerPlayerHP[j] -= dmg;
                    if (gServerPlayerHP[j] < 0) gServerPlayerHP[j] = 0;
                    
                    unsigned char data[MAX_DATA];
                    int ds = 0;
                    SetCharData2DataBlock(data, APPLY_DAMAGE_COMMAND, &ds);
                    SetIntData2DataBlock(data, j, &ds);
                    SetIntData2DataBlock(data, dmg, &ds);
                    SendData(ALL_CLIENTS, data, ds);
                    
                    gServerProjectiles[i].active = 0;
                    CheckWinnerAndTransition(); 
                    hitFound = 1;
                    break;
                }
            }
            if (hitFound) break;
        }

        if (gServerProjectiles[i].active) {
            if (gServerProjectiles[i].y < -100 || gServerProjectiles[i].y > 1500 || 
                gServerProjectiles[i].x < -100 || gServerProjectiles[i].x > 1500) {
                gServerProjectiles[i].active = 0;
            }
        }
    }
    return interval; 
}

int ExecuteCommand(char command, int pos) {
    if (!gServerInitialized) {
        for (int i = 0; i < MAX_CLIENTS; i++) gClientWeaponID[i] = -1;
        gServerInitialized = 1;
    }

    unsigned char data[MAX_DATA];
    int ds = 0;
    int endFlag = 1;

    switch(command) {
        case END_COMMAND:
            ds = 0;
            SetCharData2DataBlock(data, END_COMMAND, &ds);
            SendData(ALL_CLIENTS, data, ds);
            endFlag = 0;
            break;

        case X_COMMAND: {
            int sid, state;
            RecvIntData(pos, &sid);
            RecvIntData(pos, &state);

            if (!gXPressedClientFlags[sid]) {
                gXPressedClientFlags[sid] = 1;
                gXPressedCount++;
            }

            ds = 0;
            SetCharData2DataBlock(data, UPDATE_X_COMMAND, &ds);
            SetIntData2DataBlock(data, sid, &ds);
            SendData(ALL_CLIENTS, data, ds);

            if (gXPressedCount == GetClientNum()) {
                TimerParam *tp = malloc(sizeof(TimerParam));
                if (tp != NULL) {
                    if (state == SCREEN_STATE_LOBBY_WAIT) {
                        int num = GetClientNum();
                        for (int i = 0; i < num; i++) {
                            if (i == 0) { gPlayerPosX[i] = PADDING; gPlayerPosY[i] = PADDING; }
                            else if (i == 1) { gPlayerPosX[i] = DEFAULT_WINDOW_WIDTH - PADDING - PLAYER_SIZE; gPlayerPosY[i] = DEFAULT_WINDOW_HEIGHT - PADDING - PLAYER_SIZE; }
                            else if (i == 2) { gPlayerPosX[i] = DEFAULT_WINDOW_WIDTH - PADDING - PLAYER_SIZE; gPlayerPosY[i] = PADDING; }
                            else { gPlayerPosX[i] = PADDING; gPlayerPosY[i] = DEFAULT_WINDOW_HEIGHT - PADDING - PLAYER_SIZE; }
                            gServerPlayerHP[i] = 150; 
                        }
                        gBattleEndSent = 0;
                        tp->cmd = START_GAME_COMMAND;
                    } else if (state == SCREEN_STATE_TITLE) {
                        tp->cmd = END_COMMAND;
                    }
                    SDL_AddTimer(3000, SendCommandAfterDelay, tp);
                }
                gXPressedCount = 0;
                memset(gXPressedClientFlags, 0, sizeof(gXPressedClientFlags));
            }
            break;
        }

        case SELECT_WEAPON_COMMAND: {
            int wid;
            RecvIntData(pos, &wid);
            gClientWeaponID[pos] = wid;
            gHandsCount++;
            if (gHandsCount == GetClientNum()) {
                TimerParam *tp = malloc(sizeof(TimerParam));
                if (tp != NULL) {
                    tp->cmd = NEXT_SCREEN_COMMAND;
                    SDL_AddTimer(3000, SendCommandAfterDelay, tp);
                }
                gHandsCount = 0;
            }
            break;
        }

        case MOVE_COMMAND: {
            char d;
            RecvCharData(pos, &d);
            if (d == DIR_UP) gPlayerPosY[pos] -= 10;
            else if (d == DIR_DOWN) gPlayerPosY[pos] += 10;
            else if (d == DIR_LEFT) gPlayerPosX[pos] -= 10;
            else if (d == DIR_RIGHT) gPlayerPosX[pos] += 10;

            ds = 0;
            SetCharData2DataBlock(data, UPDATE_MOVE_COMMAND, &ds);
            SetIntData2DataBlock(data, pos, &ds);
            SetCharData2DataBlock(data, d, &ds);
            SendData(ALL_CLIENTS, data, ds);
            break;
        }

        case FIRE_COMMAND: {
            int id, x, y;
            char d;
            RecvIntData(pos, &id);
            RecvIntData(pos, &x);
            RecvIntData(pos, &y);
            RecvCharData(pos, &d);

            int weaponID = gClientWeaposID[id];
            int maxBullet = GetMaxBulletByWeapos(weaponID);

            if (CountPlayerBullets(id) >= maxBullet) {
                return endFlag;
            }
            
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (!gServerProjectiles[i].active) {
                    gServerProjectiles[i].x = x;
                    gServerProjectiles[i].y = y;
                    gServerProjectiles[i].firedByClientID = id;
                    gServerProjectiles[i].active = 1;
                    gServerProjectiles[i].direction = d;
                    break;
                }
            }

            static int ti = 0;
            if (!ti) {
                SDL_AddTimer(16, ServerGameLoop, NULL);
                ti = 1;
            }

            ds = 0;
            SetCharData2DataBlock(data, UPDATE_PROJECTILE_COMMAND, &ds);
            SetIntData2DataBlock(data, id, &ds);
            SetIntData2DataBlock(data, x, &ds);
            SetIntData2DataBlock(data, y, &ds);
            SetCharData2DataBlock(data, d, &ds);
            SendData(ALL_CLIENTS, data, ds);
            break;
        }
        default: break;
    }
    return endFlag;
}

int GetMaxBulletByWeapon(int weaponID)
{
    switch(weaponID){
    case 0: return UPPER_LEFT;
    case 1: return UPPER_RIGHT;
    case 2: return LOWER_LEFT;
    case 3: return LOWER_RIGHT;
    default: return 0;
    }
}

int CountPlayerBullets(int playerID)
{
    int count = 0;
for (int i = 0; i < MAXPROJECTILES; i++){
    if (gServerProjectiles[i].active &&
        gServerProjectiles[i].firedByClientID == playerID) {
        count++;
    }
}
return count;
}
