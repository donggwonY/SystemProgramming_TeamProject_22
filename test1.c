#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#define BOARD_SIZE    15
#define EMPTY         0
#define BLACK         1
#define WHITE         2
#define MAX_NAME_LEN  32
#define RECORD_FILE   "gomoku_records.dat"

typedef struct {
    char name[MAX_NAME_LEN];
    int  wins;
    int  losses;
    int  draws;
} PlayerRecord;

int board[BOARD_SIZE][BOARD_SIZE];
int current_row, current_col;
int current_player;
char name_black[MAX_NAME_LEN];
char name_white[MAX_NAME_LEN];

// 전적 관련 함수 원형
int load_all_records(PlayerRecord **out_records);
void save_all_records(PlayerRecord *recs, int cnt);
PlayerRecord *get_or_create_record(PlayerRecord **recs, int *cnt, const char *name);

// 게임 함수 원형
void draw_board();
int  check_horizontal_win(int player_stone, int row, int col);
int  check_vertical_win(int player_stone, int row, int col);
int  check_diagonal_win1(int player_stone, int row, int col);
int  check_diagonal_win2(int player_stone, int row, int col);
int  check_win(int player_stone, int row, int col);
int  is_on_board(int row, int col);
int  get_stone_at(int row, int col);
int  create_open3(int player_stone, int row, int col, int dr, int dc);
int  check_33(int player_stone, int row, int col);

int main() {
    setlocale(LC_ALL, "");  // 유니코드 지원
    initscr();
    keypad(stdscr, TRUE);  // 방향키 입력 활성화
    noecho();
    curs_set(1);

    // 1) 닉네임 입력
    echo();
    mvprintw(0, 0, "Enter Black player name: ");
    getnstr(name_black, MAX_NAME_LEN - 1);
    mvprintw(1, 0, "Enter White player name: ");
    getnstr(name_white, MAX_NAME_LEN - 1);
    noecho();
    clear();

    // 2) 전적 파일 로드
    PlayerRecord *records = NULL;
    int record_count = load_all_records(&records);
    // black, white 레코드 가져오기 혹은 새로 추가
    PlayerRecord *rec_black = get_or_create_record(&records, &record_count, name_black);
    PlayerRecord *rec_white = get_or_create_record(&records, &record_count, name_white);

    int ch;
    int game_run;
    int total_moves;

regame:
    // 보드 초기화
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = EMPTY;
        }
    }
    current_row = BOARD_SIZE / 2;
    current_col = BOARD_SIZE / 2;
    current_player = BLACK;
    game_run = 1;
    total_moves = 0;

    while(game_run) {
        draw_board();
        ch = getch();
        if(ch == 'q') break;

        switch(ch) {
            case KEY_UP:
                if(current_row > 0) current_row--;
                break;
            case KEY_DOWN:
                if(current_row < BOARD_SIZE - 1) current_row++;
                break;
            case KEY_LEFT:
                if(current_col > 0) current_col--;
                break;
            case KEY_RIGHT:
                if(current_col < BOARD_SIZE - 1) current_col++;
                break;
            case ' ':  // space로 착수
                if(board[current_row][current_col] == EMPTY) {
                    if(current_player == BLACK && check_33(BLACK, current_row, current_col)) {
                        mvprintw(LINES - 3, 0, "3-3 Forbidden move! press any key to continue");
                        clrtoeol();
                        refresh();
                        getch();
                        move(LINES - 3, 0);
                        clrtoeol();
                        continue;
                    } else {
                        board[current_row][current_col] = current_player;
                        total_moves++;
                        draw_board();
                        refresh();
                        if(check_win(current_player, current_row, current_col)) {
                            game_run = 0;
                            break;
                        } else {
                            current_player = (current_player == BLACK) ? WHITE : BLACK;
                        }
                        draw_board();
                        refresh();
                    }
                }
                break;
        }
    }

    // 게임 종료 후 전적 업데이트 및 메시지
    if(game_run == 0 && ch != 'q') {
        // 승자 기록 업데이트
        PlayerRecord *winner_rec = (current_player == BLACK) ? rec_black : rec_white;
        PlayerRecord *loser_rec  = (current_player == BLACK) ? rec_white : rec_black;
        winner_rec->wins++;
        loser_rec->losses++;
        save_all_records(records, record_count);

        draw_board();
        mvprintw(LINES - 3, 0, "%s wins! Total moves: %d", 
                 (current_player == BLACK ? name_black : name_white), total_moves);
        mvprintw(LINES - 2, 0, "Press r to replay, q to quit");
        refresh();
        while((ch = getch()) != 'r' && ch != 'q');
        if(ch == 'r') goto regame;
    }

    endwin();
    // 종료 직전에 메모리 해제
    free(records);
    return 0;
}

