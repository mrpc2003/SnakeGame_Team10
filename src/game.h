#ifndef GAME_H
#define GAME_H

#include "map.h"
#include "block.h"
#include <iostream>
#include <vector>
#include <ncurses.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <chrono>
#include <queue>
#include <set>
#include <stdexcept>
#include <memory>

using namespace std;

// RAII 패턴을 위한 ncurses 윈도우 래퍼 클래스
class WindowWrapper {
private:
    WINDOW* window;
    
public:
    WindowWrapper(int height, int width, int starty, int startx) 
        : window(newwin(height, width, starty, startx)) {
        if (!window) {
            throw std::runtime_error("Failed to create ncurses window");
        }
    }
    
    ~WindowWrapper() {
        if (window) {
            delwin(window);
        }
    }
    
    // 복사 방지
    WindowWrapper(const WindowWrapper&) = delete;
    WindowWrapper& operator=(const WindowWrapper&) = delete;
    
    // 이동 생성자/대입 연산자
    WindowWrapper(WindowWrapper&& other) noexcept : window(other.window) {
        other.window = nullptr;
    }
    
    WindowWrapper& operator=(WindowWrapper&& other) noexcept {
        if (this != &other) {
            if (window) {
                delwin(window);
            }
            window = other.window;
            other.window = nullptr;
        }
        return *this;
    }
    
    WINDOW* get() const { return window; }
    operator WINDOW*() const { return window; }
};

class Game
{
public:
    Game();
    ~Game();

    void refreshScreen();
    bool update(int &growthItemTimer, int &poisonItemTimer, int &timeItemTimer, int previousDirection = 0);
    bool isValid(int /*previousDirection*/);
    void generateRandCoord(int &row, int &col, bool shouldIncludeWall = false);
    void generateGate();
    void generateItems();
    void generateTItem();
    void generateGItem();
    void generatePItem();

private:
    Map gameMap;
    int currentStage = 1;
    int gateActiveDuration = 0;
    int growthItemCount = 0;
    int poisonItemCount = 0;
    int gatesUsedCount = 0;
    int maxSnakeLength = 3;
    int gameTimerSeconds = 0;
    int gameSpeedDelay = 200;
    float speedMultiplier = 1;
    int speedBoostTimer = 0;
    
    // 아이템 타이머들을 멤버 변수로 추가
    int growthItemTimer = 0;
    int poisonItemTimer = 0;
    int timeItemTimer = 0;

    char missionSnakeLengthStatus = ' ';
    char missionGrowthItemStatus = ' ';
    char missionPoisonItemStatus = ' ';
    char missionGateUseStatus = ' ';
    string gameOverReason = "";

    bool allMissionsCompleted = false;
    bool ncursesInitialized = false;

    void initializeNcurses();
    void cleanupNcurses();
    void drawBoard(WINDOW* board);
    void drawScore(WINDOW* score);
    void drawMission(WINDOW* mission);
    void handleGameOver();
    void handleMissionComplete();
    void checkMissions();
    void processInput(int key);
    void updateTimers(int &growthItemTimer, int &poisonItemTimer, int &timeItemTimer);
    void handleGateCollision();
    void handleItemCollisions();
    void resetCurrentStage();
    void goToNextStage();
    MapType getMapTypeForStage(int stage);
    void showEndingScreen();
    
    // 스테이지별 미션 목표 관리
    struct MissionTargets {
        int snakeLength;
        int growthItems;
        int poisonItems;
        int gateUses;
    };
    MissionTargets getMissionTargets(int stage);
    
    // 안전한 벡터 접근을 위한 헬퍼 함수들
    bool isSnakeBodySizeValid(size_t requiredSize) const;
    void safeAddSnakeBody();
    bool safeRemoveSnakeBody();
    void validateTerminalSize();
};

Game::Game()
{
    try {
        gameMap = Map(21, 41, 2);
        initializeNcurses();
        validateTerminalSize();
        generateItems();
        generateGate();
        
        // 초기 게임 속도를 0.2초(200ms)로 설정
        gameSpeedDelay = 200;
    } catch (const std::exception& e) {
        cleanupNcurses();
        throw;
    }
}

Game::~Game()
{
    cleanupNcurses();
}

void Game::initializeNcurses()
{
    if (!initscr()) {
        throw std::runtime_error("Failed to initialize ncurses");
    }
    ncursesInitialized = true;
    
    if (!has_colors()) {
        cleanupNcurses();
        throw std::runtime_error("Terminal does not support colors");
    }
    
    clear();
    noecho();
    cbreak();
    curs_set(0);
    start_color();
    
    // 색상 조합: 1=벽, 2=무적벽, 3=스네이크 머리(노란), 4=스네이크 몸통(밝은 초록), 9=꼬리(밝은 노랑)
    init_pair(1, COLOR_WHITE, COLOR_WHITE);   // Wall
    init_pair(2, COLOR_BLACK, COLOR_WHITE);   // Immuned Wall
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);  // Snake Head (노란색)
    init_pair(4, COLOR_GREEN, COLOR_BLACK);   // Snake Body (초록)
    init_pair(5, COLOR_BLUE, COLOR_BLUE);     // Growth Item
    init_pair(6, COLOR_RED, COLOR_RED);       // Poison Item
    init_pair(7, COLOR_BLACK, COLOR_MAGENTA); // GATE
    init_pair(8, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(9, COLOR_YELLOW, COLOR_GREEN);  // Snake Tail (밝은 노랑/초록)
    
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    srand(static_cast<unsigned int>(time(nullptr)));
}

void Game::cleanupNcurses()
{
    if (ncursesInitialized) {
        endwin();
        ncursesInitialized = false;
    }
}

void Game::validateTerminalSize()
{
    int term_rows, term_cols;
    getmaxyx(stdscr, term_rows, term_cols);
    
    // 최소 요구사항 완화 (게임 플레이 가능한 최소 크기)
    int min_width = gameMap.mapSize.width + 15;   // 맵 + 최소 UI 공간
    int min_height = gameMap.mapSize.height + 3;  // 맵 + 최소 여유 공간
    
    if (term_rows < min_height || term_cols < min_width) {
        // 예외를 던지지 않고 경고만 표시
        clear();
        refresh();
        
        // 경고 메시지를 안전한 위치에 표시
        int msg_row = (term_rows > 5) ? term_rows/2 : 1;
        int msg_col = (term_cols > 30) ? (term_cols-30)/2 : 1;
        
        if (term_rows >= 5 && term_cols >= 30) {
            mvprintw(msg_row-1, msg_col, "WARNING: Terminal too small!");
            mvprintw(msg_row, msg_col, "Required: %dx%d", min_width, min_height);
            mvprintw(msg_row+1, msg_col, "Current: %dx%d", term_cols, term_rows);
            mvprintw(msg_row+2, msg_col, "Press any key to continue...");
        } else {
            mvprintw(1, 1, "Terminal too small!");
            mvprintw(2, 1, "Press any key...");
        }
        
        refresh();
        getch();
        clear();
    }
}

