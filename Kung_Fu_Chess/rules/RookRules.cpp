#include "RookRules.h"

static void slide(const Board& board, const Piece& piece,
    int dr, int dc, std::vector<Position>& result, bool ignoreBlockers)
{
    int r = piece.getCell().row + dr;
    int c = piece.getCell().col + dc;
    while (board.isWithinBounds({ r, c })) {
        if (board.isCellEmpty({ r, c })) {
            result.push_back({ r, c });
        } else if (ignoreBlockers) {
            result.push_back({ r, c }); // חוסם, אבל ממשיכים "דרכו" (סעיף 7)
        } else {
            if (board.getPiece({ r, c })->getColor() != piece.getColor())
                result.push_back({ r, c }); // אכילה
            break; // חסום
        }
        r += dr;
        c += dc;
    }
}

std::vector<Position> RookRules::moves(const Board& board, const Piece& piece, bool ignoreBlockers) {
    std::vector<Position> result;
    slide(board, piece,  1,  0, result, ignoreBlockers); // למטה
    slide(board, piece, -1,  0, result, ignoreBlockers); // למעלה
    slide(board, piece,  0,  1, result, ignoreBlockers); // ימינה
    slide(board, piece,  0, -1, result, ignoreBlockers); // שמאלה
    return result;
}
