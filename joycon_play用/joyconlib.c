/* joyconlib.c */
#include <SDL2/SDL.h>
#include <string.h>
#include "joyconlib.h"
#include "common_utf8.h"
#include "client_func_utf8.h"
#include <SDL2/SDL_mixer.h>

/* 外部参照の宣言 */
extern int gMyClientID;
extern int gCurrentScreenState;
extern int gXPressedFlags[MAX_CLIENTS];
extern SDL_Window *gMainWindow;
extern int gCountdownValue; 

/* 外部関数 */
extern void DrawImageAndText(void);
extern void SendXCommandWithState(int clientID, int screenState);
extern void HandleWeaponSelection(int mx, int my);
extern void SetCharData2DataBlock(void *data, char charData, int *dataSize);

/* バトル用外部変数 */
extern int gControlMode;
extern Uint32 gLastFireTime;
extern int gSelectedWeaponID;
extern int gWeaponStats[MAX_WEAPONS][MAX_STATS_PER_WEAPON];
extern Mix_Chunk *gSoundFire;

#define MODE_MOVE 0
#define MODE_FIRE 1
#define SCREEN_STATE_LOBBY_WAIT 0

static joyconlib_t gJoycon;
static int gJoyconEnabled = 0;
static joycon_btn gLastJoyconBtn;

void InitJoycon() {
    if (joycon_open(&gJoycon, JOYCON_R) == JOYCON_ERR_NONE) 
    {
        gJoyconEnabled = 1;
        joycon_set_led(&gJoycon, JOYCON_LED_1_ON);
        memset(&gLastJoyconBtn, 0, sizeof(joycon_btn));
        printf("[JOYCON] Connected.\n");
    } 
    else 
    {
        gJoyconEnabled = 0;
    }
}

void CleanupJoycon() {
    if (gJoyconEnabled) 
    joycon_close(&gJoycon);
}

void ProcessJoyconInput() {
    if (!gJoyconEnabled) 
    return;

    Uint32 flags = SDL_GetWindowFlags(gMainWindow);
    if (!(flags & SDL_WINDOW_INPUT_FOCUS)) 
    return;

    if (joycon_get_state(&gJoycon) != JOYCON_ERR_NONE) 
    return;

    if (gCurrentScreenState == SCREEN_STATE_GAME_SCREEN) 
    {
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        float th = 0.2f; 
        int speed = 15;

        if (gJoycon.stick.x < -th) 
        my -= speed; 
        if (gJoycon.stick.x > th) 
         my += speed;
        if (gJoycon.stick.y > th) 
         mx -= speed;  
        if (gJoycon.stick.y < -th) 
        mx += speed; 

        SDL_WarpMouseInWindow(gMainWindow, mx, my);

        if (gJoycon.button.btn.X && !gLastJoyconBtn.btn.X) 
        {
            HandleWeaponSelection(mx, my);
        }
    }

    else if (gCurrentScreenState == SCREEN_STATE_RESULT) 
    {
        /* 2-1. モード切替 */
        if (gJoycon.button.btn.B && !gLastJoyconBtn.btn.B) 
        {
            gControlMode = (gControlMode == MODE_MOVE) ? MODE_FIRE : MODE_MOVE;
            DrawImageAndText();
        }

        /* 2-2. 8方向移動 */
        if (gControlMode == MODE_MOVE) 
        {
            if (gCountdownValue > 0) 
            return; 
            static Uint32 lastMoveTime = 0;
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - lastMoveTime >= 50) 
            {
                float th = 0.5f;
                char dir = 0;
                int up    = (gJoycon.stick.x < -th);
                int down  = (gJoycon.stick.x > th);
                int left  = (gJoycon.stick.y > th);  
                int right = (gJoycon.stick.y < -th); 

                if (up && left) 
                dir = DIR_UP_LEFT;

                else if (up && right) 
                dir = DIR_UP_RIGHT;

                else if (down && left) 
                dir = DIR_DOWN_LEFT;

                else if (down && right) 
                dir = DIR_DOWN_RIGHT;
                else if (up) 
                dir = DIR_UP;
                else if (down) 
                dir = DIR_DOWN;
                else if (left) 
                dir = DIR_LEFT;
                else if (right) 
                dir = DIR_RIGHT;
                if (dir != 0) 
                {
                    unsigned char data[MAX_DATA];
                    int ds = 0;
                    SetCharData2DataBlock(data, MOVE_COMMAND, &ds);
                    SetCharData2DataBlock(data, dir, &ds);
                    SendData(data, ds);
                    lastMoveTime = currentTime;
                }
            }
        }
        else if (gControlMode == MODE_FIRE) 
        {
            if (gJoycon.button.btn.A) 
            {
                Uint32 currentTime = SDL_GetTicks();
                int weaponID = gSelectedWeaponID < 0 ? 0 : gSelectedWeaponID;
                int ct = gWeaponStats[weaponID][STAT_CT_TIME];
                if (currentTime - gLastFireTime >= (Uint32)ct) 
                {
                    float th = 0.5f;
                    char fDir = 0;
                    if (gJoycon.stick.x < -th && gJoycon.stick.y > th) 
                    fDir = DIR_UP_LEFT;

                    else if (gJoycon.stick.x < -th && gJoycon.stick.y < -th) 
                    fDir = DIR_UP_RIGHT;

                    else if (gJoycon.stick.x > th && gJoycon.stick.y > th) 
                    fDir = DIR_DOWN_LEFT;

                    else if (gJoycon.stick.x > th && gJoycon.stick.y < -th) 
                    fDir = DIR_DOWN_RIGHT;

                    else if (gJoycon.stick.x < -th) 
                    fDir = DIR_UP;

                    else if (gJoycon.stick.x > th)  
                    fDir = DIR_DOWN;

                    else if (gJoycon.stick.y > th)  
                    fDir = DIR_LEFT;

                    else if (gJoycon.stick.y < -th) 
                    fDir = DIR_RIGHT;

                    if (fDir != 0) 
                    {
                        SendFireCommand(fDir);
                        gLastFireTime = currentTime;
                        if (gSoundFire) 
                        Mix_PlayChannel(-1, gSoundFire, 0);
                    }
                }
            }
        }
    }
    else if (gCurrentScreenState == SCREEN_STATE_LOBBY_WAIT || gCurrentScreenState == SCREEN_STATE_TITLE) 
    {
        if (gJoycon.button.btn.X && !gLastJoyconBtn.btn.X) 
        {
            if (gXPressedFlags[gMyClientID] == 0) 
            {
                gXPressedFlags[gMyClientID] = 1;
                DrawImageAndText();
                SendXCommandWithState(gMyClientID, gCurrentScreenState);
            }
        }
    }
    gLastJoyconBtn = gJoycon.button;
}
