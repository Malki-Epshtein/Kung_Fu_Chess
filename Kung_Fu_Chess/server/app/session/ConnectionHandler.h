#pragma once
#include <websocketpp/common/connection_hdl.hpp>
#include <asio/io_context.hpp>

class SessionRegistry;
class ClientSessionRegistry;
class Matchmaker;

// The full lifecycle of one WebSocket connection, outside of the message
// protocol itself: what happens the moment it opens, and everything that
// needs cleaning up the moment it closes (its room seat, its login
// identity, its matchmaking pool entry, and - if it was a seated player -
// starting its room's disconnect grace-period countdown).
class ConnectionHandler {
public:
    using ConnectionHandle = websocketpp::connection_hdl;

    ConnectionHandler(SessionRegistry& registry, ClientSessionRegistry& clientSessions,
                       Matchmaker& matchmaker, asio::io_context& ioContext);

    void onOpen(ConnectionHandle hdl);
    void onClose(ConnectionHandle hdl);

private:
    SessionRegistry&        registry_;
    ClientSessionRegistry&  clientSessions_;
    Matchmaker&               matchmaker_;
    asio::io_context&         ioContext_;
};
