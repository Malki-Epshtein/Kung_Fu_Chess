#include "KingRules.h"

std::vector<Position> KingRules::moves(const Board& board, const Piece& piece) {
    std::vector<Position> result;
    int r = piece.getCell().row;
    int c = piece.getCell().col;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            Position to{ r + dr, c + dc };
            if (!board.isWithinBounds(to)) continue;
            if (!board.isCellEmpty(to) &&
                board.getPiece(to)->getColor() == piece.getColor()) continue;
            result.push_back(to);
        }
    }
    return result;
}
