/*common_utf8.h*/
#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<assert.h>
#include<math.h>

#define PORT            (u_short)8888   /* ポート番号 */
#define MAX_CLIENTS     4               /* クライアント数の最大値 */
#define MAX_NAME_SIZE   10              /* ユーザー名の最大値*/
#define MAX_DATA        200             /* 送受信するデータの最大値 */
#define X_COMMAND       'X'   /* クライアントがXキーを押したことをサーバへ通知 */
#define UPDATE_X_COMMAND 'U'   /* サーバーが全クライアントへX押下情報を配信 */
#define END_COMMAND     'E'             /* プログラム終了コマンド */
#define NEXT_SCREEN_COMMAND 'N' /* 次の画面へ進むコマンド */
#define SELECT_WEAPON_COMMAND 'W' /* クライアントが武器を選択したことを通知 */
#define START_GAME_COMMAND 'S' /* サーバーがクライアントに武器選択画面へ移らせるコマンド */
#define MOVE_COMMAND 'M' /* クライアントが移動要求をサーバーへ通知 */
#define UPDATE_MOVE_COMMAND 'D' /* サーバーが全クライアントへ移動情報を配信 */
#define FIRE_COMMAND            'F'   /* クライアントが発砲要求をサーバーへ通知 */
#define UPDATE_PROJECTILE_COMMAND 'P' /* サーバーが全クライアントへ発射体情報を配信 */
#define APPLY_DAMAGE_COMMAND    'A'

// 発射体関連の定数
#define MAX_PROJECTILES_PER_CLIENT 5
#define MAX_PROJECTILES (MAX_PROJECTILES_PER_CLIENT * MAX_CLIENTS)
#define PROJECTILE_STEP 1 // 発射体の移動速度

#define MAX_WEAPONS 4 // 武器の数
#define MAX_STATS_PER_WEAPON 6 // 1つの武器に紐づくステータスの数 (CT, 飛距離, 威力, 体力, 連射数, 移動速度)
#define MAX_STAT_NAME_SIZE 32 // ステータス名の最大文字数

// 武器ステータスIDの定義 (配列アクセス用)
#define STAT_CT_TIME 0
#define STAT_RANGE 1
#define STAT_DAMAGE 2
#define STAT_HP 3
#define STAT_RATE 4
#define STAT_SPEED 5

// 移動方向の定義
#define DIR_UP    'U'
#define DIR_DOWN  'D'
#define DIR_LEFT  'L'
#define DIR_RIGHT 'R'
#define TRANSITION_TIMER_ID 100 /* 画面遷移時の遅延時間（ミリ秒） */

#endif
