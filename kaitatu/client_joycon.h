/*client_joycon.h*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Joy-Con 初期化（失敗してもゲームは継続可能） */
int Joycon_Init(void);

/* 毎フレーム呼び出し
   weaponSelect = 1 の時はマウス操作モード */
void Joycon_Update(int weaponSelect);

/* 終了処理 */
void Joycon_Quit(void);

#ifdef __cplusplus
}
#endif
