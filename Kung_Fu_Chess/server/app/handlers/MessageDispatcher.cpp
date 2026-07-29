#include "MessageDispatcher.h"
#include "IMessageHandler.h"
#include "LoginHandler.h"
#include "JoinRoomHandler.h"
#include "FindGameHandler.h"
#include "EnterRoomHandler.h"
#include "AuthHandler.h"
#include "AllocateRoomHandler.h"
#include "MessageRouter.h"
#include "../session/RoleName.h"
#include "../session/SessionRegistry.h"
#include "../session/GameSession.h"
#include "../networking/BroadcasterManager.h"
#include "../logic/CommandDispatcher.h"
#include "../logic/EloService.h"
#include "../logic/GameHistoryService.h"
#include "../logic/GameStateMirrorService.h"
#include "../../../shared/protocol/MessageCodec.h"
#include "../../../shared/model/Piece.h"
#include "../../../shared/bus/SubjectSafe.h"
#include "../../../shared/log/Log.h"

MessageDispatcher::MessageDispatcher(IUserRepository& users, ClientSessionRegistry& clientSessions,
                                      SessionRegistry& registry, EventBus& bus, MatchTicketRegistry& tickets,
                                      BroadcasterManager& broadcasters, EloService& eloService,
                                      GameHistoryService& gameHistoryService, GameStateMirrorService& gameStateMirror,
                                      ConnectionSender push,
                                      asio::io_context& ioContext, IClientSessionStore* sessionStore,
                                      INatsClient* nats, std::string shardAddress)
    : registry_(registry) {
    // Called once a new room actually exists (room creation or a Play
    // match) - attaches every per-room subscriber a fresh room needs:
    // NetworkBroadcaster's three client-facing topics, EloService's,
    // GameHistoryService's, and GameStateMirrorService's server-internal
    // topics. AllocateRoomHandler just calls this; it doesn't need to know
    // what "attach" entails.
    BroadcasterManager* broadcastersPtr = &broadcasters;
    EloService* eloServicePtr = &eloService;
    GameHistoryService* gameHistoryServicePtr = &gameHistoryService;
    GameStateMirrorService* gameStateMirrorPtr = &gameStateMirror;
    auto attachBroadcaster = [broadcastersPtr, eloServicePtr, gameHistoryServicePtr, gameStateMirrorPtr,
                               &bus](const std::string& name) {
        broadcastersPtr->attach(name);
        eloServicePtr->attach(bus, name);
        gameHistoryServicePtr->attach(bus, name);
        gameStateMirrorPtr->attach(bus, name);
    };

    loginHandler_      = std::make_unique<LoginHandler>(users, clientSessions);
    joinRoomHandler_   = std::make_unique<JoinRoomHandler>(registry, clientSessions);
    findGameHandler_   = std::make_unique<FindGameHandler>(clientSessions, tickets, nats, push, ioContext);

    router_ = std::make_unique<MessageRouter>();
    router_->registerHandler(MessageType::Login, *loginHandler_);
    router_->registerHandler(MessageType::JoinRoom, *joinRoomHandler_);
    router_->registerHandler(MessageType::FindGame, *findGameHandler_);

    if (sessionStore) {
        enterRoomHandler_ = std::make_unique<EnterRoomHandler>(registry, clientSessions, *sessionStore);
        router_->registerHandler(MessageType::EnterRoom, *enterRoomHandler_);

        authHandler_ = std::make_unique<AuthHandler>(clientSessions, *sessionStore);
        router_->registerHandler(MessageType::Auth, *authHandler_);
    }

    // The Game Allocator's counterpart to FindGameHandler's
    // matchmaking.assigned subscription above - answers "create/host this
    // matched room here" for whichever shard the Allocator picks. Only
    // meaningful with both NATS (to receive the request) and a real
    // SHARD_ADDRESS (so the Allocator can target this shard specifically);
    // the native Windows build and any Docker build missing either just
    // never registers a responder, and the Allocator's request to that
    // shard would simply time out (never attempted in practice, since the
    // Allocator only ever targets shards it heard a heartbeat from).
    if (nats && !shardAddress.empty()) {
        allocateRoomHandler_ = std::make_unique<AllocateRoomHandler>(registry, bus, attachBroadcaster, ioContext);
        AllocateRoomHandler* handlerPtr = allocateRoomHandler_.get();
        nats->subscribeRequest("shard." + subjectSafe(shardAddress) + ".allocate",
                                [handlerPtr](const nlohmann::json& request) { return handlerPtr->handle(request); });
    }
}

MessageDispatcher::~MessageDispatcher() = default;

std::string MessageDispatcher::process(ConnectionHandle hdl, const std::string& text) {
    Chess::Color senderRole = registry_.roleOf(hdl);

    nlohmann::json reply;
    try {
        Message decoded = MessageCodec::decode(text);
        spdlog::debug("received {} from {}: {}", MessageCodec::typeName(decoded.type), roleName(senderRole), text);

        if (IMessageHandler* handler = router_->find(decoded.type)) {//עשיתי ממשק וככה זה חוסך לי
            reply = handler->handle(hdl, decoded.payload);
        } else {
            const std::string* roomName = registry_.roomOf(hdl);
            GameSession* gameSession = roomName ? registry_.room(*roomName) : nullptr;
            if (!gameSession)
                throw std::runtime_error("connection is not in a room");

            DispatchResult result = CommandDispatcher::dispatch(decoded, gameSession->engine(), senderRole);
            spdlog::debug("dispatch {}: {}", result.success ? "OK" : "FAILED", result.message);

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
        spdlog::warn("received non-protocol text: {} (decode failed: {})", text, e.what());
        reply = { {"success", false}, {"message", std::string("decode failed: ") + e.what()} };
    }

    spdlog::debug("replied: {}", reply.dump());
    return reply.dump();
}
