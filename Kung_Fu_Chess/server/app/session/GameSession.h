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
#include <memory>
#include <string>

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
// Each tick, publishes the resulting GameSnapshot (plus score, and any
// active DisconnectStatus) on the bus instead of calling any broadcaster
// directly - NetworkBroadcaster subscribes to snapshotTopic(roomName)
// rather than being wired in here. A completed move publishes separately
// and immediately on moveLogTopic(roomName), rather than riding along on
// the periodic snapshot - a move log only ever grows, so re-sending the
// whole thing every tick would be wasted bandwidth (score is small and
// bounded, so it rides the snapshot like DisconnectStatus does). Both
// topics are per-room (Stage G) so a NetworkBroadcaster attached to one
// room never receives another room's data.
class GameSession {
public:
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

    void tick(int ms);

private:
    // Publishes `result` on gameEndedTopic and marks the game as having
    // reported its result already - called from at most one of tick()
    // (king capture) or markDisconnectResign() (disconnect), whichever
    // happens first; a no-op guard, not re-checked here, since both call
    // sites already check gameResultPublished_/engine_ state themselves
    // before calling this.
    void publishGameEnded(const GameResult& result);

    GameEngine        engine_;
    EventBus&         bus_;
    std::string       roomName_;
    ScoreObserver      scoreObserver_;
    MoveLogObserver     moveLogObserver_;
    CaptureEventObserver captureEventObserver_;
    DisconnectStatus     disconnectStatus_{};
    RoomIdentity          identity_{};
    bool                   gameResultPublished_ = false;
};
