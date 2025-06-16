#include <vector>
#include "game.h"
#include <ncurses.h>
#include <locale.h>
#include <stdexcept>
#include <iostream>

using namespace std;

// RAII 패턴을 위한 ncurses 초기화 래퍼
class NcursesInitializer {
private:
    bool initialized = false;
    
public:
    NcursesInitializer() {
        if (!initscr()) {
            throw std::runtime_error("Failed to initialize ncurses");
        }
        initialized = true;
        
        raw();
        keypad(stdscr, TRUE);
        noecho();
        curs_set(0);
        nodelay(stdscr, TRUE);
        
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_GREEN, COLOR_BLACK);
            init_pair(2, COLOR_YELLOW, COLOR_BLACK);
            init_pair(3, COLOR_RED, COLOR_BLACK);
        }
    }
    
    ~NcursesInitializer() {
        if (initialized) {
            endwin();
        }
    }
    
    // 복사 방지
    NcursesInitializer(const NcursesInitializer&) = delete;
    NcursesInitializer& operator=(const NcursesInitializer&) = delete;
};

void validateTerminalSize() {
    int term_rows, term_cols;
    getmaxyx(stdscr, term_rows, term_cols);
    
    // 최소 요구사항을 대폭 완화 (게임 기본 동작에 필요한 최소 크기만)
    int min_rows = 15;  // 기존 25에서 15로 완화
    int min_cols = 50;  // 기존 80에서 50으로 완화
    
    if (term_rows < min_rows || term_cols < min_cols) {
        // 예외를 던지지 않고 경고만 표시
        clear();
        attron(A_BOLD | A_BLINK);
        mvprintw(term_rows/2-1, (term_cols-40)/2, "WARNING: Terminal size is small!");
        attroff(A_BOLD | A_BLINK);
        mvprintw(term_rows/2, (term_cols-45)/2, "Recommended: %dx%d, Current: %dx%d", min_cols, min_rows, term_cols, term_rows);
        mvprintw(term_rows/2+1, (term_cols-35)/2, "Game may not display perfectly.");
        mvprintw(term_rows/2+2, (term_cols-25)/2, "Press any key to continue...");
        refresh();
        getch();
        clear();
    }
}

void drawSnakeArt(int start_row, int start_col) {
    // 더 명확한 SNAKE GAME 아스키 아트
    mvprintw(start_row + 0, start_col,  "   _____  _   _    _    _  __ _____  _____   ");
    mvprintw(start_row + 1, start_col,  "  / ____|| \\ | |  / \\  | |/ /| ____||  __ \\");
    mvprintw(start_row + 2, start_col,  " | (___  |  \\| | / _ \\ | ' / |  _|  | |  | |");
    mvprintw(start_row + 3, start_col,  "  \\___ \\ | . ` |/ ___ \\|  <  | |___ | |__| |");
    mvprintw(start_row + 4, start_col,  "  ____/ ||_|\\_/_/   \\_\\_|\\_\\ |_____||_____/ ");
    mvprintw(start_row + 5, start_col,  "             S N A K E   G A M E            ");
}

void drawMainMenu(int selectedOption) {
    clear();
    int term_rows, term_cols;
    getmaxyx(stdscr, term_rows, term_cols);

    // 아트와 메뉴의 크기
    int art_height = 7;
    int art_width = 44;
    int menu_height = 7;
    int menu_width = 38;
    int total_height = art_height + menu_height + 4;
    int total_width = (art_width > menu_width ? art_width : menu_width) + 4;

    // 터미널이 너무 작으면 안내
    if (term_rows < total_height + 2 || term_cols < total_width + 2) {
        clear();
        mvprintw(term_rows/2, (term_cols-30)/2, "터미널 창을 더 크게 해주세요!");
        mvprintw(term_rows/2+1, (term_cols-38)/2, "(최소 %d x %d 이상 필요)", total_width, total_height);
        refresh();
        return;
    }

    int start_row = (term_rows - total_height) / 2;
    int start_col = (term_cols - total_width) / 2;

    // 아트 출력
    drawSnakeArt(start_row, start_col + (total_width-art_width)/2);

    // 메뉴 테두리
    int menu_start_row = start_row + art_height + 2;
    int menu_start_col = start_col + (total_width-menu_width)/2;
    for(int i = 0; i < menu_width; i++) {
        mvaddch(menu_start_row, menu_start_col + i, ACS_HLINE);
        mvaddch(menu_start_row + menu_height, menu_start_col + i, ACS_HLINE);
    }
    for(int i = 1; i < menu_height; i++) {
        mvaddch(menu_start_row + i, menu_start_col, ACS_VLINE);
        mvaddch(menu_start_row + i, menu_start_col + menu_width - 1, ACS_VLINE);
    }
    mvaddch(menu_start_row, menu_start_col, ACS_ULCORNER);
    mvaddch(menu_start_row, menu_start_col + menu_width - 1, ACS_URCORNER);
    mvaddch(menu_start_row + menu_height, menu_start_col, ACS_LLCORNER);
    mvaddch(menu_start_row + menu_height, menu_start_col + menu_width - 1, ACS_LRCORNER);

    // 메뉴 옵션
    attron(A_BOLD);
    mvprintw(menu_start_row + 1, menu_start_col + (menu_width-9)/2, "MAIN MENU");
    attroff(A_BOLD);
    for(int i = 1; i <= 3; i++) {
        if(i == selectedOption) {
            attron(A_STANDOUT);
            mvprintw(menu_start_row + 2 + i, menu_start_col + (menu_width-16)/2, "> %s", 
                i == 1 ? "Play Game" :
                i == 2 ? "How to Play" : "Exit");
            attroff(A_STANDOUT);
        } else {
            mvprintw(menu_start_row + 2 + i, menu_start_col + (menu_width-16)/2, "  %s", 
                i == 1 ? "Play Game" :
                i == 2 ? "How to Play" : "Exit");
        }
    }
    
    // 조작 안내
    mvprintw(menu_start_row + menu_height, menu_start_col + (menu_width-30)/2, "Use ↑↓ to move, Enter to select");
    
    // 제작자 정보 추가
    attron(A_BOLD);
    mvprintw(menu_start_row + menu_height + 2, menu_start_col + (menu_width-14)/2, "made by Team 10");
    attroff(A_BOLD);
    
    refresh();
}

