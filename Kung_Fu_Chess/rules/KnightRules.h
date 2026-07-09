#pragma once
#include <vector>
#include "../model/Board.h"
#include "../model/Piece.h"

class KnightRules {
public:
    static std::vector<Position> moves(const Board& board, const Piece& piece);
};
