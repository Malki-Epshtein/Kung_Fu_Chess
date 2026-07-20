#pragma once
#include "../engine/GameEngine.h"
#include "../../shared/bus/EventBus.h"
#include "../../shared/model/Piece.h"
#include <memory>

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
// loop. Each tick, publishes the resulting GameSnapshot (plus any active
// DisconnectStatus) on the bus instead of calling any broadcaster directly -
// NetworkBroadcaster (Stage C3) subscribes to kSnapshotTopic rather than
// being wired in here.
class GameSession {
public:
    static constexpr const char* kSnapshotTopic = "game-state-changed";

    GameSession(std::shared_ptr<Board> board, EventBus& bus, bool simultaneousMode = true)
        : engine_(board, simultaneousMode), bus_(bus) {}

    GameEngine& engine() { return engine_; }

    void setDisconnectStatus(DisconnectStatus status) { disconnectStatus_ = status; }

    void tick(int ms);

private:
    GameEngine        engine_;
    EventBus&         bus_;
    DisconnectStatus  disconnectStatus_{};
};
