#include "ScoreObserver.h"

namespace {
    // Standard chess piece values. King and every other kind are excluded
    // (0 via the switch's default) - capturing a king already ends the game
    // via a separate mechanism (king_captured), not a scoring event.
    constexpr int PAWN_VALUE   = 1;
    constexpr int KNIGHT_VALUE = 3;
    constexpr int BISHOP_VALUE = 3;
    constexpr int ROOK_VALUE   = 5;
    constexpr int QUEEN_VALUE  = 9;
}

int ScoreObserver::pieceValue(Chess::Kind kind) {
    switch (kind) {
        case Chess::Kind::Pawn:   return PAWN_VALUE;
        case Chess::Kind::Knight: return KNIGHT_VALUE;
        case Chess::Kind::Bishop: return BISHOP_VALUE;
        case Chess::Kind::Rook:   return ROOK_VALUE;
        case Chess::Kind::Queen:  return QUEEN_VALUE;
        default:                  return 0;
    }
}

void ScoreObserver::onPieceCaptured(const Piece& captured, const CaptureImpact& /*impact*/) {
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
