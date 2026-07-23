#pragma once
#include "../../engine/GameEngine.h"
#include "../../engine/ScoreObserver.h"
#include "../../engine/MoveLogObserver.h"
#include "../../engine/CaptureEventObserver.h"
#include "RoomIdentity.h"
#include "GameResult.h"
#include "../../../shared/bus/EventBus.h"
#include "../../../shared/model/Piece.h"
#include "json.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// A disconnect grace-period countdown for one seated player (Stage D).
// WsServer owns the actual timer and updates this via setDisconnectStatus;
// GameEngine/GameSession never learn a network exists - this is folded into
// the outgoing broadcast JSON alongside (not inside) the real GameSnapshot.
struct DisconnectStatus {
    bool         active = false;
    Chess::Color color = Chess::Color::None;
    int          secondsRemaining = 0;
};

// Owns the one authoritative GameEngine for this game and runs its time
// loop, plus this room's own ScoreObserver/MoveLogObserver (attached to the
// engine at construction - each room gets its own pair, never shared).
// Each tick, sends the resulting GameSnapshot (plus score, and any active
// DisconnectStatus) via snapshotSender_ - a directly-injected callback
// (set by BroadcasterManager::attach), not EventBus. The snapshot is
// continuously-changing STATE re-sent every tick, not a one-off occurrence,
// so it doesn't fit the topic/event model the way a completed move or a
// capture does - those two genuinely are discrete events and still publish
// on the bus (moveLogTopic/captureTopic below), immediately, rather than
// riding along on the periodic snapshot (a move log only ever grows, so
// re-sending the whole thing every tick would be wasted bandwidth; score is
// small and bounded, so it rides the snapshot like DisconnectStatus does).
// Both topics are per-room (Stage G) so a NetworkBroadcaster attached to
// one room never receives another room's data.
class GameSession {
public:
    // No longer published to (see setSnapshotSender below) - kept for now
    // rather than deleted unilaterally, in case something still wants the
    // name.
    static std::string snapshotTopic(const std::string& roomName) { return "game-state-changed:" + roomName; }
    static std::string moveLogTopic(const std::string& roomName) { return "move-logged:" + roomName; }
    static std::string captureTopic(const std::string& roomName) { return "capture-event:" + roomName; }
    // Server-internal only - never subscribed to by a NetworkBroadcaster,
    // unlike the three topics above. Whatever reacts to a finished game
    // (today: EloService) subscribes here directly instead.
    static std::string gameEndedTopic(const std::string& roomName) { return "game-ended:" + roomName; }

    GameSession(std::shared_ptr<Board> board, EventBus& bus, std::string roomName, bool simultaneousMode = true);

    GameEngine& engine() { return engine_; }

    void setDisconnectStatus(DisconnectStatus status) { disconnectStatus_ = status; }

    // Called once by BroadcasterManager::attach, right after this room is
    // created - the direct delivery path for the periodic snapshot (see the
    // class comment above for why this isn't EventBus). Empty by default
    // (no-op), same convention as MoveLogObserver::onNewEntry.
    void setSnapshotSender(std::function<void(const std::string&)> sender) { snapshotSender_ = std::move(sender); }

    // Resolved fresh every tick by whoever drives the tick loop (WsServer -
    // it's the one place with both SessionRegistry and
    // ClientSessionRegistry in scope) via RoomIdentityResolver, then handed
    // in here so GameSession itself never needs to know either registry
    // exists - same "computed outside, pushed in" shape as
    // setDisconnectStatus above.
    void setIdentity(RoomIdentity identity) { identity_ = std::move(identity); }

    // Called directly by SessionRegistry's disconnect-countdown timer, the
    // moment a grace period fully expires (Stage D) - the game engine
    // itself never learns a network/disconnect exists, so this is the only
    // way a disconnect-caused ending ever reaches gameEndedTopic below.
    // loserUsername/loserElo must be captured by the caller BEFORE this -
    // the disconnected connection is already gone from SessionRegistry by
    // the time the grace period expires, so identity_ no longer has it.
    // The winner's identity is read from identity_ here instead, since
    // they're still connected and identity_ stays fresh for them.
    void markDisconnectResign(Chess::Color loser, std::string loserUsername, int loserElo);

