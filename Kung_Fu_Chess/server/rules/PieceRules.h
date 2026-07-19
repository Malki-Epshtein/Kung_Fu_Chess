#pragma once
#include <vector>
#include "../../shared/model/Board.h"
#include "../../shared/model/Piece.h"

class PieceRules {
public:
    static std::vector<Position> legalDestinations(const Board& board, const Piece& piece, bool ignoreBlockers = false);
};
