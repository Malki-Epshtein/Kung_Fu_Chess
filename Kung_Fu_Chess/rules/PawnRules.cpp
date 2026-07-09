#include "PawnRules.h"

std::vector<Position> PawnRules::moves(const Board& board, const Piece& piece) {
    std::vector<Position> result;
    int r = piece.getCell().row;
    int c = piece.getCell().col;

    // לבן זז למעלה (row-1), שחור זז למטה (row+1)
    int dir = (piece.getColor() == Chess::Color::White) ? -1 : 1;
    int startRow = (piece.getColor() == Chess::Color::White)
        ? board.getHeight() - 1 : 0;

    // צעד אחד קדימה - רק אם ריק
    Position oneStep{ r + dir, c };
    if (board.isWithinBounds(oneStep) && board.isCellEmpty(oneStep)) {
        result.push_back(oneStep);

        // שני צעדים מהשורה ההתחלתית - רק אם גם התא הבא ריק
        Position twoStep{ r + 2 * dir, c };
        if (r == startRow && board.isWithinBounds(twoStep) && board.isCellEmpty(twoStep))
            result.push_back(twoStep);
    }

    // אכילה באלכסון - רק אם יש כלי אויב
    for (int dc : {-1, 1}) {
        Position diag{ r + dir, c + dc };
        if (!board.isWithinBounds(diag)) continue;
        if (!board.isCellEmpty(diag) &&
            board.getPiece(diag)->getColor() != piece.getColor())
            result.push_back(diag);
    }

    return result;
}
