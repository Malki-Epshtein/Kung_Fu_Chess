#include "WsServer.h"
#include "../../shared/protocol/MessageCodec.h"
#include "../../shared/model/Piece.h"
#include "../app/logic/CommandDispatcher.h"
#include "../app/session/GameSession.h"
#include "../app/session/SessionRegistry.h"
#include "../app/session/ClientSessionRegistry.h"
#include "../app/logic/Matchmaker.h"
#include "../app/networking/NetworkBroadcaster.h"
#include "../app/logic/StartingBoard.h"
#include "../db/UserRepository.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace {
    using WsppServer = websocketpp::server<websocketpp::config::asio>;

    const char* typeName(MessageType type) {
        switch (type) {
            case MessageType::Hello:      return "HELLO";
            case MessageType::Move:       return "MOVE";
            case MessageType::Jump:       return "JUMP";
            case MessageType::Snapshot:   return "SNAPSHOT";
            case MessageType::Login:      return "LOGIN";
            case MessageType::CreateRoom: return "CREATE_ROOM";
            case MessageType::JoinRoom:   return "JOIN_ROOM";
            case MessageType::FindGame:   return "FIND_GAME";
            case MessageType::GameFound:  return "GAME_FOUND";
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

void WsServer::run(uint16_t port, SessionRegistry& registry, EventBus& bus, UserRepository& users, int tickMs) {
    WsppServer server;
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.clear_error_channels(websocketpp::log::elevel::all);
    server.init_asio();

    // One NetworkBroadcaster per room, each subscribed only to that room's
    // own topic (GameSession::snapshotTopic(roomName)) and sending only to
    // that room's own connections - this is what keeps two rooms' snapshots
    // from leaking into each other once more than one room exists (Stage G).
    std::unordered_map<std::string, std::unique_ptr<NetworkBroadcaster>> broadcasters;
    auto attachBroadcaster = [&server, &registry, &bus, &broadcasters](const std::string& roomName) {
        broadcasters[roomName] = std::make_unique<NetworkBroadcaster>(
            bus, GameSession::snapshotTopic(roomName),
            [&server, &registry, roomName](const std::string& text) {
                for (const auto& hdl : registry.connectionsInRoom(roomName)) {
                    websocketpp::lib::error_code ec;
                    server.send(hdl, text, websocketpp::frame::opcode::text, ec);
                    if (ec)
                        std::cout << "[server] broadcast send failed: " << ec.message() << std::endl;
                }
            });
    };
    // Any room that already existed when run() was called (e.g. the
    // startup default room created by server_main.cpp) needs its
    // broadcaster attached up front - rooms created later via CreateRoom
    // get theirs attached at creation time, below.
    for (const std::string& roomName : registry.roomNames())
        attachBroadcaster(roomName);

    // Per-connection identity (username, ELO), remembered from that
    // connection's own successful LOGIN - matchmaking (FindGame, below)
    // reads this rather than trusting a client-reported ELO, so a client
    // can't just claim whatever ELO it wants to game the ±100 match window.
    ClientSessionRegistry clientSessions;

    // Stage H: waiting pool for FindGame, and the counter behind
    // matchmaking-created rooms' ids (prefixed so they can never collide
    // with a user-typed Create/Join room name).
    Matchmaker matchmaker;
    int        nextMatchId = 1;

    server.set_open_handler([](websocketpp::connection_hdl) {
        // No room assignment happens here anymore (Stage G): a connection
        // stays roomless until it explicitly sends CreateRoom or JoinRoom.
        std::cout << "[server] client connected" << std::endl;
    });
    server.set_close_handler([&registry, &server, &clientSessions, &matchmaker](websocketpp::connection_hdl hdl) {
        // Capture the room name before leave() erases the association.
        std::string roomName;
        if (const std::string* name = registry.roomOf(hdl))
            roomName = *name;

        Chess::Color role = registry.leave(hdl);
        std::cout << "[server] client disconnected (was " << roleName(role) << ")" << std::endl;

        clientSessions.onDisconnect(hdl);
        matchmaker.remove(hdl); // no-op if it wasn't waiting for a match

        // Disconnect grace-period countdown (Stage D, owned by the registry
        // since Stage E3): started when a seated player (White/Black)
        // disconnects, never for a spectator.
        if (!roomName.empty() && (role == Chess::Color::White || role == Chess::Color::Black))
            registry.startDisconnectCountdown(roomName, role, server.get_io_service());
    });
    server.set_message_handler([&server, &registry, &users, &bus, &attachBroadcaster, &clientSessions, &matchmaker, &nextMatchId](websocketpp::connection_hdl hdl, WsppServer::message_ptr msg) {
        const std::string& text = msg->get_payload();
        Chess::Color senderRole = registry.roleOf(hdl);

        nlohmann::json reply;
        try {
            Message decoded = MessageCodec::decode(text);
            std::cout << "[server] received " << typeName(decoded.type) << " from " << roleName(senderRole)
                       << ": " << text << std::endl;

            if (decoded.type == MessageType::Login) {
                // Identity operation - never touches a room/GameEngine.
                std::string username = decoded.payload.at("username").get<std::string>();
                std::string password = decoded.payload.at("password").get<std::string>();
                LoginResult result = users.login(username, password);
                std::cout << "[server] login " << (result.success ? "OK" : "FAILED")
                           << " for '" << username << "': " << result.message << std::endl;
                if (result.success)
                    clientSessions.onLogin(hdl, username, result.elo);
                reply = { {"success", result.success}, {"message", result.message}, {"elo", result.elo} };
            } else if (decoded.type == MessageType::CreateRoom) {
                std::string name = decoded.payload.at("name").get<std::string>();
                if (registry.roomExists(name)) {
                    reply = { {"success", false}, {"message", "room already exists"} };
                } else {
                    registry.createRoom(name, makeStartingBoard(), bus, /*simultaneousMode=*/true);
                    attachBroadcaster(name);
                    // The creator becomes the room's first occupant (White) -
                    // otherwise they'd have to immediately send a separate
                    // JoinRoom right after creating it.
                    registry.joinRoom(name, hdl);
                    Chess::Color role = registry.roleOf(hdl);
                    std::cout << "[server] room '" << name << "' created, creator assigned role: "
                               << roleName(role) << std::endl;
                    reply = { {"success", true}, {"message", "room created"}, {"role", roleName(role)}, {"roomName", name} };
                }
            } else if (decoded.type == MessageType::JoinRoom) {
                std::string name = decoded.payload.at("name").get<std::string>();
                if (!registry.roomExists(name)) {
                    reply = { {"success", false}, {"message", "room not found"} };
                } else {
                    registry.joinRoom(name, hdl);
                    Chess::Color role = registry.roleOf(hdl);
                    std::cout << "[server] joined room '" << name << "', assigned role: "
                               << roleName(role) << std::endl;
                    reply = { {"success", true}, {"message", "joined room"}, {"role", roleName(role)}, {"roomName", name} };
                }
            } else if (decoded.type == MessageType::FindGame) {
                const ClientSession* session = clientSessions.sessionFor(hdl);
                if (!session) {
                    reply = { {"success", false}, {"message", "must be logged in to find a game"} };
                } else {
                    auto opponent = matchmaker.findMatch(session->elo);
                    if (opponent) {
                        matchmaker.remove(*opponent);
                        std::string name = "match-" + std::to_string(nextMatchId++);
                        registry.createRoom(name, makeStartingBoard(), bus, /*simultaneousMode=*/true);
                        attachBroadcaster(name);
                        registry.joinRoom(name, *opponent); // was waiting first -> White
                        registry.joinRoom(name, hdl);        // just matched -> Black

                        // The opponent isn't expecting a reply right now - this
                        // is the first server->client PUSH in the protocol
                        // (see Message.h), sent unprompted outside the normal
                        // reply flow below.
                        Message opponentMsg{ MessageType::GameFound,
                            { {"success", true}, {"message", "match found"}, {"roomName", name}, {"role", roleName(registry.roleOf(*opponent))} } };
                        websocketpp::lib::error_code ec;
                        server.send(*opponent, MessageCodec::encode(opponentMsg), websocketpp::frame::opcode::text, ec);
                        if (ec)
                            std::cout << "[server] GameFound push failed: " << ec.message() << std::endl;

                        // The sender DOES get the normal reply flow below,
                        // but shaped as the same GameFound envelope so both
                        // players receive an identical message shape.
                        Message senderMsg{ MessageType::GameFound,
                            { {"success", true}, {"message", "match found"}, {"roomName", name}, {"role", roleName(registry.roleOf(hdl))} } };
                        reply = nlohmann::json::parse(MessageCodec::encode(senderMsg));
                        std::cout << "[server] matched '" << name << "' via FindGame" << std::endl;
                    } else {
                        matchmaker.addToPool(hdl, session->elo);
                        reply = { {"success", true}, {"message", "searching for opponent"} };

                        // 60s search timeout - same asio::steady_timer
                        // pattern as the Stage D disconnect countdown.
                        // isWaiting() tells a real timeout apart from
                        // "already matched by someone else's FindGame in
                        // the meantime, this firing is now moot".
                        auto timer = std::make_shared<asio::steady_timer>(server.get_io_service());
                        timer->expires_after(std::chrono::seconds(60));
                        timer->async_wait([&server, &matchmaker, hdl, timer](const asio::error_code& ec) {
                            if (ec || !matchmaker.isWaiting(hdl))
                                return;
                            matchmaker.remove(hdl);
                            Message timeoutMsg{ MessageType::GameFound, { {"success", false}, {"message", "no players available"} } };
                            websocketpp::lib::error_code sendEc;
                            server.send(hdl, MessageCodec::encode(timeoutMsg), websocketpp::frame::opcode::text, sendEc);
                            std::cout << "[server] FindGame timed out, no match found" << std::endl;
                        });
                    }
                }
            } else {
                const std::string* roomName = registry.roomOf(hdl);
                GameSession* gameSession = roomName ? registry.room(*roomName) : nullptr;
                if (!gameSession)
                    throw std::runtime_error("connection is not in a room");

                DispatchResult result = CommandDispatcher::dispatch(decoded, gameSession->engine(), senderRole);
                std::cout << "[server] dispatch " << (result.success ? "OK" : "FAILED")
                           << ": " << result.message << std::endl;

                reply = { {"success", result.success}, {"message", result.message}, {"role", roleName(senderRole)} };
            }
        } catch (const std::exception& e) {
            std::cout << "[server] received non-protocol text: " << text
                       << " (decode failed: " << e.what() << ")" << std::endl;
            reply = { {"success", false}, {"message", std::string("decode failed: ") + e.what()} };
        }

        server.send(hdl, reply.dump(), msg->get_opcode());
        std::cout << "[server] replied: " << reply.dump() << std::endl;
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
            if (GameSession* gameSession = registry.room(roomName))
                gameSession->tick(tickMs);//לכח אחד מפעילה את השעון
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
