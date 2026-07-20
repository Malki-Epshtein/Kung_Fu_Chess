#pragma once
#include "../model/Position.h"
#include "../engine/GameSnapshot.h"
#include <optional>

enum class ClickOutcomeType { NoOp, Selected, SendMove, SendJump };

struct ClickOutcome {
    ClickOutcomeType type;
    Position from{};  // meaningful for SendMove
    Position to{};    // meaningful for SendMove
    Position pos{};   // meaningful for SendJump / Selected
};

// Pure two-click selection state machine: given the currently selected
// position (if any) and a new click position, decides what should happen -
// select this piece, resolve into a MOVE/JUMP to send, or do nothing. Knows
// nothing about pixels, networking, or the engine - both Controller (real
// clicks -> network messages) and ScriptRunner (scripted clicks -> direct
// CommandDispatcher calls) share this exact logic, so the two-click rules
// are defined in exactly one place instead of two.
class ClickResolver {
public:
    static ClickOutcome resolve(const GameSnapshot& snapshot,
                                 const std::optional<Position>& selectedPos,
                                 Position clickedPos);
};
