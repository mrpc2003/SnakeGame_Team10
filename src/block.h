#ifndef BLOCK_H
#define BLOCK_H
#include <iostream>
#include <vector>
#include <stdexcept>

using namespace std;

struct Coord
{
    int row, col;

    bool operator==(const Coord &other) const
    {
        return row == other.row && col == other.col;
    }

    bool operator!=(const Coord &other) const
    {
        return !(*this == other);
    }

    bool operator<(const Coord &other) const {
        return (row < other.row) || (row == other.row && col < other.col);
    }
    
    // 좌표 유효성 검사
    bool isValid(int maxRow = 100, int maxCol = 100) const {
        return row >= 0 && col >= 0 && row < maxRow && col < maxCol;
    }
    
    // 안전한 좌표 설정
    void setSafe(int newRow, int newCol, int maxRow = 100, int maxCol = 100) {
        if (newRow < 0 || newRow >= maxRow || newCol < 0 || newCol >= maxCol) {
            throw std::out_of_range("Coordinate out of bounds");
        }
        row = newRow;
        col = newCol;
    }
};

class Block
{
public:
    Coord coord;
    int objectType = 0;
    
    virtual ~Block() = default;
    virtual int getObjectType() const { return objectType; }
    
    Block() = default;
    Block(int row, int col) : coord{row, col} {
        validateCoordinates(row, col);
    }
    Block(const Block &b) = default;
    Block& operator=(const Block &b) = default;
    
protected:
    // 좌표 유효성 검사 (기본적인 범위 체크)
    void validateCoordinates(int row, int col) const {
        if (row < 0 || col < 0) {
            throw std::invalid_argument("Coordinates cannot be negative");
        }
        // 너무 큰 값도 체크 (메모리 오버플로우 방지)
        if (row > 10000 || col > 10000) {
            throw std::invalid_argument("Coordinates too large");
        }
    }
};

class Wall : public Block
{
public:
    int wallPositionType;
    
    Wall() : Block() { objectType = 1; }
    Wall(int row, int col, int wallPositionType = -1) 
        : Block(row, col), wallPositionType(wallPositionType) 
    {
        objectType = 1;
    }
    
    int getObjectType() const override { return objectType; }
};

class ImmunedWall : public Block
{
public:
    ImmunedWall() : Block() { objectType = 2; }
    ImmunedWall(int row, int col) : Block(row, col) { objectType = 2; }
    ImmunedWall(const ImmunedWall &iwall) = default;
    ImmunedWall& operator=(const ImmunedWall &iwall) = default;
    
    int getObjectType() const override { return objectType; }
};

class GrowthItem : public Block
{
public:
    GrowthItem() : Block() { objectType = 5; }
    GrowthItem(int row, int col) : Block(row, col) { objectType = 5; }
    GrowthItem(const GrowthItem &gitem) = default;
    GrowthItem& operator=(const GrowthItem &gitem) = default;
    
    int getObjectType() const override { return objectType; }
};

class PoisonItem : public Block
{
public:
    PoisonItem() : Block() { objectType = -5; }
    PoisonItem(int row, int col) : Block(row, col) { objectType = -5; }
    PoisonItem(const PoisonItem &pitem) = default;
    PoisonItem& operator=(const PoisonItem &pitem) = default;
    
    int getObjectType() const override { return objectType; }
};

class TimeItem : public Block
{
public:
    TimeItem() : Block() { objectType = 6; }
    TimeItem(int row, int col) : Block(row, col) { objectType = 6; }
    TimeItem(const TimeItem &titem) = default;
    TimeItem& operator=(const TimeItem &titem) = default;
    
    int getObjectType() const override { return objectType; }
};

class Gate : public Block
{
public:
    int exitDirection;
    bool isActive = false;
    
    Gate() : Block() { 
        objectType = 2; 
        exitDirection = 6; // 기본값: 자유 방향
    }
    
    Gate(const Wall &wall) : Block(wall.coord.row, wall.coord.col)
    {
        objectType = 2;
        // wallPositionType: 1=상단, 2=좌측, 3=우측, 4=하단, -1=내부벽
        if (wall.wallPositionType == -1) {
            exitDirection = 6; // 내부 벽은 자유 방향
        } else {
            exitDirection = 5 - wall.wallPositionType;
        }
    }
    
    Gate(int row, int col) : Block(row, col) { 
        objectType = 2; 
        exitDirection = 6; // 기본값: 자유 방향
    }
    
    int getObjectType() const override { return objectType; }
};

class SnakeHead;

class SnakeBody : public Block
{
public:
    SnakeBody() : Block() { objectType = 4; }
    SnakeBody(int row, int col) : Block(row, col) { objectType = 4; }
    SnakeBody(const SnakeHead &head);
    
    int getObjectType() const override { return objectType; }
};

class SnakeHead : public Block
{
public:
    vector<SnakeBody> snakeBodySegments;
    int currentDirection = -1;
    
    friend class SnakeBody;
    
    SnakeHead() : Block() { objectType = 3; }
    SnakeHead(int row, int col) : Block(row, col) { objectType = 3; }
    
    int getObjectType() const override { return objectType; }
    
    void move()
    {
        // 이동 전 방향 유효성 검사
        if (currentDirection < 1 || currentDirection > 4) {
            return; // 유효하지 않은 방향이면 이동하지 않음
        }
        
        Coord newCoord = coord;
        switch (currentDirection)
        {
            case 1: newCoord.row--; break; // Up
            case 2: newCoord.col--; break; // Left
            case 3: newCoord.col++; break; // Right
            case 4: newCoord.row++; break; // Down
        }
        
        // 새 좌표가 유효한 범위 내인지 확인 (기본적인 체크)
        if (newCoord.isValid(1000, 1000)) {
            coord = newCoord;
        }
        // 유효하지 않은 좌표로 이동하려 하면 그냥 무시 (게임 로직에서 처리)
    }
    
    // 안전한 몸통 추가
    void addBodySegment(int row, int col) {
        try {
            snakeBodySegments.emplace_back(row, col);
        } catch (const std::exception& e) {
            // 유효하지 않은 좌표면 머리 위치에 추가
            snakeBodySegments.emplace_back(coord.row, coord.col);
        }
    }
    
    // 안전한 몸통 제거
    bool removeBodySegment() {
        if (snakeBodySegments.size() <= 3) {
            return false; // 최소 길이 유지
        }
        snakeBodySegments.pop_back();
        return true;
    }
};

inline SnakeBody::SnakeBody(const SnakeHead &head)
    : Block(head.coord.row, head.coord.col)
{
    objectType = 4;
}

#endif