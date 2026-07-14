#pragma once
#include "../engine/GameEngine.h"
#include "../engine/ScoreObserver.h"
#include "../engine/MoveLogObserver.h"
#include "../input/Controller.h"
#include "../view/ImageView.h"
#include "../model/Board.h"
#include <memory>

class GraphicalApplication {
private:
    GameEngine      engine;
    Controller      controller;
    ImageView       view;
    ScoreObserver   scoreObserver;
    MoveLogObserver moveLogObserver;

public:
    GraphicalApplication(std::shared_ptr<Board> board)
        : engine(board, /*simultaneousMode=*/true), controller(engine) {
        engine.addCaptureObserver(&scoreObserver);
        engine.addMoveObserver(&moveLogObserver);
        view.setScoreObserver(&scoreObserver);
        view.setMoveLogObserver(&moveLogObserver);
    }

    void run();
};
