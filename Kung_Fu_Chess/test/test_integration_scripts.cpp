#include "../doctest.h"
#include "../texttests/ScriptRunner.h"
#include <fstream>
#include <string>

// Runs each integration script (.kfc) from integration/scripts/ end-to-end
// through ScriptRunner - the C++ equivalent of the course spec's
// "test_text_scripts.py" (section 5). Paths are resolved relative to this
// source file's own location (via __FILE__), not the process's current
// working directory, so these tests pass no matter where the test binary
// is actually launched from.

static std::string scriptPath(const std::string& filename) {
    std::string file = __FILE__; // .../test/test_integration_scripts.cpp
    std::string dir = file.substr(0, file.find_last_of("/\\")); // .../test
    return dir + "/../integration/scripts/" + filename;
}

static ScriptResult runScriptFile(const std::string& filename) {
    std::string path = scriptPath(filename);
    std::ifstream file(path);
    REQUIRE_MESSAGE(file.is_open(), ("could not open " + path).c_str());
    return ScriptRunner::run(file);
}

TEST_CASE("integration script - 01_board_parsing.kfc") {
    auto result = runScriptFile("01_board_parsing.kfc");
    CHECK_MESSAGE(result.passed, result.message.c_str());
}

TEST_CASE("integration script - 02_click_to_move.kfc") {
    auto result = runScriptFile("02_click_to_move.kfc");
    CHECK_MESSAGE(result.passed, result.message.c_str());
}

TEST_CASE("integration script - 03_rook_moves.kfc") {
    auto result = runScriptFile("03_rook_moves.kfc");
    CHECK_MESSAGE(result.passed, result.message.c_str());
}

TEST_CASE("integration script - 04_invalid_moves.kfc") {
    auto result = runScriptFile("04_invalid_moves.kfc");
    CHECK_MESSAGE(result.passed, result.message.c_str());
}

TEST_CASE("integration script - 05_capture.kfc") {
    auto result = runScriptFile("05_capture.kfc");
    CHECK_MESSAGE(result.passed, result.message.c_str());
}

TEST_CASE("integration script - 06_game_over.kfc") {
    auto result = runScriptFile("06_game_over.kfc");
    CHECK_MESSAGE(result.passed, result.message.c_str());
}
