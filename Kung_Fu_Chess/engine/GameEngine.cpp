#include "GameEngine.h"
#include "../rules/rule_engine.h"

bool GameEngine::hasMotionOnPath(Position from, Position to) const {
    for (const auto& m : arbiter.getActiveMotions()) {
        if (m.from == from)
            return true;
        if (m.from != m.to && m.to == to)   // קפיצה (from==to) לא חוסמת מהלך נכנס
            return true;
    }
    return false;
}

MoveResult GameEngine::requestMove(Position from, Position to) {
    if (game_over)
        return { false, "game_over" };

    if (hasMotionOnPath(from, to))
        return { false, "motion_in_progress" };

    auto validation = RuleEngine::validateMove(*board, from, to);
    if (!validation.is_valid)
        return { false, validation.reason };

    auto piece = board->getPiece(from);
    arbiter.addMotion(from, to, piece->getId());
    return { true, "ok" };
}

MoveResult GameEngine::requestJump(Position pos) {
    if (game_over)
        return { false, "game_over" };

    if (hasMotionOnPath(pos, pos))
        return { false, "motion_in_progress" };

    auto validation = RuleEngine::validateJump(*board, pos);
    if (!validation.is_valid)
        return { false, validation.reason };

    auto piece = board->getPiece(pos);
    arbiter.addJump(pos, piece->getId());
    return { true, "ok" };
}

void GameEngine::wait(int ms) {
    if (arbiter.tick(ms))
        game_over = true;
}

GameSnapshot GameEngine::snapshot() const {
    GameSnapshot snap;
    snap.board_width  = board->getWidth();
    snap.board_height = board->getHeight();
    snap.game_over    = game_over;

    for (int r = 0; r < board->getHeight(); ++r) {
        for (int c = 0; c < board->getWidth(); ++c) {
            auto piece = board->getPieceAt(r, c);
            if (!piece || piece->getKind() == Chess::Kind::None)
                continue;
            snap.pieces.push_back({ piece->getKind(), piece->getColor(), Position{ r, c }, piece->getState() });
        }
    }
    return snap;
}
