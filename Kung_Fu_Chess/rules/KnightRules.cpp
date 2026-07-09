#include "KnightRules.h"

std::vector<Position> KnightRules::moves(const Board& board, const Piece& piece) {
    std::vector<Position> result;
    int r = piece.getCell().row;
    int c = piece.getCell().col;
    const int jumps[8][2] = {
        {-2,-1},{-2,1},{-1,-2},{-1,2},
        { 1,-2},{ 1,2},{ 2,-1},{ 2,1}
    };
    for (auto& j : jumps) {
        Position to{ r + j[0], c + j[1] };
        if (!board.isWithinBounds(to)) continue;
        if (!board.isCellEmpty(to) &&
            board.getPiece(to)->getColor() == piece.getColor()) continue;
        result.push_back(to);
    }
    return result;
}
