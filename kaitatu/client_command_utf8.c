/* client_command_utf8.c */

#include "common_utf8.h"
#include "client_func_utf8.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <string.h> 

extern int gCurrentScreenState;
extern int gPlayerHP[MAX_CLIENTS];
extern Projectile gProjectiles[MAX_PROJECTILES];

/* 自分のID管理 */
static int gMyID; 

/* ★追加：トラップの状態を保持する変数（client_win_utf8.cでも使用） ★ */
int gTrapActive = 0;
int gTrapX = 0;
int gTrapY = 0;
int gTrapType = 0; 

void SetMyClientID(int id) {
    gMyID = id;
    printf("[DEBUG] My Client ID is set to: %d\n", gMyID);
}

void SetIntData2DataBlock(void *data, int intData, int *dataSize) {
    int tmp = htonl(intData);
    memcpy((char*)data + (*dataSize), &tmp, sizeof(int));
    (*dataSize) += sizeof(int);
}

void SetCharData2DataBlock(void *data, char charData, int *dataSize) {
    *(char *)((char*)data + (*dataSize)) = charData;
    (*dataSize) += sizeof(char);
}

void SendXCommandWithState(int clientID, int screenState) {
    unsigned char data[MAX_DATA];
    int ds = 0;
    
    SetCharData2DataBlock(data, X_COMMAND, &ds);
    SetIntData2DataBlock(data, clientID, &ds);
    SetIntData2DataBlock(data, screenState, &ds);
    
    SendData(data, ds);
}

int ExecuteCommand(char command) {
    int endFlag = 1;
    
    switch (command) {
        case END_COMMAND:
            endFlag = 0;
            break;

        case UPDATE_X_COMMAND: {
            int sid;
            RecvIntData(&sid);
            SetXPressedFlag(sid);
            break;
        }

        case START_GAME_COMMAND:
            SetScreenState(SCREEN_STATE_GAME_SCREEN);
            break;

        case NEXT_SCREEN_COMMAND: 
            if (gCurrentScreenState == SCREEN_STATE_GAME_SCREEN) {
                SetScreenState(SCREEN_STATE_RESULT);
            } else if (gCurrentScreenState == SCREEN_STATE_RESULT) {
                SetScreenState(SCREEN_STATE_TITLE);
            }
            break;

        case UPDATE_MOVE_COMMAND: {
            int sid;
            char d;
            RecvIntData(&sid);
            RecvCharData(&d);
            UpdatePlayerPos(sid, d);
            DrawImageAndText();
            break;
        }

        case UPDATE_PROJECTILE_COMMAND: {
            int sid, x, y;
            char d;
            RecvIntData(&sid);
            RecvIntData(&x);
            RecvIntData(&y);
            RecvCharData(&d);
            
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (!gProjectiles[i].active) {
                    gProjectiles[i] = (Projectile){x, y, sid, 1, d, 0};
                    break;
                }
            }
            DrawImageAndText();
            break;
        }

        case APPLY_DAMAGE_COMMAND: {
            int targetID, damage;
            RecvIntData(&targetID); 
            RecvIntData(&damage);
            
            if (targetID >= 0 && targetID < MAX_CLIENTS) {
                gPlayerHP[targetID] -= damage;
                /* 最大HP(150)を超えないように制限 */
                if (gPlayerHP[targetID] > 150) {
                    gPlayerHP[targetID] = 150;
                }
                if (gPlayerHP[targetID] < 0) {
                    gPlayerHP[targetID] = 0;
                }
                printf("[CLIENT] Player %d HP update: %d (change: %d)\n", targetID, gPlayerHP[targetID], damage);
            }
            DrawImageAndText();
            break;
        }

        /* ★追加：サーバーからのトラップ通知を受信 ★ */
        case UPDATE_TRAP_COMMAND: {
            int active;
            RecvIntData(&active);
            gTrapActive = active;
            
            if (active) {
                RecvIntData(&gTrapX);
                RecvIntData(&gTrapY);
                RecvIntData(&gTrapType); // ★追加 タイプを受信
                printf("[CLIENT] Trap SPAWNED at (%d, %d)\n", gTrapX, gTrapY);
            } else {
                printf("[CLIENT] Trap DESPAWNED\n");
            }
            DrawImageAndText();
            break;
        }

        case SELECT_WEAPON_COMMAND: {
            int wid;
            RecvIntData(&wid);
            break;
        }

        default:
            break;
    }
    
    return endFlag;
}

void SendEndCommand(void) {
    unsigned char d[MAX_DATA];
    int s = 0;
    
    SetCharData2DataBlock(d, END_COMMAND, &s);
    SendData(d, s);
}