void draw_board() {
    clear();
    // 테두리 및 내부 교차점 그리기
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            if(i == 0 && j == 0) mvaddch(i, j * 2, ACS_ULCORNER);
            else if(i == 0 && j == BOARD_SIZE - 1) mvaddch(i, j * 2, ACS_URCORNER);
            else if(i == BOARD_SIZE - 1 && j == 0) mvaddch(i, j * 2, ACS_LLCORNER);
            else if(i == BOARD_SIZE - 1 && j == BOARD_SIZE - 1) mvaddch(i, j * 2, ACS_LRCORNER);
            else if(i == 0) mvaddch(i, j * 2, ACS_TTEE);
            else if(i == BOARD_SIZE - 1) mvaddch(i, j * 2, ACS_BTEE);
            else if(j == 0) mvaddch(i, j * 2, ACS_LTEE);
            else if(j == BOARD_SIZE - 1) mvaddch(i, j * 2, ACS_RTEE);
            else mvaddch(i, j * 2, ACS_PLUS);

            if(j < BOARD_SIZE - 1) mvaddch(i, j * 2 + 1, ACS_HLINE);
            if(i < BOARD_SIZE - 1) mvaddch(i + 1, j * 2, ACS_VLINE);
        }
    }
    // 돌 표시
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            if(board[i][j] == BLACK) mvaddch(i, j * 2, '@');
            else if(board[i][j] == WHITE) mvaddch(i, j * 2, 'O');
        }
    }

    mvprintw(LINES - 2, 0, "Current: %s (%c)", 
             (current_player == BLACK ? name_black : name_white), 
             (current_player == BLACK ? '@' : 'O'));
    move(current_row, current_col * 2 + 1);
    refresh();
}

// 승리 검사 (가로)
int check_horizontal_win(int player_stone, int row, int col) {
    for(int start_col = col - 4; start_col <= col; start_col++) {
        if(start_col >= 0 && start_col + 4 < BOARD_SIZE) {
            int cnt = 0;
            for(int k = 0; k < 5; k++) {
                if(board[row][start_col + k] == player_stone) cnt++;
                else break;
            }
            if(cnt == 5) return 1;
        }
    }
    return 0;
}
int check_vertical_win(int player_stone, int row, int col) {
    for(int start_row = row - 4; start_row <= row; start_row++) {
        if(start_row >= 0 && start_row + 4 < BOARD_SIZE) {
            int cnt = 0;
            for(int k = 0; k < 5; k++) {
                if(board[start_row + k][col] == player_stone) cnt++;
                else break;
            }
            if(cnt == 5) return 1;
        }
    }
    return 0;
}
int check_diagonal_win1(int player_stone, int row, int col) {
    for(int i = 0; i < 5; i++) {
        int sr = row - i;
        int sc = col - i;
        if(sr >= 0 && sc >= 0 && sr + 4 < BOARD_SIZE && sc + 4 < BOARD_SIZE) {
            int cnt = 0;
            for(int j = 0; j < 5; j++) {
                if(board[sr + j][sc + j] == player_stone) cnt++;
                else break;
            }
            if(cnt == 5) return 1;
        }
    }
    return 0;
}
int check_diagonal_win2(int player_stone, int row, int col) {
    for(int i = 0; i < 5; i++) {
        int sr = row - i;
        int sc = col + i;
        if(sr >= 0 && sc < BOARD_SIZE && sr + 4 < BOARD_SIZE && sc - 4 >= 0) {
            int cnt = 0;
            for(int j = 0; j < 5; j++) {
                if(board[sr + j][sc - j] == player_stone) cnt++;
                else break;
            }
            if(cnt == 5) return 1;
        }
    }
    return 0;
}
int check_win(int player_stone, int row, int col) {
    if(check_horizontal_win(player_stone, row, col)) return 1;
    if(check_vertical_win(player_stone, row, col)) return 1;
    if(check_diagonal_win1(player_stone, row, col)) return 1;
    if(check_diagonal_win2(player_stone, row, col)) return 1;
    return 0;
}

