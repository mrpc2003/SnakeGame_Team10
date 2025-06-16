#ifndef MAP_H
#define MAP_H

#include <iostream>
#include <vector>
#include <algorithm>
#include "block.h" // Assuming block.h is already modified

using namespace std;

enum class MapType {
    BASIC,      // 기본 맵
    MAZE,       // 미로형 맵
    ISLANDS,    // 섬형 맵
    CROSS       // 십자형 맵
};

struct MapDimensions
{
    int height, width;
    
    MapDimensions(int h = 21, int w = 21) : height(h), width(w) {}
};

class Map
{
public:
    MapDimensions mapSize;
    SnakeHead snakeHeadObject;
    vector<ImmunedWall> immuneWalls;
    vector<Wall> regularWalls;
    vector<Gate> gameGates;
    GrowthItem growthItemObject;
    PoisonItem poisonItemObject;
    TimeItem timeItemObject;
    MapType currentMapType;

    Map(int mapHeight = 21, int mapWidth = 21, int initialWallCount = 0, MapType type = MapType::BASIC, int stage = 1);
    Map(const Map &m) = default;
    Map& operator=(const Map &m) = default;
    ~Map() = default;

    void print_map() const;
    bool isPositionValid(const Coord& pos) const;
    bool isPositionOccupied(const Coord& pos) const;

private:
    void initializeWalls();
    void generateRandomWalls(int count);
    void generateMazeMap();
    void generateIslandsMap();
    void generateCrossMap(int rotation);
    void generateMapByType(MapType type);
    bool isNearSnake(const Coord& pos, const SnakeHead& snakeHead);
};

// void : 0, wall : 1, immune wall : -1, gate: 2, snake head: 3, snake body: 4

Map::Map(int mapHeight, int mapWidth, int /*initialWallCount*/, MapType type, int stage)
    : mapSize(mapHeight, mapWidth)
    , gameGates(2)
    , currentMapType(type)
{
    initializeWalls();
    snakeHeadObject = SnakeHead(mapHeight / 2, mapWidth / 2);
    for(int i = 1; i <= 3; ++i) {
        snakeHeadObject.snakeBodySegments.emplace_back(mapHeight / 2 + i, mapWidth / 2);
    }
    if (type == MapType::BASIC) {
        // 내부 벽 없음 (테두리만)
    } else if (type == MapType::MAZE) {
        generateMazeMap();
    } else if (type == MapType::ISLANDS) {
        generateIslandsMap();
    } else if (type == MapType::CROSS) {
        int rotation = (stage - 1) % 4;
        generateCrossMap(rotation);
    }
    std::vector<Coord> snakeCoords;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            snakeCoords.push_back({snakeHeadObject.coord.row + dr, snakeHeadObject.coord.col + dc});
        }
    }
    for (const auto& body : snakeHeadObject.snakeBodySegments) {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                snakeCoords.push_back({body.coord.row + dr, body.coord.col + dc});
            }
        }
    }
    regularWalls.erase(std::remove_if(regularWalls.begin(), regularWalls.end(),
        [&](const Wall& w) {
            for(const auto& sc : snakeCoords) if (w.coord == sc) return true;
            return false;
        }), regularWalls.end());
}

void Map::initializeWalls()
{
    // Create border walls
    for (int i = 1; i <= mapSize.height; ++i) {
        for (int j = 1; j <= mapSize.width; ++j) {
            if (i == 1 || i == mapSize.height) {
                if (j == 1 || j == mapSize.width) {
                    immuneWalls.emplace_back(i, j);
                } else {
                    regularWalls.emplace_back(i, j, (i == 1 ? 1 : 4));
                }
            } else if (j == 1 || j == mapSize.width) {
                regularWalls.emplace_back(i, j, (j == 1 ? 2 : 3));
            }
        }
    }
}

void Map::generateMapByType(MapType type)
{
    switch(type) {
        case MapType::BASIC:
            break;
        case MapType::MAZE:
            generateMazeMap();
            break;
        case MapType::ISLANDS:
            generateIslandsMap();
            break;
        case MapType::CROSS:
            generateCrossMap(0);
            break;
    }
}

