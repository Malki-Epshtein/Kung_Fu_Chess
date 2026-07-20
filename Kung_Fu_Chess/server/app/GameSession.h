#pragma once
#include "../engine/GameEngine.h"
#include "../../shared/bus/EventBus.h"
#include "../../shared/model/Piece.h"
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
// loop. Each tick, publishes the resulting GameSnapshot (plus any active
// DisconnectStatus) on the bus instead of calling any broadcaster directly -
// NetworkBroadcaster subscribes to snapshotTopic(roomName) rather than
// being wired in here. The topic is per-room (Stage G) so that a
// NetworkBroadcaster attached to one room never receives another room's
// snapshots - that scoping is what actually prevents cross-room broadcast
// leakage once multiple rooms are active at once.
class GameSession {
public:
    static std::string snapshotTopic(const std::string& roomName) { return "game-state-changed:" + roomName; }

    GameSession(std::shared_ptr<Board> board, EventBus& bus, std::string roomName, bool simultaneousMode = true)
        : engine_(board, simultaneousMode), bus_(bus), roomName_(std::move(roomName)) {}

    GameEngine& engine() { return engine_; }

    void setDisconnectStatus(DisconnectStatus status) { disconnectStatus_ = status; }

    void tick(int ms);

private:
    GameEngine        engine_;
    EventBus&         bus_;
    std::string       roomName_;
    DisconnectStatus  disconnectStatus_{};
};
