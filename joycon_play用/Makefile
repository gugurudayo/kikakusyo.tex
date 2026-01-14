OBJS1   =       server_main.o server_net.o server_command.o
# OBJS2 には自分のプログラム（joyconlib.o）のみ含めます。
# 実装本体は LDFLAGS で外部ライブラリとしてリンクします。
OBJS2   =       client_main.o client_net.o client_command.o client_win.o joyconlib.o

TARGET1 =       server
TARGET2 =       client

CFLAGS  =       -c -DNDEBUG
# サンプルの指定に従い、システムライブラリをリンク
# -ljoyconlib: Joy-Con制御の本体
# -lhidapi-hidraw: デバイス通信用
LDFLAGS =       -ljoyconlib -lhidapi-hidraw -lm

.c.o:
	gcc $(CFLAGS) $(@:.o=_utf8.c) -o $@

all: $(TARGET1) $(TARGET2)

$(TARGET1):     $(OBJS1)
	gcc -o $(TARGET1) $(OBJS1) -lm -lSDL2

$(TARGET2):     $(OBJS2)
	gcc -o $(TARGET2) $(OBJS2) -lm -lSDL2 -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lSDL2_mixer $(LDFLAGS)

clean:
	rm -f *.o $(TARGET1) $(TARGET2)

# --- サーバー側 ---
server_main.o: server_main_utf8.c
	gcc $(CFLAGS) server_main_utf8.c -o server_main.o
server_net.o: server_net_utf8.c
	gcc $(CFLAGS) server_net_utf8.c -o server_net.o
server_command.o: server_command_utf8.c
	gcc $(CFLAGS) server_command_utf8.c -o server_command.o

# --- クライアント側 ---
client_main.o: client_main_utf8.c
	gcc $(CFLAGS) client_main_utf8.c -o client_main.o
client_net.o: client_net_utf8.c
	gcc $(CFLAGS) client_net_utf8.c -o client_net.o
client_command.o: client_command_utf8.c
	gcc $(CFLAGS) client_command_utf8.c -o client_command.o
client_win.o: client_win_utf8.c
	gcc $(CFLAGS) client_win_utf8.c -o client_win.o

# --- Joy-Con 処理 (自作部分) ---
joyconlib.o: joyconlib.c
	gcc $(CFLAGS) joyconlib.c -o joyconlib.o
