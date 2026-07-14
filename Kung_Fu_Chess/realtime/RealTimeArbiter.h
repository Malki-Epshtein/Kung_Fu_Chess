#pragma once
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "Motion.h"
#include "MotionPath.h"
#include "../model/Board.h"

class RealTimeArbiter {
private:
    static const int SHORT_REST_MS   = 500;  // rest after a jump
    static const int LONG_REST_MS    = 1000; // rest after any other action
    static const int JUMP_DURATION_MS = 1000; // time a jump spends airborne

    int                        game_clock_ms = 0;
    std::vector<Motion>        active_motions;
    std::unordered_map<int,int> cooldown_until_ms;
    std::vector<std::shared_ptr<Piece>> captured_pieces;
    Board&                     board;
    bool                       collisionEnabled;

    // returns false if the motion is fully blocked at its very first step
    bool applyNearMiss(Motion& motion) const;
    void resolveCollisions(int previous_clock, bool& king_captured);
    void resolveArrival(const Motion& m, bool& king_captured);

public:
    RealTimeArbiter(Board& board, bool collisionEnabled = false)
        : board(board), collisionEnabled(collisionEnabled) {}

    bool addMotion(Position from, Position to, int piece_id);
    void addJump(Position pos, int piece_id);
    bool tick(int ms);
    int  getClock() const { return game_clock_ms; }
    const std::vector<Motion>& getActiveMotions() const { return active_motions; }

    bool isPieceBusy(int piece_id) const;
    bool isPieceCoolingDown(int piece_id) const;
    const std::vector<std::shared_ptr<Piece>>& getCapturedPieces() const { return captured_pieces; }

    // Derives when the piece's current state began, purely from timing data
    // already tracked for other reasons (active motions, cooldown expiry) -
    // no new bookkeeping. Falls back to the current clock (harmless for
    // looping animations) when nothing is tracked, e.g. Idle.
    int getStateStartMs(int piece_id, Chess::State state) const;
};
