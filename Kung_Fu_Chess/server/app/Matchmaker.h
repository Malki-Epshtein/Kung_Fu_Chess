#pragma once
#include <websocketpp/common/connection_hdl.hpp>
#include <map>
#include <optional>

// A small waiting pool for ELO-based matchmaking (Stage H): a connection
// looking for a game via FindGame is added here if nobody currently
// waiting qualifies; the next FindGame checks everyone already waiting
// for anyone within kEloRange. Pure data structure, no socket - WsServer
// decides what to actually DO with a match (create/join a room, send
// GameFound). Mirrors SessionRegistry/ClientSessionRegistry's own
// std::map + owner_less pattern (connection_hdl has no std::hash).
class Matchmaker {
public:
    using ConnectionHandle = websocketpp::connection_hdl;
    static constexpr int kEloRange = 100;

    void addToPool(ConnectionHandle hdl, int elo);

    // No-op if hdl isn't in the pool - used both when a match is found
    // and when a waiting connection disconnects/times out.
    void remove(ConnectionHandle hdl);

    // Any waiting connection within kEloRange of `elo` - nullopt if the
    // pool is empty or nobody currently waiting qualifies.
    std::optional<ConnectionHandle> findMatch(int elo) const;

    // True while hdl is still sitting in the pool (added, not yet matched
    // or removed) - used by the search-timeout in WsServer.cpp to tell
    // "still waiting, really timed out" apart from "got matched in the
    // meantime, the timer firing is now moot".
    bool isWaiting(ConnectionHandle hdl) const;

private:
    std::map<ConnectionHandle, int, std::owner_less<ConnectionHandle>> pool_;
};
