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

    auto piece = board.getPiece(from);
    if (piece)
        piece->transitionTo(Chess::State::Moving);

    return true;
}

void RealTimeArbiter::addJump(Position pos, int piece_id) {
    active_motions.push_back({ pos, pos, game_clock_ms, game_clock_ms + JUMP_DURATION_MS, piece_id });

    auto piece = board.getPiece(pos);
    if (piece)
        piece->transitionTo(Chess::State::Jump);
}

int RealTimeArbiter::getStateStartMs(int piece_id, Chess::State state) const {
    for (const auto& m : active_motions)
        if (m.piece_id == piece_id)
            return m.start_time_ms;

    auto it = cooldown_until_ms.find(piece_id);
    if (it != cooldown_until_ms.end()) {
        int duration = 0;
        if (state == Chess::State::ShortRest) duration = SHORT_REST_MS;
        else if (state == Chess::State::LongRest) duration = LONG_REST_MS;
        // Idle (and anything else): the tracked timestamp IS the state's own
        // entry time, refreshed in tick() when the piece becomes Idle.
        int startMs = it->second - duration;
        // A state can never have started later than "now" - clamp so a
        // stale cooldown entry (e.g. left over on a piece that was
        // captured mid-rest, or a same-tick ordering edge case around a
        // capture) can never make elapsed_in_state_ms negative downstream.
        return std::min(startMs, game_clock_ms);
    }

    // Never touched this game (no motion or rest ever recorded) - treat as
    // idle since time 0, so its animation still cycles instead of freezing
    // on frame 0 forever.
    return 0;
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
        piece->transitionTo(Chess::State::Captured);
        captured_pieces.push_back(piece);
        for (auto* observer : captureObservers)
            observer->onPieceCaptured(*piece);
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

void RealTimeArbiter::resolveStationaryBlocks(int previous_clock) {
    // A slide is allowed to be requested through a currently-occupied cell
    // (the blocker might move away in time), but if that blocker is still
    // sitting there - not itself mid-motion, that pair is resolveCollisions'
    // job - when the slide actually reaches it, the slide can't continue
    // past it after all. Redirect the motion to end at that checkpoint
    // instead of its originally requested destination, then let the normal
    // resolveArrival capture/blockedByFriendly logic (already correct for a
    // final destination) handle it as if that had been the destination all
    // along.
    for (size_t i = 0; i < active_motions.size(); ) {
        Motion& m = active_motions[i];
        if (m.from == m.to) { ++i; continue; } // jump, no path to block

        auto mover = board.getPiece(m.from);
        if (!mover || mover->getKind() == Chess::Kind::Knight) {
            ++i; continue; // knights ignore blockers entirely
        }

        auto checkpoints = pathCheckpoints(m);
        bool erased = false;
        for (size_t idx = 0; idx < checkpoints.size(); ++idx) {
            const Position& cell = checkpoints[idx].first;
            int time_ms = checkpoints[idx].second;
            if (time_ms <= previous_clock || time_ms > game_clock_ms) continue;
            if (cell == m.to) continue; // final destination - resolveArrival already handles it

            auto blocker = board.getPiece(cell);
            if (!blocker || blocker->getKind() == Chess::Kind::None) continue;
            if (isPieceBusy(blocker->getId())) continue; // actively-moving blocker: resolveCollisions' job

            if (blocker->getColor() == mover->getColor()) {
                if (idx == 0) {
                    // Blocked before taking even a single step - there's no
                    // earlier checkpoint to redirect to, so resolve the
                    // failed move directly here instead of going through
                    // resolveArrival (m.from == m.to would misread as a jump
                    // and attempt an illegal Moving -> ShortRest transition).
                    cooldown_until_ms[m.piece_id] = game_clock_ms + LONG_REST_MS;
                    mover->transitionTo(Chess::State::LongRest);
                    active_motions.erase(active_motions.begin() + i);
                    erased = true;
                } else {
                    // Friendly and still there - stop one cell short of it.
                    m.to = checkpoints[idx - 1].first;
                    m.arrival_time_ms = checkpoints[idx - 1].second;
                }
            } else {
                // Enemy and still there - captured on the spot, slide ends here.
                m.to = cell;
                m.arrival_time_ms = time_ms;
            }
            break;
        }
        if (!erased) ++i;
    }
}

void RealTimeArbiter::resolveArrival(const Motion& m, bool& king_captured) {
    bool isJump = (m.from == m.to);
    cooldown_until_ms[m.piece_id] = game_clock_ms + (isJump ? SHORT_REST_MS : LONG_REST_MS);

    if (isJump) {
        auto jumper = board.getPiece(m.from);
        if (jumper && jumper->getId() == m.piece_id)
            jumper->transitionTo(Chess::State::ShortRest);
        return; // jump landing: the piece never left its cell, nothing to resolve here
    }

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
        piece->transitionTo(Chess::State::Captured);
        captured_pieces.push_back(piece);
        for (auto* observer : captureObservers)
            observer->onPieceCaptured(*piece);
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

    piece->transitionTo(Chess::State::LongRest);

    if (blockedByFriendly)
        return;

    if (target && target->getKind() == Chess::Kind::King)
        king_captured = true;

    if (target && target->getKind() != Chess::Kind::None) {
        target->transitionTo(Chess::State::Captured);
        captured_pieces.push_back(target);
        for (auto* observer : captureObservers)
            observer->onPieceCaptured(*target);
    }

    board.movePiece(m.from, m.to);

    for (auto* observer : moveObservers)
        observer->onMoveCompleted(*piece, m.from, m.to);

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

    if (collisionEnabled) {
        resolveCollisions(previous_clock, king_captured);
        resolveStationaryBlocks(previous_clock);
    }

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

    for (const auto& entry : cooldown_until_ms) {
        if (entry.second > previous_clock && entry.second <= game_clock_ms) {
            auto piece = board.getPieceById(entry.first);
            if (piece) {
                piece->transitionTo(Chess::State::Idle);
                // Refresh the tracked timestamp to "now" so getStateStartMs
                // has a correct Idle-entry reference instead of the stale
                // rest-expiry value from before this transition.
                cooldown_until_ms[entry.first] = game_clock_ms;
            }
        }
    }

    return king_captured;
}
