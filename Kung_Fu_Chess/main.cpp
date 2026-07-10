#define _CRT_SECURE_NO_WARNINGS
#include "io/BoardParser.h"
#include "io/BoardPrinter.h"
#include "model/GameState.h"
#include "engine/GameEngine.h"
#include "input/Controller.h"
#include <iostream>

int main() {
    try {
        auto board = BoardParser::parseBoardOnly(std::cin);
        GameState  state(board);
        GameEngine engine(state);
        Controller controller(board, engine);

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.find("click") == 0) {
                int x, y;
                sscanf(line.c_str(), "click %d %d", &x, &y);
                controller.handleMouseClick(x, y);
            }
            else if (line.find("wait") == 0) {
                int ms;
                sscanf(line.c_str(), "wait %d", &ms);
                engine.wait(ms);
            }
            else if (line == "print board") {
                BoardPrinter::print(*board, std::cout);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
