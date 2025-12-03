/*server_main_utf8.c*/

#include<SDL2/SDL.h>
#include"server_common_utf8.h"
#include"server_func_utf8.h"

static Uint32 SignalHandler(Uint32 interval, void *param);

int main(int argc,char *argv[])
{
	int	num;
	int	endFlag = 1;

	/* 引き数チェック */
	if(argc != 2){
		fprintf(stderr,"Usage: number of clients\n");
		exit(-1);
	}
	if((num = atoi(argv[1])) < 0 ||  num > MAX_CLIENTS){
		fprintf(stderr,"clients limit = %d \n",MAX_CLIENTS);
		exit(-1);
	}
	
	/* SDLの初期化 */
	if(SDL_Init(SDL_INIT_TIMER) < 0) {
		printf("failed to initialize SDL.\n");
		exit(-1);
	}

	/* クライアントとの接続 */
	if(SetUpServer(num) == -1){
		fprintf(stderr,"Cannot setup server\n");
		exit(-1);
	}
	
	/* 割り込み処理のセット */
	//SDL_AddTimer(5000,SignalHandler,NULL);
	
	/* メインイベントループ */
	while(endFlag){
		endFlag = SendRecvManager();
	};

	/* 終了処理 */
	Ending();

	return 0;
}
static Uint32 SignalHandler(Uint32 interval, void *param)
{
	return interval;
}
