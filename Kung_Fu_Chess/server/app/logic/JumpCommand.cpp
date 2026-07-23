#include "JumpCommand.h"
#include "../../engine/GameEngine.h"

MoveResult JumpCommand::execute(GameEngine& engine) {
    return engine.requestJump(pos);
}
