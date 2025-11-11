/*****************************************************************
ファイル名	: client_command_utf8.c
機能		: クライアントのコマンド処理
*****************************************************************/

#include"common_utf8.h"
#include"client_func_utf8.h"
#include"arpa/inet.h"

//追加部分
static int gMyClientID = -1;

void SetIntData2DataBlock(void *data,int intData,int *dataSize);
void SetCharData2DataBlock(void *data,char charData,int *dataSize);
static void RecvCircleData(void);
static void RecvRectangleData(void);
static void RecvDiamondData(void);
//追加部分
static void RecvResultData(void);
// 追加
void SetXPressedFlag(int clientID);

/*****************************************************************
関数名	: ExecuteCommand
機能	: サーバーから送られてきたコマンドを元に，
		  引き数を受信し，実行する
引数	: char	command		: コマンド
出力	: プログラム終了コマンドがおくられてきた時には0を返す．
		  それ以外は1を返す
*****************************************************************/
int ExecuteCommand(char command)
{
    int endFlag = 1;
#ifndef NDEBUG
    printf("#####\nExecuteCommand(): command = %c\n", command);
#endif

    switch(command){
        case END_COMMAND:
            endFlag = 0;
            break;

        case UPDATE_X_COMMAND: // 【追加】サーバーからのX押下更新コマンド
        {
            int senderID;
            RecvIntData(&senderID); // 4バイトの整数（ID）を受信
            printf("Client %d pressed X\n", senderID);

            SetXPressedFlag(senderID);
            break;
        }
        case NEXT_SCREEN_COMMAND: // 【追加】次の画面へ移行
        {
            printf("Received NEXT_SCREEN_COMMAND. Transitioning screen.\n");
            // 画面の状態を次の画面へ切り替える
            SetScreenState(SCREEN_STATE_GAME_SCREEN); 
            break;
        }
        default:
            break;
    }

    return endFlag;
}



/*****************************************************************
関数名	: SendEndCommand
機能	: プログラムの終了を知らせるために，
		  サーバーにデータを送る
引数	: なし
出力	: なし
*****************************************************************/
void SendEndCommand(void)
{
    unsigned char	data[MAX_DATA];
    int			dataSize;

#ifndef NDEBUG
    printf("#####\n");
    printf("SendEndCommand()\n");
#endif
    dataSize = 0;
    /* コマンドのセット */
    SetCharData2DataBlock(data,END_COMMAND,&dataSize);

    /* データの送信 */
    SendData(data,dataSize);
}


/*****************************************************************
関数名	: SetIntData2DataBlock
機能	: int 型のデータを送信用データの最後にセットする
引数	: void		*data		: 送信用データ
		  int		intData		: セットするデータ
		  int		*dataSize	: 送信用データの現在のサイズ
出力	: なし
*****************************************************************/
void SetIntData2DataBlock(void *data,int intData,int *dataSize)
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
void SetCharData2DataBlock(void *data,char charData,int *dataSize)
{
    /* 引き数チェック */
    assert(data!=NULL);
    assert(0<=(*dataSize));

    /* char 型のデータを送信用データの最後にコピーする */
    *(char *)(data + (*dataSize)) = charData;
    /* データサイズを増やす */
    (*dataSize) += sizeof(char);
}



/*****************************************************************
関数名	: RecvResultData
機能	: 結果を受信し，保存する
引数	: なし
出力	: なし
*****************************************************************/
static void RecvResultData(void)
{
    int numClients;
    char hands[MAX_CLIENTS];
    int i;
    /* クライアント数を受信する */
    RecvIntData(&numClients);

    printf("DEBUG: RecvResultData started. numClients=%d, myID=%d\n", numClients, gMyClientID);

    /* 各クライアントの手を受信する */
    for(i=0;i<numClients;i++){
        RecvCharData(&hands[i]);

         printf("DEBUG: Hand[%d] received: %c (ASCII: %d)\n", i, hands[i], hands[i]);
    }
   
}
    
/*****************************************************************
関数名 : SetMyClientID
機能  : 自身のクライアントIDをグローバル変数にセットする
引数  : int clientID : 自身のクライアントID
出力  : なし
*****************************************************************/
void SetMyClientID(int clientID)
{
    gMyClientID = clientID;
}
