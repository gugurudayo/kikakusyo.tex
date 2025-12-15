#ifndef _CLIENT_FUNC_H_
#define _CLIENT_FUNC_H_
#include"common_utf8.h"

// 発射体の構造体 
typedef struct {
    int x;
    int y;
    int clientID; // 誰が撃ったか
    int active;   // 1:有効, 0:無効
    char direction;
} Projectile;

extern Projectile gProjectiles[MAX_PROJECTILES];

extern void InitProjectiles(void);
extern void UpdateAndDrawProjectiles(void);
extern void SendFireCommand(char direction); 

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
extern void DrawImageAndText(void); 
extern int gMyClientID;     
extern char gAllName[MAX_CLIENTS][MAX_NAME_SIZE]; 
extern int gPlayerHP[MAX_CLIENTS]; 

/* client_command.c */
extern int ExecuteCommand(char command);
extern void SendEndCommand(void);
extern void SetMyClientID(int clientID);
extern void SetIntData2DataBlock(void *data, int intData, int *dataSize);
extern void SetCharData2DataBlock(void *data, char charData, int *dataSize);

#define SCREEN_STATE_LOBBY 0       
#define SCREEN_STATE_GAME_SCREEN 1  
#define SCREEN_STATE_RESULT 2
extern int ExecuteCommand(char command);
extern void SetScreenState(int state);
extern void SetXPressedFlag(int clientID); 
extern void UpdatePlayerPos(int clientID, char direction); 
extern void SetPlayerMoveStep(int clientID, int step); 
#endif
