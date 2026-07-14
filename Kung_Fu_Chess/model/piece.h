#pragma once
#include <vector>
#include "Position.h"

namespace Chess {
    enum class Color { White, Black, None };
    enum class Kind  { King, Queen, Rook, Bishop, Knight, Pawn, None };
    enum class State { Idle, Moving, Jump, ShortRest, LongRest, Captured};
}

class Piece {
protected:
    int id;
    Chess::Color color;
    Chess::Kind kind;
    Position cell;
    Chess::State state;

public:
    Piece(int id, Chess::Color color, Chess::Kind kind, Position cell, Chess::State state);

    virtual std::vector<Position> getValidMoves() const = 0;

    int          getId()    const { return id;    }
    Chess::Color getColor() const { return color; }
    Chess::Kind  getKind()  const { return kind;  }
    Position     getCell()  const { return cell;  }
    Chess::State getState() const { return state; }

    void promoteTo(Chess::Kind newKind) { kind = newKind; }
    void setCell(Position pos) { cell = pos; }
    bool transitionTo(Chess::State newState);

    virtual ~Piece() = default;
};

class EmptyPiece : public Piece {
public:
    EmptyPiece(Position cell)
        : Piece(0, Chess::Color::None, Chess::Kind::None, cell, Chess::State::Idle) {}
    std::vector<Position> getValidMoves() const override { return {}; }
};
