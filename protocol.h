// protocol.h
// server와 client가 통신할때 사용할 data structure
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define BOARD_SIZE    15
#define MAX_NAME_LEN  32
#define MAX_RECORDS   50 // 전적 보기에 표시할 최대 플레이어 수

// 돌의 종류
#define EMPTY         0
#define BLACK         1
#define WHITE         2

// 전적 정보 구조체
typedef struct {
    char name[MAX_NAME_LEN];
    int wins;
    int losses;
} PlayerRecord;

// 서버-클라이언트 간 통신 패킷
typedef struct {
    int type;
    int row;
    int col;
    int player;
    char message[1024];
    int board[BOARD_SIZE][BOARD_SIZE];
    char name[MAX_NAME_LEN];
    
    // 전적 전송을 위한 데이터
    int record_count;
    PlayerRecord records[MAX_RECORDS];

} GamePacket;

// 패킷 타입
#define REQ_NAME          1 // 클라이언트 -> 서버: 이름 전송
#define REQ_MOVE          2 // 클라이언트 -> 서버: 돌 놓기 요청
#define REQ_REMATCH       3 // 클라이언트 -> 서버: 재시작 요청
#define REQ_RECORDS       4 // 클라이언트 -> 서버: 전적 요청
#define REQ_QUIT          5 // 클라이언트 -> 서버: 게임 포기 요청

#define RES_WAIT          10 // 서버 -> 클라이언트: 상대방 대기
#define RES_START         11 // 서버 -> 클라이언트: 게임 시작
#define RES_UPDATE        12 // 서버 -> 클라이언트: 게임 상태 업데이트
#define RES_INVALID       13 // 서버 -> 클라이언트: 잘못된 수
#define RES_GAME_OVER     14 // 서버 -> 클라이언트: 게임 종료
#define INFO_REMATCH      15 // 서버 -> 클라이언트: 상대가 재시작 원함
#define RES_RECORDS       16 // 서버 -> 클라이언트: 전적 정보 응답
#define INFO_OPP_QUIT     17 // 서버 -> 클라이언트: 상대가 나감

#endif
