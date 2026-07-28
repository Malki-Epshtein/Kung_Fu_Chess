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
#include "../db/IUserRepository.h"
#include "../../shared/db/IClientSessionStore.h"
#include "../concurrency/ThreadPool.h"
#include "../../shared/log/Log.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <string>

namespace {
    using WsppServer = websocketpp::server<websocketpp::config::asio>;
}

void WsServer::run(uint16_t port, SessionRegistry& registry, EventBus& bus, IUserRepository& users,
                    IClientSessionStore* sessionStore, int tickMs) {
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
            spdlog::error("send failed: {}", ec.message());
    };

    BroadcasterManager broadcasters(bus, registry, sendToConnection);
    ConnectionHandler  connectionHandler(registry, clientSessions, matchmaker, server.get_io_service());
    MessageDispatcher  dispatcher(users, clientSessions, registry, bus, matchmaker,
                                   broadcasters, eloService, sendToConnection, server.get_io_service(), sessionStore);

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

    // Fixed-size pool (one shared job queue, no room pinned to a
    // particular worker) that actually runs each room's computeStep() -
    // defaults to hardware_concurrency() workers.
    ThreadPool tickPool;

    // Drives every room's GameSession tick on a periodic asio timer. The
    // timer callback itself still only ever runs on the main io_service
    // thread (same as the message handlers above), but the per-room work it
    // hands out is split across tickPool: computeStep() (the actual engine
    // simulation) runs on a worker thread, and publishStep() (the part that
    // touches EventBus/sends over the socket - see GameSession.h) is posted
    // back onto the io_service so it still only ever runs on the main
    // thread, same as before. tryBeginTick()/endTick() (GameSession.h) stop
    // a room whose previous tick hasn't finished publishing yet from being
    // submitted a second time.
    asio::steady_timer timer(server.get_io_service());
    std::function<void(const asio::error_code&)> onTick = [&](const asio::error_code& ec) {
        if (ec)
            return;
        for (const std::string& roomName : registry.roomNames()) {
            if (GameSession* gameSession = registry.room(roomName)) {
                gameSession->setIdentity(RoomIdentityResolver::resolve(registry, clientSessions, roomName));
                if (!gameSession->tryBeginTick())
                    continue; // previous tick for this room still in flight - skip this cycle
                auto& io = server.get_io_service();
                tickPool.submit([gameSession, tickMs, &io, &registry, roomName]() {
                    nlohmann::json payload = gameSession->computeStep(tickMs);
                    io.post([gameSession, payload, &registry, roomName]() {
                        gameSession->publishStep(payload);
                        // Game-over detection/ELO application both stay off
                        // this loop entirely - GameSession publishes
                        // gameEndedTopic itself (from publishStep(), or from
                        // markDisconnectResign() via the disconnect timer),
                        // and EloService (attached per-room in
                        // MessageDispatcher) reacts independently.
                        gameSession->endTick();
                        // If the room emptied out while this tick was in
                        // flight, leave() deferred the actual erase (see
                        // SessionRegistry::leave/reapIfSafe) rather than
                        // destroying gameSession out from under this same
                        // tick - now that endTick() just ran, it's safe to
                        // finish that erase.
                        registry.reapIfSafe(roomName);
                    });
                });
            }
        }
        timer.expires_after(std::chrono::milliseconds(tickMs));
        timer.async_wait(onTick);
    };
    timer.expires_after(std::chrono::milliseconds(tickMs));
    timer.async_wait(onTick);

    server.listen(port);
    server.start_accept();
    spdlog::info("listening on port {}", port);
    server.run();
}
