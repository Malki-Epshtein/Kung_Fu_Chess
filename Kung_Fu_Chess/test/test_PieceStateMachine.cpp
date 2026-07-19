#include "../doctest.h"
#include "../shared/model/PieceStateMachine.h"

TEST_CASE("isLegalTransition - Idle יכול לעבור ל-Moving או Jump") {
    CHECK(isLegalTransition(Chess::State::Idle, Chess::State::Moving));
    CHECK(isLegalTransition(Chess::State::Idle, Chess::State::Jump));
}

TEST_CASE("isLegalTransition - Moving עובר ל-LongRest, Jump עובר ל-ShortRest") {
    CHECK(isLegalTransition(Chess::State::Moving, Chess::State::LongRest));
    CHECK(isLegalTransition(Chess::State::Jump, Chess::State::ShortRest));
}

TEST_CASE("isLegalTransition - מנוחה קצרה וארוכה חוזרות ל-Idle") {
    CHECK(isLegalTransition(Chess::State::ShortRest, Chess::State::Idle));
    CHECK(isLegalTransition(Chess::State::LongRest, Chess::State::Idle));
}

TEST_CASE("isLegalTransition - מעבר לא חוקי מוחזר כ-false") {
    CHECK_FALSE(isLegalTransition(Chess::State::Idle, Chess::State::ShortRest));
    CHECK_FALSE(isLegalTransition(Chess::State::Moving, Chess::State::Jump));
    CHECK_FALSE(isLegalTransition(Chess::State::ShortRest, Chess::State::Moving));
}

TEST_CASE("isLegalTransition - מעבר ל-Captured תמיד חוקי, מכל מצב") {
    CHECK(isLegalTransition(Chess::State::Idle, Chess::State::Captured));
    CHECK(isLegalTransition(Chess::State::Moving, Chess::State::Captured));
    CHECK(isLegalTransition(Chess::State::Jump, Chess::State::Captured));
    CHECK(isLegalTransition(Chess::State::ShortRest, Chess::State::Captured));
    CHECK(isLegalTransition(Chess::State::LongRest, Chess::State::Captured));
}

TEST_CASE("isBusyState - Moving ו-Jump עסוקים, שאר המצבים לא") {
    CHECK(isBusyState(Chess::State::Moving));
    CHECK(isBusyState(Chess::State::Jump));
    CHECK_FALSE(isBusyState(Chess::State::Idle));
    CHECK_FALSE(isBusyState(Chess::State::ShortRest));
    CHECK_FALSE(isBusyState(Chess::State::LongRest));
}

TEST_CASE("isRestingState - ShortRest ו-LongRest במנוחה, שאר המצבים לא") {
    CHECK(isRestingState(Chess::State::ShortRest));
    CHECK(isRestingState(Chess::State::LongRest));
    CHECK_FALSE(isRestingState(Chess::State::Idle));
    CHECK_FALSE(isRestingState(Chess::State::Moving));
    CHECK_FALSE(isRestingState(Chess::State::Jump));
}

TEST_CASE("stateFromString - ממיר כל מחרוזת ידועה ל-State הנכון") {
    CHECK(stateFromString("idle")       == Chess::State::Idle);
    CHECK(stateFromString("move")       == Chess::State::Moving);
    CHECK(stateFromString("jump")       == Chess::State::Jump);
    CHECK(stateFromString("short_rest") == Chess::State::ShortRest);
    CHECK(stateFromString("long_rest")  == Chess::State::LongRest);
}

TEST_CASE("stateFromString - מחרוזת לא מוכרת זורקת חריגה") {
    CHECK_THROWS_AS(stateFromString("unknown"), std::invalid_argument);
}

TEST_CASE("stateToFolderName - ממיר כל State למחרוזת התיקייה הנכונה") {
    CHECK(stateToFolderName(Chess::State::Idle)      == "idle");
    CHECK(stateToFolderName(Chess::State::Moving)    == "move");
    CHECK(stateToFolderName(Chess::State::Jump)      == "jump");
    CHECK(stateToFolderName(Chess::State::ShortRest) == "short_rest");
    CHECK(stateToFolderName(Chess::State::LongRest)  == "long_rest");
}

TEST_CASE("stateToFolderName - Captured אין לו תיקיית ספרייטים, זורק חריגה") {
    CHECK_THROWS_AS(stateToFolderName(Chess::State::Captured), std::invalid_argument);
}

TEST_CASE("stateFromString ו-stateToFolderName - מעגל סגור לכל מצב עם ספרייטים") {
    Chess::State states[] = {
        Chess::State::Idle, Chess::State::Moving, Chess::State::Jump,
        Chess::State::ShortRest, Chess::State::LongRest
    };
    for (auto state : states)
        CHECK(stateFromString(stateToFolderName(state)) == state);
}
