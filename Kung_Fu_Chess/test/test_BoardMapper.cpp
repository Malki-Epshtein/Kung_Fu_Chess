#include "../doctest.h"
#include "../input/BoardMapper.h"

// ==================== מיפוי עמודה (x) ====================

TEST_CASE("mapToPosition - x=0..99 ממופה לעמודה 0") {
    CHECK(BoardMapper::mapToPosition(0,  0).col == 0);
    CHECK(BoardMapper::mapToPosition(99, 0).col == 0);
}

TEST_CASE("mapToPosition - x=100..199 ממופה לעמודה 1") {
    CHECK(BoardMapper::mapToPosition(100, 0).col == 1);
    CHECK(BoardMapper::mapToPosition(199, 0).col == 1);
}

// ==================== מיפוי שורה (y) ====================

TEST_CASE("mapToPosition - y=0..99 ממופה לשורה 0") {
    CHECK(BoardMapper::mapToPosition(0, 0).row  == 0);
    CHECK(BoardMapper::mapToPosition(0, 99).row == 0);
}

TEST_CASE("mapToPosition - y=100..199 ממופה לשורה 1") {
    CHECK(BoardMapper::mapToPosition(0, 100).row == 1);
    CHECK(BoardMapper::mapToPosition(0, 199).row == 1);
}

// ==================== דוגמאות מהמסמך ====================

TEST_CASE("mapToPosition - click 50 50 -> row 0, col 0") {
    Position p = BoardMapper::mapToPosition(50, 50);
    CHECK(p.row == 0);
    CHECK(p.col == 0);
}

TEST_CASE("mapToPosition - click 150 50 -> row 0, col 1") {
    Position p = BoardMapper::mapToPosition(150, 50);
    CHECK(p.row == 0);
    CHECK(p.col == 1);
}

TEST_CASE("mapToPosition - click 50 150 -> row 1, col 0") {
    Position p = BoardMapper::mapToPosition(50, 150);
    CHECK(p.row == 1);
    CHECK(p.col == 0);
}