void Map::generateMazeMap()
{
    // ㄱ, ㄴ, └, ┐ 패턴의 벽을 가장자리에서 내부로 일부만 배치
    int h = mapSize.height;
    int w = mapSize.width;

    // 좌상단 ㄱ자
    for (int j = 2; j <= 6; ++j)
        if (isPositionValid({2, j}) && !isNearSnake({2, j}, snakeHeadObject))
            regularWalls.emplace_back(2, j);
    for (int i = 2; i <= 5; ++i)
        if (isPositionValid({i, 6}) && !isNearSnake({i, 6}, snakeHeadObject))
            regularWalls.emplace_back(i, 6);

    // 우상단 ㄴ자
    for (int j = w-1; j >= w-5; --j)
        if (isPositionValid({2, j}) && !isNearSnake({2, j}, snakeHeadObject))
            regularWalls.emplace_back(2, j);
    for (int i = 2; i <= 5; ++i)
        if (isPositionValid({i, w-5}) && !isNearSnake({i, w-5}, snakeHeadObject))
            regularWalls.emplace_back(i, w-5);

    // 좌하단 └자
    for (int i = h-1; i >= h-4; --i)
        if (isPositionValid({i, 2}) && !isNearSnake({i, 2}, snakeHeadObject))
            regularWalls.emplace_back(i, 2);
    for (int j = 2; j <= 5; ++j)
        if (isPositionValid({h-4, j}) && !isNearSnake({h-4, j}, snakeHeadObject))
            regularWalls.emplace_back(h-4, j);

    // 우하단 ┐자
    for (int i = h-1; i >= h-4; --i)
        if (isPositionValid({i, w-1}) && !isNearSnake({i, w-1}, snakeHeadObject))
            regularWalls.emplace_back(i, w-1);
    for (int j = w-1; j >= w-4; --j)
        if (isPositionValid({h-4, j}) && !isNearSnake({h-4, j}, snakeHeadObject))
            regularWalls.emplace_back(h-4, j);

    // 중앙에 짧은 벽 추가(내부 공간 충분히 확보)
    for (int j = w/2-2; j <= w/2+2; ++j)
        if (isPositionValid({h/2, j}) && !isNearSnake({h/2, j}, snakeHeadObject))
            regularWalls.emplace_back(h/2, j);
}

void Map::generateIslandsMap()
{
    // 중앙에 섬의 테두리만 벽으로 생성
    int centerRow = mapSize.height / 2;
    int centerCol = mapSize.width / 2;
    int islandHalf = min(mapSize.height, mapSize.width) / 6; // 섬 크기 조절
    int top = centerRow - islandHalf;
    int bottom = centerRow + islandHalf;
    int left = centerCol - islandHalf;
    int right = centerCol + islandHalf;
    for (int i = top; i <= bottom; ++i) {
        for (int j = left; j <= right; ++j) {
            if (i == top || i == bottom || j == left || j == right) {
                Coord pos{i, j};
                if (isPositionValid(pos) && !isNearSnake(pos, snakeHeadObject)) {
                    regularWalls.emplace_back(i, j);
                }
            }
        }
    }
}

void Map::generateCrossMap(int rotation)
{
    int centerRow = mapSize.height / 2;
    int centerCol = mapSize.width / 2;
    int crossSize = min(mapSize.height, mapSize.width) / 3;
    if (rotation % 2 == 0) {
        // + 모양 (수직/수평)
        for(int i = -crossSize; i <= crossSize; i++) {
            Coord pos1{centerRow+i, centerCol};
            Coord pos2{centerRow, centerCol+i};
            if(isPositionValid(pos1) && !isNearSnake(pos1, snakeHeadObject)) regularWalls.emplace_back(pos1.row, pos1.col);
            if(isPositionValid(pos2) && !isNearSnake(pos2, snakeHeadObject)) regularWalls.emplace_back(pos2.row, pos2.col);
        }
    } else {
        // × 모양 (대각선)
        for(int i = -crossSize; i <= crossSize; i++) {
            Coord pos1{centerRow+i, centerCol+i};
            Coord pos2{centerRow+i, centerCol-i};
            if(isPositionValid(pos1) && !isNearSnake(pos1, snakeHeadObject)) regularWalls.emplace_back(pos1.row, pos1.col);
            if(isPositionValid(pos2) && !isNearSnake(pos2, snakeHeadObject)) regularWalls.emplace_back(pos2.row, pos2.col);
        }
    }
}

