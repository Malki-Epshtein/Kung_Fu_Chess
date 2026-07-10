#pragma once
#include <memory>
#include "Board.h"

struct GameState {
    std::shared_ptr<Board> board;
    bool                   game_over = false;

    GameState(std::shared_ptr<Board> board) : board(board) {}
};
