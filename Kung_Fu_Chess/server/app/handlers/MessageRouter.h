#pragma once
#include "IMessageHandler.h"
#include "../../../shared/protocol/Message.h"
#include <unordered_map>

// The registry half of the Command pattern (IMessageHandler is the
// command interface): maps each MessageType that has a dedicated handler
// to that handler. WsServer registers Login/CreateRoom/JoinRoom/FindGame
// here; MessageTypes with no registered handler (Move/Jump/...) stay on
// WsServer's existing CommandDispatcher path, since those need a
// GameEngine + sender role rather than a fixed set of constructor-injected
// dependencies.
class MessageRouter {
public:
    void registerHandler(MessageType type, IMessageHandler& handler);

    // nullptr if no handler is registered for this type.
    IMessageHandler* find(MessageType type);

private:
    std::unordered_map<MessageType, IMessageHandler*> handlers_;
};
