#include "QueenRules.h"

static void slide(const Board& board, const Piece& piece,
    int dr, int dc, std::vector<Position>& result)
{
    int r = piece.getCell().row + dr;
    int c = piece.getCell().col + dc;
    while (board.isWithinBounds({ r, c })) {
        if (board.isCellEmpty({ r, c })) {
            result.push_back({ r, c });
        } else {
            if (board.getPiece({ r, c })->getColor() != piece.getColor())
                result.push_back({ r, c });
            break;
        }
        r += dr;
        c += dc;
    }
}

std::vector<Position> QueenRules::moves(const Board& board, const Piece& piece) {
    std::vector<Position> result;
    // כיווני צריח
    slide(board, piece,  1,  0, result);
    slide(board, piece, -1,  0, result);
    slide(board, piece,  0,  1, result);
    slide(board, piece,  0, -1, result);
    // כיווני רץ
    slide(board, piece,  1,  1, result);
    slide(board, piece,  1, -1, result);
    slide(board, piece, -1,  1, result);
    slide(board, piece, -1, -1, result);
    return result;
}
