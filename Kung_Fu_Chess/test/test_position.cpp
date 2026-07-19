#include "../doctest.h"
#include "../shared/model/Position.h"

TEST_CASE("Position - שני מיקומים עם אותה שורה ועמודה שווים") {
    Position a = { 2, 3 };
    Position b = { 2, 3 };
    CHECK(a == b);
}

TEST_CASE("Position - שני מיקומים עם שורה שונה אינם שווים") {
    Position a = { 1, 3 };
    Position b = { 2, 3 };
    CHECK(a != b);
}

TEST_CASE("Position - שני מיקומים עם עמודה שונה אינם שווים") {
    Position a = { 2, 1 };
    Position b = { 2, 3 };
    CHECK(a != b);
}

TEST_CASE("Position - השדות row ו-col נשמרים כמו שהוזנו") {
    Position p = { 4, 7 };
    CHECK(p.row == 4);
    CHECK(p.col == 7);
}
