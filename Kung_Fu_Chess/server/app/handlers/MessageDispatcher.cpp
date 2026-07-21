#include "MessageDispatcher.h"
#include "IMessageHandler.h"
#include "LoginHandler.h"
#include "CreateRoomHandler.h"
#include "JoinRoomHandler.h"
#include "FindGameHandler.h"
#include "MessageRouter.h"
#include "../session/RoleName.h"
#include "../session/SessionRegistry.h"
#include "../session/GameSession.h"
#include "../networking/BroadcasterManager.h"
#include "../logic/CommandDispatcher.h"
#include "../../../shared/protocol/MessageCodec.h"
#include "../../../shared/model/Piece.h"
#include <iostream>

namespace {
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
            case MessageType::MoveLogged: return "MOVE_LOGGED";
        }
        return "UNKNOWN";
    }
}

MessageDispatcher::MessageDispatcher(UserRepository& users, ClientSessionRegistry& clientSessions,
                                      SessionRegistry& registry, EventBus& bus, Matchmaker& matchmaker,
                                      BroadcasterManager& broadcasters, ConnectionSender push,
                                      asio::io_context& ioContext)
    : registry_(registry) {
    BroadcasterManager* broadcastersPtr = &broadcasters;
    auto attachBroadcaster = [broadcastersPtr](const std::string& name) { broadcastersPtr->attach(name); };

    loginHandler_      = std::make_unique<LoginHandler>(users, clientSessions);
    createRoomHandler_ = std::make_unique<CreateRoomHandler>(registry, bus, attachBroadcaster);
    joinRoomHandler_   = std::make_unique<JoinRoomHandler>(registry);
    findGameHandler_   = std::make_unique<FindGameHandler>(clientSessions, matchmaker, registry, bus,
                                                             attachBroadcaster, push, ioContext);

    router_ = std::make_unique<MessageRouter>();
    router_->registerHandler(MessageType::Login, *loginHandler_);
    router_->registerHandler(MessageType::CreateRoom, *createRoomHandler_);
    router_->registerHandler(MessageType::JoinRoom, *joinRoomHandler_);
    router_->registerHandler(MessageType::FindGame, *findGameHandler_);
}

MessageDispatcher::~MessageDispatcher() = default;

std::string MessageDispatcher::process(ConnectionHandle hdl, const std::string& text) {
    Chess::Color senderRole = registry_.roleOf(hdl);

    nlohmann::json reply;
    try {
        Message decoded = MessageCodec::decode(text);
        std::cout << "[server] received " << typeName(decoded.type) << " from " << roleName(senderRole)
                   << ": " << text << std::endl;

        if (IMessageHandler* handler = router_->find(decoded.type)) {
            reply = handler->handle(hdl, decoded.payload);
        } else {
            const std::string* roomName = registry_.roomOf(hdl);
            GameSession* gameSession = roomName ? registry_.room(*roomName) : nullptr;
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

    std::cout << "[server] replied: " << reply.dump() << std::endl;
    return reply.dump();
}
