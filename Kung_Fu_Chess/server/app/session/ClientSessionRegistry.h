#pragma once
#include "ClientSession.h"
#include <websocketpp/common/connection_hdl.hpp>
#include <map>
#include <string>

// Tracks per-connection identity (username, ELO) from the moment of a
// successful LOGIN, keyed by connection - mirrors SessionRegistry's own
// map-of-connection_hdl pattern (std::map + owner_less, since
// connection_hdl has no std::hash). WsServer calls onLogin/onDisconnect;
// nothing else mutates this. Pure data structure, no socket - fully
// unit-testable with a fake connection_hdl, exactly like SessionRegistry.
class ClientSessionRegistry {
public:
    using ConnectionHandle = websocketpp::connection_hdl;

    void onLogin(ConnectionHandle hdl, std::string username, int elo);
    void onDisconnect(ConnectionHandle hdl);

    // nullptr if this connection never logged in (or has since disconnected).
    const ClientSession* sessionFor(ConnectionHandle hdl) const;

    // Logged-in connections currently tracked - exposed as the
    // active_connections gauge on GET /metrics (see WsServer.cpp).
    size_t size() const;

private:
    std::map<ConnectionHandle, ClientSession, std::owner_less<ConnectionHandle>> sessions_;
};
