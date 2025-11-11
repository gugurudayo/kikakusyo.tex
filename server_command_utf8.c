/*****************************************************************
ファイル名	: server_command_utf8.c
機能		: サーバーのコマンド処理
*****************************************************************/

#include"server_common_utf8.h"
#include"server_func_utf8.h"
#include<arpa/inet.h>
#include<SDL2/SDL.h>

static Uint32 SendTransitionCommand(Uint32 interval, void *param);

//グローバル関数
static char gClientHands[MAX_CLIENTS] = {0}; // 各クライアントが出した手を記録
static int gHandsCount = 0; //手を出したクライアント数を記録
static int gXPressedClientFlags[MAX_CLIENTS] = {0}; //各クライアントのXコマンド受信フラグ
static int gXPressedCount = 0; //Xコマンドを受信したクライアント数
static SDL_TimerID gTransitionTimerID = 0; // 画面遷移用タイマーID

static void SetIntData2DataBlock(void *data,int intData,int *dataSize);
static void SetCharData2DataBlock(void *data,char charData,int *dataSize);
static int GetRandomInt(int n);
static void JudgeJanken(void);


/*****************************************************************
関数名	: ExecuteCommand
機能	: クライアントから送られてきたコマンドを元に，
		  引き数を受信し，実行する
引数	: char	command		: コマンド
		  int	pos			: コマンドを送ったクライアント番号
出力	: プログラム終了コマンドが送られてきた時には0を返す．
		  それ以外は1を返す
*****************************************************************/
int ExecuteCommand(char command,int pos)
{
    unsigned char	data[MAX_DATA];
    int			dataSize,intData;
    int			endFlag = 1;

    /* 引き数チェック */
    assert(0<=pos && pos<MAX_CLIENTS);

#ifndef NOBEBUG
    printf("#####\n");
    printf("ExecuteCommand()\n");
    printf("Get command %c\n",command);
#endif
    switch(command){
	    case END_COMMAND:
			dataSize = 0;
			/* コマンドのセット */
			SetCharData2DataBlock(data,command,&dataSize);

			/* 全ユーザーに送る */
			SendData(ALL_CLIENTS,data,dataSize);

			endFlag = 0;
			break;
        
        case X_COMMAND:
        {
            int senderID;
            RecvIntData(pos, &senderID);

            if (gXPressedClientFlags[senderID] == 0) {
                gXPressedClientFlags[senderID] = 1;
                gXPressedCount++;
            }

            // X押下通知を全クライアントにブロードキャスト
            unsigned char data[MAX_DATA];
            int dataSize = 0;
            SetCharData2DataBlock(data, UPDATE_X_COMMAND, &dataSize);
            SetIntData2DataBlock(data, senderID, &dataSize);
            SendData(ALL_CLIENTS, data, dataSize);

            // 3. 全員が押したか判定
            if (gXPressedCount == GetClientNum()) {
                // 【修正】全員がXを押した！ -> 3秒後に遷移コマンドを送るタイマーをセット
                printf("All clients pressed X. Starting 3 second transition timer.\n");

                // タイマーがセットされていない場合のみセット
                if (gTransitionTimerID == 0) {
                    // 3000ms = 3秒
                    gTransitionTimerID = SDL_AddTimer(3000, SendTransitionCommand, NULL); 
                }
            }
            break;
        }

        default:
            /* 未知のコマンドが送られてきた */
            fprintf(stderr,"0x%02x is not command!\n",command);
    }
    return endFlag;
}

/*****************************************************************
関数名	: SetIntData2DataBlock
機能	: int 型のデータを送信用データの最後にセットする
引数	: void		*data		: 送信用データ
		  int		intData		: セットするデータ
		  int		*dataSize	: 送信用データの現在のサイズ
出力	: なし
*****************************************************************/
static void SetIntData2DataBlock(void *data,int intData,int *dataSize)
{
    int tmp;

    /* 引き数チェック */
    assert(data!=NULL);
    assert(0<=(*dataSize));

    tmp = htonl(intData);

    /* int 型のデータを送信用データの最後にコピーする */
    memcpy(data + (*dataSize),&tmp,sizeof(int));
    /* データサイズを増やす */
    (*dataSize) += sizeof(int);
}

/*****************************************************************
関数名	: SetCharData2DataBlock
機能	: char 型のデータを送信用データの最後にセットする
引数	: void		*data		: 送信用データ
		  int		intData		: セットするデータ
		  int		*dataSize	: 送信用データの現在のサイズ
出力	: なし
*****************************************************************/
static void SetCharData2DataBlock(void *data,char charData,int *dataSize)
{
    /* 引き数チェック */
    assert(data!=NULL);
    assert(0<=(*dataSize));

    /* int 型のデータを送信用データの最後にコピーする */
    *(char *)(data + (*dataSize)) = charData;
    /* データサイズを増やす */
    (*dataSize) += sizeof(char);
}

/*****************************************************************
関数名	: GetRandomInt
機能	: 整数の乱数を得る
引数	: int		n	: 乱数の最大値
出力	: 乱数値
*****************************************************************/
static int GetRandomInt(int n)
{
    return rand()%n;
}

void SendDataToClient(int clientID, const void *data, int dataSize)
{
    assert(clientID >= 0 && clientID < GetClientNum());
    int sock = gClients[clientID].fd; // gClients が外部から参照可能になった
    int sentBytes = send(sock, data, dataSize, 0);
    if(sentBytes != dataSize){
        fprintf(stderr, "Warning: sentBytes != dataSize for client %d\n", clientID);
    }
}

/*****************************************************************
関数名  : SendTransitionCommand
機能    : 3秒後に実行され、画面遷移コマンドをブロードキャストする
引数    : Uint32  interval : タイマー間隔
          void      *param   : ユーザーデータ (未使用)
出力    : タイマーの次の間隔 (今回は1回限りなので 0)
*****************************************************************/
static Uint32 SendTransitionCommand(Uint32 interval, void *param)
{
    unsigned char data[MAX_DATA];
    int dataSize = 0;

    printf("3 seconds elapsed. Sending NEXT_SCREEN_COMMAND.\n");

    // NEXT_SCREEN_COMMAND をブロードキャスト
    SetCharData2DataBlock(data, NEXT_SCREEN_COMMAND, &dataSize);
    SendData(ALL_CLIENTS, data, dataSize);
    
    gTransitionTimerID = 0; // タイマー完了フラグをリセット

    return 0; // 0 を返すとタイマーは一度で終了する
}