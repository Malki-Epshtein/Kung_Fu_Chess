#pragma once
#include <websocketpp/common/connection_hdl.hpp>
#include <asio/io_context.hpp>
#include <functional>
#include <memory>
#include <string>

class IUserRepository;
class ClientSessionRegistry;
class SessionRegistry;
class EventBus;
class Matchmaker;
class BroadcasterManager;
class EloService;
class IClientSessionStore;
class LoginHandler;
class CreateRoomHandler;
class JoinRoomHandler;
class FindGameHandler;
class EnterRoomHandler;
class AuthHandler;
class MessageRouter;

// The composition root for the network protocol layer: owns the concrete
// IMessageHandler implementations (Login/CreateRoom/JoinRoom/FindGame) and
// the MessageRouter that maps a MessageType to one of them - callers
// (WsServer) never see those concrete types, only this class's one public
// method (forward-declared + held by unique_ptr, so not even this header
// exposes them). MessageTypes with no registered handler (Move/Jump/...)
// fall back to CommandDispatcher, which needs a per-room GameEngine + the
// sender's role rather than a fixed set of constructor-injected
// dependencies.
class MessageDispatcher {
public:
    using ConnectionHandle = websocketpp::connection_hdl;
    using ConnectionSender = std::function<void(ConnectionHandle, const std::string&)>;

    // `sessionStore` may be null (no ENTER_ROOM support - the native
    // Windows build, or a Docker build with no REDIS_HOST set, never
    // registers that handler; a client using the old direct-WS LOGIN flow
    // never needs it anyway).
    MessageDispatcher(IUserRepository& users, ClientSessionRegistry& clientSessions,
                       SessionRegistry& registry, EventBus& bus, Matchmaker& matchmaker,
                       BroadcasterManager& broadcasters, EloService& eloService, ConnectionSender push,
                       asio::io_context& ioContext, IClientSessionStore* sessionStore = nullptr);
    ~MessageDispatcher();

    // Decodes `text`, routes it to a registered handler or the
    // CommandDispatcher fallback, and returns the reply as a JSON string.
    // The caller (WsServer) is responsible for the actual socket send,
    // since only it knows the right frame opcode to reply with.
    std::string process(ConnectionHandle hdl, const std::string& text);

private:
    SessionRegistry& registry_;

    std::unique_ptr<LoginHandler>      loginHandler_;
    std::unique_ptr<CreateRoomHandler> createRoomHandler_;
    std::unique_ptr<JoinRoomHandler>   joinRoomHandler_;
    std::unique_ptr<FindGameHandler>   findGameHandler_;
    std::unique_ptr<EnterRoomHandler>  enterRoomHandler_;
    std::unique_ptr<AuthHandler>       authHandler_;
    std::unique_ptr<MessageRouter>     router_;
};
