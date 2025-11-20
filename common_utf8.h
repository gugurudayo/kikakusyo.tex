/*****************************************************************
ファイル名	: common_utf8.h
機能		: サーバーとクライアントで使用する定数の宣言を行う
*****************************************************************/

#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<assert.h>
#include<math.h>

#define PORT			(u_short)8888	/* ポート番号 */

#define MAX_CLIENTS		4				/* クライアント数の最大値 */

#define MAX_NAME_SIZE	10 				/* ユーザー名の最大値*/

#define MAX_DATA		200				/* 送受信するデータの最大値 */

#define X_COMMAND        'X'   /* クライアントがXキーを押したことをサーバへ通知 */
#define UPDATE_X_COMMAND 'U'   /* サーバーが全クライアントへX押下情報を配信 */

#define END_COMMAND		'E'		  		/* プログラム終了コマンド */
#define NEXT_SCREEN_COMMAND 'N' /* 次の画面へ進むコマンド */
#define SELECT_WEAPON_COMMAND 'W' /* クライアントが武器を選択したことを通知 */
#define START_GAME_COMMAND 'S' /* サーバーがクライアントに武器選択画面へ移らせるコマンド */

#define TRANSITION_TIMER_ID 100 /* 画面遷移時の遅延時間（ミリ秒） */

#endif
