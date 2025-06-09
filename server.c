// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "protocol.h"

// 전적을 저장할 파일
#define RECORD_FILE "gomoku_records.dat"

// 전역변수
int board[BOARD_SIZE][BOARD_SIZE];
int client_sockets[2];
char player_names[2][MAX_NAME_LEN];
int current_player;
pthread_mutex_t game_mutex;

// 게임 진행 함수
int  check_horizontal_win(int player_stone, int row, int col);
int  check_vertical_win(int player_stone, int row, int col);
int  check_diagonal_win1(int player_stone, int row, int col);
int  check_diagonal_win2(int player_stone, int row, int col);
int  check_win(int player_stone, int row, int col);
int is_on_board(int row, int col);
int get_stone_at(int row, int col);
int is_open_three(int player_stone, int row, int col, int dr, int dc);
int check_33(int player_stone, int row, int col);
int is_open_four(int player_stone, int row, int col, int dr, int dc);
int check_44(int player_stone, int row, int col);
int check_six(int player_stone, int row, int col);

// 전적 관리 함수
int load_all_records(PlayerRecord *out_records);
void save_all_records(PlayerRecord *recs, int cnt);
PlayerRecord* get_or_create_record(PlayerRecord *recs, int *cnt, const char *name);

// server-client 간 통신
void broadcast(GamePacket* packet);
void* handle_client_session(void* arg);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <port>\n", argv[0]);
        exit(1);
    }

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    pthread_t session_thread;

    pthread_mutex_init(&game_mutex, NULL);

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind() error");
        exit(1);
    }
    if (listen(serv_sock, 2) == -1) {
        perror("listen() error");
        exit(1);
    }

    printf("오목 서버가 시작되었습니다. 플레이어의 접속을 기다립니다...\n");

    // 2명의 클라이언트 접속 대기, 수락
    for (int i = 0; i < 2; i++) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        client_sockets[i] = clnt_sock;

        GamePacket name_packet;
        recv(clnt_sock, &name_packet, sizeof(GamePacket), 0);
        strcpy(player_names[i], name_packet.name);
        
        printf("플레이어 '%s' 접속 (%d/2)\n", player_names[i], i + 1);

        // 첫 번째 클라이언트에게는 대기 메세지 전송
        if (i == 0) {
            GamePacket wait_packet;
            wait_packet.type = RES_WAIT;
            snprintf(wait_packet.message, sizeof(wait_packet.message), "상대방 플레이어를 기다리고 있습니다...");
            send(clnt_sock, &wait_packet, sizeof(GamePacket), 0);
        }
    }

    // 게임 세션 스레드 생성, 실행
    printf("모든 플레이어가 접속했습니다. 게임을 시작합니다.\n");
    pthread_create(&session_thread, NULL, handle_client_session, NULL);
    pthread_join(session_thread, NULL);

    close(client_sockets[0]);
    close(client_sockets[1]);
    close(serv_sock);
    pthread_mutex_destroy(&game_mutex);
    printf("세션이 종료되었습니다. 서버를 닫습니다.\n");
    return 0;
}


