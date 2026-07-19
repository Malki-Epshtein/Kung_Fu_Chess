#pragma once
#include "../io/BoardParser.h"
#include "../io/BoardPrinter.h"
#include "../engine/GameEngine.h"
#include "../../client/input/Controller.h"
#include <iostream>

class Application {
private:
    std::istream& input;
    GameEngine    engine;
    Controller    controller;

public:
    Application(std::istream& input)
        : input(input), engine(BoardParser::parseBoardOnly(input)), controller(engine) {}

    void run();
};
