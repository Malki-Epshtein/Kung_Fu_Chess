#pragma once
#include <websocketpp/common/connection_hdl.hpp>
#include "json.hpp"

// One incoming protocol message type, encapsulated as an object instead
// of a branch in a growing if/else chain - the same Command pattern this
// project already uses for game actions (see server/app/logic/Command.h and
// MoveCommand/JumpCommand), applied here to network message routing
// instead of GameEngine actions. Each concrete handler takes exactly the
// dependencies it needs (constructor injection), so it's unit-testable
// without a real socket - MessageRouter is what maps a MessageType to
// the right one of these.
class IMessageHandler {
public:
    using ConnectionHandle = websocketpp::connection_hdl;

    virtual ~IMessageHandler() = default;
    virtual nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) = 0;
};
