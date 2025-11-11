/*****************************************************************
ファイル名	: client_func_utf8.h
機能		: クライアントの外部関数の定義
*****************************************************************/

#ifndef _CLIENT_FUNC_H_
#define _CLIENT_FUNC_H_

#include"common_utf8.h"

/* client_net.c */
extern int SetUpClient(char* hostName,int *clientID,int *num,char clientName[][MAX_NAME_SIZE]);
extern void CloseSoc(void);
extern int RecvIntData(int *intData);
extern void SendData(void *data,int dataSize);
extern int SendRecvManager(void);
extern int RecvCharData(char *charData); // 追加部分

/* client_win.c */
extern int InitWindows(int clientID,int num,char name[][MAX_NAME_SIZE]);
extern void DestroyWindow(void);
extern void WindowEvent(int num);

/* client_command.c */
extern int ExecuteCommand(char command);

extern void SendEndCommand(void);


extern void SetMyClientID(int clientID);

extern void SetIntData2DataBlock(void *data, int intData, int *dataSize);
extern void SetCharData2DataBlock(void *data, char charData, int *dataSize);

// client_func_utf8.h の末尾付近に追記

// 画面の状態定数
#define SCREEN_STATE_LOBBY 0        // 参加者リスト表示画面
#define SCREEN_STATE_GAME_SCREEN 1  // ゲーム本編の画面（リスト非表示）

// client_command.c の外部宣言
extern int ExecuteCommand(char command);
// ...

// client_win.c 側の外部宣言（必要であれば）
extern void SetScreenState(int state); // 【必須追加】この宣言を追加する
extern void SetXPressedFlag(int clientID); // Xキー押下フラグ更新関数が既にあるはず

#endif