// 전적 관리 함수
int load_all_records(PlayerRecord *out_records) {
    FILE *f = fopen(RECORD_FILE, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    int cnt = sz / sizeof(PlayerRecord);
    if (cnt > MAX_RECORDS) cnt = MAX_RECORDS;
    rewind(f);
    fread(out_records, sizeof(PlayerRecord), cnt, f);
    fclose(f);
    return cnt;
}

void save_all_records(PlayerRecord *recs, int cnt) {
    FILE *f = fopen(RECORD_FILE, "wb");
    if (!f) return;
    fwrite(recs, sizeof(PlayerRecord), cnt, f);
    fclose(f);
}

PlayerRecord* get_or_create_record(PlayerRecord *recs, int *cnt, const char *name) {
    for (int i = 0; i < *cnt; i++) {
        if (strcmp(recs[i].name, name) == 0) {
            return &recs[i];
        }
    }
    if (*cnt >= MAX_RECORDS) return NULL; // 최대치 초과
    PlayerRecord *r = &recs[*cnt];
    strncpy(r->name, name, MAX_NAME_LEN);
    r->wins = 0;
    r->losses = 0;
    (*cnt)++;
    return r;
}

// 게임 진행 함수

// 가로 승리 검사
int check_horizontal_win(int player_stone, int row, int col) {
    for (int start_col = col - 4; start_col <= col; start_col++) {
        if (start_col >= 0 && start_col + 4 < BOARD_SIZE) {
            int cnt = 0;
            for (int k = 0; k < 5; k++) {
                if (board[row][start_col + k] == player_stone) cnt++;
                else break;
            }
            if (cnt == 5) return 1;
        }
    }
    return 0;
}

// 세로 승리 검사
int check_vertical_win(int player_stone, int row, int col) {
    for (int start_row = row - 4; start_row <= row; start_row++) {
        if (start_row >= 0 && start_row + 4 < BOARD_SIZE) {
            int cnt = 0;
            for (int k = 0; k < 5; k++) {
                if (board[start_row + k][col] == player_stone) cnt++;
                else break;
            }
            if (cnt == 5) return 1;
        }
    }
    return 0;
}

// 대각선 (\) 승리 검사
int check_diagonal_win1(int player_stone, int row, int col) {
    for (int i = 0; i < 5; i++) {
        int sr = row - i;
        int sc = col - i;
        if (sr >= 0 && sc >= 0 && sr + 4 < BOARD_SIZE && sc + 4 < BOARD_SIZE) {
            int cnt = 0;
            for (int j = 0; j < 5; j++) {
                if (board[sr + j][sc + j] == player_stone) cnt++;
                else break;
            }
            if (cnt == 5) return 1;
        }
    }
    return 0;
}

// 대각선 (/) 승리 검사
int check_diagonal_win2(int player_stone, int row, int col) {
    for (int i = 0; i < 5; i++) {
        int sr = row - i;
        int sc = col + i;
        if (sr >= 0 && sc < BOARD_SIZE && sr + 4 < BOARD_SIZE && sc - 4 >= 0) {
            int cnt = 0;
            for (int j = 0; j < 5; j++) {
                if (board[sr + j][sc - j] == player_stone) cnt++;
                else break;
            }
            if (cnt == 5) return 1;
        }
    }
    return 0;
}

// 모든 방향 승리 검사
int check_win(int player_stone, int row, int col) {
    if (check_horizontal_win(player_stone, row, col)) return 1;
    if (check_vertical_win(player_stone, row, col)) return 1;
    if (check_diagonal_win1(player_stone, row, col)) return 1;
    if (check_diagonal_win2(player_stone, row, col)) return 1;
    return 0;
}

// 보드 범위 체크
int is_on_board(int row, int col) {
    return (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE);
}

// 보드에서 돌 가져오기
int get_stone_at(int row, int col) {
    if (!is_on_board(row, col)) return -1;
    return board[row][col];
}

// (row, col)에 놓인 돌을 기준으로, (dr, dc) 방향에서 
// '놓인 돌이 열린 3목의 왼쪽/중간/오른쪽 케이스'를 이루는지 확인.
//  즉, 세 칸 연속 내 돌 + 양끝이 EMPTY인지 검사.
//  조건을 만족하면 1, 아니면 0 반환.
int is_open_three(int player_stone, int row, int col, int dr, int dc) {
    // 1) 놓인 돌이 열린 3목의 '가장 왼쪽 돌' 케이스
    //    [ (r, c) , (r+dr, c+dc) , (r+2dr, c+2dc) ]가 player_stone 이고,
    //    (r−dr, c−dc)와 (r+3dr, c+3dc) 가 EMPTY 인지 확인
    if (get_stone_at(row, col) == player_stone &&
        get_stone_at(row + dr, col + dc) == player_stone &&
        get_stone_at(row + 2*dr, col + 2*dc) == player_stone &&
        get_stone_at(row - dr, col - dc) == EMPTY &&
        get_stone_at(row + 3*dr, col + 3*dc) == EMPTY) {
        return 1; 
    }

    // 2) 놓인 돌이 열린 3목의 '중간 돌' 케이스
    //    [ (r−dr, c−dc) , (r, c) , (r+dr, c+dc) ]가 player_stone 이고,
    //    (r−2dr, c−2dc)와 (r+2dr, c+2dc) 가 EMPTY 인지 확인
    if (get_stone_at(row - dr, col - dc) == player_stone &&
        get_stone_at(row, col) == player_stone &&
        get_stone_at(row + dr, col + dc) == player_stone &&
        get_stone_at(row - 2*dr, col - 2*dc) == EMPTY &&
        get_stone_at(row + 2*dr, col + 2*dc) == EMPTY) {
        return 1; 
    }

    // 3) 놓인 돌이 열린 3목의 '가장 오른쪽 돌' 케이스
    //    [ (r−2dr, c−2dc) , (r−dr, c−dc) , (r, c) ]가 player_stone 이고,
    //    (r−3dr, c−3dc)와 (r+dr, c+dc) 가 EMPTY 인지 확인
    if (get_stone_at(row - 2*dr, col - 2*dc) == player_stone &&
        get_stone_at(row - dr, col - dc) == player_stone &&
        get_stone_at(row, col) == player_stone &&
        get_stone_at(row - 3*dr, col - 3*dc) == EMPTY &&
        get_stone_at(row + dr, col + dc) == EMPTY) {
        return 1; 
    }

    return 0;
}

// (row, col)에 player_stone을 놓았을 때,
// 1) 직접 열린 3목이 두 개 이상 생기는지, 또는
// 2) 한 방향에서 열린 3목이 정확히 하나 생겼고, 
// 그 열린 3목을 이루는 '다른 두 돌' 중 적어도 하나가 
// 다른 방향으로 또 열린 3목을 이루는지
// 위 두 가지 중 하나라도 참이면 금수(1), 아니면 정상(0).
int check_33(int player_stone, int row, int col) {
    // 1) (row,col)에 돌을 둔다
    board[row][col] = player_stone;

    int open_33_cnt = 0;
    // 4방향: → (0,1), ↓ (1,0), ↘ (1,1), ↗ (-1,1)
    int dr[4] = { 0, 1, 1, -1 };
    int dc[4] = { 1, 0, 1,  1 };

    // 만약 “두 개 이상의 열린 3목”이 바로 생긴다면 즉시 금수
    // 또는 “한 방향에서만 열린 3목”이라도, 그 케이스의 나머지 두 돌 중 하나라도 다른 방향으로 열린 3목이 있으면 금수
    for (int i = 0; i < 4; i++) {
        // 1) 놓은 돌 (row,col) 자신이 이 방향(i)에서 열린 3목을 이루는가?
        int case_open = is_open_three(player_stone, row, col, dr[i], dc[i]);
        if (!case_open) {
            continue;
        }

        // 이 지점부터 “놓은 돌이 i방향으로 열린 3목을 하나 만들어냈다”는 뜻
        open_33_cnt++;
        if (open_33_cnt >= 2) {
            // 이미 두 개 이상의 열린 3목을 만들었으므로 금수
            board[row][col] = EMPTY;
            return 1;
        }

        int r1, c1, r2, c2;

        // (1) ‘가장 왼쪽 돌’ 케이스
        if (get_stone_at(row, col) == player_stone &&
            get_stone_at(row + dr[i], col + dc[i]) == player_stone &&
            get_stone_at(row + 2*dr[i], col + 2*dc[i]) == player_stone &&
            get_stone_at(row - dr[i], col - dc[i]) == EMPTY &&
            get_stone_at(row + 3*dr[i], col + 3*dc[i]) == EMPTY) {
            r1 = row + dr[i];        c1 = col + dc[i];
            r2 = row + 2*dr[i];     c2 = col + 2*dc[i];
        }
        // (2) ‘중간 돌’ 케이스
        else if (get_stone_at(row - dr[i], col - dc[i]) == player_stone &&
                 get_stone_at(row, col) == player_stone &&
                 get_stone_at(row + dr[i], col + dc[i]) == player_stone &&
                 get_stone_at(row - 2*dr[i], col - 2*dc[i]) == EMPTY &&
                 get_stone_at(row + 2*dr[i], col + 2*dc[i]) == EMPTY) {
            r1 = row - dr[i];        c1 = col - dc[i];
            r2 = row + dr[i];        c2 = col + dc[i];
        }
        // (3) ‘가장 오른쪽 돌’ 케이스
        else {
            // is_open_three가 이미 참이므로 아래 if 문은 무조건 맞을 것이다.
            r1 = row - 2*dr[i];      c1 = col - 2*dc[i];
            r2 = row - dr[i];        c2 = col - dc[i];
        }

        // r1,c1 와 r2,c2 두 돌 각각이 다른 방향에서 또 열린 3목을 만드는지 확인
        for (int j = 0; j < 4; j++) {
            // (r1,c1)에 대해서
			if(j==i) continue;
            if (is_open_three(player_stone, r1, c1, dr[j], dc[j])) {
                open_33_cnt++;
                break;  // r1의 열린 3목은 한 번만 세면 되기 때문에 루프 탈출
            }
        }
        if (open_33_cnt >= 2) {
            board[row][col] = EMPTY;
            return 1;
        }

        for (int j = 0; j < 4; j++) {
            // (r2,c2)에 대해서
			if(j==i) continue;
            if (is_open_three(player_stone, r2, c2, dr[j], dc[j])) {
                open_33_cnt++;
                break;  // r2의 열린 3목도 한 번만 세면 된다
            }
        }
        if (open_33_cnt >= 2) {
            board[row][col] = EMPTY;
            return 1;
        }
    }

    // 2) 여태까지 열린 3목이 2개 이상 안 생겼으면 금수 아님
    board[row][col] = EMPTY;
    return 0;
}

// 열린 4 또는 끊긴 4 확인 함수
int is_open_four(int player_stone, int row, int col, int dr, int dc) {
    // 다양한 5칸 패턴 슬라이딩 검사
    for (int offset = -4; offset <= 0; offset++) {
        int count = 0, empty_count = 0;
        int stones[5];

        for (int i = 0; i < 5; i++) {
            int r = row + (offset + i) * dr;
            int c = col + (offset + i) * dc;
            stones[i] = get_stone_at(r, c);
        }

        // 5칸 중 플레이어 돌이 4개, EMPTY가 1개 → 끊긴 4 or 열린 4
        for (int i = 0; i < 5; i++) {
            if (stones[i] == player_stone) count++;
            else if (stones[i] == EMPTY) empty_count++;
        }

        if (count == 4 && empty_count == 1) {
            // 열린 4인지 확인 (양쪽 바깥칸도 비어 있어야)
            int r_before = row + (offset - 1) * dr;
            int c_before = col + (offset - 1) * dc;
            int r_after  = row + (offset + 5) * dr;
            int c_after  = col + (offset + 5) * dc;

            int before = get_stone_at(r_before, c_before);
            int after  = get_stone_at(r_after, c_after);

            if (before == EMPTY || after == EMPTY) {
                return 1;  // 열린 4 또는 끊긴 4로 간주
            }
        }
    }
    return 0;
}

// 4-4 금수 검사 함수
int check_44(int player_stone, int row, int col) {
    board[row][col] = player_stone;
	
	int dr[4] = {0, 1, 1, -1};
	int dc[4] = {1, 0, 1, 1};

    int open_44_cnt = 0;
    for (int i = 0; i < 4; i++) {
        if (is_open_four(player_stone, row, col, dr[i], dc[i])) {
            open_44_cnt++;
        }
        if (open_44_cnt >= 2) {
            board[row][col] = EMPTY;
            return 1;  // 금수
        }
    }

    board[row][col] = EMPTY;
    return 0;  // 금수 아님
}

// 6목 이상(장목)이 생성되었는지 검사하는 함수
// player_stone: 현재 착수한 플레이어의 돌
// row, col: 방금 놓은 돌의 위치
int check_six(int player_stone, int row, int col) {
    board[row][col] = BLACK;
	// 가로 방향 검사
    for (int start_col = col - 5; start_col <= col; start_col++) { // 6개 연속이므로 -5 ~ 0까지
        if (start_col >= 0 && start_col + 5 < BOARD_SIZE) { // 6번째 돌까지 보드 안에 있는지 확인
            int count = 0;
            for (int k = 0; k < 6; k++) { // 6개 연속 검사
                if (board[row][start_col + k] == player_stone) {
                    count++;
                } else {
                    break;
                }
            }
            if (count == 6) { // 6개 연속된 돌 발견
                return 1;
            }
        }
    }

    // 세로 방향 검사
    for (int start_row = row - 5; start_row <= row; start_row++) {
        if (start_row >= 0 && start_row + 5 < BOARD_SIZE) {
            int count = 0;
            for (int k = 0; k < 6; k++) {
                if (board[start_row + k][col] == player_stone) {
                    count++;
                } else {
                    break;
                }
            }
            if (count == 6) {
                return 1;
            }
        }
    }

    // 대각선 방향 (\) 검사
    for (int i = 0; i < 6; i++) {
        int start_row = row - i;
        int start_col = col - i;
        if (start_row >= 0 && start_col >= 0 && start_row + 5 < BOARD_SIZE && start_col + 5 < BOARD_SIZE) {
            int count = 0;
            for (int j = 0; j < 6; j++) {
                if (board[start_row + j][start_col + j] == player_stone) {
                    count++;
                } else {
                    break;
                }
            }
            if (count == 6) {
                return 1;
            }
        }
    }

    // 대각선 방향 (/) 검사
    for (int i = 0; i < 6; i++) {
        int start_row = row - i;
        int start_col = col + i;
        if (start_row >= 0 && start_col < BOARD_SIZE && start_row + 5 < BOARD_SIZE && start_col - 5 >= 0) {
            int count = 0;
            for (int j = 0; j < 6; j++) {
                if (board[start_row + j][start_col - j] == player_stone) {
                    count++;
                } else {
                    break;
                }
            }
            if (count == 6) {
                return 1;
            }
        }
    }

	board[row][col] = EMPTY;

    return 0; // 6목 이상이 발견되지 않음
}

// server-client간 통신
void broadcast(GamePacket* packet) {
    pthread_mutex_lock(&game_mutex);
    for (int i = 0; i < 2; i++) {
        if(client_sockets[i] != -1)
            send(client_sockets[i], packet, sizeof(GamePacket), 0);
    }
    pthread_mutex_unlock(&game_mutex);
}

void* handle_client_session(void* arg) {
    int rematch_votes[2] = {0, 0};

    while(1) { // 전체 세션 루프 (재시작을 위함)
        // 게임 초기화
        memset(board, EMPTY, sizeof(board));    // 보드 초기화
        current_player = BLACK;
        rematch_votes[0] = 0;
        rematch_votes[1] = 0;
        int game_over_flag = 0;
        
        GamePacket packet;
        packet.type = RES_START;
        memcpy(packet.board, board, sizeof(board));

        // 재시작 시 흑/백 전환
        if (arg != NULL) { // arg가 NULL이 아니면 재시작으로 간주
            char temp_name[MAX_NAME_LEN];
            strcpy(temp_name, player_names[0]);
            strcpy(player_names[0], player_names[1]);
            strcpy(player_names[1], temp_name);
            int temp_sock = client_sockets[0];
            client_sockets[0] = client_sockets[1];
            client_sockets[1] = temp_sock;
        }

        // 각 클라이언트에게 게임 시작 안내 메세지 전송
        snprintf(packet.message, sizeof(packet.message), "게임 시작! 당신은 흑돌입니다. %s님의 차례.", player_names[0]);
        send(client_sockets[0], &packet, sizeof(GamePacket), 0);
        snprintf(packet.message, sizeof(packet.message), "게임 시작! 당신은 백돌입니다. %s님의 차례를 기다리세요.", player_names[0]);
        send(client_sockets[1], &packet, sizeof(GamePacket), 0);
        
        // 게임 진행 루프
        while (!game_over_flag) {
            GamePacket req_packet;
            int player_idx = (current_player == BLACK) ? 0 : 1;
            int opponent_idx = 1 - player_idx;
            
            // 현재 턴인 클라이언트에게 요청을 수신받는다
            int bytes_read = recv(client_sockets[player_idx], &req_packet, sizeof(GamePacket), 0);
            if (bytes_read <= 0) { // 클라이언트 연결 끊김
                game_over_flag = 1;
                packet.type = INFO_OPP_QUIT;
                snprintf(packet.message, sizeof(packet.message), "상대 %s가 나갔습니다. 세션을 종료합니다.", player_names[player_idx]);
                if(client_sockets[opponent_idx] != -1)
                    send(client_sockets[opponent_idx], &packet, sizeof(GamePacket), 0);
                client_sockets[player_idx] = -1; // 소켓 비활성화
                break; // 게임 진행 루프 탈출
            }

            // 요청 타입에 따라 처리

            // 게임 포기 요청 처리
            if (req_packet.type == REQ_QUIT) {
                game_over_flag = 1;
                packet.type = RES_GAME_OVER;
                snprintf(packet.message, sizeof(packet.message), "%s님이 게임을 포기했습니다. %s님의 승리!", player_names[player_idx], player_names[opponent_idx]);
                
                // 전적 기록 (포기)
                PlayerRecord records[MAX_RECORDS];
                int count = load_all_records(records);
                PlayerRecord* winner_rec = get_or_create_record(records, &count, player_names[opponent_idx]);
                PlayerRecord* loser_rec = get_or_create_record(records, &count, player_names[player_idx]);
                if(winner_rec) winner_rec->wins++;
                if(loser_rec) loser_rec->losses++;
                save_all_records(records, count);
                
                broadcast(&packet);
                break;
            }

            if (req_packet.type == REQ_MOVE) {
                int r = req_packet.row;
                int c = req_packet.col;
                char msg[256];

                if (!is_on_board(r, c) || board[r][c] != EMPTY) {
                    snprintf(msg, sizeof(msg), "둘 수 없는 위치입니다. 다시 두세요.");
                    packet.type = RES_INVALID;
					strncpy(packet.message, msg, sizeof(packet.message)-1);
                    send(client_sockets[player_idx], &packet, sizeof(GamePacket), 0);
                    continue;
                } else if (current_player == BLACK && check_six(current_player, r, c)) {
                    game_over_flag = 1;
                    snprintf(msg, sizeof(msg), "6목 금수! %s의 패배. %s의 승리!", player_names[0], player_names[1]);
                    packet.type = RES_GAME_OVER;
                } else if (current_player == BLACK && check_33(BLACK, r, c)) {
                    snprintf(msg, sizeof(msg), "3-3 금수! 둘 수 없는 위치입니다. 다시 두세요.");
                    packet.type = RES_INVALID;
                    strncpy(packet.message, msg, sizeof(packet.message) - 1);
                    send(client_sockets[player_idx], &packet, sizeof(GamePacket), 0);
                    continue;
                } else if (current_player == BLACK && check_44(BLACK, r, c)){
                    snprintf(msg, sizeof(msg), "4-4 금수! 둘 수 없는 위치입니다. 다시 두세요.");
                    packet.type = RES_INVALID;
                    strncpy(packet.message, msg, sizeof(packet.message) - 1);
                    send(client_sockets[player_idx], &packet, sizeof(GamePacket), 0);
                    continue;
                } else {
                    board[r][c] = current_player;
                    if (check_win(current_player, r, c)) {
                        game_over_flag = 1;
                        snprintf(msg, sizeof(msg), "게임 종료! %s의 승리! (r:재시작 p:전적보기 q:종료)", player_names[player_idx]);
                        packet.type = RES_GAME_OVER;

                        // 전적 기록
                        PlayerRecord records[MAX_RECORDS];
                        int count = load_all_records(records);
                        PlayerRecord* winner_rec = get_or_create_record(records, &count, player_names[player_idx]);
                        PlayerRecord* loser_rec = get_or_create_record(records, &count, player_names[1 - player_idx]);
                        if(winner_rec) winner_rec->wins++;
                        if(loser_rec) loser_rec->losses++;
                        save_all_records(records, count);

                    } else {
                        current_player = (current_player == BLACK) ? WHITE : BLACK;
                        snprintf(msg, sizeof(msg), "%s님의 차례입니다.", player_names[(current_player == BLACK) ? 0 : 1]);
                        packet.type = RES_UPDATE;
                    }
                }
                
                strcpy(packet.message, msg);
                memcpy(packet.board, board, sizeof(board));
                broadcast(&packet);
            }
        } // while(!game_over_flag)

        if(client_sockets[0] == -1 || client_sockets[1] == -1) break; // 한명이라도 나가면 세션 종료
        
        // 게임 종료 후 처리 루프 (재시작/전적 요청)
        while(1) {
            int all_voted = (rematch_votes[0] && rematch_votes[1]);
            if(all_voted) break; // 재시작

            int any_socket_closed = (client_sockets[0] == -1 || client_sockets[1] == -1);
            if(any_socket_closed) break; // 한명이라도 나가면 세션 종료

            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(client_sockets[0], &read_fds);
            FD_SET(client_sockets[1], &read_fds);
            
            int max_fd = client_sockets[0] > client_sockets[1] ? client_sockets[0] : client_sockets[1];
            select(max_fd + 1, &read_fds, NULL, NULL, NULL);

            for(int i=0; i<2; i++) {
                if(FD_ISSET(client_sockets[i], &read_fds)) {
                    GamePacket req_packet;
                    if(recv(client_sockets[i], &req_packet, sizeof(GamePacket), 0) <= 0) {
                        client_sockets[i] = -1;
                        packet.type = INFO_OPP_QUIT;
                        snprintf(packet.message, sizeof(packet.message), "상대 %s가 나갔습니다. 세션을 종료합니다.", player_names[i]);
                        if(client_sockets[1-i] != -1) send(client_sockets[1-i], &packet, sizeof(GamePacket),0);
                        break;
                    }
                    
                    if(req_packet.type == REQ_REMATCH) {
                        rematch_votes[i] = 1;
                        packet.type = INFO_REMATCH;
                        snprintf(packet.message, sizeof(packet.message), "상대(%s)가 재시작을 원합니다.", player_names[i]);
                        send(client_sockets[1-i], &packet, sizeof(GamePacket), 0);
                    }
                    else if(req_packet.type == REQ_RECORDS) {
                        packet.type = RES_RECORDS;
                        packet.record_count = load_all_records(packet.records);
                        snprintf(packet.message, sizeof(packet.message), "전적 보기 (p: 돌아가기)");
                        send(client_sockets[i], &packet, sizeof(GamePacket), 0);
                    }
                }
            }
        }
        if(rematch_votes[0] != 1 || rematch_votes[1] != 1) break; // 재시작 투표가 완료되지 않으면 세션 종료
    } // while(1) for session
    
    return NULL;
}
