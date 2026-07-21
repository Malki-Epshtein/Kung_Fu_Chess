#include "WsServer.h"
#include "../app/session/SessionRegistry.h"
#include "../app/session/GameSession.h"
#include "../app/session/ClientSessionRegistry.h"
#include "../app/session/RoomIdentityResolver.h"
#include "../app/session/ConnectionHandler.h"
#include "../app/logic/Matchmaker.h"
#include "../app/logic/EloService.h"
#include "../app/networking/BroadcasterManager.h"
#include "../app/handlers/MessageDispatcher.h"
#include "../db/UserRepository.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>

namespace {
    using WsppServer = websocketpp::server<websocketpp::config::asio>;
}

void WsServer::run(uint16_t port, SessionRegistry& registry, EventBus& bus, UserRepository& users, int tickMs) {
    WsppServer server;
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.clear_error_channels(websocketpp::log::elevel::all);
    server.init_asio();

    ClientSessionRegistry clientSessions;
    Matchmaker            matchmaker;
    EloService            eloService(users);

    // The one place any of this subsystem's objects actually touch a
    // socket - wraps server.send for "push this text to some connection
    // outside the normal request/reply flow" (room broadcasts, and
    // FindGameHandler's GameFound push/timeout). Shared rather than
    // duplicated between BroadcasterManager and MessageDispatcher.
    auto sendToConnection = [&server](websocketpp::connection_hdl hdl, const std::string& text) {
        websocketpp::lib::error_code ec;
        server.send(hdl, text, websocketpp::frame::opcode::text, ec);
        if (ec)
            std::cout << "[server] send failed: " << ec.message() << std::endl;
    };

    BroadcasterManager broadcasters(bus, registry, sendToConnection);
    ConnectionHandler  connectionHandler(registry, clientSessions, matchmaker, server.get_io_service());
    MessageDispatcher  dispatcher(users, clientSessions, registry, bus, matchmaker,
                                   broadcasters, eloService, sendToConnection, server.get_io_service());

    server.set_open_handler([&connectionHandler](websocketpp::connection_hdl hdl) {
        connectionHandler.onOpen(hdl);
    });
    server.set_close_handler([&connectionHandler](websocketpp::connection_hdl hdl) {
        connectionHandler.onClose(hdl);
    });
    server.set_message_handler([&server, &dispatcher](websocketpp::connection_hdl hdl, WsppServer::message_ptr msg) {//פה אני מקבלת את ההודעה
        std::string reply = dispatcher.process(hdl, msg->get_payload());
        server.send(hdl, reply, msg->get_opcode());//שולח בחזרה ללקוח
    });

    // Drives every room's GameSession::tick on a periodic asio timer, on
    // the same thread as the message handlers above - no locking needed,
    // single writer. Iterating the registry means this scales to however
    // many rooms currently exist without changing again later.
    asio::steady_timer timer(server.get_io_service());
    std::function<void(const asio::error_code&)> onTick = [&](const asio::error_code& ec) {
        if (ec)
            return;
        for (const std::string& roomName : registry.roomNames()) {
            if (GameSession* gameSession = registry.room(roomName)) {
                gameSession->setIdentity(RoomIdentityResolver::resolve(registry, clientSessions, roomName));
                gameSession->tick(tickMs);//לכח אחד מפעילה את השעון
                // Game-over detection/ELO application both moved off this
                // loop entirely - GameSession publishes gameEndedTopic
                // itself (from within tick(), or from
                // markDisconnectResign() via the disconnect timer), and
                // EloService (attached per-room in MessageDispatcher)
                // reacts independently. This loop no longer needs to know
                // ELO exists at all.
            }
        }
        timer.expires_after(std::chrono::milliseconds(tickMs));
        timer.async_wait(onTick);
    };
    timer.expires_after(std::chrono::milliseconds(tickMs));
    timer.async_wait(onTick);

    server.listen(port);
    server.start_accept();
    std::cout << "[server] listening on port " << port << std::endl;
    server.run();
}
