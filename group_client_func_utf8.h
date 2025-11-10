/*****************************************************************
ファイル名	: group_client_func_utf8.h
機能		: クライアントの外部関数の定義
*****************************************************************/

#ifndef _CLIENT_FUNC_H_
#define _CLIENT_FUNC_H_

#include"group_common_utf8.h"

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
extern void DrawRectangle(int x,int y,int width,int height);
extern void DrawCircle(int x,int y,int r);
extern void DrawDiamond(int x,int y,int height);
//追加部分
extern void SetJankenState(int state);
extern void DrawJankenResult(int numClients, char hands[], int myClientID);
/* client_command.c */
extern int ExecuteCommand(char command);
extern void SendRectangleCommand(void);
extern void SendCircleCommand(int pos);
extern void SendEndCommand(void);
//追加部分
extern void SendTyokiCommand(void);
extern void SendPaCommand(void);
extern void SendGuCommand(void);

extern void SetMyClientID(int clientID);


#endif
