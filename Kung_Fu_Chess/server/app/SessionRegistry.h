#pragma once
#include "GameSession.h"
#include "../../shared/bus/EventBus.h"
#include "../../shared/model/Piece.h"
#include <websocketpp/common/connection_hdl.hpp>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Owns every active room (GameSession) by name, plus which connection
// belongs to which room and what role (White/Black/Spectator) it holds
// there. Pure bookkeeping - no networking, no asio - so it is fully
// unit-testable without a socket. WsServer (Stage E2) is the only thing
// that wires this to real connections.
class SessionRegistry {
public:
    using ConnectionHandle = websocketpp::connection_hdl;

    // Creates a new room under `name`. Returns false (does nothing) if that
    // name is already taken.
    bool createRoom(const std::string& name, std::shared_ptr<Board> board, EventBus& bus, bool simultaneousMode = true);

    bool roomExists(const std::string& name) const;

    // nullptr if no such room.
    GameSession* room(const std::string& name);

    // Every room this registry currently owns - for the tick loop to iterate.
    std::vector<std::string> roomNames() const;

    // Adds `hdl` to `name`'s room and assigns its role by join order within
    // that room (first = White, second = Black, everyone after = Spectator).
    // Returns false (does nothing) if the room doesn't exist.
    bool joinRoom(const std::string& name, ConnectionHandle hdl);

    // Removes a connection from whichever room it was in (no-op, returns
    // None, if it wasn't in one). Returns the role it held there, so the
    // caller (Stage E3 disconnect handling) knows whether to start a
    // countdown.
    Chess::Color leave(ConnectionHandle hdl);

    // Which room (if any) a connection belongs to; nullptr if none.
    const std::string* roomOf(ConnectionHandle hdl) const;

    // The role a connection holds in its room (None if not found).
    Chess::Color roleOf(ConnectionHandle hdl) const;

private:
    struct Room {
        std::unique_ptr<GameSession> session;
        std::map<ConnectionHandle, Chess::Color, std::owner_less<ConnectionHandle>> roles;
        int connectionCount = 0;
    };

    std::unordered_map<std::string, Room> rooms_;
    std::map<ConnectionHandle, std::string, std::owner_less<ConnectionHandle>> connectionRoom_;
};
