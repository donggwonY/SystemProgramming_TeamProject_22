# 컴파일러 설정
CC = gcc
CFLAGS = -Wall -Wextra -I.
LDLIBS = -lpthread -lncurses

# 실행 파일 이름
CLIENT = client
SERVER = server

# 클라이언트 소스 및 오브젝트
CLIENT_SRCS = client.c
CLIENT_OBJS = client.o

# 서버 소스 및 오브젝트 및 오브젝트
SERVER_SRCS = server.c
SERVER_OBJS = server.o

.PHONY: all clean

all: $(CLIENT) $(SERVER)

# 클라이언트 빌드 (ncurses, pthread 링크)
$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS) $(LDLIBS)

# 서버 빌드 (pthread 링크)
$(SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS) -lpthread

# 오브젝트 생성 규칙
%.o: %.c protocol.h
	$(CC) $(CFLAGS) -c $<

# 빌드 결과물 삭제
clean:
	rm -f $(CLIENT) $(SERVER) *.o
