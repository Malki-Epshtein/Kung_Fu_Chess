#include "WsServer.h"
#include "../../shared/protocol/MessageCodec.h"
#include "../../shared/model/Piece.h"
#include "../app/CommandDispatcher.h"
#include "../app/GameSession.h"
#include "../app/NetworkBroadcaster.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <set>

namespace {
    using WsppServer = websocketpp::server<websocketpp::config::asio>;
    using ConnectionSet = std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>;
    using RoleMap = std::map<websocketpp::connection_hdl, Chess::Color, std::owner_less<websocketpp::connection_hdl>>;

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

    constexpr int kDisconnectGraceSeconds = 20;
}

void WsServer::run(uint16_t port, GameSession& session, EventBus& bus, int tickMs) {
    WsppServer server;
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.clear_error_channels(websocketpp::log::elevel::all);
    server.init_asio();

    ConnectionSet connections;
    RoleMap       roles;
    int           connectionCount = 0;

    // Disconnect grace-period countdown (Stage D): started when a seated
    // player (White/Black) disconnects, never for a spectator. Once
    // started it always runs to completion - Stage D's simplified scope
    // has no reconnect/seat-reclaim support, so there is nothing that
    // would ever cancel it.
    auto startDisconnectCountdown = [&server, &session](Chess::Color color) {
        auto timer     = std::make_shared<asio::steady_timer>(server.get_io_service());
        auto remaining = std::make_shared<int>(kDisconnectGraceSeconds);
        auto handler   = std::make_shared<std::function<void(const asio::error_code&)>>();

        *handler = [&session, timer, remaining, color, handler](const asio::error_code& ec) {
            if (ec)
                return;
            session.setDisconnectStatus({ true, color, *remaining });
            if (*remaining <= 0) {
                std::cout << "[server] " << roleName(color) << " auto-resigned (disconnected too long)" << std::endl;
                return;
            }
            std::cout << "[server] " << roleName(color) << " disconnected - auto-resign in " << *remaining << "s" << std::endl;
            --(*remaining);
            timer->expires_after(std::chrono::seconds(1));
            timer->async_wait(*handler);
        };
        timer->expires_after(std::chrono::seconds(0));
        timer->async_wait(*handler);
    };

    server.set_open_handler([&connections, &roles, &connectionCount](websocketpp::connection_hdl hdl) {
        connections.insert(hdl);

        // Join order decides the role: first connection is White, second is
        // Black, everyone after that is a spectator (Chess::Color::None).
        ++connectionCount;
        Chess::Color role = connectionCount == 1 ? Chess::Color::White
                           : connectionCount == 2 ? Chess::Color::Black
                           : Chess::Color::None;
        roles[hdl] = role;

        std::cout << "[server] client connected, assigned role: " << roleName(role) << std::endl;
    });
    server.set_close_handler([&connections, &roles, &startDisconnectCountdown](websocketpp::connection_hdl hdl) {
        connections.erase(hdl);
        Chess::Color role = roles.count(hdl) ? roles.at(hdl) : Chess::Color::None;
        roles.erase(hdl);
        std::cout << "[server] client disconnected (was " << roleName(role) << ")" << std::endl;

        if (role == Chess::Color::White || role == Chess::Color::Black)
            startDisconnectCountdown(role);
    });
    server.set_message_handler([&server, &session, &roles](websocketpp::connection_hdl hdl, WsppServer::message_ptr msg) {
        const std::string& text = msg->get_payload();
        Chess::Color senderRole = roles.count(hdl) ? roles.at(hdl) : Chess::Color::None;

        nlohmann::json reply;
        try {
            Message decoded = MessageCodec::decode(text);
            std::cout << "[server] received " << typeName(decoded.type) << " from " << roleName(senderRole)
                       << ": " << text << std::endl;

            DispatchResult result = CommandDispatcher::dispatch(decoded, session.engine(), senderRole);
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
    // currently-connected client.
    NetworkBroadcaster broadcaster(bus, [&server, &connections](const std::string& text) {
        for (const auto& hdl : connections) {
            websocketpp::lib::error_code ec;
            server.send(hdl, text, websocketpp::frame::opcode::text, ec);
            if (ec)
                std::cout << "[server] broadcast send failed: " << ec.message() << std::endl;
        }
    });

    // Drives GameSession::tick on a periodic asio timer, on the same thread
    // as the message handlers above - no locking needed, single writer.
    asio::steady_timer timer(server.get_io_service());
    std::function<void(const asio::error_code&)> onTick = [&](const asio::error_code& ec) {
        if (ec)
            return;
        session.tick(tickMs);
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
