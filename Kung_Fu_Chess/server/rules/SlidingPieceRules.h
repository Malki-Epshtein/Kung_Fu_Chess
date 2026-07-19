#pragma once
#include <vector>
#include <utility>
#include "../../shared/model/Board.h"
#include "../../shared/model/Piece.h"

// Template Method: the sliding algorithm itself - walk a direction until
// blocked, optionally capture the blocker, optionally ignore blockers
// entirely (section 7) - is defined once here. Rook/Bishop/Queen used to
// each carry an identical private copy of this; they now only supply their
// own set of directions, the one part that actually varies between them.
class SlidingPieceRules {
public:
    static std::vector<Position> slide(const Board& board, const Piece& piece,
        const std::vector<std::pair<int, int>>& directions, bool ignoreBlockers);
};
