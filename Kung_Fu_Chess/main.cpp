#include <iostream>
#include <sstream>
#include "model/Board.h"
#include "BoardParser.h"
#include "BoardPrinter.h"

int main() {
    try {
        auto board = BoardParser::parseBoardOnly(std::cin);

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "print board") {
                BoardPrinter::print(*board, std::cout);
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
