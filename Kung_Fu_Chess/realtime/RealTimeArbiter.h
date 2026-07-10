#pragma once
#include <vector>
#include <algorithm>
#include "Motion.h"
#include "../model/Board.h"

class RealTimeArbiter {
private:
    int                 game_clock_ms = 0;
    std::vector<Motion> active_motions;
    Board&              board;

    static int calcTravelTime(Position from, Position to);

public:
    RealTimeArbiter(Board& board) : board(board) {}

    void addMotion(Position from, Position to, int piece_id);
    bool tick(int ms);
    int  getClock() const { return game_clock_ms; }
};
