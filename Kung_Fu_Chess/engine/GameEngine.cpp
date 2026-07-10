#include "GameEngine.h"
#include "../rules/rule_engine.h"

bool GameEngine::hasMotionOnPath(Position from, Position to) const {
    for (const auto& m : arbiter.getActiveMotions())
        if (m.from == from || m.to == to)
            return true;
    return false;
}

MoveResult GameEngine::requestMove(Position from, Position to) {
    if (state.game_over)
        return { false, "game_over" };

    if (hasMotionOnPath(from, to))
        return { false, "motion_in_progress" };

    auto validation = RuleEngine::validateMove(*state.board, from, to);
    if (!validation.is_valid)
        return { false, validation.reason };

    auto piece = state.board->getPiece(from);
    arbiter.addMotion(from, to, piece->getId());
    return { true, "ok" };
}

void GameEngine::wait(int ms) {
    if (arbiter.tick(ms))
        state.game_over = true;
}
