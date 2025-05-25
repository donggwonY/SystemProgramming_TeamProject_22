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
int check_horizontal_win(int player_stone, int row, int col);
int check_vertical_win(int player_stone, int row, int col);
int check_diagonal_win1(int player_stone, int row, int col);
int check_diagonal_win2(int player_stone, int row, int col);
int check_win(int player_stone, int row, int col);
int check_hor_open3(int row, int col);
int check_ver_open3(int row, int col);
int check_33(int row, int col);


int main(){
regame:
	int ch;					// 사용자 키 입력을 저장할 변수
	int game_run;			// game running을 확인하는 변수
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
	game_run = 1;


	while(game_run){
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
				if(board[current_row][current_col] == EMPTY){			// 해당 칸이 비어있다면
					if(check_33(current_row, current_col)){
						mvprintw(LINES-3, 0, "3-3 Forbidden move! press any key to continue");
					   	refresh();
						mvprintw(LINES-3, 0, "                                             ");
						refresh();
						continue;
					}
					else{
						board[current_row][current_col] = current_player;	// 착수
						draw_board();
						refresh();
						if(check_win(current_player, current_row, current_col)){
							game_run = 0;
							break;
						}
					}
	
					current_player = (current_player == BLACK) ? WHITE : BLACK;	// 다음 플레이어로 턴 전환
				}
				break;
		}
	}

	if(game_run==0){
		mvprintw(LINES-3, 0, "player %d win, press r to replay", current_player);
		refresh();
		ch = getch();
		if(ch=='r') goto regame;
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

// 가로방향 승리검사
// player_stone 현재 착수한 플레이어의 돌 (BLACK or WHITE), 방금 놓은 돌의 위치 row, col
int check_horizontal_win(int player_stone, int row, int col){

	// 방금 착수한 위치의 왼쪽 4칸, 오른쪽 4칸을 검사
	for(int start_col = col-4; start_col <= col; start_col++){
		
		// 검사를 시작할 위치와 끝낼 위치가 유효한 보드 내에 있는지 확인
		if(start_col >=0 && start_col+4<BOARD_SIZE){
			int check_cnt = 0;

			// start_col부터 시작하여 5칸 검사
			for(int k=0; k<5; k++){
				if(board[row][start_col+k] == player_stone){
					check_cnt++;
				}
				else{// 방금 놓은 돌과 다르다면 연속되지 않았음으로 바로 break하고
					 // start_col(시작위치)++ 하여 새롭게 검사 시작
					break;
				}
			}

			if(check_cnt==5){
				return 1;	// 연속된 5개의 돌로 오목이 완성되었다면 1 리턴
			}
		}
	}

	return 0;	// 검사가 끝난 후 check_cnt==5에 걸리지 않았다면 오목이 완성되지 않았음, 0 리턴
}

// 세로방향 승리검사
int check_vertical_win(int player_stone, int row, int col){
	for(int start_row = row-4; start_row <= row; start_row++){
		if(start_row >= 0 && start_row+4 < BOARD_SIZE){
			int check_cnt = 0;
			for(int k=0; k<5; k++){
				if(board[start_row+k][col] == player_stone){
					check_cnt++;
				}
				else{
					break;
				}
			}
			if(check_cnt==5){
				return 1;
			}
		}
	}
	return 0;
}

// 대각선방향(\) 승리검사
int check_diagonal_win1(int player_stone, int row, int col){
	for(int i=0; i<5; i++){
		// 방금 착수한 돌의 위치부터 대각선 아래(\)방향으로 검사 시작
		// 그 후 검사 시작 위치를 대각선 위(\)방향으로 한 칸씩 올라가며 검사
		int start_row = row-i;
		int start_col = col-i;
		
		if(start_row>=0 && start_col >=0 && start_row+4<BOARD_SIZE && start_col+4<BOARD_SIZE){
			int check_cnt = 0;
			for(int j=0; j<5; j++){
				if(board[start_row+j][start_col+j]==player_stone){
					check_cnt++;
				}
				else{
					break;
				}
			}
			if(check_cnt==5){
				return 1;
			}
		}
	}
	return 0;
}

// 대각선방향(/) 승리검사
int check_diagonal_win2(int player_stone, int row, int col){
	for(int i=0; i<5; i++){
		// 현재 착수한 지점으로부터 (/)아래방향으로 검사시작
		// 그 후 검사 시작 위치를 (/)위 방향으로 한 칸씩 올라가며 검사
		int start_row = row-i;
		int start_col = col+i;

		if(start_row>=0 && start_col<=BOARD_SIZE && start_row+4<BOARD_SIZE && start_col-4>=0){
			int check_cnt = 0;
			for(int j=0; j<5; j++){
				if(board[start_row+j][start_col-j]==player_stone){
					check_cnt++;
				}
				else{
					break;
				}
			}
			if(check_cnt==5){
				return 1;
			}
		}
	}
	return 0;
}

// 모든방향 승리조건 검사
int check_win(int player_stone, int row, int col){
	if(check_horizontal_win(player_stone, row, col)) return 1;
	if(check_vertical_win(player_stone, row, col)) return 1;
	if(check_diagonal_win1(player_stone, row, col)) return 1;
	if(check_diagonal_win2(player_stone, row, col)) return 1;
	return 0;
}

int check_hor_open3(int row, int col){
	if(0<=col-1 && board[row][col-1]==EMPTY)	// []
		if(col+1<BOARD_SIZE && board[row][col+1]==BLACK)	//[]XO
			if(col+2<BOARD_SIZE && board[row][col+2]==BLACK)	// []XOO
				if(col+3<BOARD_SIZE && board[row][col+3]==EMPTY)	// case1: []XOO[]
					return 1;
	else if(0<=col-1 && board[row][col-1]==BLACK)	// OX
		if(0<=col-2 && board[row][col-2]==EMPTY)	// []OX
			if(col+1<BOARD_SIZE && board[row][col+1]==BLACK)	// []OXO	
				if(col+2<BOARD_SIZE && board[row][col+2]==EMPTY)	// case2: []OXO[]
					return 2;
		else if(0<=col-2 && board[row][col-2]==BLACK)	// OOX
			if(0<=col-3 && board[row][col-3]==EMPTY)	// []OOX
				if(col+1<BOARD_SIZE && board[row][col+1]==EMPTY)	// case3: []OOX[]
					return 3;
	else// (0<=col-1 && board[row][col-1]==WHITE), open3이 아닌 경우
		return 0;
}

int check_ver_open3(int row, int col){
	if(0<=row-1 && board[row-1][col]==BLACK)	// XO +90도 회전
		if(0<=row-2 && board[row-2][col]==BLACK)	// XOO +90도 회전
			if(0<=row-3 && board[row-3][col]==EMPTY)	// XOO[] +90도 회전
				if(row+1<BOARD_SIZE && board[row+1][col]==EMPTY)	// case1: []XOO[] +90도 회전
					return 1;
		else if(0<=row-2 && board[row-2][col]==EMPTY)	// XO[]
			if(row+1<BOARD_SIZE && board[row+1][col]==BLACK)	// OXO[]
				if(row+2<BOARD_SIZE && board[row+2][col]==EMPTY)	// case2: []OXO[] +90도 회전
					return 2;
	else if(0<=row-1 && board[row-1][col]==EMPTY)
		if(row+1<BOARD_SIZE && board[row+1][col]==BLACK)
			if(row+2<BOARD_SIZE && board[row+2][col]==BLACK)
				if(row+3<BOARD_SIZE && board[row+3][col]==EMPTY)
					return 3;
	else// open3 아닌 경우
		return 0;
}

int check_33(int row, int col){
	board[row][col] = BLACK;
	if(check_hor_open3(row, col)==1){
		for(int i=0; i<3; i++){
			if(check_ver_open3(row, col+i))
				board[row][col] = EMPTY;
				return 1;
		}
	}
	
	else if(check_hor_open3(row, col)==2){
		for(int i=-1; i<2; i++){
			if(check_ver_open3(row, col+i))
				board[row][col] = EMPTY;
				return 1;
		}
	}

	else if(check_hor_open3(row, col)==3){
		for(int i=-2; i<1; i++){
			if(check_ver_open3(row, col+i))
				board[row][col] = EMPTY;
				return 1;
		}
	}
	board[row][col] = EMPTY;
	return 0;
}


		


