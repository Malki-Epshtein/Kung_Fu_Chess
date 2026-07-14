#include "../doctest.h"
#include "../view/AnimationConfig.h"
#include <string>

// Paths are resolved relative to this source file's own location (via
// __FILE__), not the process's current working directory, so these tests
// pass no matter where the test binary is actually launched from - see
// test_integration_scripts.cpp for the same pattern.
static std::string configPath(const std::string& relativePath) {
    std::string file = __FILE__; // .../test/test_AnimationConfig.cpp
    std::string dir = file.substr(0, file.find_last_of("/\\")); // .../test
    return dir + "/../view/img/" + relativePath;
}

TEST_CASE("loadAnimationConfig - קורא נכון קובץ config.json אמיתי (BB/move)") {
    auto config = loadAnimationConfig(configPath("BB/states/move/config.json"));

    CHECK(config.speed_m_per_sec == doctest::Approx(1.5));
    CHECK(config.next_state_when_finished == Chess::State::LongRest);
    CHECK(config.frames_per_sec == 12);
    CHECK(config.is_loop == true);
}

TEST_CASE("loadAnimationConfig - קובץ jump (BB) - is_loop false ומעבר ל-ShortRest") {
    auto config = loadAnimationConfig(configPath("BB/states/jump/config.json"));

    CHECK(config.speed_m_per_sec == doctest::Approx(3.0));
    CHECK(config.next_state_when_finished == Chess::State::ShortRest);
    CHECK(config.frames_per_sec == 8);
    CHECK(config.is_loop == false);
}

TEST_CASE("loadAnimationConfig - קובץ idle (BB) - מהירות אפס ולולאה") {
    auto config = loadAnimationConfig(configPath("BB/states/idle/config.json"));

    CHECK(config.speed_m_per_sec == doctest::Approx(0.0));
    CHECK(config.next_state_when_finished == Chess::State::Idle);
    CHECK(config.frames_per_sec == 6);
    CHECK(config.is_loop == true);
}

TEST_CASE("loadAnimationConfig - קובץ שלא קיים זורק חריגה") {
    CHECK_THROWS_AS(loadAnimationConfig(configPath("BB/states/idle/no_such_file.json")),
                     std::runtime_error);
}
