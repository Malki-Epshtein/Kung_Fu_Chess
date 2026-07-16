#include "RealTimeArbiter.h"
#include "RestDurations.h"
#include <algorithm>

bool RealTimeArbiter::addMotion(Position from, Position to, int piece_id) {
    int arrival = game_clock_ms + calcTravelTime(from, to);
    Motion newMotion{ from, to, game_clock_ms, arrival, piece_id };

    if (collisionEnabled && !collisionResolver.applyNearMiss(newMotion))
        return false;

    active_motions.push_back(newMotion);

    auto piece = board.getPiece(from);
    if (piece)
        piece->transitionTo(Chess::State::Moving);

    return true;
}

void RealTimeArbiter::addJump(Position pos, int piece_id) {
    active_motions.push_back({ pos, pos, game_clock_ms, game_clock_ms + RestDurations::JUMP_DURATION_MS, piece_id });

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
        if (state == Chess::State::ShortRest) duration = RestDurations::SHORT_REST_MS;
        else if (state == Chess::State::LongRest) duration = RestDurations::LONG_REST_MS;
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

bool RealTimeArbiter::tick(int ms) {
    int previous_clock = game_clock_ms;
    game_clock_ms += ms;
    bool king_captured = false;

    if (collisionEnabled) {
        collisionResolver.resolveCollisions(previous_clock, game_clock_ms, king_captured);
        collisionResolver.resolveMovingFriendlyBlocks(previous_clock, game_clock_ms);
        collisionResolver.resolveStationaryBlocks(previous_clock, game_clock_ms);
    }

    // Snapshot of everything arriving this tick, taken before any mutation,
    // so a jump landing this same tick is still visible to the capture check below.
    std::vector<Motion> arriving;
    for (const auto& m : active_motions)
        if (m.arrival_time_ms <= game_clock_ms)
            arriving.push_back(m);

    for (const auto& m : arriving)
        arrivalResolver.resolve(m, game_clock_ms, king_captured);

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
