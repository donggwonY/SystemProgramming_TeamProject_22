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
	int ch;					// 사용자 키 입력을 저장할 변수
	initscr();
	keypad(stdscr, TRUE);	// 방향키 입력 활성화
	noecho();
	curs_set(1);
	
	// board 초기화
	for(int i=0; i<BOARD_SIZE; i++){
		for(int j=0; j<BOARD_SIZE; j++){
			board[i][j] = EMPTY;
		}
	}
	
	// 게임 시작시 커서 위치를 보드 가운데로 설정, 흑돌 시작
	current_row = BOARD_SIZE/2;
	current_col = BOARD_SIZE/2;
	current_player = BLACK;


	while(1){
		draw_board();
		ch = getch();
		if(ch=='q') break;

		switch(ch){
			case KEY_UP:
				if(current_row>0) current_row--;
				break;
			case KEY_DOWN:
				if(current_row<BOARD_SIZE-1) current_row++;
				break;
			case KEY_LEFT:
				if(current_col>0) current_col--;
				break;
			case KEY_RIGHT:
				if(current_col<BOARD_SIZE-1) current_col++;
				break;
			case ' ':	// space로 착수
				if(board[current_row][current_col] == EMPTY){
					board[current_row][current_col] = current_player;
					current_player = (current_player == BLACK) ? WHITE : BLACK;	// 다음 플레이어로 턴 전환
				}
				break;
		}
	}

	endwin();
	return 0;
}


void draw_board(){
	clear();
	for(int i=0; i<BOARD_SIZE; i++){
		for(int j=0; j<BOARD_SIZE*2; j+=2){
			//mvaddch(i, j, '-');	// 가로선 그리기
		}
		if(i<BOARD_SIZE){
			for(int j=0; j<BOARD_SIZE; j++){
				//mvaddch(i, j*2, '|');	// 세로선
				mvaddch(i, j*2+1, '+');	// 교차점
				if(board[i][j] == BLACK){
					mvaddch(i, j*2+1, '@');	// 흑돌
				}
				else if(board[i][j] == WHITE){
					mvaddch(i, j*2+1, 'O');	// 백돌
				}
			}
			//mvaddch(i, BOARD_SIZE*2, '|');	// 마지막 세로선
		}
	}

	mvprintw(LINES-2, 0, "current player: %s (move : movekey, put : SPACE, quit: q)", current_player==BLACK ? "@" : "O");
   	move(current_row, current_col*2+1);
	refresh();
}

