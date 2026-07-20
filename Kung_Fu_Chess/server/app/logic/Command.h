#pragma once

class GameEngine;

// One user action (a move or a jump), encapsulated as an object instead of
// a direct method call - the shape this project would need if it ever adds
// replay support (an "Extra Route" feature the spec already lists), without
// having to redesign how Controller talks to GameEngine to get there.
class Command {
public:
    virtual void execute(GameEngine& engine) = 0;
    virtual ~Command() = default;
};
