#include "Controller.h"
#include "../view/ViewConfig.h"
#include "../../shared/protocol/MessageCodec.h"

void Controller::handleMouseClick(int x, int y) {
    int boardWidthPx  = currentSnapshot.board_width  * ViewConfig::CELL_SIZE;
    int boardHeightPx = currentSnapshot.board_height * ViewConfig::CELL_SIZE;

    if (x < 0 || y < 0 || x >= boardWidthPx || y >= boardHeightPx) {
        selectedPos = std::nullopt;
        return;
    }

    Position clickedPos = BoardMapper::mapToPosition(x, y);
    ClickOutcome outcome = ClickResolver::resolve(currentSnapshot, selectedPos, clickedPos);

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