bool Map::isNearSnake(const Coord& pos, const SnakeHead& snakeHead) {
    // 머리와 몸통 + 8방향 1칸 이내
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            Coord check = {snakeHead.coord.row + dr, snakeHead.coord.col + dc};
            if (pos == check) return true;
        }
    }
    for (const auto& body : snakeHead.snakeBodySegments) {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                Coord check = {body.coord.row + dr, body.coord.col + dc};
                if (pos == check) return true;
            }
        }
    }
    return false;
}

void Map::generateRandomWalls(int count)
{
    while (count--) {
        int row = rand() % (mapSize.height - 2) + 2;
        int col = rand() % (mapSize.width - 2) + 2;
        int length = rand() % 6 + 4;
        int direction = rand() % 4 + 1;

        for (int i = 0; i < length; ++i) {
            Coord pos{row, col};
            if (isPositionValid(pos) && !isPositionOccupied(pos) && !isNearSnake(pos, snakeHeadObject)) {
                regularWalls.emplace_back(row, col);
            }

            switch (direction) {
                case 1: row--; break; // Up
                case 2: col--; break; // Left
                case 3: col++; break; // Right
                case 4: row++; break; // Down
            }
        }
    }
}

bool Map::isPositionValid(const Coord& pos) const
{
    return pos.row >= 1 && pos.row < mapSize.height && 
           pos.col >= 1 && pos.col < mapSize.width;
}

bool Map::isPositionOccupied(const Coord& pos) const
{
    if (snakeHeadObject.coord == pos) return true;
    
    for (const auto& body : snakeHeadObject.snakeBodySegments) {
        if (body.coord == pos) return true;
    }
    
    for (const auto& wall : regularWalls) {
        if (wall.coord == pos) return true;
    }
    
    return false;
}

void Map::print_map() const
{
    // 맵의 현재 상태를 출력
    for (int i = 0; i < mapSize.height + 2; i++) {
        for (int j = 0; j < mapSize.width + 2; j++) {
            Coord pos{i, j};
            bool printed = false;

            // 테두리 출력
            if (i == 0 || i == mapSize.height + 1 || j == 0 || j == mapSize.width + 1) {
                cout << "#";
                printed = true;
                continue;
            }

            // 스네이크 헤드 출력
            if (snakeHeadObject.coord == pos) {
                cout << "H";
                printed = true;
                continue;
            }

            // 스네이크 바디 출력
            for (const auto& body : snakeHeadObject.snakeBodySegments) {
                if (body.coord == pos) {
                    cout << "B";
                    printed = true;
                    break;
                }
            }
            if (printed) continue;

            // 게이트 출력
            for (const auto& gate : gameGates) {
                if (gate.coord == pos && gate.isActive) {
                    cout << "G";
                    printed = true;
                    break;
                }
            }
            if (printed) continue;

            // 아이템 출력
            if (growthItemObject.coord == pos) {
                cout << "+";
                printed = true;
            } else if (poisonItemObject.coord == pos) {
                cout << "-";
                printed = true;
            } else if (timeItemObject.coord == pos) {
                cout << "T";
                printed = true;
            }
            if (printed) continue;

            // 벽 출력
            for (const auto& wall : regularWalls) {
                if (wall.coord == pos) {
                    cout << "W";
                    printed = true;
                    break;
                }
            }
            if (printed) continue;

            // 무적 벽 출력
            for (const auto& wall : immuneWalls) {
                if (wall.coord == pos) {
                    cout << "I";
                    printed = true;
                    break;
                }
            }
            if (printed) continue;

            // 빈 공간 출력
            cout << " ";
        }
        cout << endl;
    }
}

#endif