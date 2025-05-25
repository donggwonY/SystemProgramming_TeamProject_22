#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define BOARD_SIZE 15
#define EMPTY 0
#define BLACK 1
#define WHITE 2

int board[BOARD_SIZE][BOARD_SIZE];
int current_row, current_col;
int current_player;

void draw_board();
void init_ncurses();
void exit_ncurses();

int main(){
    // 와이드문자(유니코드) 지원
    setlocale(LC_ALL, "");

    init_ncurses(); // ncurses 초기화 함수 호출

    // board 초기화
    for(int i = 0; i < BOARD_SIZE; i++){
        for(int j = 0; j < BOARD_SIZE; j++){
            board[i][j] = EMPTY;
        }
    }
    current_row = BOARD_SIZE / 2;
    current_col = BOARD_SIZE / 2;
    current_player = BLACK;

    int ch;
    while(1){
        draw_board();
        ch = getch();
        if(ch == 'q') break;

        switch(ch){
            case KEY_UP:
                if(current_row > 0) current_row--;
                break;
            case KEY_DOWN:
                if(current_row < BOARD_SIZE - 1) current_row++;
                break;
            case KEY_LEFT:
                // current_col의 최소값을 0으로 변경
                if(current_col > 0) current_col--;
                break;
            case KEY_RIGHT:
                // current_col의 최대값을 BOARD_SIZE - 1로 변경
                if(current_col < BOARD_SIZE - 1) current_col++;
                break;
            case ' ':
                if(board[current_row][current_col] == EMPTY){
                    board[current_row][current_col] = current_player;
                    current_player = (current_player == BLACK) ? WHITE : BLACK;
                }
                break;
        }
    }

    exit_ncurses(); // ncurses 종료 함수 호출
    return 0;
}

// 화면에 오목판 그리기
void draw_board() {
    clear(); // 화면 지우기

    // 테두리 및 내부 교차점 그리기
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (i == 0 && j == 0) { // 좌상단 모서리
                mvaddch(i, j * 2, ACS_ULCORNER);
            } else if (i == 0 && j == BOARD_SIZE - 1) { // 우상단 모서리
                mvaddch(i, j * 2, ACS_URCORNER);
            } else if (i == BOARD_SIZE - 1 && j == 0) { // 좌하단 모서리
                mvaddch(i, j * 2, ACS_LLCORNER);
            } else if (i == BOARD_SIZE - 1 && j == BOARD_SIZE - 1) { // 우하단 모서리
                mvaddch(i, j * 2, ACS_LRCORNER);
            } else if (i == 0) { // 상단 가로선
                mvaddch(i, j * 2, ACS_TTEE);
            } else if (i == BOARD_SIZE - 1) { // 하단 가로선
                mvaddch(i, j * 2, ACS_BTEE);
            } else if (j == 0) { // 좌측 세로선
                mvaddch(i, j * 2, ACS_LTEE);
            } else if (j == BOARD_SIZE - 1) { // 우측 세로선
                mvaddch(i, j * 2, ACS_RTEE);
            } else { // 내부 교차점
                mvaddch(i, j * 2, ACS_PLUS);
            }

            // 가로선 연결 (오른쪽으로 그리기)
            if (j < BOARD_SIZE - 1) {
                mvaddch(i, j * 2 + 1, ACS_HLINE);
            }
            // 세로선 연결 (아래쪽으로 그리기)
            if (i < BOARD_SIZE - 1) {
                mvaddch(i + 1, j * 2, ACS_VLINE);
            }
        }
    }

    // 돌 표시
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == BLACK)
                mvaddch(i, j * 2, '@'); // 흑돌
            else if (board[i][j] == WHITE)
                mvaddch(i, j * 2, 'O'); // 백돌
        }
    }

    mvprintw(LINES - 2, 0,
             "current player: %s  (move: keypad, put: SPACE, quit: q)",
             current_player == BLACK ? "@" : "O");
    move(current_row, current_col * 2); // 현재 커서 위치로 이동
    refresh();
}

// ncurses 초기화
void init_ncurses() {
    initscr();
    keypad(stdscr, TRUE);   // 방향키 입력 활성화
    noecho();
    curs_set(1); // 커서 보이게 설정
}

// ncurses 종료
void exit_ncurses() {
    endwin();
}
