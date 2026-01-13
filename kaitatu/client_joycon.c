/*client_joycon.c*/
#include "client_joycon.h"
#include "joyconlib.h"
#include <stdio.h>

/* 既存の入力状態（キーボード・マウス側で既に使われているもの） */
extern int KeyX;
extern int KeyC;
extern int KeySpace;
extern int KeyUp;
extern int KeyDown;
extern int KeyLeft;
extern int KeyRight;

extern int MouseClick;
extern int MouseX;
extern int MouseY;

static joyconlib_t gJoycon;
static int gJoyconAvailable = 0;

int Joycon_Init(void)
{
    if (joycon_open(&gJoycon, JOYCON_R) != JOYCON_ERR_NONE) {
        printf("[JOYCON] not found\n");
        return -1;
    }

    gJoyconAvailable = 1;
    printf("[JOYCON] connected\n");
    return 0;
}

void Joycon_Update(int weaponSelect)
{
    if (!gJoyconAvailable) return;
    if (joycon_get_state(&gJoycon) != JOYCON_ERR_NONE) return;

    /* --- ボタン割り当て（横持ち）--- */
    if (gJoycon.button.btn.X) KeyX = 1;        /* Xボタン → Xキー */
    if (gJoycon.button.btn.Y) KeyC = 1;        /* Yボタン → Cキー */
    if (gJoycon.button.btn.B) KeySpace = 1;    /* Bボタン → Space */
    if (gJoycon.button.btn.A) MouseClick = 1;  /* Aボタン → クリック */

    /* --- スティック処理 --- */
    float sx = gJoycon.stick.x;
    float sy = gJoycon.stick.y;
    const float DEAD = 0.3f;

    if (weaponSelect) {
        /* 武器選択画面：マウスカーソル操作 */
        MouseX += (int)(sx * 12.0f);
        MouseY += (int)(sy * 12.0f);
    } else {
        /* 通常時：移動操作 */
        if (sx < -DEAD) KeyLeft  = 1;
        if (sx >  DEAD) KeyRight = 1;
        if (sy < -DEAD) KeyUp    = 1;
        if (sy >  DEAD) KeyDown  = 1;
    }
}

void Joycon_Quit(void)
{
    if (!gJoyconAvailable) return;
    joycon_close(&gJoycon);
    gJoyconAvailable = 0;
}
