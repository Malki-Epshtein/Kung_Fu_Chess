#pragma once
#include "../model/Position.h"

class BoardMapper {
public:
    // הצהרה על הפרונקציה בלבד
    static Position mapToPosition(int x, int y);
};
