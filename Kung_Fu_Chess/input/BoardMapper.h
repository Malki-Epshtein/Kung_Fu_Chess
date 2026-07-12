#pragma once
#include "../model/Position.h"

class BoardMapper {
public:
    static const int CELL_SIZE = 100;
    // הצהרה על הפרונקציה בלבד
    static Position mapToPosition(int x, int y);
};
