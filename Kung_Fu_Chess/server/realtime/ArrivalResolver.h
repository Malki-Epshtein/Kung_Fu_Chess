#pragma once
#include <vector>
#include <unordered_map>
#include <memory>
#include "Motion.h"
#include "CaptureObserver.h"
#include "MoveObserver.h"
#include "../../shared/model/Board.h"

// Owns all "what happens when a motion completes" logic - captures at the
// destination, blocked-by-friendly landings, jump-related captures, and
// pawn promotion. Constructed once by RealTimeArbiter with references to
// the state it needs; RealTimeArbiter still owns all of that state - this
// class never becomes a second source of truth for it.
class ArrivalResolver {
private:
    Board& board;
    const std::vector<Motion>& activeMotions;
    std::unordered_map<int, int>& cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>>& capturedPieces;
    std::vector<CaptureObserver*>& captureObservers;
    std::vector<MoveObserver*>& moveObservers;
    bool collisionEnabled;

public:
    ArrivalResolver(Board& board, const std::vector<Motion>& activeMotions,
        std::unordered_map<int, int>& cooldownUntilMs,
        std::vector<std::shared_ptr<Piece>>& capturedPieces,
        std::vector<CaptureObserver*>& captureObservers,
        std::vector<MoveObserver*>& moveObservers,
        bool collisionEnabled)
        : board(board), activeMotions(activeMotions), cooldownUntilMs(cooldownUntilMs),
          capturedPieces(capturedPieces), captureObservers(captureObservers),
          moveObservers(moveObservers), collisionEnabled(collisionEnabled) {}

    void resolve(const Motion& m, int gameClockMs, bool& kingCaptured);
};
