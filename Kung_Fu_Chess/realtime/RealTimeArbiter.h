#pragma once
#include <vector>
#include <unordered_map>
#include "Motion.h"
#include "MotionPath.h"
#include "CaptureObserver.h"
#include "MoveObserver.h"
#include "CollisionResolver.h"
#include "ArrivalResolver.h"
#include "../model/Board.h"

// Coordinates real-time motion: owns the active Motion list, cooldown
// bookkeeping, and observer registration, and drives each tick forward.
// The actual "is this motion blocked/captured" and "what happens on
// arrival" algorithms live in CollisionResolver/ArrivalResolver - this
// class still owns all the state they operate on, it just delegates the
// two algorithms out to keep itself focused on tick-level coordination.
class RealTimeArbiter {
private:
    int                          game_clock_ms = 0;
    std::vector<Motion>          active_motions;
    std::unordered_map<int, int> cooldown_until_ms;
    std::vector<std::shared_ptr<Piece>> captured_pieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*>    moveObservers;
    Board&                       board;
    bool                         collisionEnabled;
    CollisionResolver             collisionResolver;
    ArrivalResolver                arrivalResolver;

public:
    RealTimeArbiter(Board& board, bool collisionEnabled = false)
        : board(board), collisionEnabled(collisionEnabled),
          collisionResolver(board, active_motions, captured_pieces, captureObservers, cooldown_until_ms),
          arrivalResolver(board, active_motions, cooldown_until_ms, captured_pieces, captureObservers, moveObservers, collisionEnabled) {}

    bool addMotion(Position from, Position to, int piece_id);
    void addJump(Position pos, int piece_id);
    bool tick(int ms);
    int  getClock() const { return game_clock_ms; }
    const std::vector<Motion>& getActiveMotions() const { return active_motions; }

    bool isPieceBusy(int piece_id) const;
    bool isPieceCoolingDown(int piece_id) const;
    const std::vector<std::shared_ptr<Piece>>& getCapturedPieces() const { return captured_pieces; }

    void addCaptureObserver(CaptureObserver* observer) { captureObservers.push_back(observer); }
    void addMoveObserver(MoveObserver* observer) { moveObservers.push_back(observer); }

    // Derives when the piece's current state began, purely from timing data
    // already tracked for other reasons (active motions, cooldown expiry) -
    // no new bookkeeping. Falls back to the current clock (harmless for
    // looping animations) when nothing is tracked, e.g. Idle.
    int getStateStartMs(int piece_id, Chess::State state) const;
};