bool Game::isSnakeBodySizeValid(size_t requiredSize) const
{
    return gameMap.snakeHeadObject.snakeBodySegments.size() >= requiredSize;
}

void Game::safeAddSnakeBody()
{
    if (!isSnakeBodySizeValid(2)) {
        // 몸통이 2개 미만이면 기본 위치에 추가
        Coord headPos = gameMap.snakeHeadObject.coord;
        gameMap.snakeHeadObject.snakeBodySegments.push_back(
            SnakeBody(headPos.row + 1, headPos.col));
        return;
    }
    
    auto& segments = gameMap.snakeHeadObject.snakeBodySegments;
    auto last = segments.end() - 1;
    auto sec = segments.end() - 2;
    
    segments.push_back(SnakeBody(
        last->coord.row - (sec->coord.row - last->coord.row),
        last->coord.col - (sec->coord.col - last->coord.col)
    ));
}

bool Game::safeRemoveSnakeBody()
{
    if (gameMap.snakeHeadObject.snakeBodySegments.size() <= 3) {
        return false; // 최소 길이 유지
    }
    
    gameMap.snakeHeadObject.snakeBodySegments.pop_back();
    return true;
}

void Game::refreshScreen()
{
    try {
        while (true) {
            clear();
            
            int term_rows, term_cols;
            getmaxyx(stdscr, term_rows, term_cols);
            
            // 터미널 크기에 맞춰 UI 레이아웃 동적 조정
            int board_width = gameMap.mapSize.width + 2;
            int board_height = gameMap.mapSize.height + 2;
            
            // UI 윈도우 크기 계산 (터미널 크기에 따라 조정)
            int ui_width = min(27, term_cols - board_width - 2);
            if (ui_width < 15) ui_width = 15; // 최소 UI 너비
            
            int score_height = min(9, (term_rows - 2) / 2);
            int mission_height = min(9, term_rows - score_height - 2);
            
            // UI 윈도우 위치 계산
            int ui_x = min(board_width + 2, term_cols - ui_width);
            if (ui_x + ui_width > term_cols) ui_x = term_cols - ui_width;
            if (ui_x < 0) ui_x = 0;
            
            int score_y = 0;
            int mission_y = score_height + 1;
            if (mission_y + mission_height > term_rows) {
                mission_y = term_rows - mission_height;
                if (mission_y < 0) mission_y = 0;
            }
            
            // 게임 보드가 터미널 크기를 초과하는 경우 조정
            if (board_width > term_cols || board_height > term_rows) {
                // 터미널이 너무 작은 경우 오버레이 모드로 전환
                clear();
                mvprintw(term_rows/2, (term_cols-30)/2, "Terminal too small for game board!");
                mvprintw(term_rows/2+1, (term_cols-25)/2, "Please resize terminal window");
                mvprintw(term_rows/2+2, (term_cols-20)/2, "Press 'q' to quit");
                refresh();
                
                int key = getch();
                if (key == 'q' || key == 'Q') {
                    return;
                }
                continue;
            }
            
            // RAII 패턴으로 윈도우 자동 관리
            WindowWrapper board(board_height, board_width, 0, 0);
            WindowWrapper score(score_height, ui_width, score_y, ui_x);
            WindowWrapper mission(mission_height, ui_width, mission_y, ui_x);

            box(board.get(), 0, 0);
            box(score.get(), 0, 0);
            box(mission.get(), 0, 0);

            drawBoard(board.get());
            drawScore(score.get());
            drawMission(mission.get());

            refresh();
            wrefresh(board.get());
            wrefresh(score.get());
            wrefresh(mission.get());

            int key = getch();
            int previousDirection = gameMap.snakeHeadObject.currentDirection;
            processInput(key);

            if (allMissionsCompleted) {
                handleMissionComplete();
                continue;
            }

            if (!update(growthItemTimer, poisonItemTimer, timeItemTimer, previousDirection)) {
                handleGameOver();
                continue;
            }

            // 스네이크가 방향을 가지고 있을 때만 타이머 업데이트 (실제로 움직일 때만)
            if (gameMap.snakeHeadObject.currentDirection != -1) {
                updateTimers(growthItemTimer, poisonItemTimer, timeItemTimer);
                gameTimerSeconds++;
            }

            usleep(1000 * static_cast<useconds_t>((float)gameSpeedDelay / speedMultiplier));
        }
    } catch (const std::exception& e) {
        cleanupNcurses();
        std::cerr << "Game error: " << e.what() << std::endl;
        throw;
    }
}

