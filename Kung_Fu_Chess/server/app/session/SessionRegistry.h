#pragma once
#include "GameSession.h"
#include "../../../shared/bus/EventBus.h"
#include "../../../shared/model/Piece.h"
#include <websocketpp/common/connection_hdl.hpp>
#include <asio/io_context.hpp>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Owns every active room (GameSession) by name, plus which connection
// belongs to which room and what role (White/Black/Spectator) it holds
// there - and, since Stage E3, each room's disconnect grace-period
// countdown too. The room-bookkeeping methods below are pure data
// structure and fully unit-testable without a socket;
// startDisconnectCountdown is the one method that touches asio (it needs
// somewhere to run a timer) - WsServer (Stage E2) is what wires the rest
// of this to real connections.
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
    // caller (disconnect handling) knows whether to start a countdown.
    Chess::Color leave(ConnectionHandle hdl);

    // Every connection currently in room `name` (players and spectators
    // alike) - empty if the room doesn't exist. Used to scope broadcasts to
    // just that room's occupants (Stage G).
    std::vector<ConnectionHandle> connectionsInRoom(const std::string& name) const;

    // Which room (if any) a connection belongs to; nullptr if none.
    const std::string* roomOf(ConnectionHandle hdl) const;

    // The role a connection holds in its room (None if not found).
    Chess::Color roleOf(ConnectionHandle hdl) const;

    // Starts a 20s auto-resign countdown for `color` in room `roomName`,
    // running on `ioContext`. No-op if the room doesn't exist. Never
    // cancelled once started (Stage D's simplified scope has no
    // reconnect/seat-reclaim support) - always runs to completion.
    // disconnectedUsername/disconnectedElo must be captured by the caller
    // (ConnectionHandler) before this - they're handed straight through to
    // GameSession::markDisconnectResign once the countdown expires, since
    // this connection's own ClientSession is already gone by then.
    void startDisconnectCountdown(const std::string& roomName, Chess::Color color,
                                   std::string disconnectedUsername, int disconnectedElo,
                                   asio::io_context& ioContext);

private:
    struct Room {
        std::unique_ptr<GameSession> session;
        std::map<ConnectionHandle, Chess::Color, std::owner_less<ConnectionHandle>> roles;//את כל המשתמשים פלוס הלקוחות
        int connectionCount = 0;
    };

    std::unordered_map<std::string, Room> rooms_;
    std::map<ConnectionHandle, std::string, std::owner_less<ConnectionHandle>> connectionRoom_;//לבדוק באיזה חדר נמצא המתשמש הספציפי שלי עשכיו
};
