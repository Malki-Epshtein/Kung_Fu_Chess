#pragma once
#include "BoardMapper.h"
#include "../view/BoardScale.h"
#include "../../shared/input/ClickResolver.h"
#include "../../shared/protocol/Message.h"
#include <functional>
#include <optional>
#include <string>

// Tracks client-side selection state and turns clicks into protocol
// messages handed to `send` - never touches game rules or an engine
// directly (that lives server-side only now). `send` is a generic sink
// (not a concrete WsClient), so this stays unit-testable without a socket.
class Controller {
public:
    using Sender = std::function<void(const std::string&)>;

    // boardScale is held by reference, not copied - it must reflect the
    // live current size (a player can resize mid-game, see BoardScale's own
    // comment), not whatever it was at construction time.
    Controller(const GameSnapshot& currentSnapshot, Sender send, const BoardScale& boardScale)
        : currentSnapshot(currentSnapshot), send(std::move(send)), boardScale(boardScale) {}

    ClickOutcomeType handleMouseClick(int x, int y);
    GameSnapshot getSnapshot() const;

    // Set once the player's seated color is known (GraphicalApplication, in
    // run(), once the room join/GameFound reply has it - unknown at
    // construction time). Left unset (nullopt) by default so existing
    // callers that never call this - unit tests, and any future non-network
    // use of Controller - keep the old "select any piece" behavior; only a
    // real seated player restricts selection to their own color (see
    // ClickResolver's own comment for why).
    void setMyColor(Chess::Color color) { myColor_ = color; }

private:
    std::optional<Position>     selectedPos;
    const GameSnapshot&         currentSnapshot;
    Sender                      send;
    const BoardScale&           boardScale;
    std::optional<Chess::Color> myColor_;
};