void Game::drawBoard(WINDOW* board)
{
    // Draw immune walls
    for (const auto& wall : gameMap.immuneWalls) {
                wattron(board, COLOR_PAIR(2));
        mvwaddch(board, wall.coord.row, wall.coord.col, '+');
                wattroff(board, COLOR_PAIR(2));
            }
    // Draw regular walls
    for (const auto& wall : gameMap.regularWalls) {
                wattron(board, COLOR_PAIR(2));
        mvwaddch(board, wall.coord.row, wall.coord.col, ' ');
                wattroff(board, COLOR_PAIR(2));
            }
    // Draw snake body (꼬리만 따로 색상)
    int bodySize = gameMap.snakeHeadObject.snakeBodySegments.size();
    for (int i = 0; i < bodySize; ++i) {
        if (i == bodySize-1) {
            wattron(board, COLOR_PAIR(9)); // 꼬리
            mvwaddch(board, gameMap.snakeHeadObject.snakeBodySegments[i].coord.row, gameMap.snakeHeadObject.snakeBodySegments[i].coord.col, 'o');
            wattroff(board, COLOR_PAIR(9));
        } else {
                wattron(board, COLOR_PAIR(4));
            mvwaddch(board, gameMap.snakeHeadObject.snakeBodySegments[i].coord.row, gameMap.snakeHeadObject.snakeBodySegments[i].coord.col, 'O');
                wattroff(board, COLOR_PAIR(4));
            }
    }
    // Draw gates
    for (const auto& gate : gameMap.gameGates) {
                wattron(board, COLOR_PAIR(7));
        mvwaddch(board, gate.coord.row, gate.coord.col, ' ');
                wattroff(board, COLOR_PAIR(7));
            }
    // Draw snake head (노란색, 방향 문자)
    char headChar = '>';
    switch (gameMap.snakeHeadObject.currentDirection) {
        case 1: headChar = '^'; break;
        case 2: headChar = '<'; break;
        case 3: headChar = '>'; break;
        case 4: headChar = 'v'; break;
        default: headChar = 'O'; break;
    }
    wattron(board, COLOR_PAIR(3) | A_BOLD);
    mvwaddch(board, gameMap.snakeHeadObject.coord.row, gameMap.snakeHeadObject.coord.col, headChar);
    wattroff(board, COLOR_PAIR(3) | A_BOLD);
    // Draw items
            wattron(board, COLOR_PAIR(5));
    mvwaddch(board, gameMap.growthItemObject.coord.row, gameMap.growthItemObject.coord.col, '+');
            wattroff(board, COLOR_PAIR(5));
            wattron(board, COLOR_PAIR(6));
    mvwaddch(board, gameMap.poisonItemObject.coord.row, gameMap.poisonItemObject.coord.col, '-');
            wattroff(board, COLOR_PAIR(6));
            wattron(board, COLOR_PAIR(8));
    mvwaddch(board, gameMap.timeItemObject.coord.row, gameMap.timeItemObject.coord.col, 'T');
            wattroff(board, COLOR_PAIR(8));
}

void Game::drawScore(WINDOW* score)
{
    int height, width;
    getmaxyx(score, height, width);
    
    // 윈도우 크기에 맞춰 내용 조정
    if (height >= 8 && width >= 20) {
        // 표준 크기
        mvwprintw(score, 1, 1, "*******Score Board*******");
        mvwprintw(score, 2, 1, " Stage: %d/4", currentStage);
        mvwprintw(score, 3, 1, " B: %d/%d", (int)gameMap.snakeHeadObject.snakeBodySegments.size(), maxSnakeLength);
        mvwprintw(score, 4, 1, " +: %d", growthItemCount);
        mvwprintw(score, 5, 1, " -: %d", poisonItemCount);
        mvwprintw(score, 6, 1, " G: %d", gatesUsedCount);
        mvwprintw(score, 7, 1, " time: %d", gameTimerSeconds / (1000 / gameSpeedDelay));
    } else if (height >= 6 && width >= 15) {
        // 중간 크기
        mvwprintw(score, 1, 1, "Score Board");
        mvwprintw(score, 2, 1, "Stage: %d/4", currentStage);
        mvwprintw(score, 3, 1, "B:%d +:%d", (int)gameMap.snakeHeadObject.snakeBodySegments.size(), growthItemCount);
        mvwprintw(score, 4, 1, "-:%d G:%d", poisonItemCount, gatesUsedCount);
        mvwprintw(score, 5, 1, "Time: %d", gameTimerSeconds / (1000 / gameSpeedDelay));
    } else if (height >= 4 && width >= 10) {
        // 작은 크기
        mvwprintw(score, 1, 1, "S:%d/4", currentStage);
        mvwprintw(score, 2, 1, "L:%d", (int)gameMap.snakeHeadObject.snakeBodySegments.size());
    } else {
        // 최소 크기
        mvwprintw(score, 1, 1, "S%d", currentStage);
        mvwprintw(score, 2, 1, "L%d", (int)gameMap.snakeHeadObject.snakeBodySegments.size());
    }
}

void Game::drawMission(WINDOW* mission)
{
    int height, width;
    getmaxyx(mission, height, width);
    
    // 현재 스테이지의 미션 목표 가져오기
    MissionTargets targets = getMissionTargets(currentStage);
    
    // 윈도우 크기에 맞춰 내용 조정
    if (height >= 7 && width >= 25) {
        // 표준 크기
        mvwprintw(mission, 1, 1, "******Mission Board******");
        mvwprintw(mission, 2, 1, " Stage %d: %s", currentStage, 
            currentStage == 1 ? "BASIC" :
            currentStage == 2 ? "MAZE" :
            currentStage == 3 ? "ISLANDS" : "CROSS");
        mvwprintw(mission, 3, 1, " B: %d / %d (%c) ", targets.snakeLength, (int)gameMap.snakeHeadObject.snakeBodySegments.size(), missionSnakeLengthStatus);
        mvwprintw(mission, 4, 1, " +: %d / %d (%c) ", targets.growthItems, growthItemCount, missionGrowthItemStatus);
        mvwprintw(mission, 5, 1, " -: %d / %d (%c) ", targets.poisonItems, poisonItemCount, missionPoisonItemStatus);
        mvwprintw(mission, 6, 1, " G: %d / %d (%c) ", targets.gateUses, gatesUsedCount, missionGateUseStatus);
    } else if (height >= 5 && width >= 15) {
        // 중간 크기
        mvwprintw(mission, 1, 1, "Mission");
        mvwprintw(mission, 2, 1, "B:%d/%d(%c)", targets.snakeLength, (int)gameMap.snakeHeadObject.snakeBodySegments.size(), missionSnakeLengthStatus);
        mvwprintw(mission, 3, 1, "+:%d/%d(%c)", targets.growthItems, growthItemCount, missionGrowthItemStatus);
        mvwprintw(mission, 4, 1, "G:%d/%d(%c)", targets.gateUses, gatesUsedCount, missionGateUseStatus);
    } else if (height >= 3 && width >= 10) {
        // 작은 크기
        mvwprintw(mission, 1, 1, "Mission");
        mvwprintw(mission, 2, 1, "B:%d+:%d", (int)gameMap.snakeHeadObject.snakeBodySegments.size(), growthItemCount);
    } else {
        // 최소 크기
        mvwprintw(mission, 1, 1, "M");
    }
}

