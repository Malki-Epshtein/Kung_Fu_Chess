#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../doctest.h"
#include <sstream>
#include "../io/BoardParser.h"
#include "../io/BoardPrinter.h"

// TEST_CASE("valid rectangular board - print board") {
//     std::stringstream ss(
//         "Board:\n"
//         "wK . bQ\n"
//         ". wN .\n"
//         "bP . wR\n"
//         "Commands:\n"
//         "print board"
//     );
//     auto board = BoardParser::parseBoardOnly(ss);
//     std::ostringstream out;
//     BoardPrinter::print(*board, out);
//     CHECK(out.str() == "wK . bQ\n. wN .\nbP . wR\n");
// }

TEST_CASE("unknown token throws ERROR UNKNOWN_TOKEN") {
    std::stringstream ss(
        "Board:\n"
        "wK xZ\n"
        "Commands:\n"
    );
    CHECK_THROWS_WITH(BoardParser::parseBoardOnly(ss), "ERROR UNKNOWN_TOKEN");
}

TEST_CASE("row width mismatch throws ERROR ROW_WIDTH_MISMATCH") {
    std::stringstream ss(
        "Board:\n"
        "wK .\n"
        "bk\n"
        "Commands:\n"
    );
    CHECK_THROWS_WITH(BoardParser::parseBoardOnly(ss), "ERROR ROW_WIDTH_MISMATCH");
}

TEST_CASE("empty board throws ERROR EMPTY_BOARD") {
    std::stringstream ss(
        "Board:\n"
        "Commands:\n"
    );
    CHECK_THROWS_WITH(BoardParser::parseBoardOnly(ss), "ERROR EMPTY_BOARD");
}

TEST_CASE("board with single piece") {
    std::stringstream ss(
        "Board:\n"
        "wK\n"
        "Commands:\n"
    );
    auto board = BoardParser::parseBoardOnly(ss);
    std::ostringstream out;
    BoardPrinter::print(*board, out);
    CHECK(out.str() == "wK\n");
}
