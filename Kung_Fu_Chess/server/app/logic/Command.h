#pragma once
#include "../../engine/MoveResult.h"

class GameEngine;

// One user action (a move or a jump), encapsulated as an object instead of
// a direct method call - the shape this project would need if it ever adds
// replay support (an "Extra Route" feature the spec already lists), without
// having to redesign how Controller talks to GameEngine to get there.
// execute() returns whatever the underlying GameEngine call returned
// (is_accepted/reason) - CommandDispatcher needs this to report a real
// rejection back to the client instead of always claiming success.
class Command {
public:
    virtual MoveResult execute(GameEngine& engine) = 0;
    virtual ~Command() = default;
};
