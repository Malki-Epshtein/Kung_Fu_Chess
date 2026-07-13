#pragma once
#include "MoveResult.h"
#include "GameSnapshot.h"
#include "../model/GameState.h"
#include "../realtime/RealTimeArbiter.h"
#include "../model/Position.h"
#include <memory>
#include <unordered_map>

struct Premove {
    Position from;
    Position to;
};

class GameEngine {
private:
    GameState        state;
    RealTimeArbiter  arbiter;
    bool             simultaneousMode;
    std::unordered_map<int, Premove> premoves;

    bool hasMotionOnPath(Position from, Position to) const;
    void firePremoves();

public:
    GameEngine(std::shared_ptr<Board> board, bool simultaneousMode = false)
        : state(board), arbiter(*state.board, simultaneousMode), simultaneousMode(simultaneousMode) {}

    MoveResult   requestMove(Position from, Position to);
    MoveResult   requestJump(Position pos);
    void         wait(int ms);
    bool         isGameOver() const { return state.game_over; }
    GameSnapshot snapshot() const;
};
