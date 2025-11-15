/*****************************************************************
ファイル名   : client_command_utf8.c
機能      : クライアントのコマンド処理（修正版）
*****************************************************************/

#include "common_utf8.h"
#include "client_func_utf8.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>

/* グローバルID */
//static int gMyClientID = -1;

/* 先行宣言 */
void SetIntData2DataBlock(void *data,int intData,int *dataSize);
void SetCharData2DataBlock(void *data,char charData,int *dataSize);
static void RecvResultData(void);

void SetXPressedFlag(int clientID);

/* ExecuteCommand: サーバーからのコマンド処理 */
int ExecuteCommand(char command)
{
    int endFlag = 1;

    switch(command){
        case END_COMMAND:
            endFlag = 0;
            break;

        case UPDATE_X_COMMAND:
        {
            int senderID;
            RecvIntData(&senderID);
            // サーバーが誰が押したかを通知してくる
            SetXPressedFlag(senderID);
            break;
        }

        case START_GAME_COMMAND:
        {
            // サーバーが全員X押下を確認したので武器選択画面へ
            SetScreenState(SCREEN_STATE_GAME_SCREEN);
            break;
        }

        case NEXT_SCREEN_COMMAND:
        {
            // 全員が武器選択を終えた → 結果画面（Chatgpt.png）の表示
            SetScreenState(SCREEN_STATE_RESULT);
            break;
        }

        default:
            break;
    }

    return endFlag;
}

/* SendEndCommand etc. は既存実装のまま利用 */
void SendEndCommand(void)
{
    unsigned char data[MAX_DATA];
    int dataSize = 0;
    SetCharData2DataBlock(data, END_COMMAND, &dataSize);
    SendData(data, dataSize);
}

/* (既存の SetInt/SetChar の実装省略されているならそのまま使ってください) */
void SetIntData2DataBlock(void *data,int intData,int *dataSize)
{
    int tmp;
    assert(data!=NULL);
    assert(0<=(*dataSize));
    tmp = htonl(intData);
    memcpy((char*)data + (*dataSize), &tmp, sizeof(int));
    (*dataSize) += sizeof(int);
}
void SetCharData2DataBlock(void *data,char charData,int *dataSize)
{
    assert(data!=NULL);
    assert(0<=(*dataSize));
    *(char *)((char*)data + (*dataSize)) = charData;
    (*dataSize) += sizeof(char);
}

/* 結果受信（もし使うなら） */
static void RecvResultData(void)
{
    int numClients;
    char hands[MAX_CLIENTS];
    int i;
    RecvIntData(&numClients);
    for(i=0;i<numClients;i++){
        RecvCharData(&hands[i]);
    }
}

/* 自分のID設定 */
void SetMyClientID(int clientID)
{
    gMyClientID = clientID;
}
