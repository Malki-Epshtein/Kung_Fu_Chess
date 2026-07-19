#pragma once
#include "MoveResult.h"
#include "../../shared/engine/GameSnapshot.h"
#include "../../shared/model/GameState.h"
#include "../realtime/RealTimeArbiter.h"
#include "../../shared/model/Position.h"
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
    std::vector<Position> legalDestinationsFrom(Position pos) const;

    // Thin passthrough so callers never need to reach into RealTimeArbiter
    // directly - keeps the existing dependency shape (GameEngine depends on
    // RealTimeArbiter; nothing new depends on it).
    void addCaptureObserver(CaptureObserver* observer) { arbiter.addCaptureObserver(observer); }
    void addMoveObserver(MoveObserver* observer) { arbiter.addMoveObserver(observer); }
};
