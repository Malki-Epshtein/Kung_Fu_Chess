#include "SlidingPieceRules.h"

namespace {
    void slideOneDirection(const Board& board, const Piece& piece,
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
}

std::vector<Position> SlidingPieceRules::slide(const Board& board, const Piece& piece,
    const std::vector<std::pair<int, int>>& directions, bool ignoreBlockers)
{
    std::vector<Position> result;
    for (size_t i = 0; i < directions.size(); ++i)
        slideOneDirection(board, piece, directions[i].first, directions[i].second, result, ignoreBlockers);
    return result;
}
