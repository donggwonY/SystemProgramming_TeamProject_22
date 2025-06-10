// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <ncurses.h>
#include <locale.h>
#include "protocol.h"

// 클라이언트의 상태
typedef enum {
    PLAYING,
    POST_GAME,
    VIEWING_RECORDS
} ClientState;

int board[BOARD_SIZE][BOARD_SIZE];
int current_row, current_col;
char status_message[1024] = "서버에 접속 중...";
ClientState client_state = PLAYING;
pthread_mutex_t ncurses_mutex;
char ban_message[256] = ""; // 금수 안내 메시지용 전역 변수

int my_color = -1; // 내가 흑돌인지(BLACK) 백돌인지(WHITE) 저장
int current_turn = -1; // 현재 누구의 턴인지 저장 (BLACK 또는 WHITE)

void draw_game_board() {
    clear();
    // 테두리 및 교차점 그리기
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (i == 0 && j == 0) mvaddch(i, j * 2, ACS_ULCORNER);
            else if (i == 0 && j == BOARD_SIZE - 1) mvaddch(i, j * 2, ACS_URCORNER);
            else if (i == BOARD_SIZE - 1 && j == 0) mvaddch(i, j * 2, ACS_LLCORNER);
            else if (i == BOARD_SIZE - 1 && j == BOARD_SIZE - 1) mvaddch(i, j * 2, ACS_LRCORNER);
            else if (i == 0) mvaddch(i, j * 2, ACS_TTEE);
            else if (i == BOARD_SIZE - 1) mvaddch(i, j * 2, ACS_BTEE);
            else if (j == 0) mvaddch(i, j * 2, ACS_LTEE);
            else if (j == BOARD_SIZE - 1) mvaddch(i, j * 2, ACS_RTEE);
            else mvaddch(i, j * 2, ACS_PLUS);
            if (j < BOARD_SIZE - 1) mvaddch(i, j * 2 + 1, ACS_HLINE);
        }
    }
    // 돌 표시
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == BLACK) mvaddstr(i, j * 2, "@");
            else if (board[i][j] == WHITE) mvaddstr(i, j * 2, "O");
        }
    }
    mvprintw(LINES - 2, 0, "상태: %s", status_message);

    if (strlen(ban_message) > 0) {
        mvprintw(BOARD_SIZE + 2, 0, "금수 안내: %s", ban_message);
    }
    
    if (client_state == PLAYING) {
        mvprintw(LINES - 1, 0, "이동: 방향키 | 선택: 스페이스바 | 종료: q");
        move(current_row, current_col * 2);
    } else {
         mvprintw(LINES - 1, 0, "r: 재시작 | p: 전적보기 | q: 종료");
    }
    refresh();
}

void draw_records(GamePacket* packet) {
    clear();
    mvprintw(0, 0, "=== 전체 전적 ===");
    for(int i=0; i<packet->record_count; i++) {
        mvprintw(i + 2, 0, "- %s : %d승 %d패", packet->records[i].name, packet->records[i].wins, packet->records[i].losses);
    }
    mvprintw(LINES - 2, 0, "상태: %s", status_message);
    mvprintw(LINES - 1, 0, "p: 게임 화면으로 돌아가기");
    refresh();
}

