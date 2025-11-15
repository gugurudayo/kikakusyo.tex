/*****************************************************************
ファイル名   : server_command_utf8.c
機能        : サーバーのコマンド処理（全員押下/選択後3秒遅延送信）
*****************************************************************/

#include "server_common_utf8.h"
#include "server_func_utf8.h"
#include <arpa/inet.h>
#include <SDL2/SDL.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void SetIntData2DataBlock(void *data,int intData,int *dataSize);
static void SetCharData2DataBlock(void *data,char charData,int *dataSize);

/* 状態 */
static char gClientHands[MAX_CLIENTS] = {0};          // 0: 未選択, 1..: 選択済み(weaponID+1)
static int gHandsCount = 0;
static int gXPressedClientFlags[MAX_CLIENTS] = {0};   // X押下フラグ（サーバー側保持）
static int gXPressedCount = 0;

/* ===================== SDLタイマー用構造体 ===================== */
typedef struct {
    unsigned char cmd;  // 送信コマンド
} TimerParam;

/* タイマーコールバック（3秒後に全クライアントへ送信） */
static Uint32 SendCommandAfterDelay(Uint32 interval, void *param)
{
    TimerParam *p = (TimerParam*)param;
    unsigned char data[MAX_DATA];
    int dataSize = 0;
    SetCharData2DataBlock(data, p->cmd, &dataSize);
    SendData(ALL_CLIENTS, data, dataSize);
    free(param);
    printf("[SERVER] Sent command 0x%02X after 3 seconds delay\n", p->cmd);
    return 0; // 一度だけ
}

/* ===================== ExecuteCommand ===================== */
int ExecuteCommand(char command,int pos)
{
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
            // クライアントが X 押下を送ってきた（posは送信元）
            int senderID;
            RecvIntData(pos, &senderID);

            if (senderID < 0 || senderID >= GetClientNum()) break;

            if (gXPressedClientFlags[senderID] == 0) {
                gXPressedClientFlags[senderID] = 1;
                gXPressedCount++;
            }

            // 全クライアントに「誰が押したか」を通知（UPDATE_X_COMMAND）
            dataSize = 0;
            SetCharData2DataBlock(data, UPDATE_X_COMMAND, &dataSize);
            SetIntData2DataBlock(data, senderID, &dataSize);
            SendData(ALL_CLIENTS, data, dataSize);

            // もし全員押していたら => 3秒後に START_GAME_COMMAND を送信
            if (gXPressedCount == GetClientNum()) {
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

            if (senderID < 0 || senderID >= GetClientNum()) break;

            if (gClientHands[senderID] == 0) {
                gClientHands[senderID] = (char)(selectedWeaponID + 1);
                gHandsCount++;
            }

            // 全員選択で 3秒後に NEXT_SCREEN_COMMAND を送信
            if (gHandsCount == GetClientNum()) {
                TimerParam *tparam = malloc(sizeof(TimerParam));
                tparam->cmd = NEXT_SCREEN_COMMAND;
                SDL_AddTimer(3000, SendCommandAfterDelay, tparam);

                // リセットして次ラウンドへ
                gHandsCount = 0;
                memset(gClientHands, 0, sizeof(gClientHands));
            }
            break;
        }

        default:
            fprintf(stderr,"Unknown command: 0x%02x\n", (unsigned char)command);
            break;
    }

    return endFlag;
}

/* ===================== データ作成ユーティリティ ===================== */
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
