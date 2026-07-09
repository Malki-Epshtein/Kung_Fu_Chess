#pragma once
#include "../model/Board.h"
#include <iostream>

class BoardPrinter {
public:
    static void print(const Board& board, std::ostream& out);
};
