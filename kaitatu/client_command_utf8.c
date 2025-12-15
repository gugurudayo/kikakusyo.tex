/*client_command_utf8.c*/
#include "common_utf8.h"
#include "client_func_utf8.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <string.h> 

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
            SetXPressedFlag(senderID);
            break;
        }
        case START_GAME_COMMAND:
        {
            SetScreenState(SCREEN_STATE_GAME_SCREEN);
            break;
        }
        case SELECT_WEAPON_COMMAND:
        {
            int selectedWeaponID;
            RecvIntData(&selectedWeaponID);
            
            int newStep = 0;
            switch (selectedWeaponID) {
                case 0: 
                case 1: 
                    newStep = 20; // 高速
                    break;
                case 2: 
                case 3:
                default:
                    newStep = 5; // 低速
                    break;
            }
            SetPlayerMoveStep(gMyClientID, newStep);
            extern int gWeaponStats[MAX_WEAPONS][MAX_STATS_PER_WEAPON]; // 武器ステータス配列の外部宣言
            //gPlayerHP[gMyClientID] = gWeaponStats[selectedWeaponID][STAT_HP]; // ★ 削除済み ★
            break;
        }
        case NEXT_SCREEN_COMMAND:
        {
            // 全員が武器選択を終えた -> 結果画面（バトル画面）へ
            SetScreenState(SCREEN_STATE_RESULT);
            break;
        }
        case UPDATE_MOVE_COMMAND:
        {
            int clientID;
            char direction;
            RecvIntData(&clientID);
            RecvCharData(&direction); 
            UpdatePlayerPos(clientID, direction);
            DrawImageAndText(); 
            break;
        }
        
        case UPDATE_PROJECTILE_COMMAND: 
        {
            int clientID, x, y; 
            char direction; 
            RecvIntData(&clientID); // 発射元ID
            RecvIntData(&x);        // 初期X座標
            RecvIntData(&y);        // 初期Y座標
            RecvCharData(&direction); 

            for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (!gProjectiles[i].active) {
                    gProjectiles[i].active = 1;
                    gProjectiles[i].clientID = clientID;
                    gProjectiles[i].x = x; 
                    gProjectiles[i].y = y;
                    gProjectiles[i].direction = direction; 
                    break;
                }
            }
            DrawImageAndText(); 
            break;
        }

        case APPLY_DAMAGE_COMMAND: 
        {
            int targetClientID;
            int damageAmount;
            RecvIntData(&targetClientID);
            RecvIntData(&damageAmount);
            if (targetClientID >= 0 && targetClientID < MAX_CLIENTS) {
                gPlayerHP[targetClientID] -= damageAmount;
                if (gPlayerHP[targetClientID] < 0) {
                    gPlayerHP[targetClientID] = 0;
                }
            }
            DrawImageAndText(); // HPバー更新のため再描画
            break;
        }        
        default:
            break;
    }
    return endFlag;
}
/* SendEndCommand: 終了コマンド送信 */
void SendEndCommand(void)
{
    unsigned char data[MAX_DATA];
    int dataSize = 0;
    SetCharData2DataBlock(data, END_COMMAND, &dataSize);
    SendData(data, dataSize);
}

/* SetIntData2DataBlock: intデータをデータブロックに格納 */
void SetIntData2DataBlock(void *data,int intData,int *dataSize)
{
    int tmp;
    assert(data!=NULL);
    assert(0<=(*dataSize));
    tmp = htonl(intData);
    memcpy((char*)data + (*dataSize), &tmp, sizeof(int));
    (*dataSize) += sizeof(int);
}

/* SetCharData2DataBlock: charデータをデータブロックに格納 */
void SetCharData2DataBlock(void *data,char charData,int *dataSize)
{
    assert(data!=NULL);
    assert(0<=(*dataSize));
    *(char *)((char*)data + (*dataSize)) = charData;
    (*dataSize) += sizeof(char);
}

/* RecvResultData: 結果受信（未使用）*/
static void RecvResultData(void)
{
    // ... (省略) ...
}

/* SetMyClientID: 自分のID設定（ここでは未使用）*/
void SetMyClientID(int clientID)
{
    // ... (省略) ...
}
