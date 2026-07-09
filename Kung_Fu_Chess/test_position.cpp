#include "doctest.h"
#include "model/Position.h"

void test_equality_same_values() {
    Position a = { 2, 3 };
    Position b = { 2, 3 };
    assert(a == b);
}

void test_inequality_different_row() {
    Position a = { 1, 3 };
    Position b = { 2, 3 };
    assert(a != b);
}

void test_inequality_different_col() {
    Position a = { 2, 1 };
    Position b = { 2, 3 };
    assert(a != b);
}

void test_stored_values() {
    Position p = { 4, 7 };
    assert(p.row == 4);
    assert(p.col == 7);
}


