#pragma once
#include "../realtime/MoveObserver.h"
#include <vector>
#include <string>

// Implements only MoveObserver (Interface Segregation) - the move log only
// cares about completed moves, never about captures.
class MoveLogObserver : public MoveObserver {
private:
    std::vector<std::string> whiteMoves;
    std::vector<std::string> blackMoves;

    static std::string describe(const Piece& mover, Position from, Position to);

public:
    void onMoveCompleted(const Piece& mover, Position from, Position to) override;
    const std::vector<std::string>& getMoves(Chess::Color color) const;
};
