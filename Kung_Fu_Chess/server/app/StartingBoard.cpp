#include "StartingBoard.h"
#include "../io/BoardParser.h"
#include <sstream>

std::shared_ptr<Board> makeStartingBoard() {
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
    return BoardParser::parseBoardOnly(boardText);
}
