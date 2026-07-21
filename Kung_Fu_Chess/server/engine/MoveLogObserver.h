#pragma once
#include "../realtime/MoveObserver.h"
#include "../../shared/protocol/MoveEntry.h"
#include <functional>
#include <string>
#include <vector>

// Implements only MoveObserver (Interface Segregation) - the move log only
// cares about completed moves, never about captures.
class MoveLogObserver : public MoveObserver {
private:
    std::vector<MoveEntry> whiteMoves;
    std::vector<MoveEntry> blackMoves;
    int boardHeight;

    std::string describe(const Piece& mover, Position from, Position to, bool wasCapture) const;
    static std::string formatElapsed(int gameClockMs);

public:
    explicit MoveLogObserver(int boardHeight) : boardHeight(boardHeight) {}

    // Fired right after a new entry is appended (never for captures alone,
    // never before the entry actually exists in whiteMoves/blackMoves) -
    // lets a caller (GameSession) bridge into the network layer without
    // this class ever knowing EventBus/JSON exist. Empty by default (no-op).
    std::function<void(Chess::Color, const MoveEntry&)> onNewEntry;

    void onMoveCompleted(const Piece& mover, Position from, Position to, bool wasCapture, int gameClockMs) override;
    const std::vector<MoveEntry>& getMoves(Chess::Color color) const;
};