void Game::processInput(int key)
{
    int newDirection = -1;
    
    switch (key) {
        case KEY_UP:
            newDirection = 1;
            break;
        case KEY_DOWN:
            newDirection = 4;
            break;
        case KEY_RIGHT:
            newDirection = 3;
            break;
        case KEY_LEFT:
            newDirection = 2;
            break;
        // 디버그: D키로 미션 강제 클리어
        case 'd':
        case 'D':
            {
                MissionTargets targets = getMissionTargets(currentStage);
                growthItemCount = targets.growthItems;
                poisonItemCount = targets.poisonItems;
                gatesUsedCount = targets.gateUses;
                gameMap.snakeHeadObject.snakeBodySegments.resize(targets.snakeLength);
                checkMissions();
            }
            break;
        // 디버그: E키로 엔딩 바로 보기
        case 'e':
        case 'E':
            showEndingScreen();
            break;
        // 디버그: 1~4키로 스테이지 이동 (4스테이지까지만)
        case '1': case '2': case '3': case '4':
            currentStage = key - '0';
            resetCurrentStage();
            break;
    }
    
    // 방향키가 입력된 경우에만 처리
    if (newDirection != -1) {
        int currentDir = gameMap.snakeHeadObject.currentDirection;
        
        // 1. 같은 방향 키 입력은 무시
        if (currentDir == newDirection) {
            return;
        }
        
        // 2. 역방향 이동 검사 (현재 방향이 설정되어 있을 때만)
        if (currentDir != -1) {
            bool isOpposite = false;
            switch (currentDir) {
                case 1: isOpposite = (newDirection == 4); break; // 위 ↔ 아래
                case 2: isOpposite = (newDirection == 3); break; // 왼쪽 ↔ 오른쪽
                case 3: isOpposite = (newDirection == 2); break; // 오른쪽 ↔ 왼쪽
                case 4: isOpposite = (newDirection == 1); break; // 아래 ↔ 위
            }
            
            if (isOpposite) {
                // 역방향 이동 시도를 표시하기 위해 특별한 값 설정
                gameMap.snakeHeadObject.currentDirection = -2; // 역방향 시도 표시
                return;
            }
        }
        
        // 3. 유효한 방향 변경
        gameMap.snakeHeadObject.currentDirection = newDirection;
    }
}

void Game::updateTimers(int &growthItemTimer, int &poisonItemTimer, int &timeItemTimer)
{
    growthItemTimer++;
    poisonItemTimer++;
    timeItemTimer++;

    if (speedBoostTimer > 0) {
        speedBoostTimer--;
        if (speedBoostTimer == 0) {
            speedMultiplier = 1;
        }
    }
}

void Game::handleGameOver()
{
    try {
        int term_rows, term_cols;
        getmaxyx(stdscr, term_rows, term_cols);
        
        // 터미널이 너무 작은 경우 간단한 메시지만 표시
        if (term_rows < 10 || term_cols < 30) {
            clear();
            mvprintw(term_rows/2-1, (term_cols-12)/2, "Game Over!");
            mvprintw(term_rows/2, (term_cols-15)/2, "Stage: %d", currentStage);
            if (!gameOverReason.empty()) {
                mvprintw(term_rows/2+1, (term_cols-20)/2, "Reason: %.15s", gameOverReason.c_str());
            }
            mvprintw(term_rows/2+2, (term_cols-15)/2, "R:retry E:exit");
            refresh();
            
            while (true) {
                char key = getch();
                if (key == 'e' || key == 'E') {
                    cleanupNcurses();
                    exit(0);
                }
                if (key == 'r' || key == 'R') {
                    resetCurrentStage();
                    break;
                }
                usleep(100000);
            }
            return;
        }
        
        // 기존 코드 (터미널이 충분히 큰 경우)
        int reason_max_width = 22;
        std::string reason = gameOverReason;
        
        // 게임 오버 이유가 없으면 기본 메시지 사용
        if (reason.empty()) {
            reason = "Unknown error occurred.";
        }
        
        std::vector<std::string> reason_lines;
        std::string prefix = "Reason: ";
        size_t prefix_len = prefix.length();
        size_t pos = 0;
        if (reason.length() <= reason_max_width - prefix_len) {
            reason_lines.push_back(reason);
        } else {
            reason_lines.push_back(reason.substr(0, reason_max_width - prefix_len));
            pos = reason_max_width - prefix_len;
            while (pos < reason.length()) {
                reason_lines.push_back(reason.substr(pos, reason_max_width));
                pos += reason_max_width;
            }
        }
        size_t max_line_len = 0;
        for (const auto& line : reason_lines) {
            size_t len = line.length();
            if (max_line_len < len) max_line_len = len;
        }
        int min_width = 27;
        int win_width = std::max(min_width, (int)max_line_len + 10);
        if (win_width > term_cols - 2) win_width = term_cols - 2;
        // 세로 크기: 위여백(2) + 제목(1) + 여백(1) + 스테이지(1) + 여백(1) + Reason(줄수) + 여백(1) + 점수(1) + 여백(1) + 안내문구(2) + 아래여백(1)
        int win_height = 2 + 1 + 1 + 1 + 1 + (int)reason_lines.size() + 1 + 1 + 1 + 2 + 1;
        if (win_height > term_rows) {
            win_height = term_rows;
            int max_reason_lines = win_height - (2+1+1+1+1+1+1+2+1);
            if (max_reason_lines < 0) max_reason_lines = 0;
            if ((int)reason_lines.size() > max_reason_lines) {
                reason_lines.resize(max_reason_lines);
                if (!reason_lines.empty()) {
                    std::string& last = reason_lines.back();
                    if (last.length() > 3) last.replace(last.length()-3, 3, "...");
                    else last += "...";
                }
            }
        }
        
        // 윈도우 위치 계산 (중앙에 배치)
        int win_starty = (term_rows - win_height) / 2;
        int win_startx = (term_cols - win_width) / 2;
        if (win_starty < 0) win_starty = 0;
        if (win_startx < 0) win_startx = 0;
        
        WindowWrapper score(win_height, win_width, win_starty, win_startx);
        
        while (true) {
            wclear(score.get());
            box(score.get(), 0, 0);
            int y = 2;
            mvwprintw(score.get(), y++, 2, "*******Game Over*******");
            y++; // 여백
            mvwprintw(score.get(), y++, 4, "Stage: %d", currentStage);
            y++; // 여백
            for (size_t i = 0; i < reason_lines.size(); ++i) {
                if (i == 0)
                    mvwprintw(score.get(), y++, 4, "Reason: %s", reason_lines[i].c_str());
                else
                    mvwprintw(score.get(), y++, 4, "        %s", reason_lines[i].c_str());
            }
            y++; // 여백
            mvwprintw(score.get(), y++, 4, "Score: %d", maxSnakeLength);
            y++; // 여백
            // 안내문구를 박스의 마지막에서 3, 2번째 줄에 위치
            mvwprintw(score.get(), win_height-3, 4, "Press 'R' to retry");
            mvwprintw(score.get(), win_height-2, 4, "Press 'E' to exit");
            wrefresh(score.get());

            char key = getch();
            if (key == 'e' || key == 'E') {
                cleanupNcurses();
                exit(0);
            }
            if (key == 'r' || key == 'R') {
                resetCurrentStage();
                break;
            }
            usleep(100000);
        }
    } catch (const std::exception& e) {
        cleanupNcurses();
        std::cerr << "Game Over screen error: " << e.what() << std::endl;
        throw;
    }
}

