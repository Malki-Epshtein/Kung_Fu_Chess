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
#include "../logic/EloService.h"
#include "../../../shared/protocol/MessageCodec.h"
#include "../../../shared/model/Piece.h"
#include <iostream>

MessageDispatcher::MessageDispatcher(UserRepository& users, ClientSessionRegistry& clientSessions,
                                      SessionRegistry& registry, EventBus& bus, Matchmaker& matchmaker,
                                      BroadcasterManager& broadcasters, EloService& eloService, ConnectionSender push,
                                      asio::io_context& ioContext)
    : registry_(registry) {
    // Called once a new room actually exists (room creation or a Play
    // match) - attaches every per-room subscriber a fresh room needs:
    // NetworkBroadcaster's three client-facing topics, and EloService's
    // server-internal gameEndedTopic. CreateRoomHandler/FindGameHandler
    // just call this; neither needs to know what "attach" entails.
    BroadcasterManager* broadcastersPtr = &broadcasters;
    EloService* eloServicePtr = &eloService;
    auto attachBroadcaster = [broadcastersPtr, eloServicePtr, &bus](const std::string& name) {
        broadcastersPtr->attach(name);
        eloServicePtr->attach(bus, name);
    };

    loginHandler_      = std::make_unique<LoginHandler>(users, clientSessions);
    createRoomHandler_ = std::make_unique<CreateRoomHandler>(registry, clientSessions, bus, attachBroadcaster);
    joinRoomHandler_   = std::make_unique<JoinRoomHandler>(registry, clientSessions);
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
        std::cout << "[server] received " << MessageCodec::typeName(decoded.type) << " from " << roleName(senderRole)
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
        // Every reply tagged with the request's own type - the client
        // switches on this instead of guessing the message's shape from
        // which fields happen to be present (see MessageCodec::typeName).
        // Skipped if the handler already stamped its own type (only
        // FindGameHandler does this today, for the immediate-match case,
        // where the reply's real shape is GameFound, not FindGame - blindly
        // overwriting it here used to silently break that player's client,
        // which filters incoming messages on "type" == "GAME_FOUND" and
        // discards anything else).
        if (!reply.contains("type"))
            reply["type"] = MessageCodec::typeName(decoded.type);
    } catch (const std::exception& e) {
        std::cout << "[server] received non-protocol text: " << text
                   << " (decode failed: " << e.what() << ")" << std::endl;
        reply = { {"success", false}, {"message", std::string("decode failed: ") + e.what()} };
    }

    std::cout << "[server] replied: " << reply.dump() << std::endl;
    return reply.dump();
}
