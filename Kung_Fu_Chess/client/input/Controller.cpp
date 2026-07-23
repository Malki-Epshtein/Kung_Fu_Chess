#include "Controller.h"
#include "../../shared/protocol/MessageCodec.h"

ClickOutcomeType Controller::handleMouseClick(int x, int y) {
    int cellSize = boardScale.cellSize();
    int boardWidthPx  = currentSnapshot.board_width  * cellSize;
    int boardHeightPx = currentSnapshot.board_height * cellSize;

    if (x < 0 || y < 0 || x >= boardWidthPx || y >= boardHeightPx) {
        selectedPos = std::nullopt;
        return ClickOutcomeType::NoOp;
    }

    Position clickedPos = BoardMapper::mapToPosition(x, y, cellSize);
    ClickOutcome outcome = ClickResolver::resolve(currentSnapshot, selectedPos, clickedPos, myColor_);

    switch (outcome.type) {
        case ClickOutcomeType::Selected:
            selectedPos = outcome.pos;
            break;

        case ClickOutcomeType::SendMove: {
            Message m;
            m.type = MessageType::Move;
            m.payload = { {"from", {{"row", outcome.from.row}, {"col", outcome.from.col}}},
                          {"to",   {{"row", outcome.to.row},   {"col", outcome.to.col}}} };
            send(MessageCodec::encode(m));
            selectedPos = std::nullopt;
            break;
        }

        case ClickOutcomeType::SendJump: {
            Message m;
            m.type = MessageType::Jump;
            m.payload = { {"pos", {{"row", outcome.pos.row}, {"col", outcome.pos.col}}} };
            send(MessageCodec::encode(m));
            selectedPos = std::nullopt;
            break;
        }

        case ClickOutcomeType::NoOp:
            break;
    }
    return outcome.type;
}

GameSnapshot Controller::getSnapshot() const {
    GameSnapshot snap = currentSnapshot;
    if (selectedPos.has_value()) {
        snap.has_selection = true;
        snap.selected_cell = selectedPos.value();
        // legalMoves intentionally left empty - rule evaluation is
        // server-side only now; highlighting deferred (2026-07-19 decision).
    }
    return snap;
}