void Game::handleMissionComplete()
{
    try {
        int term_rows, term_cols;
        getmaxyx(stdscr, term_rows, term_cols);
        
        // 터미널이 너무 작은 경우 간단한 메시지만 표시
        if (term_rows < 8 || term_cols < 25) {
            clear();
            mvprintw(term_rows/2-1, (term_cols-18)/2, "Mission Complete!");
            mvprintw(term_rows/2, (term_cols-15)/2, "Stage %d Clear!", currentStage);
            mvprintw(term_rows/2+1, (term_cols-12)/2, "R:next E:exit");
            refresh();
            
            while (true) {
                char key = getch();
                if (key == 'e' || key == 'E') {
                    cleanupNcurses();
                    exit(0);
                }
                if (key == 'r' || key == 'R') {
                    goToNextStage();
                    break;
                }
                usleep(100000);
            }
            return;
        }
        
        // 기존 코드 (터미널이 충분히 큰 경우) - 중앙에 배치
        int win_width = min(27, term_cols - 4);
        int win_height = min(9, term_rows - 4);
        int win_starty = (term_rows - win_height) / 2;
        int win_startx = (term_cols - win_width) / 2;
        
        WindowWrapper score(win_height, win_width, win_starty, win_startx);
        
        while (true) {
            wclear(score.get());
            box(score.get(), 0, 0);
            mvwprintw(score.get(), 1, 1, "***Mission Complete!***");
            mvwprintw(score.get(), 3, 2, "Stage %d Clear!", currentStage);
            mvwprintw(score.get(), 4, 2, "Next Stage: %d", currentStage + 1);
            mvwprintw(score.get(), 5, 2, "Max Length: %d", maxSnakeLength);
            mvwprintw(score.get(), 6, 2, "Press 'R' to continue");
            mvwprintw(score.get(), 7, 2, "Press 'E' to exit");
            wrefresh(score.get());

            char key = getch();
            if (key == 'e' || key == 'E') {
                cleanupNcurses();
                exit(0);
            }
            if (key == 'r' || key == 'R') {
                goToNextStage();
                break;
            }
            usleep(100000);
        }
    } catch (const std::exception& e) {
        cleanupNcurses();
        std::cerr << "Mission Complete screen error: " << e.what() << std::endl;
        throw;
    }
}

void Game::resetCurrentStage()
{
    gameMap = Map(21, 41, rand() % 4 + 2, getMapTypeForStage(currentStage), currentStage);
    gateActiveDuration = 0;
    growthItemCount = 0;
    poisonItemCount = 0;
    gatesUsedCount = 0;
    maxSnakeLength = 3;
    gameTimerSeconds = 0;
    speedMultiplier = 1;
    
    // 아이템 타이머들 초기화
    growthItemTimer = 0;
    poisonItemTimer = 0;
    timeItemTimer = 0;
    
    // 미션 상태 초기화
    missionSnakeLengthStatus = ' ';
    missionGrowthItemStatus = ' ';
    missionPoisonItemStatus = ' ';
    missionGateUseStatus = ' ';
    allMissionsCompleted = false;
    
    // 게임 오버 이유 초기화
    gameOverReason = "";
    
    generateItems();
    generateGate();
    
    // 스테이지별 게임 속도 설정 (점진적으로 빨라짐)
    switch(currentStage) {
        case 1: gameSpeedDelay = 250; break; // 가장 느림 (쉬움)
        case 2: gameSpeedDelay = 200; break; // 중간
        case 3: gameSpeedDelay = 170; break; // 빠름
        case 4: gameSpeedDelay = 150; break; // 가장 빠름 (어려움)
        default: gameSpeedDelay = 200; break;
    }
}

void Game::goToNextStage()
{
    currentStage++;
    if(currentStage > 4) {
        showEndingScreen();
        currentStage = 1;
    }
    resetCurrentStage();
}

void Game::checkMissions()
{
    MissionTargets targets = getMissionTargets(currentStage);
    
    missionSnakeLengthStatus = (static_cast<int>(gameMap.snakeHeadObject.snakeBodySegments.size()) >= targets.snakeLength) ? 'v' : ' ';
    missionGrowthItemStatus = (growthItemCount >= targets.growthItems) ? 'v' : ' ';
    missionPoisonItemStatus = (poisonItemCount >= targets.poisonItems) ? 'v' : ' ';
    missionGateUseStatus = (gatesUsedCount >= targets.gateUses) ? 'v' : ' ';

    allMissionsCompleted = (missionSnakeLengthStatus == 'v' && 
                          missionGrowthItemStatus == 'v' && 
                          missionPoisonItemStatus == 'v' && 
                          missionGateUseStatus == 'v');
}

