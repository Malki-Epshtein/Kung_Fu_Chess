#pragma once
#include <vector>
#include <unordered_map>
#include <memory>
#include "Motion.h"
#include "CaptureObserver.h"
#include "../model/Board.h"

// Owns all "does this motion get blocked or captured mid-path" logic -
// friendly near-miss truncation, enemy path-crossing collisions, and
// stationary blockers still sitting where a slide is headed. Constructed
// once by RealTimeArbiter with references to the state it needs to read
// and mutate; RealTimeArbiter still owns all of that state - this class
// never becomes a second source of truth for it.
class CollisionResolver {
private:
    Board& board;
    std::vector<Motion>& activeMotions;
    std::vector<std::shared_ptr<Piece>>& capturedPieces;
    std::vector<CaptureObserver*>& captureObservers;
    std::unordered_map<int, int>& cooldownUntilMs;

    bool isPieceSliding(int pieceId) const;

public:
    CollisionResolver(Board& board, std::vector<Motion>& activeMotions,
        std::vector<std::shared_ptr<Piece>>& capturedPieces,
        std::vector<CaptureObserver*>& captureObservers,
        std::unordered_map<int, int>& cooldownUntilMs)
        : board(board), activeMotions(activeMotions), capturedPieces(capturedPieces),
          captureObservers(captureObservers), cooldownUntilMs(cooldownUntilMs) {}

    // returns false if the motion is fully blocked at its very first step
    bool applyNearMiss(Motion& motion) const;
    void resolveCollisions(int previousClock, int gameClockMs, bool& kingCaptured);
    void resolveMovingFriendlyBlocks(int previousClock, int gameClockMs);
    void resolveStationaryBlocks(int previousClock, int gameClockMs);
};
