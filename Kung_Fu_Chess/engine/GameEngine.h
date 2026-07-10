#pragma once
#include "MoveResult.h"
#include "../model/GameState.h"
#include "../realtime/RealTimeArbiter.h"
#include "../model/Position.h"

class GameEngine {
private:
    GameState&      state;
    RealTimeArbiter arbiter;

    bool hasMotionOnPath(Position from, Position to) const;

public:
    GameEngine(GameState& state) : state(state), arbiter(*state.board) {}

    MoveResult requestMove(Position from, Position to);
    void       wait(int ms);
    bool       isGameOver() const { return state.game_over; }
};
