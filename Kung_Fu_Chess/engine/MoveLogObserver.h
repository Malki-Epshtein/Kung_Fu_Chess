#pragma once
#include "../realtime/MoveObserver.h"
#include <vector>
#include <string>
#include <chrono>

struct MoveEntry {
    std::string timestamp; // wall-clock elapsed since game start, "MM:SS.mmm"
    std::string notation;  // simplified algebraic notation, e.g. "Nc6", "exd5"
};

// Implements only MoveObserver (Interface Segregation) - the move log only
// cares about completed moves, never about captures.
class MoveLogObserver : public MoveObserver {
private:
    std::vector<MoveEntry> whiteMoves;
    std::vector<MoveEntry> blackMoves;
    int boardHeight;
    std::chrono::steady_clock::time_point startTime;

    std::string describe(const Piece& mover, Position from, Position to, bool wasCapture) const;
    std::string formatElapsed() const;

public:
    explicit MoveLogObserver(int boardHeight)
        : boardHeight(boardHeight), startTime(std::chrono::steady_clock::now()) {}

    void onMoveCompleted(const Piece& mover, Position from, Position to, bool wasCapture) override;
    const std::vector<MoveEntry>& getMoves(Chess::Color color) const;
};
