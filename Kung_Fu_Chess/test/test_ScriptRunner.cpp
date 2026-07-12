#include "../doctest.h"
#include "../texttests/ScriptRunner.h"
#include <sstream>

// דוגמת סעיף 13/14 במדויק: תנועת צריח לאורך זמן, מאומתת רק דרך print board
TEST_CASE("ScriptRunner - דוגמת המסמך: תנועת צריח מאומתת דרך print board") {
    std::stringstream script(
        "Board:\n"
        ". wR .\n"
        ". . .\n"
        ". . bK\n"
        "Commands:\n"
        "click 150 50\n"
        "click 150 250\n"
        "wait 2000\n"
        "print board\n"
        ". . .\n"
        ". . .\n"
        ". wR bK\n"
    );

    auto result = ScriptRunner::run(script);
    CHECK(result.passed);
}

// דוגמת איטרציה 5 במדויק: תנועה חלקית בזמן אמת - print board לפני ואחרי ההגעה
TEST_CASE("ScriptRunner - דוגמת המסמך: תנועה חלקית לפני ואחרי ההגעה") {
    std::stringstream script(
        "Board:\n"
        ". wR .\n"
        ". . .\n"
        ". . bK\n"
        "Commands:\n"
        "click 150 50\n"
        "click 150 250\n"
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

    auto result = ScriptRunner::run(script);
    CHECK(result.passed);
}

// דוגמת איטרציה 6 במדויק: אכילת מלך וסיום משחק - מהלך נוסף לא משנה את הלוח
TEST_CASE("ScriptRunner - דוגמת המסמך: אכילת מלך וסיום משחק") {
    std::stringstream script(
        "Board:\n"
        "wR . bK\n"
        ". . wN\n"
        ". . .\n"
        "Commands:\n"
        "click 50 50\n"
        "click 250 50\n"
        "wait 2000\n"
        "print board\n"
        ". . wR\n"
        ". . wN\n"
        ". . .\n"
        "click 250 150\n"
        "click 50 250\n"
        "wait 2000\n"
        "print board\n"
        ". . wR\n"
        ". . wN\n"
        ". . .\n"
    );

    auto result = ScriptRunner::run(script);
    CHECK(result.passed);
}

// ודאות ששיטת ההשוואה אכן תופסת אי-התאמה, ולא תמיד מחזירה true
TEST_CASE("ScriptRunner - אי-התאמה בין הצפוי לפלט בפועל נכשלת") {
    std::stringstream script(
        "Board:\n"
        ". wR .\n"
        ". . .\n"
        ". . bK\n"
        "Commands:\n"
        "print board\n"
        ". wR .\n"
        ". . .\n"
        ". . wK\n" // שגוי בכוונה: bK צריך להישאר bK
    );

    auto result = ScriptRunner::run(script);
    CHECK_FALSE(result.passed);
    CHECK(result.message != "");
}
