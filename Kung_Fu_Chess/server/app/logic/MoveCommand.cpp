#include "MoveCommand.h"
#include "../../engine/GameEngine.h"

MoveResult MoveCommand::execute(GameEngine& engine) {
    return engine.requestMove(from, to);
}
