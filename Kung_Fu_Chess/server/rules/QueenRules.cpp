#include "QueenRules.h"
#include "SlidingPieceRules.h"

std::vector<Position> QueenRules::moves(const Board& board, const Piece& piece, bool ignoreBlockers) {
    static const std::vector<std::pair<int, int>> directions = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},   // כיווני צריח
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}  // כיווני רץ
    };
    return SlidingPieceRules::slide(board, piece, directions, ignoreBlockers);
}
