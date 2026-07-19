#pragma once
#include "MoveValidation.h"
#include "../../shared/model/Board.h"
#include "../../shared/model/Position.h"

class RuleEngine {
public:
    static MoveValidation validateMove(const Board& board, Position from, Position to, bool relaxedBlocking = false);
    static MoveValidation validateJump(const Board& board, Position pos);
};
