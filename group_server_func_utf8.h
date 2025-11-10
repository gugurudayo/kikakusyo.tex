/*****************************************************************
ファイル名	: group_server_func_utf8.h
機能		: サーバーの外部関数の定義
*****************************************************************/

#ifndef _SERVER_FUNC_H_
#define _SERVER_FUNC_H_

#include"group_server_common_utf8.h"

/* server_net.c */
extern int SetUpServer(int num);
extern void Ending(void);
extern int RecvIntData(int pos,int *intData);
extern void SendData(int pos,void *data,int dataSize);
extern int SendRecvManager(void);
// 追加部分
extern int GetClientNum(void);

/* server_command.c */
extern int ExecuteCommand(char command,int pos);
extern void SendDiamondCommand(void);
// 追加部分
extern void InitClientHands(void);

#endif
