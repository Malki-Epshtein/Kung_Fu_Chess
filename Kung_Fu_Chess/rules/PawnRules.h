#pragma once
#include <vector>
#include "../model/Board.h"
#include "../model/Piece.h"

class PawnRules {
public:
    static std::vector<Position> moves(const Board& board, const Piece& piece);
};
