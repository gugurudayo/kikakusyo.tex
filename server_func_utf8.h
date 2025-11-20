#ifndef _SERVER_FUNC_H_
#define _SERVER_FUNC_H_

#include"server_common_utf8.h"

/* CLIENT 型と配列の外部宣言 */
typedef struct {
    int fd;
    char name[MAX_NAME_SIZE];
} CLIENT;

extern CLIENT gClients[MAX_CLIENTS];

/* server_net.c */
extern int SetUpServer(int num);
extern void Ending(void);
extern int RecvIntData(int pos,int *intData);
extern void SendData(int pos,void *data,int dataSize);
extern int SendRecvManager(void);
extern int GetClientNum(void);

/* server_command.c */
extern int ExecuteCommand(char command,int pos);
extern void SendDiamondCommand(void);
extern void SendDataToClient(int clientID, const void *data, int dataSize);

#endif
