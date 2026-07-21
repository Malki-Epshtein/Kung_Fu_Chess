#pragma once
#include "../../engine/GameEngine.h"
#include "../../engine/ScoreObserver.h"
#include "../../engine/MoveLogObserver.h"
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

    GameSession(std::shared_ptr<Board> board, EventBus& bus, std::string roomName, bool simultaneousMode = true);

    GameEngine& engine() { return engine_; }

    void setDisconnectStatus(DisconnectStatus status) { disconnectStatus_ = status; }

    // The room's full move history so far, ready to embed as backfill in a
    // room-join reply for a connection joining a game already in progress.
    nlohmann::json fullMoveLog() const;

    void tick(int ms);

private:
    GameEngine        engine_;
    EventBus&         bus_;
    std::string       roomName_;
    ScoreObserver      scoreObserver_;
    MoveLogObserver     moveLogObserver_;
    DisconnectStatus     disconnectStatus_{};
};
