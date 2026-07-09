#include "io/BoardParser.h"
#include "io/BoardPrinter.h"
#include "logic/Controller.h"
#include <iostream>

int main() {
    // 1. שלב ה-Parsing
    auto board = BoardParser::parseBoardOnly(std::cin);
    Controller controller(board);

    // 2. שלב הפקודות
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.find("click") == 0) {
            int x, y;
            sscanf(line.c_str(), "click %d %d", &x, &y);
            controller.handleMouseClick(x, y);
        }
        else if (line == "print board") {
            BoardPrinter::print(*board, std::cout);
        }
    }
    return 0;
}
