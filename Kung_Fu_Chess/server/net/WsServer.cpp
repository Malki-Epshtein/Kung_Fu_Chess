#include "WsServer.h"
#include "../../shared/protocol/MessageCodec.h"
#include "../../shared/model/Piece.h"
#include "../app/CommandDispatcher.h"
#include "../app/GameSession.h"
#include "../app/SessionRegistry.h"
#include "../app/NetworkBroadcaster.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <set>
#include <string>

namespace {
    using WsppServer = websocketpp::server<websocketpp::config::asio>;
    using ConnectionSet = std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>;

    const char* typeName(MessageType type) {
        switch (type) {
            case MessageType::Hello:    return "HELLO";
            case MessageType::Move:     return "MOVE";
            case MessageType::Jump:     return "JUMP";
            case MessageType::Snapshot: return "SNAPSHOT";
        }
        return "UNKNOWN";
    }

    const char* roleName(Chess::Color color) {
        switch (color) {
            case Chess::Color::White: return "White";
            case Chess::Color::Black: return "Black";
            default:                  return "Spectator";
        }
    }
}

void WsServer::run(uint16_t port, SessionRegistry& registry, EventBus& bus, int tickMs) {
    WsppServer server;
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.clear_error_channels(websocketpp::log::elevel::all);
    server.init_asio();

    ConnectionSet connections;

    server.set_open_handler([&connections, &registry](websocketpp::connection_hdl hdl) {
        connections.insert(hdl);
        registry.joinRoom(WsServer::kDefaultRoomName, hdl);
        std::cout << "[server] client connected, assigned role: " << roleName(registry.roleOf(hdl)) << std::endl;
    });
    server.set_close_handler([&connections, &registry, &server](websocketpp::connection_hdl hdl) {
        connections.erase(hdl);

        // Capture the room name before leave() erases the association.
        std::string roomName;
        if (const std::string* name = registry.roomOf(hdl))
            roomName = *name;

        Chess::Color role = registry.leave(hdl);
        std::cout << "[server] client disconnected (was " << roleName(role) << ")" << std::endl;

        // Disconnect grace-period countdown (Stage D, owned by the registry
        // since Stage E3): started when a seated player (White/Black)
        // disconnects, never for a spectator.
        if (!roomName.empty() && (role == Chess::Color::White || role == Chess::Color::Black))
            registry.startDisconnectCountdown(roomName, role, server.get_io_service());
    });
    server.set_message_handler([&server, &registry](websocketpp::connection_hdl hdl, WsppServer::message_ptr msg) {
        const std::string& text = msg->get_payload();
        Chess::Color senderRole = registry.roleOf(hdl);

        nlohmann::json reply;
        try {
            const std::string* roomName = registry.roomOf(hdl);
            GameSession* gameSession = roomName ? registry.room(*roomName) : nullptr;
            if (!gameSession)
                throw std::runtime_error("connection is not in a room");

            Message decoded = MessageCodec::decode(text);
            std::cout << "[server] received " << typeName(decoded.type) << " from " << roleName(senderRole)
                       << ": " << text << std::endl;

            DispatchResult result = CommandDispatcher::dispatch(decoded, gameSession->engine(), senderRole);
            std::cout << "[server] dispatch " << (result.success ? "OK" : "FAILED")
                       << ": " << result.message << std::endl;

            reply = { {"success", result.success}, {"message", result.message}, {"role", roleName(senderRole)} };
        } catch (const std::exception& e) {
            std::cout << "[server] received non-protocol text: " << text
                       << " (decode failed: " << e.what() << ")" << std::endl;
            reply = { {"success", false}, {"message", std::string("decode failed: ") + e.what()} };
        }

        server.send(hdl, reply.dump(), msg->get_opcode());
        std::cout << "[server] replied: " << reply.dump() << std::endl;
    });

    // Subscribes itself to the bus for as long as this function runs; on
    // each published snapshot, sends the serialized text to every
    // currently-connected client. One room today (Stage E2), so "everyone"
    // is exactly that room's occupants - real per-room filtering arrives
    // with real multi-room support in Stage G.
    NetworkBroadcaster broadcaster(bus, [&server, &connections](const std::string& text) {
        for (const auto& hdl : connections) {
            websocketpp::lib::error_code ec;
            server.send(hdl, text, websocketpp::frame::opcode::text, ec);
            if (ec)
                std::cout << "[server] broadcast send failed: " << ec.message() << std::endl;
        }
    });

    // Drives every room's GameSession::tick on a periodic asio timer, on
    // the same thread as the message handlers above - no locking needed,
    // single writer. One room today; iterating the registry means this
    // already scales to many without changing again later.
    asio::steady_timer timer(server.get_io_service());
    std::function<void(const asio::error_code&)> onTick = [&](const asio::error_code& ec) {
        if (ec)
            return;
        for (const std::string& roomName : registry.roomNames()) {
            if (GameSession* gameSession = registry.room(roomName))
                gameSession->tick(tickMs);
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
