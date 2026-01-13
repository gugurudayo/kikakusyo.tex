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
    int distance;
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

static int gTrapActive = 0;   /* legacy (unused) */

#define TRAP_SIZE 80
#define TRAP_LIFETIME_MS 10000           // 罠の表示時間
#define TRAP_SPAWN_INTERVAL_MS 5000     // ぽこぽこ出す間隔（ms）
#define MAX_TRAPS 3

typedef struct {
    int active;
    int type;
    SDL_Rect rect;
    Uint32 expireAt;
} ServerTrap;

static ServerTrap gTraps[MAX_TRAPS];
static Uint32 gNextTrapSpawnAt = 0; 

// ★追加: 毒トラップ(紫)の継続ダメージ管理
#define POISON_TICKS 30             // 30秒継続
#define POISON_DMG_PER_TICK 1      // 1秒ごとに1ダメージ
#define POISON_INTERVAL_MS 1000     // 何msごとにダメージを入れるか
static int gPoisonRemain[MAX_CLIENTS] = {0};
static Uint32 gPoisonNextTick[MAX_CLIENTS] = {0};

// ★追加: 温泉(回復)トラップの継続回復(HoT)
#define HOTSPRING_TICKS 30         // 30秒継続
#define HOTSPRING_HEAL_PER_TICK 1 // 1秒ごとに1回復
#define HOTSPRING_INTERVAL_MS 1000 // 何msごとに回復するか
static int gHotspringRemain[MAX_CLIENTS] = {0};
static Uint32 gHotspringNextTick[MAX_CLIENTS] = {0};


int gPlayerPosX[MAX_CLIENTS] = {0}; 
int gPlayerPosY[MAX_CLIENTS] = {0};
static int GetMaxBulletByWeapon(int weaponID);
static int CountPlayerBullets(int playerID);

