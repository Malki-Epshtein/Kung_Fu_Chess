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
//
// myColor (default nullopt = no restriction, ScriptRunner's/the test
// harness's behavior - a script isn't "playing as" a fixed color, it just
// acts as whichever color already owns the piece being clicked). Controller
// passes its own seated color once known, so a real player can never even
// select the opponent's piece - previously the initial-selection branch let
// you select ANY piece regardless of color, so selecting an opponent's piece
// then clicking it again produced a SendJump the server would reject as
// illegal, but only after jump.wav had already played optimistically
// client-side alongside illegal_move.wav.
class ClickResolver {
public:
    static ClickOutcome resolve(const GameSnapshot& snapshot,
                                 const std::optional<Position>& selectedPos,
                                 Position clickedPos,
                                 std::optional<Chess::Color> myColor = std::nullopt);
};
