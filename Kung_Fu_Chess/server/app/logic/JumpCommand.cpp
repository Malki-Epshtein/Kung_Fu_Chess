#include "JumpCommand.h"
#include "../../engine/GameEngine.h"

void JumpCommand::execute(GameEngine& engine) {
    engine.requestJump(pos);
}
