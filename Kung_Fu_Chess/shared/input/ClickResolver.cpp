#include "ClickResolver.h"

namespace {
    const SnapshotPiece* findAt(const GameSnapshot& snapshot, Position pos) {
        for (const auto& p : snapshot.pieces)
            if (p.cell == pos)
                return &p;
        return nullptr;
    }
}

ClickOutcome ClickResolver::resolve(const GameSnapshot& snapshot,
                                     const std::optional<Position>& selectedPos,
                                     Position clickedPos) {
    if (!selectedPos.has_value()) {
        if (findAt(snapshot, clickedPos))
            return { ClickOutcomeType::Selected, {}, {}, clickedPos };
        return { ClickOutcomeType::NoOp, {}, {}, {} };
    }

    if (clickedPos == selectedPos.value())
        return { ClickOutcomeType::SendJump, {}, {}, clickedPos };

    const SnapshotPiece* clickedPiece  = findAt(snapshot, clickedPos);
    const SnapshotPiece* selectedPiece = findAt(snapshot, selectedPos.value());

    if (clickedPiece && selectedPiece &&
        clickedPiece->color == selectedPiece->color &&
        clickedPiece->color != Chess::Color::None &&
        selectedPiece->kind != Chess::Kind::Knight) {
        return { ClickOutcomeType::Selected, {}, {}, clickedPos };
    }

    return { ClickOutcomeType::SendMove, selectedPos.value(), clickedPos, {} };
}
