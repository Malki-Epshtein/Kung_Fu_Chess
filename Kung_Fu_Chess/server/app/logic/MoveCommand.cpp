#include "MoveCommand.h"
#include "../../engine/GameEngine.h"

void MoveCommand::execute(GameEngine& engine) {
    engine.requestMove(from, to);
}
