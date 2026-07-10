#include "Controller.h"

void Controller::handleMouseClick(int x, int y) {
    int cols = board->getWidth();
    int rows = board->getHeight();

    int boardWidth  = cols * BoardMapper::CELL_SIZE;
    int boardHeight = rows * BoardMapper::CELL_SIZE;

    if (x < 0 || y < 0 || x >= boardWidth || y >= boardHeight) {
        selectedPos = nullptr;
        return;
    }

    Position clickedPos = BoardMapper::mapToPosition(x, y);

    if (selectedPos == nullptr) {
        if (!board->isCellEmpty(clickedPos))
            selectedPos = std::make_shared<Position>(clickedPos);
    }
    else {
        auto clickedPiece  = board->getPiece(clickedPos);
        auto selectedPiece = board->getPiece(*selectedPos);

        if (clickedPiece && selectedPiece &&
            clickedPiece->getColor() == selectedPiece->getColor() &&
            clickedPiece->getColor() != Chess::Color::None) {
            selectedPos = std::make_shared<Position>(clickedPos);
        }
        else {
            engine.requestMove(*selectedPos, clickedPos);
            selectedPos = nullptr;
        }
    }
}
