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
extern int RecvCharData(char *charData); 

/* client_win.c */
extern int InitWindows(int clientID,int num,char name[][MAX_NAME_SIZE]);
extern void DestroyWindow(void);
extern void WindowEvent(int num);
extern int gMyClientID;     // 自分のクライアント番号
extern char gAllName[MAX_CLIENTS][MAX_NAME_SIZE]; // 全員の名前


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

#endif
