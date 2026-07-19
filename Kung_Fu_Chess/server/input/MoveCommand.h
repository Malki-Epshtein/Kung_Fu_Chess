#pragma once
#include "Command.h"
#include "../../shared/model/Position.h"

class MoveCommand : public Command {
private:
    Position from;
    Position to;

public:
    MoveCommand(Position from, Position to) : from(from), to(to) {}
    void execute(GameEngine& engine) override;
};
