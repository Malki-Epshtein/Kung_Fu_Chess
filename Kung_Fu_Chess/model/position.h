#include <iostream>

struct Position {
    int row;
    int col;

    // אופרטור השוואה (כדי שנוכל להשתמש ב-==)
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }

    // אופרטור אי-שוואה (שימושי לעיתים קרובות)
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }

    // ייצוג קריא להדפסה (למשל לצורכי דיבאג)
    friend std::ostream& operator<<(std::ostream& os, const Position& pos) {
        return os << "(" << pos.row << "," << pos.col << ")";
    }
};