bool Game::update(int &growthItemTimer, int &poisonItemTimer, int &timeItemTimer, int previousDirection)
{
    // 먼저 역방향 이동 검사
    if (gameMap.snakeHeadObject.currentDirection == -2) {
        gameOverReason = "Tried moving in the opposite direction.";
        return false; // 게임 종료
    }
    
    if (gateActiveDuration == 0)
    {
        gameMap.gameGates[0].isActive = false;
        gameMap.gameGates[1].isActive = false;
    }
    else
        gateActiveDuration--;
    
    if (gameMap.snakeHeadObject.currentDirection != -1)
    {
        gameMap.snakeHeadObject.snakeBodySegments.insert(gameMap.snakeHeadObject.snakeBodySegments.begin(), SnakeBody(gameMap.snakeHeadObject));
        gameMap.snakeHeadObject.snakeBodySegments.pop_back();
    }
    
    gameMap.snakeHeadObject.move();
    
    // 게이트 통과 처리
    for (size_t i = 0; i < gameMap.gameGates.size(); i++)
    {
        auto it = gameMap.gameGates.begin() + i;
        if (it->coord == gameMap.snakeHeadObject.coord)
        {
            it->isActive = true;
            gateActiveDuration = static_cast<int>(gameMap.snakeHeadObject.snakeBodySegments.size());
            auto other = (i == 0 ? gameMap.gameGates.begin() + 1 : gameMap.gameGates.begin());
            
            // 게이트 출구 방향 결정 및 안전한 출구 위치 계산
            int exitDirection = other->exitDirection;
            Coord exitPosition = other->coord;
            
            if (exitDirection == 6) // 자유 방향 (벽 중앙)
            {
                int inDir = gameMap.snakeHeadObject.currentDirection;
                int dirPriority[4];
                dirPriority[0] = inDir; // 진입 방향과 일치하는 방향
                // 시계 방향
                dirPriority[1] = (inDir == 1) ? 3 : (inDir == 2) ? 1 : (inDir == 3) ? 4 : 2;
                // 반시계 방향
                dirPriority[2] = (inDir == 1) ? 2 : (inDir == 2) ? 4 : (inDir == 3) ? 1 : 3;
                // 반대 방향
                dirPriority[3] = (inDir == 1) ? 4 : (inDir == 2) ? 3 : (inDir == 3) ? 2 : 1;
                
                // 가능한 방향 찾기
                bool foundValidExit = false;
                for (int k = 0; k < 4; ++k) {
                    int d = dirPriority[k];
                    Coord testPos = other->coord;
                    switch (d) {
                        case 1: testPos.row--; break;
                        case 2: testPos.col--; break;
                        case 3: testPos.col++; break;
                        case 4: testPos.row++; break;
                    }
                    
                    // 벽 충돌 검사
                    bool blocked = false;
                    for (const auto& w : gameMap.regularWalls) {
                        if (w.coord == testPos) { 
                            blocked = true; 
                            break; 
                        }
                    }
                    for (const auto& w : gameMap.immuneWalls) {
                        if (w.coord == testPos) { 
                            blocked = true; 
                            break; 
                        }
                    }
                    
                    // 맵 경계 검사
                    if (testPos.row < 1 || testPos.row >= gameMap.mapSize.height || 
                        testPos.col < 1 || testPos.col >= gameMap.mapSize.width) {
                        blocked = true;
                    }
                    
                    if (!blocked) {
                        exitDirection = d;
                        exitPosition = testPos;
                        foundValidExit = true;
                        break;
                    }
                }
                
                // 유효한 출구를 찾지 못한 경우 게이트 위에 그대로 둠
                if (!foundValidExit) {
                    exitDirection = inDir;
                    exitPosition = other->coord;
                }
            } else {
                // 고정 방향인 경우에도 출구 위치 계산
                switch (exitDirection) {
                    case 1: exitPosition.row--; break;
                    case 2: exitPosition.col--; break;
                    case 3: exitPosition.col++; break;
                    case 4: exitPosition.row++; break;
                }
                
                // 출구 위치가 막혀있는지 검사
                bool blocked = false;
                for (const auto& w : gameMap.regularWalls) {
                    if (w.coord == exitPosition) { 
                        blocked = true; 
                        break; 
                    }
                }
                for (const auto& w : gameMap.immuneWalls) {
                    if (w.coord == exitPosition) { 
                        blocked = true; 
                        break; 
                    }
                }
                
                // 맵 경계 검사
                if (exitPosition.row < 1 || exitPosition.row >= gameMap.mapSize.height || 
                    exitPosition.col < 1 || exitPosition.col >= gameMap.mapSize.width) {
                    blocked = true;
                }
                
                // 출구가 막혀있으면 게이트 위에 그대로 둠
                if (blocked) {
                    exitPosition = other->coord;
                }
            }
            
            // 스네이크를 안전한 출구 위치로 텔레포트
            gameMap.snakeHeadObject.coord = exitPosition;
            gameMap.snakeHeadObject.currentDirection = exitDirection;
            
            // 다른 게이트도 활성화
            other->isActive = true;
            
            gatesUsedCount++;
            break;
        }
    }

    // 아이템 5초(50틱)마다 자동 재생성
    if (growthItemTimer >= 50) { generateGItem(); growthItemTimer = 0; }
    if (poisonItemTimer >= 50) { generatePItem(); poisonItemTimer = 0; }
    if (timeItemTimer >= 50) { generateTItem(); timeItemTimer = 0; }

    if (gameMap.snakeHeadObject.coord == gameMap.growthItemObject.coord)
    {
        beep();
        growthItemCount++;
        generateGItem();
        growthItemTimer = 0;
        safeAddSnakeBody();
    }
    if (gameMap.snakeHeadObject.coord == gameMap.poisonItemObject.coord)
    {
        beep();
        poisonItemCount++;
        generatePItem();
        poisonItemTimer = 0;
        if (!safeRemoveSnakeBody()) {
            gameOverReason = "Length is less than 3.";
            return false;
        }
    }
    if (gameMap.snakeHeadObject.coord == gameMap.timeItemObject.coord)
    {
        beep();
        generateTItem();
        timeItemTimer = 0;
        speedMultiplier = 1.5;
        speedBoostTimer = 40;
    }

    // mission
    checkMissions();

    // allMissionsCompleted
    if (static_cast<int>(gameMap.snakeHeadObject.snakeBodySegments.size()) > maxSnakeLength)
        maxSnakeLength = static_cast<int>(gameMap.snakeHeadObject.snakeBodySegments.size());

    return isValid(previousDirection);
}

