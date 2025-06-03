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

// 전적 관련 함수 원형
int  load_all_records(PlayerRecord **out_records);
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
int is_open_four(int player_stone, int row, int col, int dr, int dc);
int check_44(int player_stone, int row, int col);


// 화면 상태(enum)
typedef enum {
    SCR_WELCOME,      // 환영 화면 (메뉴 선택)
    SCR_INPUT_NAME,   // 닉네임 입력 (흑돌·백돌)
    SCR_GAMEPLAY,     // 실제 게임 플레이
    SCR_RECORDS,      // 전적 보기 화면
    SCR_GAME_OVER,    // 게임 종료 후 재시작/전적/종료 선택
    SCR_EXIT          // 프로그램 종료
} ScreenState;

// 전역 변수
int board[BOARD_SIZE][BOARD_SIZE];
int current_row, current_col;
int current_player;
char name_black[MAX_NAME_LEN];
char name_white[MAX_NAME_LEN];

int main() {
    setlocale(LC_ALL, "");        // 유니코드 지원
    initscr();
    keypad(stdscr, TRUE);         // 방향키 입력 활성화
    noecho();
    curs_set(1);

    ScreenState state = SCR_WELCOME;
    PlayerRecord *records = NULL;
    int record_count = 0;
    PlayerRecord *rec_black = NULL, *rec_white = NULL;
    int winner = 0;
    int ch, total_moves;

    while (state != SCR_EXIT) {
        switch (state) {
            // 1) 환영 화면: 메뉴 선택
            case SCR_WELCOME:
                clear();
                mvprintw(0, 0, "=== Gomoku ===");
                mvprintw(2, 0, "1. New Game");
                mvprintw(3, 0, "2. View Records");
                mvprintw(4, 0, "3. Quit");
                mvprintw(6, 0, "Select any number");
                refresh();

                ch = getch();
                if (ch == '1') {
                    state = SCR_INPUT_NAME;
                }
                else if (ch == '2') {
                    state = SCR_RECORDS;
                }
                else {
                    state = SCR_EXIT;
                }
                break;

            // 2) 닉네임 입력 화면
            case SCR_INPUT_NAME:
                echo();
                clear();
                mvprintw(0, 0, "Enter Black player name: ");
                getnstr(name_black, MAX_NAME_LEN - 1);
                mvprintw(1, 0, "Enter White player name: ");
                getnstr(name_white, MAX_NAME_LEN - 1);
                noecho();
                clear();

                // 전적 파일 로드 및 해당 닉네임 레코드 초기화
                record_count = load_all_records(&records);
                rec_black = get_or_create_record(&records, &record_count, name_black);
                rec_white = get_or_create_record(&records, &record_count, name_white);

                state = SCR_GAMEPLAY;
                break;

            // 3) 실제 게임 플레이
            case SCR_GAMEPLAY: {
                // 보드 초기화
                for (int i = 0; i < BOARD_SIZE; i++) {
                    for (int j = 0; j < BOARD_SIZE; j++) {
                        board[i][j] = EMPTY;
                    }
                }
                current_row = BOARD_SIZE / 2;
                current_col = BOARD_SIZE / 2;
                current_player = BLACK;
                total_moves = 0;
                winner = 0;
                int game_run_flag = 1;

                while (game_run_flag) {
                    draw_board();
                    ch = getch();
                    if (ch == 'q') {
                        game_run_flag = 0;
                        state = SCR_WELCOME;
                        break;
                    }
                    switch (ch) {
                        case KEY_UP:
                            if (current_row > 0) current_row--;
                            break;
                        case KEY_DOWN:
                            if (current_row < BOARD_SIZE - 1) current_row++;
                            break;
                        case KEY_LEFT:
                            if (current_col > 0) current_col--;
                            break;
                        case KEY_RIGHT:
                            if (current_col < BOARD_SIZE - 1) current_col++;
                            break;
                        case ' ':
                            if (board[current_row][current_col] == EMPTY) {
                                if (current_player == BLACK && check_33(BLACK, current_row, current_col)) {
                                    mvprintw(LINES - 3, 0, "3-3 Forbidden move! press any key to continue");
                                    clrtoeol();
                                    refresh();
                                    getch();
                                    move(LINES - 3, 0);
                                    clrtoeol();
                                    continue;
                                }
							   	else if(current_player == BLACK && check_44(BLACK, current_row, current_col)){
									mvprintw(LINES - 3, 0, "4-4 Forbidden move! press any key to continue");
                                    clrtoeol();
                                    refresh();
                                    getch();
                                    move(LINES - 3, 0);
                                    clrtoeol();
                                    continue;
								}	
								else {
                                    board[current_row][current_col] = current_player;
                                    total_moves++;
                                    draw_board();
                                    refresh();
                                    if (check_win(current_player, current_row, current_col)) {
                                        game_run_flag = 0;
                                        winner = current_player;
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
                } // while(game_run_flag)

                // 게임 종료 후 처리
                if (winner == BLACK || winner == WHITE) {
                    // 전적 업데이트
                    PlayerRecord *winner_rec = (winner == BLACK) ? rec_black : rec_white;
                    PlayerRecord *loser_rec  = (winner == BLACK) ? rec_white : rec_black;
                    winner_rec->wins++;
                    loser_rec->losses++;
                    save_all_records(records, record_count);

                    draw_board();
                    mvprintw(LINES - 3, 0, "%s wins! Total moves: %d",
                             (winner == BLACK ? name_black : name_white), total_moves);
                    mvprintw(LINES - 2, 0, "Press r to replay, p to view records, q to quit");
                    refresh();
                    state = SCR_GAME_OVER;
                }
                // q를 눌러 중간에 빠져나온 경우 이미 SCR_WELCOME으로 설정됨
                break;
            }

            // 4) 게임 종료 후 화면: 재시작/전적/종료 선택
            case SCR_GAME_OVER:
                while ((ch = getch())) {
                    if (ch == 'r') {
                        state = SCR_INPUT_NAME;
                        break;
                    }
                    else if (ch == 'p') {
                        state = SCR_RECORDS;
                        break;
                    }
                    else if (ch == 'q') {
                        state = SCR_EXIT;
                        break;
                    }
                }
                break;

            // 5) 전적 보기 화면
            case SCR_RECORDS: {
                clear();
                mvprintw(0, 0, "=== Player Records ===");
                PlayerRecord *allrec = NULL;
                int cnt = load_all_records(&allrec);
                for (int i = 0; i < cnt; i++) {
                    mvprintw(i + 1, 0, "%s: %dW - %dL - %dD",
                             allrec[i].name,
                             allrec[i].wins,
                             allrec[i].losses,
                             allrec[i].draws);
                }
                mvprintw(cnt + 2, 0, "Press any key to return to menu...");
                refresh();
                getch();
                free(allrec);
                state = SCR_WELCOME;
                break;
            }

            case SCR_EXIT:
            default:
                break;
        } // switch(state)
    } // while(state != SCR_EXIT)

    endwin();
    free(records);
    return 0;
}


// 화면 그리기 함수
void draw_board() {
    clear();
    // 테두리 및 내부 교차점 그리기
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
            if (i < BOARD_SIZE - 1) mvaddch(i + 1, j * 2, ACS_VLINE);
        }
    }
    // 돌 표시
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == BLACK) mvaddch(i, j * 2, '@');
            else if (board[i][j] == WHITE) mvaddch(i, j * 2, 'O');
        }
    }
    mvprintw(LINES - 5, 0, "Use arrow keys to move cursor, SPACE to place stone, Q to quit.");   
    mvprintw(LINES - 4, 0, "(Stones are placed to the left of the cursor.)");
    mvprintw(LINES - 2, 0, "Current: %s (%c)",
             (current_player == BLACK ? name_black : name_white),
             (current_player == BLACK ? '@' : 'O'));
    move(current_row, current_col * 2 + 1);
    refresh();
}


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

// 전적 파일 읽기
int load_all_records(PlayerRecord **out_records) {
    FILE *f = fopen(RECORD_FILE, "rb");
    if (!f) { *out_records = NULL; return 0; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    int cnt = sz / sizeof(PlayerRecord);
    rewind(f);
    *out_records = malloc(cnt * sizeof(PlayerRecord));
    fread(*out_records, sizeof(PlayerRecord), cnt, f);
    fclose(f);
    return cnt;
}

// 전적 파일 저장 (덮어쓰기)
void save_all_records(PlayerRecord *recs, int cnt) {
    FILE *f = fopen(RECORD_FILE, "wb");
    if (!f) return;
    fwrite(recs, sizeof(PlayerRecord), cnt, f);
    fclose(f);
}

// 이름으로 레코드 찾거나 새로 추가
PlayerRecord *get_or_create_record(PlayerRecord **recs, int *cnt, const char *name) {
    for (int i = 0; i < *cnt; i++) {
        if (strcmp((*recs)[i].name, name) == 0) {
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

