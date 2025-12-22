/* client_command_utf8.c */

#include "common_utf8.h"
#include "client_func_utf8.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <string.h> 

extern int gCurrentScreenState;
extern int gPlayerHP[MAX_CLIENTS];
extern Projectile gProjectiles[MAX_PROJECTILES];
#define UPPER_LEFT 20
#define LOWER_LEFT 20
#define UPPER_RIGHT 20
#define LOWER_RIGHT 20

/* 自分のID管理 */
static int gMyID; 

void SetMyClientID(int id) {
    gMyID = id;
    printf("[DEBUG] My Client ID is set to: %d\n", gMyID);
}

void SetIntData2DataBlock(void *data, int intData, int *dataSize) {
    int tmp = htonl(intData);
    memcpy((char*)data + (*dataSize), &tmp, sizeof(int));
    (*dataSize) += sizeof(int);
}

void SetCharData2DataBlock(void *data, char charData, int *dataSize) {
    *(char *)((char*)data + (*dataSize)) = charData;
    (*dataSize) += sizeof(char);
}

void SendXCommandWithState(int clientID, int screenState) {
    unsigned char data[MAX_DATA];
    int ds = 0;
    
    SetCharData2DataBlock(data, X_COMMAND, &ds);
    SetIntData2DataBlock(data, clientID, &ds);
    SetIntData2DataBlock(data, screenState, &ds);
    
    SendData(data, ds);
}

int ExecuteCommand(char command) {
    int endFlag = 1;
    
    switch (command) {
        case END_COMMAND:
            endFlag = 0;
            break;

        case UPDATE_X_COMMAND: {
            int sid;
            RecvIntData(&sid);
            SetXPressedFlag(sid);
            break;
        }

        case START_GAME_COMMAND:
            SetScreenState(SCREEN_STATE_GAME_SCREEN);
            break;

        case NEXT_SCREEN_COMMAND: 
            if (gCurrentScreenState == SCREEN_STATE_GAME_SCREEN) {
                SetScreenState(SCREEN_STATE_RESULT);
            } else if (gCurrentScreenState == SCREEN_STATE_RESULT) {
                SetScreenState(SCREEN_STATE_TITLE);
            }
            break;

        case UPDATE_MOVE_COMMAND: {
            int sid;
            char d;
            RecvIntData(&sid);
            RecvCharData(&d);
            UpdatePlayerPos(sid, d);
            DrawImageAndText();
            break;
        }

        case UPDATE_PROJECTILE_COMMAND: {
		int sid, x, y, weapon = 1;
		char d;
            RecvIntData(&sid);
            RecvIntData(&x);
            RecvIntData(&y);
            RecvCharData(&d);
	    //RecvIntData(&weapon);

	    for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (!gProjectiles[i].active) {
                    gProjectiles[i] = (Projectile){x, y, sid, 1, d};
                    break;
                }
            }
            DrawImageAndText();
            break;
	    
	
          /*
	   printf("%d\n", weapon); 
	   switch (weapon)
	   { 
		   case 0:{
            for (int i = 0; i < UPPER_LEFT; i++) {
                if (!gProjectiles[i].active) {
                    gProjectiles[i] = (Projectile){x, y, sid, 1, d};
		    printf("i=%d\n", i);
                    break;
                }
            }
            DrawImageAndText();
	   break;
			  }
		case 1:{
	   for (int i = 0; i < UPPER_RIGHT; i++) {
                if (!gProjectiles[i].active) {
                    gProjectiles[i] = (Projectile){x, y, sid, 1, d};
                    break;
                }
            }
            DrawImageAndText();
           
            break;
		       }
	case 2:{
	    for (int i = 0; i < LOWER_LEFT; i++) {
                if (!gProjectiles[i].active) {
                    gProjectiles[i] = (Projectile){x, y, sid, 1, d};
                    break;
                }
            }
            DrawImageAndText();
           
            break;
	       }
	case 3:{
	    for (int i = 0; i < LOWER_RIGHT; i++) {
                if (!gProjectiles[i].active) {
                    gProjectiles[i] = (Projectile){x, y, sid, 1, d};
                    break;
                }
            }
            DrawImageAndText();
           
            break;
	   }
	   }*/
    }

        case APPLY_DAMAGE_COMMAND: {
            int targetID, damage;
            RecvIntData(&targetID); 
            RecvIntData(&damage);
            
            if (targetID >= 0 && targetID < MAX_CLIENTS) {
                gPlayerHP[targetID] -= damage;
                if (gPlayerHP[targetID] < 0) {
                    gPlayerHP[targetID] = 0;
                }
                printf("[CLIENT] Player %d took %d dmg. HP: %d\n", targetID, damage, gPlayerHP[targetID]);
            }
            DrawImageAndText();
            break;
        }

        case SELECT_WEAPON_COMMAND: {
            int wid;
            RecvIntData(&wid);
            break;
        }

        default:
            break;
    }
    
    return endFlag;
}

void SendEndCommand(void) {
    unsigned char d[MAX_DATA];
    int s = 0;
    
    SetCharData2DataBlock(d, END_COMMAND, &s);
    SendData(d, s);
}
