#pragma once
#include <vector>
#include "../model/Board.h"
#include "../model/Piece.h"

class PieceRules {
public:
    static std::vector<Position> legalDestinations(const Board& board, const Piece& piece);
};
