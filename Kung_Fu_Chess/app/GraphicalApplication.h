#pragma once
#include "../engine/GameEngine.h"
#include "../input/Controller.h"
#include "../view/ImageView.h"
#include "../model/Board.h"
#include <memory>

class GraphicalApplication {
private:
    GameEngine engine;
    Controller controller;
    ImageView  view;

public:
    GraphicalApplication(std::shared_ptr<Board> board)
        : engine(board), controller(engine) {}

    void run();
};