int gServerWeaponStats[MAX_WEAPONS][MAX_STATS_PER_WEAPON] = {
    { 500,  1000, 10, 3 },
    { 1500, 1500, 30, 1 },
    { 1000, 1200, 20, 2 },
    { 800,  800,  15, 4 }
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

/* ★ Trap (multi) helpers ★ */

static void SendTrapUpdate(int active, int trapId, int x, int y, int type) {
    unsigned char data[MAX_DATA];
    int ds = 0;
    SetCharData2DataBlock(data, UPDATE_TRAP_COMMAND, &ds);
    SetIntData2DataBlock(data, active, &ds);
    SetIntData2DataBlock(data, trapId, &ds);
    if (active) {
        SetIntData2DataBlock(data, x, &ds);
        SetIntData2DataBlock(data, y, &ds);
        SetIntData2DataBlock(data, type, &ds);
    }
    SendData(ALL_CLIENTS, data, ds);
}

static void DespawnTrap(int trapId) {
    if (trapId < 0 || trapId >= MAX_TRAPS) return;
    if (!gTraps[trapId].active) return;
    gTraps[trapId].active = 0;
    SendTrapUpdate(0, trapId, 0, 0, 0);
}

static int SpawnTrap(void) {
    // 空きスロットを探す
    int slot = -1;
    for (int i = 0; i < MAX_TRAPS; i++) {
        if (!gTraps[i].active) { slot = i; break; }
    }
    if (slot < 0) return -1;

    int overlap;
    int screenW = 1300; // DEFAULT_WINDOW_WIDTH
    int screenH = 1000; // DEFAULT_WINDOW_HEIGHT
    int blockCols = 4;
    int blockRows = 2;
    int cellW = screenW / blockCols;
    int cellH = screenH / blockRows;
    int blockSize = 150; // 壁のサイズ

    SDL_Rect tr = {0, 0, TRAP_SIZE, TRAP_SIZE};

    // 壁＆他トラップとの重なり回避
    int tries = 0;
    do {
        overlap = 0;
        tr.x = rand() % (screenW - 200) + 100;
        tr.y = rand() % (screenH - 200) + 100;

        // 壁との重なり
        for (int i = 0; i < 8; i++) {
            int bx = (i % blockCols) * cellW + (cellW / 2) - (blockSize / 2);
            int by = (i / blockCols) * cellH + (cellH / 2) - (blockSize / 2);
            SDL_Rect blockRect = {bx, by, blockSize, blockSize};
            if (SDL_HasIntersection(&tr, &blockRect)) { overlap = 1; break; }
        }

        // 他のトラップと重なりすぎない（完全にNGにすると詰まるので、軽く回避）
        if (!overlap) {
            for (int i = 0; i < MAX_TRAPS; i++) {
                if (!gTraps[i].active) continue;
                if (SDL_HasIntersection(&tr, &gTraps[i].rect)) { overlap = 1; break; }
            }
        }

        tries++;
        if (tries > 50) break; // これ以上は妥協
    } while (overlap);

    gTraps[slot].active = 1;
    gTraps[slot].type = rand() % 4;  // 0..3
    gTraps[slot].rect = tr;
    gTraps[slot].expireAt = SDL_GetTicks() + TRAP_LIFETIME_MS;

    SendTrapUpdate(1, slot, tr.x, tr.y, gTraps[slot].type);
    return slot;
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

static int CheckWallCollision(int x, int y)
{
    int screenW = 1300;
    int screenH = 1000;
    int cols = 4;
    int rows = 2;
    int cell_w = screenW / cols;
    int cell_h = screenH / rows;
    int blockSize = 150;

    SDL_Rect bullet = { x, y, PROJECTILE_SIZE, PROJECTILE_SIZE };

    for (int i = 0; i < 8; i++) {
        int r = i / cols;
        int c = i % cols;

        int bx = c * cell_w + (cell_w / 2) - (blockSize / 2);
        int by = r * cell_h + (cell_h / 2) - (blockSize / 2);

        SDL_Rect block = { bx, by, blockSize, blockSize };

        if (SDL_HasIntersection(&bullet, &block)) {
            return 1; // 壁に当たった
        }
    }
    return 0;
}


static Uint32 ServerGameLoop(Uint32 interval, void *param) {
    int numClients = GetClientNum();

    // ★追加: 毒トラップの継続ダメージ(DoT)
    Uint32 now = SDL_GetTicks();
    for (int i = 0; i < numClients; i++) {
        if (gPoisonRemain[i] <= 0) continue;
        if (gServerPlayerHP[i] <= 0) { gPoisonRemain[i] = 0; continue; }
        if (now >= gPoisonNextTick[i]) {
            int dmg = POISON_DMG_PER_TICK;
            gServerPlayerHP[i] -= dmg;
            if (gServerPlayerHP[i] < 0) gServerPlayerHP[i] = 0;

            unsigned char data[MAX_DATA];
            int ds = 0;
            SetCharData2DataBlock(data, APPLY_DAMAGE_COMMAND, &ds);
            SetIntData2DataBlock(data, i, &ds);
            SetIntData2DataBlock(data, dmg, &ds);
            SendData(ALL_CLIENTS, data, ds);

            gPoisonRemain[i]--;
            gPoisonNextTick[i] = now + POISON_INTERVAL_MS;

            CheckWinnerAndTransition();
        }
    }



    // ★追加: 温泉(回復)トラップの継続回復(HoT)
    for (int i = 0; i < numClients; i++) {
        if (gHotspringRemain[i] <= 0) continue;
        if (gServerPlayerHP[i] <= 0) { gHotspringRemain[i] = 0; continue; }
        if (now >= gHotspringNextTick[i]) {
            int heal = HOTSPRING_HEAL_PER_TICK;
            // サーバ側HP更新 (回復なので + )
            gServerPlayerHP[i] += heal;
            if (gServerPlayerHP[i] > 150) gServerPlayerHP[i] = 150;

            unsigned char data[MAX_DATA];
            int ds = 0;
            // 既存のAPPLY_DAMAGE_COMMANDを流用: amountが負なら回復として扱える設計
            SetCharData2DataBlock(data, APPLY_DAMAGE_COMMAND, &ds);
            SetIntData2DataBlock(data, i, &ds);
            SetIntData2DataBlock(data, -heal, &ds);
            SendData(ALL_CLIENTS, data, ds);

            gHotspringRemain[i]--;
            gHotspringNextTick[i] = now + HOTSPRING_INTERVAL_MS;
        }
    }
    // 1. トラップ出現（ぽこぽこ）
    Uint32 nowTicks = SDL_GetTicks();
    if (nowTicks >= gNextTrapSpawnAt) {
        SpawnTrap();
        gNextTrapSpawnAt = nowTicks + TRAP_SPAWN_INTERVAL_MS;
    }

    // 1b. トラップの寿命処理
    for (int ti = 0; ti < MAX_TRAPS; ti++) {
        if (gTraps[ti].active && nowTicks >= gTraps[ti].expireAt) {
            DespawnTrap(ti);
        }
    }

    // 2. トラップ判定（複数対応）
for (int ti = 0; ti < MAX_TRAPS; ti++) {
    if (!gTraps[ti].active) continue;

    for (int i = 0; i < numClients; i++) {
        if (gServerPlayerHP[i] <= 0) continue;

        SDL_Rect playerRect = {gPlayerPosX[i], gPlayerPosY[i], PLAYER_SIZE, PLAYER_SIZE};
        if (SDL_HasIntersection(&playerRect, &gTraps[ti].rect)) {
            int amount = 0;

            if (gTraps[ti].type == 0) {
                amount = -20; // 回復(+20)
            } else if (gTraps[ti].type == 1) {
                amount = 30;  // ダメージ(30)
            } else if (gTraps[ti].type == 2) {
                // 毒: 1秒ごとに1ダメージを30秒
                gPoisonRemain[i] = POISON_TICKS;
                gPoisonNextTick[i] = SDL_GetTicks() + POISON_INTERVAL_MS;
            } else if (gTraps[ti].type == 3) {
                // 温泉: 1秒ごとに1回復を30秒
                gHotspringRemain[i] = HOTSPRING_TICKS;
                gHotspringNextTick[i] = SDL_GetTicks() + HOTSPRING_INTERVAL_MS;
            }

            if (amount != 0) {
                gServerPlayerHP[i] -= amount;
                if (gServerPlayerHP[i] > 150) gServerPlayerHP[i] = 150;
                if (gServerPlayerHP[i] < 0) gServerPlayerHP[i] = 0;

                unsigned char data[MAX_DATA];
                int ds = 0;
                SetCharData2DataBlock(data, APPLY_DAMAGE_COMMAND, &ds);
                SetIntData2DataBlock(data, i, &ds);
                SetIntData2DataBlock(data, amount, &ds);
                SendData(ALL_CLIENTS, data, ds);
            }

            // 罠は踏まれたら消える
            DespawnTrap(ti);
            CheckWinnerAndTransition();
            break;
        }
    }
}

// 3. 弾の処理 (修正ポイント)
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!gServerProjectiles[i].active) continue;

        int sid = gServerProjectiles[i].firedByClientID;
        int weaponID = gClientWeaponID[sid];
        
        // ★ ステータスの取得
        int dmg = gServerWeaponStats[weaponID][STAT_DAMAGE];
        int maxRange = gServerWeaponStats[weaponID][STAT_RANGE]; 
        
        char dir = gServerProjectiles[i].direction;

        for (int k = 0; k < SERVER_PROJECTILE_STEP; k++) {

    int nextX = gServerProjectiles[i].x;
    int nextY = gServerProjectiles[i].y;

    // ① 次の座標を計算（まだ反映しない）
    if (dir == DIR_UP) nextY--;
    else if (dir == DIR_DOWN) nextY++;
    else if (dir == DIR_LEFT) nextX--;
    else if (dir == DIR_RIGHT) nextX++;
    else if (dir == DIR_UP_LEFT)    { nextY--; nextX--; }
    else if (dir == DIR_UP_RIGHT)   { nextY--; nextX++; }
    else if (dir == DIR_DOWN_LEFT)  { nextY++; nextX--; }
    else if (dir == DIR_DOWN_RIGHT) { nextY++; nextX++; }

    // ② 先に壁判定
    if (CheckWallCollision(nextX, nextY) ||
    CheckWallCollision(nextX + PROJECTILE_SIZE - 1, nextY) ||
    CheckWallCollision(nextX, nextY + PROJECTILE_SIZE - 1) ||
    CheckWallCollision(nextX + PROJECTILE_SIZE - 1, nextY + PROJECTILE_SIZE - 1)) {
    gServerProjectiles[i].active = 0;
    break;
}

    // ③ 問題なければ移動を確定
    gServerProjectiles[i].x = nextX;
    gServerProjectiles[i].y = nextY;

    // ④ 飛距離チェック
    gServerProjectiles[i].distance++;
    if (gServerProjectiles[i].distance >= maxRange) {
        gServerProjectiles[i].active = 0;
        break;
    }

    // ⑤ プレイヤー当たり判定
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
            break;
        }
    }

    if (!gServerProjectiles[i].active) break;
}


        // 画面外判定
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
            
            int step = 10; 
            int nextX = gPlayerPosX[pos];
            int nextY = gPlayerPosY[pos];

            // 1. 移動後の暫定座標を計算
            if (d == DIR_UP)         { nextY -= step; }
            else if (d == DIR_DOWN)  { nextY += step; }
            else if (d == DIR_LEFT)  { nextX -= step; }
            else if (d == DIR_RIGHT) { nextX += step; }
            else if (d == DIR_UP_LEFT)    { nextY -= step; nextX -= step; }
            else if (d == DIR_UP_RIGHT)   { nextY -= step; nextX += step; }
            else if (d == DIR_DOWN_LEFT)  { nextY += step; nextX -= step; }
            else if (d == DIR_DOWN_RIGHT) { nextY += step; nextX += step; }

            // 2. 8つの正方形（壁）との当たり判定
            int canMove = 1;
            
            // クライアントの描画(client_win_utf8.c)と完全に一致させる定数
            int screenW = 1300; // DEFAULT_WINDOW_WIDTH
            int screenH = 1000; // DEFAULT_WINDOW_HEIGHT
            int cols = 4;
            int rows = 2;
            int cell_w = screenW / cols;
            int cell_h = screenH / rows;
            int bSize = 150; // BLOCK_SIZE
            int pSize = 50;  // PLAYER_SIZE (SQ_SIZE)

            for (int i = 0; i < 8; i++) {
                int r = i / cols;
                int c = i % cols;
                
                // 正方形の左上座標を計算（描画ロジックと同一）
                int bx = c * cell_w + (cell_w / 2) - (bSize / 2);
                int by = r * cell_h + (cell_h / 2) - (bSize / 2);

                // AABB衝突判定（プレイヤーが正方形の範囲に少しでも重なったら衝突）
                if (nextX < bx + bSize &&
                    nextX + pSize > bx &&
                    nextY < by + bSize &&
                    nextY + pSize > by) {
                    canMove = 0; 
                    // printf("[DEBUG] Collision with block %d at (%d, %d)\n", i, bx, by);
                    break;
                }
            }

            // 3. 画面外（ウィンドウ端）の判定も追加（念のため）
            if (nextX < 0 || nextX + pSize > screenW || nextY < 0 || nextY + pSize > screenH) {
                canMove = 0;
            }

            // 4. 壁に当たっていなければ座標を更新
            if (canMove) {
                gPlayerPosX[pos] = nextX;
                gPlayerPosY[pos] = nextY;
            }

            // 5. 全クライアントへ現在の確定座標を通知
            // 注意：当たって動けなかった場合も、現在の座標を送ることでクライアント側の位置を補正します
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

            int weaponID = gClientWeaponID[id];
            
            // ★ ステータス表から最大連射数(STAT_RATE)を取得
            int maxBullet = gServerWeaponStats[weaponID][STAT_RATE];

            // 弾数制限のチェック
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
                    
                    // ★ 重要：飛距離計算用のカウントをリセット
                    gServerProjectiles[i].distance = 0; 
                    break;
                }
            }

            // 初回発射時のみゲームループタイマーを開始
            static int ti = 0;
            if (!ti) {
                SDL_AddTimer(16, ServerGameLoop, NULL);
                ti = 1;
            }

            // 全クライアントへ弾の生成を通知
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
for (int i = 0; i < MAX_PROJECTILES; i++){
    if (gServerProjectiles[i].active &&
        gServerProjectiles[i].firedByClientID == playerID) {
        count++;
    }
}
return count;
}
