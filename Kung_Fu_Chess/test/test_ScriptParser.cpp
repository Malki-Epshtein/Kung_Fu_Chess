#include "../doctest.h"
#include "../texttests/ScriptParser.h"
#include <sstream>

// ==================== click ====================

TEST_CASE("parse - click בודד מפורש נכון") {
    std::stringstream ss("click 150 50\n");
    auto commands = ScriptParser::parse(ss, 3);

    CHECK(commands.size() == 1);
    CHECK(commands[0].type == ScriptCommandType::Click);
    CHECK(commands[0].x == 150);
    CHECK(commands[0].y == 50);
}

// ==================== wait ====================

TEST_CASE("parse - wait בודד מפורש נכון") {
    std::stringstream ss("wait 1000\n");
    auto commands = ScriptParser::parse(ss, 3);

    CHECK(commands.size() == 1);
    CHECK(commands[0].type == ScriptCommandType::Wait);
    CHECK(commands[0].ms == 1000);
}

// ==================== print board ====================

TEST_CASE("parse - print board קולט את שורות הלוח הצפויות לפי גובה הלוח") {
    std::stringstream ss(
        "print board\n"
        ". . .\n"
        ". . .\n"
        ". wR bK\n"
    );
    auto commands = ScriptParser::parse(ss, 3);

    CHECK(commands.size() == 1);
    CHECK(commands[0].type == ScriptCommandType::PrintBoard);
    REQUIRE(commands[0].expectedBoard.size() == 3);
    CHECK(commands[0].expectedBoard[0] == ". . .");
    CHECK(commands[0].expectedBoard[1] == ". . .");
    CHECK(commands[0].expectedBoard[2] == ". wR bK");
}

// ==================== דוגמת המסמך המלאה ====================

TEST_CASE("parse - רצף פקודות מלא לפי דוגמת המסמך") {
    std::stringstream ss(
        "click 150 50\n"
        "click 150 250\n"
        "wait 2000\n"
        "print board\n"
        ". . .\n"
        ". . .\n"
        ". wR bK\n"
    );
    auto commands = ScriptParser::parse(ss, 3);

    REQUIRE(commands.size() == 4);

    CHECK(commands[0].type == ScriptCommandType::Click);
    CHECK(commands[0].x == 150);
    CHECK(commands[0].y == 50);

    CHECK(commands[1].type == ScriptCommandType::Click);
    CHECK(commands[1].x == 150);
    CHECK(commands[1].y == 250);

    CHECK(commands[2].type == ScriptCommandType::Wait);
    CHECK(commands[2].ms == 2000);

    CHECK(commands[3].type == ScriptCommandType::PrintBoard);
    REQUIRE(commands[3].expectedBoard.size() == 3);
    CHECK(commands[3].expectedBoard[2] == ". wR bK");
}

TEST_CASE("parse - שני בלוקים של print board נשמרים בנפרד") {
    std::stringstream ss(
        "wait 1000\n"
        "print board\n"
        ". wR .\n"
        ". . .\n"
        ". . bK\n"
        "wait 1000\n"
        "print board\n"
        ". . .\n"
        ". . .\n"
        ". wR bK\n"
    );
    auto commands = ScriptParser::parse(ss, 3);

    REQUIRE(commands.size() == 4);
    CHECK(commands[1].type == ScriptCommandType::PrintBoard);
    CHECK(commands[1].expectedBoard[0] == ". wR .");
    CHECK(commands[3].type == ScriptCommandType::PrintBoard);
    CHECK(commands[3].expectedBoard[2] == ". wR bK");
}

// ==================== שורות ריקות מתעלמים מהן ====================

TEST_CASE("parse - שורות ריקות בין פקודות מתעלמים מהן") {
    std::stringstream ss(
        "click 50 50\n"
        "\n"
        "wait 1000\n"
        "\n"
    );
    auto commands = ScriptParser::parse(ss, 3);

    REQUIRE(commands.size() == 2);
    CHECK(commands[0].type == ScriptCommandType::Click);
    CHECK(commands[1].type == ScriptCommandType::Wait);
}

// ==================== פקודה לא-מוכרת ====================

TEST_CASE("parse - פקודה לא מוכרת זורקת שגיאה") {
    std::stringstream ss("jump 50 50\n");
    CHECK_THROWS_WITH(ScriptParser::parse(ss, 3), "ERROR UNKNOWN_COMMAND");
}
