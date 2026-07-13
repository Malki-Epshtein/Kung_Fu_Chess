#include "RealTimeArbiter.h"

bool RealTimeArbiter::applyNearMiss(Motion& motion) const {
    // Friendly near-miss: if this motion's path shares a cell with an
    // already-active, same-color, non-Knight motion's path, and this motion
    // would reach that cell at the same simulated time or later, stop one
    // cell short of it instead of continuing through.
    auto movingPiece = board.getPiece(motion.from);
    if (!movingPiece || movingPiece->getKind() == Chess::Kind::Knight)
        return true; // Knights are exempt

    auto newPath = pathCheckpoints(motion);
    int truncateAt = -1; // index into newPath to stop before

    for (const auto& other : active_motions) {
        if (other.from == other.to) continue; // jump, no path
        auto otherPiece = board.getPiece(other.from);
        if (!otherPiece || otherPiece->getKind() == Chess::Kind::Knight) continue;
        if (otherPiece->getColor() != movingPiece->getColor()) continue;

        auto otherPath = pathCheckpoints(other);
        for (size_t i = 0; i < newPath.size(); ++i)
            for (size_t j = 0; j < otherPath.size(); ++j)
                if (newPath[i].first == otherPath[j].first &&
                    newPath[i].second >= otherPath[j].second &&
                    (truncateAt == -1 || (int)i < truncateAt))
                    truncateAt = (int)i;
    }

    if (truncateAt == 0)
        return false; // very first step already claimed by a friendly motion

    if (truncateAt > 0) {
        motion.to = newPath[truncateAt - 1].first;
        motion.arrival_time_ms = newPath[truncateAt - 1].second;
    }
    return true;
}

bool RealTimeArbiter::addMotion(Position from, Position to, int piece_id) {
    int arrival = game_clock_ms + calcTravelTime(from, to);
    Motion newMotion{ from, to, game_clock_ms, arrival, piece_id };

    if (collisionEnabled && !applyNearMiss(newMotion))
        return false;

    active_motions.push_back(newMotion);
    return true;
}

void RealTimeArbiter::addJump(Position pos, int piece_id) {
    active_motions.push_back({ pos, pos, game_clock_ms, game_clock_ms + 1000, piece_id });
}

bool RealTimeArbiter::isPieceBusy(int piece_id) const {
    for (const auto& m : active_motions)
        if (m.piece_id == piece_id)
            return true;
    return false;
}

bool RealTimeArbiter::isPieceCoolingDown(int piece_id) const {
    auto it = cooldown_until_ms.find(piece_id);
    return it != cooldown_until_ms.end() && it->second > game_clock_ms;
}