// 서버로부터 패킷을 수신하는 역할을 하는 스레드 함수
void* receive_handler(void* arg) {
    int sock = *(int*)arg;
    GamePacket packet;

    while (client_state != -1 && recv(sock, &packet, sizeof(GamePacket), 0) > 0) {
        pthread_mutex_lock(&ncurses_mutex);
        //strcpy(status_message, packet.message);

        // 서버로부터 수신받은 패킷 타입에 따라 처리
        switch (packet.type) {
            case RES_START:
                client_state = PLAYING;
                memcpy(board, packet.board, sizeof(board));
                strcpy(status_message, packet.message);
                
                // 내 색깔과 현재 턴 정보 저장
                if (strstr(packet.message, "흑돌")) {
                    my_color = BLACK;
                } else {
                    my_color = WHITE;
                }
                current_turn = packet.player; // 서버가 알려준 현재 턴

                draw_game_board();
            case RES_UPDATE:
                client_state = PLAYING;
                memcpy(board, packet.board, sizeof(board));
				strcpy(status_message, packet.message);
                current_turn = packet.player; // 업데이트된 턴 정보 저장
                draw_game_board();
                break;
            case RES_GAME_OVER:
                client_state = (packet.type == RES_GAME_OVER) ? POST_GAME : PLAYING;
                memcpy(board, packet.board, sizeof(board));
				strcpy(status_message, packet.message);
                current_turn = -1; // 게임이 끝났으므로 턴 정보 초기화
                draw_game_board();
                break;
            case RES_RECORDS:
                client_state = VIEWING_RECORDS;
				strcpy(status_message, packet.message);
                draw_records(&packet);
                break;
            case INFO_REMATCH:
				strcpy(status_message, packet.message);
				draw_game_board();
				break;
            case RES_INVALID:
                strncpy(ban_message, packet.message, sizeof(ban_message) - 1);
                ban_message[sizeof(ban_message) - 1] = '\0'; // null-termination
                draw_game_board();
                break;
            case INFO_OPP_QUIT:
                // 게임 보드를 다시 그려 메시지 업데이트
                if (packet.type == INFO_OPP_QUIT) client_state = -1; // 종료 플래그
				strcpy(status_message, packet.message);
                draw_game_board();
                break;
            default:
				strcpy(status_message, packet.message);
                draw_game_board();
        }
        pthread_mutex_unlock(&ncurses_mutex);
    }
    pthread_mutex_lock(&ncurses_mutex);
    client_state = -1; // 서버 연결 종료
    strcpy(status_message, "서버와 연결이 끊겼습니다. q를 눌러 종료하세요.");
    draw_game_board();
    pthread_mutex_unlock(&ncurses_mutex);
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "사용법: %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    int sock;
    struct sockaddr_in serv_addr;
    pthread_t recv_thread;

    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect() error");
        exit(1);
    }
    
    // ncurses 초기화
    setlocale(LC_ALL, ""); // 유니코드 문자 지원
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(2);
    pthread_mutex_init(&ncurses_mutex, NULL);

    // 서버에 이름 전송
    GamePacket name_packet;
    name_packet.type = REQ_NAME;
    strncpy(name_packet.name, argv[3], MAX_NAME_LEN -1);
    send(sock, &name_packet, sizeof(GamePacket), 0);

    current_row = BOARD_SIZE / 2;
    current_col = BOARD_SIZE / 2;
    
    // 수신 스레드 생성
    pthread_create(&recv_thread, NULL, receive_handler, &sock);

    // 메인 스레드: 사용자 입력 처리
    while (client_state != -1) {

        pthread_mutex_lock(&ncurses_mutex);
        if (client_state != VIEWING_RECORDS) {
            draw_game_board();
        }
        pthread_mutex_unlock(&ncurses_mutex);

        int ch = getch();
        if (client_state == -1) break;

        GamePacket req_packet;

		if(ch=='q'){
			if (client_state == PLAYING) { // 게임 중이었다면 포기 요청 전송
                req_packet.type = REQ_QUIT;
                send(sock, &req_packet, sizeof(GamePacket), 0);
            }
            client_state = -1; // 클라이언트 상태를 -1로 변경하여 루프 탈출
            continue;          // 즉시 루프 조건 검사
        }


        if (client_state == PLAYING) {
            switch (ch) {
                case KEY_UP:    if(current_row > 0) current_row--; break;
                case KEY_DOWN:  if(current_row < BOARD_SIZE - 1) current_row++; break;
                case KEY_LEFT:  if(current_col > 0) current_col--; break;
                case KEY_RIGHT: if(current_col < BOARD_SIZE - 1) current_col++; break;
                case ' ':
                    // 현재 게임 중이고, 내 턴일 때만 동작하도록 함
                    if (current_turn == my_color) { 
                        ban_message[0] = '\0';
                        req_packet.type = REQ_MOVE;
                        req_packet.row = current_row;
                        req_packet.col = current_col;
                        send(sock, &req_packet, sizeof(GamePacket), 0);
                        
                        pthread_mutex_lock(&ncurses_mutex);
                        strcpy(status_message, "서버의 응답을 기다립니다...");
                        pthread_mutex_unlock(&ncurses_mutex);
                    } else {
                        // 내 턴이 아닐 때는 메시지만 표시하고 아무것도 안 함
                        pthread_mutex_lock(&ncurses_mutex);
                        strcpy(status_message, "상대방의 턴입니다. 둘 수 없습니다.");
                        pthread_mutex_unlock(&ncurses_mutex);
                    }
                    break;
                case 'q':
                    // 'q'를 누르면 서버에 포기 요청을 보냄
                    req_packet.type = REQ_QUIT;
                    send(sock, &req_packet, sizeof(GamePacket), 0);
                    client_state = -1; // 클라이언트는 즉시 루프 종료
                    break;
            }
        } else if (client_state == POST_GAME) {
            switch (ch) {
                case 'r':
                    req_packet.type = REQ_REMATCH;
                    send(sock, &req_packet, sizeof(GamePacket), 0);
                    pthread_mutex_lock(&ncurses_mutex);
                    strcpy(status_message, "상대방의 재시작 응답을 기다립니다...");
                    draw_game_board();
                    pthread_mutex_unlock(&ncurses_mutex);
                    break;
                case 'p':
                    req_packet.type = REQ_RECORDS;
                    send(sock, &req_packet, sizeof(GamePacket), 0);
                    break;
                case 'q': client_state = -1; break;
            }
        } else if (client_state == VIEWING_RECORDS) {
            if (ch == 'p') {
                // 게임 종료 화면으로 돌아가기 (서버에 요청할 필요 없음, 클라이언트 상태만 변경)
                pthread_mutex_lock(&ncurses_mutex);
                client_state = POST_GAME;
                strcpy(status_message, "게임이 종료되었습니다.");
                draw_game_board();
                pthread_mutex_unlock(&ncurses_mutex);
            } else if (ch == 'q') {
                client_state = -1;
            }
        }
        
        if (client_state == PLAYING) {
            pthread_mutex_lock(&ncurses_mutex);
            move(current_row, current_col * 2);
            refresh();
            pthread_mutex_unlock(&ncurses_mutex);
        }
    }

    close(sock);
    pthread_join(recv_thread, NULL);
    
    endwin();
    pthread_mutex_destroy(&ncurses_mutex);
    printf("클라이언트를 종료합니다.\n");
    return 0;
}
