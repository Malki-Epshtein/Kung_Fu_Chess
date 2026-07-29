#include "ConnectionHandler.h"
#include "SessionRegistry.h"
#include "ClientSessionRegistry.h"
#include "RoleName.h"
#include "../logic/MatchTicketRegistry.h"
#include "../../../shared/model/Piece.h"
#include "../../../shared/log/Log.h"

ConnectionHandler::ConnectionHandler(SessionRegistry& registry, ClientSessionRegistry& clientSessions,
                                      MatchTicketRegistry& tickets, INatsClient* nats, asio::io_context& ioContext)
    : registry_(registry), clientSessions_(clientSessions), tickets_(tickets), nats_(nats), ioContext_(ioContext) {}

void ConnectionHandler::onOpen(ConnectionHandle /*hdl*/) {
    // No room assignment happens here (Stage G): a connection stays
    // roomless until it explicitly sends JoinRoom (or EnterRoom, for the
    // HTTP-login flow).
    spdlog::info("client connected");
}

void ConnectionHandler::onClose(ConnectionHandle hdl) {
    // Capture the room name before leave() erases the association.
    std::string roomName;
    if (const std::string* name = registry_.roomOf(hdl))
        roomName = *name;

    Chess::Color role = registry_.leave(hdl);
    spdlog::info("client disconnected (was {})", roleName(role));

    // Captured before onDisconnect() erases it below - needed later (see
    // GameSession::markDisconnectResign) to apply an ELO update once this
    // seated player's auto-resign grace period expires. By then this
    // connection's own ClientSession is long gone.
    std::string username;
    int elo = 0;
    if (const ClientSession* session = clientSessions_.sessionFor(hdl)) {
        username = session->username;
        elo = session->elo;
    }

    clientSessions_.onDisconnect(hdl);

    // No-op if hdl wasn't waiting on a ticket. Publishing the cancel lets
    // the Matchmaker service drop it from its pool immediately instead of
    // matching a now-dead connection and waiting a full round trip to
    // discover MatchTicketRegistry::resolve() finds nothing on this side.
    if (std::optional<std::string> ticketId = tickets_.cancel(hdl); ticketId && nats_)
        nats_->publish("matchmaking.cancel", { {"ticketId", *ticketId} });

    // Disconnect grace-period countdown (Stage D, owned by the registry
    // since Stage E3): started when a seated player (White/Black)
    // disconnects, never for a spectator.
    if (!roomName.empty() && (role == Chess::Color::White || role == Chess::Color::Black))
        registry_.startDisconnectCountdown(roomName, role, username, elo, ioContext_);
}
