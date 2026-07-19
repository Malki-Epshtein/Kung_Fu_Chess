#include "rule_engine.h"
#include "PieceRules.h"

MoveValidation RuleEngine::validateMove(const Board& board, Position from, Position to, bool relaxedBlocking) {
    if (!board.isWithinBounds(from) || !board.isWithinBounds(to))
        return { false, "outside_board" };

    auto piece = board.getPiece(from);
    if (!piece || piece->getColor() == Chess::Color::None)
        return { false, "empty_source" };

    // Section 7: with relaxedBlocking, a friendly-occupied destination is not
    // rejected outright either - the blocker might move out of the way
    // before this piece gets there (enforced for real at arrival time by
    // ArrivalResolver/CollisionResolver). Knight is always exempt regardless
    // (rule 8).
    auto dest = board.getPiece(to);
    if (dest && dest->getColor() == piece->getColor() &&
        piece->getKind() != Chess::Kind::Knight && !relaxedBlocking)
        return { false, "friendly_destination" };

    for (const auto& pos : PieceRules::legalDestinations(board, *piece, relaxedBlocking))
        if (pos == to)
            return { true, "ok" };

    return { false, "illegal_piece_move" };
}

MoveValidation RuleEngine::validateJump(const Board& board, Position pos) {
    if (!board.isWithinBounds(pos))
        return { false, "outside_board" };

    auto piece = board.getPiece(pos);
    if (!piece || piece->getColor() == Chess::Color::None)
        return { false, "empty_source" };

    return { true, "ok" };
}
