#pragma once
#include "BoardMapper.h"
#include "../model/Board.h"
#include "../model/Position.h"
#include "GameEngine.h"
#include <memory>

class Controller {
private:
    std::shared_ptr<Position> selectedPos = nullptr;
    std::shared_ptr<Board> board; 
    GameEngine& engine;           

public:
    Controller(std::shared_ptr<Board> b, GameEngine& ge) : board(b), engine(ge) {}
    void handleMouseClick(int x, int y);
};
