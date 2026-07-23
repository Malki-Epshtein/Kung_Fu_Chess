#pragma once
#include "Command.h"
#include "../../../shared/model/Position.h"

class JumpCommand : public Command {
private:
    Position pos;

public:
    explicit JumpCommand(Position pos) : pos(pos) {}
    MoveResult execute(GameEngine& engine) override;
};
