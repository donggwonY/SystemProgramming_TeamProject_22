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

int main(){
    // 와이드문자(유니코드) 지원
    setlocale(LC_ALL, "");

    initscr();
    keypad(stdscr, TRUE);   // 방향키 입력 활성화
    noecho();
    curs_set(1);

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
                if(current_col > 1) current_col--;
                break;
            case KEY_RIGHT:
                if(current_col < BOARD_SIZE) current_col++;
                break;
            case ' ':
                if(board[current_row][current_col] == EMPTY){
                    board[current_row][current_col] = current_player;
                    current_player = (current_player == BLACK) ? WHITE : BLACK;
                }
                break;
        }
    }

    endwin();
    return 0;
}

// 화면에 오목판 그리기
void draw_board() {
    clear();  // 화면 지우기
              //
    // 1) 가로선: 각 행마다 전체 너비만큼 '─' 그리기
    for (int i = 0; i < BOARD_SIZE; i++) {
        mvhline(i, 0, ACS_HLINE, BOARD_SIZE * 2 + 2);
    }

    // 2) 세로선: 각 열마다 '│' 그리기
    for (int j = 0; j <= BOARD_SIZE+1; j++) {
        mvvline(0, j * 2, ACS_VLINE, BOARD_SIZE);
    }

    // 3) 내부 교차점(┼) 및 돌 표시: 테두리 제외
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 1; j < BOARD_SIZE+1; j++) {
            mvaddch(i, j * 2, ACS_PLUS);
            if (board[i][j] == BLACK)
                mvaddch(i, j * 2, '@');  // 흑돌
            else if (board[i][j] == WHITE)
                mvaddch(i, j * 2, 'O');  // 백돌
        }
    }

    mvprintw(LINES-2, 0,
        "current player: %s  (move: 방향키, put: SPACE, quit: q)",
        current_player == BLACK ? "@" : "O");
    move(current_row, current_col * 2);
    refresh();
}