void RealTimeArbiter::resolveCollisions(int previous_clock, bool& king_captured) {
    // Enemy path collision: paths crossing the same cell at the same
    // simulated millisecond, within the window this tick just advanced
    // through. Knights are exempt. Whichever motion started moving first
    // survives; ties break on the lower piece_id (stable/deterministic).
    struct Checkpoint { Position cell; int time_ms; size_t motionIndex; };
    std::vector<Checkpoint> checkpoints;
    for (size_t i = 0; i < active_motions.size(); ++i) {
        const Motion& m = active_motions[i];
        if (m.from == m.to) continue;
        auto piece = board.getPiece(m.from);
        if (!piece || piece->getKind() == Chess::Kind::Knight) continue;
        for (const auto& step : pathCheckpoints(m))
            if (step.second > previous_clock && step.second <= game_clock_ms)
                checkpoints.push_back({ step.first, step.second, i });
    }

    std::vector<bool> loses(active_motions.size(), false);
    for (size_t i = 0; i < checkpoints.size(); ++i) {
        for (size_t j = i + 1; j < checkpoints.size(); ++j) {
            if (checkpoints[i].motionIndex == checkpoints[j].motionIndex) continue;
            if (checkpoints[i].cell != checkpoints[j].cell) continue;
            if (checkpoints[i].time_ms != checkpoints[j].time_ms) continue;

            size_t idxA = checkpoints[i].motionIndex;
            size_t idxB = checkpoints[j].motionIndex;
            const Motion& mA = active_motions[idxA];
            const Motion& mB = active_motions[idxB];
            auto pieceA = board.getPiece(mA.from);
            auto pieceB = board.getPiece(mB.from);
            if (!pieceA || !pieceB || pieceA->getColor() == pieceB->getColor())
                continue;

            bool aWins = (mA.start_time_ms != mB.start_time_ms)
                ? (mA.start_time_ms < mB.start_time_ms)
                : (mA.piece_id < mB.piece_id);

            loses[aWins ? idxB : idxA] = true;
        }
    }

    bool anyLoss = false;
    for (size_t i = 0; i < active_motions.size(); ++i) {
        if (!loses[i]) continue;
        anyLoss = true;
        const Motion& m = active_motions[i];
        auto piece = board.getPiece(m.from);
        if (!piece) continue;
        if (piece->getKind() == Chess::Kind::King)
            king_captured = true;
        board.removePiece(m.from);
    }

    if (anyLoss) {
        std::vector<Motion> survivors;
        for (size_t i = 0; i < active_motions.size(); ++i)
            if (!loses[i])
                survivors.push_back(active_motions[i]);
        active_motions = survivors;
    }
}

void RealTimeArbiter::resolveArrival(const Motion& m, bool& king_captured) {
    cooldown_until_ms[m.piece_id] = game_clock_ms + COOLDOWN_MS;

    if (m.from == m.to)
        return; // jump landing: the piece never left its cell, nothing to resolve here

    auto piece = board.getPiece(m.from);
    if (!piece || piece->getId() != m.piece_id)
        return;

    bool capturedByJump = false;
    for (const auto& other : active_motions) {
        if (other.from == other.to && other.from == m.to &&
            other.arrival_time_ms >= game_clock_ms) {
            auto jumper = board.getPiece(other.from);
            if (jumper && jumper->getColor() != piece->getColor()) {
                capturedByJump = true;
                break;
            }
        }
    }

    if (capturedByJump) {
        board.removePiece(m.from);
        return;
    }

    auto target = board.getPiece(m.to);

    // Rule 8: a non-Knight piece can never kill a friendly piece by landing
    // on it - the arrival simply fails, the mover stays put. A Knight is the
    // one exception - it falls through to the normal capture below.
    bool blockedByFriendly = collisionEnabled && target &&
        target->getColor() == piece->getColor() &&
        piece->getKind() != Chess::Kind::Knight;

    if (blockedByFriendly)
        return;

    if (target && target->getKind() == Chess::Kind::King)
        king_captured = true;
    board.movePiece(m.from, m.to);

    if (piece->getKind() == Chess::Kind::Pawn) {
        int promotionRow = (piece->getColor() == Chess::Color::White) ? 0 : board.getHeight() - 1;
        if (m.to.row == promotionRow)
            piece->promoteTo(Chess::Kind::Queen);
    }
}

bool RealTimeArbiter::tick(int ms) {
    int previous_clock = game_clock_ms;
    game_clock_ms += ms;
    bool king_captured = false;

    if (collisionEnabled)
        resolveCollisions(previous_clock, king_captured);

    // Snapshot of everything arriving this tick, taken before any mutation,
    // so a jump landing this same tick is still visible to the capture check below.
    std::vector<Motion> arriving;
    for (const auto& m : active_motions)
        if (m.arrival_time_ms <= game_clock_ms)
            arriving.push_back(m);

    for (const auto& m : arriving)
        resolveArrival(m, king_captured);

    for (auto it = active_motions.begin(); it != active_motions.end();) {
        if (it->arrival_time_ms <= game_clock_ms)
            it = active_motions.erase(it);
        else
            ++it;
    }

    return king_captured;
}
