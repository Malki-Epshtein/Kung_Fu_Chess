#include "Piece.h" 


Piece::Piece(int id, Chess::Color color, Chess::Kind kind, Position cell, Chess::State state)
    : id(id), color(color), kind(kind), cell(cell), state(state) 
{
   
    if (this->kind == Chess::Kind::None) {
        this->state = Chess::State::Idle;
    }
}