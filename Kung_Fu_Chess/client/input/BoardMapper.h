#pragma once
#include "../../shared/model/Position.h"
#include "../view/ViewConfig.h"

class BoardMapper {
public:
    // cellSize defaults to ViewConfig::CELL_SIZE so the other two call sites
    // (test_BoardMapper.cpp, texttests/ScriptRunner.cpp) keep working
    // unchanged - only Controller (the real GUI click path) passes the
    // live BoardScale::cellSize() explicitly.
    static Position mapToPosition(int x, int y, int cellSize = ViewConfig::CELL_SIZE);
};
