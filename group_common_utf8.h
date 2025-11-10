/*****************************************************************
ファイル名	: group_common_utf8.h
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

#define END_COMMAND		'E'		  		/* プログラム終了コマンド */
#define CIRCLE_COMMAND	'C'				/* 円表示コマンド */
#define RECT_COMMAND	'R'				/* 四角表示コマンド */
#define DIAMOND_COMMAND	'D'				/* 菱形表示コマンド */

//追加部分
#define TYOKI_COMMAND 'T'         /* チョキ表示コマンド */
#define PA_COMMAND   'P'         /* パー表示コマンド */
#define GU_COMMAND   'G'         /* グー表示コマンド */

#define RESULT_COMMAND 'S'      /* 勝敗表示コマンド */

#endif
