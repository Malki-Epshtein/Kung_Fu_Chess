#pragma once
#include "../../shared/model/GameState.h"
#include "../../shared/engine/GameSnapshot.h"
#include <iostream>

class BoardPrinter {
public:
    static void print(const GameState& state, std::ostream& out);
    static void print(const GameSnapshot& snapshot, std::ostream& out);
};
