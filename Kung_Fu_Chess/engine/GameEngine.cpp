#include "GameEngine.h"
#include "../rules/rule_engine.h"
#include "../model/PieceStateMachine.h"
#include <vector>

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
    if (state.game_over)
        return { false, "game_over" };

    if (simultaneousMode) {
        auto piece = state.board->getPiece(from);

        if (piece && isBusyState(piece->getState()))
            return { false, "motion_in_progress" };

        if (piece && isRestingState(piece->getState())) {
            auto validation = RuleEngine::validateMove(*state.board, from, to, simultaneousMode);
            if (!validation.is_valid) {
                premoves.erase(piece->getId()); // an illegal move cancels any pending premove
                return { false, validation.reason };
            }
            premoves[piece->getId()] = { from, to };
            return { true, "queued" };
        }
    }
    else if (hasMotionOnPath(from, to)) {
        return { false, "motion_in_progress" };
    }

    auto validation = RuleEngine::validateMove(*state.board, from, to, simultaneousMode);
    if (!validation.is_valid)
        return { false, validation.reason };

    auto piece = state.board->getPiece(from);
    if (!arbiter.addMotion(from, to, piece->getId()))
        return { false, "friendly_blocked" };
    return { true, "ok" };
}

MoveResult GameEngine::requestJump(Position pos) {
    if (state.game_over)
        return { false, "game_over" };

    if (hasMotionOnPath(pos, pos))
        return { false, "motion_in_progress" };

    auto validation = RuleEngine::validateJump(*state.board, pos);
    if (!validation.is_valid)
        return { false, validation.reason };

    auto piece = state.board->getPiece(pos);
    arbiter.addJump(pos, piece->getId());
    return { true, "ok" };
}

void GameEngine::wait(int ms) {
    if (arbiter.tick(ms))
        state.game_over = true;

    firePremoves();
}

void GameEngine::firePremoves() {
    std::vector<int> readyPieceIds;
    for (const auto& entry : premoves) {
        auto piece = state.board->getPieceById(entry.first);
        if (piece && piece->getState() == Chess::State::Idle)
            readyPieceIds.push_back(entry.first);
    }

    for (int pieceId : readyPieceIds) {
        Premove pm = premoves[pieceId];
        premoves.erase(pieceId);

        auto validation = RuleEngine::validateMove(*state.board, pm.from, pm.to, simultaneousMode);
        if (!validation.is_valid)
            continue; // target became illegal by the time cooldown ended - drop it

        auto piece = state.board->getPiece(pm.from);
        if (piece)
            arbiter.addMotion(pm.from, pm.to, piece->getId());
    }
}

GameSnapshot GameEngine::snapshot() const {
    GameSnapshot snap;
    snap.board_width  = state.board->getWidth();
    snap.board_height = state.board->getHeight();
    snap.game_over    = state.game_over;

    for (int r = 0; r < state.board->getHeight(); ++r) {
        for (int c = 0; c < state.board->getWidth(); ++c) {
            auto piece = state.board->getPieceAt(r, c);
            if (!piece || piece->getKind() == Chess::Kind::None)
                continue;
            snap.pieces.push_back({ piece->getKind(), piece->getColor(), Position{ r, c }, piece->getState() });
        }
    }
    return snap;
}
