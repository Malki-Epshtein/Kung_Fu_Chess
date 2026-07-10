#include "rule_engine.h"
#include "PieceRules.h"

MoveValidation RuleEngine::validateMove(const Board& board, Position from, Position to) {
    if (!board.isWithinBounds(from) || !board.isWithinBounds(to))
        return { false, "outside_board" };

    auto piece = board.getPiece(from);
    if (!piece || piece->getColor() == Chess::Color::None)
        return { false, "empty_source" };

    auto dest = board.getPiece(to);
    if (dest && dest->getColor() == piece->getColor())
        return { false, "friendly_destination" };

    for (const auto& pos : PieceRules::legalDestinations(board, *piece))
        if (pos == to)
            return { true, "ok" };

    return { false, "illegal_piece_move" };
}
