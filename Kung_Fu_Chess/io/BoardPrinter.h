#pragma once
#include "../model/GameState.h"
#include "../engine/GameSnapshot.h"
#include <iostream>

class BoardPrinter {
public:
    static void print(const GameState& state, std::ostream& out);
    static void print(const GameSnapshot& snapshot, std::ostream& out);
};
