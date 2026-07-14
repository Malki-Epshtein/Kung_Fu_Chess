#include "io/BoardParser.h"
#include "app/GraphicalApplication.h"
#include <iostream>
#include <sstream>

int main() {
    try {
        std::istringstream boardText(
            "Board:\n"
            "bR bN bB bQ bK bB bN bR\n"
            "bP bP bP bP bP bP bP bP\n"
            ". . . . . . . .\n"
            ". . . . . . . .\n"
            ". . . . . . . .\n"
            ". . . . . . . .\n"
            "wP wP wP wP wP wP wP wP\n"
            "wR wN wB wQ wK wB wN wR\n"
            "Commands:\n"
        );

        GraphicalApplication app(BoardParser::parseBoardOnly(boardText));
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
