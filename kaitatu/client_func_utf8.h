#ifndef _CLIENT_FUNC_H_
#define _CLIENT_FUNC_H_
#include"common_utf8.h"

// 発射体の構造体 (★修正: directionを追加★)
typedef struct {
    int x;
    int y;
    int clientID; // 誰が撃ったか
    int active;   // 1:有効, 0:無効
    char direction; // ★ 発射方向を保持 ★
} Projectile;

extern Projectile gProjectiles[MAX_PROJECTILES];

extern void InitProjectiles(void);
extern void UpdateAndDrawProjectiles(void);
extern void SendFireCommand(char direction); // ★ 修正: directionを引数に追加 ★

/* client_net.c */
extern int SetUpClient(char* hostName,int *clientID,int *num,char clientName[][MAX_NAME_SIZE]);
extern void CloseSoc(void);
extern int RecvIntData(int *intData);
extern void SendData(void *data,int dataSize);
extern int SendRecvManager(void);
extern int RecvCharData(char *charData); 

/* client_win.c */
extern int InitWindows(int clientID,int num,char name[][MAX_NAME_SIZE]);
extern void DestroyWindow(void);
extern void WindowEvent(int num);
extern void DrawImageAndText(void); // DrawImageAndTextの宣言
extern int gMyClientID;     // 自分のクライアント番号
extern char gAllName[MAX_CLIENTS][MAX_NAME_SIZE]; // 全員の名前
extern int gPlayerHP[MAX_CLIENTS]; // 全員のHP配列の外部宣言を維持

/* client_command.c */
extern int ExecuteCommand(char command);
extern void SendEndCommand(void);
extern void SetMyClientID(int clientID);
extern void SetIntData2DataBlock(void *data, int intData, int *dataSize);
extern void SetCharData2DataBlock(void *data, char charData, int *dataSize);

// 画面の状態定数
#define SCREEN_STATE_LOBBY 0       
#define SCREEN_STATE_GAME_SCREEN 1  
#define SCREEN_STATE_RESULT 2
extern int ExecuteCommand(char command);
extern void SetScreenState(int state);
extern void SetXPressedFlag(int clientID); 
extern void UpdatePlayerPos(int clientID, char direction); // 座標更新ヘルパー関数
extern void SetPlayerMoveStep(int clientID, int step); // 速度設定関数
#endif
