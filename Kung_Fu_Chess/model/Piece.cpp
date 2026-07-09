#include "Piece.h" // קישור להגדרות במחלקה

// מימוש הבנאי (Constructor)
Piece::Piece(int id, Chess::Color color, Chess::Kind kind, Position cell, Chess::State state)
    : id(id), color(color), kind(kind), cell(cell), state(state) // רשימת אתחול (Initializer List)
{
    // בדיקה לוגית - אם הסוג הוא 'None' (תא ריק), נכפה מצב 'Idle'
    if (this->kind == Chess::Kind::None) {
        this->state = Chess::State::Idle;
    }
}