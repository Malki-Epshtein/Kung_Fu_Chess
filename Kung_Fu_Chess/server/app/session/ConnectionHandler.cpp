#include "ConnectionHandler.h"
#include "SessionRegistry.h"
#include "ClientSessionRegistry.h"
#include "RoleName.h"
#include "../logic/Matchmaker.h"
#include "../../../shared/model/Piece.h"
#include <iostream>

ConnectionHandler::ConnectionHandler(SessionRegistry& registry, ClientSessionRegistry& clientSessions,
                                      Matchmaker& matchmaker, asio::io_context& ioContext)
    : registry_(registry), clientSessions_(clientSessions), matchmaker_(matchmaker), ioContext_(ioContext) {}

void ConnectionHandler::onOpen(ConnectionHandle /*hdl*/) {
    // No room assignment happens here (Stage G): a connection stays
    // roomless until it explicitly sends CreateRoom or JoinRoom.
    std::cout << "[server] client connected" << std::endl;
}

void ConnectionHandler::onClose(ConnectionHandle hdl) {
    // Capture the room name before leave() erases the association.
    std::string roomName;
    if (const std::string* name = registry_.roomOf(hdl))
        roomName = *name;

    Chess::Color role = registry_.leave(hdl);
    std::cout << "[server] client disconnected (was " << roleName(role) << ")" << std::endl;

    clientSessions_.onDisconnect(hdl);
    matchmaker_.remove(hdl); // no-op if it wasn't waiting for a match

    // Disconnect grace-period countdown (Stage D, owned by the registry
    // since Stage E3): started when a seated player (White/Black)
    // disconnects, never for a spectator.
    if (!roomName.empty() && (role == Chess::Color::White || role == Chess::Color::Black))
        registry_.startDisconnectCountdown(roomName, role, ioContext_);
}
