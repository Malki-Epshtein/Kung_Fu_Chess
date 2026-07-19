#pragma once
#include <vector>
#include "../../shared/model/Board.h"
#include "../../shared/model/Piece.h"

class QueenRules {
public:
    static std::vector<Position> moves(const Board& board, const Piece& piece, bool ignoreBlockers = false);
};