bool Game::isValid(int /*previousDirection*/)
{
    // 역방향 이동 시도 검사
    if (gameMap.snakeHeadObject.currentDirection == -2) {
        gameOverReason = "Tried moving in the opposite direction.";
        return false;
    }
    
    // 게이트가 활성화된 상태인지 확인 (두 게이트 모두 확인)
    bool isOnActiveGate = false;
    for (const auto& gate : gameMap.gameGates) {
        if (gate.coord == gameMap.snakeHeadObject.coord && gate.isActive) {
            isOnActiveGate = true;
            break;
        }
    }
    
    // 벽과의 충돌 검사 (활성화된 게이트 위에 있으면 벽 충돌 무시)
    if (!isOnActiveGate) {
        for (const auto& wall : gameMap.regularWalls) {
            if (wall.coord == gameMap.snakeHeadObject.coord) {
                gameOverReason = "Collided with the wall.";
                return false;
            }
        }
        
        for (const auto& wall : gameMap.immuneWalls) {
            if (wall.coord == gameMap.snakeHeadObject.coord) {
                gameOverReason = "Collided with the immune wall.";
                return false;
            }
        }
    }
    
    // 몸통과 벽 충돌 검사
    for (const auto& body : gameMap.snakeHeadObject.snakeBodySegments) {
        for (const auto& wall : gameMap.regularWalls) {
            if (wall.coord == body.coord) {
                gameOverReason = "Snake body overlapped with wall.";
                return false;
            }
        }
        for (const auto& wall : gameMap.immuneWalls) {
            if (wall.coord == body.coord) {
                gameOverReason = "Snake body overlapped with immune wall.";
                return false;
            }
        }
    }
    
    // 자기 몸통과의 충돌 검사
    for (const auto& body : gameMap.snakeHeadObject.snakeBodySegments) {
        if (body.coord == gameMap.snakeHeadObject.coord) {
            gameOverReason = "Collided with the body.";
            return false;
        }
    }
    
    // 최소 길이 검사
    if (gameMap.snakeHeadObject.snakeBodySegments.size() < 3) {
        gameOverReason = "Length is less than 3.";
        return false;
    }
    
    return true;
}

void Game::generateRandCoord(int &row, int &col, bool shouldIncludeWall)
    {
        while (1)
        {
        row = rand() % (gameMap.mapSize.height - 1) + 2;
        col = rand() % (gameMap.mapSize.width - 1) + 2;
            Coord tmp;
        tmp.row = row;
        tmp.col = col;
            bool same = false;
            if (!shouldIncludeWall)
            {
            for (auto it = gameMap.regularWalls.begin(); it != gameMap.regularWalls.end(); it++)
            {
                if (it->coord == tmp)
                    same = true;
            }
        }
        for (auto it = gameMap.snakeHeadObject.snakeBodySegments.begin(); it != gameMap.snakeHeadObject.snakeBodySegments.end(); it++)
            {
                if (it->coord == tmp)
                    same = true;
            }
        for (auto it = gameMap.gameGates.begin(); it != gameMap.gameGates.end(); it++)
        {
            if (it->coord == tmp)
                same = true;
        }
        if (gameMap.snakeHeadObject.coord == tmp)
            same = true;
        if (gameMap.growthItemObject.coord == tmp)
                same = true;
        if (gameMap.poisonItemObject.coord == tmp)
                same = true;
        // 아이템이 벽에 갇히지 않도록 상하좌우가 모두 벽이 아닌지 체크
        bool surrounded = false;
        int dr[4] = {-1, 1, 0, 0};
        int dc[4] = {0, 0, -1, 1};
        int wallCount = 0;
        for (int d = 0; d < 4; ++d) {
            Coord adj{row + dr[d], col + dc[d]};
            for (auto it = gameMap.regularWalls.begin(); it != gameMap.regularWalls.end(); it++) {
                if (it->coord == adj) wallCount++;
            }
        }
        if (wallCount == 4) surrounded = true;
        if (!same && !surrounded)
            break;
    }
}

void Game::generateGate()
{
    int wallIndex1, wallIndex2;
    
    // 게이트로 사용 가능한 벽인지 확인하는 함수
    auto isGateWallValid = [&](const Wall& wall) {
        // 1. 맵 경계에서 너무 가까운 곳은 제외 (모서리 근처)
        if (wall.coord.row <= 2 || wall.coord.row >= gameMap.mapSize.height - 1 ||
            wall.coord.col <= 2 || wall.coord.col >= gameMap.mapSize.width - 1) {
            return false;
        }
        
        // 2. 상하좌우 중 최소 2방향이 빈 공간이어야 함 (진출로 확보)
        int dr[4] = {-1, 1, 0, 0};
        int dc[4] = {0, 0, -1, 1};
        int openDirections = 0;
        
        for (int d = 0; d < 4; ++d) {
            Coord adj{wall.coord.row + dr[d], wall.coord.col + dc[d]};
            
            // 벽이 아니고 맵 범위 내인지 확인
            bool isWall = false;
            for (const auto& w : gameMap.regularWalls) {
                if (w.coord == adj) { 
                    isWall = true; 
                    break; 
                }
            }
            for (const auto& w : gameMap.immuneWalls) {
                if (w.coord == adj) { 
                    isWall = true; 
                    break; 
                }
            }
            
            // 빈 공간이고 맵 범위 내라면 진출 가능한 방향
            if (!isWall && adj.row > 1 && adj.row < gameMap.mapSize.height && 
                adj.col > 1 && adj.col < gameMap.mapSize.width) {
                openDirections++;
            }
        }
        
        // 최소 3방향 이상 진출 가능해야 함 (더 안전한 게이트 생성)
        return openDirections >= 3;
    };
    
    // 유효한 벽들만 필터링
    vector<int> validWallIndices;
    for (size_t i = 0; i < gameMap.regularWalls.size(); ++i) {
        if (isGateWallValid(gameMap.regularWalls[i])) {
            validWallIndices.push_back(static_cast<int>(i));
        }
    }
    
    // 유효한 벽이 2개 이상 있어야 게이트 생성 가능
    if (validWallIndices.size() < 2) {
        // 유효한 벽이 부족하면 기준을 낮춰서 다시 시도 (최소 2방향)
        validWallIndices.clear();
        for (size_t i = 0; i < gameMap.regularWalls.size(); ++i) {
            const Wall& wall = gameMap.regularWalls[i];
            if (wall.coord.row > 2 && wall.coord.row < gameMap.mapSize.height - 2 &&
                wall.coord.col > 2 && wall.coord.col < gameMap.mapSize.width - 2) {
                
                // 최소 1방향이라도 열려있으면 사용
                int dr[4] = {-1, 1, 0, 0};
                int dc[4] = {0, 0, -1, 1};
                bool hasOpenDirection = false;
                
                for (int d = 0; d < 4; ++d) {
                    Coord adj{wall.coord.row + dr[d], wall.coord.col + dc[d]};
                    bool isWall = false;
                    for (const auto& w : gameMap.regularWalls) {
                        if (w.coord == adj) { isWall = true; break; }
                    }
                    for (const auto& w : gameMap.immuneWalls) {
                        if (w.coord == adj) { isWall = true; break; }
                    }
                    if (!isWall) {
                        hasOpenDirection = true;
                        break;
                    }
                }
                
                if (hasOpenDirection) {
                    validWallIndices.push_back(static_cast<int>(i));
                }
            }
        }
    }
    
    if (validWallIndices.size() >= 2) {
        // 유효한 벽들 중에서 랜덤 선택
        wallIndex1 = validWallIndices[rand() % validWallIndices.size()];
        do {
            wallIndex2 = validWallIndices[rand() % validWallIndices.size()];
        } while (wallIndex1 == wallIndex2);
    } else {
        // 최후의 수단: 테두리 벽 중에서 모서리가 아닌 곳 선택
        vector<int> borderWalls;
        for (size_t i = 0; i < gameMap.regularWalls.size(); ++i) {
            const Wall& wall = gameMap.regularWalls[i];
            // 테두리 벽 중에서 모서리가 아닌 곳만 선택
            if ((wall.coord.row == 1 && wall.coord.col > 5 && wall.coord.col < gameMap.mapSize.width - 4) ||
                (wall.coord.row == gameMap.mapSize.height && wall.coord.col > 5 && wall.coord.col < gameMap.mapSize.width - 4) ||
                (wall.coord.col == 1 && wall.coord.row > 5 && wall.coord.row < gameMap.mapSize.height - 4) ||
                (wall.coord.col == gameMap.mapSize.width && wall.coord.row > 5 && wall.coord.row < gameMap.mapSize.height - 4)) {
                borderWalls.push_back(static_cast<int>(i));
            }
        }
        
        if (borderWalls.size() >= 2) {
            wallIndex1 = borderWalls[rand() % borderWalls.size()];
            do {
                wallIndex2 = borderWalls[rand() % borderWalls.size()];
            } while (wallIndex1 == wallIndex2);
        } else {
            // 최종 보장: 무작위로 두 개의 다른 벽 선택
            wallIndex1 = rand() % gameMap.regularWalls.size();
            do {
                wallIndex2 = rand() % gameMap.regularWalls.size();
            } while (wallIndex1 == wallIndex2);
        }
    }
    
    gameMap.gameGates[0] = Gate(gameMap.regularWalls[wallIndex1]);
    gameMap.gameGates[1] = Gate(gameMap.regularWalls[wallIndex2]);
}

