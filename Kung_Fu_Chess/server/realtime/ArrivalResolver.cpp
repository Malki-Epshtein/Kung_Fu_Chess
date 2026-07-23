#include "ArrivalResolver.h"
#include "RestDurations.h"

void ArrivalResolver::resolve(const Motion& m, int gameClockMs, bool& kingCaptured) {
    bool isJump = (m.from == m.to);
    cooldownUntilMs[m.piece_id] = gameClockMs + (isJump ? RestDurations::SHORT_REST_MS : RestDurations::LONG_REST_MS);

    if (isJump) {
        auto jumper = board.getPiece(m.from);
        if (jumper && jumper->getId() == m.piece_id)
            jumper->transitionTo(Chess::State::ShortRest);
        return; // jump landing: the piece never left its cell, nothing to resolve here
    }

    auto piece = board.getPiece(m.from);
    if (!piece || piece->getId() != m.piece_id)
        return;

    auto target = board.getPiece(m.to);

    // A piece that jumped (or is still in the short rest right after landing
    // from one) defends its cell: anyone arriving there is captured instead
    // of capturing it, friend or foe - jump protection overrides the normal
    // friendly-blocking rule. Checked as a state, not an activeMotions
    // timestamp - the exact instant the jump's own Motion resolves is too
    // narrow a window to rely on, and by then the jumper's Piece object may
    // already be gone if the arriving piece got there first.
    bool targetRecentlyJumped = target &&
        (target->getState() == Chess::State::Jump || target->getState() == Chess::State::ShortRest);

    if (targetRecentlyJumped) {
        // piece (the arriver) is the one captured here, and it's removed
        // from the board this same tick - capturingPieceId names itself
        // deliberately: it'll simply be absent from the very next snapshot,
        // which correctly triggers the client's immediate-fire fallback
        // (see CaptureObserver's comment) rather than waiting on a motion
        // that no longer exists. impactProgress=1.0 for the same reason
        // ArrivalResolver's other capture below uses it - captured right
        // at the end of a motion.
        piece->transitionTo(Chess::State::Captured);
        capturedPieces.push_back(piece);
        for (auto* observer : captureObservers)
            observer->onPieceCaptured(*piece, CaptureImpact{ piece->getId(), m.to, 1.0 });
        board.removePiece(m.from);
        return;
    }

    // Rule 8: a non-Knight piece can never kill a friendly piece by landing
    // on it - the arrival simply fails, the mover stays put. A Knight is the
    // one exception - it falls through to the normal capture below.
    bool blockedByFriendly = collisionEnabled && target &&
        target->getColor() == piece->getColor() &&
        piece->getKind() != Chess::Kind::Knight;

    piece->transitionTo(Chess::State::LongRest);

    if (blockedByFriendly)
        return;

    if (target && target->getKind() == Chess::Kind::King)
        kingCaptured = true;

    bool wasCapture = target && target->getKind() != Chess::Kind::None;
    if (wasCapture) {
        target->transitionTo(Chess::State::Captured);
        capturedPieces.push_back(target);
        // piece (the mover, m.piece_id) is the one still standing - it's
        // captured target by landing on it, so impactProgress=1.0: the
        // capture becomes visible at the exact instant piece finishes
        // arriving at m.to, the same moment collisionCell (m.to) is where
        // it visibly is anyway.
        for (auto* observer : captureObservers)
            observer->onPieceCaptured(*target, CaptureImpact{ piece->getId(), m.to, 1.0 });
    }

    board.movePiece(m.from, m.to);

    for (auto* observer : moveObservers)
        observer->onMoveCompleted(*piece, m.from, m.to, wasCapture, gameClockMs);

    if (piece->getKind() == Chess::Kind::Pawn) {
        int promotionRow = (piece->getColor() == Chess::Color::White) ? 0 : board.getHeight() - 1;
        if (m.to.row == promotionRow)
            piece->promoteTo(Chess::Kind::Queen);
    }
}
