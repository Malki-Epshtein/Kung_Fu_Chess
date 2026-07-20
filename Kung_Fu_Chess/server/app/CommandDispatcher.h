#pragma once
#include "../../shared/protocol/Message.h"
#include "../../shared/model/Piece.h"
#include "../engine/GameEngine.h"
#include <string>

struct DispatchResult {
    bool        success;
    std::string message;
};

// Turns a decoded protocol Message into the matching Command and runs it
// against the engine. Has no idea a network exists - takes a Message, a
// GameEngine, and the sender's assigned color/role, nothing else - so it
// can be unit-tested without any socket. senderColor == Chess::Color::None
// means "spectator": never allowed to move or jump a piece. A sender may
// only move/jump a piece of their own color.
class CommandDispatcher {
public:
    static DispatchResult dispatch(const Message& message, GameEngine& engine, Chess::Color senderColor);
};
