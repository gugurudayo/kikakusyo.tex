/*common_utf8.h*/
#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<assert.h>
#include<math.h>

typedef struct {
    int x;
    int y;
    int clientID; // 誰が撃ったか
    int active;   // 1:有効, 0:無効
    char direction;
    int distance;  // 飛距離管理用
} Projectile;


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

#define MAX_PROJECTILES_PER_CLIENT 5
#define MAX_PROJECTILES (MAX_PROJECTILES_PER_CLIENT * MAX_CLIENTS)
#define PROJECTILE_STEP 1 

#define MAX_WEAPONS 4 
#define MAX_STATS_PER_WEAPON 4 // 1つの武器に紐づくステータスの数 (CT, 飛距離, 威力, 体力, 連射数, 移動速度)
#define MAX_STAT_NAME_SIZE 32 
#define STAT_CT_TIME 0 // クールタイム
#define STAT_RANGE   1 // 飛距離
#define STAT_DAMAGE  2 // 威力
#define STAT_RATE    3 // 連射数（リロードなしで撃てる数）
#define DIR_UP    'U'
#define DIR_DOWN  'D'
#define DIR_LEFT  'L'
#define DIR_RIGHT 'R'
#define DIR_UP_LEFT    '1'
#define DIR_UP_RIGHT   '2'
#define DIR_DOWN_LEFT  '3'
#define DIR_DOWN_RIGHT '4'
#define TRANSITION_TIMER_ID 100 /* 画面遷移時の遅延時間（ミリ秒） */
#define UPDATE_TRAP_COMMAND 0x08 /* サーバーが全クライアントへ罠設置情報を配信 */
#endif
