#pragma once
#include "GameSession.h"
#include "../../../shared/db/IRoomDirectory.h"
#include "../../../shared/db/IReconnectStore.h"
#include "../../../shared/bus/EventBus.h"
#include "../../../shared/bus/INatsClient.h"
#include "../../../shared/model/Piece.h"
#include <websocketpp/common/connection_hdl.hpp>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
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

    // `directory`/`shardAddress`/`nats`/`reconnectStore` are optional
    // (default: no-op) - only server_main.cpp's Docker/Linux build ever
    // passes a real RedisRoomDirectory + this shard's address + a real
    // NatsClient + a real RedisReconnectStore; every test and the native
    // Windows build get the same behavior as before these existed.
    explicit SessionRegistry(IRoomDirectory* directory = nullptr, std::string shardAddress = "",
                              INatsClient* nats = nullptr, IReconnectStore* reconnectStore = nullptr);

    // Creates a new room under `name`. Returns false (does nothing) if that
    // name is already taken.
    bool createRoom(const std::string& name, std::shared_ptr<Board> board, EventBus& bus, bool simultaneousMode = true);

    bool roomExists(const std::string& name) const;

    // nullptr if no such room.
    GameSession* room(const std::string& name);

    // Every room this registry currently owns - for the tick loop to iterate.
    std::vector<std::string> roomNames() const;

    // Adds `hdl` to `name`'s room. If `username` matches a seat reserved by
    // reserveSeats() for this room, it gets that reserved color regardless
    // of arrival order; otherwise role is assigned by join order within the
    // room (first = White, second = Black, everyone after = Spectator) -
    // the original behavior, unaffected for any room with no reservations.
    // Returns false (does nothing) if the room doesn't exist.
    bool joinRoom(const std::string& name, ConnectionHandle hdl, const std::string& username = "");

    // Reserves White/Black by username for a room before either player has
    // actually reconnected to claim their seat - set by the shard-side
    // AllocateRoomHandler right after createRoom, for a room the Game
    // Allocator matched two players into (Server_Design.md step 6), so
    // joinRoom can seat them correctly no matter which one reconnects
    // first. No-op if the room doesn't exist.
    void reserveSeats(const std::string& name, const std::string& whiteUsername, const std::string& blackUsername);

    // Removes a connection from whichever room it was in (no-op, returns
    // None, if it wasn't in one). Returns the role it held there, so the
    // caller (disconnect handling) knows whether to start a countdown.
    // If this empties the room and its GameSession has a computeStep() in
    // flight on a worker thread (see GameSession::tickInFlight), the actual
    // erase is deferred - see reapIfSafe - rather than destroying the
    // GameSession out from under that worker.
    Chess::Color leave(ConnectionHandle hdl);

    // Erases room `name` if it was emptied out by leave() while its tick was
    // still in flight (pendingRemoval) and that tick has since finished. A
    // no-op otherwise - safe to call every tick. WsServer calls this right
    // after a room's publishStep()/endTick() complete, since that's the one
    // moment guaranteed to observe tickInFlight() go false.
    void reapIfSafe(const std::string& name);

    // Every connection currently in room `name` (players and spectators
    // alike) - empty if the room doesn't exist. Used to scope broadcasts to
    // just that room's occupants (Stage G).
    std::vector<ConnectionHandle> connectionsInRoom(const std::string& name) const;

    // Which room (if any) a connection belongs to; nullptr if none.
    const std::string* roomOf(ConnectionHandle hdl) const;

    // The role a connection holds in its room (None if not found).
    Chess::Color roleOf(ConnectionHandle hdl) const;

    // Starts a 20s auto-resign countdown for `color` in room `roomName`,
    // running on `ioContext`. No-op if the room doesn't exist. Also holds
    // that seat under disconnectedUsername (same mechanism as
    // reserveSeats()) so a reconnecting client gets its color back rather
    // than being seated as a new arrival - see cancelDisconnectCountdown().
    // disconnectedUsername/disconnectedElo must be captured by the caller
    // (ConnectionHandler) before this - they're handed straight through to
    // GameSession::markDisconnectResign once the countdown expires, since
    // this connection's own ClientSession is already gone by then. If a
    // reconnectStore was given at construction, every tick also mirrors
    // this status to Redis (documentation only - nothing reads it back to
    // actually reconnect a room; see IReconnectStore.h).
    void startDisconnectCountdown(const std::string& roomName, Chess::Color color,
                                   std::string disconnectedUsername, int disconnectedElo,
                                   asio::io_context& ioContext);

    // Cancels a running auto-resign countdown for `color` in `roomName`,
    // provided `username` is the same player it was started for (a
    // different user asking for the same seat is refused, leaving the
    // countdown running). Called by EnterRoomHandler right after a
    // reconnecting client reclaims its seat. Clears the disconnect banner
    // (GameSession::setDisconnectStatus) and the Redis mirror
    // (reconnectStore_->clearDisconnected), same cleanup the countdown's own
    // expiry path does. Returns false (no-op) if there's no such countdown.
    bool cancelDisconnectCountdown(const std::string& roomName, Chess::Color color,
                                    const std::string& username);

private:
    struct Room {
        std::unique_ptr<GameSession> session;
        std::map<ConnectionHandle, Chess::Color, std::owner_less<ConnectionHandle>> roles;//את כל המשתמשים פלוס הלקוחות
        int connectionCount = 0;
        // Set by reserveSeats() - username -> the color joinRoom() should
        // assign that username regardless of arrival order. Empty for
        // every room created outside the matchmaking allocate path.
        std::unordered_map<std::string, Chess::Color> seatReservations;
        // Set by leave() instead of erasing immediately, when the room
        // emptied out while session->tickInFlight() was still true -
        // reapIfSafe() finishes the erase once that tick completes.
        bool pendingRemoval = false;

        // One entry per color currently mid-countdown - lets
        // cancelDisconnectCountdown() find and stop the right timer, and
        // confirm the reconnecting username matches who actually disconnected.
        struct PendingDisconnect {
            std::string username;
            std::shared_ptr<asio::steady_timer> timer;
        };
        std::map<Chess::Color, PendingDisconnect> pendingDisconnects;
    };

    std::unordered_map<std::string, Room> rooms_;
    std::map<ConnectionHandle, std::string, std::owner_less<ConnectionHandle>> connectionRoom_;//לבדוק באיזה חדר נמצא המתשמש הספציפי שלי עשכיו

    IRoomDirectory*   directory_ = nullptr;
    std::string       shardAddress_;
    INatsClient*      nats_ = nullptr;
    IReconnectStore*  reconnectStore_ = nullptr;
};
