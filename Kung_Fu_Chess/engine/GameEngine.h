#pragma once
#include "MoveResult.h"
#include "GameSnapshot.h"
#include "../model/Board.h"
#include "../realtime/RealTimeArbiter.h"
#include "../model/Position.h"
#include <memory>

class GameEngine {
private:
    std::shared_ptr<Board> board;
    bool                   game_over = false;
    RealTimeArbiter        arbiter;

    bool hasMotionOnPath(Position from, Position to) const;

public:
    GameEngine(std::shared_ptr<Board> board) : board(board), arbiter(*board) {}

    MoveResult   requestMove(Position from, Position to);
    MoveResult   requestJump(Position pos);
    void         wait(int ms);
    bool         isGameOver() const { return game_over; }
    GameSnapshot snapshot() const;
};
