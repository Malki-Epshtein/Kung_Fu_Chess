#include "Piece.h"
#include "PieceStateMachine.h"


Piece::Piece(int id, Chess::Color color, Chess::Kind kind, Position cell, Chess::State state)
    : id(id), color(color), kind(kind), cell(cell), state(state)
{

    if (this->kind == Chess::Kind::None) {
        this->state = Chess::State::Idle;
    }
}

bool Piece::transitionTo(Chess::State newState) {
    if (!isLegalTransition(state, newState))
        return false;

    state = newState;
    return true;
}