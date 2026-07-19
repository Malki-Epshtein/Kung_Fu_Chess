#pragma once
#include <iostream>

struct Position {
    int row;
    int col;

    bool operator==(const Position& other) const;
    bool operator!=(const Position& other) const;
};

inline std::ostream& operator<<(std::ostream& os, const Position& pos) {
    return os << "(" << pos.row << "," << pos.col << ")";
}
