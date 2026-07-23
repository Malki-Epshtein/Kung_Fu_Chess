#include "CollisionResolver.h"
#include "MotionPath.h"
#include "RestDurations.h"
#include <algorithm>
#include <climits>

namespace {
    // A motion's occupancy of one cell along its path, and one edge (step
    // between two adjacent cells) along its path - both derived once from
    // pathCheckpoints so resolveCollisions and resolveMovingFriendlyBlocks
    // can share the same detection: same-cell overlap (staggered arrivals,
    // not just an exact-same-millisecond match) and edge-swap overlap (two
    // pieces trading adjacent squares, which never share a "cell").
    // checkpointIndex is the index into that motion's own pathCheckpoints -
    // needed by resolveMovingFriendlyBlocks to know where to truncate back to.
    struct Occupancy { Position cell; int enter; int leave; size_t motionIndex; int checkpointIndex; };
    struct Step { Position from; Position to; int enter; int leave; size_t motionIndex; int checkpointIndex; };

    //זה אומר לי שאפשר
    bool overlaps(int enter1, int leave1, int enter2, int leave2) {
        return std::max(enter1, enter2) < std::min(leave1, leave2);
    }

    void collect(const Board& board, const std::vector<Motion>& activeMotions,
        int previousClock, int gameClockMs,
        std::vector<Occupancy>& occ, std::vector<Step>& steps)
    {
        for (size_t i = 0; i < activeMotions.size(); ++i) {
            const Motion& m = activeMotions[i];
            if (m.from == m.to) continue; // jump, no path
            auto piece = board.getPiece(m.from);
            if (!piece || piece->getKind() == Chess::Kind::Knight) continue;

            auto checkpoints = pathCheckpoints(m);
            Position prevCell = m.from;
            int prevTime = m.start_time_ms;
            for (size_t k = 0; k < checkpoints.size(); ++k) {
                int enter = checkpoints[k].second;
                int leave = (k + 1 < checkpoints.size()) ? checkpoints[k + 1].second : INT_MAX;
                if (leave > previousClock && enter <= gameClockMs)
                    occ.push_back({ checkpoints[k].first, enter, leave, i, (int)k });//השהיה
                if (enter > previousClock && prevTime <= gameClockMs)
                    steps.push_back({ prevCell, checkpoints[k].first, prevTime, enter, i, (int)k });//הצעדים של ההחלפות
                prevCell = checkpoints[k].first;
                prevTime = enter;
            }
        }
    }
}

bool CollisionResolver::isPieceSliding(int pieceId) const {
    // A jump (from==to) never actually leaves its cell, so it's always a
    // certain blocker - unlike a real slide, which might move out of the
    // way before the other motion gets there. Only a real slide defers to
    // resolveCollisions/resolveMovingFriendlyBlocks.
    for (const auto& m : activeMotions)
        if (m.piece_id == pieceId && m.from != m.to)
            return true;
    return false;
}

