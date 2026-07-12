#pragma once
#include "BoardMapper.h"
#include "../engine/GameEngine.h"
#include <memory>

class Controller {
private:
    std::shared_ptr<Position> selectedPos = nullptr;
    GameEngine& engine;

public:
    Controller(GameEngine& ge) : engine(ge) {}
    void         handleMouseClick(int x, int y);
    void         handleJump(int x, int y);
    void         handleWait(int ms);
    GameSnapshot getSnapshot() const;
};