void Game::generateItems()
    {
        generateGItem();
        generatePItem();
        generateTItem();
    }

void Game::generateTItem()
    {
    int row, col;
        generateRandCoord(row, col);
    gameMap.timeItemObject = TimeItem(row, col);
    }

void Game::generateGItem()
    {
    int row, col;
        generateRandCoord(row, col);
    gameMap.growthItemObject = GrowthItem(row, col);
    }

void Game::generatePItem()
    {
    int row, col;
        generateRandCoord(row, col);
    gameMap.poisonItemObject = PoisonItem(row, col);
}

MapType Game::getMapTypeForStage(int stage)
{
    switch(stage) {
        case 1: return MapType::BASIC;
        case 2: return MapType::MAZE;
        case 3: return MapType::ISLANDS;
        case 4: return MapType::CROSS;
        default: return MapType::BASIC;
    }
}

void Game::showEndingScreen()
{
    try {
        clear();
        refresh();
        int term_rows, term_cols;
        getmaxyx(stdscr, term_rows, term_cols);
        int art_height = 10;
        int art_width = 60;
        int start_row = (term_rows - art_height) / 2;
        int start_col = (term_cols - art_width) / 2;
        
        WindowWrapper ending(art_height+8, art_width+4, start_row-2, start_col-2);
        
        box(ending.get(), 0, 0);
        wattron(ending.get(), A_BOLD);
        // 깨지지 않는 NCURSES 문자로 SNAKE GAME 아트 (백슬래시 이스케이프)
        const char* snake_art[] = {
            "+==========================================================+",
            "|   _____  _   _    _    _  __ _____  _____   _____      |",
            "|  / ____|| \\ | |  / \\  | |/ /| ____||  __ \\ |  __ \\     |",
            "| | (___  |  \\| | / _ \\ | ' / |  _|  | |  | || |  | |    |",
            "|  \\___ \\ | . ` |/ ___ \\|  <  | |___ | |__| || |__| |    |",
            "|  ____/ ||_|\\_/_/   \\_\\_|\\_\\ |_____||_____/ |_____/     |",
            "|                                                        |",
            "|                * * *  CONGRATULATIONS!  * * *          |",
            "+==========================================================+"
        };
        for(int i=0; i<9; ++i) {
            mvwprintw(ending.get(), 1+i, 1, "%s", snake_art[i]);
            wrefresh(ending.get());
            usleep(90000);
        }
        wattroff(ending.get(), A_BOLD);
        wattron(ending.get(), A_BLINK);
        mvwprintw(ending.get(), 11, 8, "* * * YOU CLEARED ALL STAGES! * * *");
        wattroff(ending.get(), A_BLINK);
        wattron(ending.get(), A_BOLD);
        mvwprintw(ending.get(), 13, 8, "Press 'R' to restart or 'Q' to quit");
        wattroff(ending.get(), A_BOLD);
        wrefresh(ending.get());
        nodelay(stdscr, FALSE);
        while (true) {
            int ch = wgetch(ending.get());
            if (ch == 'q' || ch == 'Q') {
                cleanupNcurses();
                exit(0);
            }
            if (ch == 'r' || ch == 'R') {
                clear();
                refresh();
                break;
            }
        }
        nodelay(stdscr, TRUE);
    } catch (const std::exception& e) {
        cleanupNcurses();
        std::cerr << "Ending screen error: " << e.what() << std::endl;
        throw;
    }
}

Game::MissionTargets Game::getMissionTargets(int stage)
{
    switch(stage) {
        case 1: // 스테이지 1: 쉬운 입문 난이도
            return {5, 3, 1, 1}; // 길이 5, +아이템 3개, -아이템 1개, 게이트 1회
        case 2: // 스테이지 2: 중간 난이도 (미로)
            return {7, 5, 2, 2}; // 길이 7, +아이템 5개, -아이템 2개, 게이트 2회
        case 3: // 스테이지 3: 높은 난이도 (섬)
            return {9, 7, 3, 3}; // 길이 9, +아이템 7개, -아이템 3개, 게이트 3회
        case 4: // 스테이지 4: 최고 난이도 (십자형) - 플레이 가능한 수준
            return {12, 8, 4, 2}; // 길이 12, +아이템 8개, -아이템 4개, 게이트 2회
        default:
            return {5, 3, 1, 1}; // 기본값
    }
}

#endif