bool CollisionResolver::applyNearMiss(Motion& motion) const {
    // Friendly near-miss: if this motion's path shares a cell with an
    // already-active, same-color, non-Knight motion's path, and this motion
    // would reach that cell at the same simulated time or later, stop one
    // cell short of it instead of continuing through.
    auto movingPiece = board.getPiece(motion.from);
    if (!movingPiece || movingPiece->getKind() == Chess::Kind::Knight)
        return true; // Knights are exempt

    auto newPath = pathCheckpoints(motion);
    int truncateAt = -1; // index into newPath to stop before

    for (const auto& other : activeMotions) {
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

void CollisionResolver::resolveCollisions(int previousClock, int gameClockMs, bool& kingCaptured) {
    // Enemy collision: either two motions occupy the same cell during
    // overlapping time windows (catches staggered arrivals, not just an
    // exact-same-millisecond match), or they swap adjacent cells (their
    // paths cross an edge without ever sharing a cell). Knights are exempt.
    // Whichever motion started moving first survives; ties break on the
    // lower piece_id (stable/deterministic).
    std::vector<Occupancy> occ;
    std::vector<Step> steps;
    collect(board, activeMotions, previousClock, gameClockMs, occ, steps);

    std::vector<bool> loses(activeMotions.size(), false);
    // Per losing motion index, the animation-event data the survivor's
    // capture should carry (see CaptureObserver.h) - the winner's own id,
    // where the paths crossed, and the winner's own travelProgress at that
    // exact moment (computed from real overlap timing, not a distance
    // estimate) so the client can fire the capture sound in sync with what
    // it's actually drawing, not the instant this resolver decided the
    // outcome.
    std::unordered_map<size_t, CaptureImpact> impactFor;

    auto mark = [&](size_t idxA, size_t idxB, Position cellA, Position cellB, int enterA, int enterB) {
        const Motion& mA = activeMotions[idxA];
        const Motion& mB = activeMotions[idxB];
        auto pieceA = board.getPiece(mA.from);
        auto pieceB = board.getPiece(mB.from);
        if (!pieceA || !pieceB || pieceA->getColor() == pieceB->getColor())
            return;

        bool aWins = (mA.start_time_ms != mB.start_time_ms)
            ? (mA.start_time_ms < mB.start_time_ms)
            : (mA.piece_id < mB.piece_id);

        size_t loserIdx        = aWins ? idxB : idxA;
        const Motion& winner    = aWins ? mA : mB;
        Position collisionCell = aWins ? cellA : cellB; // the winner's own proposed cell for this pairing

        // The moment both pieces actually overlap - the later of the two
        // entries into the shared space, not either one's own entry alone.
        int collisionTimeMs = std::max(enterA, enterB);
        int duration = winner.arrival_time_ms - winner.start_time_ms;
        double impactProgress = duration > 0
            ? std::clamp(static_cast<double>(collisionTimeMs - winner.start_time_ms) / duration, 0.0, 1.0)
            : 1.0;

        loses[loserIdx] = true;
        impactFor[loserIdx] = CaptureImpact{ winner.piece_id, collisionCell, impactProgress };
    };

    for (size_t i = 0; i < occ.size(); ++i)
        for (size_t j = i + 1; j < occ.size(); ++j) {
            if (occ[i].motionIndex == occ[j].motionIndex) continue;
            if (occ[i].cell != occ[j].cell) continue;
            if (!overlaps(occ[i].enter, occ[i].leave, occ[j].enter, occ[j].leave)) continue;
            mark(occ[i].motionIndex, occ[j].motionIndex, occ[i].cell, occ[j].cell, occ[i].enter, occ[j].enter);
        }

    for (size_t i = 0; i < steps.size(); ++i)
        for (size_t j = i + 1; j < steps.size(); ++j) {
            if (steps[i].motionIndex == steps[j].motionIndex) continue;
            if (!(steps[i].from == steps[j].to && steps[i].to == steps[j].from)) continue;
            if (!overlaps(steps[i].enter, steps[i].leave, steps[j].enter, steps[j].leave)) continue;
            // Edge swap: no single shared cell (the two paths cross a shared
            // edge, never a shared cell) - each side's own "to" is a
            // reasonable stand-in for where that piece visually was at the
            // crossing, and mark() already picks whichever belongs to the
            // winner.
            mark(steps[i].motionIndex, steps[j].motionIndex, steps[i].to, steps[j].to, steps[i].enter, steps[j].enter);
        }

    bool anyLoss = false;
    for (size_t i = 0; i < activeMotions.size(); ++i) {
        if (!loses[i]) continue;
        anyLoss = true;
        const Motion& m = activeMotions[i];
        auto piece = board.getPiece(m.from);
        if (!piece) continue;
        if (piece->getKind() == Chess::Kind::King)
            kingCaptured = true;
        piece->transitionTo(Chess::State::Captured);
        capturedPieces.push_back(piece);
        // impactFor[i] is always populated here (every loses[i]=true is set
        // in the same mark() call as its impactFor[i] entry) - the fallback
        // is just defensive.
        auto it = impactFor.find(i);
        CaptureImpact impact = it != impactFor.end() ? it->second : CaptureImpact{ piece->getId(), m.from, 1.0 };
        for (auto* observer : captureObservers)
            observer->onPieceCaptured(*piece, impact);
        board.removePiece(m.from);
    }

    if (anyLoss) {
        std::vector<Motion> survivors;
        for (size_t i = 0; i < activeMotions.size(); ++i)
            if (!loses[i])
                survivors.push_back(activeMotions[i]);
        activeMotions = survivors;
    }
}

void CollisionResolver::resolveMovingFriendlyBlocks(int previousClock, int gameClockMs) {
    // Same detection as resolveCollisions (cell overlap + edge swap), but
    // for same-color pairs: whichever motion started later is the one
    // "moving into" territory the earlier mover already committed to, so it
    // stops - reusing the exact truncate-to-previous-checkpoint/cancel-at-
    // first-step mechanics resolveStationaryBlocks already uses for a
    // stationary blocker.
    std::vector<Occupancy> occ;
    std::vector<Step> steps;
    collect(board, activeMotions, previousClock, gameClockMs, occ, steps);

    std::unordered_map<size_t, int> stopAt; // motionIndex -> earliest checkpoint it must not enter
    auto mark = [&](size_t idxA, size_t idxB, int cpIndexA, int cpIndexB) {
        const Motion& mA = activeMotions[idxA];
        const Motion& mB = activeMotions[idxB];
        auto pieceA = board.getPiece(mA.from);
        auto pieceB = board.getPiece(mB.from);
        if (!pieceA || !pieceB || pieceA->getColor() != pieceB->getColor())
            return;

        bool aWins = (mA.start_time_ms != mB.start_time_ms)
            ? (mA.start_time_ms < mB.start_time_ms)
            : (mA.piece_id < mB.piece_id);

        size_t loserIdx = aWins ? idxB : idxA;
        int loserCpIndex = aWins ? cpIndexB : cpIndexA;
        auto it = stopAt.find(loserIdx);
        if (it == stopAt.end() || loserCpIndex < it->second)
            stopAt[loserIdx] = loserCpIndex;
    };

    for (size_t i = 0; i < occ.size(); ++i)
        for (size_t j = i + 1; j < occ.size(); ++j) {
            if (occ[i].motionIndex == occ[j].motionIndex) continue;
            if (occ[i].cell != occ[j].cell) continue;
            if (!overlaps(occ[i].enter, occ[i].leave, occ[j].enter, occ[j].leave)) continue;
            mark(occ[i].motionIndex, occ[j].motionIndex, occ[i].checkpointIndex, occ[j].checkpointIndex);
        }

    for (size_t i = 0; i < steps.size(); ++i)
        for (size_t j = i + 1; j < steps.size(); ++j) {
            if (steps[i].motionIndex == steps[j].motionIndex) continue;
            if (!(steps[i].from == steps[j].to && steps[i].to == steps[j].from)) continue;
            if (!overlaps(steps[i].enter, steps[i].leave, steps[j].enter, steps[j].leave)) continue;
            mark(steps[i].motionIndex, steps[j].motionIndex, steps[i].checkpointIndex, steps[j].checkpointIndex);
        }

    std::vector<size_t> toErase;
    for (const auto& entry : stopAt) {
        size_t idx = entry.first;
        int cpIndex = entry.second;
        Motion& m = activeMotions[idx];
        auto piece = board.getPiece(m.from);
        if (!piece) continue;

        if (cpIndex == 0) {
            cooldownUntilMs[m.piece_id] = gameClockMs + RestDurations::LONG_REST_MS;
            piece->transitionTo(Chess::State::LongRest);
            toErase.push_back(idx);
        } else {
            auto checkpoints = pathCheckpoints(m);
            m.to = checkpoints[cpIndex - 1].first;
            m.arrival_time_ms = checkpoints[cpIndex - 1].second;
        }
    }

    if (!toErase.empty()) {
        std::sort(toErase.begin(), toErase.end());
        for (auto it = toErase.rbegin(); it != toErase.rend(); ++it)
            activeMotions.erase(activeMotions.begin() + *it);
    }
}

void CollisionResolver::resolveStationaryBlocks(int previousClock, int gameClockMs) {
    // A slide is allowed to be requested through a currently-occupied cell
    // (the blocker might move away in time), but if that blocker is still
    // sitting there - not itself mid-motion, that pair is resolveCollisions'
    // job - when the slide actually reaches it, the slide can't continue
    // past it after all. Redirect the motion to end at that checkpoint
    // instead of its originally requested destination, then let the normal
    // ArrivalResolver capture/blockedByFriendly logic (already correct for a
    // final destination) handle it as if that had been the destination all
    // along.
    for (size_t i = 0; i < activeMotions.size(); ) {
        Motion& m = activeMotions[i];
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
            if (time_ms <= previousClock || time_ms > gameClockMs) continue;
            if (cell == m.to) continue; // final destination - ArrivalResolver already handles it

            auto blocker = board.getPiece(cell);
            if (!blocker || blocker->getKind() == Chess::Kind::None) continue;
            if (isPieceSliding(blocker->getId())) continue; // actively-sliding blocker: resolveCollisions' job

            // A jumping (or just-landed, still-ShortRest) blocker defends its
            // cell against anyone arriving there, friend or foe alike (see
            // ArrivalResolver's targetRecentlyJumped) - so redirect the slide
            // all the way to it instead of stopping short, even if friendly.
            bool blockerJumping = blocker->getState() == Chess::State::Jump ||
                blocker->getState() == Chess::State::ShortRest;

            if (blocker->getColor() == mover->getColor() && !blockerJumping) {
                if (idx == 0) {
                    // Blocked before taking even a single step - there's no
                    // earlier checkpoint to redirect to, so resolve the
                    // failed move directly here instead of going through
                    // ArrivalResolver (m.from == m.to would misread as a
                    // jump and attempt an illegal Moving -> ShortRest
                    // transition).
                    cooldownUntilMs[m.piece_id] = gameClockMs + RestDurations::LONG_REST_MS;
                    mover->transitionTo(Chess::State::LongRest);
                    activeMotions.erase(activeMotions.begin() + i);
                    erased = true;
                } else {
                    // Friendly and still there - stop one cell short of it.
                    m.to = checkpoints[idx - 1].first;
                    m.arrival_time_ms = checkpoints[idx - 1].second;
                }
            } else {
                // Enemy and still there, or a jumping/just-landed blocker of
                // either color - captured on the spot, slide ends here.
                m.to = cell;
                m.arrival_time_ms = time_ms;
            }
            break;
        }
        if (!erased) ++i;
    }
}
