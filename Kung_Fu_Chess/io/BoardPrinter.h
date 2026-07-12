#pragma once
#include "../model/Board.h"
#include "../engine/GameSnapshot.h"
#include <iostream>

class BoardPrinter {
public:
    static void print(const Board& board, std::ostream& out);
    static void print(const GameSnapshot& snapshot, std::ostream& out);
};
