#pragma once
#include "BoardMapper.h"
#include "../../server/engine/GameEngine.h"
#include <optional>

class Controller {
private:
    std::optional<Position> selectedPos;
    GameEngine& engine;

public:
    Controller(GameEngine& ge) : engine(ge) {}
    void         handleMouseClick(int x, int y);
    void         handleJump(int x, int y);
    void         handleWait(int ms);
    GameSnapshot getSnapshot() const;
};