    // The room's full move history so far, ready to embed as backfill in a
    // room-join reply for a connection joining a game already in progress.
    nlohmann::json fullMoveLog() const;

    // tick() itself stays safe to call however it always was (computeStep
    // then publishStep, back-to-back) - see computeStep/publishStep below
    // for the split that lets a caller run the two halves on different
    // threads instead, once it needs to.
    void tick(int ms) { publishStep(computeStep(ms)); }

    // The simulation step - safe to run off the main thread. Advances the
    // engine and returns the resulting snapshot payload (already tagged,
    // scored, etc.) instead of sending it anywhere. Touches only this
    // room's own engine_/scoreObserver_/identity_/disconnectStatus_ - never
    // shared with any other room's processing. moveLogObserver_/
    // captureEventObserver_'s callbacks may fire synchronously from inside
    // engine_.wait() (see the constructor) - they enqueue into pending_
    // (mutex-guarded, safe from any thread) rather than publishing
    // directly, since EventBus::publish/subscribe themselves are not
    // synchronized.
    nlohmann::json computeStep(int ms);

    // The delivery step - must run on the main thread (the one every other
    // room's message handling and this room's own EventBus subscribers
    // already exclusively run on). Drains whatever computeStep() just
    // queued into pending_ and publishes it, sends the snapshot, and checks
    // for a king-capture ending - exactly what tick() used to do inline,
    // just no longer coupled to computeStep() running on the same thread.
    void publishStep(const nlohmann::json& snapshotPayload);

    // Guards against submitting this same room's computeStep() to the
    // thread pool a second time while its previous tick is still running
    // (e.g. tickMs elapsing on the main thread's timer faster than a slow
    // tick finishes on its worker) - the caller (WsServer's onTick) checks
    // tryBeginTick() before submitting, and must call endTick() once
    // publishStep() for that same tick has completed. Not needed by tick()
    // itself (synchronous, never overlaps with itself), only by a caller
    // that runs computeStep()/publishStep() on separate threads.
    bool tryBeginTick() {
        bool expected = false;
        return tickInFlight_.compare_exchange_strong(expected, true);
    }
    void endTick() { tickInFlight_.store(false); }

    // Whether a computeStep() submitted for this room is still running on a
    // worker thread. SessionRegistry checks this before erasing an emptied
    // room - erasing it while this is true would destroy engine_ (and
    // everything else computeStep() is touching) out from under that worker,
    // so removal must be deferred until it's false again (see
    // SessionRegistry::leave/reapIfSafe).
    bool tickInFlight() const { return tickInFlight_.load(); }

private:
    // Publishes `result` on gameEndedTopic and marks the game as having
    // reported its result already - called from at most one of
    // publishStep() (king capture) or markDisconnectResign() (disconnect),
    // whichever happens first; a no-op guard, not re-checked here, since
    // both call sites already check gameResultPublished_/engine_ state
    // themselves before calling this.
    void publishGameEnded(const GameResult& result);

    // One queued move-log or capture notification, waiting to be published
    // on the main thread - see computeStep's comment for why this exists.
    struct PendingPublish {
        std::string    topic;
        nlohmann::json data;
    };

    GameEngine        engine_;
    EventBus&         bus_;
    std::string       roomName_;
    ScoreObserver      scoreObserver_;
    MoveLogObserver     moveLogObserver_;
    CaptureEventObserver captureEventObserver_;
    DisconnectStatus     disconnectStatus_{};
    RoomIdentity          identity_{};
    bool                   gameResultPublished_ = false;
    std::function<void(const std::string&)> snapshotSender_;
    std::mutex                   pendingMutex_;
    std::vector<PendingPublish>  pending_;
    std::atomic<bool>            tickInFlight_{false};
};
