/*client_main_utf8.c*/
#include"common_utf8.h"
#include"client_func_utf8.h"
#include<stdio.h>
#include <stdlib.h> 
#include <SDL2/SDL.h> 

int main(int argc,char *argv[])
{
	int		num;
	char	name[MAX_CLIENTS][MAX_NAME_SIZE];
	int		endFlag=1;
	char	localHostName[]="localhost";
	char	*serverName;
	int		clientID;

	/* 引き数チェック */
	if(argc == 1){
		serverName = localHostName;
	}
	else if(argc == 2){
		serverName = argv[1];
	}
	else{
		fprintf(stderr, "Usage: %s [Server Name]. Cannot find a Server Name.\n", argv[0]);
		return -1;
	}
	/* サーバーとの接続 */
	if(SetUpClient(serverName,&clientID,&num,name)==-1){
		fprintf(stderr,"setup failed : SetUpClient\n");
		return -1;
	}	
	/* ウインドウの初期化 */
	if(InitWindows(clientID,num,name)==-1){
		fprintf(stderr,"setup failed : InitWindows\n");
		CloseSoc(); 
		return -1;
	}
	/* メインイベントループ */
	while(endFlag){
		SDL_PumpEvents(); 		
		WindowEvent(num);
		endFlag = SendRecvManager();

		if (endFlag) {
            DrawImageAndText();
        }
	};
	/* 終了処理 */
	DestroyWindow();
	CloseSoc();
	return 0;
}
