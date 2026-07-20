#pragma once
#include "BoardMapper.h"
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

    Controller(const GameSnapshot& currentSnapshot, Sender send)
        : currentSnapshot(currentSnapshot), send(std::move(send)) {}

    void         handleMouseClick(int x, int y);
    GameSnapshot getSnapshot() const;

private:
    std::optional<Position> selectedPos;
    const GameSnapshot&     currentSnapshot;
    Sender                  send;
};
