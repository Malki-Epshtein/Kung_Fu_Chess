#include "BishopRules.h"
#include "SlidingPieceRules.h"

std::vector<Position> BishopRules::moves(const Board& board, const Piece& piece, bool ignoreBlockers) {
    static const std::vector<std::pair<int, int>> directions = {
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    return SlidingPieceRules::slide(board, piece, directions, ignoreBlockers);
}
