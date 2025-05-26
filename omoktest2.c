#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>

#define BOARD_SIZE 15
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define BLACK_STONE "●"
#define WHITE_STONE "○"

int board[BOARD_SIZE][BOARD_SIZE];
int current_row, current_col;
int current_player;

void draw_board();
int check_horizontal_win(int player_stone, int row, int col);
int check_vertical_win(int player_stone, int row, int col);
int check_diagonal_win1(int player_stone, int row, int col);
int check_diagonal_win2(int player_stone, int row, int col);
int check_win(int player_stone, int row, int col);
int is_on_board(int row, int col);
int get_stone_at(int row, int col);
int create_open3(int player_stone, int row, int col, int dr, int dc);
int check_33(int player_stone, int row, int col);


int main(){
regame:
	setlocale(LC_ALL, "");	// 유니코드 지원
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
					if(current_player == BLACK && check_33(BLACK, current_row, current_col)){
						mvprintw(LINES-3, 0, "3-3 Forbidden move! press any key to continue");
					   	clrtoeol();
						refresh();
						getch();
						move(LINES-3, 0);
						clrtoeol();
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
						else{
							current_player = (current_player == BLACK) ? WHITE : BLACK;
						}
						draw_board();
						refresh();
					}
				}
				break;
			}
		}

	if(game_run==0 && !(ch=='q')){
		draw_board();
		mvprintw(LINES-3, 0, "player %d win, press r to replay", current_player);
		clrtoeol();
		refresh();
		while((ch = getch()) != 'r' && ch != 'q');
		if(ch=='r') goto regame;
	}

	endwin();
	return 0;
}


void draw_board(){
	clear();
	// 테두리 및 내부 교차점 그리기
    for (int i=0; i<BOARD_SIZE; i++) {
        for (int j=0; j<BOARD_SIZE; j++) {
            if (i==0 && j==0) { // 왼쪽 상단 모서리
                mvaddch(i, j*2, ACS_ULCORNER);
            } else if (i==0 && j==BOARD_SIZE-1) { // 오른쪽 상단 모서리
                mvaddch(i, j * 2, ACS_URCORNER);
            } else if (i==BOARD_SIZE-1 && j==0) { // 왼쪽 하단 모서리
                mvaddch(i, j*2, ACS_LLCORNER);
            } else if (i==BOARD_SIZE-1 && j==BOARD_SIZE-1) { // 오른쪽 하단 모서리
                mvaddch(i, j*2, ACS_LRCORNER);
            } else if (i==0) { // 상단 가로선
                mvaddch(i, j*2, ACS_TTEE);
            } else if (i==BOARD_SIZE-1) { // 하단 가로선
                mvaddch(i, j*2, ACS_BTEE);
            } else if (j==0) { // 왼쪽 테두리
                mvaddch(i, j*2, ACS_LTEE);
            } else if (j==BOARD_SIZE-1) { // 오른쪽 테두리
                mvaddch(i, j*2, ACS_RTEE);
            } else { // 내부 교차점
                mvaddch(i, j*2, ACS_PLUS);
            }

            // 가로선 연결
            if (j<BOARD_SIZE-1) {
                mvaddch(i, j*2+1, ACS_HLINE);
            }
            // 세로선 연결
            if (i<BOARD_SIZE-1) {
                mvaddch(i+1, j*2, ACS_VLINE);
            }
        }
    }

    // 돌 표시
    for (int i=0; i<BOARD_SIZE; i++) {
        for (int j=0; j<BOARD_SIZE; j++) {
            if (board[i][j]==BLACK)
                mvaddch(i, j*2, '@'); // 흑돌
            else if (board[i][j] == WHITE)
                mvaddch(i, j*2, 'O'); // 백돌
        }
    };

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

int is_on_board(int row, int col){
	return 0<=row && row<BOARD_SIZE && 0<=col && col<BOARD_SIZE;
}

int get_stone_at(int row, int col){
	if(!is_on_board(row, col)){
		return -1;
	}
	return board[row][col];
}

int create_open3(int player_stone, int row, int col, int dr, int dc){
	if( get_stone_at(row-dr, col-dc) == EMPTY &&
		get_stone_at(row+dr, col+dc) == player_stone &&
		get_stone_at(row+2*dr, col+2*dc) == player_stone &&
		get_stone_at(row+3*dr, col+2*dc) == EMPTY){
		// case1: []XOO[]
		return 1;
	}
	if( get_stone_at(row-2*dr, col-2*dc) == EMPTY &&
		get_stone_at(row-dr, col-dc) == player_stone &&
		get_stone_at(row+dr, col+dc) == player_stone &&
		get_stone_at(row+2*dr, col+2*dc) == EMPTY){
		// case2: []OXO[]
		return 2;
	}
	if( get_stone_at(row-3*dr, col-3*dc) == EMPTY &&
		get_stone_at(row-2*dr, col-2*dc) == player_stone &&
		get_stone_at(row-dr, col-dc) == player_stone &&
		get_stone_at(row+dr, col+dc) == EMPTY){
		//case3: []OOX[]
		return 3;
	}

	return 0;
}

int check_33(int player_stone, int row, int col){

	board[row][col] = player_stone;	// 임시 착수, 이후 테스트

	int open_33_cnt = 0;
	int dr[] = {0, 1, 1, 1};	// delta row
	int dc[] = {1, 0, 1, -1};	// delta col
	
	// i=0 : 가로로 검사, i=1: 세로로 검사
	// i=2 : (\)방향으로 검사, i=3: (/)방향으로 검사
	for(int i=0; i<4; i++){
		if(create_open3(player_stone, row, col, dr[i], dc[i])){
			open_33_cnt++;
		}
	}
	
	board[row][col] = EMPTY;		// 테스트 후 원래상태로 초기화
	
	return open_33_cnt >= 2;
}	
