#include "RookRules.h"
#include "SlidingPieceRules.h"

std::vector<Position> RookRules::moves(const Board& board, const Piece& piece, bool ignoreBlockers) {
    static const std::vector<std::pair<int, int>> directions = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}
    };
    return SlidingPieceRules::slide(board, piece, directions, ignoreBlockers);
}
