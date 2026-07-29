#pragma once
#include "../../../shared/bus/INatsClient.h"
#include <websocketpp/common/connection_hdl.hpp>
#include <asio/io_context.hpp>

class SessionRegistry;
class ClientSessionRegistry;
class MatchTicketRegistry;

// The full lifecycle of one WebSocket connection, outside of the message
// protocol itself: what happens the moment it opens, and everything that
// needs cleaning up the moment it closes (its room seat, its login
// identity, its pending matchmaking ticket, and - if it was a seated
// player - starting its room's disconnect grace-period countdown).
class ConnectionHandler {
public:
    using ConnectionHandle = websocketpp::connection_hdl;

    // `nats` may be null (native Windows build, or no NATS_URL) - a
    // disconnecting connection with a pending ticket just gets forgotten
    // locally in that case, same degrade-without-crashing pattern used
    // everywhere else NATS/Redis are optional.
    ConnectionHandler(SessionRegistry& registry, ClientSessionRegistry& clientSessions,
                       MatchTicketRegistry& tickets, INatsClient* nats, asio::io_context& ioContext);

    void onOpen(ConnectionHandle hdl);
    void onClose(ConnectionHandle hdl);

private:
    SessionRegistry&        registry_;
    ClientSessionRegistry&  clientSessions_;
    MatchTicketRegistry&     tickets_;
    INatsClient*              nats_;
    asio::io_context&         ioContext_;
};