void showHowToPlay() {
    try {
        int term_rows, term_cols;
        getmaxyx(stdscr, term_rows, term_cols);
        int box_height = 16;
        int box_width = 60;
        int start_row = (term_rows - box_height) / 2;
        int start_col = (term_cols - box_width) / 2;
        if (term_rows < box_height + 2 || term_cols < box_width + 2) {
            clear();
            mvprintw(term_rows/2, (term_cols-30)/2, "터미널 창을 더 크게 해주세요!");
            mvprintw(term_rows/2+1, (term_cols-38)/2, "(최소 %d x %d 이상 필요)", box_width, box_height);
            refresh();
            getch();
            clear();
            refresh();
            return;
        }
        
        // RAII 패턴으로 윈도우 관리
        class WindowRAII {
        private:
            WINDOW* win;
        public:
            WindowRAII(int h, int w, int y, int x) : win(newwin(h, w, y, x)) {
                if (!win) throw std::runtime_error("Failed to create window");
            }
            ~WindowRAII() { if (win) delwin(win); }
            WINDOW* get() const { return win; }
        };
        
        WindowRAII howto(box_height, box_width, start_row, start_col);
        
        box(howto.get(), 0, 0);
        wattron(howto.get(), A_BOLD);
        mvwprintw(howto.get(), 1, (box_width-13)/2, "HOW TO PLAY");
        wattroff(howto.get(), A_BOLD);
        mvwprintw(howto.get(), 3, 3, "Controls:");
        mvwprintw(howto.get(), 4, 8, "↑↓←→ : Move the snake");
        mvwprintw(howto.get(), 5, 8, "Enter : Select menu option");
        mvwprintw(howto.get(), 7, 3, "Items:");
        mvwprintw(howto.get(), 8, 8, "+ : Growth Item (Increase length)");
        mvwprintw(howto.get(), 9, 8, "- : Poison Item (Decrease length)");
        mvwprintw(howto.get(), 10, 8, "T : Time Item (Speed boost)");
        mvwprintw(howto.get(), 11, 8, "G : Gate (Teleport)");
        mvwprintw(howto.get(), 13, 3, "* Complete all missions to advance stage!");
        mvwprintw(howto.get(), box_height-2, (box_width-32)/2, "Press any key to return to main menu");
        wrefresh(howto.get());
        getch();
        clear();
        refresh();
    } catch (const std::exception& e) {
        clear();
        mvprintw(0, 0, "Error in How to Play: %s", e.what());
        refresh();
        getch();
        clear();
        refresh();
    }
}

int main() {
    setlocale(LC_ALL, "");
    
    try {
        NcursesInitializer ncursesInitializer;
        validateTerminalSize();
        
        int inputCharacter, menuOptionSelected = 1;
        int lastMenuOption = 0; // 이전 메뉴 옵션을 추적
        Game gameInstance;
        
        // 초기 메뉴 그리기
        drawMainMenu(menuOptionSelected);
        
        while(1) {
            inputCharacter = getch();
            
            // 키 입력이 없으면 계속 대기
            if (inputCharacter == ERR) {
                usleep(50000); // 50ms 대기
                continue;
            }
            
            switch(inputCharacter) {
                case KEY_UP:
                    menuOptionSelected = (menuOptionSelected > 1) ? menuOptionSelected - 1 : 3;
                    break;
                case KEY_DOWN:
                    menuOptionSelected = (menuOptionSelected < 3) ? menuOptionSelected + 1 : 1;
                    break;
                case 10: // Enter key
                    if(menuOptionSelected == 1) {
                        gameInstance = Game();
                        gameInstance.refreshScreen();
                        // 게임에서 돌아온 후 메뉴 다시 그리기
                        drawMainMenu(menuOptionSelected);
                        lastMenuOption = menuOptionSelected;
                    }
                    else if(menuOptionSelected == 2) {
                        nodelay(stdscr, FALSE);
                        showHowToPlay();
                        nodelay(stdscr, TRUE);
                        // How to Play에서 돌아온 후 메뉴 다시 그리기
                        drawMainMenu(menuOptionSelected);
                        lastMenuOption = menuOptionSelected;
                    }
                    else if(menuOptionSelected == 3) {
                        return 0;
                    }
                    continue; // Enter 키 처리 후 메뉴 다시 그리기 건너뛰기
            }
            
            // 메뉴 옵션이 변경된 경우에만 다시 그리기
            if (menuOptionSelected != lastMenuOption) {
                drawMainMenu(menuOptionSelected);
                lastMenuOption = menuOptionSelected;
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