int is_on_board(int row, int col) {
    return (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE);
}
int get_stone_at(int row, int col) {
    if(!is_on_board(row, col)) return -1;
    return board[row][col];
}
int create_open3(int player_stone, int row, int col, int dr, int dc) {
    if(get_stone_at(row - dr, col - dc) == EMPTY &&
       get_stone_at(row + dr, col + dc) == player_stone &&
       get_stone_at(row + 2*dr, col + 2*dc) == player_stone &&
       get_stone_at(row + 3*dr, col + 3*dc) == EMPTY) {
        return 1;
    }
    if(get_stone_at(row - 2*dr, col - 2*dc) == EMPTY &&
       get_stone_at(row - dr, col - dc) == player_stone &&
       get_stone_at(row + dr, col + dc) == player_stone &&
       get_stone_at(row + 2*dr, col + 2*dc) == EMPTY) {
        return 2;
    }
    if(get_stone_at(row - 3*dr, col - 3*dc) == EMPTY &&
       get_stone_at(row - 2*dr, col - 2*dc) == player_stone &&
       get_stone_at(row - dr, col - dc) == player_stone &&
       get_stone_at(row + dr, col + dc) == EMPTY) {
        return 3;
    }
    return 0;
}
int check_33(int player_stone, int row, int col) {
    board[row][col] = player_stone;
    int open_33_cnt = 0;
    int dr[4] = {0, 1, 1, 1};
    int dc[4] = {1, 0, 1, -1};
    for(int i = 0; i < 4; i++) {
        if(create_open3(player_stone, row, col, dr[i], dc[i])) open_33_cnt++;
    }
    board[row][col] = EMPTY;
    return (open_33_cnt >= 2);
}

// 레코드 파일 읽기
int load_all_records(PlayerRecord **out_records) {
    FILE *f = fopen(RECORD_FILE, "rb");
    if(!f) { *out_records = NULL; return 0; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    int cnt = sz / sizeof(PlayerRecord);
    rewind(f);
    *out_records = malloc(cnt * sizeof(PlayerRecord));
    fread(*out_records, sizeof(PlayerRecord), cnt, f);
    fclose(f);
    return cnt;
}
// 레코드 파일 저장 (덮어쓰기)
void save_all_records(PlayerRecord *recs, int cnt) {
    FILE *f = fopen(RECORD_FILE, "wb");
    if(!f) return;
    fwrite(recs, sizeof(PlayerRecord), cnt, f);
    fclose(f);
}
// 이름으로 레코드 찾거나 새로 추가
PlayerRecord *get_or_create_record(PlayerRecord **recs, int *cnt, const char *name) {
    for(int i = 0; i < *cnt; i++) {
        if(strcmp((*recs)[i].name, name) == 0) {
            return &(*recs)[i];
        }
    }
    *recs = realloc(*recs, (*cnt + 1) * sizeof(PlayerRecord));
    PlayerRecord *r = &(*recs)[*cnt];
    strncpy(r->name, name, MAX_NAME_LEN);
    r->wins = r->losses = r->draws = 0;
    (*cnt)++;
    return r;
}

