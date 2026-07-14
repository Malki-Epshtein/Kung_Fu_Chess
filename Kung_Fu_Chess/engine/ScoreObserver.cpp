#include "ScoreObserver.h"

// Standard chess piece values. King is excluded (0) - capturing a king
// already ends the game via a separate mechanism (king_captured), not a
// scoring event.
int ScoreObserver::pieceValue(Chess::Kind kind) {
    switch (kind) {
        case Chess::Kind::Pawn:   return 1;
        case Chess::Kind::Knight: return 3;
        case Chess::Kind::Bishop: return 3;
        case Chess::Kind::Rook:   return 5;
        case Chess::Kind::Queen:  return 9;
        default:                  return 0;
    }
}

void ScoreObserver::onPieceCaptured(const Piece& captured) {
    int value = pieceValue(captured.getKind());

    // Capturing an enemy piece earns the OPPOSING color the points.
    if (captured.getColor() == Chess::Color::White)
        blackScore += value;
    else if (captured.getColor() == Chess::Color::Black)
        whiteScore += value;
}

int ScoreObserver::getScore(Chess::Color color) const {
    if (color == Chess::Color::White) return whiteScore;
    if (color == Chess::Color::Black) return blackScore;
    return 0;
}
